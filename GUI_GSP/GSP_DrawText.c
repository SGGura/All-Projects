#include <p32xxxx.h>
#include <xc.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include "GUI_GSP.h"
#include "GSP_DrawText.h"


const uint8_t _lv_bpp4_opa_table[16] = {0,  17, 34,  51,  /*Opacity mapping with bpp = 4*/
                                        68, 85, 102, 119,
                                        136, 153, 170, 187,
                                        204, 221, 238, 255
                                       };


void * _lv_utils_bsearch(const void * key, const void *bs, uint32_t n, uint32_t size, int32_t(*cmp)(const void * pRef, const void * pElement));
static int32_t unicode_list_compare(const void * ref, const void * element);
bool lv_font_get_glyph_dsc(const lv_font_t * font_p, lv_font_glyph_dsc_t * dsc_out, uint32_t letter, uint32_t letter_next);
static uint32_t get_glyph_dsc_id(const lv_font_t * font, uint32_t letter);
static uint32_t lv_txt_utf8_next(const char * txt, uint32_t * i);

void DrawTextUTF8(int X, int Y, const lv_font_t *font, char *text, WORD_COLOR color, int letter_spacing, int invert)
{
	unsigned int letter;
	int n_c = 0;
	int x;
	bool first_char = true;  // Флаг для первого символа (не добавляем spacing перед ним)
MoveTo(X, Y);
	
	// Предвычисляем font_dsc один раз для всех символов
	lv_font_fmt_txt_dsc_t *font_dsc = (lv_font_fmt_txt_dsc_t *)font->dsc;
	
	do
	{
		letter = lv_txt_utf8_next(text, &n_c);
		if(letter)
		{
			// Добавляем межбуквенный интервал перед символом (кроме первого)
			if(!first_char && letter_spacing != 0)
			{
MoveTo(GetX() + letter_spacing, GetY());
			}
			first_char = false;
			
			const uint8_t *buf = lv_font_get_bitmap_fmt_txt(font, letter);
			uint32_t index_l = get_glyph_dsc_id(font, letter);
			
			// Оптимизация: используем указатель напрямую вместо void* арифметики
			const lv_font_fmt_txt_glyph_dsc_t *glyph = &font_dsc->glyph_dsc[index_l];

			// Предвычисляем значения вне циклов
			int x_start = GetX();
			int y_base = GetY() + font->line_height - glyph->box_h;
			int pixel_x_offset = glyph->ofs_x;
			int pixel_y_offset_base = glyph->ofs_y;
			int pixel_y_offset_bpp1 = pixel_y_offset_base + font->base_line;
			
			int n = 0;  // Индекс в буфере bitmap для текущего символа
			
			switch(font_dsc->bpp)
			{
				case 1:
				{
SetColor(color);
					unsigned char byte = 0;
					unsigned char mask = 0;
					int bit_counter = 0;  // Заменяем i++ % 8 на счетчик
					
					for(int y_idx = 0; y_idx < glyph->box_h; ++y_idx)
					{
						x = x_start;
						int pixel_y = y_base - pixel_y_offset_bpp1 + y_idx;
						
						for(int x_idx = 0; x_idx < glyph->box_w; ++x_idx)
						{
							// Оптимизация: замена (i++ % 8) на битовую операцию
							if(bit_counter == 0) 
							{
								byte = buf[n++];
								mask = 0b10000000;
								bit_counter = 8;
							}
							
							if(byte & mask) 
							{
								if(invert) PutPixelXOR(x + pixel_x_offset, pixel_y);
								else       PutPixel(x + pixel_x_offset, pixel_y);
							}
							++x;
							mask >>= 1;
							--bit_counter;
						}		
					}
					
					// Оптимизация: замена деления на битовый сдвиг где возможно
					int sh_x = glyph->adv_w >> 4;  // glyph->adv_w/16
					if((glyph->adv_w & 0x0F) > 5) ++sh_x;  // glyph->adv_w%16 > 5
MoveTo(x_start + sh_x, GetY());
					break;
				}
				case 4:
				{
					// Предвычисляем цвета для всех возможных значений (0-15)
					unsigned short precomputed_colors[16];
					for(int c_idx = 0; c_idx < 16; ++c_idx)
					{
						precomputed_colors[c_idx] = (color * _lv_bpp4_opa_table[c_idx]) / 255;
					}
					
					unsigned char byte = 0;
					unsigned char mask = 0;
					int nibble_counter = 0;  // Заменяем i++ % 2 на счетчик
					
					for(int y_idx = 0; y_idx < glyph->box_h; ++y_idx)
					{
						x = x_start;
						int pixel_y = y_base - pixel_y_offset_base + y_idx;
						
						for(int x_idx = 0; x_idx < glyph->box_w; ++x_idx)
						{
							// Оптимизация: замена (i++ % 2) на битовую операцию
							if(nibble_counter == 0) 
							{
								byte = buf[n++];
								mask = 0b11110000;
								nibble_counter = 2;
							}
							
							if(byte & mask) 
							{
								unsigned short c;
								if(mask == 0b11110000) 
								{
									c = precomputed_colors[byte >> 4];
								}
								else
								{
									c = precomputed_colors[byte & 0x0f];
								}
SetColor(c);
								if(invert) PutPixelXOR(x + pixel_x_offset, pixel_y);
								else       PutPixel(x + pixel_x_offset, pixel_y);
							}
							++x;
							mask >>= 4;
							--nibble_counter;
						}		
					}
					
					// Оптимизация: замена деления на битовый сдвиг
					int sh_x = glyph->adv_w >> 4;  // glyph->adv_w/16
					if((glyph->adv_w & 0x0F) > 5) ++sh_x;  // glyph->adv_w%16 > 5
MoveTo(x_start + sh_x, GetY());
					break;
				}
			}
		}
	}while(letter);
}
//*************************************************************************
int GetTextHightUTF8(const lv_font_t *font)
{
	return(font->line_height);
}
//*************************************************************************
int GetTextWidthUTF8(const lv_font_t *font, char *text, int letter_spacing)
{
	unsigned int letter;
	int whidth = 0;
	int n_c = 0;
	int char_count = 0;
	do
	{
		letter = lv_txt_utf8_next(text, &n_c);
		if(letter)
		{
			uint32_t index_l = get_glyph_dsc_id(font, letter);
			lv_font_fmt_txt_dsc_t *font_dsc =  (lv_font_fmt_txt_dsc_t *)font->dsc;
			void *tmp_glyph = (void *)font_dsc->glyph_dsc;
			tmp_glyph = tmp_glyph + (index_l* sizeof(lv_font_fmt_txt_glyph_dsc_t));
			lv_font_fmt_txt_glyph_dsc_t *glyph = (lv_font_fmt_txt_glyph_dsc_t *)tmp_glyph;

			int sh_x = glyph->adv_w/16;
			if(glyph->adv_w%16 > 5) ++sh_x;
			whidth += sh_x;
			char_count++;
		}
	}while(letter);
	
	// Добавляем межбуквенные интервалы (между символами, не перед первым и не после последнего)
	if(char_count > 1 && letter_spacing != 0)
	{
		whidth += letter_spacing * (char_count - 1);
	}
	
	return(whidth);
}
//*******************************************************************
short DrawAllignTextUTF8(short x, short y,  short len, short pos, char *text , lv_font_t *font, WORD_COLOR collor_T, int letter_spacing, int invert)
{
	short width, height;

	width = GetTextWidthUTF8(font, text, letter_spacing);
	height = GetTextHightUTF8(font);

	if(len < width) width = len;

	//SetColor(collor_T);
	switch(pos)
	{
		case ALLIGN_TEXT_LEFT:
		{
			DrawTextUTF8(x, y, font, text, collor_T, letter_spacing, invert);
			break;
		}
		case ALLIGN_TEXT_RIGHT:
		{
			DrawTextUTF8(x+(len-width), y, font, text, collor_T, letter_spacing, invert);
			break;
		}
		case ALLIGN_TEXT_CENTER:
		{
			DrawTextUTF8(x+((len-width) >> 1), y, font, text, collor_T, letter_spacing, invert);
			break;
		}
	}
	return(width);
}
//*************************************************************
static uint32_t get_glyph_dsc_id(const lv_font_t * font, uint32_t letter)
{
	if (letter == '\0') return 0;

	lv_font_fmt_txt_dsc_t * fdsc = (lv_font_fmt_txt_dsc_t *) font->dsc;

	/*Check the cache first*/
	if (fdsc->cache && letter == fdsc->cache->last_letter) return fdsc->cache->last_glyph_id;

	uint16_t i;
	for (i = 0; i < fdsc->cmap_num; i++)
	{

		/*Relative code point*/
		uint32_t rcp = letter - fdsc->cmaps[i].range_start;
		if (rcp > fdsc->cmaps[i].range_length) continue;
		uint32_t glyph_id = 0;
		if (fdsc->cmaps[i].type == LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY)
		{
			glyph_id = fdsc->cmaps[i].glyph_id_start + rcp;
		}
		else if (fdsc->cmaps[i].type == LV_FONT_FMT_TXT_CMAP_FORMAT0_FULL)
		{
			const uint8_t * gid_ofs_8 = fdsc->cmaps[i].glyph_id_ofs_list;
			glyph_id = fdsc->cmaps[i].glyph_id_start + gid_ofs_8[rcp];
		}
		else if (fdsc->cmaps[i].type == LV_FONT_FMT_TXT_CMAP_SPARSE_TINY)
		{
			uint16_t key = rcp;
			uint16_t * p = _lv_utils_bsearch(&key, fdsc->cmaps[i].unicode_list, fdsc->cmaps[i].list_length,
											 sizeof (fdsc->cmaps[i].unicode_list[0]), unicode_list_compare);

			if (p)
			{
				lv_uintptr_t ofs = p - fdsc->cmaps[i].unicode_list;
				glyph_id = fdsc->cmaps[i].glyph_id_start + ofs;
			}
		}
		else if (fdsc->cmaps[i].type == LV_FONT_FMT_TXT_CMAP_SPARSE_FULL)
		{
			uint16_t key = rcp;
			uint16_t * p = _lv_utils_bsearch(&key, fdsc->cmaps[i].unicode_list, fdsc->cmaps[i].list_length,
											 sizeof (fdsc->cmaps[i].unicode_list[0]), unicode_list_compare);

			if (p)
			{
				lv_uintptr_t ofs = p - fdsc->cmaps[i].unicode_list;
				const uint16_t * gid_ofs_16 = fdsc->cmaps[i].glyph_id_ofs_list;
				glyph_id = fdsc->cmaps[i].glyph_id_start + gid_ofs_16[ofs];
			}
		}

		/*Update the cache*/
		if (fdsc->cache)
		{
			fdsc->cache->last_letter = letter;
			fdsc->cache->last_glyph_id = glyph_id;
		}
		return glyph_id;
	}

	if (fdsc->cache)
	{
		fdsc->cache->last_letter = letter;
		fdsc->cache->last_glyph_id = 0;
	}
	return 0;
}
//********************************************************

