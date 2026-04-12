# Contributing Guidelines

> **Quality Gate & Governance**
> 
> Every commit represents a potential change in the behavior of a physical device weighing hundreds of kilograms. Code quality is a matter of safety.

## 1. Git Flow
We utilize task-based branching. Direct commits to the `main` branch are strictly prohibited.
**Branch Naming Conventions:**
* `feat/name` — new functionality.
* `fix/name` — bug resolution.
* `refactor/name` — code restructuring without logic alteration.
* `docs/name` — documentation updates.

**Commit Messages:**
Adhere to the **Conventional Commits** specification:
* `feat: add lidar safety check`
* `fix: resolve null pointer exception in navigation`
* `docs: update installation instructions`

## 2. Pull Request (PR) Process
1. Ensure that the `./scripts/local_ci.bash` script passes successfully within your local development container (`docker compose exec dev bash`).
2. The PR must contain a clear and comprehensive description of the changes.
3. The PR must be reviewed and approved by at least one other developer (Code Review).
4. Continuous Integration (GitHub Actions) checks must pass.

## 3. Safety & Compliance Standards
The MONA architecture is designed to comply with strict industrial standards for Autonomous Mobile Robots (AMR).

#### 1. ISO 3691-4 (Behavior and Environment)
*The primary safety standard for Driverless Industrial Trucks.*
* **Code Impact:** Dictates the logic for switching LiDAR safety zones, braking distances, and speed limits based on the operating environment.
* **Implementation:** The navigation stack must support dynamic restriction maps (Costmaps) and deceleration zones.

#### 2. ISO 13849-1 (Control Reliability)
*Functional safety standard defining Performance Levels (PL).*
* **Code Impact:** Requires the implementation of failure diagnostics and fault tolerance mechanisms.
* **Implementation:**
    * **Watchdog Timers:** All control nodes must implement reset timers (Dead man's switch).
    * **Lifecycle Management:** Utilization of Managed Nodes to guarantee deterministic driver initialization.
    * **QoS Profiles:** Application of the `Reliability: Reliable` policy for critical safety topics (e.g., E-Stop, `/cmd_vel`).

#### 3. Integration with [LISA](https://github.com/vladubase/lisa_api) (Logistics Intelligence & Swarm API)
*Standardized interface for AMR fleet communication with the cloud server (based on VDA 5050 principles).*
* **Code Impact:** Unification of the robot state machine, utilization of dynamically assigned namespaces (`mona_001`, `mona_002`), and MQTT telemetry protocols.
* **Implementation:** Swarm agents must broadcast isolated telemetry and accept navigation goals from the global LISA planner.

## 4. Fault Tolerance Testing (Software-In-the-Loop)
**It is strictly prohibited** to leave test flags in production code (e.g., `if (simulate_fault) drop_data()`).
FDIR testing (Fault Injection) must be performed using standard OS tools and ROS 2 Lifecycle transitions.
```bash
# Simulate sensor freeze (transition to Inactive)
ros2 lifecycle set /mona_lidar_merger deactivate

# Simulate fatal process crash (Segfault)
ros2 lifecycle set /safety_node shutdown
```
After executing the command, you should observe the FDIR activation process in the logs, including hardware contactor cutoffs and automatic recovery attempts.

---
## General Workflow:

#### Step 1. Retrieve the latest project version
```bash
git checkout main
git pull origin main
````

#### Step 2. Create a new branch
Name the branch descriptively using the prefixes listed above.
```bash
git checkout -b feat/my-new-feature
```

#### Step 3. Commit changes
```bash
# Stage all modified files or specific directories
git add .
# OR
git add src/my_cool_pkg

# Create the commit
git commit -m "feat: implement basic velocity controller"
```

#### Step 4. Push to the remote repository
```bash
git push -u origin feat/my-new-feature
```

Afterward, navigate to GitHub and click the **Compare & pull request** button. The assigned reviewer (senior developer) will then check your changes. Upon successful completion of the CI automated tests (GitHub Actions) and reviewer approval, your changes will be merged into the `main` branch.
