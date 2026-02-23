/*
 * @file rtc.h
 * @brief Header file for Real-Time Clock (RTC) driver
 * @author friedrichOsDev
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>

#define CMOS_ADDRESS 0x70
#define CMOS_DATA 0x71

#define RTC_SECONDS 0x00
#define RTC_MINUTES 0x02
#define RTC_HOURS 0x04
#define RTC_DAY 0x07
#define RTC_MONTH 0x08
#define RTC_YEAR 0x09

#define RTC_STATUS_A 0x0A
#define RTC_STATUS_B 0x0B

typedef struct {
    uint8_t seconds;
    uint8_t minutes;
    uint8_t hours;
    uint8_t day;
    uint8_t month;
    uint16_t year;
} rtc_time_t;

void rtc_init(void);
rtc_time_t rtc_get_time(void);
