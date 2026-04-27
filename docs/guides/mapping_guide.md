# Mapping Pipeline Guide

> **Warehouse Cartography**
> 
> This document outlines the procedure for launching the simulation, generating a 2D occupancy grid of the facility, and serializing the map artifacts for subsequent utilization by the Nav2 global planner.

## 1. Infrastructure and Agent Initialization

Ensure your workspace is fully built before initiating the mapping sequence. To prevent DDS cross-talk during mapping, ensure you operate within an isolated local network.
```bash
./scripts/bringup_1_robot.bash
```

---

## 2. Controlling the Agent

The `slam_toolbox` node is highly optimized for computational efficiency. It is configured to update the pose graph strictly when physical movement is detected (`minimum_travel_distance: 0.1`). To initialize the map, you must begin moving the robot.

Use the connected gamepad (holding the L2 Deadman switch) or publish commands directly via the terminal:
```bash
# Terminal 3: Publish a continuous circular trajectory
ros2 topic pub -r 15 /mona_001/cmd_teleop geometry_msgs/msg/Twist "{linear: {x: 0.5, y: 0.0}, angular: {z: 0.5}}"
```

**Verification within RViz2:**
1. Ensure the **Fixed Frame** in the Global Options is set to `map`.
2. Verify that the `Map` (Occupancy Grid) display is rendering and dynamically expanding as the robot explores new areas.
3. Confirm that the red `LaserScan` points align precisely with physical obstacles (walls, racks) and do not reflect the robot's own chassis.

---

## 3. Saving the Static Map (Nav2 Artifacts)

Executing the CLI map saver (`map_saver_cli`) in ROS 2 can occasionally fail due to clock desynchronization between the host OS and the simulation. Map saving must be executed **exclusively via the RViz2 graphical plugin**.
1. In the top menu of RViz2, navigate to: `Panels -> Add New Panel -> slam_toolbox -> SlamToolboxPlugin`.
2. In the newly opened panel, locate the **Save Map** input field.
3. Enter the relative repository path (excluding file extensions): `src/mona_core/maps/warehouse_map`
4. Click the **Save Map** button.
5. Verify that `warehouse_map.pgm` (the image matrix) and `warehouse_map.yaml` (the metadata) have been generated in the target directory.

---

## 4. Map Serialization (Lifelong Mapping)

If your mapping session is incomplete and you intend to expand the map at a later date, you must serialize the pose graph.
1. In the same `SlamToolboxPlugin` panel, locate the **Serialize Map** input field.
2. Enter the relative repository path: `src/mona_core/maps/warehouse_map_serialized`
3. Click **Serialize Map**.
4. Verify that the `.data` and `.posegraph` binary files have been successfully saved to the target directory.