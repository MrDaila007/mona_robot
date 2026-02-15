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

// Points per scan:    FOV/res = 150/0.17 ~ 880 точек за скан
// Point size:         PointCloud2 - float32 - 4байта - 16 байт
// Volume of scan:     880 * 16 ~ 14200 байт ~ 13.8 кБ
// Update rate:        15 Герц
// Bandwidth:          (13.8 * 15) * 4 = 827 кБ/с = 6.6 Мбит/с


#include <memory>
#include <string>
#include <vector>

#include "laser_geometry/laser_geometry.hpp"                // Motion Skew solver
#include "message_filters/subscriber.h"                     // The Synchronization Layer
#include "message_filters/sync_policies/approximate_time.h"
#include "message_filters/time_synchronizer.h"
#include "rclcpp/rclcpp.hpp"
#include "rclcpp_lifecycle/lifecycle_node.hpp"
#include "lifecycle_msgs/msg/state.hpp"
#include "std_msgs/msg/bool.hpp"
#include "sensor_msgs/msg/laser_scan.hpp"
#include "sensor_msgs/msg/point_cloud2.hpp"
#include "sensor_msgs/point_cloud2_iterator.hpp"
#include "tf2_ros/buffer.h"
#include "tf2_ros/transform_listener.h"
#include "tf2_sensor_msgs/tf2_sensor_msgs.hpp"

// Удобный алиас для возвращаемого типа коллбэков
using CallbackReturn = rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn;


class LidarMerger : public rclcpp_lifecycle::LifecycleNode {
public:
    // Конструктор должен быть пустым или минимальным.
    // Вся инициализация переносится в on_configure.
    explicit LidarMerger(const rclcpp::NodeOptions &options) : LifecycleNode(
                                                                   "mona_lidar_merger",
                                                                   options) {
    }

    // 1. CONFIGURE: Выделяем память, создаем TF и подписчиков
    CallbackReturn on_configure(const rclcpp_lifecycle::State &) override {
        RCLCPP_INFO(get_logger(), "Configuring Lidar Merger...");

        // Инициализация TF
        tf_buffer_   = std::make_shared<tf2_ros::Buffer>(this->get_clock());
        tf_listener_ = std::make_shared<tf2_ros::TransformListener>(*tf_buffer_);

        // Настраиваем QoS для совместимости с драйверами лидаров (Best Effort)
        rmw_qos_profile_t custom_qos = rmw_qos_profile_sensor_data;

        // Создаём Lifecycle Publisher
        publisher_ = this->create_publisher<sensor_msgs::msg::PointCloud2>(
            "perception/combined_cloud", 10);

        power_sub_ = this->create_subscription<std_msgs::msg::Bool>(
            "/hardware/power/perception_pc", 10,
            std::bind(&LidarMerger::power_callback, this, std::placeholders::_1)
        );

        // Инициализация подписчиков
        // Передаем 'this' (узел), название топика и профиль QoS
        sub_front_.subscribe(this, "lidar_front/scan", custom_qos);
        sub_back_.subscribe(this, "lidar_back/scan", custom_qos);
        sub_left_.subscribe(this, "lidar_left/scan", custom_qos);
        sub_right_.subscribe(this, "lidar_right/scan", custom_qos);

        // Инициализация синхронизатора
        // Очередь размером 10 сообщений. Это буфер, в котором алгоритм ищет совпадения.
        sync_ = std::make_shared<message_filters::Synchronizer<SyncPolicy>>(
            SyncPolicy(10), sub_front_, sub_back_, sub_left_, sub_right_);
        sync_->registerCallback(
            std::bind(
                &LidarMerger::fused_callback, this,
                std::placeholders::_1, std::placeholders::_2, std::placeholders::_3,
                std::placeholders::_4));

        return CallbackReturn::SUCCESS;
    }

    // 2. ACTIVATE: Включаем паблишер
    CallbackReturn on_activate(const rclcpp_lifecycle::State &state) override {
        (void)state;        // Подавляем предупреждение о неиспользуемом параметре
        RCLCPP_INFO(get_logger(), "Activating Lidar Merger...");

        // В Lifecycle нодах паблишер по умолчанию заглушен.
        publisher_->on_activate();

        return CallbackReturn::SUCCESS;
    }

    // 3. DEACTIVATE: Выключаем паблишер
    CallbackReturn on_deactivate(const rclcpp_lifecycle::State &state) override {
        (void)state;
        RCLCPP_INFO(get_logger(), "Deactivating Lidar Merger...");

        publisher_->on_deactivate();

        return CallbackReturn::SUCCESS;
    }

    // 4. CLEANUP: Очищаем память
    CallbackReturn on_cleanup(const rclcpp_lifecycle::State &) override {
        RCLCPP_INFO(get_logger(), "Cleaning up Lidar Merger...");

        // Сбрасываем умные указатели и отписываемся
        sync_.reset();
        tf_listener_.reset();
        tf_buffer_.reset();
        publisher_.reset();

        sub_front_.unsubscribe();
        sub_back_.unsubscribe();
        sub_left_.unsubscribe();
        sub_right_.unsubscribe();

        return CallbackReturn::SUCCESS;
    }

    // 5. SHUTDOWN
    CallbackReturn on_shutdown(const rclcpp_lifecycle::State &state) override {
        (void)state;
        RCLCPP_INFO(get_logger(), "Shutting down Lidar Merger...");

        sync_.reset();
        publisher_.reset();

        return CallbackReturn::SUCCESS;
    }

