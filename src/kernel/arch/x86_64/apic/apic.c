/**
 * @file apic.c
 * @brief Initialization of the APIC.
 * @author friedrichOsDev
 */

#include "apic.h"
#include "../acpi/acpi.h"
#include "pic.h"
#include <arch/arch.h>
#include <kernel/panic.h>
#include <kernel/sync.h>
#include <mm/memory.h>

static volatile uint8_t *lapic_base = NULL;
static volatile uint8_t *ioapic_base = NULL;

static spinlock_t ioapic_lock = SPINLOCK_INIT;
static spinlock_t icr_lock = SPINLOCK_INIT;

static bool apic_initialized = false;

uint32_t lapic_timer_calibrated_initcnt = 0;
uint32_t lapic_timer_target_hz = 0;

/**
 * @brief Writes a value to a IOAPIC register.
 *
 * @param[in] reg The register to write to.
 * @param[in] val The value to write.
 */
static void ioapic_write(uint32_t reg, uint32_t val) {
    irq_flags_t flags = spinlock_acquire_irqsave(&ioapic_lock);

    volatile uint32_t *index_reg = (volatile uint32_t *)(ioapic_base + IOAPIC_REG_INDEX);
    volatile uint32_t *data_reg = (volatile uint32_t *)(ioapic_base + IOAPIC_REG_DATA);

    *index_reg = reg;
    *data_reg = val;

    spinlock_release_irqrestore(&ioapic_lock, flags);
}

/**
 * @brief Reads a value from a IOAPIC register
 *
 * @param[in] reg The register to read from.
 * @return The value read from the register.
 */
static uint32_t ioapic_read(uint32_t reg) {
    irq_flags_t flags = spinlock_acquire_irqsave(&ioapic_lock);

    volatile uint32_t *index_reg = (volatile uint32_t *)(ioapic_base + IOAPIC_REG_INDEX);
    volatile uint32_t *data_reg = (volatile uint32_t *)(ioapic_base + IOAPIC_REG_DATA);

    *index_reg = reg;
    uint32_t ret = *data_reg;

    spinlock_release_irqrestore(&ioapic_lock, flags);
    return ret;
}

/**
 * @brief Initializes the APIC.
 *
 * @return E_OK on success, memory error otherwise.
 */
kerr_t apic_init() {
    pic_disable();
    log_debug("8259 PIC disabled");

    uint64_t lapic_phys = madt->local_apic_address;
    if (madt_parsed.lapic_override_count > 0 && madt_parsed.lapic_overrides != NULL) {
        lapic_phys = madt_parsed.lapic_overrides[0].local_apic_address;
        log_debug("LAPIC address overridden by ACPI table\n");
    }

    uint64_t ioapic_phys = IOAPIC_DEFAULT_PHYS;
    if (madt_parsed.ioapic_count > 0 && madt_parsed.ioapics != NULL) {
        ioapic_phys = madt_parsed.ioapics[0].ioapic_address;
    } else {
        log_debug("using fallback IOAPIC address (Phys: %x)", ioapic_phys);
    }

    log_debug("mapping LAPIC (Phys: %x) and IOAPIC (Phys: %x)", lapic_phys, ioapic_phys);

    virt_addr_t mem;
    kerr_t err = vmm_map_mmio(vmm_kernel_space(), lapic_phys, 1, &mem);
    if (k_failed(err)) {
        log_error("failed to map LAPIC memory");
        return err;
    }
    lapic_base = (volatile uint8_t *)mem;
    err = vmm_map_mmio(vmm_kernel_space(), ioapic_phys, 1, &mem);
    if (k_failed(err)) {
        log_error("failed to map IOAPIC memory");
        return err;
    }
    ioapic_base = (volatile uint8_t *)mem;

    // activate LAPIC via MSR
#define IA32_APIC_BASE_MSR 0x1B
#define IA32_APIC_BASE_MSR_ENABLE 0x800
    uint32_t low, high;
    ASM_CODE("rdmsr" : "=a"(low), "=d"(high) : "c"(IA32_APIC_BASE_MSR));
    uint64_t apic_base_msr = ((uint64_t)high << 32) | low;
    apic_base_msr |= IA32_APIC_BASE_MSR_ENABLE;
    low = apic_base_msr & 0xFFFFFFFF;
    high = (uint32_t)(apic_base_msr >> 32);
    ASM_CODE("wrmsr" ::"a"(low), "d"(high), "c"(IA32_APIC_BASE_MSR));

    lapic_write(LAPIC_REG_DFR, 0xFFFFFFFF);
    lapic_write(LAPIC_REG_LDR, (lapic_read(LAPIC_REG_LDR) & 0x00FFFFFF) | (1 << 24));
    lapic_write(LAPIC_REG_SIVR, lapic_read(LAPIC_REG_SIVR) | 0x100 | 0xFF);
    lapic_write(LAPIC_REG_TPR, 0);

    uint32_t ver = ioapic_read(IOAPIC_REG_VER);
    uint32_t max_intr = (ver >> 16) & 0xFF;
    for (uint32_t i = 0; i <= max_intr; i++) {
        ioapic_write(IOAPIC_REG_RED_TABLE(i), 0x00010000);
        ioapic_write(IOAPIC_REG_RED_TABLE(i) + 1, 0);
    }

    log_debug("LAPIC initialized and enabled. BSP ID: %x", lapic_read(LAPIC_REG_ID) >> 24);

    apic_initialized = true;
    return E_OK;
}

