/**
 * @file print.c
 * @author friedrichOsDev
 */
 
#include <print.h>
#include <convert.h>
#include <memory.h>
#include <console.h>
#include <kernel.h>

/**
 * @brief Formatted print to the system console.
 * @param format The format string.
 * @return The number of characters printed, or -1 on error.
 */
int printf(const uint32_t* format, ...) {
    if (init_state < INIT_CONSOLE) {
        return -1;
    }
    int res = 0;

    va_list args;
    va_start(args, format);
    res = uvsnprintf(NULL, 0, format, args);
    va_end(args);
    
    if (res < 0) return -1;

    uint32_t* buffer = (uint32_t*)kmalloc((res + 1) * sizeof(uint32_t));
    if (!buffer) return -1;

    va_start(args, format);
    uvsnprintf(buffer, res + 1, format, args);
    va_end(args);

    console_puts(buffer);
    kfree((virt_addr_t)buffer);

    return res;
}

/**
 * @brief Formatted print to a fixed-size buffer.
 * @param dest The destination buffer.
 * @param size The size of the buffer.
 * @param format The format string.
 * @return The number of characters that would have been written.
 */
int usnprintf(uint32_t* dest, size_t size, const uint32_t* format, ...) {
    int res = 0;
    va_list args;
    va_start(args, format);
    res = uvsnprintf(dest, size, format, args);
    va_end(args);
    return res;
}

/**
 * @brief Formatted print to a fixed-size buffer using a va_list.
 * @param dest The destination buffer.
 * @param size The size of the buffer.
 * @param format The format string.
 * @param args The list of arguments.
 * @return The number of characters that would have been written.
 */
