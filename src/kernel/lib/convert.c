/**
 * @file convert.c
 * @brief Konvertierungsfunktionen
 * @author friedrichOsDev
 */

#include <lib/convert.h>
#include <stdbool.h>

/**
 * @brief Interne Hilfsfunktion zur Umwandlung einer Zahl in einen String.
 *
 * @param buffer Zielpuffer (entweder `char*` oder `uint32_t*`).
 * @param value Der zu konvertierende Wert.
 * @param base Basis des Zahlensystems.
 * @param is_wide `true` für 32-Bit-Zeichen (uint32_t*), `false` für ASCII (char*).
 * @return Länge des erzeugten Strings.
 */
static int uint_to_str_internal(void *buffer, uint64_t value, int base, bool is_wide) {
    char temp_buf[66];
    int idx = 0;

    if (value == 0) {
        if (is_wide) {
            ((uint32_t *)buffer)[0] = U'0';
            ((uint32_t *)buffer)[1] = U'\0';
        } else {
            ((char *)buffer)[0] = '0';
            ((char *)buffer)[1] = '\0';
        }
        return 1;
    }

    while (value != 0) {
        const uint64_t rem = value % base;
        value /= base;
        temp_buf[idx++] = (rem > 9) ? (char)((rem - 10) + 'A') : (char)(rem + '0');
    }

    const int len = idx;
    for (int i = 0; i < len; i++) {
        const char digit = temp_buf[len - 1 - i];
        if (is_wide) {
            ((uint32_t *)buffer)[i] = (uint32_t)digit;
        } else {
            ((char *)buffer)[i] = digit;
        }
    }

    if (is_wide) {
        ((uint32_t *)buffer)[len] = U'\0';
    } else {
        ((char *)buffer)[len] = '\0';
    }

    return len;
}

int uint_to_str(uint64_t value, uint32_t *buffer, int base) {
    return uint_to_str_internal(buffer, value, base, true);
}

int uint_to_wstr(uint64_t value, char *buffer, int base) {
    return uint_to_str_internal(buffer, value, base, false);
}

int double_to_str(double value, char *buf, int prec) {
    if (prec < 0)
        prec = 6;
    if (prec > 9)
        prec = 9;

    union {
        double d;
        uint64_t u;
    } pun = {.d = value};

    uint64_t exp_bits = (pun.u >> 52) & 0x7FF;
    uint64_t mant_bits = pun.u & 0x000FFFFFFFFFFFFFULL;

    if (exp_bits == 0x7FF) {
        if (mant_bits != 0) {
            buf[0] = 'n';
            buf[1] = 'a';
            buf[2] = 'n';
            buf[3] = '\0';
            return 3;
        }
        int idx = 0;
        if ((pun.u >> 63) != 0)
            buf[idx++] = '-';
        buf[idx++] = 'i';
        buf[idx++] = 'n';
        buf[idx++] = 'f';
        buf[idx] = '\0';
        return idx;
    }

    int idx = 0;
    if ((pun.u >> 63) != 0) {
        buf[idx++] = '-';
        value = -value;
    }

    uint64_t ipart = (uint64_t)value;
    double fpart = value - (double)ipart;

    idx += uint_to_wstr(ipart, buf + idx, 10);

    if (prec > 0) {
        buf[idx++] = '.';

        double mult = 1.0;
        for (int i = 0; i < prec; i++)
            mult *= 10.0;
        uint64_t fdigits = (uint64_t)(fpart * mult + 0.5);

        char fbuf[16];
        int flen = uint_to_wstr(fdigits, fbuf, 10);

        for (int i = 0; i < (prec - flen); i++) {
            buf[idx++] = '0';
        }
        for (int i = 0; i < flen; i++) {
            buf[idx++] = fbuf[i];
        }
    }

    buf[idx] = '\0';
    return idx;
}

int double_to_wstr(double value, uint32_t *buf, int prec) {
    char cbuf[64];
    int len = double_to_str(value, cbuf, prec);
    for (int i = 0; i <= len; i++) {
        buf[i] = (uint32_t)(unsigned char)cbuf[i];
    }
    return len;
}