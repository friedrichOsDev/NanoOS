/*
 * @file shell.c
 * @brief Kernel shell application
 * @author friedrichOsDev
 */

#include <shell.h>
#include <console.h>
#include <serial.h>
#include <string.h>
#include <heap.h>
#include <rtc.h>
#include <print.h>
#include <acpi.h>
#include <kernel.h>
#include <disk.h>

char command_buffer[MAX_COMMAND_LENGTH];
size_t command_buffer_pos = 0;
command_list_t commands;

/*
 * Clear command implementation
 * @param argc: Number of arguments
 * @param argv: Array of argument strings
 */
void shell_command_clear(int argc, char** argv) {
    (void)argc;
    (void)argv;
    console_clear();
}

/*
 * Version command implementation
 * @param argc: Number of arguments
 * @param argv: Array of argument strings
 */
void shell_command_version(int argc, char** argv) {
    (void)argc;
    (void)argv;
    console_puts("Kernel Version 1.0.0\n");
}

/*
 * Reboot command implementation
 * @param argc: Number of arguments
 * @param argv: Array of argument strings
 */
void shell_command_reboot(int argc, char** argv) {
    (void)argc;
    (void)argv;
    console_puts("Rebooting system...\n");
    
    // for now a Triple fault will cause a reboot
    uint16_t empty_idt[3] = {0, 0, 0};
    __asm__ __volatile__ ("lidt %0" : : "m"(empty_idt));
}

/*
 * Time command implementation
 * @param argc: Number of arguments
 * @param argv: Array of argument strings
 */
void shell_command_time(int argc, char** argv) {
    (void)argc;
    (void)argv;
    rtc_time_t time = rtc_get_time();
    printf("%d:%d:%d %d.%d.%d\n", time.hours, time.minutes, time.seconds, time.day, time.month, time.year);
}

/*
 * Get MMAP Info command implementation
 * @param argc: Number of arguments
 * @param argv: Array of argument strings
 */
void shell_command_mmapinfo(int argc, char** argv) {
    (void)argc;
    (void)argv;
    printf("Memory Map:\n");
    for (size_t i = 0; i < mmap_info->entry_count; i++) {
        mmap_entry_t* entry = &mmap_info->entries[i];
        printf("Base: 0x%x%x, Length: 0x%x%x, Type: %d\n", entry->base_addr_high, entry->base_addr_low, entry->length_high, entry->length_low, entry->type);
    }
}

/*
 * Get RSDP Info command implementation
 * @param argc: Number of arguments
 * @param argv: Array of argument strings
 */
void shell_command_rsdpinfo(int argc, char** argv) {
    (void)argc;
    (void)argv;
    rsdp_t* rsdp = acpi_get_rsdp();
    if (rsdp == NULL) {
        console_puts("RSDP not found.\n");
        return;
    }

    // null terminate signature and oemid for safe printing
    char signature[9];
    memcpy(signature, rsdp->signature, 8);
    signature[8] = '\0';
    char oemid[7];
    memcpy(oemid, rsdp->oemid, 6);
    oemid[6] = '\0';

    printf("RSDP Address: 0x%x\n", (uint32_t)rsdp);
    printf("RSDP Signature: %s\n", signature);
    printf("RSDP Checksum: 0x%x\n", rsdp->checksum);
    printf("RSDP OEM ID: %s\n", oemid);
    printf("RSDP Revision: %d\n", rsdp->revision);
    printf("RSDP RSDT Address: 0x%x\n", rsdp->rsdt_address);

    if (rsdp->revision >= 2) {
        printf("RSDP Length: %d\n", rsdp->length);
        printf("RSDP XSDT Address: 0x%x\n", (uint32_t)rsdp->xsdt_address);
        printf("RSDP Extended Checksum: 0x%x\n", rsdp->extended_checksum);
    }
}

/*
 * Get RSDT Info command implementation
 * @param argc: Number of arguments
 * @param argv: Array of argument strings
 */