    CallbackReturn on_error(const rclcpp_lifecycle::State &previous_state) override {
        RCLCPP_ERROR(
            get_logger(), "Error handling transition from %s",
            previous_state.label().c_str());

        // Очищаем ресурсы, если нужно
        publisher_.reset();

        return CallbackReturn::FAILURE;     // Переведет ноду в state FINALIZED (смерть)
    }

private:
    bool has_power_ = true;
    rclcpp::Subscription<std_msgs::msg::Bool>::SharedPtr power_sub_;
    void power_callback(const std_msgs::msg::Bool::SharedPtr msg) {
        has_power_ = msg->data;
        if (!has_power_) {
            RCLCPP_ERROR(this->get_logger(), "POWER CUT! Perception module is going blind...");
        } else {
            RCLCPP_INFO(this->get_logger(), "POWER RESTORED! Perception module booting up...");
        }
    }

    void fused_callback(
        const sensor_msgs::msg::LaserScan::ConstSharedPtr &front,
        const sensor_msgs::msg::LaserScan::ConstSharedPtr &back,
        const sensor_msgs::msg::LaserScan::ConstSharedPtr &left,
        const sensor_msgs::msg::LaserScan::ConstSharedPtr &right) {
        // Если данные приходят но нода не ACTIVE - не обрабатываем
        if (this->get_current_state().id() != lifecycle_msgs::msg::State::PRIMARY_STATE_ACTIVE) {
            return;
        }
        if (!has_power_) {
            return;     // Имитируем отсутствие питания - дропаем все данные!
        }

        auto combined_cloud = std::make_shared<sensor_msgs::msg::PointCloud2>();
        std::vector<sensor_msgs::msg::LaserScan::ConstSharedPtr> scans = {front, back, left, right};

        // Проецируем каждый лидар в 3D облако в системе base_link
        // Автоматически учитываем временные метки каждого луча для коррекции искажений
        for (const auto &scan : scans) {
            sensor_msgs::msg::PointCloud2 cloud_out;
            try {
                // В ROS 2 использование TimePointZero гарантирует получение последнего
                // TF без блокировки. Так как лидар зафиксирован относительно base_link,
                // этот TF не меняется со временем (Static TF).
                // Функция transformLaserScanToPointCloud сама извлечёт TF.
                projector_.transformLaserScanToPointCloud(
                    "base_link",
                    *scan,
                    cloud_out,
                    *tf_buffer_
                );

                if (combined_cloud->data.empty()) {
                    *combined_cloud = cloud_out;
                } else {
                    concatenate_pointclouds(*combined_cloud, cloud_out);
                }
            } catch (const std::exception &e) {
                // Используем THROTTLE, чтобы не спамить в консоль, если TF реально отвалился
                RCLCPP_WARN_THROTTLE(
                    this->get_logger(), *this->get_clock(), 1000,
                    "Projection error for %s: %s", scan->header.frame_id.c_str(), e.what());
            }
        }

        combined_cloud->header.stamp    = this->get_clock()->now();
        combined_cloud->header.frame_id = "base_link";

        publisher_->publish(*combined_cloud);
    }

    // Вспомогательная функция для эффективного объединения байтовых буферов облаков
#if 0
    void concatenate_pointclouds(
        sensor_msgs::msg::PointCloud2 &base,
        const sensor_msgs::msg::PointCloud2 &add) {
        // Пробуем заменить insert на std::memcpy
        base.data.insert(base.data.end(), add.data.begin(), add.data.end());
        base.width   += add.width;
        base.row_step = base.width * base.point_step;
        base.is_dense = base.is_dense && add.is_dense;
    }
#endif
    void concatenate_pointclouds(
        sensor_msgs::msg::PointCloud2 &base,
        const sensor_msgs::msg::PointCloud2 &add) {
        // Запоминаем текущий размер данных
        size_t original_size = base.data.size();

        // Единожды выделяем память под объединённый буфер
        // Это предотвращает лишние аллокации (realloc)
        base.data.resize(original_size + add.data.size());

        // Быстрое копирование блоков памяти на аппаратном уровне
        std::memcpy(
            base.data.data() + original_size,   // Указатель на конец старых данных
            add.data.data(),                    // Указатель на начало новых данных
            add.data.size()                     // Количество байт для копирования
        );

        base.width   += add.width;
        base.row_step = base.width * base.point_step;
        base.is_dense = base.is_dense && add.is_dense;
    }

    // Motion Skew solver
    laser_geometry::LaserProjection projector_;
    std::shared_ptr<tf2_ros::Buffer> tf_buffer_;
    std::shared_ptr<tf2_ros::TransformListener> tf_listener_;

    // Явно указываем тип LifecycleNode для message_filters
    // Накапливаем сообщения, но не вызывают callback сразу
    message_filters::Subscriber<sensor_msgs::msg::LaserScan,
        rclcpp_lifecycle::LifecycleNode> sub_front_, sub_back_, sub_left_,
        sub_right_;

    typedef message_filters::sync_policies::ApproximateTime<
            sensor_msgs::msg::LaserScan, sensor_msgs::msg::LaserScan,
            sensor_msgs::msg::LaserScan, sensor_msgs::msg::LaserScan> SyncPolicy;

    // Синхронизатор
    std::shared_ptr<message_filters::Synchronizer<SyncPolicy>> sync_;

    rclcpp_lifecycle::LifecyclePublisher<sensor_msgs::msg::PointCloud2>::SharedPtr publisher_;
};

int main(int argc, char **argv) {
    rclcpp::init(argc, argv);

    rclcpp::executors::SingleThreadedExecutor executor;

    // Передаём NodeOptions для Lifecycle
    auto node = std::make_shared<LidarMerger>(rclcpp::NodeOptions());

    // Для Lifecycle нод нужно получать get_node_base_interface()
    executor.add_node(node->get_node_base_interface());

    executor.spin();

    rclcpp::shutdown();
    return 0;
}