int uvsnprintf(uint32_t* dest, size_t size, const uint32_t* format, va_list args) {
    size_t i = 0;
    const uint32_t* p = format;

    while (*p != U'\0') {
        if (*p == U'%') {
            p++;

            bool pad_zero_flag = false;
            if (*p == U'0') {
                pad_zero_flag = true;
                p++;
            }

            bool r_width = false;
            if (*p == U'-') {
                r_width = true;
                p++;
            }
                        
            int width = 0;
            while (*p >= U'0' && *p <= U'9') {
                width = width * 10 + (*p - U'0');
                p++;
            }

            int length_mod = 0; // 0=default, 1=l, 3=z, 4=h, 5=hh
            if (*p == U'l') {
                length_mod = 1;
                p++;
            } else if (*p == U'z') {
                length_mod = 3;
                p++;
            } else if (*p == U'h') {
                length_mod = 4;
                p++;
                if (*p == U'h') {
                    length_mod = 5;
                    p++;
                }
            }

            switch (*p) {
                case U'd':
                case U'i': {
                    int32_t val_d;
                    if (length_mod == 1) val_d = va_arg(args, int32_t);
                    else if (length_mod == 4) val_d = (int16_t)va_arg(args, int);
                    else if (length_mod == 5) val_d = (int8_t)va_arg(args, int);
                    else val_d = va_arg(args, int);
                    
                    uint32_t buf_d[32];

                    uint32_t absolute_value;
                    if (val_d < 0) {
                        if (dest && i < size - 1) dest[i] = U'-';
                        i++;
                        absolute_value = (uint32_t)-(val_d + 1) + 1;
                    } else {
                        absolute_value = (uint32_t)val_d;
                    }

                    int len_d = uint_to_str(absolute_value, buf_d, 10);
                    int d_pad_len = (width > len_d) ? (width - len_d) : 0;

                    // padding left with spaces
                    if (!r_width && !pad_zero_flag) {
                        while (d_pad_len-- > 0) {
                            if (dest && i < size - 1) dest[i] = U' ';
                            i++;
                        }
                    }

                    // prefix
                    int start_index = 0;
                    if (buf_d[0] == U'-') {
                        if (dest && i < size - 1) dest[i] = U'-';
                        start_index = 1;
                        i++;
                    }

                    // padding left with zeros
                    if (!r_width && pad_zero_flag) {
                        while (d_pad_len-- > 0) {
                            if (dest && i < size - 1) dest[i] = U'0';
                            i++;
                        }
                    }

                    // actual number
                    for (int j = start_index; j < len_d; j++) {
                        if (dest && i < size - 1) dest[i] = buf_d[j];
                        i++;
                    }

                    // padding right (always with spaces)
                    if (r_width) {
                        while (d_pad_len-- > 0) {
                            if (dest && i < size - 1) dest[i] = U' ';
                            i++;
                        }
                    }
                    
                    break;
                }
                case U'u': {
                    uint32_t val_u;
                    if (length_mod == 1) val_u = va_arg(args, uint32_t);
                    else if (length_mod == 3) val_u = va_arg(args, size_t);
                    else if (length_mod == 4) val_u = (uint16_t)va_arg(args, unsigned int);
                    else if (length_mod == 5) val_u = (uint8_t)va_arg(args, unsigned int);
                    else val_u = va_arg(args, unsigned int);

                    uint32_t buf_u[32];
                    int len_u = uint_to_str(val_u, buf_u, 10);
                    int u_pad_len = (width > len_u) ? (width - len_u) : 0;

                    // padding left with spaces
                    if (!r_width && !pad_zero_flag) {
                        while (u_pad_len-- > 0) {
                            if (dest && i < size - 1) dest[i] = U' ';
                            i++;
                        }
                    }

                    // padding left with zeros
                    if (!r_width && pad_zero_flag) {
                        while (u_pad_len-- > 0) {
                            if (dest && i < size - 1) dest[i] = U'0';
                            i++;
                        }
                    }

                    // actual number
                    for (int j = 0; j < len_u; j++) {
                        if (dest && i < size - 1) dest[i] = buf_u[j];
                        i++;
                    }

                    // padding right (always with spaces)
                    if (r_width) {
                        while (u_pad_len-- > 0) {
                            if (dest && i < size - 1) dest[i] = U' ';
                            i++;
                        }
                    }

                    break;
                }
                case U'x':
                case U'X': {
                    uint32_t val_x;
                    if (length_mod == 1) val_x = va_arg(args, uint32_t);
                    else if (length_mod == 4) val_x = (uint16_t)va_arg(args, unsigned int);
                    else if (length_mod == 5) val_x = (uint8_t)va_arg(args, unsigned int);
                    else val_x = va_arg(args, unsigned int);

                    uint32_t buf_x[32];
                    int len_x = uint_to_str(val_x, buf_x, 16);
                    int x_pad_len = (width > len_x + 2) ? (width - (len_x + 2)) : 0;

                    // padding left with spaces
                    if (!r_width && !pad_zero_flag) {
                        while (x_pad_len-- > 0) {
                            if (dest && i < size - 1) dest[i] = U' ';
                            i++;
                        }
                    }

                    // prefix
                    if (dest && i < size - 1) dest[i] = U'0';
                    i++;
                    if (dest && i < size - 1) dest[i] = U'x';
                    i++;

                    // padding left with zeros
                    if (!r_width && pad_zero_flag) {
                        while (x_pad_len-- > 0) {
                            if (dest && i < size - 1) dest[i] = U'0';
                            i++;
                        }
                    }

                    // actual number
                    for (int j = 0; j < len_x; j++) {
                        if (dest && i < size - 1) dest[i] = buf_x[j];
                        i++;
                    }

                    // padding right (always with spaces)
                    if (r_width) {
                        while (x_pad_len-- > 0) {
                            if (dest && i < size - 1) dest[i] = U' ';
                            i++;
                        }
                    }
                    
                    break;
                }
                case U'o': {
                    uint32_t val_o;
                    if (length_mod == 1) val_o = va_arg(args, uint32_t);
                    else if (length_mod == 4) val_o = (uint16_t)va_arg(args, unsigned int);
                    else if (length_mod == 5) val_o = (uint8_t)va_arg(args, unsigned int);
                    else val_o = va_arg(args, unsigned int);
                    
                    uint32_t buf_o[32];
                    int len_o = uint_to_str(val_o, buf_o, 8);
                    int o_pad_len = (width > len_o + 2) ? (width - (len_o + 2)) : 0;

                    // padding left with spaces
                    if (!r_width && !pad_zero_flag) {
                        while (o_pad_len-- > 0) {
                            if (dest && i < size - 1) dest[i] = U' ';
                            i++;
                        }
                    }

                    // prefix
                    if (dest && i < size - 1) dest[i] = U'0';
                    i++;
                    if (dest && i < size - 1) dest[i] = U'o';
                    i++;

                    // padding left with zeros
                    if (!r_width && pad_zero_flag) {
                        while (o_pad_len-- > 0) {
                            if (dest && i < size - 1) dest[i] = U'0';
                            i++;
                        }
                    }

                    // actual number
                    for (int j = 0; j < len_o; j++) {
                        if (dest && i < size - 1) dest[i] = buf_o[j];
                        i++;
                    }

                    // padding right (always with spaces)
                    if (r_width) {
                        while (o_pad_len-- > 0) {
                            if (dest && i < size - 1) dest[i] = U' ';
                            i++;
                        }
                    }
                    
                    break;
                }
                case U's': {
                    const uint32_t* val_s = va_arg(args, const uint32_t*);
                    if (!val_s) val_s = U"(null)";
                    int len_s = 0;
                    while (val_s[len_s] != U'\0') len_s++;
                    int s_pad_len = (width > len_s) ? (width - len_s) : 0;

                    // padding left with spaces
                    if (!r_width) {
                        while (s_pad_len-- > 0) {
                            if (dest && i < size - 1) dest[i] = U' ';
                            i++;
                        }
                    }

                    // actual string
                    for (int j = 0; j < len_s; j++) {
                        if (dest && i < size - 1) dest[i] = val_s[j];
                        i++;
                    }

                    // padding right (always with spaces)
                    if (r_width) {
                        while (s_pad_len-- > 0) {
                            if (dest && i < size - 1) dest[i] = U' ';
                            i++;
                        }
                    }
                    
                    break;
                }
                case U'c': {
                    uint32_t val_c = (uint32_t)va_arg(args, int);
                    if (dest && i < size - 1) dest[i] = val_c;
                    i++;
                    break;
                }
                case U'p': {
                    uintptr_t val_p = va_arg(args, uintptr_t);
                    uint32_t buf_p[32];
                    int len_p = uint_to_str(val_p, buf_p, 16);

                    int p_width = (width > 0) ? width : 10;
                    int p_pad = (p_width > len_p + 2) ? (p_width - (len_p + 2)) : 0;

                    // padding left with spaces
                    if (!r_width && !pad_zero_flag) {
                        while (p_pad-- > 0) {
                            if (dest && i < size - 1) dest[i] = U' ';
                            i++;
                        }
                    }

                    // prefix
                    if (dest && i < size - 1) dest[i] = U'0';
                    i++;
                    if (dest && i < size - 1) dest[i] = U'x';
                    i++;

                    // padding left with zeros
                    if (!r_width && pad_zero_flag) {
                        while (p_pad-- > 0) {
                            if (dest && i < size - 1) dest[i] = U'0';
                            i++;
                        }
                    }

                    // actual address
                    for (int j = 0; j < len_p; j++) {
                        if (dest && i < size - 1) dest[i] = buf_p[j];
                        i++;
                    }

                    // padding right (always with spaces)
                    if (r_width) {
                        while (p_pad-- > 0) {
                            if (dest && i < size - 1) dest[i] = U' ';
                            i++;
                        }
                    }
                    
                    break;
                }
                case U'%': {
                    if (dest && i < size - 1) dest[i] = U'%';
                    i++;
                    break;
                }
                default: {
                    if (dest && i < size - 1) dest[i] = U'%';
                    i++;
                    if (dest && i < size - 1) dest[i] = (uint32_t)*p;
                    i++;
                    break;
                }
            }
        } else {
            if (dest && i < size - 1) dest[i] = (uint32_t)*p;
            i++;
        }
        p++;
    }
    
    if (dest && size > 0) {
        size_t last_char = (i < size) ? i : size - 1;
        dest[last_char] = U'\0';
    }

    return (int)i;
}

