# MONA: Modular Open Navigating AMR
> Проект автономного мобильного робота для складской логистики. Система построена на базе ROS 2 Humble и использует контейнеризированную среду разработки для гарантии воспроизводимости кода.

[![CI](https://github.com/vladubase/mona_robot/actions/workflows/ci.yml/badge.svg?branch=main)](https://github.com/vladubase/mona_robot/actions/workflows/ci.yml)
[![CodeQL](https://github.com/vladubase/mona_robot/actions/workflows/github-code-scanning/codeql/badge.svg?branch=main)](https://github.com/vladubase/mona_robot/actions/workflows/github-code-scanning/codeql)
[![CodeFactor](https://www.codefactor.io/repository/github/vladubase/mona_robot/badge)](https://www.codefactor.io/repository/github/vladubase/mona_robot)
[![OpenSSF Scorecard](https://api.securityscorecards.dev/projects/github.com/vladubase/mona_robot/badge)](https://api.securityscorecards.dev/projects/github.com/vladubase/mona_robot)

![Mona Robot Simulation Preview](docs/images/mona_gazebo_preview.png)
## О проекте
**MONA** (Modular Open Navigating AMR) — масштабируемая архитектура управления флотом роботов. Основной упор сделан на безопасность (Safety-Critical), модульность и соответствие индустриальным стандартам.
## Быстрый старт
Проект использует строгую изоляцию среды через Docker.
```bash
# Клонирование репозитория
git clone git@github.com:vladubase/mona_robot.git ~/mona_robot
cd mona_robot

# Запуск среды разработки
docker compose up -d --build

# Вход в контейнер
docker exec -it mona_dev bash
```
## Навигация по документации
Вся техническая документация расположена в директории `docs/`.

| **Документ**                                | **Аудитория**    | **Описание**                                       |
| ------------------------------------------- | ---------------- | -------------------------------------------------- |
| **[01_SETUP.md](docs/01_SETUP.md)**         | DevOps           | Настройка Docker, сети и доступа к оборудованию.   |
| **[02_CPP_GUIDE.md](docs/02_CPP_GUIDE.md)** | Разработчики C++ | Стандарты написания кода, шаблоны нод и CMake.     |
| **[03_LIFECYCLE.md](docs/03_LIFECYCLE.md)** | Архитекторы      | Машина состояний (Managed Nodes) и логика запуска. |
| **[04_WORKFLOW.md](docs/04_WORKFLOW.md)**   | Все разработчики | Инструкции по сборке, тестированию и CI.           |
| **[CONTRIBUTING.md](docs/CONTRIBUTING.md)** | Контрибьюторы    | Git Flow, стандарты безопасности и правила PR.     |
