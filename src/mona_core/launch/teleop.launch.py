#!/usr/bin/env python3

# Copyright 2026 vladubase
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

from launch import LaunchDescription
from launch_ros.actions import Node
from launch.substitutions import LaunchConfiguration

GAMEPAD = "DUALSENSE"  # PS5
# GAMEPAD = "DUALSHOCK"   # PS4
# GAMEPAD = "XBOX"

if GAMEPAD == "DUALSENSE":
    # --- AXES ---
    # Value range: -1.0 to 1.0
    AXIS_LEFT_Y = 0  # Left stick (Up/Down)
    AXIS_LEFT_X = 1  # Left stick (Left/Right)
    AXIS_L2 = 2  # Left trigger (Analog)
    AXIS_RIGHT_X = 3  # Right stick (Left/Right)
    AXIS_RIGHT_Y = 4  # Right stick (Up/Down)
    AXIS_R2 = 5  # Right trigger (Analog)
    AXIS_DPAD_X = 6  # D-Pad (Left/Right)
    AXIS_DPAD_Y = 7  # D-Pad (Up/Down)

    # --- BUTTONS ---
    # Values: 0 (released) or 1 (pressed)
    BTN_CROSS = 0  # Cross (X)
    BTN_CIRCLE = 1  # Circle (O)
    BTN_TRIANGLE = 2  # Triangle (△)
    BTN_SQUARE = 3  # Square (□)

    BTN_L1 = 4  # Left bumper (L1)
    BTN_R1 = 5  # Right bumper (R1)
    BTN_L2 = 6  # Left trigger (L2 - digital)
    BTN_R2 = 7  # Right trigger (R2 - digital)

    BTN_CREATE = 8  # Create/Share button (left of touchpad)
    BTN_OPTIONS = 9  # Options button (right of touchpad)
    BTN_PS = 10  # PlayStation button (Center logo)

    BTN_L3 = 11  # Left stick click (L3)
    BTN_R3 = 12  # Right stick click (R3)
    BTN_TOUCHPAD = 13  # Touchpad click


def generate_launch_description():
    use_sim_time = LaunchConfiguration("use_sim_time", default="true")

    return LaunchDescription(
        [
            Node(
                package="joy",
                executable="joy_node",
                name="joy_node",
                parameters=[
                    {
                        "dev": "/dev/input/js0",
                        "deadzone": 0.05,
                        "autorepeat_rate": 50.0,
                        "use_sim_time": use_sim_time,
                    }
                ],
            ),
            Node(
                package="teleop_twist_joy",
                executable="teleop_node",
                name="teleop_twist_joy_node",
                parameters=[
                    {
                        "axis_linear": {"x": AXIS_LEFT_X, "y": AXIS_LEFT_Y},
                        "axis_angular": {"yaw": AXIS_RIGHT_X},
                        "scale_linear": {"x": 0.5, "y": 0.5},
                        "scale_angular": {"yaw": 0.5},
                        "enable_button": BTN_L2,
                        "enable_turbo_button": BTN_R2,
                        "scale_linear_turbo": {"x": 1.0, "y": 1.0},
                        "scale_angular_turbo": {"yaw": 1.0},
                        "use_sim_time": use_sim_time,
                    }
                ],
                remappings=[("cmd_vel", "cmd_teleop")],
            ),
        ]
    )
