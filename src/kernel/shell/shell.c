
#include "_shell.h"
#include <shell/shell.h>

bool shell_initialized = false;

kerr_t shell_init() {
    K_TRY(shell_input_init());
    shell_initialized = true;
    return E_OK;
}
