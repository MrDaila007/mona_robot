FROM ros:humble-ros-base

ENV DEBIAN_FRONTEND=noninteractive

# 1. User Mapping Configuration
ARG USERNAME=mona_dev
ARG USER_UID=1000
ARG USER_GID=$USER_UID

# 2. System and ROS 2 Dependencies Installation
# Includes Kisak PPA for updated Mesa drivers required by modern Linux kernels
RUN apt-get update && apt-get install -y software-properties-common curl sudo \
    && add-apt-repository ppa:kisak/kisak-mesa -y \
    && apt-get update && apt-get install -y \
    build-essential cmake python3-colcon-common-extensions \
    git \
    ros-humble-xacro ros-humble-rviz2 ros-humble-ros-gz \
    ros-humble-laser-geometry ros-humble-tf2-sensor-msgs \
    ros-humble-rqt-graph ros-humble-rqt-common-plugins \
    ros-humble-robot-localization \
    ros-humble-navigation2 ros-humble-nav2-bringup \
    ros-humble-slam-toolbox \
    ros-humble-pointcloud-to-laserscan \
    python3-pip \
    libgl1-mesa-glx libgl1-mesa-dri mesa-utils \
    mesa-vulkan-drivers vulkan-tools \
    # Патч бага Ogre3D/Mesa: Динамический поиск и корректное исправление шейдера
    && SHADER_FILE=$(find /opt/ros/humble -name "indexed_8bit_image.frag" | head -n 1) \
    && if [ -n "$SHADER_FILE" ]; then \
        sed -i '/#version/a #extension GL_ARB_shading_language_420pack : enable' "$SHADER_FILE" && \
        sed -i 's/uniform sampler2D alpha_image;/layout(binding = 0) uniform sampler2D alpha_image;/g' "$SHADER_FILE" && \
        sed -i 's/uniform sampler1D palette;/layout(binding = 1) uniform sampler1D palette;/g' "$SHADER_FILE"; \
        echo "Shader patched successfully at $SHADER_FILE"; \
    fi \
    && rm -rf /var/lib/apt/lists/* \
    && pip3 install --no-cache-dir --upgrade pip \
    && pip3 install --no-cache-dir "pycodestyle<2.9.0" "flake8<5.0.0" "autopep8<2.1.0"

# 3. User Creation and Privilege Assignment
RUN groupadd --gid $USER_GID $USERNAME \
    && useradd --uid $USER_UID --gid $USER_GID -m $USERNAME \
    && echo "$USERNAME ALL=(root) NOPASSWD:ALL" > /etc/sudoers.d/$USERNAME \
    && chmod 0440 /etc/sudoers.d/$USERNAME \
    && usermod -aG video,audio $USERNAME \
    && (getent group render && usermod -aG render $USERNAME || true)

# 4. Workspace Initialization
USER $USERNAME
WORKDIR /home/$USERNAME/mona_ws

# 5. Source Code Migration
COPY --chown=$USERNAME:$USER_GID src src
COPY --chown=$USERNAME:$USER_GID configs configs
COPY --chown=$USERNAME:$USER_GID scripts scripts

# 6. Environment Sourcing
RUN echo "source /opt/ros/humble/setup.bash" >> /home/${USERNAME}/.bashrc && \
    echo "source /home/${USERNAME}/mona_ws/install/setup.bash" >> /home/${USERNAME}/.bashrc