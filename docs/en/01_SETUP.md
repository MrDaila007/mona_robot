# Environment Setup and Execution

> **Environment as Code**
> 
> The development environment must be fully deterministic. The infrastructure is described in the Dockerfile.

This document outlines the deployment procedure within an isolated Docker environment. The "Simulation First" approach guarantees identical development environments and minimizes host hardware dependencies.

## 1. Prerequisites
The project is fully containerized. The host machine must have the following software components installed:
- Git
- Docker Engine (v24.0 or higher)
- Docker Compose (V2 plugin)

### 1.1. Setup for Linux (Ubuntu/Arch Linux)
Install Docker Engine via the official convenience script:
```bash
curl -fsSL [https://get.docker.com](https://get.docker.com) -o get-docker.sh
sudo sh get-docker.sh
sudo usermod -aG docker $USER
newgrp docker
```
_Note:_ Ensure that integrated graphics drivers (e.g., `mesa`) are installed on the host OS.

### 1.2. Setup for Windows 11 (WSL2)
1. Install Docker Desktop for Windows.
2. Navigate to Settings -> Resources -> WSL Integration in Docker Desktop.
3. Enable integration for your specific Ubuntu distribution (22.04 LTS).
4. Restart Docker Desktop. No manual Docker installation is required inside the Ubuntu terminal.

## 2. Repository Cloning
```bash
git clone git@github.com:vladubase/mona_robot.git
cd mona_robot
```

## 3. Container Build and Execution
For the initial deployment, or whenever `Dockerfile`/`docker-compose.yml` configurations are modified, a complete image rebuild is required:
```bash
docker compose build --no-cache
docker compose up -d --force-recreate
```

For subsequent executions, use:
```bash
docker compose up -d
```

## 4. Development and Testing
All ROS 2 interactions and code compilation must be performed exclusively within the container:
```bash
docker exec -it mona_dev bash

./scripts/run_sim.bash
```
