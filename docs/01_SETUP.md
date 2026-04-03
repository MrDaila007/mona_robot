# Настройка окружения и запуск (Environment Setup)

> **Environment as Code**
> 
> Среда разработки должна быть полностью детерминирована. Инфраструктура описана в Dockerfile.

Данный документ описывает процедуру развертывания проекта в изолированной среде Docker. Подход "Simulation First" обеспечивает идентичность сред разработки и минимизирует зависимость от аппаратного обеспечения хоста.

## 1. Системные требования
* Git
* Docker Engine (версия 24.0+)
* Docker Compose (плагин V2)

### 1.1. Настройка для Linux (Ubuntu/Arch Linux)
Установка Docker Engine через официальный скрипт:
```bash
curl -fsSL [https://get.docker.com](https://get.docker.com) -o get-docker.sh
sudo sh get-docker.sh
sudo usermod -aG docker $USER
newgrp docker
```
Примечание: Убедитесь, что установлены пакеты для работы со встроенной графикой (например, `mesa`).

### 1.2. Настройка для Windows 11 (WSL2)
1. Установите Docker Desktop для Windows.
2. В настройках Docker Desktop перейдите в раздел Settings -> Resources -> WSL Integration.
3. Включите интеграцию для используемого дистрибутива Ubuntu (22.04 LTS).
4. Перезапустите Docker Desktop. Установка Docker внутри терминала Ubuntu не требуется.

## 2. Клонирование репозитория
```bash
git clone git@github.com:vladubase/mona_robot.git
cd mona_robot
```

## 3. Настройка сети и оборудования (docker-compose.yml)

### Область видимости ROS 2 (DDS)
По умолчанию контейнер настроен на изоляцию трафика внутри вашего ПК, чтобы не перегружать локальную сеть тяжёлыми топиками. Если вам нужно подключить второй ноутбук для отладки или Rviz:
1. Откройте `docker-compose.yml`.
2. Измените значение переменной окружения `ROS_LOCALHOST_ONLY=1` на `ROS_LOCALHOST_ONLY=0`.
3. Перезапустите контейнер: `make down && make up`.

### 4. Подключение геймпада (DualSense / Xbox)
Система автоматически пробрасывает `/dev/input` в контейнер. Однако, на хост-машине (Linux) ваш пользователь должен иметь права на чтение устройств ввода:
```bash
sudo usermod -aG input $USER
# После этого необходимо выйти из системы и зайти снова (Log out / Log in)
```

## 5. Сборка и запуск контейнера
При первом развертывании, а также при изменении конфигураций (`Dockerfile`, `docker-compose.yml`), необходимо выполнить полную сборку образа:
```bash
docker compose build --no-cache
docker compose up -d --force-recreate
```

Для последующих запусков достаточно использовать:
```bash
docker compose up -d
```

## 6. Разработка и тестирование
Взаимодействие с ROS 2 и компиляция кода осуществляются исключительно внутри контейнера:
```bash
docker exec -it mona_dev bash

# Ограничиваем DDS локальным интерфейсом во избежание сетевых коллизий
export ROS_LOCALHOST_ONLY=1
ros2 daemon stop && ros2 daemon start
./scripts/run_sim.bash
```
