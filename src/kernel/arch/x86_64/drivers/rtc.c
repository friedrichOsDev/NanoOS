/**
 * @file rtc.c
 * @brief RTC (realtime clock) driver.
 * @author friedrichOsDev
 */

#include "_rtc.h"
#include <arch/arch.h>
#include <drivers/rtc.h>
#include <kernel/sync.h>

static rtc_time_t boot_time;
static uint64_t boot_epoch = 0;
static spinlock_t cmos_lock = SPINLOCK_INIT;

/**
 * @brief Reads a byte from a CMOS/RTC register via indexed I/O ports.
 *
 * @param[in] reg CMOS register index to read from.
 * @return Value read from the specified CMOS register.
 */
static uint8_t cmos_read(uint8_t reg) {
    irq_flags_t flags = spinlock_acquire_irqsave(&cmos_lock);
    uint8_t nmi_bit = inb(CMOS_ADDR) & 0x80;
    outb(CMOS_ADDR, reg | nmi_bit);
    uint8_t val = inb(CMOS_DATA);
    spinlock_release_irqrestore(&cmos_lock, flags);
    return val;
}

/**
 * @brief Checks if the RTC update-in-progress (UIP) flag is currently set.
 *
 * @return non-zero if an update is in progress, 0 otherwise.
 */
static int rtc_is_updating() { return cmos_read(RTC_REG_STATUS_A) & 0x80; }

/**
 * @brief Converts a Binary-Coded Decimal (BCD) value to binary format.
 *
 * @param[in] bcd BCD encoded byte value.
 * @return Converted standard binary byte value.
 */
static inline uint8_t bcd_to_bin(uint8_t bcd) {
    return (uint8_t)(((bcd >> 4) * 10) + (bcd & 0x0F));
}

/**
 * @brief Reads the current date and time directly from the hardware RTC registers safely.
 *
 * @return Structure containing the current decoded hardware time.
 */
static rtc_time_t rtc_read_hardware() {
    rtc_time_t t1, t2;

    do {
        while (rtc_is_updating())
            ;

        t1.second = cmos_read(RTC_REG_SECONDS);
        t1.minute = cmos_read(RTC_REG_MINUTES);
        t1.hour = cmos_read(RTC_REG_HOURS);
        t1.day = cmos_read(RTC_REG_DAY);
        t1.month = cmos_read(RTC_REG_MONTH);
        t1.year = cmos_read(RTC_REG_YEAR);

        while (rtc_is_updating())
            ;

        t2.second = cmos_read(RTC_REG_SECONDS);
        t2.minute = cmos_read(RTC_REG_MINUTES);
        t2.hour = cmos_read(RTC_REG_HOURS);
        t2.day = cmos_read(RTC_REG_DAY);
        t2.month = cmos_read(RTC_REG_MONTH);
        t2.year = cmos_read(RTC_REG_YEAR);
    } while (t1.second != t2.second || t1.minute != t2.minute || t1.hour != t2.hour || t1.day != t2.day || t1.month != t2.month || t1.year != t2.year);

    uint8_t status_b = cmos_read(RTC_REG_STATUS_B);

    if (!(status_b & 0x04)) {
        t1.second = bcd_to_bin(t1.second);
        t1.minute = bcd_to_bin(t1.minute);
        t1.hour = bcd_to_bin(t1.hour & 0x7F);
        t1.day = bcd_to_bin(t1.day);
        t1.month = bcd_to_bin(t1.month);
        t1.year = bcd_to_bin((uint8_t)t1.year);
    }

    if (!(status_b & 0x02) && (t1.hour & 0x80)) {
        t1.hour = (uint8_t)(((t1.hour & 0x7F) + 12) % 24);
    }

    t1.year = (uint16_t)(t1.year + 2000);
    return t1;
}

/**
 * @brief Converts an RTC time structure to a standard UNIX epoch timestamp.
 *
 * @param[in] t Pointer to the rtc_time_t structure to convert.
 * @return Corresponding UNIX epoch timestamp in seconds.
 */
uint64_t rtc_to_unix(const rtc_time_t *t) {
    int64_t y = t->year;
    int64_t m = t->month;

    if (m <= 2) {
        y -= 1;
        m += 12;
    }

    int64_t era = (y >= 0 ? y : y - 399) / 400;
    int64_t yoe = y - era * 400;                                 // [0, 399]
    int64_t doy = (153 * (m - 3) + 2) / 5 + (int64_t)t->day - 1; // [0, 365]
    int64_t doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;         // [0, 146096]
    int64_t days = era * 146097 + doe - 719468;

    return (uint64_t)days * 86400ULL +
           (uint64_t)t->hour * 3600ULL +
           (uint64_t)t->minute * 60ULL +
           (uint64_t)t->second;
}

/** @brief Initializes the RTC subsystem, reads system boot time, and calculates the boot epoch. */
void rtc_init() {
    boot_time = rtc_read_hardware();
    boot_epoch = rtc_to_unix(&boot_time);

    log_debug("boot_time: %04d-%02d-%02d %02d:%02d:%02d UTC", boot_time.year, boot_time.month, boot_time.day, boot_time.hour, boot_time.minute, boot_time.second);
    log_var(boot_epoch);
}

/**
 * @brief Retrieves the recorded system boot time structure.
 *
 * @return Boot time structure.
 */
rtc_time_t rtc_get_boot_time() { return boot_time; }

/**
 * @brief Gets the current system UNIX time by adding HPET uptime to the boot epoch.
 *
 * @return Current UNIX timestamp in seconds.
 */
uint64_t time_get_unix() { return boot_epoch + (hpet_uptime_ms() / 1000); }

/**
 * @brief Computes and returns the current date and time based on the active UNIX timestamp.
 *
 * @return Current decoded date and time structure.
 */
rtc_time_t time_get_now() {
    uint64_t now_epoch = time_get_unix();
    rtc_time_t t;

    int64_t days = (int64_t)(now_epoch / 86400ULL);
    uint32_t rem_secs = (uint32_t)(now_epoch % 86400ULL);

    t.hour = (uint8_t)(rem_secs / 3600);
    rem_secs %= 3600;
    t.minute = (uint8_t)(rem_secs / 60);
    t.second = (uint8_t)(rem_secs % 60);

    days += 719468;
    int64_t era = (days >= 0 ? days : days - 146096) / 146097;
    uint32_t doe = (uint32_t)(days - era * 146097);                       // [0, 146096]
    uint32_t yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365; // [0, 399]
    int64_t y = (int64_t)yoe + era * 400;
    uint32_t doy = doe - (365 * yoe + yoe / 4 - yoe / 100); // [0, 365]
    uint32_t mp = (5 * doy + 2) / 153;                      // [0, 11]

    t.day = (uint8_t)(doy - (153 * mp + 2) / 5 + 1);
    t.month = (uint8_t)(mp < 10 ? mp + 3 : mp - 9);
    t.year = (uint16_t)(y + (t.month <= 2));

    return t;
}
