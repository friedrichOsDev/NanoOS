FROM ubuntu:latest

SHELL ["/bin/bash", "-c"]

# Install dependencies
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
    nasm \
    git \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

# Vars
ENV PREFIX="/opt/cross"
ENV TARGET="x86_64-elf"
ENV PATH="$PREFIX/bin:$PATH"
ENV SRC="/src"

# Create necessary directories
RUN mkdir -p "$PREFIX" "$SRC"
WORKDIR "$SRC"

# Build and install Binutils
RUN wget https://ftp.gnu.org/gnu/binutils/binutils-2.45.tar.gz \
    && tar -xzf binutils-2.45.tar.gz \
    && mkdir build-binutils && cd build-binutils \
    && ../binutils-2.45/configure --target="$TARGET" --prefix="$PREFIX" --with-sysroot --disable-nls --disable-werror \
    && make -j"$(nproc)" && make install \
    && cd .. && rm -rf build-binutils binutils-2.45 binutils-2.45.tar.gz

# Build and install GCC (C + libgcc only)
RUN wget https://ftp.gnu.org/gnu/gcc/gcc-15.2.0/gcc-15.2.0.tar.gz \
    && tar -xzf gcc-15.2.0.tar.gz \
    && cd gcc-15.2.0 && ./contrib/download_prerequisites && cd .. \
    && mkdir build-gcc && cd build-gcc \
    && ../gcc-15.2.0/configure --target="$TARGET" --prefix="$PREFIX" --disable-nls --enable-languages=c,c++ --without-headers \
    && make all-gcc -j"$(nproc)" && make all-target-libgcc -j"$(nproc)" \
    && make install-gcc install-target-libgcc \
    && cd .. && rm -rf build-gcc gcc-15.2.0 gcc-15.2.0.tar.gz

# Verify installation
RUN "$TARGET-gcc" --version && "$TARGET-ld" --version

WORKDIR /workspace
CMD ["/bin/bash"]
