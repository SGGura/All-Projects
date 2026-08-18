/*
 * gfx.h - minimal RGB565 framebuffer drawing, no dynamic memory, no libc.
 *
 * Every primitive takes screen coordinates. The buffer may cover only a
 * horizontal band of the screen (x_off / y_off), so a microcontroller with
 * little RAM can render one band at a time instead of a full 150 KB frame.
 */
#ifndef GFX_H
#define GFX_H

#include <stdint.h>

typedef struct {
    uint16_t *buf;   /* w * h pixels, RGB565, big-endian order is up to the panel driver */
    int16_t   w, h;  /* size of the buffer                                               */
    int16_t   x_off; /* screen x of buffer column 0                                      */
    int16_t   y_off; /* screen y of buffer row 0                                         */
} gfx_t;

#define GFX_RGB(r, g, b) \
    ((uint16_t)((((r) & 0xF8) << 8) | (((g) & 0xFC) << 3) | ((b) >> 3)))

static inline uint16_t gfx_rgb(uint8_t r, uint8_t g, uint8_t b)
{
    return GFX_RGB(r, g, b);
}

/* Scales an RGB888 triple by percent/100 (clamped) and packs it. */
uint16_t gfx_rgb_scaled(uint8_t r, uint8_t g, uint8_t b, int percent);

void gfx_clear(gfx_t *g, uint16_t color);
void gfx_pixel(gfx_t *g, int x, int y, uint16_t color);
void gfx_fill_rect(gfx_t *g, int x, int y, int w, int h, uint16_t color);
void gfx_hline(gfx_t *g, int x, int y, int w, uint16_t color);
void gfx_vline(gfx_t *g, int x, int y, int h, uint16_t color);
/* Outline of thickness t drawn inside the given rectangle. */
void gfx_frame(gfx_t *g, int x, int y, int w, int h, int t, uint16_t color);

/* Bitmap text, 5x7 cell with a 1 px gap, uppercase-only (see font5x7.c). */
int  gfx_text_width(const char *s, int scale);
int  gfx_text_height(int scale);
void gfx_text(gfx_t *g, int x, int y, const char *s, uint16_t color, int scale);
void gfx_text_center(gfx_t *g, int cx, int y, const char *s, uint16_t color, int scale);
void gfx_text_right(gfx_t *g, int right, int y, const char *s, uint16_t color, int scale);

/* Unsigned decimal into a caller supplied buffer, optionally zero padded.
   Returns the string, which always points into out. */
char *gfx_utoa(char *out, uint32_t value, int pad);

#endif /* GFX_H */
