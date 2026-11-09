/**
 * @file i8042.c
 * @brief PC/AT-compatible 8042 controller and PS/2 device transport.
 * @author friedrichOsDev
 */
#include <arch/arch.h>
#include "../../../arch/x86_64/apic/apic.h"
#include <drivers/hid/ps2/ps2.h>

#define I8042_WAIT_LIMIT 1000000U
#define I8042_CMD_READ_CONFIG 0x20
#define I8042_CMD_WRITE_CONFIG 0x60
#define I8042_CMD_DISABLE_A 0xAD
#define I8042_CMD_ENABLE_A 0xAE
#define I8042_CMD_DISABLE_B 0xA7
#define I8042_CMD_ENABLE_B 0xA8
#define I8042_CMD_TEST_CONTROLLER 0xAA
#define I8042_CMD_TEST_A 0xAB
#define I8042_CMD_TEST_B 0xA9
#define I8042_CMD_WRITE_B 0xD4

#define I8042_CFG_IRQ_A 0x01
#define I8042_CFG_IRQ_B 0x02
#define I8042_CFG_CLOCK_A 0x10
#define I8042_CFG_CLOCK_B 0x20
#define I8042_CFG_TRANSLATE 0x40

static bool controller_ready;
static bool keyboard_ready;
static bool mouse_ready;
static uint8_t controller_config;

/**
 * @brief Handles wait input empty.
 * @return Result of the operation.
 */
static bool wait_input_empty() {
    for (uint32_t i = 0; i < I8042_WAIT_LIMIT; i++) {
        if (!(inb(PS2_STATUS_PORT) & PS2_STATUS_INPUT_FULL)) return true;
        arch_pause();
    }
    return false;
}

/**
 * @brief Handles wait output full.
 * @return Result of the operation.
 */
static bool wait_output_full() {
    for (uint32_t i = 0; i < I8042_WAIT_LIMIT; i++) {
        if (inb(PS2_STATUS_PORT) & PS2_STATUS_OUTPUT_FULL) return true;
        arch_pause();
    }
    return false;
}

/**
 * @brief Handles controller command.
 * @param[in] command Function input.
 * @return Result of the operation.
 */
static kerr_t controller_command(uint8_t command) {
    if (!wait_input_empty()) return E_TIMEOUT;
    outb(PS2_COMMAND_PORT, command);
    return E_OK;
}

/**
 * @brief Handles controller write.
 * @param[in] value Function input.
 * @return Result of the operation.
 */
static kerr_t controller_write(uint8_t value) {
    if (!wait_input_empty()) return E_TIMEOUT;
    outb(PS2_DATA_PORT, value);
    return E_OK;
}

/**
 * @brief Handles controller read.
 * @param[in] value Function input.
 * @return Result of the operation.
 */
static kerr_t controller_read(uint8_t *value) {
    if (!value) return E_INVAL;
    if (!wait_output_full()) return E_TIMEOUT;
    *value = inb(PS2_DATA_PORT);
    return E_OK;
}

/**
 * @brief Handles drain output.
 */
static void drain_output() {
    for (uint32_t i = 0; i < 32; i++) {
        if (!(inb(PS2_STATUS_PORT) & PS2_STATUS_OUTPUT_FULL)) break;
        ()inb(PS2_DATA_PORT);
    }
}

/**
 * @brief Handles read config.
 * @param[in] config Function input.
 * @return Result of the operation.
 */
static kerr_t read_config(uint8_t *config) {
    K_TRY(controller_command(I8042_CMD_READ_CONFIG));
/**
 * @brief Handles controller read.
 * @param[in] config Function input.
 * @return Result of the operation.
 */
    return controller_read(config);
}

static kerr_t write_config(uint8_t config) {
    K_TRY(controller_command(I8042_CMD_WRITE_CONFIG));
/**
 * @brief Handles controller write.
 * @param[in] auxiliary Function input.
 * @param[in] value Function input.
 * @return Result of the operation.
 */
    return controller_write(config);
}

static kerr_t device_write(bool auxiliary, uint8_t value) {
    if (auxiliary) K_TRY(controller_command(I8042_CMD_WRITE_B));
/**
 * @brief Handles controller write.
 * @param[in] auxiliary Function input.
 * @param[in] value Function input.
 * @return Result of the operation.
 */
    return controller_write(value);
}

