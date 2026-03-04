/**
 * @file i8042.c
 * @author friedrichOsDev
 */

#include <i8042.h>
#include <kernel.h>
#include <serial.h>
#include <io.h>

static bool dual_channel = false;

/**
 * @brief Waits for the output buffer to be full (data available to read).
 * @return true if data is available, false if timeout occurs.
 */
bool i8042_wait_read(void) {
    for (uint32_t i = 0; i < I8042_RETRIES; i++) {
        if (i8042_read_status() & I8042_STATUS_OUTPUT_BUFFER_FULL) return true;
        __asm__ __volatile__("pause");
    }
    return false;
}

/**
 * @brief Waits for the input buffer to be empty (ready to accept data/commands).
 * @return true if ready, false if timeout occurs.
 */
bool i8042_wait_write(void) {
    for (uint32_t i = 0; i < I8042_RETRIES; i++) {
        if (!(i8042_read_status() & I8042_STATUS_INPUT_BUFFER_FULL)) return true;
        __asm__ __volatile__("pause");
    }
    return false;
}

/**
 * @brief Reads the status register of the i8042 controller.
 * @return The status byte.
 */
uint8_t i8042_read_status(void) {
    return inb(I8042_STATUS_PORT);
}

/**
 * @brief Writes a command to the i8042 controller.
 * @param command The command byte to write.
 */
void i8042_write_command(uint8_t command) {
    if (i8042_wait_write()) {
        outb(I8042_COMMAND_PORT, command);
    }
}

/**
 * @brief Writes data to the i8042 data port.
 * @param data The data byte to write.
 */
void i8042_write_data(uint8_t data) {
    if (i8042_wait_write()) {
        outb(I8042_DATA_PORT, data);
    }
}

/**
 * @brief Reads data from the i8042 data port.
 * @return The data byte read, or 0 if timeout.
 */
uint8_t i8042_read_data(void) {
    if (i8042_wait_read()) {
        return inb(I8042_DATA_PORT);
    }
    return 0;
}

/**
 * @brief Initializes the i8042 PS/2 controller and the keyboard.
 */
void i8042_init(void) {
    serial_printf("i8042: Initializing controller...\n");
    i8042_write_command(I8042_DISABLE_KBD);
    i8042_write_command(I8042_DISABLE_AUX);

    while (i8042_read_status() & I8042_STATUS_OUTPUT_BUFFER_FULL) {
        i8042_read_data();
    }

    i8042_write_command(I8042_READ_CONFIG);
    uint8_t config = i8042_read_data();
    
    config |= I8042_CONFIG_PORT1_INTERRUPT | I8042_CONFIG_PORT1_TRANSLATION;
    config &= ~(I8042_CONFIG_PORT2_INTERRUPT | I8042_CONFIG_PORT2_CLOCK_DISABLE); 

    i8042_write_command(I8042_WRITE_CONFIG);
    i8042_write_data(config);

    i8042_write_command(I8042_SELF_TEST);
    if (i8042_read_data() != 0x55) {
        serial_printf("i8042: Controller self-test failed!\n");
        return;
    }

    i8042_write_command(I8042_ENABLE_AUX);
    i8042_write_command(I8042_READ_CONFIG);
    config = i8042_read_data();
    if (!(config & I8042_CONFIG_PORT2_CLOCK_DISABLE)) {
        dual_channel = true;
        serial_printf("i8042: Dual channel controller detected.\n");
        i8042_write_command(I8042_DISABLE_AUX);
    }

    i8042_write_command(I8042_TEST_KBD);
    if (i8042_read_data() != 0x00) {
        serial_printf("i8042: Keyboard interface test failed!\n");
        return;
    }

    i8042_write_command(I8042_ENABLE_KBD);
    i8042_write_data(I8042_KBD_RESET);
    
    uint8_t res1 = i8042_read_data();
    uint8_t res2 = i8042_read_data();

    if (res1 != I8042_KBD_ACK || res2 != 0xAA) {
        serial_printf("i8042: Keyboard reset failed (Res: %x, %x)\n", res1, res2);
    }

    init_state = INIT_I8042;
    serial_printf("i8042: Initialization complete.\n");
}