#include <rclcpp/rclcpp.hpp>

// Наследуемся от rclcpp::Node — это стандарт для C++ нод
class MonaCoreNode : public rclcpp::Node {
public:
    // Конструктор: задаем имя ноды ("mona_core_node")
    MonaCoreNode() : Node("mona_core_node") {
        // RCLCPP_INFO — это аналог printf/cout, но с метками времени и уровнями логирования
        RCLCPP_INFO(this->get_logger(), "Hello, MONA! C++ Node is running.");
    }
};

int main(int argc, char ** argv) {
    // 1. Инициализация ROS 2 (парсинг аргументов)
    rclcpp::init(argc, argv);

    // 2. Создание ноды и передача её в "исполнитель" (spin)
    // make_shared создает умный указатель (smart pointer)
    auto node = std::make_shared<MonaCoreNode>();

    // spin запускает бесконечный цикл обработки событий (таймеры, топики)
    // Пока событий нет, нода просто "висит" и ждет (Hello выведется в конструкторе)
    rclcpp::spin(node);

    // 3. Корректное завершение
    rclcpp::shutdown();
    return 0;
}
