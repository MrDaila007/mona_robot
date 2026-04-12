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


#include <algorithm>
#include <cmath>
#include <unordered_map>
#include "mona_safety/safety_node.hpp"

#define COLOR_BLUE  "\033[34m"
#define COLOR_RESET "\033[0m"

namespace mona_safety
{
SafetyNode::SafetyNode(const rclcpp::NodeOptions &options)
    : rclcpp_lifecycle::LifecycleNode("safety_node", options), diagnostic_updater_(this),
      current_state_(SafetyState::NORMAL) {
    this->declare_parameter("max_speed_normal", 1.0);
    this->declare_parameter("max_speed_degraded", 0.3);

    diagnostic_updater_.setHardwareID("mona_base_controller");
    diagnostic_updater_.add("Safety Node", this, &SafetyNode::produce_diagnostics);
}

SafetyNode::~SafetyNode() {
    set_hardware_contactors(false);
}

std::string SafetyNode::safety_state_to_string(SafetyState state) {
    switch (state) {
        case SafetyState::NORMAL:          return "NORMAL";
        case SafetyState::DEGRADED:        return "DEGRADED: " + last_mux_status_;
        case SafetyState::PROTECTIVE_STOP: return "PROTECTIVE_STOP";
        case SafetyState::EMERGENCY:       return "EMERGENCY";
        default:                           return "UNKNOWN";
    }
}

CallbackReturn SafetyNode::on_configure(const rclcpp_lifecycle::State &) {
    max_speed_normal_   = this->get_parameter("max_speed_normal").as_double();
    max_speed_degraded_ = this->get_parameter("max_speed_degraded").as_double();

    auto callback_group = this->create_callback_group(rclcpp::CallbackGroupType::Reentrant);
    rclcpp::SubscriptionOptions sub_opts;
    sub_opts.callback_group = callback_group;

    smooth_sub_ = create_subscription<geometry_msgs::msg::Twist>(
        "cmd_vel_smoothed", 10,
        std::bind(&SafetyNode::smoothed_vel_callback, this, std::placeholders::_1), sub_opts);

    health_sub_ = create_subscription<std_msgs::msg::String>(
        "system/health_state", 10,
        std::bind(&SafetyNode::health_state_callback, this, std::placeholders::_1), sub_opts);

    mux_status_sub_ = create_subscription<std_msgs::msg::String>(
        "mux_status", 10,
        std::bind(&SafetyNode::mux_status_callback, this, std::placeholders::_1), sub_opts);

    // Subscribe to odometry to monitor actual physical stops
    odom_sub_ = create_subscription<nav_msgs::msg::Odometry>(
        "odom", rclcpp::SystemDefaultsQoS(),
        std::bind(&SafetyNode::odom_callback, this, std::placeholders::_1));

    srv_estop_ = create_service<std_srvs::srv::Trigger>(
        "emergency_stop",
        std::bind(
            &SafetyNode::estop_callback, this, std::placeholders::_1,
            std::placeholders::_2));
    srv_estop_reset_ = create_service<std_srvs::srv::Trigger>(
        "emergency_stop_reset",
        std::bind(
            &SafetyNode::estop_reset_callback, this, std::placeholders::_1,
            std::placeholders::_2)
    );

    motor_pub_        = create_publisher<geometry_msgs::msg::Twist>("hardware/motor_cmd", 10);
    robot_status_pub_ = create_publisher<std_msgs::msg::String>("robot_status", 10);

    RCLCPP_INFO(get_logger(), "Safety Node CONFIGURED.");
    return CallbackReturn::SUCCESS;
}

CallbackReturn SafetyNode::on_activate(const rclcpp_lifecycle::State &) {
    is_processing_allowed_ = true;
    motor_pub_->on_activate();
    robot_status_pub_->on_activate();
    set_hardware_contactors(false);

    RCLCPP_INFO(get_logger(), "Safety Core ACTIVE. Contactors OFF. Waiting for FDIR clearance...");
    return CallbackReturn::SUCCESS;
}

CallbackReturn SafetyNode::on_deactivate(const rclcpp_lifecycle::State &) {
    is_processing_allowed_ = false;
    set_hardware_contactors(false);
    smooth_sub_.reset();
    health_sub_.reset();
    mux_status_sub_.reset();
    odom_sub_.reset();
    srv_estop_.reset();
    srv_estop_reset_.reset();
    motor_pub_.reset();
    robot_status_pub_.reset();

    RCLCPP_WARN(get_logger(), "Safety Node DEACTIVATED. System may be unstable!");
    return CallbackReturn::SUCCESS;
}

CallbackReturn SafetyNode::on_cleanup(const rclcpp_lifecycle::State &) {
    is_processing_allowed_ = false;
    set_hardware_contactors(false);
    smooth_sub_.reset();
    health_sub_.reset();
    mux_status_sub_.reset();
    odom_sub_.reset();
    srv_estop_.reset();
    srv_estop_reset_.reset();
    motor_pub_.reset();
    robot_status_pub_.reset();

    return CallbackReturn::SUCCESS;
}

CallbackReturn SafetyNode::on_shutdown(const rclcpp_lifecycle::State &) {
    is_processing_allowed_ = false;
    set_hardware_contactors(false);

    return CallbackReturn::SUCCESS;
}

CallbackReturn SafetyNode::on_error(const rclcpp_lifecycle::State &) {
    is_processing_allowed_ = false;
    set_hardware_contactors(false);

    return CallbackReturn::SUCCESS;
}

void SafetyNode::set_hardware_contactors(bool enable) {
    if (hardware_contactors_closed_ == enable) {return;}
    hardware_contactors_closed_ = enable;

    if (enable) {
        RCLCPP_INFO(
            get_logger(),
            COLOR_BLUE "HARDWARE: Contactors CLOSED. Motors POWERED." COLOR_RESET);
    } else {
        RCLCPP_WARN(get_logger(), "HARDWARE: Contactors OPENED. Motors DEAD.");
    }
}

void SafetyNode::publish_stop() {
    geometry_msgs::msg::Twist stop_msg;
    stop_msg.linear.x  = 0.0;
    stop_msg.linear.y  = 0.0;
    stop_msg.angular.z = 0.0;

    if (motor_pub_->is_activated()) {motor_pub_->publish(stop_msg);}
}

void SafetyNode::publish_global_status() {
    // Check if the lifecycle publisher is activated
    if (!robot_status_pub_->is_activated()) {return;}

    std_msgs::msg::String msg;

    // Strict status priority hierarchy utilizing the state converter
    if (e_stop_active_ || current_state_ == SafetyState::EMERGENCY) {
        msg.data = safety_state_to_string(SafetyState::EMERGENCY);
    } else if (current_state_ == SafetyState::PROTECTIVE_STOP) {
        msg.data = safety_state_to_string(SafetyState::PROTECTIVE_STOP);
    } else if (current_state_ == SafetyState::DEGRADED) {
        msg.data = safety_state_to_string(SafetyState::DEGRADED);
    } else {
        // If hardware is operational, broadcast the active control state (TwistMux)
        msg.data = last_mux_status_.empty() ? "UNKNOWN" : last_mux_status_;
    }

    robot_status_pub_->publish(msg);
}

void SafetyNode::smoothed_vel_callback(const geometry_msgs::msg::Twist::SharedPtr msg) {
    if (!is_processing_allowed_ || !motor_pub_->is_activated()) {return;}

    if (e_stop_active_ || current_state_ == SafetyState::PROTECTIVE_STOP ||
        current_state_ == SafetyState::EMERGENCY)
    {
        publish_stop();
        return;
    }

    geometry_msgs::msg::Twist clipped_msg = *msg;
    double current_limit =
        (current_state_ == SafetyState::DEGRADED) ? max_speed_degraded_ : max_speed_normal_;

    clipped_msg.linear.x = std::clamp(clipped_msg.linear.x, -current_limit, current_limit);
    clipped_msg.linear.y = std::clamp(clipped_msg.linear.y, -current_limit, current_limit);

    motor_pub_->publish(clipped_msg);
}

void SafetyNode::health_state_callback(const std_msgs::msg::String::SharedPtr msg) {
    if (!is_processing_allowed_) {return;}  // ZOMBIE NODE PROTECTION

    std::string fdir_state    = msg->data;
    bool        state_changed = (fdir_state != last_fdir_state_);
    last_fdir_state_          = fdir_state;

    // Static map for string-to-enum conversion
    static const std::unordered_map<std::string, FdirCommand> command_map = {
        {"SYSTEM_STARTUP", FdirCommand::SYSTEM_STARTUP},
        {"EMERGENCY", FdirCommand::EMERGENCY},
        {"PROTECTIVE_STOP", FdirCommand::PROTECTIVE_STOP},
        {"DEGRADED", FdirCommand::DEGRADED},
        {"SOFTWARE_OK", FdirCommand::SOFTWARE_OK}
    };

    // Parse the incoming command
    auto        it  = command_map.find(fdir_state);
    FdirCommand cmd = (it != command_map.end()) ? it->second : FdirCommand::UNKNOWN;

    // Execute state transition logic based on the parsed command
    switch (cmd) {
        case FdirCommand::SYSTEM_STARTUP:
            // 1. Initialization (Contactors OFF)
            current_state_ = SafetyState::PROTECTIVE_STOP;
            publish_stop();
            set_hardware_contactors(false);
            if (state_changed) {
                RCLCPP_INFO(
                    get_logger(),
                    COLOR_BLUE "System is initializing... Contactors OFF." COLOR_RESET);
            }
            break;

        case FdirCommand::EMERGENCY:
            // 2. Emergency (Latch E-Stop, contactors OFF)
            e_stop_active_ = true;
            current_state_ = SafetyState::EMERGENCY;
            publish_stop();
            set_hardware_contactors(false);
            if (state_changed) {
                RCLCPP_FATAL(get_logger(), "FDIR Commanded EMERGENCY! Contactors OFF.");
            }
            break;

        case FdirCommand::PROTECTIVE_STOP:
            // 3. Protective Stop (Power is ON, but motion is halted)
            current_state_ = SafetyState::PROTECTIVE_STOP;
            publish_stop();
            if (!e_stop_active_) {set_hardware_contactors(true);}
            if (state_changed) {
                RCLCPP_WARN(
                    get_logger(), "FDIR Commanded PROTECTIVE STOP. Waiting for primary sensors...");
            }
            break;

        case FdirCommand::DEGRADED:
            // 4. Degraded Mode (Operational, but with velocity restrictions)
            current_state_ = SafetyState::DEGRADED;
            if (!e_stop_active_) {
                set_hardware_contactors(true);
            }
            break;

        case FdirCommand::SOFTWARE_OK:
            // 5. Normal Operation
            if (!e_stop_active_) {
                current_state_ = SafetyState::NORMAL;
                set_hardware_contactors(true);
            } else {
                if (state_changed) {
                    RCLCPP_WARN(
                        get_logger(),
                        "FDIR Commanded SOFTWARE_OK, but E-STOP is latched! Ignoring.");
                }
            }
            break;

        case FdirCommand::UNKNOWN:
        default:
            // 6. UNKNOWN STATE (Global fallback - power off everything)
            set_hardware_contactors(false);
            if (state_changed) {
                RCLCPP_ERROR(
                    get_logger(), "UNKNOWN FDIR STATE: '%s'. Safety shutdown triggered!",
                    fdir_state.c_str());
            }
            break;
    }
}

void SafetyNode::mux_status_callback(const std_msgs::msg::String::SharedPtr msg) {
    if (!is_processing_allowed_) {return;}
    last_mux_status_ = msg->data;
    publish_global_status();
}

void SafetyNode::odom_callback(const nav_msgs::msg::Odometry::SharedPtr msg) {
    if (!is_processing_allowed_) {return;}

    double current_speed = std::hypot(msg->twist.twist.linear.x, msg->twist.twist.linear.y);

    if (current_state_ == SafetyState::PROTECTIVE_STOP && current_speed > 0.05) {
        e_stop_active_ = true;
        current_state_ = SafetyState::EMERGENCY;
        set_hardware_contactors(false);
        publish_global_status();

        RCLCPP_FATAL(
            get_logger(), "ESCALATION: Robot moving during Soft Stop! Hard E-STOP triggered!");
    }
}

void SafetyNode::estop_callback(
    const std::shared_ptr<std_srvs::srv::Trigger::Request>,
    std::shared_ptr<std_srvs::srv::Trigger::Response> response) {
    if (!is_processing_allowed_) {return;}

    e_stop_active_ = true;
    current_state_ = SafetyState::EMERGENCY;
    publish_stop();
    set_hardware_contactors(false);
    publish_global_status();

    response->success = true;
    response->message = "EMERGENCY STOP (LEVEL 2) ACTIVATED!";
    RCLCPP_FATAL(get_logger(), "!!! LEVEL 2 E-STOP: MOTORS DE-ENERGIZED !!!");
}

void SafetyNode::estop_reset_callback(
    const std::shared_ptr<std_srvs::srv::Trigger::Request>,
    std::shared_ptr<std_srvs::srv::Trigger::Response> response) {
    if (current_state_ == SafetyState::EMERGENCY || e_stop_active_) {
        current_state_ = SafetyState::NORMAL;  // Reset safety state
        e_stop_active_ = false;
        set_hardware_contactors(true);

        publish_global_status();

        RCLCPP_INFO(this->get_logger(), "Safety system RESET. Ready for operation.");
        response->success = true;
        response->message = "System recovered from EMERGENCY. Manual control enabled.";
    } else {
        response->success = false;
        response->message = "Reset ignored: System is not in EMERGENCY state.";
    }
}

void SafetyNode::produce_diagnostics(diagnostic_updater::DiagnosticStatusWrapper &stat) {
    if (hardware_contactors_closed_) {
        stat.summary(
            diagnostic_msgs::msg::DiagnosticStatus::OK,
            "System Normal. Contactors Closed.");
    } else {
        stat.summary(
            diagnostic_msgs::msg::DiagnosticStatus::WARN,
            "System Offline. Contactors Opened.");
    }
}
}  // namespace mona_safety

// Register the component within the ROS 2 infrastructure
#include "rclcpp_components/register_node_macro.hpp"
RCLCPP_COMPONENTS_REGISTER_NODE(mona_safety::SafetyNode)
