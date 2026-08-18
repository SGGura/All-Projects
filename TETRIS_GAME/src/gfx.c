#include "gfx.h"
#include "font5x7.h"

#define GLYPH_PITCH (FONT_WIDTH + 1)

uint16_t gfx_rgb_scaled(uint8_t r, uint8_t g, uint8_t b, int percent)
{
    int rr = (int)r * percent / 100;
    int gg = (int)g * percent / 100;
    int bb = (int)b * percent / 100;

    if (rr > 255) rr = 255;
    if (gg > 255) gg = 255;
    if (bb > 255) bb = 255;

    return GFX_RGB((uint8_t)rr, (uint8_t)gg, (uint8_t)bb);
}

void gfx_clear(gfx_t *g, uint16_t color)
{
    int32_t n = (int32_t)g->w * g->h;
    uint16_t *p = g->buf;

    while (n--) {
        *p++ = color;
    }
}

void gfx_pixel(gfx_t *g, int x, int y, uint16_t color)
{
    x -= g->x_off;
    y -= g->y_off;

    if (x < 0 || y < 0 || x >= g->w || y >= g->h) {
        return;
    }
    g->buf[(int32_t)y * g->w + x] = color;
}

void gfx_fill_rect(gfx_t *g, int x, int y, int w, int h, uint16_t color)
{
    int x0, y0, x1, y1, row;

    if (w <= 0 || h <= 0) {
        return;
    }

    x0 = x - g->x_off;
    y0 = y - g->y_off;
    x1 = x0 + w;
    y1 = y0 + h;

    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x1 > g->w) x1 = g->w;
    if (y1 > g->h) y1 = g->h;

    for (row = y0; row < y1; row++) {
        uint16_t *p = &g->buf[(int32_t)row * g->w + x0];
        int n = x1 - x0;
        while (n-- > 0) {
            *p++ = color;
        }
    }
}

void gfx_hline(gfx_t *g, int x, int y, int w, uint16_t color)
{
    gfx_fill_rect(g, x, y, w, 1, color);
}

void gfx_vline(gfx_t *g, int x, int y, int h, uint16_t color)
{
    gfx_fill_rect(g, x, y, 1, h, color);
}

void gfx_frame(gfx_t *g, int x, int y, int w, int h, int t, uint16_t color)
{
    if (w <= 0 || h <= 0 || t <= 0) {
        return;
    }
    if (t * 2 >= w || t * 2 >= h) {
        gfx_fill_rect(g, x, y, w, h, color);
        return;
    }
    gfx_fill_rect(g, x, y, w, t, color);
    gfx_fill_rect(g, x, y + h - t, w, t, color);
    gfx_fill_rect(g, x, y + t, t, h - 2 * t, color);
    gfx_fill_rect(g, x + w - t, y + t, t, h - 2 * t, color);
}

static int glyph_index(char c)
{
    if (c >= 'a' && c <= 'z') {
        c = (char)(c - 'a' + 'A');
    }
    if ((unsigned char)c < FONT_FIRST_CHAR || (unsigned char)c > FONT_LAST_CHAR) {
        return -1;
    }
    return (unsigned char)c - FONT_FIRST_CHAR;
}

int gfx_text_width(const char *s, int scale)
{
    int n = 0;

    while (*s++) {
        n++;
    }
    if (n == 0) {
        return 0;
    }
    return (n * GLYPH_PITCH - 1) * scale;
}

int gfx_text_height(int scale)
{
    return FONT_HEIGHT * scale;
}

void gfx_text(gfx_t *g, int x, int y, const char *s, uint16_t color, int scale)
{
    if (scale < 1) {
        scale = 1;
    }

    for (; *s; s++, x += GLYPH_PITCH * scale) {
        int idx = glyph_index(*s);
        const uint8_t *glyph;
        int col;

        if (idx < 0) {
            continue;
        }
        glyph = &font5x7[idx * FONT_WIDTH];

        for (col = 0; col < FONT_WIDTH; col++) {
            uint8_t bits = glyph[col];
            int row;

            for (row = 0; row < FONT_HEIGHT; row++) {
                if (bits & (1u << row)) {
                    if (scale == 1) {
                        gfx_pixel(g, x + col, y + row, color);
                    } else {
                        gfx_fill_rect(g, x + col * scale, y + row * scale,
                                      scale, scale, color);
                    }
                }
            }
        }
    }
}

void gfx_text_center(gfx_t *g, int cx, int y, const char *s, uint16_t color, int scale)
{
    gfx_text(g, cx - gfx_text_width(s, scale) / 2, y, s, color, scale);
}

void gfx_text_right(gfx_t *g, int right, int y, const char *s, uint16_t color, int scale)
{
    gfx_text(g, right - gfx_text_width(s, scale), y, s, color, scale);
}

char *gfx_utoa(char *out, uint32_t value, int pad)
{
    char tmp[12];
    int n = 0;
    int i = 0;

    do {
        tmp[n++] = (char)('0' + (value % 10u));
        value /= 10u;
    } while (value != 0u && n < (int)sizeof(tmp));

    while (n < pad && n < (int)sizeof(tmp)) {
        tmp[n++] = '0';
    }
    while (n > 0) {
        out[i++] = tmp[--n];
    }
    out[i] = '\0';

    return out;
}