void shell_command_rsdtinfo(int argc, char** argv) {
    (void)argc;
    (void)argv;
    rsdp_t* rsdp = acpi_get_rsdp();
    if (rsdp == NULL) {
        console_puts("RSDP not found.\n");
        return;
    }

    rsdt_t* rsdt = acpi_get_rsdt();
    if (rsdt == NULL) {
        console_puts("RSDT not found.\n");
        return;
    }

    char signature[5];
    memcpy(signature, rsdt->header.signature, 4);
    signature[4] = '\0';
    char oemid[7];
    memcpy(oemid, rsdt->header.oemid, 6);
    oemid[6] = '\0';
    char oem_table_id[9];
    memcpy(oem_table_id, rsdt->header.oem_table_id, 8);
    oem_table_id[8] = '\0';

    printf("RSDT Address: 0x%x\n", (uint32_t)rsdt);
    printf("RSDT Signature: %s\n", signature);
    printf("RSDT Length: %d\n", rsdt->header.length);
    printf("RSDT Revision: %d\n", rsdt->header.revision);
    printf("RSDT Checksum: 0x%x\n", rsdt->header.checksum);
    printf("RSDT OEM ID: %s\n", oemid);
    printf("RSDT OEM Table ID: %s\n", oem_table_id);
    printf("RSDT OEM Revision: 0x%x\n", rsdt->header.oem_revision);
    printf("RSDT Creator ID: 0x%x\n", rsdt->header.creator_id);
    printf("RSDT Creator Revision: 0x%x\n", rsdt->header.creator_revision);

    uint32_t num_sdts = (rsdt->header.length - sizeof(acpi_sdt_header_t)) / sizeof(uint32_t);
    printf("RSDT Contains %d other SDT entries:\n", num_sdts);
    for (uint32_t i = 0; i < num_sdts; i++) {
        printf("  SDT %d Address: 0x%x\n", i, rsdt->pointer_to_other_sdt[i]);
    }
}

/*
 * Get FADT Info command implementation
 * @param argc: Number of arguments
 * @param argv: Array of argument strings
 */
