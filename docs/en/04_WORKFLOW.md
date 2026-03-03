# Daily Workflow

> **Build, Test, Repeat**
>
> System stability is achieved through regular testing and strict build hygiene.

## 1. Building the project

We use `colcon`.

### Incremental build (for fast iteration)

```bash
colcon build --symlink-install
```

### Clean build

Mandatory before merging branches or when the system behaves strangely. Use the provided script:

```bash
./scripts/run_sim.bash
```

## 2. Updating the environment

After each build you need to refresh environment variables:

```bash
source install/setup.bash
```

## 3. Local integration (Local CI)

Before committing, run the quality gate script:

```bash
./scripts/local_ci.bash
```

**The script performs:**

1. Auto‑formatting (`uncrustify`, `black`).
2. Linting (`cpplint`, `flake8`).
3. Running tests.

## 4. Testing

Run unit and integration tests:

```bash
colcon test
colcon test-result --verbose
```

## 5. Dependency management (rosdep)

### Why is this needed in C++?

Unlike Python, where dependencies are isolated in virtual environments, C++ dependencies in ROS 2 are **system libraries** (header files `.hpp` and binaries `.so` in `/usr/lib`).

When you add `<depend>lifecycle_msgs</depend>` to `package.xml`, CMake does not download the library for you; it expects it to be installed on the system. The `rosdep` utility:

- Reads `package.xml`.
- Sees `<depend>lifecycle_msgs</depend>`.
- Looks up that on Ubuntu this package is called `ros-humble-lifecycle-msgs`.
- Runs `sudo apt-get install ros-humble-lifecycle-msgs`.

### When to run it?

You should run this command if:

1. You cloned the project for the first time.
2. You (or your colleague) added a new library to `package.xml` and the project fails to build with an error like `Could not find a package configuration file provided by...`.

### Command

```bash
# --from-paths src: scan the src/ folder
# --ignore-src: do not try to install packages we are developing ourselves
# -r: continue installing even if some packages fail
# -y: answer "yes" to all apt-get questions

rosdep update && rosdep install --from-paths src --ignore-src -r -y
```

## 6. Common issues (Troubleshooting)

### WARNING: colcon.colcon_ros.prefix_path.ament

* **Symptom:** Many yellow warnings during build.  
* **Cause:** You removed `build/` and `install/` directories in an active terminal.  
* **Solution:** Just ignore them or restart the terminal.

### CMake Error: ament_cmake_symlink_install_directory

* **Symptom:** Errors while copying folders (launch, config).  
* **Cause:** `CMakeLists.txt` references a folder that does not exist.  
* **Solution:** Remove the extra `install(DIRECTORY ...)` line from `CMakeLists.txt`.

