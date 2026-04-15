#!/usr/bin/env python3

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
# MONA FDIR Manager (Fault Detection, Isolation, and Recovery)
# ==============================================================================

import os
import yaml
import time
import rclpy
from rclpy.node import Node
from std_msgs.msg import Bool, String
from lifecycle_msgs.srv import ChangeState, GetState
from lifecycle_msgs.msg import Transition, State
from ament_index_python.packages import get_package_share_directory
from rclpy.qos import QoSProfile, DurabilityPolicy, ReliabilityPolicy

# Colors for console output formatting
COLOR_BLUE = "\033[94m"
COLOR_BOLD_RED = "\033[1;31m"
COLOR_RESET = "\033[0m"

# ==============================================================================
# GLOBAL CONSTANTS & TUNING PARAMETERS
# ==============================================================================
STARTUP_TIMEOUT_BUFFER_SEC = (
    10.0  # Buffer time added to expected startup time before declaring a boot failure
)
WAKEUP_RETRY_INTERVAL_SEC = 2.0  # Time between consecutive wakeup requests
MAX_WAKEUP_ATTEMPTS = (
    5  # Maximum number of software wakeup requests before declaring DEFECTIVE
)
SOFTWARE_DISCHARGE_TIME = (
    0.5  # Virtual "discharge" time for software cleanup operations
)

# Security: Allowed standard ROS 2 Lifecycle State IDs (0 to 15)
# Any ID outside this set is treated as memory corruption or a cybersecurity breach.
VALID_LIFECYCLE_STATES = {0, 1, 2, 3, 4, 10, 11, 12, 13, 14, 15}


class SystemHealthState:
    """
    Enumerates the global system health states.
    Dictates whether the robot can move and how hardware contactors react.
    """

    SYSTEM_STARTUP = "SYSTEM_STARTUP"  # Initialization in progress. Contactors OFF.
    SOFTWARE_OK = "SOFTWARE_OK"  # Normal operation.
    DEGRADED = "DEGRADED"  # Operating at reduced velocities.
    RECONFIGURED = "RECONFIGURED"  # Reconfigured mode (e.g., driving in reverse).
    PROTECTIVE_STOP = (
        "PROTECTIVE_STOP"  # Stopped, waiting for a PRIMARY node to reboot.
    )
    EMERGENCY = "EMERGENCY"  # Fatal error, hardware contactors opened.


class ModuleRecoveryState:
    """
    Finite State Machine (FSM) states for individual component recovery.
    """

    INITIAL_STARTUP = "INITIAL_STARTUP"  # Initial boot sequence upon robot power-up.
    NOMINAL = "NOMINAL"  # Operating nominally.
    POWER_OFF = "POWER_OFF"  # Initiating power cutoff or software cleanup.
    WAIT_DISCHARGE = (
        "WAIT_DISCHARGE"  # Waiting for capacitors to discharge or software to halt.
    )
    POWER_ON = "POWER_ON"  # Restoring power or software initiation.
    WAIT_BOOT = "WAIT_BOOT"  # Waiting for the sensor OS/firmware to boot.
    WAKEUP_RETRY = "WAKEUP_RETRY"  # Attempting ROS 2 lifecycle activation.
    DEFECTIVE = "DEFECTIVE"  # Component permanently failed.


