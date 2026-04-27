# Manual Teleoperation (DualSense)

> **Human-in-the-Loop Control**
> 
> Manual control (`MANUAL` state) of the MONA robot is executed via a Sony DualSense (PS5) gamepad. To guarantee physical safety, all signals are routed through a strict FDIR and smoothing pipeline before reaching the motor controllers.

## 1. Kinematic Control Pipeline
1. **Raw Input (`joy_node`)**: Reads the host device `/dev/input/jsX`. It is configured to broadcast continuously (`autorepeat_rate: 50.0 Hz`) to satisfy the TwistMux communication watchdog.
2. **Axis Mapping (`teleop_twist_joy`)**: Translates raw button arrays and analog stick values into standardized `geometry_msgs/msg/Twist` vectors.
3. **Multiplexing (`twist_mux_node`)**: Receives the command on the `/[namespace]/cmd_teleop` topic. It asserts the highest priority, immediately preempting any active Nav2 goals.
4. **Smoothing & Validation (`safety_node`)**: The signal is processed by an Exponential Moving Average (EMA) filter to prevent mechanical jerking. The Safety Node validates the command against the current FDIR state (`EMERGENCY` lockouts) before publishing the final vector to `/[namespace]/hardware/motor_cmd` at 100 Hz.

---

## 2. DualSense Control Layout
* **L2 Trigger (Deadman Switch)**: The operator vigilance button. **Unless this trigger is fully depressed, all analog stick movements are strictly ignored** (in compliance with ISO 13849-1 safety standards).
* **Left Stick (Vertical)**: Linear Velocity Forward/Backward (Robot X-Axis).
* **Left Stick (Horizontal)**: Linear Velocity Left/Right (Strafing, Robot Y-Axis). *Note: Effective only on Mecanum-equipped chassis configurations.*
* **Right Stick (Horizontal)**: Angular Velocity (Rotation in place, Robot Yaw/Z-Axis).
* **R2 Trigger (Turbo Mode)**: Depressing this trigger while holding the L2 Deadman Switch removes standard software velocity limiters, allowing the chassis to achieve maximum mechanical speed.

---

## 3. Launching with Gamepad Support

By default, gamepad support is explicitly disabled. This ensures compatibility with automated CI pipelines and allows developers without physical controllers to run the simulation seamlessly. To activate the teleoperation stack, the `use_gamepad:=true` argument must be passed.

Within our Swarm infrastructure, this is handled automatically by the single-agent testing script:

```bash
./scripts/bringup_1_robot.bash
```

If you are launching a custom agent manually via Docker Compose, apply the argument as follows:
```bash
docker compose run --rm mona-robot ros2 launch mona_core robot.launch.py namespace:=mona_001 use_gamepad:=true
```

> [!NOTE]
> When operating within Docker, the compose configuration automatically mounts the host's device tree via volumes: - /dev/input:/dev/input:ro to grant the container read-only access to the physical gamepad.