/* Device replies are read only after the corresponding command. */
static kerr_t device_read(bool auxiliary, uint8_t *value) {
    if (!value) return E_INVAL;
    for (uint32_t i = 0; i < I8042_WAIT_LIMIT; i++) {
        uint8_t status = inb(PS2_STATUS_PORT);
        if (status & PS2_STATUS_OUTPUT_FULL) {
            uint8_t data = inb(PS2_DATA_PORT);
            if ((status & (PS2_STATUS_TIMEOUT | PS2_STATUS_PARITY)) != 0) continue;
            if (((status & PS2_STATUS_AUXILIARY) != 0) == auxiliary) {
                *value = data;
                return E_OK;
            }
        }
        arch_pause();
    }
    return E_TIMEOUT;
}

/**
 * @brief Handles device command.
 * @param[in] auxiliary Function input.
 * @param[in] command Function input.
 * @return Result of the operation.
 */
static kerr_t device_command(bool auxiliary, uint8_t command) {
    for (uint32_t attempt = 0; attempt < 3; attempt++) {
        uint8_t response = 0;
        K_TRY(device_write(auxiliary, command));
        K_TRY(device_read(auxiliary, &response));
        if (response == 0xFA) return E_OK;
        if (response != 0xFE) return E_CHECKSUM;
    }
    return E_TIMEOUT;
}

/**
 * @brief Handles keyboard irq.
 * @param[in] regs Function input.
 */
static void keyboard_irq(struct registers *regs) { ps2_irq_handler(regs); }
/**
 * @brief Handles mouse irq.
 * @param[in] regs Function input.
 */
static void mouse_irq(struct registers *regs) { ps2_irq_handler(regs); }

/**
 * @brief Handles ps2 irq handler.
 * @param[in] regs Function input.
 */
void ps2_irq_handler(struct registers *regs) {
    ()regs;
    /* IRQ1 and IRQ12 share the data port: route by the controller AUX status bit. */
    for (uint32_t i = 0; i < 16; i++) {
        uint8_t status = inb(PS2_STATUS_PORT);
        if (!(status & PS2_STATUS_OUTPUT_FULL)) break;
        uint8_t byte = inb(PS2_DATA_PORT);
        if (status & (PS2_STATUS_TIMEOUT | PS2_STATUS_PARITY)) continue;
        if (status & PS2_STATUS_AUXILIARY) {
            if (mouse_ready) ps2_mouse_byte(byte);
        } else if (keyboard_ready) {
            ps2_keyboard_byte(byte);
        }
    }
}

/**
 * @brief Handles ps2 controller init.
 * @return Result of the operation.
 */
