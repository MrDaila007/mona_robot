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

GAMEPAD = "DUALSENSE"
# GAMEPAD = "XBOX"

if GAMEPAD == "DUALSENSE":
    # --- ОСИ (AXES) ---
    # Диапазон значений: от -1.0 до 1.0
    AXIS_LEFT_Y = 0  # Левый стик (Вверх/Вниз)
    AXIS_LEFT_X = 1  # Левый стик (Влево/Вправо)
    AXIS_L2 = 2  # Левый курок (Аналоговый)
    AXIS_RIGHT_X = 3  # Правый стик (Влево/Вправо)
    AXIS_RIGHT_Y = 4  # Правый стик (Вверх/Вниз)
    AXIS_R2 = 5  # Правый курок (Аналоговый)
    AXIS_DPAD_X = 6  # Крестовина (Влево/Вправо)
    AXIS_DPAD_Y = 7  # Крестовина (Вверх/Вниз)

    # --- КНОПКИ (BUTTONS) ---
    # Значения: 0 (отпущена) или 1 (нажата)
    BTN_CROSS = 0  # Крестик (X)
    BTN_CIRCLE = 1  # Кружок (O)
    BTN_TRIANGLE = 2  # Треугольник (△)
    BTN_SQUARE = 3  # Квадрат (□)

    BTN_L1 = 4  # Левый верхний бампер (L1)
    BTN_R1 = 5  # Правый верхний бампер (R1)
    BTN_L2 = 6  # Левый нижний курок (L2 - как цифровая кнопка)
    BTN_R2 = 7  # Правый нижний курок (R2 - как цифровая кнопка)

    BTN_CREATE = 8  # Кнопка Create/Share (слева от тачпада)
    BTN_OPTIONS = 9  # Кнопка Options (справа от тачпада)
    BTN_PS = 10  # Кнопка PlayStation (Логотип по центру)

    BTN_L3 = 11  # Нажатие на левый стик
    BTN_R3 = 12  # Нажатие на правый стик
    BTN_TOUCHPAD = 13  # Нажатие на саму сенсорную панель


def generate_launch_description():
    return LaunchDescription([
        Node(
            package='joy',
            executable='joy_node',
            name='joy_node',
            parameters=[{
                'dev': '/dev/input/js0',
                'deadzone': 0.05,
                'autorepeat_rate': 50.0,
            }]
        ),

        Node(
            package='teleop_twist_joy',
            executable='teleop_node',
            name='teleop_twist_joy_node',
            parameters=[{
                'axis_linear':          {'x': AXIS_LEFT_X, 'y': AXIS_LEFT_Y},
                'axis_angular':         {'yaw': AXIS_RIGHT_X},

                'scale_linear':         {'x': 0.5, 'y': 0.5},
                'scale_angular':        {'yaw': 0.5},

                'enable_button':        BTN_L2,

                'enable_turbo_button':  BTN_R2,
                'scale_linear_turbo':   {'x': 1.0, 'y': 1.0},
                'scale_angular_turbo':  {'yaw': 1.0}
            }],
            remappings=[
                ('/cmd_vel', '/cmd_teleop')
            ]
        )
    ])