/**
 * @brief Formatted print to a fixed-size buffer.
 * @param dest The destination buffer.
 * @param size The size of the buffer.
 * @param format The format string.
 * @return The number of characters that would have been written.
 */
int snprintf(char* dest, size_t size, const char* format, ...) {
    int res = 0;
    va_list args;
    va_start(args, format);
    res = vsnprintf(dest, size, format, args);
    va_end(args);
    return res;
}

/**
 * @brief Formatted print to a fixed-size buffer using a va_list.
 * @param dest The destination buffer.
 * @param size The size of the buffer.
 * @param format The format string.
 * @param args The list of arguments.
 * @return The number of characters that would have been written.
 */
int vsnprintf(char* dest, size_t size, const char* format, va_list args) {
    size_t i = 0;
    const char* p = format;

    while (*p != '\0') {
        if (*p == '%') {
            p++;

            bool pad_zero_flag = false;
            if (*p == '0') {
                pad_zero_flag = true;
                p++;
            }

            bool r_width = false;
            if (*p == '-') {
                r_width = true;
                p++;
            }
                        
            int width = 0;
            while (*p >= '0' && *p <= '9') {
                width = width * 10 + (*p - '0');
                p++;
            }

            int length_mod = 0; // 0=default, 1=l, 3=z, 4=h, 5=hh
            if (*p == 'l') {
                length_mod = 1;
                p++;
            } else if (*p == 'z') {
                length_mod = 3;
                p++;
            } else if (*p == 'h') {
                length_mod = 4;
                p++;
                if (*p == 'h') {
                    length_mod = 5;
                    p++;
                }
            }

            switch (*p) {
                case 'd':
                case 'i': {
                    int32_t val_d;
                    if (length_mod == 1) val_d = va_arg(args, int32_t);
                    else if (length_mod == 4) val_d = (int16_t)va_arg(args, int);
                    else if (length_mod == 5) val_d = (int8_t)va_arg(args, int);
                    else val_d = va_arg(args, int);
                    
                    char buf_d[32];

                    uint32_t absolute_value;
                    if (val_d < 0) {
                        if (dest && i < size - 1) dest[i] = '-';
                        i++;
                        absolute_value = (uint32_t)-(val_d + 1) + 1;
                    } else {
                        absolute_value = (uint32_t)val_d;
                    }

                    int len_d = uint_to_str_legacy(absolute_value, buf_d, 10);
                    int d_pad_len = (width > len_d) ? (width - len_d) : 0;

                    // padding left with spaces
                    if (!r_width && !pad_zero_flag) {
                        while (d_pad_len-- > 0) {
                            if (dest && i < size - 1) dest[i] = ' ';
                            i++;
                        }
                    }

                    // prefix
                    int start_index = 0;
                    if (buf_d[0] == '-') {
                        if (dest && i < size - 1) dest[i] = '-';
                        start_index = 1;
                        i++;
                    }

                    // padding left with zeros
                    if (!r_width && pad_zero_flag) {
                        while (d_pad_len-- > 0) {
                            if (dest && i < size - 1) dest[i] = '0';
                            i++;
                        }
                    }

                    // actual number
                    for (int j = start_index; j < len_d; j++) {
                        if (dest && i < size - 1) dest[i] = buf_d[j];
                        i++;
                    }

                    // padding right (always with spaces)
                    if (r_width) {
                        while (d_pad_len-- > 0) {
                            if (dest && i < size - 1) dest[i] = ' ';
                            i++;
                        }
                    }
                    
                    break;
                }
                case 'u': {
                    uint32_t val_u;
                    if (length_mod == 1) val_u = va_arg(args, uint32_t);
                    else if (length_mod == 3) val_u = va_arg(args, size_t);
                    else if (length_mod == 4) val_u = (uint16_t)va_arg(args, unsigned int);
                    else if (length_mod == 5) val_u = (uint8_t)va_arg(args, unsigned int);
                    else val_u = va_arg(args, unsigned int);

                    char buf_u[32];
                    int len_u = uint_to_str_legacy(val_u, buf_u, 10);
                    int u_pad_len = (width > len_u) ? (width - len_u) : 0;

                    // padding left with spaces
                    if (!r_width && !pad_zero_flag) {
                        while (u_pad_len-- > 0) {
                            if (dest && i < size - 1) dest[i] = ' ';
                            i++;
                        }
                    }

                    // padding left with zeros
                    if (!r_width && pad_zero_flag) {
                        while (u_pad_len-- > 0) {
                            if (dest && i < size - 1) dest[i] = '0';
                            i++;
                        }
                    }

                    // actual number
                    for (int j = 0; j < len_u; j++) {
                        if (dest && i < size - 1) dest[i] = buf_u[j];
                        i++;
                    }

                    // padding right (always with spaces)
                    if (r_width) {
                        while (u_pad_len-- > 0) {
                            if (dest && i < size - 1) dest[i] = ' ';
                            i++;
                        }
                    }

                    break;
                }
                case 'x':
                case 'X': {
                    uint32_t val_x;
                    if (length_mod == 1) val_x = va_arg(args, uint32_t);
                    else if (length_mod == 4) val_x = (uint16_t)va_arg(args, unsigned int);
                    else if (length_mod == 5) val_x = (uint8_t)va_arg(args, unsigned int);
                    else val_x = va_arg(args, unsigned int);

                    char buf_x[32];
                    int len_x = uint_to_str_legacy(val_x, buf_x, 16);
                    int x_pad_len = (width > len_x + 2) ? (width - (len_x + 2)) : 0;

                    // padding left with spaces
                    if (!r_width && !pad_zero_flag) {
                        while (x_pad_len-- > 0) {
                            if (dest && i < size - 1) dest[i] = ' ';
                            i++;
                        }
                    }

                    // prefix
                    if (dest && i < size - 1) dest[i] = '0';
                    i++;
                    if (dest && i < size - 1) dest[i] = 'x';
                    i++;

                    // padding left with zeros
                    if (!r_width && pad_zero_flag) {
                        while (x_pad_len-- > 0) {
                            if (dest && i < size - 1) dest[i] = '0';
                            i++;
                        }
                    }

                    // actual number
                    for (int j = 0; j < len_x; j++) {
                        if (dest && i < size - 1) dest[i] = buf_x[j];
                        i++;
                    }

                    // padding right (always with spaces)
                    if (r_width) {
                        while (x_pad_len-- > 0) {
                            if (dest && i < size - 1) dest[i] = ' ';
                            i++;
                        }
                    }
                    
                    break;
                }
                case 'o': {
                    uint32_t val_o;
                    if (length_mod == 1) val_o = va_arg(args, uint32_t);
                    else if (length_mod == 4) val_o = (uint16_t)va_arg(args, unsigned int);
                    else if (length_mod == 5) val_o = (uint8_t)va_arg(args, unsigned int);
                    else val_o = va_arg(args, unsigned int);
                    
                    char buf_o[32];
                    int len_o = uint_to_str_legacy(val_o, buf_o, 8);
                    int o_pad_len = (width > len_o + 2) ? (width - (len_o + 2)) : 0;

                    // padding left with spaces
                    if (!r_width && !pad_zero_flag) {
                        while (o_pad_len-- > 0) {
                            if (dest && i < size - 1) dest[i] = ' ';
                            i++;
                        }
                    }

                    // prefix
                    if (dest && i < size - 1) dest[i] = '0';
                    i++;
                    if (dest && i < size - 1) dest[i] = 'o';
                    i++;

                    // padding left with zeros
                    if (!r_width && pad_zero_flag) {
                        while (o_pad_len-- > 0) {
                            if (dest && i < size - 1) dest[i] = '0';
                            i++;
                        }
                    }

                    // actual number
                    for (int j = 0; j < len_o; j++) {
                        if (dest && i < size - 1) dest[i] = buf_o[j];
                        i++;
                    }

                    // padding right (always with spaces)
                    if (r_width) {
                        while (o_pad_len-- > 0) {
                            if (dest && i < size - 1) dest[i] = ' ';
                            i++;
                        }
                    }
                    
                    break;
                }
                case 's': {
                    const char* val_s = va_arg(args, const char*);
                    if (!val_s) val_s = "(null)";
                    int len_s = 0;
                    while (val_s[len_s] != '\0') len_s++;
                    int s_pad_len = (width > len_s) ? (width - len_s) : 0;

                    // padding left with spaces
                    if (!r_width) {
                        while (s_pad_len-- > 0) {
                            if (dest && i < size - 1) dest[i] = ' ';
                            i++;
                        }
                    }

                    // actual string
                    for (int j = 0; j < len_s; j++) {
                        if (dest && i < size - 1) dest[i] = val_s[j];
                        i++;
                    }

                    // padding right (always with spaces)
                    if (r_width) {
                        while (s_pad_len-- > 0) {
                            if (dest && i < size - 1) dest[i] = ' ';
                            i++;
                        }
                    }
                    
                    break;
                }
                case 'c': {
                    char val_c = (char)va_arg(args, int);
                    if (dest && i < size - 1) dest[i] = val_c;
                    i++;
                    break;
                }
                case 'p': {
                    uintptr_t val_p = va_arg(args, uintptr_t);
                    char buf_p[32];
                    int len_p = uint_to_str_legacy(val_p, buf_p, 16);

                    int p_width = (width > 0) ? width : 10;
                    int p_pad = (p_width > len_p + 2) ? (p_width - (len_p + 2)) : 0;

                    // padding left with spaces
                    if (!r_width && !pad_zero_flag) {
                        while (p_pad-- > 0) {
                            if (dest && i < size - 1) dest[i] = ' ';
                            i++;
                        }
                    }

                    // prefix
                    if (dest && i < size - 1) dest[i] = '0';
                    i++;
                    if (dest && i < size - 1) dest[i] = 'x';
                    i++;

                    // padding left with zeros
                    if (!r_width && pad_zero_flag) {
                        while (p_pad-- > 0) {
                            if (dest && i < size - 1) dest[i] = '0';
                            i++;
                        }
                    }

                    // actual address
                    for (int j = 0; j < len_p; j++) {
                        if (dest && i < size - 1) dest[i] = buf_p[j];
                        i++;
                    }

                    // padding right (always with spaces)
                    if (r_width) {
                        while (p_pad-- > 0) {
                            if (dest && i < size - 1) dest[i] = ' ';
                            i++;
                        }
                    }
                    
                    break;
                }
                case '%': {
                    if (dest && i < size - 1) dest[i] = '%';
                    i++;
                    break;
                }
                default: {
                    if (dest && i < size - 1) dest[i] = '%';
                    i++;
                    if (dest && i < size - 1) dest[i] = *p;
                    i++;
                    break;
                }
            }
        } else {
            if (dest && i < size - 1) dest[i] = *p;
            i++;
        }
        p++;
    }
    
    if (dest && size > 0) {
        size_t last_char = (i < size) ? i : size - 1;
        dest[last_char] = '\0';
    }

    return (int)i;
}
