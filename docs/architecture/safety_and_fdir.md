# Safety Architecture and FDIR Pipeline

> **Fault Detection, Isolation, and Recovery (FDIR)**
> 
> The autonomous mobile robot MONA utilizes a hybrid, dual-layer safety system. It is engineered to withstand both software and hardware faults (Graceful Degradation), strictly preventing harm to the environment, personnel, or the robot itself.

## Regulatory Compliance and Industrial Standards

The MONA safety architecture is designed with strict adherence to industrial functional safety standards. While a standard ROS 2 implementation on a non-RTOS Linux kernel cannot be certified independently without hardware redundancy, we implement core patterns derived from the following directives:
* **IEC 61508 (Functional Safety):** Utilization of distinct degradation states (`DEGRADED`), an error escalation mechanism (FDIR), and an independent `SafetyNode` acting as the software Logic Solver.
* **ISO 13849-1 (Safety of Machinery):** Implementation of the Deadman Switch principle (L2 button on the teleoperation controller), 100Hz communication watchdog monitoring, and unconditional hardware contactor de-energization upon loss of velocity control.
* **ISO 26262 (Road Vehicles - Functional Safety):** Application of continuous Health State monitoring and the deterministic transition of the system into a Safe State (`PROTECTIVE_STOP` / `EMERGENCY`) upon anomaly detection.

---

## 1. Hybrid Safety Architecture

1. **Control Logic Echelon (`mona_control::TwistMuxNode`):** Operates at 100 Hz. Strictly responsible for routing and priority arbitration (`/cmd_teleop` vs `/cmd_nav`). It includes an asymptotic Exponential Moving Average (EMA) filter to smooth the peak kinetic loads of the heavy chassis during manual teleoperation. Upon gamepad override, this echelon explicitly dispatches a `CancelGoal` request to the Nav2 server.
2. **Hardware Safety Echelon (`mona_safety::SafetyNode`):** The supreme logical arbiter of the system (100 Hz). It performs no velocity filtering but maintains exclusive, direct access to the hardware contactor relays. It is protected against asynchronous "zombie-callbacks" via atomic state flags. If the agent is in a `PROTECTIVE_STOP` state but physical displacement is detected (via odometry feedback), this node instantly escalates the status to a hardware `EMERGENCY` and physically severs motor power.
3. **FDIR Manager (`mona_core`):** A deterministic, high-frequency Lifecycle Component (10 Hz) acting as the supreme watchdog within each agent's namespace (e.g., `/mona_001`). It monitors local C++ components (TwistMux, Safety, LidarMerger) via non-blocking `service_is_ready` evaluations. Utilizing **Fate Isolation**, components are deployed across three independent OS-level processes (`sensor_control_executor`, `isolated_safety_executor`, `isolated_fdir_executor`). This guarantees that a fatal segmentation fault in expendable perception algorithms cannot terminate the FDIR watchdog. Upon detecting a subsystem freeze, it orchestrates a hard power-cycle recovery sequence governed by the `fdir_policy.yaml`.

### Hardware Redundancy Diagram

