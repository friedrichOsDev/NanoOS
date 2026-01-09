# NanoOS

NanoOS is a minimal 32-bit operating system built from scratch for educational purposes.

## Project Goal

This project serves as the practical foundation for a scientific paper ("Facharbeit") in computer science. The primary goal is to implement the fundamental concepts of an operating system, including:

 - boot process
 - memory management
 - interrupt handling
 - process management
 - basic device drivers

## Features

NanoOS is in an early stage of development. The current implementation includes:

*   **Multi-Stage Bootloader:** A custom bootloader written in 16-bit and 32-bit assembly to initialize the CPU and load the kernel.
*   **32-bit Kernel:** A basic kernel written in C, running in protected mode.
*   **CPU Management:**
    *   Global Descriptor Table (GDT)
    *   Interrupt Descriptor Table (IDT) for handling interrupts.
    *   Programmable Interrupt Controller (PIC) and Interrupt Request (IRQ) handling.
*   **Memory Management:** A simple heap implementation for dynamic memory allocation.
*   **Video Driver:** A basic framebuffer driver for console output, allowing the kernel to print text to the screen.
*   **Core Library:** A small, custom standard library for string manipulation and I/O functions.

## Getting Started

There are two ways to build and run NanoOS: using Docker (recommended for ease of use) or natively on Linux.

### Option 1: Using Docker (Recommended)

This is the easiest method as it automatically sets up the required cross-compiler toolchain in a container.

*Prerequisites:*
*   [Docker](https://www.docker.com/get-started) must be installed and running.

*Steps:*

1.  *Clone the repository:*
    ```sh
    git clone https://github.com/friedrichOsDev/NanoOS.git
    cd NanoOS
    ```

2.  *Build the Docker Image:*
    
    This command creates a Docker image named nano-cross with all the necessary build tools (like x86_64-elf-gcc and nasm). You only need to do this once.

    On Linux:
    ```sh
    ./scripts/linux/build_docker.sh
    ```
    On Windows:
    ```bat
    .\scripts\windows\build_docker.bat
    ```

3.  *Build the OS inside Docker:*
    
    This command runs the make command inside the Docker container to compile the OS. The project directory is mounted into the container, so the output files will appear in your local build folder.

    On Linux:
    ```sh
    docker run --rm -v $(pwd):/workspace nano-cross make
    ```
    On Windows (using PowerShell):
    ```powershell
    docker run --rm -v ${PWD}:/workspace nano-cross make
    ```
    After this step, you will find the nanoos.img file in the build/ directory.

4.  *Run the OS in QEMU:*
    
    You will need [QEMU](https://www.qemu.org/download/) installed on your host machine to run the OS image.

    On Linux:
    ```sh
    ./scripts/linux/run_qemu.sh
    ```
    On Windows:
    ```bat
    .\scripts\windows\run_qemu.bat
    ```

### Option 2: Native Linux Build

This method requires you to manually install the cross-compiler toolchain. The Dockerfile can be used as a reference for which dependencies are needed.

*Prerequisites:*
*   A Linux environment (like Ubuntu).
*   build-essential, nasm, qemu-system-x86.
*   A cross-compiler toolchain for x86_64-elf. Building this manually can be complex. There are many tutorials available online for setting up an x86 ELF cross-compiler.

*Steps:*

1.  *Clone the repository:*
    ```sh
    git clone https://github.com/friedrichOsDev/NanoOS.git
    cd NanoOS
    ```

2.  *Build the OS:*
    If you have the toolchain in your PATH, you can simply run make:
    ```sh
    make
    ```

    This will create the build/nanoos.img file.

3.  *Run the OS in QEMU:*
    ```sh
    ./scripts/linux/run_qemu.sh
    ```

    Note: The script uses qemu-system-x86_64 but the OS is 32-bit. Using qemu-system-i386 -fda build/nanoos.img is also a good alternative.

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Acknowledgments
*   Inspired by various OS development tutorials and resources available online.
*   Special thanks to the OSDev community for their invaluable resources and support.

## Contact
For questions or suggestions, please open an issue on GitHub or contact me directly at f39597054@gmail.com.