/**
 * @file smp.c
 * @brief Symmetric Multiprocessing (SMP) Implementation
 * @author friedrichOsDev
 */

#include <arch/x86_64/cpu/acpi.h>
#include <arch/x86_64/cpu/apic.h>
#include <arch/x86_64/cpu/gdt.h>
#include <arch/x86_64/cpu/hpet.h>
#include <arch/x86_64/cpu/idt.h>
#include <arch/x86_64/cpu/smp.h>
#include <arch/x86_64/drivers/serial.h>
#include <arch/x86_64/mm/heap.h>
#include <arch/x86_64/mm/vmm.h>
#include <core/panic.h>
#include <core/scheduler.h>
#include <lib/print.h>
#include <lib/string.h>

extern uint8_t smp_trampoline_start[];
extern uint8_t smp_trampoline_end[];
extern uint64_t smp_trampoline_pml4;
extern uint64_t smp_trampoline_stack;
extern uint64_t smp_trampoline_entry;

extern phys_addr_t kernel_pml4_phys;
extern struct gdt_ptr gdtp;
extern struct idt_ptr idtp;

cpu_local_t cpus[MAX_CPUS];
size_t smp_cpu_count = 1;

static volatile bool ap_boot_flag = false;

static void smp_enable_lapic(void) {
    // activate LAPIC in MSR
    uint32_t low, high;
    __asm__ __volatile__("rdmsr" : "=a"(low), "=d"(high) : "c"(0x1B));
    low |= (1 << 11);
    __asm__ __volatile__("wrmsr" : : "a"(low), "d"(high), "c"(0x1B));

    lapic_write(LAPIC_REG_DFR, 0xFFFFFFFF);
    lapic_write(LAPIC_REG_LDR,
                (lapic_read(LAPIC_REG_LDR) & 0x00FFFFFF) | (1 << 24));
    lapic_write(LAPIC_REG_SIVR, lapic_read(LAPIC_REG_SIVR) | 0x100 | 0xFF);
    lapic_write(LAPIC_REG_TPR, 0);
}

cpu_local_t *smp_get_current_cpu(void) {
    uint32_t lapic_id = lapic_get_id();
    for (size_t i = 0; i < smp_cpu_count; i++) {
        if (cpus[i].lapic_id == lapic_id) {
            return &cpus[i];
        }
    }
    return &cpus[0];
}

/**
 * 64-Bit C entry for all Application Processors (APs)
 */
void smp_ap_main(void) {
    // load GDT & IDT
    gdt_flush((uint64_t)&gdtp);
    idt_load((uint64_t)&idtp);

    // initialize Local APIC of the AP
    smp_enable_lapic();

    lapic_timer_start_ap();

    uint32_t lapic_id = lapic_get_id();
    cpu_local_t *local_cpu = NULL;
    for (size_t i = 0; i < smp_cpu_count; i++) {
        if (cpus[i].lapic_id == lapic_id) {
            local_cpu = &cpus[i];
            break;
        }
    }

    if (!local_cpu) {
        panic("SMP: AP core booted with unknown LAPIC ID", lapic_id);
    }

    // create different idle tasks for each core
    char idle_name[16];
    snprintf(idle_name, sizeof(idle_name), "idle_%d", local_cpu->cpu_id);
    local_cpu->idle_thread = thread_create_on_cpu(
        kernel_process, idle_task, NULL, idle_name, local_cpu->cpu_id);
    pop_next_ready_thread_for_cpu(local_cpu->cpu_id);

    local_cpu->current_thread = local_cpu->idle_thread;

    local_cpu->online = true;
    ap_boot_flag = true;

    serial_printf(COM1, "SMP: CPU Core %d (APIC ID %d) online and ready!\n",
                  local_cpu->cpu_id, local_cpu->lapic_id);

    // enable interrupts
    __asm__ __volatile__("sti");

    // start scheduler at the core
    while (1) {
        scheduler_schedule();
        __asm__ __volatile__("hlt");
    }
}

void smp_init(void) {
    serial_printf(COM1, "SMP: scanning MADT for CPU cores...\n");

    if (!madt) {
        serial_printf(
            COM1, "SMP: no MADT table found, staying in single-core mode\n");
        return;
    }

    uint32_t bsp_lapic_id = lapic_get_id();

    // register BSP (Core 0)
    cpus[0].cpu_id = 0;
    cpus[0].lapic_id = bsp_lapic_id;
    cpus[0].online = true;

    // search MADT for all cores
    uint8_t *ptr = madt->entries;
    uint8_t *end = (uint8_t *)madt + madt->header.length;

    while (ptr < end) {
        madt_entry_header_t *entry = (madt_entry_header_t *)ptr;
        if (entry->type == MADT_LAPIC_TYPE) {
            madt_lapic_entry_t *lapic = (madt_lapic_entry_t *)entry;
            // Flags Bit 0 = Processor Enabled
            if ((lapic->flags & 1) && lapic->apic_id != bsp_lapic_id) {
                if (smp_cpu_count < MAX_CPUS) {
                    cpus[smp_cpu_count].cpu_id = smp_cpu_count;
                    cpus[smp_cpu_count].lapic_id = lapic->apic_id;
                    cpus[smp_cpu_count].online = false;
                    smp_cpu_count++;
                }
            }
        }
        ptr += entry->length;
    }

    serial_printf(COM1, "SMP: detected %d CPU core(s) in system\n",
                  smp_cpu_count);
    if (smp_cpu_count == 1)
        return;

    // copy smb trampoline
    size_t trampoline_size =
        (size_t)(smp_trampoline_end - smp_trampoline_start);
    memcpy((void *)P2V(0x8000), smp_trampoline_start, trampoline_size);

    // start every core
    for (size_t i = 1; i < smp_cpu_count; i++) {
        uint32_t target_apic_id = cpus[i].lapic_id;

        // allocate stack for the core
        void *ap_stack = (void *)kmalloc(STACK_SIZE);
        uint64_t ap_stack_top = (uint64_t)ap_stack + STACK_SIZE;

        // write the parameter (for each core different)
        uint64_t *trampoline_vars =
            (uint64_t *)P2V(0x8000 + ((uint64_t)&smp_trampoline_pml4 -
                                      (uint64_t)smp_trampoline_start));

        trampoline_vars[0] = kernel_pml4_phys;      // smp_trampoline_pml4
        trampoline_vars[1] = ap_stack_top;          // smp_trampoline_stack
        trampoline_vars[2] = (uint64_t)smp_ap_main; // smp_trampoline_entry

        ap_boot_flag = false;

        serial_printf(COM1, "SMP: booting Core %d (APIC ID %d)...\n", i,
                      target_apic_id);

        // INIT-SIPI-SIPI
        lapic_send_init(target_apic_id);
        hpet_mdelay(10); // wait 10 ms

        // Startup IPI witch Vektor 0x08 (phys: 0x08 * 4096 = 0x8000)
        lapic_send_sipi(target_apic_id, 0x08);
        hpet_udelay(200); // wait 200 us

        if (!ap_boot_flag) {
            lapic_send_sipi(target_apic_id, 0x08);
            hpet_mdelay(10);
        }

        // wait 50 ms for the core to be bootet
        uint64_t timeout = 50;
        while (!ap_boot_flag && timeout > 0) {
            hpet_mdelay(1);
            timeout--;
        }

        if (cpus[i].online) {
            serial_printf(COM1, "SMP: Core %d successfully initialized!\n", i);
        } else {
            serial_printf(
                COM1, "SMP: warning Core %d failed to boot (timeout)!\n", i);
        }
    }

    serial_printf(COM1, "SMP: all available cores started.\n");
}