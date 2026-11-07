#include "libm.h"
#include <pch.h>

#undef signgam

int __signgam = 0;
weak_alias(__signgam, signgam);