const uint8_t * lv_font_get_bitmap_fmt_txt(const lv_font_t * font, uint32_t unicode_letter)
{

	if (unicode_letter == '\t') unicode_letter = ' ';

	lv_font_fmt_txt_dsc_t * fdsc = (lv_font_fmt_txt_dsc_t *) font->dsc;
	uint32_t gid = get_glyph_dsc_id(font, unicode_letter);
	if (!gid) return NULL;

	const lv_font_fmt_txt_glyph_dsc_t * gdsc = &fdsc->glyph_dsc[gid];

	if (fdsc->bitmap_format == LV_FONT_FMT_TXT_PLAIN)
	{
		return &fdsc->glyph_bitmap[gdsc->bitmap_index];
	}

}
//************************************************************************

bool lv_font_get_glyph_dsc_fmt_txt(const lv_font_t * font, lv_font_glyph_dsc_t * dsc_out, uint32_t unicode_letter, uint32_t unicode_letter_next)
{
	return true;
}
//*************************************************************************************
bool lv_font_get_glyph_dsc(const lv_font_t * font_p, lv_font_glyph_dsc_t * dsc_out, uint32_t letter, uint32_t letter_next)
{
	const lv_font_t * placeholder_font = NULL;
	const lv_font_t * f = font_p;

	dsc_out->resolved_font = NULL;

	while (f)
	{
		bool found = f->get_glyph_dsc(f, dsc_out, letter, letter_next);
		if (found)
		{
			if (!dsc_out->is_placeholder)
			{
				dsc_out->resolved_font = f;
				return true;
			}
			else if (placeholder_font == NULL)
			{
				placeholder_font = f;
			}
		}
		f = f->fallback;
	}

	if (placeholder_font != NULL)
	{
		placeholder_font->get_glyph_dsc(placeholder_font, dsc_out, letter, letter_next);
		dsc_out->resolved_font = placeholder_font;
		return true;
	}


	if (letter < 0x20 ||
		letter == 0xf8ff || /*LV_SYMBOL_DUMMY*/
		letter == 0x200c)
	{ /*ZERO WIDTH NON-JOINER*/
		dsc_out->box_w = 0;
		dsc_out->adv_w = 0;
	}
	else
	{
		dsc_out->box_w = font_p->line_height / 2;
		dsc_out->adv_w = dsc_out->box_w + 2;
	}

	dsc_out->resolved_font = NULL;
	dsc_out->box_h = font_p->line_height;
	dsc_out->ofs_x = 0;
	dsc_out->ofs_y = 0;
	dsc_out->bpp = 1;
	dsc_out->is_placeholder = true;

	return false;
}

