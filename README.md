# MONA: Modular Open Navigating AMR

> Проект автономного мобильного робота для складской логистики. Система построена на базе ROS 2 Humble и использует контейнеризированную среду разработки для гарантии воспроизводимости кода.

[![CI](https://github.com/vladubase/mona_robot/actions/workflows/ci.yml/badge.svg?branch=main)](https://github.com/vladubase/mona_robot/actions/workflows/ci.yml)
[![CodeQL](https://github.com/vladubase/mona_robot/actions/workflows/github-code-scanning/codeql/badge.svg?branch=main)](https://github.com/vladubase/mona_robot/actions/workflows/github-code-scanning/codeql)
[![CodeFactor](https://www.codefactor.io/repository/github/vladubase/mona_robot/badge)](https://www.codefactor.io/repository/github/vladubase/mona_robot)
[![OpenSSF Scorecard](https://api.securityscorecards.dev/projects/github.com/vladubase/mona_robot/badge)](https://api.securityscorecards.dev/projects/github.com/vladubase/mona_robot)
[![OpenSSF Best Practices](https://www.bestpractices.dev/projects/11949/badge)](https://www.bestpractices.dev/projects/11949)

![Mona Robot Simulation Preview](docs/images/mona_gazebo_preview.png)
## О проекте
**MONA** (Modular Open Navigating AMR) — масштабируемая архитектура управления флотом роботов. Основной упор сделан на безопасность (Safety-Critical), аппаратное резервирование и отказоустойчивость (FDIR) в соответствии с индустриальными стандартами.

## Ключевые архитектурные особенности
* **Промышленная безопасность (ISO 13849-1 / IEC 61508):** Гибридная архитектура FDIR (Fault Detection, Isolation, and Recovery). Аппаратное резервирование цепей отключения контакторов, Watchdog и EMA-сглаживание пиковых нагрузок тяжёлого шасси (при телеуправлении).
* **Сетевой стек и DDS:** Прямой доступ к хост-сети (`network_mode: host`) с гибким управлением видимостью (`ROS_LOCALHOST_ONLY`) и жёсткой фиксацией реализации DDS (`rmw_fastrtps_cpp`).
* **Fleet Management Ready:** Архитектура закладывает основу для будущей интеграции со стандартами управления флотом VDA 5050 и Open-RMF.
* **Строгий CI/CD:** покрытие кода статическими анализаторами (Clang-Tidy, CPPCheck, Uncrustify, Flake8) и модульным тестированием (GTest).

## Структура репозитория (Component Architecture)

Проект построен на базе компонентной архитектуры ROS 2 (Zero-copy IPC), разделённой на логические домены:
* **`mona_control/`** — Модуль диспетчеризации (Twist Mux). Отвечает за маршрутизацию команд от джойстика и Nav2, EMA-сглаживание, интерполяцию скоростей на 100 Гц и отмену (preemption) автономных задач при ручном перехвате.
* **`mona_core/`** — Главный пакет-оркестратор (Bringup). Содержит единые `.launch.py` файлы, глобальные параметры (`.yaml`), карты и менеджер FDIR (Fault Detection, Isolation, and Recovery) на Python.
* **`mona_description/`** — Визуальное и физическое представление робота (URDF, Xacro, 3D-меши).
* **`mona_perception/`** — Модуль восприятия. Содержит `LidarMergerNode` для объединения данных с нескольких лазерных сканеров в единое облако точек.
* **`mona_safety/`** — Аппаратный страж (`SafetyNode`). Обрабатывает экстренные остановки (E-Stop), управляет аппаратными контакторами, обрезает скорости при деградации системы и эскалирует аварии при несанкционированном движении по одометрии.

## Быстрый старт
Проект использует строгую изоляцию среды через Docker.
```bash
# Клонирование репозитория
git clone git@github.com:vladubase/mona_robot.git ~/mona_robot
cd mona_robot

# Запуск среды разработки (CPU-only, без GPU)
docker compose up -d --build

# Запуск с поддержкой NVIDIA GPU (через дополнительный compose-файл)
# Перед этим убедитесь, что установлены драйверы и nvidia-container-toolkit,
# а команда ниже успешно отрабатывает:
#   nvidia-smi
docker compose -f docker-compose.yml -f docker-compose.gpu.yml up -d --build

# Вход в контейнер
docker exec -it mona_dev bash
```

## Быстрый старт через Makefile
Для удобства есть `Makefile` с короткими алиасами:
```bash
# Запуск среды разработки (CPU-only, без GPU)
make up      # alias: u

# Запуск с поддержкой NVIDIA GPU
make up-gpu  # alias: ug

# Остановка контейнеров
make down    # alias: d
```

## Навигация по документации
Вся техническая документация расположена в директории `docs/`.

| **Категория**                  | **Документ**                                                                                       | **Описание**                                                                                                                    |
| ------------------------------ | -------------------------------------------------------------------------------------------------- | ------------------------------------------------------------------------------------------------------------------------------- |
| **Введение (Getting Started)** | Сборка и запуск<br>**[01_SETUP.md](docs/01_SETUP.md)**                                             | Содержит инструкции по развертыванию Docker-контейнера, пробросу оборудования и параметрам запуска базовой симуляции.           |
|                                | Рабочий процесс<br>**[04_WORKFLOW.md](docs/04_WORKFLOW.md)**                                       | Описывает стратегию ветвления (Git-flow) и базовые шаги локальной сборки пакетов.                                               |
|                                | Документация по нодам и топикам ROS 2<br>**[05_NODES_AND_TOPICS.md](docs/05_NODES_AND_TOPICS.md)** | Содерит описание и инструкции по использованию нод и топиков, особенно Lifecycle и FDIR.                                        |
|                                | Контрибьютинг<br>**[CONTRIBUTING.md](docs/CONTRIBUTING.md)**                                       | Правила участия в разработке, стандарты именования и оформления коммитов.                                                       |
| **Архитектура (Architecture)** | Безопасность и FDIR<br>**[03_SAFETY_AND_FDIR.md](docs/03_SAFETY_AND_FDIR.md)**                     | Описание системы Fault Detection, Isolation, and Recovery и принципов аппаратного резервирования.                               |
|                                | Сеть и интеграция<br>**[networking_and_fleet.md](docs/architecture/networking_and_fleet.md)**      | Описание конфигурации сетевого стека Docker (host mode), настроек DDS и подготовки архитектуры к стандартам VDA 5050.           |
|                                | Одометрия и SLAM<br>**[mapping_and_odometry.md](docs/architecture/mapping_and_odometry.md)**       | Схема слияния данных сенсоров (EKF), описание TF-дерева и базовых узлов навигационного стека.                                   |
| **Руководства (Guides)**       | Ручное управление<br>**[teleoperation.md](docs/guides/teleoperation.md)**                          | Описание работы Deadman Switch, маппинга осей DualSense и логики передачи команд скорости.                                      |
|                                | Настройка геймпада<br>**[gamepad_setup.md](docs/guides/gamepad_setup.md)**                         | Описание принципа поиска и настройки геймпада.                                                                                  |
|                                | Картирование<br>**[mapping_guide.md](docs/guides/mapping_guide.md)**                               | Инструкция по использованию slam_toolbox и различия между обновлением статической карты и работой динамической local_costmap.   |
|                                | Стиль C++<br>**[02_CPP_GUIDE.md](docs/02_CPP_GUIDE.md)**                                           | Правила автоматического форматирования (Uncrustify), настройки линтеров и требования к написанию узлов в виде ROS 2 Components. |
