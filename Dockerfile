FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    build-essential \
    clang-14 \
    llvm-14 \
    llvm-14-dev \
    llvm-14-tools \
    lld-14 \
    git \
    cmake \
    python3 \
    python3-pip \
    python3-dev \
    automake \
    ninja-build \
    libtool-bin \
    libglib2.0-dev \
    wget \
    curl \
    vim \
    gdb \
    && rm -rf /var/lib/apt/lists/*

RUN apt-get update && apt-get install -y \
    libsdl2-dev \
    libgme-dev \
    libopenal-dev \
    libmpg123-dev \
    libsndfile1-dev \
    libfluidsynth-dev \
    libjpeg-dev \
    zlib1g-dev \
    libbz2-dev \
    libgtk-3-dev \
    nasm \
    && rm -rf /var/lib/apt/lists/*

RUN ln -s /usr/bin/clang-14 /usr/bin/clang && \
    ln -s /usr/bin/clang++-14 /usr/bin/clang++ && \
    ln -s /usr/bin/llvm-config-14 /usr/bin/llvm-config

WORKDIR /opt
RUN git clone https://github.com/AFLplusplus/AFLplusplus.git afl && \
    cd afl && \
    make distrib && \
    make install

WORKDIR /work

CMD ["/bin/bash"]
