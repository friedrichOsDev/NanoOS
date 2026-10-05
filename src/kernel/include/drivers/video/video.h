/**
 * @file video.h
 * @brief Hardware Abstraction Layer (HAL) Schnittstelle für Video-Treiber.
 * @author friedrichOsDev
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Repräsentiert die Eigenschaften eines Grafikmodus.
 */
typedef struct {
    uint32_t width;  /**< Breite in Pixeln. */
    uint32_t height; /**< Höhe in Pixeln. */
    uint32_t bpp;    /**< Farbtiefe in Bits pro Pixel (z. B. 32). */
    uint32_t pitch;  /**< Zeilenabstand in Bytes. */
} video_mode_t;

/**
 * @brief Treiberschnittstelle (VTable) für alle Hardware-Grafiktreiber.
 */
typedef struct video_driver_ops {
    const char *driver_name; /**< Name des Treibers für Debug-Ausgaben. */

    /** Initialisiert die spezifische Grafikhardware. */
    bool (*init)();

    /** Gibt Hardware-Ressourcen des Treibers frei. */
    void (*deinit)();

    /** Wechselt den Grafikmodus (falls vom Treiber unterstützt). */
    bool (*set_mode)(video_mode_t mode);

    /** Ruft den aktuellen Grafikmodus ab. */
    bool (*get_mode)(video_mode_t *out_mode);

    /** Liefert die Adresse des physischen/virtuellen Video-Speichers. */
    void *(*get_framebuffer_address)();

    /**
     * Kopiert den Backbuffer/Compositor-Inhalt in den Bildschirmspeicher.
     * Kann Hardware-Beschleunigung oder DMA nutzen.
     */
    void (*flush)(const void *src_buffer, uint64_t size);
} video_driver_ops_t;

/* --- HAL Public API --- */

/**
 * @brief Registriert einen konkreten Treiber beim Video-HAL.
 * @param ops Zeiger auf die Treiber-VTable.
 * @return `true` bei Erfolg, sonst `false`.
 */
bool video_register_driver(const video_driver_ops_t *ops);

/**
 * @brief Initialisiert das Video-Subsystem mit dem registrierten Treiber.
 * @return `true` bei Erfolg, sonst `false`.
 */
bool video_init();

/**
 * @brief Stoppt das Video-Subsystem und den aktiven Treiber.
 */
void video_deinit();

/**
 * @brief Wechselt die Bildschirmauflösung über den aktiven Treiber.
 */
bool video_set_mode(uint32_t width, uint32_t height, uint32_t bpp);

/**
 * @brief Ruft den aktuellen Modus des aktiven Treibers ab.
 */
bool video_get_mode(video_mode_t *out_mode);

/**
 * @brief Übermittelt den fertigen Backbuffer an den aktiven Treiber.
 */
void video_flush(const void *src_buffer, uint64_t size);

/**
 * @brief Liefert die Basisadresse des aktiven Framebuffers.
 */
void *video_get_framebuffer_address();

/**
 * @brief Liefert den Namen des aktuell aktiven Treibers.
 */
const char *video_get_driver_name();