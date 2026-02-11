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

import rclpy
from rclpy.node import Node
from lifecycle_msgs.srv import ChangeState
from lifecycle_msgs.msg import Transition


class LifecycleManager(Node):
    def __init__(self):
        super().__init__('mona_lifecycle_manager')

        # СПИСОК УПРАВЛЯЕМЫХ НОД В ПОРЯДКЕ ЗАПУСКА
        # 1. Safety (чтобы робот мог остановиться)
        # 2. Perception (чтобы робот начал видеть)
        self.managed_nodes = [
            'safety_node',
            'mona_lidar_merger'
        ]

        self.get_logger().info(f"Lifecycle Manager started. Managing: {self.managed_nodes}")

        # Создаем клиенты для каждой ноды
        self.service_clients = {}
        for node_name in self.managed_nodes:
            service_name = f'/{node_name}/change_state'
            self.service_clients[node_name] = self.create_client(ChangeState, service_name)

    def run_startup_sequence(self):
        """Полный цикл запуска: Configure -> Activate"""
        self.get_logger().info("Waiting for services to be available...")

        # 1. Ждём появления всех сервисов
        for node_name, client in self.service_clients.items():
            if not client.wait_for_service(timeout_sec=10.0):
                self.get_logger().error(f"Service {client.srv_name} not available! Aborting.")
                return False

        self.get_logger().info("All services found. Starting configuration sequence...")

        # 2. CONFIGURE (Настройка)
        for node_name in self.managed_nodes:
            if not self.change_state(node_name, Transition.TRANSITION_CONFIGURE):
                return False

        self.get_logger().info("Configuration complete. Starting activation sequence...")

        # 3. ACTIVATE (Включение)
        for node_name in self.managed_nodes:
            if not self.change_state(node_name, Transition.TRANSITION_ACTIVATE):
                return False

        self.get_logger().info("SYSTEM FULLY ACTIVE!")
        return True

    def change_state(self, node_name, transition_id):
        """Обрабатываем запрос на смену состояния"""
        client = self.service_clients[node_name]
        req = ChangeState.Request()
        req.transition.id = transition_id

        self.get_logger().info(f"Requesting transition {transition_id} for {node_name}...")

        future = client.call_async(req)
        rclpy.spin_until_future_complete(self, future)

        if future.result() is not None:
            if future.result().success:
                self.get_logger().info(f"[{node_name}] Transition ID {transition_id} : SUCCESS")
                return True
            else:
                self.get_logger().error(f"[{node_name}] Transition ID {transition_id} : FAILED")
                return False
        else:
            self.get_logger().error(f"[{node_name}] service call {transition_id} : FAILED")
            return False


def main(args=None):
    rclpy.init(args=args)
    manager = LifecycleManager()

    # Запускаем последовательность конфигурации и активации
    success = manager.run_startup_sequence()

    if success:
        # TODO: Health Check Monitor
        # Здесь мы можем создать таймер, который 1 раз в секунду проверяет состояние нод.

        try:
            # Блокируем поток, пока нода работает
            rclpy.spin(manager)
        except KeyboardInterrupt:
            # Штатное завершение по Ctrl+C
            manager.get_logger().info("Keyboard Interrupt. Shutting down...")
    else:
        # FATAL - самый высокий уровень ошибки
        # означает что система не может продолжать работу
        manager.get_logger().fatal("STARTUP SEQUENCE FAILED! SYSTEM IS UNSTABLE.")

    # Чистим ресурсы
    manager.destroy_node()
    rclpy.shutdown()


if __name__ == '__main__':
    main()
