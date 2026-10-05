/**
 * @file framebuffer.h
 * @brief Schnittstelle für den Framebuffer Treiber.
 * @author friedrichOsDev
 */

#pragma once

#include <core/init.h>
#include <core/sync.h>
#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Repräsentiert eine Farbe im RGBA-Format (32-Bit gepackt).
 */
typedef struct {
    uint8_t b; /**< Blau-Kanal (0–255). */
    uint8_t g; /**< Grün-Kanal (0–255). */
    uint8_t r; /**< Rot-Kanal (0–255). */
    uint8_t a; /**< Alpha-Kanal für Transparenz (0 = transparent, 255 = deckend). */
} __attribute__((packed)) color_t;

/**
 * @brief Erstellt ein `color_t`-Objekt mit voller Deckkraft (Alpha = 255).
 * @param r_val Rot-Wert (0–255).
 * @param g_val Grün-Wert (0–255).
 * @param b_val Blau-Wert (0–255).
 */
#define MAKE_COLOR(r_val, g_val, b_val) \
    ((color_t){.b = (b_val), .g = (g_val), .r = (r_val), .a = 255})

/**
 * @brief Erstellt ein `color_t`-Objekt mit explizitem Alpha-Wert.
 * @param r_val Rot-Wert (0–255).
 * @param g_val Grün-Wert (0–255).
 * @param b_val Blau-Wert (0–255).
 * @param a_val Alpha-Wert (0–255).
 */
#define MAKE_COLOR_ALPHA(r_val, g_val, b_val, a_val) \
    ((color_t){.b = (b_val), .g = (g_val), .r = (r_val), .a = (a_val)})

/**
 * @brief Gibt eine Kopie einer Farbe mit neuem Alpha-Wert zurück.
 * @param color Basis-Farbe vom Typ `color_t`.
 * @param a_val Neuer Alpha-Wert (0–255).
 */
#define SET_COLOR_ALPHA(color, a_val) \
    ((color_t){.b = (color).b, .g = (color).g, .r = (color).r, .a = (a_val)})

/* --- Vordefinierte Standardfarben --- */
static const color_t COLOR_BLACK = MAKE_COLOR(0, 0, 0);                /**< Schwarz */
static const color_t COLOR_DARK_GRAY = MAKE_COLOR(64, 64, 64);         /**< Dunkelgrau */
static const color_t COLOR_GRAY = MAKE_COLOR(128, 128, 128);           /**< Grau */
static const color_t COLOR_LIGHT_GRAY = MAKE_COLOR(211, 211, 211);     /**< Hellgrau */
static const color_t COLOR_WHITE = MAKE_COLOR(255, 255, 255);          /**< Weiß */
static const color_t COLOR_RED = MAKE_COLOR(255, 0, 0);                /**< Rot */
static const color_t COLOR_GREEN = MAKE_COLOR(0, 255, 0);              /**< Grün */
static const color_t COLOR_BLUE = MAKE_COLOR(0, 0, 255);               /**< Blau */
static const color_t COLOR_YELLOW = MAKE_COLOR(255, 255, 0);           /**< Gelb */
static const color_t COLOR_CYAN = MAKE_COLOR(0, 255, 255);             /**< Cyan */
static const color_t COLOR_MAGENTA = MAKE_COLOR(255, 0, 255);          /**< Magenta */
static const color_t COLOR_ORANGE = MAKE_COLOR(255, 165, 0);           /**< Orange */
static const color_t COLOR_PINK = MAKE_COLOR(255, 192, 203);           /**< Rosa */
static const color_t COLOR_PURPLE = MAKE_COLOR(128, 0, 128);           /**< Violett */
static const color_t COLOR_BROWN = MAKE_COLOR(139, 69, 19);            /**< Braun */
static const color_t COLOR_TRANSPARENT = MAKE_COLOR_ALPHA(0, 0, 0, 0); /**< Vollkommen transparent */

/**
 * @brief Zweidimensionale Punkt- bzw. Koordinatenstruktur.
 */
typedef struct {
    int64_t x; /**< X-Koordinate. */
    int64_t y; /**< Y-Koordinate. */
} point_t;

/**
 * @brief Dimensionen eines Bildschirms oder Puffers.
 */
