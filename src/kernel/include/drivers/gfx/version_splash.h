/**
 * @file version_splash.h
 * @brief Version splash screen renderer for NanoOS
 * @author friedrichOsDev
 */

#pragma once

#include <pch.h>

/**
 * @brief Displays a version splash screen centered on the screen for a given duration.
 * 
 * @param[in] version_str Version string to display (e.g. "0.0.3").
 * @param[in] duration_ms Duration in milliseconds to show the splash screen.
 * @return E_OK on success, or an error code on failure.
 */
kerr_t version_splash_show(const char *version_str, uint64_t duration_ms);
