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

#include <memory>
#include <string>
#include <vector>

#include "laser_geometry/laser_geometry.hpp"
#include "message_filters/subscriber.h"
#include "message_filters/sync_policies/approximate_time.h"
#include "message_filters/time_synchronizer.h"
#include "rclcpp/rclcpp.hpp"
#include "sensor_msgs/msg/laser_scan.hpp"
#include "sensor_msgs/msg/point_cloud2.hpp"
#include "sensor_msgs/point_cloud2_iterator.hpp"
#include "tf2_ros/buffer.h"
#include "tf2_ros/transform_listener.h"
#include "tf2_sensor_msgs/tf2_sensor_msgs.hpp"


class MonaLidarMerger : public rclcpp::Node {
public:
    MonaLidarMerger() : Node("mona_lidar_merger") {
        // Инициализация TF для трансформации точек в base_link
        tf_buffer_   = std::make_shared<tf2_ros::Buffer>(this->get_clock());
        tf_listener_ = std::make_shared<tf2_ros::TransformListener>(*tf_buffer_);

        // Подписываемся на топики, указанные в твоем lidar.xacro
        sub_front_.subscribe(this, "lidar_front/scan");
        sub_back_.subscribe(this, "lidar_back/scan");
        sub_left_.subscribe(this, "lidar_left/scan");
        sub_right_.subscribe(this, "lidar_right/scan");

        // Синхронизация данных. Очередь (10) и ссылки на подписчиков.
        sync_ = std::make_shared<message_filters::Synchronizer<SyncPolicy>>(
            SyncPolicy(10), sub_front_, sub_back_, sub_left_, sub_right_);

        sync_->registerCallback(
            std::bind(
                &MonaLidarMerger::merge_callback, this,
                std::placeholders::_1, std::placeholders::_2, std::placeholders::_3,
                std::placeholders::_4));

        publisher_ = this->create_publisher<sensor_msgs::msg::PointCloud2>(
            "perception/combined_cloud", 10);

        RCLCPP_INFO(this->get_logger(), "Mona Lidar Merger started. Target frame: base_link");
    }

private:
    void merge_callback(
        const sensor_msgs::msg::LaserScan::ConstSharedPtr & front,
        const sensor_msgs::msg::LaserScan::ConstSharedPtr & back,
        const sensor_msgs::msg::LaserScan::ConstSharedPtr & left,
        const sensor_msgs::msg::LaserScan::ConstSharedPtr & right) {
        auto combined_cloud =
            std::make_shared<sensor_msgs::msg::PointCloud2>();
        std::vector<sensor_msgs::msg::LaserScan::ConstSharedPtr> scans = {front, back, left, right};

        for (const auto & scan : scans) {
            sensor_msgs::msg::PointCloud2 cloud_out;
            try {
                // Ждем трансформацию до 100мс, если её еще нет в буфере
                if (tf_buffer_->canTransform(
                        "base_link", scan->header.frame_id,
                        scan->header.stamp, rclcpp::Duration::from_seconds(0.1)))
                {
                    projector_.transformLaserScanToPointCloud(
                        "base_link", *scan, cloud_out,
                        *tf_buffer_);

                    if (combined_cloud->data.empty()) {
                        *combined_cloud = cloud_out;
                    } else {
                        concatenate_pointclouds(*combined_cloud, cloud_out);
                    }
                } else {
                    RCLCPP_WARN(
                        this->get_logger(), "Timeout waiting for transform from %s to base_link",
                        scan->header.frame_id.c_str());
                }
            } catch (const std::exception & e) {
                RCLCPP_ERROR(this->get_logger(), "Projection error: %s", e.what());
            }
        }

        combined_cloud->header.stamp    = this->get_clock()->now();
        combined_cloud->header.frame_id = "base_link";
        publisher_->publish(*combined_cloud);
    }

    // Вспомогательная функция для эффективного объединения байтовых буферов облаков
    void concatenate_pointclouds(
        sensor_msgs::msg::PointCloud2 & base,
        const sensor_msgs::msg::PointCloud2 & add) {
        base.data.insert(base.data.end(), add.data.begin(), add.data.end());
        base.width   += add.width;
        base.row_step = base.width * base.point_step;
        base.is_dense = base.is_dense && add.is_dense;
    }

    laser_geometry::LaserProjection projector_;
    std::shared_ptr<tf2_ros::Buffer> tf_buffer_;
    std::shared_ptr<tf2_ros::TransformListener> tf_listener_;

    message_filters::Subscriber<sensor_msgs::msg::LaserScan> sub_front_, sub_back_, sub_left_,
        sub_right_;
    typedef message_filters::sync_policies::ApproximateTime<
            sensor_msgs::msg::LaserScan, sensor_msgs::msg::LaserScan,
            sensor_msgs::msg::LaserScan, sensor_msgs::msg::LaserScan> SyncPolicy;
    std::shared_ptr<message_filters::Synchronizer<SyncPolicy>> sync_;

    rclcpp::Publisher<sensor_msgs::msg::PointCloud2>::SharedPtr publisher_;
};

int main(int argc, char ** argv) {
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<MonaLidarMerger>());
    rclcpp::shutdown();
    return 0;
}
