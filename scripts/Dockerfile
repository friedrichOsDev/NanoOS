# ==========================================
# STAGE 1: Toolchain Builder (Cross Compiler)
# ==========================================
FROM ubuntu:24.04 AS toolchain-builder

SHELL ["/bin/bash", "-c"]
ENV DEBIAN_FRONTEND=noninteractive

# Minimal Packages
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
    && rm -rf /var/lib/apt/lists/*

ENV PREFIX="/opt/cross"
ENV TARGET="x86_64-elf"
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

# ==========================================
# STAGE 2: ASMFMT Builder
# ==========================================
FROM golang:1.22-alpine AS builder

RUN apk add --no-cache git
RUN go install github.com/klauspost/asmfmt/cmd/asmfmt@latest

# ==========================================
# STAGE 3: Final Devcontainer
# ==========================================
FROM ubuntu:24.04

SHELL ["/bin/bash", "-c"]
ENV DEBIAN_FRONTEND=noninteractive

# Copy Toolchain from Toolchain Builder
COPY --from=toolchain-builder /opt/cross /opt/cross
COPY --from=builder /go/bin/asmfmt /usr/local/bin/asmfmt

ENV PREFIX="/opt/cross"
ENV TARGET="x86_64-elf"
ENV PATH="$PREFIX/bin:$PATH"

RUN apt-get update && apt-get install -y \
    build-essential \
    clangd \
    clang-format \
    make \
    nasm \
    xorriso \
    grub-pc-bin \
    grub-efi-amd64-bin \
    grub-efi-ia32-bin \
    qemu-system \
    mtools \
    dosfstools \
    gdb \
    git \
    ca-certificates \
    parted \
    cmake \
    sudo \
    btop \
    wget \
    curl \
    make \
    && rm -rf /var/lib/apt/lists/*

# Verify Toolchain
RUN $TARGET-gcc --version && \
    $TARGET-ld --version

# Create User
RUN usermod -l vscode ubuntu \
    && groupmod -n vscode ubuntu \
    && usermod -d /home/vscode -m vscode \
    && echo vscode ALL=\(root\) NOPASSWD:ALL > /etc/sudoers.d/vscode \
    && chmod 0440 /etc/sudoers.d/vscode

# Create Working directory and Fix Owner
RUN mkdir -p /workspace && chown -R vscode:vscode /workspace
WORKDIR /workspace

USER vscode

CMD ["/bin/bash"]
