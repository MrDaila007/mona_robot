#!/bin/bash
set -e

# ==============================================================================
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

# ==============================================================================
# Script:   entrypoint.bash
# Purpose:  Source ROS 2 environments and execute the provided command.
# ==============================================================================

source /opt/ros/humble/setup.bash

if [ -f "$HOME/mona_ws/install/setup.bash" ]; then
    source "$HOME/mona_ws/install/setup.bash"
fi

exec "$@"