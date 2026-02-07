#!/bin/bash

# ==============================================================================
# Script Name: local_ci.bash
# Description: Automates the Clean -> Build -> Test cycle for ROS 2 workspace.
# Usage:       Run from the workspace root (e.g., ./scripts/local_ci.bash)
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
# 1. Clean Workspace
# ------------------------------------------------------------------------------
echo -e "${YELLOW}[1/3] Cleaning workspace (build, install, log)...${NC}"

rm -rf build/ install/ log/

# Verify if clean was successful
if [ $? -ne 0 ]; then
    echo -e "${RED}[ERROR] Failed to clean directories.${NC}"
    exit 1
fi

# ------------------------------------------------------------------------------
# 2. Build Packages
# ------------------------------------------------------------------------------
echo -e "${YELLOW}[2/3] Building packages...${NC}"

# Run colcon build
colcon build --cmake-clean-cache --symlink-install

# Check the exit code of the build process
if [ $? -ne 0 ]; then
    echo -e "${RED}[ERROR] Build failed.${NC}"
    exit 1
fi

# ------------------------------------------------------------------------------
# 3. Run Tests
# ------------------------------------------------------------------------------
echo -e "${YELLOW}[3/3] Running tests...${NC}"

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
# 4. Verify Results
# ------------------------------------------------------------------------------
echo -e "${YELLOW}Verifying test results...${NC}"

# 'colcon test-result' returns a non-zero exit code if any test failed
colcon test-result --verbose

if [ $? -eq 0 ]; then
    echo -e "${GREEN}=================================================${NC}"
    echo -e "${GREEN}[SUCCESS] All checks passed. Build and Tests are OK.${NC}"
    echo -e "${GREEN}=================================================${NC}"
    exit 0
else
    echo -e "${RED}=================================================${NC}"
    echo -e "${RED}[FAILURE] Tests failed. See details above.${NC}"
    echo -e "${RED}=================================================${NC}"
    exit 1
fi