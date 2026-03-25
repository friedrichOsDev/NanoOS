/**
 * @file rtc.c
 * @author friedrichOsDev
 */

#include <rtc.h>
#include <io.h>
#include <serial.h>
#include <convert.h>
#include <timer.h>
#include <print.h>
#include <kernel.h>
#include <acpi.h>

static rtc_time_t rtc_time;
static uint32_t rtc_update_event_id;
static uint8_t rtc_century = 20;

/**
 * @brief Checks if the RTC is currently updating.
 * 
 * @return true if an update is in progress.
 */
static bool is_update_in_progress(void) {
    outb(CMOS_ADDRESS, RTC_STATUS_A);
    return (inb(CMOS_DATA) & 0x80);
}

/**
 * @brief Reads a value from a specific RTC register.
 * 
 * @param reg The register address to read.
 * @return The byte read from the register.
 */
static uint8_t read_rtc_register(uint8_t reg) {
    outb(CMOS_ADDRESS, reg);
    return inb(CMOS_DATA);
}

/**
 * @brief Updates the internal rtc_time structure with current CMOS values.
 * 
 * Handles BCD to decimal conversion and 12/24 hour format normalization.
 */
void rtc_update_time(void) {
    rtc_time_t last;

    do {
        last = rtc_time;
        while (is_update_in_progress());

        uint8_t status_b = read_rtc_register(RTC_STATUS_B);
        bool is_bcd = !(status_b & 0x04);
        bool is_hour_24 = (status_b & 0x02);

        if (!is_bcd) {
            rtc_time.seconds = read_rtc_register(RTC_SECONDS);
            rtc_time.minutes = read_rtc_register(RTC_MINUTES);
            rtc_time.day = read_rtc_register(RTC_DAY);
            rtc_time.month = read_rtc_register(RTC_MONTH);
            rtc_time.year = read_rtc_register(RTC_YEAR) + rtc_century * 100;

            uint8_t hour_reg = read_rtc_register(RTC_HOURS);
            if (!is_hour_24) {
                bool pm = (hour_reg & 0x80);
                rtc_time.hours = hour_reg & 0x7F;
                if (pm && rtc_time.hours < 12) rtc_time.hours += 12;
                if (!pm && rtc_time.hours == 12) rtc_time.hours = 0;
            } else {
                rtc_time.hours = hour_reg;
            }
        } else {
            rtc_time.seconds = bcd_to_dezimal(read_rtc_register(RTC_SECONDS));
            rtc_time.minutes = bcd_to_dezimal(read_rtc_register(RTC_MINUTES));
            rtc_time.day = bcd_to_dezimal(read_rtc_register(RTC_DAY));
            rtc_time.month = bcd_to_dezimal(read_rtc_register(RTC_MONTH));
            rtc_time.year = bcd_to_dezimal(read_rtc_register(RTC_YEAR)) + rtc_century * 100;

            uint8_t hour_reg = read_rtc_register(RTC_HOURS);
            if (!is_hour_24) {
                bool pm = (hour_reg & 0x80);
                rtc_time.hours = bcd_to_dezimal(hour_reg & 0x7F);
                if (pm && rtc_time.hours < 12) rtc_time.hours += 12;
                if (!pm && rtc_time.hours == 12) rtc_time.hours = 0;
            } else {
                rtc_time.hours = bcd_to_dezimal(hour_reg);
            }
        }

    } while (last.seconds != rtc_time.seconds || last.minutes != rtc_time.minutes);
}

/**
 * @brief Initializes the Real Time Clock.
 * 
 * Sets up a recurring timer event to update the time every second.
 */
void rtc_init(void) {
    uint8_t century_reg = 0;

    if (fadt != NULL && fadt->century != 0) {
        century_reg = fadt->century;
    } else {
        serial_printf("RTC: FADT century register not available, trying default 0x32\n");
        century_reg = 0x32; // De-facto Standard für x86
    }

    uint8_t century_val = read_rtc_register(century_reg);
    if (century_val != 0) {
        rtc_century = bcd_to_dezimal(century_val);
    } else {
        serial_printf("RTC: FADT century register is 0, using default 20\n");
    }

    rtc_time.seconds = 0;
    rtc_time.minutes = 0;
    rtc_time.hours = 0;
    rtc_time.day = 1;
    rtc_time.month = 1;
    rtc_time.year = rtc_century * 100; // default to 20xx

    event_t rtc_update_event = {
        .event_id = 0,
        .handler = rtc_update_time,
        .interval = TIMER_FREQUENCY, // every second
        .target_tick = timer_get_ticks() + TIMER_FREQUENCY,
        .repeat = true,
        .active = true
    };

    rtc_update_event_id = timer_add_event(rtc_update_event);
    (void)rtc_update_event_id;
    rtc_update_time();

    init_state = INIT_RTC;
}

/**
 * @brief Gets the current RTC time.
 * 
 * @return A structure containing the current date and time.
 */
rtc_time_t rtc_get_time(void) {
    return rtc_time;
}

/**
 * @brief Returns a formatted string of the current time.
 * 
 * @param type The format type (1 for European, default for ISO-like).
 * @return A pointer to a static buffer containing the formatted string.
 */
const char* rtc_get_time_format(uint32_t type) {
    static char buffer[20];
    int length = 0;
    switch (type) {
        case 1: {
            // Format: dd.mm.yyyy hh:mm:ss
            length = snprintf(buffer, sizeof(buffer), "%02d.%02d.%04d %02d:%02d:%02d", rtc_time.day, rtc_time.month, rtc_time.year, rtc_time.hours, rtc_time.minutes, rtc_time.seconds);
            break;
        }
        default:
            // Format: yyyy-mm-dd hh:mm:ss
            length = snprintf(buffer, sizeof(buffer), "%04d.%02d.%02d %02d:%02d:%02d", rtc_time.year, rtc_time.month, rtc_time.day, rtc_time.hours, rtc_time.minutes, rtc_time.seconds);
            break;
    }
    (void)length;
    return buffer;
}