kerr_t ps2_controller_init() {
    uint8_t response = 0;

    /* No i8042 FADT flag is treated as advisory: QEMU and legacy firmware can
       expose a working controller even when firmware metadata is incomplete. */
    ()controller_command(I8042_CMD_DISABLE_A);
    ()controller_command(I8042_CMD_DISABLE_B);
    drain_output();

    if (k_failed(controller_command(I8042_CMD_TEST_CONTROLLER)) ||
        k_failed(controller_read(&response)) || response != 0x55) {
        log_warn("i8042 controller self-test failed or controller absent");
        return E_NOPROBE;
    }

    /* Controller self-test may reset the command byte. Read it only now. */
    K_TRY(read_config(&controller_config));
    controller_config &= (uint8_t)~(I8042_CFG_IRQ_A | I8042_CFG_IRQ_B | I8042_CFG_TRANSLATE);
    controller_config |= I8042_CFG_CLOCK_A | I8042_CFG_CLOCK_B;
    K_TRY(write_config(controller_config));

    bool second_channel = false;
    if (!k_failed(controller_command(I8042_CMD_ENABLE_B)) &&
        !k_failed(controller_command(I8042_CMD_READ_CONFIG)) &&
        !k_failed(controller_read(&response))) {
        second_channel = (response & I8042_CFG_CLOCK_B) == 0;
    }
    ()controller_command(I8042_CMD_DISABLE_B);

    bool first_ok = !k_failed(controller_command(I8042_CMD_TEST_A)) &&
                    !k_failed(controller_read(&response)) && response == 0;
    bool second_ok = second_channel &&
                     !k_failed(controller_command(I8042_CMD_TEST_B)) &&
                     !k_failed(controller_read(&response)) && response == 0;

    if (!first_ok && !second_ok) return E_NOPROBE;

    if (first_ok) {
        ()controller_command(I8042_CMD_ENABLE_A);
        if (!k_failed(device_command(false, 0xFF))) {
            uint8_t reset = 0;
            if (!k_failed(device_read(false, &reset)) && reset == 0xAA) {
                /* Discard optional keyboard identification bytes after BAT. */
                drain_output();
                /* Select scan-code set 2; disable controller translation. */
                if (!k_failed(device_command(false, 0xF0)) &&
                    !k_failed(device_command(false, 0x02)) &&
                    !k_failed(device_command(false, 0xF4))) keyboard_ready = true;
            }
        }
    }

    if (second_ok) {
        ()controller_command(I8042_CMD_ENABLE_B);
        if (!k_failed(device_command(true, 0xFF))) {
            uint8_t reset = 0;
            if (!k_failed(device_read(true, &reset)) && reset == 0xAA) {
                /* Discard optional device ID bytes after BAT. */
                drain_output();
                if (!k_failed(device_command(true, 0xF6))) {
                /* IntelliMouse wheel negotiation: sample rates 200, 100, 80. */
                bool wheel_mode = !k_failed(device_command(true, 0xF3)) &&
                                  !k_failed(device_write(true, 200)) &&
                                  !k_failed(device_read(true, &response)) && response == 0xFA &&
                                  !k_failed(device_command(true, 0xF3)) &&
                                  !k_failed(device_write(true, 100)) &&
                                  !k_failed(device_read(true, &response)) && response == 0xFA &&
                                  !k_failed(device_command(true, 0xF3)) &&
                                  !k_failed(device_write(true, 80)) &&
                                  !k_failed(device_read(true, &response)) && response == 0xFA;
                if (!k_failed(device_command(true, 0xF4))) {
                mouse_ready = true;
                /* Query device ID after negotiation; ID 3/4 uses four-byte packets. */
                if (wheel_mode && !k_failed(device_command(true, 0xF2))) {
                    uint8_t id = 0;
                    if (!k_failed(device_read(true, &id)) && (id == 0x03 || id == 0x04)) {
                        ps2_mouse_set_id(id);
                    }
                }
                }
                }
            }
        }
    }

    if (!keyboard_ready && !mouse_ready) return E_NOPROBE;

    controller_config &= (uint8_t)~(I8042_CFG_IRQ_A | I8042_CFG_IRQ_B | I8042_CFG_TRANSLATE);
    if (keyboard_ready) controller_config |= I8042_CFG_IRQ_A;
    if (mouse_ready) controller_config |= I8042_CFG_IRQ_B;
    if (keyboard_ready) controller_config &= (uint8_t)~I8042_CFG_CLOCK_A;
    if (mouse_ready) controller_config &= (uint8_t)~I8042_CFG_CLOCK_B;
    K_TRY(write_config(controller_config));

    irq_install_handler(1, keyboard_irq);
    irq_install_handler(12, mouse_irq);
    uint32_t destination = lapic_get_id();
    if (keyboard_ready) ioapic_route_irq(1, 33, (uint8_t)destination);
    if (mouse_ready) ioapic_route_irq(12, 44, (uint8_t)destination);

    controller_ready = true;
    log_info("i8042 initialized: keyboard=%s mouse=%s",
             keyboard_ready ? "yes" : "no", mouse_ready ? "yes" : "no");
    return E_OK;
}

/**
 * @brief Handles ps2 controller deinit.
 */
void ps2_controller_deinit() {
    if (!controller_ready) return;
    ()controller_command(I8042_CMD_DISABLE_A);
    ()controller_command(I8042_CMD_DISABLE_B);
    controller_config &= (uint8_t)~(I8042_CFG_IRQ_A | I8042_CFG_IRQ_B);
    ()write_config(controller_config);
    keyboard_ready = false;
    mouse_ready = false;
    controller_ready = false;
}
