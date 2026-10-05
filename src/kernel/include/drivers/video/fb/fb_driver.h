/**
 * @file fb_driver.h
 * @brief Treiber für generische Framebuffer.
 * @author friedrichOsDev
 */

#pragma once

#include <stdbool.h>

/**
 * @brief Registriert den generischen Framebuffer-Treiber beim Video-HAL.
 * @return `true` bei erfolgreicher Registrierung.
 */
bool generic_fb_driver_register();