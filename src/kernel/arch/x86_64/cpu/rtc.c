/**
 * @file rtc.c
 * @brief Treiber für die Real-Time Clock (CMOS RTC) und Zeitberechnungen.
 * @author friedrichOsDev
 */

#include <arch/x86_64/cpu/hpet.h>
#include <arch/x86_64/cpu/rtc.h>
#include <arch/x86_64/drivers/serial.h>
#include <core/sync.h>
#include <lib/io.h>

/** @brief Gespeicherter Zeitpunkt des Systemstarts */
static rtc_time_t boot_time;
/** @brief Gespeicherter Unix-Timestamp des Systemstarts */
static uint64_t boot_epoch = 0;
/** @brief Spinlock zur Absicherung von CMOS-Lesezugriffen */
static spinlock_t cmos_lock = SPINLOCK_INIT;

/**
 * @brief Liest ein Byte aus einem angegebenen CMOS-Register unter Wahrung des Spinlocks.
 * @param reg Zu lesende Registeradresse.
 * @return Wert aus dem CMOS-Register.
 */
static uint8_t cmos_read(uint8_t reg) {
    uint64_t flags = spinlock_acquire_irqsave(&cmos_lock);
    uint8_t nmi_bit = inb(CMOS_ADDR) & 0x80;
    outb(CMOS_ADDR, reg | nmi_bit);
    uint8_t val = inb(CMOS_DATA);
    spinlock_release_irqrestore(&cmos_lock, flags);
    return val;
}

/**
 * @brief Prüft, ob die RTC derzeit ein internes Update durchführt (Bit 7 in Status A).
 * @return 1 wenn ein Update läuft, sonst 0.
 */
static int rtc_is_updating() { return cmos_read(RTC_REG_STATUS_A) & 0x80; }

/**
 * @brief Wandelt eine BCD-codierte 8-Bit-Zahl in ein binäres Format um.
 * @param bcd BCD-Wert.
 * @return Binärer Wert.
 */
static inline uint8_t bcd_to_bin(uint8_t bcd) {
    return ((bcd >> 4) * 10) + (bcd & 0x0F);
}

/**
 * @brief Liest die aktuelle Zeit direkt aus den Hardware-Registern der RTC.
 *
 * Um Inkonsistenzen während eines Zähler-Updates zu vermeiden, wird der Wert doppelt gelesen.
 * Zudem werden BCD-Dekodierung und 12-Stunden-Format-Konvertierung durchgeführt.
 *
 * @return Das eingelesene Datum und die Uhrzeit.
 */
static rtc_time_t rtc_read_hardware() {
    rtc_time_t t1, t2;

    /* Zweimaliges Lesen zur Absicherung gegen Werteübergänge */
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

    /* Konvertierung von BCD zu Binär, falls Bit 2 in Status B nicht gesetzt ist */
    if (!(status_b & 0x04)) {
        t1.second = bcd_to_bin(t1.second);
        t1.minute = bcd_to_bin(t1.minute);
        t1.hour = bcd_to_bin(t1.hour & 0x7F);
        t1.day = bcd_to_bin(t1.day);
        t1.month = bcd_to_bin(t1.month);
        t1.year = bcd_to_bin((uint8_t)t1.year);
    }

    /* Handhabung des 12-Stunden-Formats */
    if (!(status_b & 0x02) && (t1.hour & 0x80)) {
        t1.hour = ((t1.hour & 0x7F) + 12) % 24;
    }

    t1.year += 2000; // Ausgegangen von Jahren zwischen 2000 und 2099
    return t1;
}

uint64_t rtc_to_unix(const rtc_time_t *t) {
    int64_t y = t->year;
    int64_t m = t->month;

    if (m <= 2) {
        y -= 1;
        m += 12;
    }

    int64_t era = (y >= 0 ? y : y - 399) / 400;
    uint64_t yoe = (uint64_t)(y - era * 400);             // Intervall [0, 399]
    uint64_t doy = (153 * (m - 3) + 2) / 5 + t->day - 1;  // Intervall [0, 365]
    uint64_t doe = yoe * 365 + yoe / 4 - yoe / 100 + doy; // Intervall [0, 146096]
    int64_t days = era * 146097 + (int64_t)doe - 719468;

    return (uint64_t)days * 86400ULL + t->hour * 3600ULL + t->minute * 60ULL + t->second;
}

void rtc_init() {
    boot_time = rtc_read_hardware();
    boot_epoch = rtc_to_unix(&boot_time);

    serial_printf(COM1, "RTC: boot time: %04d-%02d-%02d %02d:%02d:%02d UTC\n", boot_time.year, boot_time.month, boot_time.day, boot_time.hour, boot_time.minute, boot_time.second);
    serial_printf(COM1, "RTC: boot epoch: %llu\n", boot_epoch);
}

rtc_time_t rtc_get_boot_time() { return boot_time; }

uint64_t time_get_unix() { return boot_epoch + (hpet_uptime_ms() / 1000); }

rtc_time_t time_get_now() {
    uint64_t now_epoch = time_get_unix();
    rtc_time_t t;

    int64_t days = now_epoch / 86400ULL;
    uint32_t rem_secs = now_epoch % 86400ULL;

    t.hour = rem_secs / 3600;
    rem_secs %= 3600;
    t.minute = rem_secs / 60;
    t.second = rem_secs % 60;

    /* Zivil-Kalender-Algorithmus zur Ermittlung des Datums aus den Vergangenen Tagen seit Epoche */
    days += 719468;
    int64_t era = (days >= 0 ? days : days - 146096) / 146097;
    uint32_t doe = (uint32_t)(days - era * 146097);                       // Intervall [0, 146096]
    uint32_t yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365; // Intervall [0, 399]
    int64_t y = (int64_t)yoe + era * 400;
    uint32_t doy = doe - (365 * yoe + yoe / 4 - yoe / 100); // Intervall [0, 365]
    uint32_t mp = (5 * doy + 2) / 153;                      // Intervall [0, 11]

    t.day = doy - (153 * mp + 2) / 5 + 1;
    t.month = mp < 10 ? mp + 3 : mp - 9;
    t.year = y + (t.month <= 2);

    return t;
}