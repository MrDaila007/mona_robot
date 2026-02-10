FROM ros:humble-ros-base

ENV DEBIAN_FRONTEND=noninteractive

# 1. User Mapping
ARG USERNAME=mona_dev
ARG USER_UID=1000
ARG USER_GID=$USER_UID

# 2. Установка пакетов (System + Dev + Sim + Audio)
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    python3-colcon-common-extensions \
    ros-humble-xacro \
    ros-humble-rviz2 \
    ros-humble-ros-gz \
    sudo \
    python3-pip \
    && rm -rf /var/lib/apt/lists/* \
    && pip3 install --no-cache-dir --upgrade pip \
    && pip3 install --no-cache-dir "pycodestyle<2.9.0" "flake8<5.0.0" "autopep8<2.1.0"

# 3. Создание пользователя и настройка прав
RUN groupadd --gid $USER_GID $USERNAME \
    && useradd --uid $USER_UID --gid $USER_GID -m $USERNAME \
    && echo $USERNAME ALL=\(root\) NOPASSWD:ALL > /etc/sudoers.d/$USERNAME \
    && chmod 0440 /etc/sudoers.d/$USERNAME \
    && usermod -aG video,audio $USERNAME

# 4. Переключаемся на пользователя
USER $USERNAME

# 5. Настройка Workspace
WORKDIR /home/$USERNAME/mona_ws

# 6. Копируем код с правильными правами!
# --chown гарантирует, что владельцем файлов станет mona_dev, а не root
COPY --chown=$USERNAME:$USER_GID src src
COPY --chown=$USERNAME:$USER_GID configs configs

# 7. Авто-source
RUN echo "source /opt/ros/humble/setup.bash" >> /home/$USERNAME/.bashrc && \
    echo "source /home/$USERNAME/mona_ws/install/setup.bash" >> /home/$USERNAME/.bashrc