typedef struct {
    uint64_t width;  /**< Breite in Pixeln. */
    uint64_t height; /**< Höhe in Pixeln. */
} rect_size_t;

/**
 * @brief Repräsentiert eine Zeichenfläche (Software-Puffer).
 */
typedef struct {
    uint32_t *buffer; /**< Zeiger auf den pixelbasierten Bildspeicher (RGBA32). */
    rect_size_t size; /**< Abmessungen der Zeichenfläche. */
    uint64_t pitch;   /**< Zeilenabstand (Pitch) in Bytes. */
} canvas_t;

/**
 * @brief Repräsentiert eine z-geordnete Ebene im Compositor.
 */
typedef struct layer {
    point_t pos;        /**< Position der Ebene auf dem Bildschirm. */
    int32_t z_index;    /**< Z-Index zur Bestimmung der Zeichenreihenfolge (höher = vorne). */
    bool visible;       /**< Sichtbarkeits-Flag der Ebene. */
    canvas_t canvas;    /**< Zughöriger Canvas-Puffer der Ebene. */
    mutex_t lock;       /**< Mutex zur Synchronisation des Zugriffs auf die Ebene. */
    struct layer *next; /**< Zeiger auf die nächste Ebene in der Verkettung. */
} layer_t;

/* --- Canvas Primitive API --- */

/**
 * @brief Erstellt eine neue Zeichenfläche (Canvas).
 * @param size Gewünschte Abmessungen der Zeichenfläche.
 * @return Zeiger auf die erstelle `canvas_t`-Struktur oder NULL bei Speichermangel.
 */
canvas_t *canvas_create(rect_size_t size);

/**
 * @brief Gibt eine Zeichenfläche und deren Puffer frei.
 * @param canvas Zeiger auf das zu zerstörende Canvas-Objekt.
 */
void canvas_destroy(canvas_t *canvas);

/**
 * @brief Füllt die gesamte Zeichenfläche mit einer festen Farbe.
 * @param canvas Ziel-Canvas.
 * @param color Füllfarbe.
 */
void canvas_clear(canvas_t *canvas, color_t color);

/**
 * @brief Zeichnet einen einzelnen Pixel mit Alpha-Blending auf ein Canvas.
 * @param canvas Ziel-Canvas.
 * @param pos Position des Pixels.
 * @param color Farbe des Pixels.
 */
void canvas_draw_pixel(canvas_t *canvas, point_t pos, color_t color);

/**
 * @brief Zeichnet ein Dreieck auf ein Canvas.
 * @param canvas Ziel-Canvas.
 * @param vertex1 Erster Eckpunkt.
 * @param vertex2 Zweiter Eckpunkt.
 * @param vertex3 Dritter Eckpunkt.
 * @param color Farbe des Dreiecks.
 * @param filled `true` für ein gefülltes Dreieck, sonst Kontur.
 * @param border_size Linienstärke der Kontur (falls `filled = false`).
 */
void canvas_draw_triangle(canvas_t *canvas, point_t vertex1, point_t vertex2, point_t vertex3, color_t color, bool filled, uint64_t border_size);

/**
 * @brief Zeichnet ein Rechteck auf ein Canvas.
 * @param canvas Ziel-Canvas.
 * @param pos Position der oberen linken Ecke.
 * @param size Größe des Rechtecks.
 * @param color Farbe des Rechtecks.
 * @param filled `true` für ein gefülltes Rechteck, sonst Kontur.
 * @param border_size Rahmenstärke der Kontur (falls `filled = false`).
 */
void canvas_draw_rectangle(canvas_t *canvas, point_t pos, rect_size_t size, color_t color, bool filled, uint64_t border_size);

/**
 * @brief Zeichnet eine Linie mittels Bresenham-Algorithmus auf ein Canvas.
 * @param canvas Ziel-Canvas.
 * @param start Startpunkt der Linie.
 * @param end Endpunkt der Linie.
 * @param color Farbe der Linie.
 * @param thickness Linienstärke in Pixeln.
 */
void canvas_draw_line(canvas_t *canvas, point_t start, point_t end, color_t color, uint64_t thickness);

