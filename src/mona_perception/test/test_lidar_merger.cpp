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


#include <gtest/gtest.h>
#include <memory>

#include "rclcpp/rclcpp.hpp"
#include "lifecycle_msgs/msg/state.hpp"
#include "lifecycle_msgs/msg/transition.hpp"
#include "mona_perception/lidar_merger_node.hpp"

// Тестовый стенд (Fixture) для инициализации окружения ROS 2
class TestLidarMerger : public ::testing::Test {
protected:
    static void SetUpTestSuite() {
        rclcpp::init(0, nullptr);
    }

    static void TearDownTestSuite() {
        rclcpp::shutdown();
    }
};

// Test 1: Проверка инициализации и переходов жизненного цикла (lifecycle)
TEST_F(TestLidarMerger, LifecycleTransition) {
    rclcpp::NodeOptions options;
    auto node = std::make_shared<mona_perception::LidarMergerNode>(options);

    // Проверяем начальное состояние (UNCONFIGURED)
    EXPECT_EQ(
        node->get_current_state().id(),
        lifecycle_msgs::msg::State::PRIMARY_STATE_UNCONFIGURED
    );

    // Вызываем on_configure()
    // Это проверяет выделение памяти под tf2_buffer и message_filters
    auto state_after_configure = node->trigger_transition(
        lifecycle_msgs::msg::Transition::TRANSITION_CONFIGURE);

    EXPECT_EQ(
        state_after_configure.id(),
        lifecycle_msgs::msg::State::PRIMARY_STATE_INACTIVE
    );

    // Вызываем on_activate()
    // Это проверяет активацию паблишеров
    auto state_after_activate = node->trigger_transition(
        lifecycle_msgs::msg::Transition::TRANSITION_ACTIVATE);

    EXPECT_EQ(
        state_after_activate.id(),
        lifecycle_msgs::msg::State::PRIMARY_STATE_ACTIVE
    );

    // Вызываем on_deactivate()
    // Это проверяет активацию паблишеров
    auto state_after_deactivate = node->trigger_transition(
        lifecycle_msgs::msg::Transition::TRANSITION_DEACTIVATE);

    EXPECT_EQ(
        state_after_deactivate.id(),
        lifecycle_msgs::msg::State::PRIMARY_STATE_INACTIVE
    );

    // Вызываем on_shutdown()
    // Это проверяет корректное освобождение ресурсов
    auto state_after_shutdown = node->trigger_transition(
        lifecycle_msgs::msg::Transition::TRANSITION_INACTIVE_SHUTDOWN);

    EXPECT_EQ(
        state_after_shutdown.id(),
        lifecycle_msgs::msg::State::PRIMARY_STATE_FINALIZED
    );
}
