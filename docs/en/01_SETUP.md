# Environment Setup

> **Environment as Code**
>
> The development environment must be fully deterministic. The infrastructure is described in the `Dockerfile`.

## 1. Host requirements

* OS: Linux (Ubuntu 22.04 recommended)
* Docker Engine (version 24.0+)
* Docker Compose Plugin
* Git

## 2. Container architecture

Base image: `ros:humble-ros-base`.

**The image includes:**

* ROS 2 Humble (core packages)
* Build tools (`colcon`, `gcc`, `cmake`)
* Linters and formatters (`uncrustify`, `cpplint`, `cppcheck`)
* Project dependencies (installed via `rosdep`)

## 3. Workspace layout

```text
MONA_ws/
├── configs/            # Code quality configs
├── docs/               # Documentation
├── scripts/            # CI and ROS 2 launch scripts
├── src/                # Package source code
├── docker-compose.yml  # Orchestration and network parameters
└── Dockerfile          # Image build description
```

## 4. Container lifecycle

### First start

For the first deployment or when `Dockerfile` / `package.xml` changes, you need a full rebuild:

```bash
docker compose up -d --build
```

### Entering the environment

All development commands (build, test, run) must be executed **only** inside the container:

```bash
docker exec -it mona_dev bash
```

### Environment variables

Configuration is provided either via a `.env` file (if present) or directly in `docker-compose.yml`.

### Hardware access

The container runs in privileged mode (`privileged: true`) with host networking (`network_mode: host`) to ensure:

1. Direct access to USB ports (Lidar, IMU, microcontrollers).
2. Proper DDS (ROS 2 discovery) operation without NAT.

