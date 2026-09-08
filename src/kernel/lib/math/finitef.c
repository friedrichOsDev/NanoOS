#define _GNU_SOURCE
#include <lib/math/math.h>

int finitef(float x) { return isfinite(x); }
