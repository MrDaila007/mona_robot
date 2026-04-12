# Gamepad Setup and Debugging (DualSense / Xbox)

> **Hardware Input Configuration**
> 
> When connecting modern gamepads (specifically the Sony DualSense) via Bluetooth, the Linux kernel often enumerates the device as multiple virtual controllers. The kernel driver maps the physical buttons, the touchpad, and the motion sensors (gyroscope/accelerometer) to completely separate `/dev/input/js*` ports.

If the simulation is mistakenly pointed to the motion sensor port, the `joy_node` will flood the ROS 2 DDS network with data at 200-300 Hz, causing severe bandwidth saturation and triggering false positive timeouts within the `twist_mux` safety logic.

## Locating the Correct Controller Port

**Step 1: Install Joystick Testing Utilities**
For Ubuntu/Debian:
```bash
sudo apt-get update && sudo apt-get install joystick
```
For Arch Linux:
```bash
sudo pacman -Syu joyutils
```

**Step 2: List Available Input Devices**
```bash
ls /dev/input/ | grep js

# Expected Output Example:
# js0
# js1
```

**Step 3: Test and Identify the Correct Port**
Execute `jstest` for the first available port. Move the analog sticks and press the physical buttons to verify response data:
```bash
jstest /dev/input/js0

# Example 1: The Correct Port (Physical Buttons & Axes)
Driver version is 2.1.0.
Joystick (DualSense Wireless Controller) has 8 axes (X, Y, Z, Rx, Ry, Rz, Hat0X, Hat0Y)
and 13 buttons (BtnA, BtnB, BtnX, BtnY, BtnTL, BtnTR, BtnTL2, BtnTR2, BtnSelect, BtnStart, BtnMode, BtnThumbL, BtnThumbR).
Testing ... (interrupt to exit)
Axes:  0:  1032  1:  -517  2:-32767  3:   774  4: -1291  5:-32767  6:     0  7:     0 Buttons:  0:off  1:off  2:off  3:off  4:off  5:off  6:off  7:off  8:off  9:off 10:off 11:off 12:off

# Example 2: The Incorrect Port (Motion Sensors / Gyroscope)
Driver version is 2.1.0.
Joystick (DualSense Wireless Controller Motion Sensors) has 6 axes (X, Y, Z, Rx, Ry, Rz)
and 0 buttons ().
Testing ... (interrupt to exit)
Axes:  0:  -175  1:  8436  2:  1219  3:    -3  4:    -4  5:    -1
```
_If you see "Motion Sensors" and rapidly fluctuating axes without touching the controller, **do not use this port**._

**Step 4: Update the Configuration** If your physical controller is mapped to `js1` instead of the default `js0`, update the `dev` parameter in `src/mona_core/launch/teleop.launch.py`.
```python
Node(
	package="joy",
	executable="joy_node",
	name="joy_node",
	parameters=[
		{
			"dev": "/dev/input/js0",  # Please specify the correct port here.
			"deadzone": 0.05,
			"autorepeat_rate": 50.0,
			"use_sim_time": use_sim_time,
		}
	],
),
```