void shell_command_fadtinfo(int argc, char** argv) {
    (void)argc;
    (void)argv;
    fadt_t* fadt = acpi_get_fadt();
    if (fadt == NULL) {
        console_puts("FADT not found.\n");
        return;
    }

    char signature[5];
    memcpy(signature, fadt->header.signature, 4);
    signature[4] = '\0';
    char oemid[7];
    memcpy(oemid, fadt->header.oemid, 6);
    oemid[6] = '\0';
    char oem_table_id[9];
    memcpy(oem_table_id, fadt->header.oem_table_id, 8);
    oem_table_id[8] = '\0';

    printf("FADT Address: 0x%x\n", (uint32_t)fadt);
    printf("FADT Signature: %s\n", signature);
    printf("FADT Length: %d\n", fadt->header.length);
    printf("FADT Revision: %d\n", fadt->header.revision);
    printf("FADT Checksum: 0x%x\n", fadt->header.checksum);
    printf("FADT OEM ID: %s\n", oemid);
    printf("FADT OEM Table ID: %s\n", oem_table_id);
    printf("FADT OEM Revision: 0x%x\n", fadt->header.oem_revision);
    printf("FADT Creator ID: 0x%x\n", fadt->header.creator_id);
    printf("FADT Creator Revision: 0x%x\n", fadt->header.creator_revision);
    printf("FADT Firmware Control: 0x%x\n", fadt->firmware_ctrl);
    printf("FADT DSDT: 0x%x\n", fadt->dsdt);
    printf("FADT Preferred Power Management Profile: %d\n", fadt->preferred_power_management_profile);
    printf("FADT SCI Interrupt: %d\n", fadt->SCI_interrupt);
    printf("FADT SMI Command Port: 0x%x\n", fadt->SMI_command_port);
    printf("FADT ACPI Enable: 0x%x\n", fadt->acpi_enable);
    printf("FADT ACPI Disable: 0x%x\n", fadt->acpi_disable);
    printf("FADT S4BIOS Request: 0x%x\n", fadt->s4bios_req);
    printf("FADT PM1a Event Block: 0x%x\n", fadt->PM1a_event_block);
    printf("FADT PM1b Event Block: 0x%x\n", fadt->PM1b_event_block);
    printf("FADT PM1a Control Block: 0x%x\n", fadt->PM1a_control_block);
    printf("FADT PM1b Control Block: 0x%x\n", fadt->PM1b_control_block);
    printf("FADT PM2 Control Block: 0x%x\n", fadt->PM2_control_block);
    printf("FADT PM Timer Block: 0x%x\n", fadt->PM_timer_block);
    printf("FADT GPE0 Block: 0x%x\n", fadt->GPE0_block);
    printf("FADT GPE1 Block: 0x%x\n", fadt->GPE1_block);
    printf("FADT PM1 Event Length: %d\n", fadt->PM1_event_length);
    printf("FADT PM1 Control Length: %d\n", fadt->PM1_control_length);
    printf("FADT PM2 Control Length: %d\n", fadt->PM2_control_length);
    printf("FADT GPE0 Length: %d\n", fadt->GPE0_length);
    printf("FADT GPE1 Length: %d\n", fadt->GPE1_length);
    printf("FADT GPE1 Base: %d\n", fadt->GPE1_base);
    printf("FADT C-State Register: 0x%x\n", fadt->c_state_register);
    printf("FADT Worst Case C2 Latency: %d\n", fadt->worst_case_c2_latency);
    printf("FADT Worst Case C3 Latency: %d\n", fadt->worst_case_c3_latency);
    printf("FADT Flush Size: %d\n", fadt->flush_size);
    printf("FADT Flush Stride: %d\n", fadt->flush_stride);
    printf("FADT Duty Offset: %d\n", fadt->duty_offset);
    printf("FADT Duty Width: %d\n", fadt->duty_width);
    printf("FADT Day Alarm: %d\n", fadt->day_alarm);
    printf("FADT Month Alarm: %d\n", fadt->month_alarm);
    printf("FADT Century: %d\n", fadt->century);
    printf("FADT Boot Architecture Flags: 0x%x\n", fadt->boot_architecture_flags);
    printf("FADT Flags: 0x%x\n", fadt->flags);
    printf("FADT Reset Register Address: in gas_t format\n");
    printf("FADT Reset Command: 0x%x\n", fadt->reset_command);
    printf("FADT X Firmware Control (64-bit address casted to 32-bit address): 0x%x\n", (uint32_t)fadt->X_firmware_control);
    printf("FADT X DSDT (64-bit address casted to 32-bit address): 0x%x\n", (uint32_t)fadt->X_dsdt);
    printf("FADT X PM1a Event Block Address: in gas_t format\n");
    printf("FADT X PM1b Event Block Address: in gas_t format\n");
    printf("FADT X PM1a Control Block Address: in gas_t format\n");
    printf("FADT X PM1b Control Block Address: in gas_t format\n");
    printf("FADT X PM2 Control Block Address: in gas_t format\n");
    printf("FADT X PM Timer Block Address: in gas_t format\n");
    printf("FADT X GPE0 Block Address: in gas_t format\n");
    printf("FADT X GPE1 Block Address: in gas_t format\n");
}

/*
 * Get MADT Info command implementation
 * @param argc: Number of arguments
 * @param argv: Array of argument strings
 */
