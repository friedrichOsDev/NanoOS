/**
 * @file _rtc.h
 * @brief Internal RTC driver header.
 * @author friedrichOsDev
 */

#pragma once

#define CMOS_ADDR 0x70
#define CMOS_DATA 0x71

#define RTC_REG_SECONDS 0x00
#define RTC_REG_MINUTES 0x02
#define RTC_REG_HOURS 0x04
#define RTC_REG_DAY 0x07
#define RTC_REG_MONTH 0x08
#define RTC_REG_YEAR 0x09
#define RTC_REG_CENTURY 0x32
#define RTC_REG_STATUS_A 0x0A
#define RTC_REG_STATUS_B 0x0B