//**************************************************************************************

void * _lv_utils_bsearch(const void * key, const void * bs, uint32_t n, uint32_t size, int32_t(*cmp)(const void * pRef, const void * pElement))
{
	const char * middle;
	int32_t c;

	for (middle = bs; n != 0;)
	{
		middle += (n / 2) * size;
		if ((c = (*cmp)(key, middle)) > 0)
		{
			n = (n / 2) - ((n & 1) == 0);
			bs = (middle += size);
		}
		else if (c < 0)
		{
			n /= 2;
			middle = bs;
		}
		else
		{
			return (char *) middle;
		}
	}
	return NULL;
}

//***********************************************************************
static int32_t unicode_list_compare(const void * ref, const void * element)
{
	return ((int32_t) (*(uint16_t *) ref)) - ((int32_t) (*(uint16_t *) element));
}
//*************************************************************************
static uint32_t lv_txt_utf8_next(const char * txt, uint32_t * i)
{
    /**
     * Unicode to UTF-8
     * 00000000 00000000 00000000 0xxxxxxx -> 0xxxxxxx
     * 00000000 00000000 00000yyy yyxxxxxx -> 110yyyyy 10xxxxxx
     * 00000000 00000000 zzzzyyyy yyxxxxxx -> 1110zzzz 10yyyyyy 10xxxxxx
     * 00000000 000wwwzz zzzzyyyy yyxxxxxx -> 11110www 10zzzzzz 10yyyyyy 10xxxxxx
     */

    uint32_t result = 0;

    /*Dummy 'i' pointer is required*/
    uint32_t i_tmp = 0;
    if(i == NULL) i = &i_tmp;

    /*Normal ASCII*/
    if(LV_IS_ASCII(txt[*i])) {
        result = txt[*i];
        (*i)++;
    }
    /*Real UTF-8 decode*/
    else {
        /*2 bytes UTF-8 code*/
        if(LV_IS_2BYTES_UTF8_CODE(txt[*i])) {
            result = (uint32_t)(txt[*i] & 0x1F) << 6;
            (*i)++;
            if(LV_IS_INVALID_UTF8_CODE(txt[*i])) return 0;
            result += (txt[*i] & 0x3F);
            (*i)++;
        }
        /*3 bytes UTF-8 code*/
        else if(LV_IS_3BYTES_UTF8_CODE(txt[*i])) {
            result = (uint32_t)(txt[*i] & 0x0F) << 12;
            (*i)++;

            if(LV_IS_INVALID_UTF8_CODE(txt[*i])) return 0;
            result += (uint32_t)(txt[*i] & 0x3F) << 6;
            (*i)++;

            if(LV_IS_INVALID_UTF8_CODE(txt[*i])) return 0;
            result += (txt[*i] & 0x3F);
            (*i)++;
        }
        /*4 bytes UTF-8 code*/
        else if(LV_IS_4BYTES_UTF8_CODE(txt[*i])) {
            result = (uint32_t)(txt[*i] & 0x07) << 18;
            (*i)++;

            if(LV_IS_INVALID_UTF8_CODE(txt[*i])) return 0;
            result += (uint32_t)(txt[*i] & 0x3F) << 12;
            (*i)++;

            if(LV_IS_INVALID_UTF8_CODE(txt[*i])) return 0;
            result += (uint32_t)(txt[*i] & 0x3F) << 6;
            (*i)++;

            if(LV_IS_INVALID_UTF8_CODE(txt[*i])) return 0;
            result += txt[*i] & 0x3F;
            (*i)++;
        }
        else {
            (*i)++; /*Not UTF-8 char. Go the next.*/
        }
    }
    return result;
}
