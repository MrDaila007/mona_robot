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


#include "mona_control/twist_mux_node.hpp"

using namespace std::chrono_literals;

namespace mona_control
{
TwistMuxNode::TwistMuxNode(const rclcpp::NodeOptions &options)
    : rclcpp_lifecycle::LifecycleNode("twist_mux_node", options), last_teleop_time_(this->now()),
      last_nav_time_(this->now()), current_state_(MuxState::UCFG), manual_timeout_(2.0),
      cmd_timeout_(0.5), ema_alpha_(0.1) {
    this->declare_parameter("command_timeout", 0.5);
    this->declare_parameter("manual_takeover_time", 2.0);
    this->declare_parameter("ema_alpha", 0.1);
}

TwistMuxNode::~TwistMuxNode() {}

std::string TwistMuxNode::mux_state_to_string(MuxState state) {
    switch (state) {
        case MuxState::UCFG:       return "UNCONFIGURED";
        case MuxState::IDLE:       return "IDLE";
        case MuxState::MANUAL:     return "MANUAL";
        case MuxState::AUTONOMOUS: return "AUTONOMOUS";
        case MuxState::BLOCKED:    return "BLOCKED_BY_SAFETY";
        default:                   return "UNKNOWN";
    }
}

CallbackReturn TwistMuxNode::on_configure(const rclcpp_lifecycle::State &) {
    cmd_timeout_    = this->get_parameter("command_timeout").as_double();
    manual_timeout_ = this->get_parameter("manual_takeover_time").as_double();
    ema_alpha_      = this->get_parameter("ema_alpha").as_double();

    auto callback_group = this->create_callback_group(rclcpp::CallbackGroupType::Reentrant);
    rclcpp::SubscriptionOptions sub_opts;
    sub_opts.callback_group = callback_group;

    // --- ПАБЛИШЕРЫ --- //
    smooth_pub_ = this->create_publisher<geometry_msgs::msg::Twist>("/cmd_vel_smoothed", 10);
    status_pub_ = this->create_publisher<std_msgs::msg::String>("/mux_status", 10);

    // --- ПОДПИСКИ --- //
    // Телеоператор (Высокий приоритет)
    teleop_sub_ = this->create_subscription<geometry_msgs::msg::Twist>(
        "/cmd_teleop", 10, std::bind(
            &TwistMuxNode::teleop_callback, this,
            std::placeholders::_1), sub_opts);

    // Навигация/Сервер (Средний приоритет)
    nav_sub_ = this->create_subscription<geometry_msgs::msg::Twist>(
        "/cmd_nav", 10, std::bind(
            &TwistMuxNode::nav_callback, this,
            std::placeholders::_1), sub_opts);

    // Подписка на глобальный статус, чтобы знать об E-STOP
    robot_status_sub_ = this->create_subscription<std_msgs::msg::String>(
        "/robot_status", 10, std::bind(
            &TwistMuxNode::robot_status_callback, this,
            std::placeholders::_1), sub_opts);

    watchdog_timer_ = this->create_wall_timer(
        10ms, std::bind(&TwistMuxNode::control_loop, this), callback_group);

    RCLCPP_INFO(get_logger(), "TwistMux CONFIGURED. EMA Alpha: %.2f", ema_alpha_);
    return CallbackReturn::SUCCESS;
}

CallbackReturn TwistMuxNode::on_activate(const rclcpp_lifecycle::State &) {
    smooth_pub_->on_activate();
    status_pub_->on_activate();

    last_teleop_time_ = this->now();
    last_nav_time_    = this->now();
    current_state_    = MuxState::IDLE;

    RCLCPP_INFO(get_logger(), "TwistMux ACTIVE.");
    return CallbackReturn::SUCCESS;
}

CallbackReturn TwistMuxNode::on_deactivate(const rclcpp_lifecycle::State &) {
    smooth_pub_->on_deactivate();
    status_pub_->on_deactivate();

    return CallbackReturn::SUCCESS;
}

CallbackReturn TwistMuxNode::on_cleanup(const rclcpp_lifecycle::State &) {
    nav_sub_.reset();
    teleop_sub_.reset();
    robot_status_sub_.reset();
    smooth_pub_.reset();
    status_pub_.reset();
    watchdog_timer_.reset();
    nav_client_.reset();

    return CallbackReturn::SUCCESS;
}

CallbackReturn TwistMuxNode::on_shutdown(const rclcpp_lifecycle::State &) {
    return CallbackReturn::SUCCESS;
}

void TwistMuxNode::publish_status(std::string status_text) {
    std_msgs::msg::String msg;
    msg.data = status_text;

    if (status_pub_->is_activated()) {
        status_pub_->publish(msg);
    }
}

// Коллбэк для чтения состояния E-STOP
void TwistMuxNode::robot_status_callback(const std_msgs::msg::String::SharedPtr msg) {
    if (msg->data == "EMERGENCY" || msg->data == "PROTECTIVE_STOP") {
        is_safety_blocked_ = true;
    } else {
        is_safety_blocked_ = false;
    }
}

void TwistMuxNode::teleop_callback(const geometry_msgs::msg::Twist::SharedPtr msg) {
    std::lock_guard<std::mutex> lock(mux_mutex_);

    // Фильтр спама нулями
    bool is_zero_cmd =
        (std::hypot(msg->linear.x, msg->linear.y) < 0.001 && std::abs(msg->angular.z) < 0.001);

    if (is_zero_cmd) {
        // Запоминаем 0, чтобы робот плавно остановился, НО не продлеваем таймер блокировки Nav2
        last_teleop_msg_ = *msg;
    } else {
        last_teleop_time_ = this->now();

        // Перехват управления: отмена цели Nav2
        if (current_state_ == MuxState::AUTONOMOUS) {
            if (nav_client_ && nav_client_->action_server_is_ready()) {
                nav_client_->async_cancel_all_goals();
                RCLCPP_WARN(get_logger(), "MANUAL TAKEOVER: Canceling active Nav2 goal!");
            }
        }
        current_state_   = MuxState::MANUAL;
        last_teleop_msg_ = *msg;
    }
}

void TwistMuxNode::nav_callback(const geometry_msgs::msg::Twist::SharedPtr msg) {
    std::lock_guard<std::mutex> lock(mux_mutex_);
    last_nav_time_ = this->now();

    // Блокировка навигации, если недавно был активен джойстик
    if ((this->now() - last_teleop_time_).seconds() < manual_timeout_) {return;}

    current_state_ = MuxState::AUTONOMOUS;
    last_nav_msg_  = *msg;
}

void TwistMuxNode::control_loop() {
    if (!smooth_pub_->is_activated()) {return;}

    std::lock_guard<std::mutex> lock(mux_mutex_);
    auto   now = this->now();
    double time_since_teleop = (now - last_teleop_time_).seconds();
    double time_since_nav    = (now - last_nav_time_).seconds();

    // Внедрение состояния BLOCKED
    if (is_safety_blocked_) {
        current_state_ = MuxState::BLOCKED;
        target_vel_    = geometry_msgs::msg::Twist();   // Форсируем нули
    } else if ((time_since_teleop > cmd_timeout_) && (time_since_nav > cmd_timeout_)) {
        current_state_ = MuxState::IDLE;
        target_vel_    = geometry_msgs::msg::Twist();
    } else {
        if (current_state_ == MuxState::MANUAL) {
            target_vel_ = last_teleop_msg_;
        } else if (current_state_ == MuxState::AUTONOMOUS) {
            target_vel_ = last_nav_msg_;
        }
    }

    publish_status(mux_state_to_string(current_state_));

    // EMA Filter
    current_vel_.linear.x = ema_alpha_ * target_vel_.linear.x + (1.0 - ema_alpha_) *
        current_vel_.linear.x;
    current_vel_.linear.y = ema_alpha_ * target_vel_.linear.y + (1.0 - ema_alpha_) *
        current_vel_.linear.y;
    current_vel_.angular.z = ema_alpha_ * target_vel_.angular.z + (1.0 - ema_alpha_) *
        current_vel_.angular.z;

    smooth_pub_->publish(current_vel_);
}
}  // namespace mona_control

// Регистрация компонента в инфраструктуре ROS 2
#include "rclcpp_components/register_node_macro.hpp"
RCLCPP_COMPONENTS_REGISTER_NODE(mona_control::TwistMuxNode)
