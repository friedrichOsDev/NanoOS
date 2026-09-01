/**
 * @file apic.c
 * @brief Advanced Programmable Interrupt Controller Code
 * @author friedrichOsDev
 */

#include <arch/x86_64/cpu/acpi.h>
#include <arch/x86_64/cpu/apic.h>
#include <arch/x86_64/cpu/pic.h>
#include <arch/x86_64/drivers/serial.h>
#include <arch/x86_64/mm/vmm.h>
#include <core/panic.h>
#include <arch/x86_64/cpu/hpet.h>
#include <core/sync.h>

static volatile uint8_t *lapic_base = NULL;
static volatile uint8_t *ioapic_base = NULL;
static spinlock_t ioapic_lock = SPINLOCK_INIT;
static spinlock_t icr_lock = SPINLOCK_INIT;

uint32_t lapic_timer_calibrated_initcnt = 0;
uint32_t lapic_timer_target_hz = 0;

/**
 * Writes a Value to a register of the I/O-APIC
 * @param reg The register
 * @param val The value
 */
static void ioapic_write(uint32_t reg, uint32_t val) {
    uint64_t flags = spinlock_acquire_irqsave(&ioapic_lock);

    volatile uint32_t *index_reg =
        (volatile uint32_t *)(ioapic_base + IOAPIC_REG_INDEX);
    volatile uint32_t *data_reg =
        (volatile uint32_t *)(ioapic_base + IOAPIC_REG_DATA);

    *index_reg = reg;
    *data_reg = val;

    spinlock_release_irqrestore(&ioapic_lock, flags);
}

/**
 * Reads a Value from a register of the I/O-APIC
 * @param reg The register
 * @return Returns the read value
 */
static uint32_t ioapic_read(uint32_t reg) {
    uint64_t flags = spinlock_acquire_irqsave(&ioapic_lock);

    volatile uint32_t *index_reg =
        (volatile uint32_t *)(ioapic_base + IOAPIC_REG_INDEX);
    volatile uint32_t *data_reg =
        (volatile uint32_t *)(ioapic_base + IOAPIC_REG_DATA);

    *index_reg = reg;
    uint32_t ret = *data_reg;

    spinlock_release_irqrestore(&ioapic_lock, flags);
    return ret;
}

/**
 * Initializes the APIC
 */
void apic_init() {
    pic_disable();
    serial_printf(COM1, "APIC: 8259 PIC disabled\n");

    uint64_t lapic_phys = madt->local_apic_address;
    if (madt_parsed.lapic_override_count > 0 &&
        madt_parsed.lapic_overrides != NULL) {
        lapic_phys = madt_parsed.lapic_overrides[0].local_apic_address;
        serial_printf(COM1, "APIC: LAPIC address overridden by ACPI table\n");
    }

    uint64_t ioapic_phys = IOAPIC_DEFAULT_PHYS;
    if (madt_parsed.ioapic_count > 0 && madt_parsed.ioapics != NULL) {
        ioapic_phys = madt_parsed.ioapics[0].ioapic_address;
    } else {
        serial_printf(COM1, "APIC: using fallback IOAPIC address (Phys: %x)\n",
                      ioapic_phys);
    }

    serial_printf(COM1, "APIC: Mapping LAPIC (Phys: %x) & IOAPIC (Phys: %x)\n",
                  lapic_phys, ioapic_phys);
    lapic_base = (volatile uint8_t *)vmm_map_mmio((page_table_t *)kernel_pml4,
                                                  lapic_phys, 1);
    ioapic_base = (volatile uint8_t *)vmm_map_mmio((page_table_t *)kernel_pml4,
                                                   ioapic_phys, 1);

    if (!lapic_base || !ioapic_base) {
        panic("apic failed to map lapic or ioapic address", 0);
    }

// 1. Enable LAPIC in MSR
#define IA32_APIC_BASE_MSR 0x1B
#define IA32_APIC_BASE_MSR_ENABLE 0x800
    uint32_t low, high;
    __asm__ __volatile__("rdmsr"
                         : "=a"(low), "=d"(high)
                         : "c"(IA32_APIC_BASE_MSR));
    uint64_t apic_base_msr = ((uint64_t)high << 32) | low;
    apic_base_msr |= IA32_APIC_BASE_MSR_ENABLE;
    low = apic_base_msr & 0xFFFFFFFF;
    high = apic_base_msr >> 32;
    __asm__ __volatile__("wrmsr"
                         :
                         : "a"(low), "d"(high), "c"(IA32_APIC_BASE_MSR));

    // 2. Set Logical Destination and Destination Format Registers
    lapic_write(LAPIC_REG_DFR, 0xFFFFFFFF); // Flat mode
    lapic_write(LAPIC_REG_LDR, (lapic_read(LAPIC_REG_LDR) & 0x00FFFFFF) |
                                   (1 << 24)); // Logical ID 1

    // 3. Enable LAPIC software-wise and map Spurious Interrupt Vector to 0xFF
    lapic_write(LAPIC_REG_SIVR, lapic_read(LAPIC_REG_SIVR) | 0x100 | 0xFF);

    // 4. Clear Task Priority Register to accept all interrupts
    lapic_write(LAPIC_REG_TPR, 0);

    // 5. Mask all redirection entries of I/O APIC by default
    uint32_t ver = ioapic_read(IOAPIC_REG_VER);
    uint32_t max_intr = (ver >> 16) & 0xFF;
    for (uint32_t i = 0; i <= max_intr; i++) {
        ioapic_write(
            IOAPIC_REG_RED_TABLE(i),
            0x00010000); // Masked, Edge, Active High, Physical, Fixed Vector 0
        ioapic_write(IOAPIC_REG_RED_TABLE(i) + 1, 0);
    }

    serial_printf(COM1, "APIC: LAPIC initialized and enabled. BSP ID: %x\n",
                  lapic_read(LAPIC_REG_ID) >> 24);
}

