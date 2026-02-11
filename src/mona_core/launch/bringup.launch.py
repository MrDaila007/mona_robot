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

import os
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import IncludeLaunchDescription, SetEnvironmentVariable, ExecuteProcess
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node, LifecycleNode
import xacro


def generate_launch_description():
    # Переменные и пути
    pkg_mona_description = get_package_share_directory('mona_description')
    pkg_ros_gz_sim = get_package_share_directory('ros_gz_sim')

    # Пути к файлам
    xacro_file = os.path.join(pkg_mona_description, 'urdf',  'mona.urdf.xacro')
    world_file = os.path.join(pkg_mona_description, 'worlds', 'warehouse.sdf')
    rviz_config_file = os.path.join(pkg_mona_description, 'rviz',  'mona.rviz')

    # Настройка окружения
    resource_env = SetEnvironmentVariable(
        name='IGN_GAZEBO_RESOURCE_PATH',
        value=[os.path.dirname(pkg_mona_description), ':', pkg_mona_description]
    )

    # Обработка URDF
    robot_description_config = xacro.process_file(xacro_file)
    robot_description = {'robot_description': robot_description_config.toxml()}

    # ОСНОВНЫЕ НОДЫ
    use_sim_time = LaunchConfiguration('use_sim_time', default='true')

    # 1. Robot State Publisher
    node_robot_state_publisher = Node(
        package='robot_state_publisher',
        executable='robot_state_publisher',
        output='screen',
        parameters=[robot_description, {'use_sim_time': use_sim_time}]
    )

    # 2. Запуск Gazebo Sim
    gz_sim = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            os.path.join(pkg_ros_gz_sim, 'launch', 'gz_sim.launch.py')
        ),
        launch_arguments={'gz_args': ['-r ', world_file]}.items(),
    )

    # 3. Спавн робота
    spawn_entity = Node(
        package='ros_gz_sim',
        executable='create',
        arguments=[
            '-topic', 'robot_description',
            '-name', 'mona_robot',
            '-z', '0.01'
        ],
        output='screen'
    )

    # 4. МОСТ (Bridge) ROS <-> IGNITION
    bridge = Node(
        package='ros_gz_bridge',
        executable='parameter_bridge',
        arguments=[
            '/cmd_vel@geometry_msgs/msg/Twist]ignition.msgs.Twist',
            '/odom@nav_msgs/msg/Odometry[ignition.msgs.Odometry',
            '/tf@tf2_msgs/msg/TFMessage[ignition.msgs.Pose_V',
            '/joint_states@sensor_msgs/msg/JointState[ignition.msgs.Model',
            '/clock@rosgraph_msgs/msg/Clock[ignition.msgs.Clock',
            '/lidar_front/scan@sensor_msgs/msg/LaserScan[ignition.msgs.LaserScan',
            '/lidar_back/scan@sensor_msgs/msg/LaserScan[ignition.msgs.LaserScan',
            '/lidar_left/scan@sensor_msgs/msg/LaserScan[ignition.msgs.LaserScan',
            '/lidar_right/scan@sensor_msgs/msg/LaserScan[ignition.msgs.LaserScan',
            '/imu/data@sensor_msgs/msg/Imu@ignition.msgs.IMU'
        ],
        parameters=[{
            'use_sim_time': use_sim_time,
            'qos_overrides./imu/data.subscriber.reliability': 'best_effort'
        }],
        output='screen'
    )

    # 5. Rviz2
    node_rviz = Node(
        package='rviz2',
        executable='rviz2',
        name='rviz2',
        arguments=['-d', rviz_config_file],
        parameters=[{'use_sim_time': use_sim_time}],
        output='screen'
    )

    safety_node = LifecycleNode(
        package='mona_core',
        executable='safety_node',
        name='safety_node',
        namespace='',
        output='screen',
        parameters=[{'use_sim_time': use_sim_time}]
    )

    lidar_merger = LifecycleNode(
        package='mona_perception',
        executable='mona_perception_node',
        name='mona_lidar_merger',
        namespace='',
        output='screen',
        parameters=[{'use_sim_time': use_sim_time}]
    )

    lifecycle_manager = Node(
        package='mona_core',
        executable='lifecycle_manager.py',
        name='mona_lifecycle_manager',
        output='screen',
        parameters=[{'use_sim_time': use_sim_time}]
    )

    rosbag_record = ExecuteProcess(
        cmd=[
            'ros2', 'bag', 'record',
            '-o', 'mona_flight_data',
            '/cmd_vel',
            '/cmd_vel_teleop',
            '/cmd_vel_nav',
            '/robot_status',
            '/diagnostics'
        ],
        output='screen'
    )

    return LaunchDescription([
        resource_env,
        node_robot_state_publisher,
        gz_sim,
        spawn_entity,
        bridge,
        node_rviz,
        safety_node,
        lidar_merger,
        lifecycle_manager,
        rosbag_record
    ])
