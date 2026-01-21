/*
 * @file rtc.c
 * @brief Real-Time Clock (RTC) driver implementation
 * @author friedrichOsDev
 */

#include <rtc.h>
#include <io.h>
#include <cpu.h>
#include <serial.h>

/*
 * Global RTC time variable
 */
static rtc_time_t rtc_time;

/*
 * Global century variable
 * @note If 0 the century is assumed to be 2000
 */
static uint8_t rtc_century = 0;

/*
 * A function to initialize the RTC driver
 * @param void
 */

void rtc_init(void) {
    rtc_time.hours = 0;
    rtc_time.minutes = 0;
    rtc_time.seconds = 0;
    rtc_time.day = 0;
    rtc_time.month = 0;
    rtc_time.year = 0;
}

/*
 * A helper function to check if an update is in progress
 * @param void
 * @return true if an update is in progress, false otherwise
 */
static bool is_update_in_progress(void) {
    outb(CMOS_ADDRESS, RTC_STATUS_A);
    return (inb(CMOS_DATA) & 0x80);
}

/*
 * A helper function to read a register from the RTC
 * @param reg: The register to read
 * @return uint8_t: The value of the register
 */
static uint8_t read_rtc_register(int reg) {
    outb(CMOS_ADDRESS, reg);
    return inb(CMOS_DATA);
}

/*
 * A helper function to convert BCD to decimal
 * @param bcd: The BCD value to convert
 * @return uint8_t: The decimal value
 */
static uint8_t bcd_to_decimal(uint8_t bcd) {
    return ((bcd / 16) * 10) + (bcd & 0x0F);
}

/*
 * A function to update the RTC time
 * @param void
 * @note If bit 1 (24/12) of Status Register B is 0, the RTC is in 12-hour mode
 * @note If bit 2 (DM) of Status Register B is 0, the RTC values are in BCD format
 * @note If bit 7 (PM) of the hours register is set, the time is PM in 12-hour mode
 */
void rtc_update_time(void) {
    while (is_update_in_progress());

    uint8_t status_b = read_rtc_register(RTC_STATUS_B);
    bool is_bcd = !(status_b & 0x04);

    uint8_t raw_seconds = read_rtc_register(RTC_SECONDS);
    uint8_t raw_minutes = read_rtc_register(RTC_MINUTES);
    uint8_t raw_hours = read_rtc_register(RTC_HOURS);
    uint8_t raw_day = read_rtc_register(RTC_DAY);
    uint8_t raw_month = read_rtc_register(RTC_MONTH);
    uint8_t raw_year = read_rtc_register(RTC_YEAR);
    uint8_t raw_century = read_rtc_register(RTC_CENTURY);

    int seconds = is_bcd ? bcd_to_decimal(raw_seconds) : raw_seconds;
    int minutes = is_bcd ? bcd_to_decimal(raw_minutes) : raw_minutes;
    int hours = is_bcd ? bcd_to_decimal(raw_hours & 0x7F) : (raw_hours & 0x7F);
    int day = is_bcd ? bcd_to_decimal(raw_day) : raw_day;
    int month = is_bcd ? bcd_to_decimal(raw_month) : raw_month;
    int year = is_bcd ? bcd_to_decimal(raw_year) : raw_year;
    rtc_century = is_bcd ? bcd_to_decimal(raw_century) : raw_century;
    
    if (!(status_b & 0x02)) { 
        if (raw_hours & 0x80) { 
            hours = (hours == 12) ? 12 : hours + 12; 
        } else {
            hours = (hours == 12) ? 0 : hours; 
        }
    }

    rtc_time.seconds = seconds;
    rtc_time.minutes = minutes;
    rtc_time.hours = hours;
    rtc_time.day = day;
    rtc_time.month = month;
    rtc_time.year = year + (rtc_century * 100);
    rtc_time.hours = (rtc_time.hours + 1) % 24;
}

/*
 * A function to get the current RTC time
 * @param void
 * @return rtc_time_t: The current RTC time
 */
rtc_time_t rtc_get_time(void) {
    rtc_time_t time;
    cpu_disable_interrupts();
    time = rtc_time;
    cpu_enable_interrupts();
    return time;
}