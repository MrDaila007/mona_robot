# SLAM and Odometry Architecture

> **Deterministic Localization in Dynamic Environments**
> 
> The MONA localization and mapping subsystem is built upon the ROS 2 Humble stack, leveraging `slam_toolbox` in online asynchronous mode. To guarantee the mathematical stability of the SLAM algorithms, the architecture enforces a strict coordinate transform (TF) hierarchy and high-frequency sensor filtering.

## 1. Pipeline Overview

### 1.1. Coordinate Transform Tree (TF Tree)

The system strictly adheres to the ROS REP-105 standard for coordinate frames. In a swarm deployment, all agent-specific frames are prefixed with their isolated namespace:

`map` -> `[namespace]/odom` -> `[namespace]/base_footprint` -> `[namespace]/base_link` -> `[sensor_links]`

* **`map` -> `[namespace]/odom`**: Published by the `slam_toolbox` (or a dedicated localization node). Compensates for cumulative odometry drift via correlative laser scan matching.
* **`[namespace]/odom` -> `[namespace]/base_footprint`**: Published by the `ekf_node` (Extended Kalman Filter from the `robot_localization` package). Fuses raw sensor data and operates at a strict 50 Hz.
* **`[namespace]/base_footprint` -> `[namespace]/base_link`**: A static transform mapping the robot's 2D ground projection (Z=0) to its physical center of mass (Z=0.17m).

---

## 2. Data Sources and Processing

### 2.1. Ground Truth Odometry (Simulation)

Due to the inherent limitations of the `MecanumDrive` plugin in Ignition Gazebo (e.g., dropped timestamps, message type collisions, and lack of covariance matrices), odometry is directly sourced via the `OdometryPublisher` plugin. This guarantees a stable 50 Hz update rate and exact absolute coordinates in the simulation, preventing mathematical collapse within the SLAM node.

> [!WARNING]  
> **Race Condition Prevention:** The raw `/tf` topic from Gazebo is strictly excluded from the `ros_gz_bridge`. This prevents simulation transforms from conflicting with the mathematically filtered transforms published by the internal ROS 2 EKF node.

### 2.2. LiDAR Processing and Fusion

The heavy-duty chassis (250 kg) is equipped with 4 LiDARs (150° FOV each), embedded within the chassis corners to provide 360-degree coverage.
1. **Fusion:** Data streams from all 4 LiDARs are aggregated into a single `PointCloud2` message by the `mona_lidar_merger` component utilizing Zero-Copy IPC memory transfers.
2. **Projection (`pointcloud_to_laserscan`):** The fused 2D point cloud is projected down into a 2D `LaserScan`.
   * **Target Frame:** `[namespace]/base_footprint` (eliminates latency errors that can occur if static TF trees are delayed during boot).
   * **Use Inf:** `True` (critically required to properly clear free space in the Nav2 Costmaps).

---

## 3. SLAM Toolbox Configuration

The system utilizes the robust **Ceres Solver** configured with `SPARSE_NORMAL_CHOLESKY`.
* **`scan_buffer_maximum_scan_distance`**: 40.0m (optimized for large-scale warehouse environments).
* **`minimum_travel_distance` / `heading`**: 0.1m / 0.1rad (ensures the pose graph is updated only upon physical chassis displacement, preventing stationary noise accumulation).
* **Message Queue (`scan_buffer_size`)**: `30` (provides architectural resilience against transient TF synchronization delays).

---

## 4. Map Preservation and Serialization
Map artifacts required for the Nav2 stack are generated using the official graphical `SlamToolboxPlugin` within RViz2. To maintain repository hygiene, artifacts are stored inside the `src/mona_core/maps/` directory.

* **Save Map (`.pgm`, `.yaml`):** Generates a standard 2D Occupancy Grid. This static map is utilized by the Nav2 Map Server for global path planning and costmap initialization.
  *Output path: `src/mona_core/maps/warehouse_map`*
* **Serialize Map (`.posegraph`, `.data`):** Preserves the internal SLAM mathematical pose graph. This enables "Lifelong Mapping" or "Localization Mode", allowing the robot to resume operations after a full system reboot without losing prior map data.
  *Output path: `src/mona_core/maps/warehouse_map_serialized`*