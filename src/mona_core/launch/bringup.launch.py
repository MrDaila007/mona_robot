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
from launch.actions import IncludeLaunchDescription, SetEnvironmentVariable, \
    ExecuteProcess, DeclareLaunchArgument
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.conditions import IfCondition
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node, ComposableNodeContainer
from launch_ros.descriptions import ComposableNode
import xacro


def generate_launch_description():
    # Переменные и пути
    pkg_mona_core = get_package_share_directory('mona_core')
    pkg_mona_description = get_package_share_directory('mona_description')
    pkg_mona_perception = get_package_share_directory('mona_perception')
    pkg_ros_gz_sim = get_package_share_directory('ros_gz_sim')

    # Пути к файлам
    xacro_file = os.path.join(pkg_mona_description, 'urdf', 'mona.urdf.xacro')
    world_file = os.path.join(pkg_mona_description, 'worlds', 'warehouse.sdf')
    rviz_config_file = os.path.join(pkg_mona_description, 'rviz', 'mona.rviz')
    ekf_config_path = os.path.join(
        get_package_share_directory('mona_core'),
        'configs',
        'ekf.yaml'
    )

    # Геймпад
    use_gamepad_arg = DeclareLaunchArgument(
        'use_gamepad',
        default_value='false',
        description='Set to true to enable DualSense teleop.'
    )
    use_gamepad = LaunchConfiguration('use_gamepad')

    # Настройка окружения
    resource_env = SetEnvironmentVariable(
        name='IGN_GAZEBO_RESOURCE_PATH',
        value=[os.path.dirname(pkg_mona_description), ':', pkg_mona_description]
    )

    # Принудительная подсветка логов ROS 2
    color_env = SetEnvironmentVariable(
        name='RCUTILS_COLORIZED_OUTPUT',
        value='1'
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
            '/hardware/motor_cmd@geometry_msgs/msg/Twist]ignition.msgs.Twist',
            '/odom@nav_msgs/msg/Odometry[ignition.msgs.Odometry',
            '/joint_states@sensor_msgs/msg/JointState[ignition.msgs.Model',
            '/clock@rosgraph_msgs/msg/Clock[ignition.msgs.Clock',
            '/lidar_front/scan@sensor_msgs/msg/LaserScan[ignition.msgs.LaserScan',
            '/lidar_back/scan@sensor_msgs/msg/LaserScan[ignition.msgs.LaserScan',
            '/lidar_left/scan@sensor_msgs/msg/LaserScan[ignition.msgs.LaserScan',
            '/lidar_right/scan@sensor_msgs/msg/LaserScan[ignition.msgs.LaserScan',
            '/imu/data@sensor_msgs/msg/Imu@ignition.msgs.IMU',
            '/camera/image_raw@sensor_msgs/msg/Image[ignition.msgs.Image',
            '/camera/camera_info@sensor_msgs/msg/CameraInfo[ignition.msgs.CameraInfo'
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

    # 6. Узел Foxglove Bridge для связи с веб-интерфейсом
    foxglove_bridge_node = Node(
        package='foxglove_bridge',
        executable='foxglove_bridge',
        name='foxglove_bridge',
        parameters=[{
            'port': 8765,
            'send_buffer_limit': 100000000,  # Увеличиваем буфер для передачи облаков точек (лидаров)
            'use_sim_time': use_sim_time
        }],
        output='screen'
    )

    # 7. ZERO-COPY COMPONENT CONTAINER
    # Запускаем мультипоточный контейнер, в который загружаем C++ плагины
    mona_core_container = ComposableNodeContainer(
        name='mona_core_container',
        namespace='',
        package='rclcpp_components',
        executable='component_container_mt',    # mt = Multi-Threaded
        output='screen',
        composable_node_descriptions=[
            # Модуль безопасности
            ComposableNode(
                package='mona_safety',
                plugin='mona_safety::SafetyNode',
                name='safety_node',
                parameters=[{'use_sim_time': use_sim_time}]
            ),
            # Модуль слияния лидаров
            ComposableNode(
                package='mona_perception',
                plugin='mona_perception::LidarMergerNode',
                name='mona_lidar_merger',
                parameters=[{'use_sim_time': use_sim_time}]
            ),
            # Модуль управления
            ComposableNode(
                package='mona_control',
                plugin='mona_control::TwistMuxNode',
                name='twist_mux_node',
                parameters=[{'use_sim_time': use_sim_time}]
            ),
        ]
    )

    # 8. FDIR Manager
    fdir_manager = Node(
        package='mona_core',
        executable='fdir_manager.py',
        name='mona_fdir_manager',
        output='screen',
        parameters=[{'use_sim_time': use_sim_time}]
    )

    # 9. Perception
    perception_launch = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            os.path.join(pkg_mona_perception, 'launch', 'perception.launch.py')
        ),
        launch_arguments={'use_sim_time': use_sim_time}.items(),
    )

    # 10. Инициализация узла robot_localization
    ekf_node = Node(
        package='robot_localization',
        executable='ekf_node',
        name='ekf_filter_node',
        output='screen',
        parameters=[ekf_config_path, {'use_sim_time': True}],
        remappings=[('/odometry/filtered', '/odom_filtered')]
    )

    # 11. Интеграция подсистемы SLAM
    slam_launch = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            os.path.join(pkg_mona_core, 'launch', 'slam.launch.py')
        ),
        launch_arguments={'use_sim_time': use_sim_time}.items(),
    )

    # 12. Автономная навигация Nav2
    navigation_launch = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            os.path.join(pkg_mona_core, 'launch', 'navigation.launch.py')
        ),
        launch_arguments={'use_sim_time': use_sim_time}.items(),
    )

    # 13. Ручное управление (Геймпад)
    teleop_launch = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            os.path.join(pkg_mona_core, 'launch', 'teleop.launch.py')
        ),
        condition=IfCondition(use_gamepad)      # Запустится ТОЛЬКО если use_gamepad:=true
    )

    # 14. Rosbag
    rosbag_record = ExecuteProcess(
        cmd=[
            'ros2', 'bag', 'record',
            # '-o', 'mona_flight_data',
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
        use_gamepad_arg,
        color_env,
        node_robot_state_publisher,
        gz_sim,
        spawn_entity,
        bridge,
        node_rviz,
        foxglove_bridge_node,
        mona_core_container,
        perception_launch,
        fdir_manager,
        rosbag_record,
        ekf_node,
        slam_launch,
        navigation_launch,
        teleop_launch
    ])
