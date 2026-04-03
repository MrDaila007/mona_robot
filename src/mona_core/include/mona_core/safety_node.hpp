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


#ifndef MONA_CORE__SAFETY_NODE_HPP_
#define MONA_CORE__SAFETY_NODE_HPP_

#include <chrono>
#include <memory>
#include <mutex>
#include <string>
#include <algorithm>

#include "rclcpp/rclcpp.hpp"
#include "rclcpp_lifecycle/lifecycle_node.hpp"
#include "geometry_msgs/msg/twist.hpp"
#include "std_msgs/msg/bool.hpp"
#include "std_msgs/msg/float32.hpp"
#include "std_msgs/msg/string.hpp"
#include "std_srvs/srv/trigger.hpp"
#include "lifecycle_msgs/msg/state.hpp"
#include "nav_msgs/msg/odometry.hpp"

// ANSI Коды для раскраски логов
#define COLOR_BLUE  "\033[34m"
#define COLOR_RESET "\033[0m"

enum class RobotState {
    UCFG,               // Unconfigured
    IDLE,               // Нет команд
    MANUAL,             // Управляет человек
    AUTONOMOUS,         // Управляет сервер
    DEGRADED,           // Проблемы с производительностью (медленный режим)
    PROTECTIVE_STOP,    // Стоим, ждём ребута PRIMARY Узла
    EMERGENCY           // E-STOP активен
};


namespace mona_core
{
// Алиас для удобства работы с жизненным циклом
using CallbackReturn = rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn;

class SafetyNode : public rclcpp_lifecycle::LifecycleNode {
public:
    explicit SafetyNode(const rclcpp::NodeOptions &options);
    virtual ~SafetyNode();

    // Переопределение методов жизненного цикла (Lifecycle)
    CallbackReturn on_configure(const rclcpp_lifecycle::State &state) override;
    CallbackReturn on_activate(const rclcpp_lifecycle::State &state) override;
    CallbackReturn on_deactivate(const rclcpp_lifecycle::State &state) override;
    CallbackReturn on_cleanup(const rclcpp_lifecycle::State &state) override;
    CallbackReturn on_shutdown(const rclcpp_lifecycle::State &state) override;
    CallbackReturn on_error(const rclcpp_lifecycle::State &previous_state) override;

private:
    // --- ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ --- //
    void set_hardware_contactors(bool enable);
    void publish_stop();
    void publish_status(std::string status_text);
    void publish_velocity_clipped(const geometry_msgs::msg::Twist &msg);

    // --- CALLBACKS --- //
    void health_state_callback(const std_msgs::msg::String::SharedPtr msg);
    void teleop_callback(const geometry_msgs::msg::Twist::SharedPtr msg);
    void nav_callback(const geometry_msgs::msg::Twist::SharedPtr msg);
    void odom_callback(const nav_msgs::msg::Odometry::SharedPtr msg);
    void estop_callback(
        const std::shared_ptr<std_srvs::srv::Trigger::Request>,
        std::shared_ptr<std_srvs::srv::Trigger::Response> response);
    void watchdog_routine();

    // --- VARIABLES --- //
    rclcpp_lifecycle::LifecyclePublisher<geometry_msgs::msg::Twist>::SharedPtr cmd_vel_out_;
    rclcpp_lifecycle::LifecyclePublisher<std_msgs::msg::String>::SharedPtr status_pub_;
    rclcpp_lifecycle::LifecyclePublisher<std_msgs::msg::Bool>::SharedPtr contactor_pub_;

    rclcpp::Subscription<geometry_msgs::msg::Twist>::SharedPtr sub_teleop_;
    rclcpp::Subscription<geometry_msgs::msg::Twist>::SharedPtr sub_nav_;
    rclcpp::Subscription<nav_msgs::msg::Odometry>::SharedPtr sub_odom_;
    rclcpp::Subscription<std_msgs::msg::String>::SharedPtr sub_health_state_;
    rclcpp::Service<std_srvs::srv::Trigger>::SharedPtr srv_estop_;
    rclcpp::TimerBase::SharedPtr watchdog_timer_;

    std::mutex mux_mutex_;
    rclcpp::Time last_teleop_time_;
    rclcpp::Time last_nav_time_;

    // Переменные для сглаживания (EMA-фильтр)
    geometry_msgs::msg::Twist target_cmd_vel_;
    geometry_msgs::msg::Twist smoothed_cmd_vel_;
    double teleop_smoothing_factor_;

    RobotState current_state_;
    std::string last_fdir_state_ = "";

    std::atomic<bool> e_stop_active_{false};
    std::atomic<bool> is_processing_allowed_{false};    // ЗАЩИТА ОТ ЗОМБИ КОЛЛБЭКОВ

    bool contactors_enabled_ = false;

    double cmd_timeout_;
    double manual_timeout_;
    double max_speed_normal_;
    double max_speed_degraded_;
};
}   // namespace mona_core

#endif  // MONA_CORE__SAFETY_NODE_HPP_