class MonitoredComponent:
    """
    Represents a single monitored hardware or software component in the FDIR system.
    Tracks its lifecycle state and handles recovery sequences automatically.
    """

    def __init__(
        self, name, tier, reset_mechanism, reset_topic, startup_time, node_ref
    ):
        """
        Initializes the monitored component.

        @param name:            The ROS 2 node name of the component.
        @param tier:            Criticality level ("FATAL", "PRIMARY", "AUXILIARY").
        @param reset_mechanism: Mechanism to use for recovery ("hardware_relay" or "lifecycle_service").
        @param reset_topic:     The ROS topic string for the hardware relay (empty for software).
        @param startup_time:    Expected duration in seconds for the component to fully boot.
        @param node_ref:        Reference to the parent FDIRManager node (used for logging and client creation).
        """
        self.name = name
        self.tier = tier
        self.reset_mechanism = reset_mechanism
        self.reset_topic = reset_topic
        self.startup_time = startup_time
        self.node_ref = node_ref

        # Only create a hardware publisher if the mechanism is hardware_relay
        if self.reset_mechanism == "hardware_relay" and self.reset_topic:
            self.power_pub = self.node_ref.create_publisher(Bool, self.reset_topic, 1)
        else:
            self.power_pub = None

        self.get_state_client = self.node_ref.create_client(
            GetState, f"{self.name}/get_state"
        )
        self.change_state_client = self.node_ref.create_client(
            ChangeState, f"{self.name}/change_state"
        )

        self.is_alive = False
        self.is_compromised = False  # Security flag for unknown/malicious states
        self.time_lost = time.time()
        self.rec_state = ModuleRecoveryState.INITIAL_STARTUP
        self.state_timer_start = time.time()
        self.wakeup_attempts = 0
        self.last_wakeup_try_time = 0.0

        self.pending_state_future = None
        self.pending_change_future = None
        self.last_known_state = None

    def update_state_async(self):
        """
        Asynchronously queries the component's ROS 2 lifecycle state.
        Validates the state ID against known ROS 2 states to prevent injection/corruption.

        @return: None
        """
        if self.pending_change_future is not None:
            if self.pending_change_future.done():
                self.pending_change_future = None
            return

        if self.pending_state_future is not None:
            if self.pending_state_future.done():
                try:
                    res = self.pending_state_future.result()
                    state_id = res.current_state.id

                    # SECURITY & INTEGRITY CHECK
                    if state_id not in VALID_LIFECYCLE_STATES:
                        self.node_ref.get_logger().fatal(
                            f"[{self.name}] SECURITY ALERT: Unknown/Corrupted state ID '{state_id}' detected!"
                        )
                        self.is_compromised = True
                        self.last_known_state = None
                    else:
                        self.is_compromised = False
                        self.last_known_state = state_id

                except Exception as e:
                    self.node_ref.get_logger().error(
                        f"[{self.name}] GetState Error: {e}"
                    )
                    self.last_known_state = None

                self.pending_state_future = None
            return

        if not self.get_state_client.service_is_ready():
            self.last_known_state = None
            return

        req = GetState.Request()
        self.pending_state_future = self.get_state_client.call_async(req)

    def mark_dead(self, current_time):
        """
        Flags the component as disconnected/inactive.

        @param current_time: Current system timestamp.
        """
        if self.is_alive:
            self.is_alive = False
            self.time_lost = current_time
            self.node_ref.get_logger().warn(f"[{self.name}] LOST CONNECTION!")

    def mark_alive(self):
        """
        Flags the component as fully operational and resets recovery attempts.
        """
        if not self.is_alive:
            self.is_alive = True
            self.rec_state = ModuleRecoveryState.NOMINAL
            self.wakeup_attempts = 0
            self.node_ref.get_logger().info(f"[{self.name}] IS ACTIVE AND READY!")

    def process_recovery(self, current_time, power_off_duration, timeout_before_reboot):
        """
        Executes the FSM logic to recover the component based on its current recovery state.

        @param current_time:            Current system timestamp.
        @param power_off_duration:      Global configuration for hardware discharge time.
        @param timeout_before_reboot:   Global configuration for watchdog timeout.
        """
        if self.is_alive:
            return

        if self.rec_state == ModuleRecoveryState.INITIAL_STARTUP:
            if current_time - self.last_wakeup_try_time >= WAKEUP_RETRY_INTERVAL_SEC:
                self.last_wakeup_try_time = current_time
                self.send_wakeup_request()

                # Magic number replaced with defined constant
                if (
                    current_time - self.state_timer_start
                    > self.startup_time + STARTUP_TIMEOUT_BUFFER_SEC
                ):
                    self.node_ref.get_logger().warn(
                        f"[{self.name}] Failed to start initially. Moving to REBOOT SEQUENCE."
                    )
                    self.rec_state = ModuleRecoveryState.POWER_OFF
            return

        if (
            self.rec_state == ModuleRecoveryState.NOMINAL
            and (current_time - self.time_lost) > timeout_before_reboot
        ):
            self.node_ref.get_logger().warn(
                f"[{self.name}] Timeout reached. INITIATING {self.reset_mechanism.upper()} REBOOT!"
            )
            self.rec_state = ModuleRecoveryState.POWER_OFF

        if self.rec_state == ModuleRecoveryState.POWER_OFF:
            if self.reset_mechanism == "hardware_relay" and self.power_pub:
                self.power_pub.publish(Bool(data=False))
                self.node_ref.get_logger().info(f"[{self.name}] HARDWARE POWER CUT.")
            else:
                self.node_ref.get_logger().info(
                    f"[{self.name}] SOFTWARE LIFECYCLE CLEANUP INITIATED."
                )
                self.send_cleanup_request()

            self.state_timer_start = current_time
            self.rec_state = ModuleRecoveryState.WAIT_DISCHARGE

        elif self.rec_state == ModuleRecoveryState.WAIT_DISCHARGE:
            discharge_time = (
                power_off_duration
                if self.reset_mechanism == "hardware_relay"
                else SOFTWARE_DISCHARGE_TIME
            )
            if current_time - self.state_timer_start >= discharge_time:
                self.rec_state = ModuleRecoveryState.POWER_ON

        elif self.rec_state == ModuleRecoveryState.POWER_ON:
            if self.reset_mechanism == "hardware_relay" and self.power_pub:
                self.power_pub.publish(Bool(data=True))
                self.node_ref.get_logger().info(
                    f"[{self.name}] HARDWARE POWER RESTORED. Waiting {self.startup_time}s for boot..."
                )
            else:
                self.node_ref.get_logger().info(
                    f"[{self.name}] SOFTWARE INITIALIZATION. Waiting {self.startup_time}s..."
                )

            self.state_timer_start = current_time
            self.rec_state = ModuleRecoveryState.WAIT_BOOT

        elif self.rec_state == ModuleRecoveryState.WAIT_BOOT:
            if current_time - self.state_timer_start >= self.startup_time:
                self.wakeup_attempts = 0
                self.rec_state = ModuleRecoveryState.WAKEUP_RETRY

        elif self.rec_state == ModuleRecoveryState.WAKEUP_RETRY:
            if current_time - self.last_wakeup_try_time >= WAKEUP_RETRY_INTERVAL_SEC:
                if self.wakeup_attempts >= MAX_WAKEUP_ATTEMPTS:
                    self.node_ref.get_logger().fatal(
                        f"{COLOR_BOLD_RED}[{self.name}] {MAX_WAKEUP_ATTEMPTS} WAKEUP ATTEMPTS FAILED. DEFECT DECLARED!{COLOR_RESET}"
                    )
                    self.rec_state = ModuleRecoveryState.DEFECTIVE
                else:
                    self.wakeup_attempts += 1
                    self.last_wakeup_try_time = current_time
                    self.node_ref.get_logger().info(
                        f"[{self.name}] Wakeup attempt {self.wakeup_attempts}/{MAX_WAKEUP_ATTEMPTS}..."
                    )
                    self.send_wakeup_request()

    def send_cleanup_request(self):
        """
        Sends a ROS 2 lifecycle transition request to cleanly deactivate and unconfigure the node.
        """
        if not self.change_state_client.service_is_ready():
            return

        req = ChangeState.Request()
        if self.last_known_state == State.PRIMARY_STATE_ACTIVE:
            req.transition.id = Transition.TRANSITION_DEACTIVATE
        elif self.last_known_state == State.PRIMARY_STATE_INACTIVE:
            req.transition.id = Transition.TRANSITION_CLEANUP
        else:
            return

        self.pending_change_future = self.change_state_client.call_async(req)

    def send_wakeup_request(self):
        """
        Sends a ROS 2 lifecycle transition request to configure and activate the node.
        """
        if not self.change_state_client.service_is_ready():
            return

        req = ChangeState.Request()
        if self.last_known_state == State.PRIMARY_STATE_UNCONFIGURED:
            req.transition.id = Transition.TRANSITION_CONFIGURE
            self.node_ref.get_logger().info(
                f"[{self.name}] Sending CONFIG transition..."
            )
        elif self.last_known_state == State.PRIMARY_STATE_INACTIVE:
            req.transition.id = Transition.TRANSITION_ACTIVATE
            self.node_ref.get_logger().info(
                f"[{self.name}] Sending ACTIVATE transition..."
            )
        else:
            return

        self.pending_change_future = self.change_state_client.call_async(req)


