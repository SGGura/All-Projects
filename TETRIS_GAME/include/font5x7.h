/*
 * font5x7.h - 5x7 bitmap font covering ASCII 0x20..0x5F.
 *
 * Lowercase input is folded to uppercase by the renderer, anything else is
 * drawn as a blank. Each glyph is five column bytes, bit 0 = top row.
 */
#ifndef FONT5X7_H
#define FONT5X7_H

#include <stdint.h>

#define FONT_FIRST_CHAR 0x20
#define FONT_LAST_CHAR  0x5F
#define FONT_WIDTH      5
#define FONT_HEIGHT     7

extern const uint8_t font5x7[(FONT_LAST_CHAR - FONT_FIRST_CHAR + 1) * FONT_WIDTH];

#endif /* FONT5X7_H */
