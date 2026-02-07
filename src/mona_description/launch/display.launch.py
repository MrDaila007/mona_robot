import os
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.substitutions import Command
from launch_ros.actions import Node


def generate_launch_description():
    pkg_path = get_package_share_directory('mona_description')

    # Путь к xacro файлу
    xacro_file = os.path.join(pkg_path, 'urdf', 'mona.urdf.xacro')

    # Обработка xacro: запускаем команду 'xacro' и сохраняем результат в переменную
    robot_description_config = Command(['xacro ', xacro_file])

    # Создаем параметр robot_description
    params = {'robot_description': robot_description_config}

    # Нода State Publisher
    node_robot_state_publisher = Node(
        package='robot_state_publisher',
        executable='robot_state_publisher',
        output='screen',
        parameters=[params]
    )

    # Нода Rviz2
    node_rviz = Node(
        package='rviz2',
        executable='rviz2',
        name='rviz2',
        output='screen'
    )

    return LaunchDescription([
        node_robot_state_publisher,
        node_rviz
    ])
