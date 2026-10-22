#include <pch.h>

long double rintl(long double x) {
    __asm__("frndint" : "+t"(x));
    return x;
}
