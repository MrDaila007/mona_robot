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

#include <rclcpp/rclcpp.hpp>

class MonaCoreNode : public rclcpp::Node {
public:
    MonaCoreNode() : Node("mona_core_node") {
        RCLCPP_INFO(this->get_logger(), "Hello, MONA! C++ Node is running.");
    }
};

int main(int argc, char ** argv) {
    rclcpp::init(argc, argv);

    auto node = std::make_shared<MonaCoreNode>();
    rclcpp::spin(node);

    rclcpp::shutdown();
    return 0;
}