Both controlling nodes possess independent access paths to the hardware contactor relays. If the `safety_node` process suffers a critical software failure (e.g., a `Segfault`), the isolated `fdir_manager` seizes hardware control, continuously broadcasting a cutoff signal and Zero Velocity Override via the redundant bus.
```mermaid
%%{init: {'theme': 'base', 'themeVariables': { 'primaryColor': '#e1f5fe', 'edgeLabelBackground':'#ffffff', 'tertiaryColor': '#fff'}}}%%
graph TD
    %% Input Interfaces
    subInter["Input Interfaces<br>(Teleop / Nav2)"]
    
    %% Software Layer (IPC)
    subgraph SW_Layer ["Compute Platform (IPC)"]
        direction LR
        SN[("SafetyNode<br>(C++ | 100Hz)")]
        FM[("FDIRManager<br>(C++ | 10Hz)")]
        Watchdog{{"Watchdog Monitor<br>Heartbeat Check"}}
        
        %% Monitoring Links
        SN -- Heartbeat --> Watchdog
        Watchdog -- Failure Detect --> FM
    end
    
    %% Data Streams
    subInter -. "Velocity Commands" .-> SN
    subInter -. "System Status" .-> FM
    
    %% Hardware Bridge / MCU
    subgraph HW_Bridge ["Hardware Bridge / MCU"]
        LogicOr{{"Virtual OR<br>(Override Logic)"}}
        IOCtrl["I/O Driver<br>(Relay Interface)"]
    end
    
    %% === CONTROL PATHS ===
    
    %% 1. Primary Path (C++) - Normal operation
    SN ==>|"Cmd: ENABLE"| LogicOr
    
    %% 2. Redundant Path (C++) - Emergency Override
    FM ==>|"ESCALATION: Zero Velocity & DISABLE"| LogicOr
    
    %% Final logic to driver
    LogicOr --> IOCtrl
    
    %% Power Electronics
    subgraph Physical_HW ["Physical Safety Circuit"]
        Relay["Physical Contactor Relay<br>Power to Drivers"]
        Dead["Motors DE-ENERGIZED<br>(Safe State)"]
    end
    
    %% Hardware Signal
    IOCtrl ==> Relay
    
    %% Result
    Relay -- "Emergency Cutoff" --> Dead
    
    %% === DIAGRAM STYLING ===
    %% Node Colors
    style SN fill:#e3f2fd,stroke:#1565c0,stroke-width:2px;
    style FM fill:#fff9c4,stroke:#fbc02d,stroke-width:2px;
    style Relay fill:#f44336,color:white,stroke:#b71c1c,stroke-width:2px;
    style Dead fill:#b71c1c,color:white,stroke:#b71c1c,stroke-width:2px;
    
    %% Link Colors and Types
    linkStyle 4 stroke:#1565c0,stroke-width:3px,label: PRIMARY;
    linkStyle 5 stroke:#f44336,stroke-width:4px,stroke-dasharray: 5 5,label: REDUNDANT;
    linkStyle 7 stroke:#f44336,stroke-width:3px;
    
    %% Subgraph Styles
    style SW_Layer stroke-width:2px,stroke-dasharray: 5 5;
    style Physical_HW fill:#ffebee,stroke:#b71c1c;
```

---

## 2. Node Criticality Levels (Tiers)

The robot's fallback behavior upon component failure depends strictly on the predefined tier of the failing node, as configured in `fdir_policy.yaml`:
- **FATAL**: Motor controllers, core `safety_node`. The robot is physically uncontrollable without these.
    - _Response_: Immediate **EMERGENCY STOP**. Complete hardware contactor power cutoff.
- **PRIMARY**: Main LiDAR, Odometry source.
    - _Response_: Transition to **PROTECTIVE STOP**. The robot halts via software (motors actively hold position). The failed sensor is sent a hardware power-cycle command.
- **AUXILIARY**: Rear/Side LiDARs, IMU.
    - _Response_: Transition to **DEGRADED MODE**. The robot continues navigating but enforces heavily restricted velocity limits. Background recovery processes attempt to reboot the sensor.

---

## 3. FDIR State Machine (Recovery Process)

When a sensor fails, the automated recovery sequence is initiated. To protect mechanical components and prevent infinite reboot loops, the Finite State Machine (FSM) adheres to an industrial **3-Strikes Rule**:
- **Detection:** If a node fails to report a state within the predefined tolerance (50ms) or transitions to an `ERROR` state, FDIR initiates the recovery sequence.
- **Recovery Loop (State Edge Logged):**
    - **WAIT_DISCHARGE:** FDIR signals the hardware to open relays, actively draining the sensor's internal capacitors to ensure a pristine cold boot.
    - **POWER_ON:** Hardware is re-energized.
    - **WAIT_BOOT:** FDIR suspends active polling, granting the component sufficient time to initialize and transition to an `ACTIVE` state.
