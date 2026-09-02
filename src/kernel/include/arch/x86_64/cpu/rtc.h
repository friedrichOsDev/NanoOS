/**
 * @file rtc.h
 * @brief Real-Time Clock (CMOS RTC)
 * @author friedrichOsDev
 */

#pragma once

#include <stdint.h>

#define CMOS_ADDR 0x70
#define CMOS_DATA 0x71

#define RTC_REG_SECONDS 0x00
#define RTC_REG_MINUTES 0x02
#define RTC_REG_HOURS 0x04
#define RTC_REG_DAY 0x07
#define RTC_REG_MONTH 0x08
#define RTC_REG_YEAR 0x09
#define RTC_REG_CENTURY 0x32 // May not exist on all hardware
#define RTC_REG_STATUS_A 0x0A
#define RTC_REG_STATUS_B 0x0B

typedef struct {
    uint8_t second;
    uint8_t minute;
    uint8_t hour;
    uint8_t day;
    uint8_t month;
    uint16_t year;
} rtc_time_t;

void rtc_init(void);
rtc_time_t rtc_get_boot_time(void);
uint64_t rtc_to_unix(const rtc_time_t *t);
uint64_t time_get_unix(void);
rtc_time_t time_get_now(void);