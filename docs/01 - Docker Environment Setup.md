
# Цель
Создать воспроизводимую среду разработки (Dev Environment) для проекта MONA, базирующуюся на ROS 2 Humble. Среда должна поддерживать кросс-компиляцию, прямой доступ к "железу" и горячую подмену кода.

# Иерархия образов ROS 2
В Docker Hub есть несколько контейнеров (tags) для ROS 2 Humble. Понимание разницы между ними критично для оптимизации размера проекта.
1. **`ros:humble-ros-core`**: Самый "голый". Только библиотеки коммуникации и запуска. Нет ни инструментов сборки, ни общих библиотек (tf2, urdf). Идеально для финального деплоя на очень слабые контроллеры.
2. **`ros:humble-ros-base`**: "Золотая середина" для робота. Включает `core` + базовые инструменты (геометрия, работа с файлами робота, логирование). Именно это обычно ставят на бортовые компьютеры.
3. **`ros:humble-desktop`**: Включает `base` + GUI инструменты (Rviz2, демки). Это тяжелый образ, нужен для отладки на ноутбуке.

Будем использовать **`ros:humble-ros-base`**. Это позволит нам держать образ легким, но при этом иметь всё необходимое для работы робота.

# Структура проекта
```bash
MONA_ws/
├── src/                # Исходный код пакетов (хранится на хосте)
├── Dockerfile          # Описание сборки образа
└── docker-compose.yml  # Оркестрация запуска
```

### 1. Dockerfile (Dev Layer)
Мы используем официальный образ `ros:humble-ros-base` как фундамент, добавляя поверх него инструменты разработки.

```Dockerfile
# 1. Используем официальный базовый образ ROS 2 (Bare Bones + Base tools)
# Это автоматически даёт нам Ubuntu 22.04 + настроенный ROS 2
FROM ros:humble-ros-base

# 2. Настройки неинтерактивности для apt
ENV DEBIAN_FRONTEND=noninteractive

# 3. Устанавливаем инструменты для сборки (Dev Tools)
# build-essential: gcc, g++, make
# cmake: система сборки
# git: контроль версий
# python3-colcon-common-extensions: основной инструмент сборки в ROS 2
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    python3-colcon-common-extensions \
    ros-humble-xacro ros-humble-rviz2 \
    && rm -rf /var/lib/apt/lists/*

# 4. Настройка рабочей директории (Workspace)
WORKDIR /root/mona_ws

# Создаем папку src, которую будем подменять через Volumes
RUN mkdir src

# 5. Автоматическая настройка окружения
# Добавляем source ROS 2 в .bashrc, чтобы команды ros2 были доступны сразу
RUN echo "source /opt/ros/humble/setup.bash" >> /root/.bashrc
```

### 2. Docker Compose (Runtime Configuration)
Конфигурация обеспечивает "прозрачность" контейнера для сети и оборудования, что критично для Robotics/Embedded разработки.

```yml
services:
  mona_dev:
    build:
      context: .
      dockerfile: Dockerfile
    image: mona_robot:humble
    container_name: mona_dev
    
    # === Hardware & Network Access ===
    # host: убирает изоляцию сети (нужно для ROS 2 Discovery)
    network_mode: host
    # ipc/pid host: позволяет использовать Shared Memory (Zero Copy)
    ipc: host
    pid: host
    # privileged: дает доступ ко всем устройствам в /dev (STM32, Lidar, Camera)
    privileged: true
    
    # === Development Workflow ===
    # Синхронизация кода между хостом (Arch Linux) и контейнером
    volumes:
      - ./src:/root/mona_ws/src
      
    # Бесконечный цикл, чтобы контейнер не завершал работу
    command: sleep infinity
    tty: true
    stdin_open: true
```

### 3. Запуск и проверка
Сборка и запуск в фоновом режиме:
```bash
docker compose up -d --build
```

Вход внутрь контейнера:
```bash
docker exec -it mona_dev bash
```

Проверяем работоспособность
```bash
ros2
```
