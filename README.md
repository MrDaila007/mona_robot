# MONA: Modular Open Navigating AMR

> Проект автономного мобильного робота для складской логистики. Система построена на базе ROS 2 Humble и использует контейнеризированную среду разработки для гарантии воспроизводимости кода.
>
> English version: see [README_EN.md](README_EN.md).

[![CI](https://github.com/vladubase/mona_robot/actions/workflows/ci.yml/badge.svg?branch=main)](https://github.com/vladubase/mona_robot/actions/workflows/ci.yml)
[![CodeQL](https://github.com/vladubase/mona_robot/actions/workflows/github-code-scanning/codeql/badge.svg?branch=main)](https://github.com/vladubase/mona_robot/actions/workflows/github-code-scanning/codeql)
[![CodeFactor](https://www.codefactor.io/repository/github/vladubase/mona_robot/badge)](https://www.codefactor.io/repository/github/vladubase/mona_robot)
[![OpenSSF Scorecard](https://api.securityscorecards.dev/projects/github.com/vladubase/mona_robot/badge)](https://api.securityscorecards.dev/projects/github.com/vladubase/mona_robot)
[![OpenSSF Best Practices](https://www.bestpractices.dev/projects/11949/badge)](https://www.bestpractices.dev/projects/11949)

![Mona Robot Simulation Preview](docs/images/mona_gazebo_preview.png)
## О проекте
**MONA** (Modular Open Navigating AMR) — масштабируемая архитектура управления флотом роботов. Основной упор сделан на безопасность (Safety-Critical), аппаратное резервирование и отказоустойчивость (FDIR) в соответствии с индустриальными стандартами.
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

| **Документ**                                                    | **Аудитория**    | **Описание**                                     |
| --------------------------------------------------------------- | ---------------- | ------------------------------------------------ |
| **[01_SETUP.md](docs/ru/01_SETUP.md)**                         | DevOps           | Настройка Docker, сети и доступа к оборудованию. |
| **[02_CPP_GUIDE.md](docs/ru/02_CPP_GUIDE.md)**                 | Разработчики C++ | Стандарты написания кода, шаблоны нод и CMake.   |
| **[03_SAFETY_AND_FDIR.md](docs/ru/03_SAFETY_AND_FDIR.md)**     | Архитекторы      | Пайплайн безопасности, FDIR, логика E-Stop.      |
| **[04_WORKFLOW.md](docs/ru/04_WORKFLOW.md)**                   | Все разработчики | Инструкции по сборке, тестированию и CI.         |
| **[CONTRIBUTING.md](docs/ru/CONTRIBUTING.md)**                 | Контрибьюторы    | Git Flow, Fault Injection тестирование и PR.     |
