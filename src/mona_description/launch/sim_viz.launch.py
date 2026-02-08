import os
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import IncludeLaunchDescription, SetEnvironmentVariable
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch_ros.actions import Node


def generate_launch_description():
    pkg_share = get_package_share_directory('mona_description')

    # --- НАСТРОЙКА GAZEBO ---
    # Чтобы Gazebo видел ресурсы по package://, нужно добавить путь к install/share
    # в переменную окружения GAZEBO_MODEL_PATH

    # pkg_share указывает на .../install/mona_description/share/mona_description
    # Нам нужен родительский каталог: .../install/mona_description/share
    install_dir = os.path.dirname(pkg_share)

    # Проверяем текущее значение переменной, чтобы не перезатереть системные пути
    if 'GAZEBO_MODEL_PATH' in os.environ:
        gazebo_model_path = os.environ['GAZEBO_MODEL_PATH'] + os.pathsep + install_dir
    else:
        gazebo_model_path = install_dir

    # Создаем действие для установки переменной
    set_gazebo_model_path = SetEnvironmentVariable(
        name='GAZEBO_MODEL_PATH',
        value=gazebo_model_path
    )
    # ------------------------

    # 1. Путь к конфигу RViz
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
        set_gazebo_model_path,
        simulation_launch,
        rviz_node
    ])
