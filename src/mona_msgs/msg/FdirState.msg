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

# Global System Health State governed by FDIR Manager

std_msgs/Header header

# Enumeration of valid states
uint8 STATE_SYSTEM_STARTUP = 0
uint8 STATE_SOFTWARE_OK = 1
uint8 STATE_DEGRADED = 2
uint8 STATE_RECONFIGURED = 3
uint8 STATE_PROTECTIVE_STOP = 4
uint8 STATE_EMERGENCY = 5

uint8 current_state
string diagnostic_message   # (e.g., "Lidar lost connection")