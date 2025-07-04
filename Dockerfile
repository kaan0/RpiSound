FROM debian:bookworm

# Install build dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    gcc-12 g++-12 \
    libasound2-dev \
    gdb \
    nano \
    sudo \
    python3 \
    python3-pip \
    python3-numpy \
    gcc-aarch64-linux-gnu \
    g++-aarch64-linux-gnu \
    clang-format \
    cppcheck \
    && apt-get clean

# Set gcc-12/g++-12 as default
RUN update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-12 100 \
    && update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-12 100

# Create devuser and add to sudoers with no password prompt
RUN useradd -ms /bin/bash devuser \
    && usermod -aG sudo devuser \
    && echo "devuser ALL=(ALL) NOPASSWD:ALL" >> /etc/sudoers

WORKDIR /rpisound

RUN chown -R devuser:devuser /rpisound || true

USER devuser

CMD [ "bash", "-l" ]
