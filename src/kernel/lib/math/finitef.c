#define _GNU_SOURCE
#include <pch.h>

int finitef(float x) { return isfinite(x); }