/**
 * Writes a Value to a register of the local APIC
 * @param reg The register
 * @param val
 */
void lapic_write(uint32_t reg, uint32_t val) {
    volatile uint32_t *addr = (volatile uint32_t *)(lapic_base + reg);
    *addr = val;
}

/**
 * Reads a Value from a register of the local APIC
 * @param reg The register
 * @return Returns the read value
 */
uint32_t lapic_read(uint32_t reg) {
    volatile uint32_t *addr = (volatile uint32_t *)(lapic_base + reg);
    return *addr;
}

/**
 * Sends the "End of Interrupt" signal to the local APIC
 */
void lapic_eoi(void) { lapic_write(LAPIC_REG_EOI, 0); }

/**
 * Routes an external hardware interrupt (IRQ) to an IDT vector
 * @param irq The classic ISA interrupt vector (e.g. 1 for keyboard, or GSI pin
 * like 20)
 * @param vector The desired target IDT vector (e.g. 0x20/32)
 * @param cpu_id The ID of the target core (usually 0)
 */
void ioapic_route_irq(uint8_t irq, uint8_t vector, uint8_t cpu_id) {
    uint32_t gsi = irq;
    uint32_t flags = 0; // Default: edge triggered, active high, unmasked

    // Search MADT for Interrupt Source Override (ISO)
    if (madt_parsed.iso_count > 0 && madt_parsed.isos != NULL) {
        for (uint32_t i = 0; i < madt_parsed.iso_count; i++) {
            madt_iso_entry_t iso = madt_parsed.isos[i];
            if (iso.source == irq) {
                gsi = iso.global_system_interrupt;

                // Parse polarity and trigger flags
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

    uint32_t low = vector | flags; // Mask = 0 (Unmasked), Destination Mode = 0
                                   // (Physical), Delivery Mode = 000 (Fixed)
    uint32_t high = ((uint32_t)cpu_id) << 24;

    ioapic_write(IOAPIC_REG_RED_TABLE(gsi), low);
    ioapic_write(IOAPIC_REG_RED_TABLE(gsi) + 1, high);
}

/**
 * Returns the hardware APIC ID of the current core
 */
uint32_t lapic_get_id(void) {
    if (!lapic_base)
        return 0;
    return (lapic_read(LAPIC_REG_ID) >> 24) & 0xFF;
}

/**
 * Sends INIT IPI to a target core
 * @param lapic_id the ID of the core to send the init to
 */
void lapic_send_init(uint32_t lapic_id) {
    lapic_write(LAPIC_REG_ICR_HIGH, ((uint32_t)lapic_id) << 24);
    // Delivery Mode = 5 (INIT), Assert (1 << 14), Edge Triggered
    lapic_write(LAPIC_REG_ICR_LOW, 0x00004500);
}

/**
 * Sends startup IPI (SIPI) to a target core
 * @param lapic_id the ID of the core to send the sipi to
 * @param vector page number in 1M region (e.g. 0x08 for phys 0x8000)
 */
void lapic_send_sipi(uint32_t lapic_id, uint8_t vector) {
    lapic_write(LAPIC_REG_ICR_HIGH, ((uint32_t)lapic_id) << 24);
    // Delivery Mode = 6 (Startup), Assert (1 << 14), Vektor
    lapic_write(LAPIC_REG_ICR_LOW, 0x00004600 | vector);
}

/**
 * Sends reschedule IPI to all cores
 */
void lapic_send_broadcast_reschedule_ipi() {
    if (!lapic_base)
        return;

    uint64_t flags = spinlock_acquire_irqsave(&icr_lock);

    while (lapic_read(LAPIC_REG_ICR_LOW) & (1 << 12)) {
        __asm__ __volatile__("pause");
    }
    // Shorthand = 3 (All Excluding Self), Delivery Mode = Fixed (0), Vector =
    // 0xFD
    lapic_write(LAPIC_REG_ICR_HIGH, 0);
    lapic_write(LAPIC_REG_ICR_LOW, (3 << 18) | IPI_RESCHEDULE_VECTOR);

    spinlock_release_irqrestore(&icr_lock, flags);
}

/**
 * Sends stop IPI to all other cores to halt execution
 */
void lapic_send_broadcast_stop_ipi() {
    if (!lapic_base)
        return;

    uint64_t flags = spinlock_acquire_irqsave(&icr_lock);

    while (lapic_read(LAPIC_REG_ICR_LOW) & (1 << 12)) {
        __asm__ __volatile__("pause");
    }

    // Shorthand = 3 (All Excluding Self), Delivery Mode = Fixed (0), Vector = 0xFC
    lapic_write(LAPIC_REG_ICR_HIGH, 0);
    lapic_write(LAPIC_REG_ICR_LOW, (3 << 18) | IPI_STOP_VECTOR);

    spinlock_release_irqrestore(&icr_lock, flags);
}

/**
 * Calibrates the LAPIC timer frequency using the HPET as reference,
 * then starts the timer in periodic mode.
 * @note Must be called on the BSP first.
 * @param target_hz Desired interrupt frequency (e.g. 1000 for 1 kHz / 1ms ticks)
 */
void lapic_timer_calibrate_and_start(uint32_t target_hz) {
    lapic_timer_target_hz = target_hz;

    // 1. Set divider to 16
    lapic_write(LAPIC_REG_TIMER_DIV, LAPIC_TIMER_DIV_16);

    // 2. Set initial count to max for measurement
    lapic_write(LAPIC_REG_TIMER_LVT, LAPIC_TIMER_MASKED); // Mask during calibration
    lapic_write(LAPIC_REG_TIMER_INITCNT, 0xFFFFFFFF);

    // 3. Wait exactly 10ms using HPET
    hpet_mdelay(10);

    // 4. Read how many ticks elapsed in 10ms
    uint32_t elapsed = 0xFFFFFFFF - lapic_read(LAPIC_REG_TIMER_CURRCNT);

    // 5. Calculate ticks per second, then ticks per interval
    uint64_t ticks_per_second = (uint64_t)elapsed * 100; // 10ms * 100 = 1s
    uint32_t init_count = (uint32_t)(ticks_per_second / target_hz);

    lapic_timer_calibrated_initcnt = init_count;

    serial_printf(COM1, "LAPIC TIMER: calibrated on CPU %d: %llu ticks/s, init_count=%u for %u Hz\n", lapic_get_id(), ticks_per_second, init_count, target_hz);

    // 6. Start periodic timer with calibrated value
    lapic_write(LAPIC_REG_TIMER_DIV, LAPIC_TIMER_DIV_16);
    lapic_write(LAPIC_REG_TIMER_LVT, LAPIC_TIMER_PERIODIC | LAPIC_TIMER_VECTOR);
    lapic_write(LAPIC_REG_TIMER_INITCNT, init_count);
}

/**
 * Starts the LAPIC timer on an AP using pre-calibrated values from the BSP.
 * Assumes all cores run at the same frequency (true for modern CPUs).
 */
void lapic_timer_start_ap(void) {
    if (lapic_timer_calibrated_initcnt == 0) {
        serial_printf(COM1, "LAPIC TIMER: error - not calibrated yet!\n");
        return;
    }

    lapic_write(LAPIC_REG_TIMER_DIV, LAPIC_TIMER_DIV_16);
    lapic_write(LAPIC_REG_TIMER_LVT, LAPIC_TIMER_PERIODIC | LAPIC_TIMER_VECTOR);
    lapic_write(LAPIC_REG_TIMER_INITCNT, lapic_timer_calibrated_initcnt);

    serial_printf(COM1, "LAPIC TIMER: started on CPU %d (init_count=%u, %u Hz)\n",
                    lapic_get_id(), lapic_timer_calibrated_initcnt, lapic_timer_target_hz);
}