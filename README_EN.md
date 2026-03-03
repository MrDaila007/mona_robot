# MONA: Modular Open Navigating AMR

> Autonomous mobile robot project for warehouse logistics.  
> The system is based on ROS 2 Humble and uses a containerized development environment to guarantee reproducibility.

[![CI](https://github.com/vladubase/mona_robot/actions/workflows/ci.yml/badge.svg?branch=main)](https://github.com/vladubase/mona_robot/actions/workflows/ci.yml)
[![CodeQL](https://github.com/vladubase/mona_robot/actions/workflows/github-code-scanning/codeql/badge.svg?branch=main)](https://github.com/vladubase/mona_robot/actions/workflows/github-code-scanning/codeql)
[![CodeFactor](https://www.codefactor.io/repository/github/vladubase/mona_robot/badge)](https://www.codefactor.io/repository/github/vladubase/mona_robot)
[![OpenSSF Scorecard](https://api.securityscorecards.dev/projects/github.com/vladubase/mona_robot/badge)](https://api.securityscorecards.dev/projects/github.com/vladubase/mona_robot)
[![OpenSSF Best Practices](https://www.bestpractices.dev/projects/11949/badge)](https://www.bestpractices.dev/projects/11949)

![Mona Robot Simulation Preview](docs/images/mona_gazebo_preview.png)

## About the project

**MONA** (Modular Open Navigating AMR) is a scalable architecture for a fleet of mobile robots.  
The main focus is on safety-critical behavior, hardware redundancy and fault tolerance (FDIR) in line with industrial standards.

## Quick start (Docker Compose)

The project uses strict environment isolation via Docker.

```bash
# Clone the repository
git clone git@github.com:vladubase/mona_robot.git ~/mona_robot
cd mona_robot

# Start development environment (CPU‑only, no GPU)
docker compose up -d --build

# Start with NVIDIA GPU support (using an additional compose file)
# Make sure NVIDIA drivers and nvidia-container-toolkit are installed
# and the following command succeeds:
#   nvidia-smi
docker compose -f docker-compose.yml -f docker-compose.gpu.yml up -d --build

# Attach to the dev container
docker exec -it mona_dev bash
```

## Quick start via Makefile

For convenience there is a `Makefile` with short aliases:

```bash
# Start development environment (CPU‑only, no GPU)
make up      # alias: u

# Start with NVIDIA GPU support
make up-gpu  # alias: ug

# Stop containers
make down    # alias: d
```

## Documentation navigation

All technical documentation lives in the `docs/` directory (currently in Russian):

| **Document**                                | **Audience**     | **Description**                                               |
| ------------------------------------------ | ---------------- | ------------------------------------------------------------- |
| **[01_SETUP.md](docs/01_SETUP.md)**        | DevOps           | Environment setup, Docker, networking and hardware access.    |
| **[02_CPP_GUIDE.md](docs/02_CPP_GUIDE.md)**| C++ Developers   | C++ coding standards, node templates and CMake patterns.      |
| **[03_SAFETY_AND_FDIR.md](docs/03_SAFETY_AND_FDIR.md)** | Architects | Safety pipeline, FDIR, E‑Stop logic.                         |
| **[04_WORKFLOW.md](docs/04_WORKFLOW.md)**  | All developers   | Build, testing and CI workflow.                               |
| **[CONTRIBUTING.md](docs/CONTRIBUTING.md)**| Contributors     | Git Flow, fault injection testing and pull request process.   |