/**
 * @brief Writes a value to a LAPIC register.
 *
 * @param[in] reg The register to write to.
 * @param[in] val The value to write to the register.
 */
void lapic_write(uint32_t reg, uint32_t val) {
    volatile uint32_t *addr = (volatile uint32_t *)(lapic_base + reg);
    *addr = val;
}

/**
 * @brief Reads a value from a LAPIC register.
 *
 * @param[in] reg The register to read from.
 * @return The value read from the register.
 */
uint32_t lapic_read(uint32_t reg) {
    volatile uint32_t *addr = (volatile uint32_t *)(lapic_base + reg);
    return *addr;
}

/** @brief Send EOI (End Of Interrupt). */
void lapic_eoi() { lapic_write(LAPIC_REG_EOI, 0); }

/**
 * @brief Route an IRQ to a specific CPU.
 *
 * @param[in] irq The IRQ number to route.
 * @param[in] vector The IRQ vector to route to.
 * @param[in] cpu_id The CPU ID to route the IRQ to.
 */
void ioapic_route_irq(uint8_t irq, uint8_t vector, uint8_t cpu_id) {
    uint32_t gsi = irq;
    uint32_t flags = 0;

    if (madt_parsed.iso_count > 0 && madt_parsed.isos != NULL) {
        for (uint32_t i = 0; i < madt_parsed.iso_count; i++) {
            madt_iso_entry_t iso = madt_parsed.isos[i];
            if (iso.source == irq) {
                gsi = iso.global_system_interrupt;

                uint16_t iso_flags = iso.flags;
                uint16_t polarity = iso_flags & 0x3;
                uint16_t trigger = (iso_flags >> 2) & 0x3;

                if (polarity == 3) {
                    flags |= (1 << 13); // Active Low
                }
                if (trigger == 3) {
                    flags |= (1 << 15); // Level triggered
                }
            }
        }
    }

    uint32_t low = vector | flags; // Mask = 0 (Unmasked), Destination Mode = 0 (Physical), Delivery Mode = 000 (Fixed)
    uint32_t high = ((uint32_t)cpu_id) << 24;

    ioapic_write(IOAPIC_REG_RED_TABLE(gsi), low);
    ioapic_write(IOAPIC_REG_RED_TABLE(gsi) + 1, high);
}

/**
 * @brief Get the ID of the local APIC.
 *
 * @return The ID of the local APIC.
 */
uint32_t lapic_get_id() {
    if (!lapic_base) {
        log_warn("APIC may be uninitialized");
        return 0;
    }
    return (lapic_read(LAPIC_REG_ID) >> 24) & 0xFF;
}

/**
 * @brief Send an INIT IPI to a LAPIC.
 *
 * @param[in] lapic_id The ID of the LAPIC to send the IPI to.
 */
void lapic_send_init(uint32_t lapic_id) {
    lapic_write(LAPIC_REG_ICR_HIGH, ((uint32_t)lapic_id) << 24);
    // Delivery Mode = 5 (INIT), Assert (1 << 14), Edge Triggered
    lapic_write(LAPIC_REG_ICR_LOW, 0x00004500);
}

/**
 * @brief Send a STARTUP IPI to a LAPIC.
 *
 * @param[in] lapic_id The ID of the LAPIC to send the IPI to.
 * @param[in] vector The vector number of the startup function.
 */
void lapic_send_sipi(uint32_t lapic_id, uint8_t vector) {
    lapic_write(LAPIC_REG_ICR_HIGH, ((uint32_t)lapic_id) << 24);
    // Delivery Mode = 6 (Startup), Assert (1 << 14), Vektor
    lapic_write(LAPIC_REG_ICR_LOW, 0x00004600 | vector);
}

/** @brief Send a BROADCAST RESCHEDULE IPI to all LAPICs. */
void lapic_send_broadcast_reschedule_ipi() {
    if (!lapic_base) {
        log_warn("APIC may be uninitialized");
        return;
    }

    irq_flags_t flags = spinlock_acquire_irqsave(&icr_lock);

    while (lapic_read(LAPIC_REG_ICR_LOW) & (1 << 12)) {
        arch_pause();
    }
    // Shorthand = 3 (All Excluding Self), Delivery Mode = Fixed (0), Vektor = 0xFD
    lapic_write(LAPIC_REG_ICR_HIGH, 0);
    lapic_write(LAPIC_REG_ICR_LOW, (3 << 18) | IPI_RESCHEDULE_VECTOR);

    spinlock_release_irqrestore(&icr_lock, flags);
}

