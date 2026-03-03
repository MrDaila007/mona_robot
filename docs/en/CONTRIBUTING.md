# Contributing Guidelines

> **Quality Gate & Governance**
>
> Every commit can change the behavior of a physical system weighing hundreds of kilograms. Code quality is a safety concern.

## 1. Git Flow

We use task‑based branching. Direct commits to `main` are forbidden.

**Branch naming:**

* `feat/name` — new feature.
* `fix/name` — bug fix.
* `refactor/name` — refactoring without behavior change.
* `docs/name` — documentation updates.

**Commit messages:**

Follow the **Conventional Commits** specification:

* `feat: add lidar safety check`
* `fix: resolve null pointer exception in navigation`
* `docs: update installation instructions`

## 2. Pull Request (PR) process

1. Make sure `./scripts/local_ci.bash` passes locally.
2. The PR must contain a clear description of the changes.
3. The PR must be reviewed by at least one other developer (code review).
4. CI (GitHub Actions) must be green.

## 3. Safety and architecture standards (Safety & Compliance)

The MONA architecture is designed with industrial AMR safety standards in mind.

#### 1. ISO 3691‑4 (Behavior and environment)

*The main safety standard for driverless industrial trucks.*

* **Impact on code:** Defines the logic for lidar safety zones, braking distances and speed limits depending on the environment.  
* **Implementation:** The navigation stack must support dynamic costmaps and slowdown zones.

#### 2. ISO 13849‑1 (Control system reliability)

*Functional safety standard (Performance Levels – PL).*

* **Impact on code:** Requires diagnostic and fault‑tolerance mechanisms.  
* **Implementation:**
  * **Watchdog timers:** All control nodes must have watchdogs (dead man’s switch).
  * **Lifecycle management:** Use managed nodes to guarantee correct driver initialization.
  * **QoS:** Use `Reliability: Reliable` for critical topics (E‑Stop, `/cmd_vel`).

#### 3. VDA 5050 (Interoperability)

*Standard interface between AGV/AMR and the central fleet manager.*

* **Impact on code:** Unifies the robot state machine and data exchange protocols (MQTT).  
* **Implementation:** The `mona_core` architecture should allow easy integration of an `mqtt_bridge` that translates ROS 2 messages to VDA format.

## 4. Fault‑tolerance testing (Software‑In‑The‑Loop)

It is **strictly forbidden** to leave test flags in production code (e.g. `if (simulate_fault) drop_data()`).

FDIR (fault injection) testing must be done using standard OS tools and ROS 2 lifecycle.

To simulate a sensor connection loss during simulation, use the console:

```bash
# Simulate a stuck sensor (transition to Inactive)
ros2 lifecycle set /mona_lidar_merger deactivate

# Simulate a fatal process failure (segfault)
ros2 lifecycle set /safety_node shutdown
```

After running these commands you should see FDIR activation in the logs, power cut‑off and automatic recovery attempts.

---

## Standard workflow

#### Step 1. Get the latest version

```bash
git checkout main
git pull origin main
````

#### Step 2. Create a feature branch and commit

Name the branch clearly using the prefixes above.

```bash
git checkout -b feat/my-new-feature
```

#### Step 3. Save your changes

```bash
# Add all modified files or specific paths
git add .
# OR
git add src/my_cool_pkg

# Create a commit
git commit -m "feat: implement basic velocity controller"
```

#### Step 4. Push to remote

```bash
git push -u origin feat/my-new-feature
```

After that, go to GitHub and click **"Compare & pull request"**.  
A maintainer will review your PR. CI will run tests and, if everything is green, your changes will be merged into `main`.