void shell_command_madtinfo(int argc, char** argv) {
    (void)argc;
    (void)argv;
    madt_t* madt = acpi_get_madt();
    madt_parsed_t* madt_parsed = acpi_get_madt_parsed();
    if (madt == NULL || madt_parsed == NULL) {
        console_puts("MADT not found.\n");
        return;
    }

    printf("MADT Address: 0x%x\n", (uint32_t)madt);
    printf("Local APIC Address: 0x%x\n", madt->local_apic_address);
    printf("Flags: 0x%x\n", madt->flags);

    for (size_t i = 0; i < madt_parsed->lapic_count; i++) {
        madt_lapic_entry_t* lapic = &madt_parsed->lapics[i];
        printf("LAPIC %d: Processor ID: %d, APIC ID: %d, Flags: 0x%x\n", i, lapic->processor_id, lapic->apic_id, lapic->flags);
    }

    for (size_t i = 0; i < madt_parsed->ioapic_count; i++) {
        madt_ioapic_entry_t* ioapic = &madt_parsed->ioapics[i];
        printf("IOAPIC %d: IOAPIC ID: %d, Address: 0x%x, Global System Interrupt Base: %d\n", i, ioapic->io_apic_id, ioapic->io_apic_address, ioapic->global_system_interrupt_base);
    }

    for (size_t i = 0; i < madt_parsed->iso_count; i++) {
        madt_iso_entry_t* iso = &madt_parsed->isos[i];
        printf("ISO %d: Bus Source: %d, IRQ Source: %d, Global System Interrupt: %d, Flags: 0x%x\n", i, iso->bus_source, iso->irq_source, iso->global_system_interrupt, iso->flags);
    }

    for (size_t i = 0; i < madt_parsed->ioapic_nmi_count; i++) {
        madt_ioapic_nmi_entry_t* ioapic_nmi = &madt_parsed->ioapic_nmis[i];
        printf("IOAPIC NMI %d: NMI Source: %d, Flags: 0x%x, Global System Interrupt: %d\n", i, ioapic_nmi->nmi_source, ioapic_nmi->flags, ioapic_nmi->global_system_interrupt);
    }

    for (size_t i = 0; i < madt_parsed->lapic_nmi_count; i++) {
        madt_lapic_nmi_entry_t* lapic_nmi = &madt_parsed->lapic_nmis[i];
        printf("LAPIC NMI %d: Processor ID: %d, Flags: 0x%x, LINTIN: %d\n", i, lapic_nmi->processor_id, lapic_nmi->flags, lapic_nmi->lintin);
    }

    for (size_t i = 0; i < madt_parsed->lapic_address_override_count; i++) {
        madt_lapic_address_override_entry_t* lapic_address_override = &madt_parsed->lapic_address_overrides[i];
        printf("LAPIC Address Override %d: Local APIC Address: 0x%x%x\n", i, (uint32_t)(lapic_address_override->local_apic_address >> 32), (uint32_t)(lapic_address_override->local_apic_address & 0xFFFFFFFF));
    }

    for (size_t i = 0; i < madt_parsed->lx2apic_count; i++) {
        madt_lx2apic_entry_t* lx2apic = &madt_parsed->lx2apics[i];
        printf("Lx2APIC %d: x2APIC ID: %d, Flags: 0x%x, ACPI ID: %d\n", i, lx2apic->x2apic_id, lx2apic->flags, lx2apic->acpi_id);
    }
}

/*
 * Get disk info command implementation
 * @param argc: Number of arguments
 * @param argv: Array of argument strings
 */
void shell_command_diskinfo(int argc, char** argv) {
    if (argc != 2) {
        console_puts("Usage: diskinfo <disk_number>\n");
        return;
    }
    
    int disk_number = atoi(argv[1]);
    disk_t* disk = get_disk(disk_number);
    if (disk == NULL) {
        console_puts("Disk not found.\n");
        return;
    } else {
        printf("Disk %d Info:\n", disk_number);
        printf("Name: %s\n", disk->name);
        printf("Sector Count: %d\n", disk->sector_count);
        printf("Type: %d\n", disk->type);
    }
}

/*
 * A function to initialize the shell
 * @param void
 */