/**
 * @brief Zeichnet einen Kreis auf ein Canvas.
 * @param canvas Ziel-Canvas.
 * @param center Mittelpunkt des Kreises.
 * @param radius Radius des Kreises in Pixeln.
 * @param color Farbe des Kreises.
 * @param filled `true` für einen gefüllten Kreis, sonst Kontur.
 * @param border_size Rahmenstärke der Kontur (falls `filled = false`).
 */
void canvas_draw_circle(canvas_t *canvas, point_t center, uint64_t radius, color_t color, bool filled, uint64_t border_size);

/* --- Layer Management API --- */

/**
 * @brief Erstellt eine neue Layer-Ebene und fügt sie sortiert in die Rendering-Liste ein.
 * @param size Abmessungen des Layers.
 * @param z_index Z-Index für die Tiefen-Reihenfolge.
 * @return Zeiger auf den erstellten Layer oder NULL bei Speichermangel.
 */
layer_t *layer_create(rect_size_t size, int32_t z_index);

/**
 * @brief Entfernt einen Layer sicher aus der Liste und gibt seine Ressourcen frei.
 * @param layer Zeiger auf den zu löschenden Layer.
 */
void layer_destroy(layer_t *layer);

/**
 * @brief Setzt die X/Y-Position eines Layers auf dem Bildschirm.
 * @param layer Ziel-Layer.
 * @param pos Neue Zielposition.
 */
void layer_set_position(layer_t *layer, point_t pos);

/**
 * @brief Ändert die Größe eines Layers und reallokiert seinen Puffer.
 * @param layer Ziel-Layer.
 * @param size Neue Abmessungen.
 */
void layer_set_size(layer_t *layer, rect_size_t size);

/**
 * @brief Schaltet die Sichtbarkeit eines Layers um.
 * @param layer Ziel-Layer.
 * @param visible `true`, um den Layer darzustellen, `false` zum Verstecken.
 */
void layer_set_visible(layer_t *layer, bool visible);

/**
 * @brief Ändert den Z-Index eines Layers und sortiert die Ebene neu ein.
 * @param layer Ziel-Layer.
 * @param z_index Neuer Z-Index.
 */
void layer_set_zindex(layer_t *layer, int32_t z_index);

/**
 * @brief Sperrt den Mutex des Layers vor Beginn von Zeichenoperationen.
 * @param layer Ziel-Layer.
 */
void layer_draw_begin(layer_t *layer);

/**
 * @brief Entsperrt den Mutex des Layers nach Abschluss von Zeichenoperationen.
 * @param layer Ziel-Layer.
 */
void layer_draw_end(layer_t *layer);

/* --- Core Framebuffer Driver API --- */

/**
 * @brief Initialisiert den Framebuffer-Treiber, allokiert den Backbuffer und startet den Renderer-Thread.
 */
void fb_init(void);

/**
 * @brief Stoppt das Rendering und gibt die Framebuffer-Ressourcen frei.
 */
void fb_deinit(void);

/**
 * @brief Aktiviert den Rendering-Prozess des Compositors.
 */
void fb_enable_rendering(void);

/**
 * @brief Pausiert den Rendering-Prozess des Compositors.
 */
void fb_disable_rendering(void);

/**
 * @brief Setzt die Ziel-Framerate (FPS) für das Render-System.
 * @param fps Bildwiederholrate in Bildern pro Sekunde.
 */
void fb_set_target_fps(uint64_t fps);

/**
 * @brief Liefert die aktuell gemessenen Frames pro Sekunde (FPS).
 * @return Aktuelle Framerate.
 */
uint64_t fb_get_current_fps(void);

/**
 * @brief Ruft die globale Bildschirmbreite in Pixeln ab.
 * @return Breite des Framebuffers.
 */
static inline uint64_t fb_get_width(void) {
    return kernel_fb_info.fb_width;
}

/**
 * @brief Ruft die globale Bildschirmhöhe in Pixeln ab.
 * @return Höhe des Framebuffers.
 */
static inline uint64_t fb_get_height(void) {
    return kernel_fb_info.fb_height;
}

/**
 * @brief Ruft die Gesamtabmessung des Bildschirms ab.
 * @return `rect_size_t` mit Breite und Höhe des Framebuffers.
 */
static inline rect_size_t fb_get_size(void) {
    return (rect_size_t){.width = fb_get_width(), .height = fb_get_height()};
}