#include <lib/math/libm.h>
#include <lib/math/math.h>

#undef signgam

int __signgam = 0;
weak_alias(__signgam, signgam);