- **Terminal Fault Escalation:** The system permits a maximum of 3 recovery iterations. If the 3rd attempt is unsuccessful, the node is permanently designated as `DEFECTIVE`. The overarching system escalates into an irreversible **EMERGENCY** state (Lockout), mandating a full manual physical restart by an operator.

---

## 4. Zero Velocity Override Mechanism

To preempt "Runaway Robot" scenarios - where a perception or control node might crash while a non-zero velocity command remains latched in the hardware bridge - the FDIR Manager incorporates a secondary failsafe known as **Zero Velocity Override**.
- Upon the escalation of the global state to `STATE_EMERGENCY` (due to a component `TERMINAL FAULT` or communication severance), the isolated FDIR process intercepts the command pipeline.
- It unconditionally and continuously publishes a `Twist` message populated with zero-values directly to the hardware bridge, overriding any latent trajectories and guaranteeing immediate deceleration regardless of the `TwistMux` state.

---

## 5. Stop Categories

The architecture distinguishes between two fundamental halt types:
1. **Soft Stop (Protective Stop)**
    - Triggered by temporary teleoperation network loss (Watchdog > 0.5s) or the failure of a PRIMARY sensor.
    - Contactors **REMAIN ENERGIZED**.
    - Motors operate in Active Braking mode (holding position torque) to prevent the chassis from rolling down inclines.
2. **Hard Stop (Emergency Stop)**
    - Triggered by software faults (FATAL node Segfault), Terminal Faults (exhaustion of the 3-Strikes recovery), physical displacement during a Protective Stop (Hardware Feedback mismatch), or a manual physical E-STOP button press.
    - Contactors are **OPENED (DE-ENERGIZED)**.
    - Power to the motor bridge drivers is severed, and physical mechanical brakes are engaged.

---

## 6. State Variables Architecture (Functional Safety Domains)

To comply with industrial safety standards (ISO 13849 / SIL), the control software is partitioned into isolated operational domains (Pattern: _Input -> Processing -> Output -> Infrastructure_). Every state variable is strictly bound to its respective domain, eliminating Race Conditions and logical collisions across the multi-process, Fate-Isolated environment.

All flags below are implemented as `std::atomic<bool>` to guarantee thread safety.
- **`hardware_button_pressed_` (Sensor / Input Domain)**
    - **Purpose:** Reflects the physical electrical state of the chassis E-STOP button.
    - **Rationale:** Serves as a hardware interlock against software resets. According to safety directives, remotely clearing a fault via the global fleet orchestrator ([LISA API](https://github.com/vladubase/lisa_api)) is strictly prohibited until a human operator physically releases the mushroom button on the agent.
- **`e_stop_active_` (Software Latch / Processing Domain)**
    - **Purpose:** The logical memory of a Level 2 EMERGENCY.
    - **Rationale:** Acts as a persistent latch. It can be triggered by external services even if the physical button is not pressed. When `true`, the node unconditionally drops all routine state updates (e.g., `NORMAL`, `DEGRADED`) from the FDIR subsystem. The robot remains paralyzed until an explicit call to the `/reset_e_stop` service is made.
- **`contactors_enabled_` (Actuator / Output Domain)**
    - **Purpose:** The actual commanded state of the high-voltage motor relays.
    - **Rationale:** Decoupled from `e_stop_active_` because de-energizing the motors is not exclusively an emergency reaction. During prolonged `IDLE` states or specific Protective Stops, contactors are deliberately opened to conserve power and prevent hardware degradation, without throwing the system into a critical E-STOP state.
- **`is_processing_allowed_` (ROS 2 Infrastructure / Lifecycle Domain)**
    - **Purpose:** An atomic gatekeeper permitting the execution of asynchronous callbacks.
    - **Rationale:** Intrinsically tied to the `rclcpp_lifecycle` architecture. Because ROS 2 topics are handled asynchronously by a thread pool, a message might arrive exactly as the node transitions to `Inactive`. This variable guarantees the suppression of "zombie-callbacks", strictly preventing a deactivated node from inadvertently dispatching commands to the hardware actuators.