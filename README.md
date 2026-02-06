# MONA Robot: Warehouse AMR

[![CI](https://github.com/vladubase/mona_robot/actions/workflows/ci.yml/badge.svg)](https://github.com/vladubase/mona_robot/actions/workflows/ci.yml)

**MONA** — это проект автономного мобильного робота (AMR) для складской логистики. Робот оснащен колесами Илона (Mecanum wheels) для всенаправленного движения и рассчитан на перевозку грузов до 400 кг.

Проект разрабатывается с использованием **ROS 2 Humble**, **Docker** и современных практик **CI/CD**.

## 🚀 Ключевые особенности

* **Holonomic Drive:** Кинематическая модель для Mecanum-колес.
* **Safety Lifecycle Node (C++):** Реализация управляемого узла (Managed Node) для обеспечения безопасности. Гарантирует остановку робота при деактивации системы.
* **Industrial Simulation:** Среда Gazebo Classic с моделью склада.
* **DevOps Ready:**
    * 🐳 Полностью докеризированное окружение разработки.
    * 🔄 CI/CD pipeline на базе GitHub Actions (сборка и тесты при каждом PR).

## 🛠️ Технологический стек

* **Framework:** ROS 2 Humble Hawksbill
* **Языки:** C++ 17, Python 3.10
* **Симуляция:** Gazebo Classic, RViz 2
* **Инфраструктура:** Docker, Docker Compose, GitHub Actions

## 💻 Запуск проекта

Для запуска не требуется устанавливать ROS 2 на хост-машину. Достаточно Docker.

### 1. Клонирование и запуск контейнера
```bash
git clone [https://github.com/vladubase/mona_robot.git](https://github.com/vladubase/mona_robot.git)
cd mona_robot
docker compose up -d
```

### 2. Подключение к контейнеру
```Bash
docker exec -it mona_dev bash
```

### 3. Сборка и запуск симуляции
Внутри контейнера:

```Bash
colcon build
source install/setup.bash
ros2 launch mona_description sim_viz.launch.py
```

## Roadmap
[x] Базовая симуляция (URDF, Gazebo)
[x] Docker Environment
[x] Safety Node (Lifecycle, C++)
[x] CI/CD Pipeline
[ ] Навигация (Nav2)
[ ] Интеграция VDA5050 (MQTT Bridge)
[ ] Покрытие Unit-тестами

Разработано в рамках демонстрационного проекта по современной робототехнике.