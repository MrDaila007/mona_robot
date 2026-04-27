# Environment Setup and Launch

> **Environment as Code**
> 
> The development environment must be fully deterministic. The infrastructure is defined entirely within the Docker Compose configuration. This document outlines the procedure for deploying the project within an isolated, microservice-based Docker ecosystem. The "Simulation First" approach ensures parity between development and production environments while minimizing dependencies on the host hardware. Utilizing a Swarm Architecture allows you to scale the number of active agents without reconfiguring the host system.

## 1. System Requirements
* Git
* Docker Engine (version 24.0+)
* Docker Compose (V2 plugin)

### 1.1. Linux Setup (Ubuntu / Arch Linux)
Install Docker Engine via the official convenience script:
```bash
curl -fsSL [https://get.docker.com](https://get.docker.com) -o get-docker.sh
sudo sh get-docker.sh
sudo usermod -aG docker $USER
newgrp docker
```
Note: Ensure that packages required for integrated graphics (e.g., `mesa`) are installed on your host system to support hardware acceleration.

### 1.2. Windows 11 Setup (WSL2)
- Install Docker Desktop for Windows.
- Navigate to **Settings -> Resources -> WSL Integration** within Docker Desktop.
- Enable integration for your active WSL2 distribution (e.g., Ubuntu 22.04).
- Run all subsequent build scripts exclusively from within the WSL2 terminal.

---

## 2. Clonning repository
```bash
git clone git@github.com:vladubase/mona_robot.git ~/MONA_ws
cd MONA_ws
```

---

## 3. Cloud and Fleet Management
Integration with the fleet management server [LISA (Logistics Intelligence & Swarm API)](https://github.com/vladubase/lisa_api) is achieved via MQTT bridges. This prevents heavy ROS 2 DDS traffic from leaking outside the local host network.

---

## 4. Gamepad Integration (Sony / Xbox)
The Docker container automatically mounts `/dev/input` from the host. However, your Linux user must possess read permissions for input devices:

```bash
sudo usermod -aG input $USER
# You must log out and log back in for the group changes to take effect.
```

---

## 5. Build and Simulation Launch (Swarm Mode)
The architecture consists of independent containerized services. The initial compilation of the ROS 2 Workspace occurs during the Docker image build phase.

**1. Build the Docker Image (Required initially and upon dependency changes):**
```bash
make rebuild
# OR
docker compose build --no-cache
```

**2. Launch the Simulation Environment:** Use the dedicated bash scripts to ensure proper synchronization of the global clock and staggered node initialization.

Open **Terminal 1** to start the infrastructure (Gazebo Server and Global RViz):
```bash
./scripts/start_world.bash
```

Open **Terminal 2** to deploy the swarm (specify the desired number of agents):
```bash
# Deploys a fleet of 3 robots (mona_001, mona_002, mona_003)
./scripts/start_fleet.bash 3
```

Alternatively, for single-robot testing with gamepad teleoperation enabled:
```bash
./scripts/bringup_1_robot.bash
```

---

## 6. Development and Testing
For manual compilation of C++ code and execution of static analysis tools (Clang-Tidy, CPPCheck, Flake8), use the dedicated `dev` service.

```bash
# Start the development container in the background
make up
# Or for GPU-accelerated environments:
make up-gpu

# Attach to the container's bash shell
docker compose exec -it dev bash

# Inside the container, run the automated CI pipeline
./scripts/run_ci_checks.bash
```
