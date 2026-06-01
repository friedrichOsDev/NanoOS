# NanoOS

NanoOS is a simple kernel development project that I am developing in my free time.

## Built-in Shell Commands

Here are all the currently implemented commands:

### General Commands
*   **`clear`**: Clears the console screen text buffer.
*   **`welcome`**: Displays the ASCII art welcome banner.
*   **`help`**: Prints the list of available commands and their basic usage.
*   **`time`**: Retrieves and prints the current formatted time from the Real-Time Clock (RTC).
*   **`shutdown`**: Powers off the system safely via ACPI.

### System & Memory Diagnostics
*   **`heap`**: Dumps the layout of the kernel heap, displaying block addresses, sizes (in bytes), and their current status (`FREE` or `USED`).
*   **`acpiinfo`**: Dumps general information regarding the detected ACPI tables.
*   **`fadtinfo`**: Inspects and displays Fixed ACPI Description Table (FADT) details.
*   **`madtinfo`**: Inspects and displays Multiple APIC Description Table (MADT) details, useful for interrupt controller configuration.

### Storage & Partition Management
*   **`storage`**: Dumps structural information about all initialized storage drives.
*   **`partman`**: Displays detected partition tables and layout information via the Partition Manager.
*   **`storage_read <disk_index> <sector>`**
    *   Reads a single sector from the specified disk index and prints its content as a raw hex dump to the console.
*   **`storage_write <disk_index> <sector> <data>`**
    *   Writes data to a specific sector. The `<data>` parameter expects plain hex strings (e.g., `DEADBEEF`), which are parsed into raw bytes and written to disk.

## Building
1. Build the docker image using the [Build Docker](scripts/linux/build_docker.sh) script
2. Build the os ISO using the [Run Docker](scripts/linux/run_docker.sh) script
3. Execute the os:
   - Flash to USB Drive using the [Flash ISO](scripts/linux/flash_iso.sh) script
   - Run with QEMU using the [Run QEMU](scripts/linux/run_qemu.sh) script
