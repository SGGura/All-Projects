#ifndef DRAWTEXT_H
#define	DRAWTEXT_H
#include "Fonts/LVGL/GSP_lvgl.h"

void DrawTextUTF8(int X, int Y, const lv_font_t *font, char *text, WORD_COLOR collor_T, int letter_spacing, int invert);
int GetTextWidthUTF8(const lv_font_t *font, char *text, int letter_spacing);
short DrawAllignTextUTF8(short x, short y,  short len, short pos, char *text , lv_font_t *font, WORD_COLOR collor_T, int letter_spacing, int invert);
int GetTextHightUTF8(const lv_font_t *font);

#endif	/* DRAWTEXT_H */

