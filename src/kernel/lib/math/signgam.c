#include "libm.h"
#include <lib/math.h>

#undef signgam

int __signgam = 0;
weak_alias(__signgam, signgam);