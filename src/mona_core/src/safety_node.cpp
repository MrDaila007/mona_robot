// Copyright 2026 vladubase
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.


#include "mona_core/safety_node.hpp"

using namespace std::chrono_literals;

namespace mona_core
{
SafetyNode::SafetyNode(const rclcpp::NodeOptions &options) : rclcpp_lifecycle::LifecycleNode(
                                                                 "safety_node", options) {}
SafetyNode::~SafetyNode() {
    // Гарантируем отключение даже если объект уничтожается
    set_hardware_contactors(false);
}

// --- LIFECYCLE: CONFIGURE --- //
// Создаём паблишеры и таймеры, выделяем ресурсы
CallbackReturn SafetyNode::on_configure(const rclcpp_lifecycle::State &) {
    // Состояние - несконфигурировано
    current_state_ = RobotState::UCFG;

    // Читаем параметры
    this->declare_parameter("command_timeout", 0.5);
    this->declare_parameter("manual_takeover_time", 5.0);
    this->declare_parameter("max_speed_normal", 1.0);
    this->declare_parameter("max_speed_degraded", 0.3);
    this->declare_parameter("acceleration_alpha", 0.05);

    cmd_timeout_             = this->get_parameter("command_timeout").as_double();
    manual_timeout_          = this->get_parameter("manual_takeover_time").as_double();
    max_speed_normal_        = this->get_parameter("max_speed_normal").as_double();
    max_speed_degraded_      = this->get_parameter("max_speed_degraded").as_double();
    teleop_smoothing_factor_ = this->get_parameter("acceleration_alpha").as_double();

    // Создаем группы коллбэков (Reentrant - параллельное выполнение)
    auto callback_group = this->create_callback_group(rclcpp::CallbackGroupType::Reentrant);
    rclcpp::SubscriptionOptions sub_opts;
    sub_opts.callback_group = callback_group;

    // --- ПАБЛИШЕРЫ --- ///
    cmd_vel_out_ = this->create_publisher<geometry_msgs::msg::Twist>("cmd_vel", 10);
    status_pub_  = this->create_publisher<std_msgs::msg::String>("robot_status", 10);

    rclcpp::QoS qos_profile(1);
    qos_profile.reliable();
    qos_profile.transient_local();
    contactor_pub_ = this->create_publisher<std_msgs::msg::Bool>(
        "hardware/contactor_cmd",
        qos_profile);

    // --- ПОДПИСКИ --- //
    // Телеоператор (Высокий приоритет)
    sub_teleop_ = this->create_subscription<geometry_msgs::msg::Twist>(
        "cmd_teleop", 10,
        std::bind(&SafetyNode::teleop_callback, this, std::placeholders::_1),
        sub_opts
    );

    // Навигация/Сервер (Средний приоритет)
    sub_nav_ = this->create_subscription<geometry_msgs::msg::Twist>(
        "cmd_nav", 10,
        std::bind(&SafetyNode::nav_callback, this, std::placeholders::_1),
        sub_opts
    );

    // Подписка на одометрию для контроля фактической остановки
    sub_odom_ = this->create_subscription<nav_msgs::msg::Odometry>(
        "odom", rclcpp::SystemDefaultsQoS(),
        std::bind(&SafetyNode::odom_callback, this, std::placeholders::_1)
    );

    // Приказы от FDIR менеджера
    sub_health_state_ = this->create_subscription<std_msgs::msg::String>(
        "/system/health_state", 10,
        std::bind(&SafetyNode::health_state_callback, this, std::placeholders::_1),
        sub_opts
    );

    // --- СЕРВИСЫ --- //
    // E-Stop
    srv_estop_ = this->create_service<std_srvs::srv::Trigger>(
        "emergency_stop",
        std::bind(
            &SafetyNode::estop_callback, this, std::placeholders::_1,
            std::placeholders::_2)
    );

    // Главный Watchdog таймер (100 Гц - очень быстро)
    watchdog_timer_ = this->create_wall_timer(
        10ms,
        std::bind(&SafetyNode::watchdog_routine, this),
        callback_group
    );

    RCLCPP_INFO(get_logger(), "Safety Node CONFIGURED. Watchdog timeout: %.2f s", cmd_timeout_);
    return rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn::SUCCESS;
}

// --- LIFECYCLE: ACTIVATE --- //
CallbackReturn SafetyNode::on_activate(const rclcpp_lifecycle::State &) {
    is_processing_allowed_ = true;  // Разрешаем обработку сообщений

    // Активируем паблишер
    cmd_vel_out_->on_activate();
    status_pub_->on_activate();
    contactor_pub_->on_activate();

    current_state_    = RobotState::IDLE;
    last_teleop_time_ = this->now();
    last_nav_time_    = this->now();

    // Сбрасываем цели и текущую скорость
    target_cmd_vel_   = geometry_msgs::msg::Twist();
    smoothed_cmd_vel_ = geometry_msgs::msg::Twist();

    if (!e_stop_active_) {set_hardware_contactors(true);}

    RCLCPP_INFO(get_logger(), "Safety Node ACTIVE.");
    return rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn::SUCCESS;
}

// --- LIFECYCLE: DEACTIVATE --- //
CallbackReturn SafetyNode::on_deactivate(const rclcpp_lifecycle::State &) {
    // Сначала отправляем стоп для безопасности
    is_processing_allowed_ = false;     // Блокируем ЗОМБИ-коллбэки
    set_hardware_contactors(false);
    publish_stop();

    // Деактивируем
    cmd_vel_out_->on_deactivate();
    status_pub_->on_deactivate();
    contactor_pub_->on_deactivate();

    RCLCPP_WARN(get_logger(), "Safety Node DEACTIVATED. System may be unstable!");
    return rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn::SUCCESS;
}

// --- LIFECYCLE: CLEANUP --- //
CallbackReturn SafetyNode::on_cleanup(const rclcpp_lifecycle::State &) {
    // Защита от тихой смерти
    is_processing_allowed_ = false;
    set_hardware_contactors(false);

    // Очищаем ресурсы
    cmd_vel_out_.reset();
    status_pub_.reset();
    contactor_pub_.reset();
    watchdog_timer_.reset();

    RCLCPP_INFO(get_logger(), "Safety Node CLEANED UP.");
    return rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn::SUCCESS;
}

// --- LIFECYCLE: SHUTDOWN --- //
CallbackReturn SafetyNode::on_shutdown(const rclcpp_lifecycle::State &state) {
    is_processing_allowed_ = false;     // Блокируем всё
    // Защита от тихой смерти
    set_hardware_contactors(false);
    publish_stop();
    cmd_vel_out_.reset();

    RCLCPP_INFO(get_logger(), "Safety Node SHUTTING DOWN from %s", state.label().c_str());
    return rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn::SUCCESS;
}

// --- LIFECYCLE: ON ERROR --- //
CallbackReturn SafetyNode::on_error(const rclcpp_lifecycle::State &) {
    // Защита от тихой смерти
    is_processing_allowed_ = false;
    set_hardware_contactors(false);
    publish_stop();

    return rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn::SUCCESS;
}

// --- ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ --- //
void SafetyNode::set_hardware_contactors(bool enable) {
    if (contactors_enabled_ == enable) {return;}
    contactors_enabled_ = enable;

    std_msgs::msg::Bool msg;
    msg.data = enable;
    if (contactor_pub_->is_activated()) {contactor_pub_->publish(msg);}

    if (enable) {
        RCLCPP_INFO(
            get_logger(),
            COLOR_BLUE "HARDWARE: Contactors CLOSED. Motors POWERED." COLOR_RESET);
    } else {RCLCPP_WARN(get_logger(), "HARDWARE: Contactors OPENED. Motors DEAD.");}
}

void SafetyNode::publish_stop() {
    geometry_msgs::msg::Twist stop_msg;
    stop_msg.linear.x = 0.0; stop_msg.linear.y = 0.0; stop_msg.angular.z = 0.0;

    if (cmd_vel_out_->is_activated()) {cmd_vel_out_->publish(stop_msg);}
}

void SafetyNode::publish_status(std::string status_text) {
    std_msgs::msg::String msg;
    msg.data = status_text;

    if (status_pub_->is_activated()) {status_pub_->publish(msg);}
}

void SafetyNode::publish_velocity_clipped(const geometry_msgs::msg::Twist &msg) {
    if ((current_state_ == RobotState::PROTECTIVE_STOP) ||
        (current_state_ == RobotState::EMERGENCY)) {return;}

    geometry_msgs::msg::Twist clipped_msg = msg;
    double current_limit =
        (current_state_ == RobotState::DEGRADED) ? max_speed_degraded_ : max_speed_normal_;

    clipped_msg.linear.x = std::clamp(clipped_msg.linear.x, -current_limit, current_limit);
    clipped_msg.linear.y = std::clamp(clipped_msg.linear.y, -current_limit, current_limit);

    if (cmd_vel_out_->is_activated()) {cmd_vel_out_->publish(clipped_msg);}
}

// --- CALLBACKS --- //
void SafetyNode::health_state_callback(const std_msgs::msg::String::SharedPtr msg) {
    if (!is_processing_allowed_) {
        return;                           // ЗАЩИТА ОТ ЗОМБИ
    }
    // Логируем сообщения только при смене статуса
    std::string fdir_state    = msg->data;
    bool        state_changed = (fdir_state != last_fdir_state_);
    last_fdir_state_          = fdir_state;

    if (fdir_state == "SYSTEM_STARTUP") {
        current_state_ = RobotState::PROTECTIVE_STOP;
        publish_stop();
        set_hardware_contactors(false);
        if (state_changed) {
            RCLCPP_INFO(
                get_logger(),
                COLOR_BLUE "System is initializing... Contactors OFF." COLOR_RESET);
        }
    } else if (fdir_state == "EMERGENCY") {
        e_stop_active_ = true;
        publish_stop();
        set_hardware_contactors(false);
        if (state_changed) {
            RCLCPP_FATAL(get_logger(), "FDIR Commanded EMERGENCY! Contactors OFF.");
        }
    } else if (fdir_state == "PROTECTIVE_STOP") {
        current_state_ = RobotState::PROTECTIVE_STOP;
        publish_stop();
        set_hardware_contactors(true);
        if (state_changed) {
            RCLCPP_WARN(
                get_logger(), "FDIR Commanded PROTECTIVE STOP. Waiting for primary sensors...");
        }
    } else if (fdir_state == "DEGRADED") {
        current_state_ = RobotState::DEGRADED;
        set_hardware_contactors(true);
    } else if (fdir_state == "NORMAL" && !e_stop_active_) {
        // Если мы возвращаемся из состояния ошибки, сбрасываем статус в IDLE.
        // При наличии свежих команд телеоператора или навигатора состояние
        // автоматически переключится в MANUAL или AUTONOMOUS в watchdog_routine.
        if (current_state_ == RobotState::PROTECTIVE_STOP ||
            current_state_ == RobotState::DEGRADED)
        {
            current_state_ = RobotState::IDLE;
        }
        set_hardware_contactors(true);
    }
}

void SafetyNode::teleop_callback(const geometry_msgs::msg::Twist::SharedPtr msg) {
    if (!is_processing_allowed_ || e_stop_active_) {return;}

    std::lock_guard<std::mutex> lock(mux_mutex_);
    last_teleop_time_ = this->now();
    current_state_    = RobotState::MANUAL;

    target_cmd_vel_ = *msg;
}

void SafetyNode::nav_callback(const geometry_msgs::msg::Twist::SharedPtr msg) {
    if (!is_processing_allowed_ || e_stop_active_) {return;}

    std::lock_guard<std::mutex> lock(mux_mutex_);
    last_nav_time_ = this->now();

    if ((this->now() - last_teleop_time_).seconds() < manual_timeout_) {return;}

    if (current_state_ != RobotState::PROTECTIVE_STOP) {
        current_state_ = RobotState::AUTONOMOUS;

        target_cmd_vel_ = *msg;
    }
}

void SafetyNode::odom_callback(const nav_msgs::msg::Odometry::SharedPtr msg) {
    if (!is_processing_allowed_) {return;}

    double current_speed = std::hypot(msg->twist.twist.linear.x, msg->twist.twist.linear.y);

    if (current_state_ == RobotState::PROTECTIVE_STOP && current_speed > 0.05) {
        e_stop_active_ = true;
        current_state_ = RobotState::EMERGENCY;
        set_hardware_contactors(false);
        RCLCPP_FATAL(
            get_logger(), "ESCALATION: Robot moving during Soft Stop! Hard E-STOP triggered!");
    }
}

void SafetyNode::estop_callback(
    const std::shared_ptr<std_srvs::srv::Trigger::Request>,
    std::shared_ptr<std_srvs::srv::Trigger::Response> response) {
    if (!is_processing_allowed_) {return;}

    e_stop_active_ = true;
    current_state_ = RobotState::EMERGENCY;
    publish_stop();
    set_hardware_contactors(false);
    response->success = true;
    response->message = "EMERGENCY STOP (LEVEL 2) ACTIVATED!";
    RCLCPP_FATAL(get_logger(), "!!! LEVEL 2 E-STOP: MOTORS DE-ENERGIZED !!!");
}

void SafetyNode::watchdog_routine() {
    if (!is_processing_allowed_) {return;}
    if (this->get_current_state().id() != lifecycle_msgs::msg::State::PRIMARY_STATE_ACTIVE) {
        return;
    }

    if (e_stop_active_) {
        publish_stop();
        set_hardware_contactors(false);
        publish_status("EMERGENCY");
        return;
    }

    std::lock_guard<std::mutex> lock(mux_mutex_);
    auto   now = this->now();
    double time_since_teleop = (now - last_teleop_time_).seconds();
    double time_since_nav    = (now - last_nav_time_).seconds();

    if ((time_since_teleop > cmd_timeout_) && (time_since_nav > cmd_timeout_)) {
        if (current_state_ != RobotState::PROTECTIVE_STOP) {
            current_state_ = RobotState::IDLE;
        }
        // Если таймаут, плавно тормозим (цель = 0)
        target_cmd_vel_ = geometry_msgs::msg::Twist();
        publish_status("NO_CMD_DATA");
    } else {
        publish_status("ACTIVE");
    }

    // --- УМНАЯ МАРШРУТИЗАЦИЯ И СГЛАЖИВАНИЕ (100 Гц) ---
    if (current_state_ == RobotState::AUTONOMOUS) {
        // Автопилот (Nav2) имеет свой точный контроллер (TEB/MPPI/DWB). Пропускаем напрямую.
        smoothed_cmd_vel_ = target_cmd_vel_;
    } else {
        // Для ручного управления (MANUAL) или при мягкой остановке (IDLE) применяем EMA-фильтр
        smoothed_cmd_vel_.linear.x += teleop_smoothing_factor_ *
            (target_cmd_vel_.linear.x - smoothed_cmd_vel_.linear.x);
        smoothed_cmd_vel_.linear.y += teleop_smoothing_factor_ *
            (target_cmd_vel_.linear.y - smoothed_cmd_vel_.linear.y);
        smoothed_cmd_vel_.angular.z += teleop_smoothing_factor_ *
            (target_cmd_vel_.angular.z - smoothed_cmd_vel_.angular.z);

        // Чтобы робот не "полз" бесконечно из-за асимптоты фильтра, обнуляем микро-скорости
        if (std::abs(smoothed_cmd_vel_.linear.x) < 0.001) {smoothed_cmd_vel_.linear.x = 0.0;}
        if (std::abs(smoothed_cmd_vel_.linear.y) < 0.001) {smoothed_cmd_vel_.linear.y = 0.0;}
        if (std::abs(smoothed_cmd_vel_.angular.z) < 0.001) {smoothed_cmd_vel_.angular.z = 0.0;}
    }

    publish_velocity_clipped(smoothed_cmd_vel_);
}
}   // namespace mona_core


// Регистрация компонента в инфраструктуре ROS 2
#include "rclcpp_components/register_node_macro.hpp"
RCLCPP_COMPONENTS_REGISTER_NODE(mona_core::SafetyNode)
