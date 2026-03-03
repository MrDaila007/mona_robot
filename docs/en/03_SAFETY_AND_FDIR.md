# Safety Architecture and FDIR (Safety Pipeline)

> **Fault Detection, Isolation, and Recovery (FDIR)**
>
> The MONA autonomous robot uses a hybrid two‑level safety system. It is designed to tolerate software and hardware failures (graceful degradation) while preventing harm to the environment or the robot itself.

## 1. Hybrid safety architecture

The architecture is split into two independent loops to meet **ISO 13849‑1** hardware redundancy requirements:

1. **Safety Node (C++ / Low‑level)**: Runs at high frequency (100 Hz). Responsible for velocity control, hardware E‑Stop handling, command interpolation (watchdog) and immediate contactor opening.
2. **FDIR Manager (Python / High‑level)**: Acts as a *lifecycle manager*. Slow loop (5 Hz) that polls nodes, reads `fdir_policy.yaml` and performs hardware power‑cycle for stuck sensors.

### Hardware redundancy diagram

Both nodes have direct access to the contactor relay. If the `safety_node` core crashes (segfault), the `fdir_manager` takes over and continuously sends cut‑off signals on the hardware bus.

## 2. Node criticality tiers

Robot behavior on failure depends on which component has crashed. This is described in `fdir_policy.yaml`:

- **FATAL (Critical)**: Motor controllers, main safety node. If they fail, the robot is uncontrollable.  
  - _Reaction_: Immediate **EMERGENCY STOP**. Power to contactors is cut.

- **PRIMARY (Primary)**: Main lidar, odometry.  
  - _Reaction_: Transition to **PROTECTIVE STOP**. The robot is stopped in software (motors hold position). The sensor is power‑cycled for recovery.

- **AUXILIARY (Auxiliary)**: Rear/side lidars, IMU.  
  - _Reaction_: Transition to **DEGRADED MODE**. The robot continues moving with reduced speed limits. Sensor recovery runs in the background.

## 3. FDIR state machine (Recovery process)

When a sensor fails, the automatic recovery process `ModuleRecoveryState` is triggered. It includes fully powering the component off and on again.

## 4. Stop categories

We distinguish two stop types:

1. **Soft stop (Protective Stop)**
   - Triggered by temporary joystick network loss (watchdog > 0.5 s) or failure of primary sensors.
   - Contactors **REMAIN CLOSED**.
   - Motors operate in holding / active braking mode to prevent uncontrolled rolling.

2. **Hard stop (Emergency Stop)**
   - Triggered by software failure (FATAL node segfault), robot motion during protective stop (hardware feedback) or pressing the red E‑Stop button.
   - Contactors **OPEN**.
   - Power to drivers is removed and mechanical brakes engage.

