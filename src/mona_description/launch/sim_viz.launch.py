import os
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import IncludeLaunchDescription
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch_ros.actions import Node

def generate_launch_description():
    pkg_share = get_package_share_directory('mona_description')

    # 1. Путь к конфигу RViz
    # Мы пока его не создали, но скажем RViz искать его здесь
    rviz_config_file = os.path.join(pkg_share, 'rviz', 'mona.rviz')

    # 2. Включаем наш файл симуляции (Gazebo + Robot + State Publisher)
    simulation_launch = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            os.path.join(pkg_share, 'launch', 'simulation.launch.py')
        )
    )

    # 3. Запускаем RViz2 с загрузкой конфига
    rviz_node = Node(
        package='rviz2',
        executable='rviz2',
        name='rviz2',
        arguments=['-d', rviz_config_file],
        output='screen'
    )

    return LaunchDescription([
        simulation_launch,
        rviz_node
    ])