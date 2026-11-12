/**
 * @file test_screen.c
 * @brief Static renderer test screen for the kernel shell.
 * @author friedrichOsDev
 */

#include <drivers/gfx/compositor.h>
#include <shell/renderer.h>
#include <shell/screen.h>
#include <shell/test_screen.h>

/**
 * @brief Create and render a static shell screen model for renderer testing.
 *
 * No input is polled and no command execution is provided in this stage.
 *
 * @return E_OK on success or an error code.
 */
kerr_t shell_test_screen_show(void) {
    rect_size_t size = compositor_get_size();
    if (size.width < 8U || size.height < 16U) {
        return E_NOTAVAIL;
    }

    shell_screen_t screen = {0};
    size_t columns = (size_t)(size.width / 8U);
    size_t rows = (size_t)(size.height / 16U);

    K_TRY(shell_screen_init(&screen, columns, rows));
    K_TRY(shell_screen_clear(&screen));

    kerr_t err = shell_screen_write(&screen, 0U, 0U, "NanoOS Kernel Shell");
    if (!k_failed(err)) {
        err = shell_screen_write(&screen, 0U, 1U, "Renderer test - fullscreen character-cell model");
    }
    if (!k_failed(err)) {
        err = shell_screen_write(&screen, 0U, 2U, "Framebuffer / compositor / embedded PSF2 font");
    }
    if (!k_failed(err)) {
        err = shell_screen_write(&screen, 0U, 3U, "Unicode test: ae oe ue - ae oe ue");
    }
    if (!k_failed(err)) {
        err = shell_screen_write(&screen, 0U, 5U, "Input: disabled");
    }
    if (!k_failed(err)) {
        err = shell_screen_write(&screen, 0U, 6U, "Commands: disabled");
    }
    if (!k_failed(err)) {
        err = shell_screen_write(&screen, 0U, 8U, "friedrichOsDev@nanoos> ");
    }

    if (!k_failed(err)) {
        err = shell_renderer_init();
    }
    if (!k_failed(err)) {
        /* One-past-the-end disables the cursor overlay for this static screen. */
        err = shell_renderer_render(&screen, screen.columns * screen.rows);
    }

    shell_screen_destroy(&screen);
    return err;
}
