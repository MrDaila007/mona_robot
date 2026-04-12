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
from launch.substitutions import LaunchConfiguration
from launch.actions import DeclareLaunchArgument
from launch_ros.actions import Node


def generate_launch_description():
    use_sim_time = LaunchConfiguration("use_sim_time", default="true")
    use_sim_time_arg = DeclareLaunchArgument(
        "use_sim_time",
        default_value="true",
        description="Use simulation (Gazebo) clock if true",
    )

    namespace = LaunchConfiguration("namespace", default="mona_001")
    namespace_arg = DeclareLaunchArgument(
        "namespace",
        default_value="mona_001",
        description="Top-level namespace for the robot",
    )

    # Converting PointCloud2 to LaserScan for SLAM
    pointcloud_to_laserscan = Node(
        package="pointcloud_to_laserscan",
        executable="pointcloud_to_laserscan_node",
        name="pointcloud_to_laserscan",
        output="screen",
        remappings=[("cloud_in", "perception/combined_cloud"), ("scan", "scan")],
        parameters=[
            {
                "target_frame": [namespace, "/base_footprint"],
                "transform_tolerance": 0.05,
                "min_height": 0.1,
                "max_height": 0.2,
                "angle_min": -3.14159,
                "angle_max": 3.14159,
                "angle_increment": 0.0087,
                "scan_time": 0.066,
                "range_min": 0.05,
                "range_max": 40.0,
                "use_inf": True,
                "inf_epsilon": 1.0,
                "use_sim_time": use_sim_time,
            },
        ],
    )

    return LaunchDescription(
        [
            use_sim_time_arg,
            namespace_arg,
            pointcloud_to_laserscan,
        ]
    )
