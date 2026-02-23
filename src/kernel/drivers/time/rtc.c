/*
 * @file rtc.c
 * @brief Real-Time Clock (RTC) driver
 * @author friedrichOsDev
 */

#include <rtc.h>
#include <io.h>
#include <serial.h>
#include <convert.h>
#include <timer.h>
// #include <acpi.h>

static rtc_time_t rtc_time;
static uint32_t rtc_update_event_id;
static uint8_t rtc_century = 20; // later fadt_get_century() in rtc_init()

/*
 * Check if the RTC is currently updating
 * @return true if update is in progress, false otherwise
 */
static bool is_update_in_progress(void) {
    outb(CMOS_ADDRESS, RTC_STATUS_A);
    return (inb(CMOS_DATA) & 0x80);
}

/*
 * Read a value from the specified RTC register
 * @param reg The RTC register to read from
 * @return The value read from the RTC register
 */
static uint8_t read_rtc_register(uint8_t reg) {
    outb(CMOS_ADDRESS, reg);
    return inb(CMOS_DATA);
}


/*
 * Update the current time from the RTC registers
 * @param void
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


/*
 * Initialize the RTC driver
 * @param void
 */
void rtc_init(void) {
    // TODO: get century from FADT
    // rtc_century = fadt_get_century();
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
}

/*
 * Get the current time from the RTC
 * @param void
 * @return The current time as an rtc_time_t structure
 */
rtc_time_t rtc_get_time(void) {
    return rtc_time;
}
