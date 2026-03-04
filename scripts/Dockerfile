FROM ubuntu:24.04

SHELL ["/bin/bash", "-c"]

ENV DEBIAN_FRONTEND=noninteractive

# Dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    bison \
    flex \
    libgmp3-dev \
    libmpc-dev \
    libmpfr-dev \
    libisl-dev \
    texinfo \
    wget \
    g++ \
    gcc \
    make \
    nasm \
    xorriso \
    grub-pc-bin \
    grub-efi-amd64-bin \
    grub-efi-ia32-bin \
    mtools \
    dosfstools \
    qemu-system-x86 \
    qemu-utils \
    gdb \
    git \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

# Cross Compiler config
ENV PREFIX="/opt/cross"
ENV TARGET="i686-elf"
ENV PATH="$PREFIX/bin:$PATH"
ENV SRC="/usr/src"

RUN mkdir -p "$PREFIX" "$SRC"
WORKDIR "$SRC"

# Build Binutils
RUN wget https://ftp.gnu.org/gnu/binutils/binutils-2.45.tar.gz \
    && tar -xzf binutils-2.45.tar.gz \
    && mkdir build-binutils && cd build-binutils \
    && ../binutils-2.45/configure \
        --target=$TARGET \
        --prefix=$PREFIX \
        --with-sysroot \
        --disable-nls \
        --disable-werror \
    && make -j$(nproc) \
    && make install \
    && cd .. \
    && rm -rf build-binutils binutils-2.45 binutils-2.45.tar.gz

# Build GCC
RUN wget https://ftp.gnu.org/gnu/gcc/gcc-14.2.0/gcc-14.2.0.tar.gz \
    && tar -xzf gcc-14.2.0.tar.gz \
    && cd gcc-14.2.0 && ./contrib/download_prerequisites && cd .. \
    && mkdir build-gcc && cd build-gcc \
    && ../gcc-14.2.0/configure \
        --target=$TARGET \
        --prefix=$PREFIX \
        --disable-nls \
        --enable-languages=c,c++ \
        --without-headers \
    && make all-gcc -j$(nproc) \
    && make all-target-libgcc -j$(nproc) \
    && make install-gcc \
    && make install-target-libgcc \
    && cd .. \
    && rm -rf build-gcc gcc-14.2.0 gcc-14.2.0.tar.gz

# Verify Toolchain
RUN $TARGET-gcc --version && \
    $TARGET-ld --version

# Working directory
WORKDIR /workspace

CMD ["/bin/bash"]
