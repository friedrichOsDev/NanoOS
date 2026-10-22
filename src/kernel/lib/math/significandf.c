#define _GNU_SOURCE
#include <pch.h>

float significandf(float x) { return scalbnf(x, -ilogbf(x)); }
