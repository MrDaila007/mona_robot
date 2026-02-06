#include <memory>
#include <chrono>

#include "rclcpp/rclcpp.hpp"
#include "rclcpp_lifecycle/lifecycle_node.hpp"
#include "geometry_msgs/msg/twist.hpp"
#include "lifecycle_msgs/msg/state.hpp"

using namespace std::chrono_literals;

class SafetyNode : public rclcpp_lifecycle::LifecycleNode {
public:
    // Конструктор: инициализируем имя узла
    explicit SafetyNode(const rclcpp::NodeOptions & options = rclcpp::NodeOptions())
    : rclcpp_lifecycle::LifecycleNode("safety_node", options)
    {
    }
    
    // 1. TRANSITION: Unconfigured -> Inactive
    // Создаём паблишеры и таймеры (выделяем ресурсы)
    rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn
    on_configure(const rclcpp_lifecycle::State &) {
        RCLCPP_INFO(get_logger(), "Configuring Safety Node...");

        // Создаём Lifecycle Publisher (не шлёт данные пока не будет активным)
        cmd_vel_pub_ = this->create_publisher<geometry_msgs::msg::Twist>("cmd_vel", 10);

        // Создаём таймер, но пока не запускаем логику
        timer_ = this->create_wall_timer(100ms, std::bind(&SafetyNode::publish_safety_stop, this));

        return rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn::SUCCESS;
    }

    // 2. TRANSITION: Inactive -> Active
    // Включаем паблишеры и начинаем работу
    rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn
    on_activate(const rclcpp_lifecycle::State &) {
        RCLCPP_INFO(get_logger(), "Activating Safety Node! Sending zero velocity.");

        // Активируем паблишер
        cmd_vel_pub_->on_activate();

        // Запускаем таймер отправки сообщений
        timer_->reset();

        return rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn::SUCCESS;
    }

    // 3. TRANSITION: Active -> Inactive
    // Останавливаем работу (например, если сработал E-Stop верхнего уровня)
    rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn
    on_deactivate(const rclcpp_lifecycle::State &) {
        RCLCPP_INFO(get_logger(), "Deactivating Safety Node...");

        timer_->cancel();
        cmd_vel_pub_->on_deactivate();

        return rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn::SUCCESS;
    }

    // 4. TRANSITION: Inactive -> Unconfigured
    // Очищаем ресурсы
    rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn
    on_cleanup(const rclcpp_lifecycle::State &) {
        RCLCPP_INFO(get_logger(), "Cleaning up Safety Node...");

        timer_->reset();
        cmd_vel_pub_.reset();

        return rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn::SUCCESS;
    }

    // 5. STUTDOWN
    rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn
    on_shutdown(const rclcpp_lifecycle::State & state) {
        RCLCPP_INFO(get_logger(), "Shutting down from state: %s", state.label().c_str());

        timer_->reset();
        cmd_vel_pub_.reset();

        return rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn::SUCCESS;
    }

private:
    void publish_safety_stop() {
        // Если узел активен - шлём нули
        if (this->get_current_state().id() == lifecycle_msgs::msg::State::PRIMARY_STATE_ACTIVE) {
            auto msg = std::make_unique<geometry_msgs::msg::Twist>();
            msg->linear.x = 0.0;
            msg->angular.z = 0.0;
            cmd_vel_pub_->publish(std::move(msg));
        }
    }

    rclcpp_lifecycle::LifecyclePublisher<geometry_msgs::msg::Twist>::SharedPtr cmd_vel_pub_;
    rclcpp::TimerBase::SharedPtr timer_;
};

int main(int argc, char * argv[]) {
    rclcpp::init(argc, argv);

    // Создаём исполнитель (Executor) для обработки коллбэков
    rclcpp::executors::SingleThreadedExecutor executor;

    auto node = std::make_shared<SafetyNode>();

    executor.add_node(node->get_node_base_interface());

    executor.spin();

    rclcpp::shutdown();
    return 0;
}