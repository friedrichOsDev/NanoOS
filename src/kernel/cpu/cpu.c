#include <cpu.h>
#include <serial.h>

void cpu_enable_write_combining(uint32_t base, uint32_t size) {
    uint32_t mtrr_cap_low, mtrr_cap_high;
    cpu_get_msr(MSR_IA32_MTRRCAP, &mtrr_cap_low, &mtrr_cap_high);

    if (!(mtrr_cap_low & (1 << 8))) {
        serial_puts("cpu_enable_write_combining: MTRRs not supported\n");
        return;
    }

    if (!(mtrr_cap_low & (1 << 10))) {
        serial_puts("cpu_enable_write_combining: Write-combining not supported\n");
        return;
    }

    int vcnt = mtrr_cap_low & 0xFF;
    serial_puts("cpu_enable_write_combining: Found ");
    serial_put_int(vcnt);
    serial_puts("\n");

    serial_puts("cpu_enable_write_combining: Disabling interrupts\n");
    cpu_disable_interrupts();

    uint32_t cr0 = cpu_get_cr0();
    serial_puts("cpu_enable_write_combining: Original CR0: 0x");
    serial_put_hex(cr0);
    serial_puts("\n");

    serial_puts("cpu_enable_write_combining: Entering no-fill cache mode\n");
    cpu_set_cr0((cr0 & ~0x20000000) | 0x40000000);

    serial_puts("cpu_enable_write_combining: Flushing caches\n");
    cpu_wbinvd();

    uint32_t mtrr_def_type_low, mtrr_def_type_high;
    cpu_get_msr(MSR_IA32_MTRR_DEF_TYPE, &mtrr_def_type_low, &mtrr_def_type_high);
    serial_puts("cpu_enable_write_combining: Disabling MTRRs\n");
    cpu_set_msr(MSR_IA32_MTRR_DEF_TYPE, mtrr_def_type_low & ~0x800, mtrr_def_type_high);

    int free_mtrr = -1;
    for (int i = 0; i < vcnt; i++) {
        uint32_t phys_mask_low, phys_mask_high;
        cpu_get_msr(MSR_IA32_MTRR_PHYSMASK0 + i * 2, &phys_mask_low, &phys_mask_high);
        if (!(phys_mask_low & 0x800)) {
            free_mtrr = i;
            break;
        }
    }

    if (free_mtrr == -1) {
        serial_puts("cpu_enable_write_combining: No free MTRRs found\n");
    } else {
        serial_puts("cpu_enable_write_combining: Found free MTRR at index ");
        serial_put_int(free_mtrr);
        serial_puts("\n");

        uint32_t mask = ~(size - 1);
        serial_puts("cpu_enable_write_combining: Setting MTRR for base 0x");
        serial_put_hex(base);
        serial_puts(" size 0x");
        serial_put_hex(size);
        serial_puts("\n");

        cpu_set_msr(MSR_IA32_MTRR_PHYSBASE0 + free_mtrr * 2, base | 6, 0); // 6 is WC type
        cpu_set_msr(MSR_IA32_MTRR_PHYSMASK0 + free_mtrr * 2, mask | 0x800, 0);
    }
    
    serial_puts("cpu_enable_write_combining: Re-enabling MTRRs\n");
    cpu_set_msr(MSR_IA32_MTRR_DEF_TYPE, mtrr_def_type_low | 0x800, mtrr_def_type_high);

    serial_puts("cpu_enable_write_combining: Flushing caches again\n");
    cpu_wbinvd();

    serial_puts("cpu_enable_write_combining: Restoring CR0\n");
    cpu_set_cr0(cr0);

    serial_puts("cpu_enable_write_combining: Enabling interrupts\n");
    cpu_enable_interrupts();

    serial_puts("cpu_enable_write_combining: MTRR WC enabled\n");
}