class FDIRManager(Node):
    """
    Main manager node. Reads configuration from YAML and orchestrates all components.
    """

    def __init__(self):
        """
        Constructor. Loads FDIR policies, initializes publishers, and creates component objects.
        """
        super().__init__("mona_fdir_manager")

        pkg_share = get_package_share_directory("mona_core")
        config_path = os.path.join(pkg_share, "configs", "fdir_policy.yaml")

        try:
            with open(config_path, "r") as file:
                yaml_data = yaml.safe_load(file)
            params = yaml_data["fdir_manager"]["ros__parameters"]
        except Exception as e:
            self.get_logger().fatal(
                f"{COLOR_BOLD_RED}Failed to load fdir_policy.yaml: {e}{COLOR_RESET}"
            )
            raise e

        self.power_off_time = params.get("recovery_power_off_time", 3.0)
        self.timeout_reboot = params.get("timeout_before_reboot", 5.0)

        self.health_state_pub = self.create_publisher(String, "system/health_state", 10)
        qos_profile = QoSProfile(
            depth=1,
            reliability=ReliabilityPolicy.RELIABLE,
            durability=DurabilityPolicy.TRANSIENT_LOCAL,
        )
        self.emergency_contactor_pub = self.create_publisher(
            Bool, "hardware/contactor_cmd", qos_profile
        )

        self.components = {}
        components_config = params.get("components", {})

        for comp_key, comp_data in components_config.items():
            if not comp_data.get("enabled", True):
                self.get_logger().info(
                    f"Skipping disabled component: {comp_data.get('node_name', comp_key)}"
                )
                continue

            node_name = comp_data["node_name"]
            reset_mechanism = comp_data.get("reset_mechanism", "lifecycle_service")
            reset_topic = comp_data.get("reset_topic", "")

            self.components[node_name] = MonitoredComponent(
                name=node_name,
                tier=comp_data["tier"],
                reset_mechanism=reset_mechanism,
                reset_topic=reset_topic,
                startup_time=comp_data.get("startup_time", 5.0),
                node_ref=self,
            )

        self.system_fully_active = False
        self.timer = self.create_timer(0.2, self.health_eval_tick)
        self.get_logger().info(
            f"FDIR Manager started. Tracking {len(self.components)} components."
        )

    def health_eval_tick(self):
        """
        Timer callback executed at 5Hz. Evaluates global system health based on the state
        of all tracked components and triggers emergency contacts if necessary.
        """
        current_time = time.time()
        global_state = SystemHealthState.SOFTWARE_OK

        is_starting = False
        all_active = True
        security_breach_detected = False

        for comp in self.components.values():
            comp.update_state_async()

        for name, comp in self.components.items():
            # Security Check Evaluation
            if comp.is_compromised:
                security_breach_detected = True

            is_active = comp.last_known_state == State.PRIMARY_STATE_ACTIVE

            if comp.rec_state == ModuleRecoveryState.INITIAL_STARTUP:
                is_starting = True

            if not is_active:
                all_active = False
                comp.mark_dead(current_time)
                comp.process_recovery(
                    current_time, self.power_off_time, self.timeout_reboot
                )

                if comp.tier == "FATAL":
                    global_state = SystemHealthState.EMERGENCY
                elif (
                    comp.tier == "PRIMARY"
                    and global_state != SystemHealthState.EMERGENCY
                ):
                    if comp.rec_state == ModuleRecoveryState.DEFECTIVE:
                        global_state = SystemHealthState.RECONFIGURED
                    else:
                        global_state = SystemHealthState.PROTECTIVE_STOP
                elif comp.tier == "AUXILIARY" and global_state not in [
                    SystemHealthState.EMERGENCY,
                    SystemHealthState.PROTECTIVE_STOP,
                ]:
                    global_state = SystemHealthState.DEGRADED
            else:
                comp.mark_alive()

        # If a security/integrity breach was detected, OVERRIDE all logic and kill the system.
        if security_breach_detected:
            global_state = SystemHealthState.EMERGENCY
            all_active = False
            self.get_logger().fatal(
                f"{COLOR_BOLD_RED}SECURITY OVERRIDE! UNKNOWN COMPONENT STATES. FORCING EMERGENCY STOP.{COLOR_RESET}"
            )

        if is_starting and not security_breach_detected:
            global_state = SystemHealthState.SYSTEM_STARTUP

        self.health_state_pub.publish(String(data=global_state))

        # HARDWARE REDUNDANCY (Bold red FATAL logging)
        if global_state == SystemHealthState.EMERGENCY:
            self.emergency_contactor_pub.publish(Bool(data=False))
            if int(current_time * 5) % 5 == 0:
                self.get_logger().fatal(
                    f"{COLOR_BOLD_RED}EMERGENCY STATE ACTIVE! BROADCASTING HARDWARE CONTACTOR CUTOFF!{COLOR_RESET}"
                )

        # Highlighted blue output when the system is fully operational
        if all_active and not self.system_fully_active:
            self.system_fully_active = True
            self.get_logger().info(
                f"{COLOR_BLUE}ALL MODULES STARTED. SYSTEM READY.{COLOR_RESET}"
            )
        elif not all_active:
            self.system_fully_active = False


def main(args=None):
    """
    Node entrypoint.
    """
    rclpy.init(args=args)
    manager = FDIRManager()

    try:
        rclpy.spin(manager)
    except KeyboardInterrupt:
        manager.get_logger().info("Keyboard Interrupt. Shutting down FDIR manager...")
    finally:
        manager.destroy_node()
        rclpy.shutdown()


if __name__ == "__main__":
    main()
