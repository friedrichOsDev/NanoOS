/**
 * @file renderer.h
 * @brief Graphics renderer for the kernel shell screen model.
 * @author friedrichOsDev
 */

#pragma once

#include <shell/screen.h>

kerr_t shell_renderer_init();
kerr_t shell_renderer_render(const shell_screen_t *screen, size_t cursor_index);
kerr_t shell_renderer_deinit();
