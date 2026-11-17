/**
 * @file shell.h
 * @brief Public shell subsystem interface.
 * @author friedrichOsDev
 */

#pragma once

#include <pch.h>

/**
 * @brief Initializes the shell subsystem.
 *
 * @return E_OK on success, or an error code on failure.
 */
kerr_t shell_init(void);

/**
 * @brief Requests non-blocking shell shutdown.
 *
 * Returns E_NOTAVAIL while the renderer thread is still cleaning up. Call
 * again after the renderer has exited to complete shutdown.
 *
 * @return E_OK when shutdown is complete, E_NOTAVAIL while pending.
 */
kerr_t shell_deinit(void);

/**
 * @brief Destroys the shell subsystem.
 *
 * This is an alias for shell_deinit().
 *
 * @return E_OK when shutdown is complete, E_NOTAVAIL while pending.
 */
kerr_t shell_destroy(void);
