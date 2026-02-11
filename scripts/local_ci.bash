#!/bin/bash

# ==============================================================================
# Script Name: local_ci.bash
# Description: Automates the Clean -> Build -> Test cycle for ROS 2 workspace
# Usage:       Run from the workspace root (e.g., ./scripts/local_ci.bash)
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

echo -e "${BLUE}[INFO] Starting Local Continuous Integration sequence...${NC}"


# ------------------------------------------------------------------------------
# 0. Pre-flight Check
# ------------------------------------------------------------------------------
# Check if we are in the workspace root by looking for the 'src' directory.
if [ ! -d "src" ]; then
    echo -e "${RED}[ERROR] Directory 'src' not found.${NC}"
    echo "Please run this script from the root of your ROS 2 workspace."
    exit 1
fi


# ------------------------------------------------------------------------------
# 1. Auto-format Code (Fix style before testing)
# ------------------------------------------------------------------------------
echo -e "${YELLOW}[1/4] Auto-formatting code to match standards...${NC}"

# Исправляем C++ (используем конфиг uncrustify)
if command -v ament_uncrustify &> /dev/null; then
    ament_uncrustify -c configs/uncrustify.cfg --reformat src/ > /dev/null 2>&1
    echo -e "${GREEN}[OK] C++    formatted.${NC}"
else
    echo -e "${RED}[WARN] ament_uncrustify not found, skipping C++ format.${NC}"
fi

# Исправляем Python (используем autopep8)
if command -v autopep8 &> /dev/null; then
    autopep8 --in-place --recursive --max-line-length 120 src/
    echo -e "${GREEN}[OK] Python formatted.${NC}"
else
    # Если autopep8 не в PATH, попробуем запустить через python3 -m
    python3 -m autopep8 --in-place --recursive --global-config configs/setup.cfg src/
    echo -e "${GREEN}[OK] Python formatted (via module).${NC}"
fi

# Удаляем пробелы в конце строк (trailing whitespace) во всех CMakeLists.txt
find src -name "CMakeLists.txt" -exec sed -i 's/[[:space:]]*$//' {} +


# ------------------------------------------------------------------------------
# 2. Clean Workspace
# ------------------------------------------------------------------------------
echo -e "${YELLOW}[2/4] Cleaning workspace (build, install, log)...${NC}"

rm -rf build/ install/ log/

# Verify if clean was successful
if [ $? -ne 0 ]; then
    echo -e "${RED}[ERROR] Failed to clean directories.${NC}"
    exit 1
fi


# ------------------------------------------------------------------------------
# 3. Build Packages
# ------------------------------------------------------------------------------
echo -e "${YELLOW}[3/4] Building packages...${NC}"

# Run colcon build
colcon build --cmake-clean-cache --symlink-install

# Check the exit code of the build process
if [ $? -ne 0 ]; then
    echo -e "${RED}[ERROR] Build failed.${NC}"
    exit 1
fi


# ------------------------------------------------------------------------------
# 4. Run Tests
# ------------------------------------------------------------------------------
echo -e "${YELLOW}[4/4] Running tests...${NC}"

# Source the newly built workspace to ensure tests see the environment
if [ -f "install/setup.bash" ]; then
    source install/setup.bash
else
    echo -e "${RED}[ERROR] install/setup.bash not found. Build might have failed silently.${NC}"
    exit 1
fi

# Execute tests
# Note: 'colcon test' usually returns 0 even if tests fail (it just runs them).
# We check the actual results in the next step.
colcon test

if [ $? -ne 0 ]; then
    echo -e "${RED}[ERROR] Failed to execute test runner.${NC}"
    exit 1
fi


# ------------------------------------------------------------------------------
# 5. Verify Results
# ------------------------------------------------------------------------------
echo -e "${YELLOW}Verifying test results...${NC}"

# 'colcon test-result' returns a non-zero exit code if any test failed
colcon test-result --verbose

if [ $? -eq 0 ]; then
    echo -e "${GREEN}====================================================${NC}"
    echo -e "${GREEN}[SUCCESS] All checks passed. Build and Tests are OK.${NC}"
    echo -e "${GREEN}====================================================${NC}"
    
    echo -e "\n${BLUE}Recommended next steps:${NC}"
    echo -e "  1. Create a new feature branch: ${YELLOW}git checkout -b <branch-name>${NC}"
    echo -e "  2. Commit your changes:         ${YELLOW}git commit -am \"your message\"${NC}"
    echo -e "  3. Push to origin:              ${YELLOW}git push origin <branch-name>${NC}"
    echo -e "  4. Open a Pull Request on GitHub."
    exit 0
else
    echo -e "${RED}=================================================${NC}"
    echo -e "${RED}[FAILURE] Tests failed. See details above.${NC}"
    echo -e "${RED}=================================================${NC}"
    
    echo -e "\n${YELLOW}[!] Please fix the linting or logic errors before committing.${NC}"
    exit 1
fi