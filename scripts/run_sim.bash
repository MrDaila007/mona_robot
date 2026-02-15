#!/bin/bash

# ==============================================================================
# Script Name: run_sim.bash
# Description: Full cycle: Clean -> Build -> Launch Mona Simulation
# Usage:       Run from the workspace root (e.g., ./scripts/run_sim.bash)
# 
# Copyright 2026 vladubase
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
# ==============================================================================

# Define colors for output clarity
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}[INFO] Starting Mona Robot Simulation sequence...${NC}"

# ------------------------------------------------------------------------------
# 1. Pre-flight Check
# ------------------------------------------------------------------------------
if [ ! -d "src" ]; then
    echo -e "${RED}[ERROR] Directory 'src' not found.${NC}"
    echo "Please run this script from the root of your ROS 2 workspace."
    exit 1
fi

# ------------------------------------------------------------------------------
# 2. Clean Workspace
# ------------------------------------------------------------------------------
echo -e "${YELLOW}[1/3] Cleaning workspace (build, install, log)...${NC}"

if ! rm -rf build/ install/ log/; then
    echo -e "${RED}[ERROR] Failed to clean directories.${NC}"
    exit 1
fi
echo -e "${GREEN}[OK] Workspace is clean.${NC}"

# ------------------------------------------------------------------------------
# 3. Build Packages
# ------------------------------------------------------------------------------
echo -e "${YELLOW}[2/3] Building packages...${NC}"

# Используем --symlink-install для быстрой правки xacro/rviz/python без пересборки
if ! colcon build --cmake-clean-cache --symlink-install; then
    echo -e "${RED}[ERROR] Build failed. Please check the logs above.${NC}"
    exit 1
fi
echo -e "${GREEN}[OK] Build successful.${NC}"

# ------------------------------------------------------------------------------
# 4. Environment Setup & Launch
# ------------------------------------------------------------------------------
echo -e "${YELLOW}[3/3] Setting up environment and launching simulation...${NC}"

if [ -f "install/setup.bash" ]; then
    source install/setup.bash
else
    echo -e "${RED}[ERROR] install/setup.bash not found!${NC}"
    exit 1
fi

echo -e "${GREEN}=================================================${NC}"
echo -e "${GREEN}[READY] Launching mona_core bringup...${NC}"
echo -e "${BLUE}Press Ctrl+C to stop the simulation.${NC}"
echo -e "${GREEN}=================================================${NC}"

# Включаем принудительную раскраску
export RCUTILS_COLORIZED_OUTPUT=1

# Запускаем через exec, чтобы системные сигналы (SIGINT) корректно пробрасывались
exec ros2 launch mona_core bringup.launch.py