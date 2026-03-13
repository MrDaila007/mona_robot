#!/bin/bash

# ==============================================================================
# Script Name: run_sim.bash
# Description: Full cycle workspace preparation and simulation launch.
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

set -e

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

echo -e "${BLUE}[INFO] Initializing Mona Robot Simulation sequence...${NC}"

# Graphic and Network Compatibility Overrides
export QT_QPA_PLATFORM=xcb
export GDK_BACKEND=x11
export IGN_GAZEBO_RENDER_ENGINE_GUI=ogre
export IGN_GAZEBO_RENDER_ENGINE_SERVER=ogre
export IGN_IP=127.0.0.1
export ROS_LOCALHOST_ONLY=1
export RCUTILS_COLORIZED_OUTPUT=1


# 1. Pre-flight Check
if [ ! -d "src" ]; then
    echo -e "${RED}[ERROR] Directory 'src' not found. Execute from workspace root.${NC}"
    exit 1
fi

# 2. Workspace Cleanup
echo -e "${YELLOW}[1/3] Cleaning workspace directories...${NC}"
rm -rf build/ install/ log/
echo -e "${GREEN}[OK] Workspace cleaned.${NC}"

# 3. Build Process
echo -e "${YELLOW}[2/3] Building packages...${NC}"
if ! colcon build --cmake-clean-cache --symlink-install; then
    echo -e "${RED}[ERROR] Build process failed. Inspect logs above.${NC}"
    exit 1
fi
echo -e "${GREEN}[OK] Build completed successfully.${NC}"

# 4. Environment Setup
echo -e "${YELLOW}[3/3] Configuring environment variables...${NC}"
if [ -f "install/setup.bash" ]; then
    source install/setup.bash
else
    echo -e "${RED}[ERROR] install/setup.bash is missing.${NC}"
    exit 1
fi

echo -e "${GREEN}=================================================${NC}"
echo -e "${GREEN}[READY] Launching mona_core bringup...${NC}"
echo -e "${BLUE}Press Ctrl+C to terminate the simulation.${NC}"
echo -e "${GREEN}=================================================${NC}"

exec ros2 launch mona_core bringup.launch.py