/** @brief Send a BROADCAST STOP IPI to all LAPICs. */
void lapic_send_broadcast_stop_ipi() {
    if (!lapic_base) {
        log_warn("APIC may be uninitialized");
        return;
    }

    irq_flags_t flags = spinlock_acquire_irqsave(&icr_lock);

    while (lapic_read(LAPIC_REG_ICR_LOW) & (1 << 12)) {
        arch_pause();
    }

    // Shorthand = 3 (All Excluding Self), Delivery Mode = Fixed (0), Vektor = 0xFC
    lapic_write(LAPIC_REG_ICR_HIGH, 0);
    lapic_write(LAPIC_REG_ICR_LOW, (3 << 18) | IPI_STOP_VECTOR);

    spinlock_release_irqrestore(&icr_lock, flags);
}

/** @brief Send a BROADCAST TLB IPI to all LAPICs. */
void lapic_send_broadcast_tlb_ipi() {
    if (!lapic_base) {
        log_warn("APIC may be uninitialized");
        return;
    }

    irq_flags_t flags = spinlock_acquire_irqsave(&icr_lock);

    while (lapic_read(LAPIC_REG_ICR_LOW) & (1 << 12)) {
        arch_pause();
    }

    // Shorthand = 3 (All Excluding Self), Delivery Mode = Fixed (0), Vektor = 0xFB
    lapic_write(LAPIC_REG_ICR_HIGH, 0);
    lapic_write(LAPIC_REG_ICR_LOW, (3 << 18) | IPI_TLB_SHOOTDOWN_VECTOR);

    spinlock_release_irqrestore(&icr_lock, flags);
}

/**
 * @brief Calibrate the LAPIC timer and start it.
 *
 * @param[in] target_hz The target frequency for the LAPIC timer in Hz.
 */
void lapic_timer_calibrate_and_start(uint32_t target_hz) {
    lapic_timer_target_hz = target_hz;

    lapic_write(LAPIC_REG_TIMER_DIV, LAPIC_TIMER_DIV_16);
    lapic_write(LAPIC_REG_TIMER_LVT, LAPIC_TIMER_MASKED);
    lapic_write(LAPIC_REG_TIMER_INITCNT, 0xFFFFFFFF);

    hpet_mdelay(10);
    uint32_t elapsed = 0xFFFFFFFF - lapic_read(LAPIC_REG_TIMER_CURRCNT);
    uint64_t ticks_per_second = (uint64_t)elapsed * 100; // 10ms * 100 = 1s
    uint32_t init_count = (uint32_t)(ticks_per_second / target_hz);

    lapic_timer_calibrated_initcnt = init_count;

    log_debug("LAPIC timer calibrated on CPU %d: %llu ticks/s, init_count=%u for %u Hz", lapic_get_id(), ticks_per_second, init_count, target_hz);

    lapic_write(LAPIC_REG_TIMER_DIV, LAPIC_TIMER_DIV_16);
    lapic_write(LAPIC_REG_TIMER_LVT, LAPIC_TIMER_PERIODIC | LAPIC_TIMER_VECTOR);
    lapic_write(LAPIC_REG_TIMER_INITCNT, init_count);
}

/**
 * @brief Initializes the LAPIC timer with the specified target frequency.
 *
 * @param[in] target_hz The target frequency for the LAPIC timer in Hz.
 */
void lapic_init(uint32_t target_hz) {
    lapic_timer_calibrate_and_start(target_hz);
}

/** @brief Starts the LAPIC timer on a AP (Application Processor) core. */
void lapic_timer_start_ap() {
    if (lapic_timer_calibrated_initcnt == 0) {
        log_error("LAPIC timer not calibrated yet");
        return;
    }

    lapic_write(LAPIC_REG_TIMER_DIV, LAPIC_TIMER_DIV_16);
    lapic_write(LAPIC_REG_TIMER_LVT, LAPIC_TIMER_PERIODIC | LAPIC_TIMER_VECTOR);
    lapic_write(LAPIC_REG_TIMER_INITCNT, lapic_timer_calibrated_initcnt);

    log_debug("LAPIC timer started on CPU %d (init_count=%u, %u Hz)", lapic_get_id(), lapic_timer_calibrated_initcnt, lapic_timer_target_hz);
}

/** @brief Send a BROADCAST RESCHEDULE IPI to all LAPICs. */
void arch_ipi_reschedule() {
    lapic_send_broadcast_reschedule_ipi();
}

/** @brief Send a STOP IPI to all LAPICs. */
void arch_ipi_stop_others() {
    lapic_send_broadcast_stop_ipi();
}

/**
 * @brief Flush the TLB of the local CPU.
 *
 * @param[in] vaddr The virtual address to flush the TLB for.
 */
void arch_tlb_flush_local(virt_addr_t vaddr) {
    ASM_CODE("invlpg (%0)" : : "r"(vaddr) : "memory");
}

/** @brief Send a BROADCAST TLB IPI to all LAPICs. */
void arch_tlb_shootdown() {
    lapic_send_broadcast_tlb_ipi();
}

/**
 * @brief Check if the APIC is initialized.
 *
 * @return true if initialized, false otherwise.
 */
bool is_apic_initialized() {
    return apic_initialized;
}