void shell_init(void) {
    console_clear();
    console_puts(SHELL_PROMPT);

    commands.command_count = 0;
    command_buffer_pos = 0;
    command_buffer[0] = '\0';
    serial_puts("shell_init: initialized command buffer\n");

    shell_command_t clear_command = {
        .name = "clear",
        .handler = shell_command_clear,
        .description = "Clears the console screen"
    };
    shell_register_command(&clear_command);

    shell_command_t version_command = {
        .name = "version",
        .handler = shell_command_version,
        .description = "Displays the kernel version"
    };
    shell_register_command(&version_command);

    shell_command_t reboot_command = {
        .name = "reboot",
        .handler = shell_command_reboot,
        .description = "Reboots the system"
    };
    shell_register_command(&reboot_command);

    shell_command_t time_command = {
        .name = "time",
        .handler = shell_command_time,
        .description = "Displays the current RTC time"
    };
    shell_register_command(&time_command);
    
    shell_command_t mmapinfo_command = {
        .name = "mmapinfo",
        .handler = shell_command_mmapinfo,
        .description = "Displays memory map information"
    };
    shell_register_command(&mmapinfo_command);

    shell_command_t rsdpinfo_command = {
        .name = "rsdpinfo",
        .handler = shell_command_rsdpinfo,
        .description = "Displays ACPI RSDP information"
    };
    shell_register_command(&rsdpinfo_command);

    shell_command_t rsdtinfo_command = {
        .name = "rsdtinfo",
        .handler = shell_command_rsdtinfo,
        .description = "Displays ACPI RSDT information"
    };
    shell_register_command(&rsdtinfo_command);

    shell_command_t fadtinfo_command = {
        .name = "fadtinfo",
        .handler = shell_command_fadtinfo,
        .description = "Displays ACPI FADT information"
    };
    shell_register_command(&fadtinfo_command);

    shell_command_t madtinfo_command = {
        .name = "madtinfo",
        .handler = shell_command_madtinfo,
        .description = "Displays ACPI MADT information"
    };
    shell_register_command(&madtinfo_command);

    shell_command_t diskinfo_command = {
        .name = "diskinfo",
        .handler = shell_command_diskinfo,
        .description = "Displays information about a specific disk"
    };
    shell_register_command(&diskinfo_command);
}

/*
 * Handle a complete command
 * @param command: The command string to handle
 */
void shell_handle_command(const char* command) {
    if (command == NULL || command[0] == '\0') return;
    
    char* argv[MAX_ARGUMENTS];
    int argc = 0;

    char* cmd_copy = (char*)command;
    char* token = cmd_copy;

    while (*cmd_copy) {
        if (*cmd_copy == ' ') {
            *cmd_copy = '\0';
            if (token != cmd_copy && argc < MAX_ARGUMENTS) {
                argv[argc++] = token;
            }
            token = cmd_copy + 1;
        }
        cmd_copy++;
    }
    if (argc < MAX_ARGUMENTS && *token != '\0') {
        argv[argc++] = token;

    }
    
    if (argc == 0) return;

    for (size_t i = 0; i < commands.command_count; i++) {
        if (strcmp(argv[0], commands.commands[i].name) == 0) {
            commands.commands[i].handler(argc, argv);
            return;
        }
    }

    console_puts("Unknown command: ");
    console_puts(argv[0]);
    console_putc('\n');
}

/*
 * Register a new command
 * @param command: Pointer to the command structure of the command to register
 */
void shell_register_command(const shell_command_t *command) {
    if (!command) return;

    serial_puts("Shell: Registering command: ");
    serial_puts(command->name);
    serial_puts("\n");
    
    if (commands.command_count >= MAX_COMMANDS) {
        serial_puts("Shell: Too many commands registered!\n");
        return;
    }

    commands.commands[commands.command_count].name = command->name;
    commands.commands[commands.command_count].handler = command->handler;
    commands.commands[commands.command_count].description = command->description;
    commands.command_count++;
}

/*
 * Handle input character for the shell
 * @param c: The input character
 */
void shell_handle_input(char c) {
    if (c != (char)0) {
        if (c == '\b') {
            if (command_buffer_pos > 0) {
                command_buffer_pos--;
                console_putc(c);
            }
            return;
        } 
        
        if (c == '\n') {
            console_putc(c);
            command_buffer[command_buffer_pos] = '\0';

            shell_handle_command(command_buffer);

            command_buffer_pos = 0;
            command_buffer[0] = '\0';

            console_puts(SHELL_PROMPT);
            return;
        }

        if (command_buffer_pos < MAX_COMMAND_LENGTH - 1) {
            command_buffer[command_buffer_pos] = c;
            command_buffer_pos++;
            command_buffer[command_buffer_pos] = '\0';
            console_putc(c);
            return;
        }
    }   
}
