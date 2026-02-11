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

#include <chrono>
#include <memory>
#include <mutex>
#include <string>

#include "rclcpp/rclcpp.hpp"
#include "rclcpp_lifecycle/lifecycle_node.hpp"
#include "geometry_msgs/msg/twist.hpp"
#include "std_msgs/msg/bool.hpp"
#include "std_msgs/msg/float32.hpp"
#include "std_msgs/msg/string.hpp"
#include "std_srvs/srv/trigger.hpp"
#include "lifecycle_msgs/msg/state.hpp"

using namespace std::chrono_literals;

enum class RobotState {
    UCFG,           // Unconfigured
    IDLE,           // Нет команд
    MANUAL,         // Управляет человек
    AUTONOMOUS,     // Управляет сервер
    DEGRADED,       // Проблемы с производительностью (медленный режим)
    EMERGENCY       // E-STOP активен
};

class SafetyNode : public rclcpp_lifecycle::LifecycleNode {
public:
    // Инициализируем имя узла
    explicit SafetyNode(const rclcpp::NodeOptions &options) : rclcpp_lifecycle::LifecycleNode(
                                                                  "safety_node", options) {}

    // --- LIFECYCLE: CONFIGURE --- //
    // Создаём паблишеры и таймеры, выделяем ресурсы
    rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn
    on_configure(const rclcpp_lifecycle::State &) override {
        // Состояние - несконфигурировано
        current_state_ = RobotState::UCFG;

        // Читаем параметры
        this->declare_parameter("command_timeout", 0.5);
        this->declare_parameter("manual_takeover_time", 2.0);
        cmd_timeout_    = this->get_parameter("command_timeout").as_double();
        manual_timeout_ = this->get_parameter("manual_takeover_time").as_double();

        // Создаем группы коллбэков (Reentrant - параллельное выполнение)
        auto callback_group = this->create_callback_group(rclcpp::CallbackGroupType::Reentrant);
        rclcpp::SubscriptionOptions sub_opts;
        sub_opts.callback_group = callback_group;

        // Паблишер (Итоговая скорость)
        cmd_vel_out_ = this->create_publisher<geometry_msgs::msg::Twist>("cmd_vel", 10);
        // Паблишер статуса (для LED ленты и логов)
        status_pub_ = this->create_publisher<std_msgs::msg::String>("robot_status", 10);

        // Подписки
        // 1. Телеоператор (Высокий приоритет)
        sub_teleop_ = this->create_subscription<geometry_msgs::msg::Twist>(
            "cmd_vel_teleop", 10,
            std::bind(&SafetyNode::teleop_callback, this, std::placeholders::_1),
            sub_opts
        );

        // 2. Навигация/Сервер (Средний приоритет)
        sub_nav_ = this->create_subscription<geometry_msgs::msg::Twist>(
            "cmd_vel_nav", 10,
            std::bind(&SafetyNode::nav_callback, this, std::placeholders::_1),
            sub_opts
        );

        // Сервис E-Stop
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
    rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn
    on_activate(const rclcpp_lifecycle::State &) override {
        // Активируем паблишер
        cmd_vel_out_->on_activate();
        status_pub_->on_activate();

        current_state_ = RobotState::IDLE;

        last_teleop_time_ = this->now();
        last_nav_time_    = this->now();
        e_stop_active_    = false;

        RCLCPP_INFO(get_logger(), "Safety Node ACTIVE. System is monitoring.");

        return rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn::SUCCESS;
    }

    // --- LIFECYCLE: DEACTIVATE --- //
    rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn
    on_deactivate(const rclcpp_lifecycle::State &) override {
        // Сначала отправляем стоп для безопасности!
        publish_stop();

        // Потом это, пока ещё работают!
        cmd_vel_out_->on_deactivate();
        status_pub_->on_deactivate();

        RCLCPP_WARN(get_logger(), "Safety Node DEACTIVATED. System may be unstable!");

        return rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn::SUCCESS;
    }

    // --- LIFECYCLE: CLEANUP ---
    rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn
    on_cleanup(const rclcpp_lifecycle::State &) override {
        // Очищаем ресурсы
        cmd_vel_out_.reset();
        status_pub_.reset();
        watchdog_timer_.reset();

        RCLCPP_INFO(get_logger(), "Safety Node Cleaned up.");

        return rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn::SUCCESS;
    }

    // --- LIFECYCLE: SHUTDOWN ---
    rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn
    on_shutdown(const rclcpp_lifecycle::State &state) override {
        RCLCPP_INFO(get_logger(), "Safety Node SHUTTING DOWN from %s", state.label().c_str());

        cmd_vel_out_.reset();

        return rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn::SUCCESS;
    }

    // TODO(vluadubase): on_error.

private:
    // --- CALLBACKS --- //
    void teleop_callback(const geometry_msgs::msg::Twist::SharedPtr msg) {
        if (e_stop_active_) {return;}

        std::lock_guard<std::mutex> lock(mux_mutex_);
        last_teleop_time_ = this->now();

        // Человек перехватывает управление
        current_state_ = RobotState::MANUAL;

        // Пропускаем команду сразу
        cmd_vel_out_->publish(*msg);
    }

    void nav_callback(const geometry_msgs::msg::Twist::SharedPtr msg) {
        if (e_stop_active_) {return;}

        std::lock_guard<std::mutex> lock(mux_mutex_);
        last_nav_time_ = this->now();

        // Проверяем, не управляет ли человек прямо сейчас
        double time_since_teleop = (this->now() - last_teleop_time_).seconds();
        if (time_since_teleop < manual_timeout_) {
            // Игнорируем сервер, человек рулит
            return;
        }

        // Если всё ок - едем автономно
        current_state_ = RobotState::AUTONOMOUS;

        cmd_vel_out_->publish(*msg);
    }

    void estop_callback(
        const std::shared_ptr<std_srvs::srv::Trigger::Request>,
        std::shared_ptr<std_srvs::srv::Trigger::Response> response) {
        e_stop_active_ = true;
        current_state_ = RobotState::EMERGENCY;

        publish_stop();

        response->success = true;
        response->message = "EMERGENCY STOP ACTIVATED!";

        RCLCPP_ERROR(get_logger(), "!!! EMERGENCY STOP TRIGGERED !!!");
    }

    // --- WATCHDOG LOGIC --- //
    void watchdog_routine() {
        // Активна ли нода вообще. Если нет - выходим.
        if (this->get_current_state().id() != lifecycle_msgs::msg::State::PRIMARY_STATE_ACTIVE) {
            return;
        }

        if (e_stop_active_) {
            publish_stop();
            publish_status("EMERGENCY");
            return;
        }

        std::lock_guard<std::mutex> lock(mux_mutex_);
        auto now = this->now();

        double time_since_teleop = (now - last_teleop_time_).seconds();
        double time_since_nav    = (now - last_nav_time_).seconds();

        // 1. Если человек бросил управление И сервер молчит
        if ((time_since_teleop > cmd_timeout_) && (time_since_nav > cmd_timeout_)) {
            current_state_ = RobotState::IDLE;

            // Safe stop
            publish_stop();
            publish_status("IDLE_NO_DATA");
            return;
        }
        // 2. Если человек управляет
        if (time_since_teleop < manual_timeout_) {
            publish_status("MANUAL_CONTROL");
            return;
        }
        // 3. Если сервер управляет
        publish_status("AUTONOMOUS");
    }

    void publish_stop() {
        geometry_msgs::msg::Twist stop_msg;

        stop_msg.linear.x  = 0.0;
        stop_msg.linear.y  = 0.0;
        stop_msg.angular.z = 0.0;

        // Проверяем активность перед отправкой
        // Хотя LifecyclePublisher делает это сам
        if (cmd_vel_out_->is_activated()) {
            cmd_vel_out_->publish(stop_msg);
        }
    }

    void publish_status(std::string status_text) {
        std_msgs::msg::String msg;

        msg.data = status_text;

        // Проверяем активность перед отправкой
        if (status_pub_->is_activated()) {
            status_pub_->publish(msg);
        }
    }

    // --- VARIABLES --- //
    rclcpp_lifecycle::LifecyclePublisher<geometry_msgs::msg::Twist>::SharedPtr cmd_vel_out_;
    rclcpp_lifecycle::LifecyclePublisher<std_msgs::msg::String>::SharedPtr status_pub_;
    rclcpp::Subscription<geometry_msgs::msg::Twist>::SharedPtr sub_teleop_;
    rclcpp::Subscription<geometry_msgs::msg::Twist>::SharedPtr sub_nav_;
    rclcpp::Service<std_srvs::srv::Trigger>::SharedPtr srv_estop_;
    rclcpp::TimerBase::SharedPtr watchdog_timer_;

    std::mutex mux_mutex_;
    rclcpp::Time last_teleop_time_;
    rclcpp::Time last_nav_time_;
    RobotState current_state_;
    bool e_stop_active_ = false;

    double cmd_timeout_;
    double manual_timeout_;
};

int main(int argc, char *argv[]) {
    rclcpp::init(argc, argv);

    // Мультипоточный Executor для обработки коллбэков
    rclcpp::executors::MultiThreadedExecutor executor;

    auto node = std::make_shared<SafetyNode>(rclcpp::NodeOptions());

    executor.add_node(node->get_node_base_interface());

    executor.spin();

    rclcpp::shutdown();
    return 0;
}
