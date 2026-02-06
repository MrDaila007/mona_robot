FROM ros:humble-ros-base

ENV DEBIAN_FRONTEND=noninteractive

# 1. Установка пакетов (System + Dev + Sim)
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    python3-colcon-common-extensions \
    ros-humble-xacro \
    ros-humble-rviz2 \
    ros-humble-gazebo-ros-pkgs \
    # Добавляем sudo, чтобы обычный пользователь мог ставить пакеты
    sudo \
    && rm -rf /var/lib/apt/lists/*

# 2. User Mapping (Создание клона твоего пользователя)
ARG USERNAME=mona_dev
ARG USER_UID=1000
ARG USER_GID=$USER_UID

RUN groupadd --gid $USER_GID $USERNAME \
    && useradd --uid $USER_UID --gid $USER_GID -m $USERNAME \
    && echo $USERNAME ALL=\(root\) NOPASSWD:ALL > /etc/sudoers.d/$USERNAME \
    && chmod 0440 /etc/sudoers.d/$USERNAME

# 3. Переключаемся на пользователя
USER $USERNAME

# 4. Настройка Workspace
# Теперь мы работаем в home директории пользователя, а не в /root
WORKDIR /home/$USERNAME/mona_ws

# Создаем src (права будут корректными, т.к. мы уже не root)
RUN mkdir src

# 5. Авто-source
RUN echo "source /opt/ros/humble/setup.bash" >> /home/$USERNAME/.bashrc && \
    echo "source /home/$USERNAME/mona_ws/install/setup.bash" >> /home/$USERNAME/.bashrc