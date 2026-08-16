/**
 * @file lvgl.h
 * Include all LVGL related headers
 */

#ifndef LVGL_H
#define LVGL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define LV_USE_USER_DATA 1

/***************************
 * CURRENT VERSION OF LVGL
 ***************************/
#define LVGL_VERSION_MAJOR 8
#define LVGL_VERSION_MINOR 3
#define LVGL_VERSION_PATCH 0
#define LVGL_VERSION_INFO "dev"

#define LV_FONT_UNSCII_16   1

/** Format of font character map.*/
enum 
{
    LV_FONT_FMT_TXT_CMAP_FORMAT0_FULL,
    LV_FONT_FMT_TXT_CMAP_SPARSE_FULL,
    LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY,
    LV_FONT_FMT_TXT_CMAP_SPARSE_TINY,
};

enum 
{
    LV_FONT_SUBPX_NONE,
    LV_FONT_SUBPX_HOR,
    LV_FONT_SUBPX_VER,
    LV_FONT_SUBPX_BOTH,
};

#define LV_IS_ASCII(value)              ((value & 0x80U) == 0x00U)
#define LV_IS_2BYTES_UTF8_CODE(value)   ((value & 0xE0U) == 0xC0U)
#define LV_IS_3BYTES_UTF8_CODE(value)   ((value & 0xF0U) == 0xE0U)
#define LV_IS_4BYTES_UTF8_CODE(value)   ((value & 0xF8U) == 0xF0U)
#define LV_IS_INVALID_UTF8_CODE(value)  ((value & 0xC0U) != 0x80U)

/** Bitmap formats*/
typedef enum 
{
    LV_FONT_FMT_TXT_PLAIN      = 0,
    LV_FONT_FMT_TXT_COMPRESSED = 1,
    LV_FONT_FMT_TXT_COMPRESSED_NO_PREFILTER = 1,
} lv_font_fmt_txt_bitmap_format_t;

#define LV_ATTRIBUTE_LARGE_CONST

/* uint8_t, uint16_t, int16_t и т.д. — из <stdint.h> */

typedef uint8_t lv_font_fmt_txt_cmap_type_t;
typedef int16_t lv_coord_t;
typedef	unsigned int	      __uintptr_t;
#ifndef uintptr_t
#define	uintptr_t	__uintptr_t
#endif
typedef uintptr_t lv_uintptr_t;



#define LV_VERSION_CHECK(x,y,z) (x == LVGL_VERSION_MAJOR && (y < LVGL_VERSION_MINOR || (y == LVGL_VERSION_MINOR && z <= LVGL_VERSION_PATCH)))

/**
 * Wrapper functions for VERSION macros
 */

static inline int lv_version_major(void)
{
    return LVGL_VERSION_MAJOR;
}

static inline int lv_version_minor(void)
{
    return LVGL_VERSION_MINOR;
}

static inline int lv_version_patch(void)
{
    return LVGL_VERSION_PATCH;
}

static inline const char *lv_version_info(void)
{
    return LVGL_VERSION_INFO;
}

typedef struct 
{
    uint32_t bitmap_index;          /**< Start index of the bitmap. A font can be max 4 GB.*/
    uint32_t adv_w;                 /**< Draw the next glyph after this width. 28.4 format (real_value * 16 is stored).*/
    uint16_t box_w;                 /**< Width of the glyph's bounding box*/
    uint16_t box_h;                 /**< Height of the glyph's bounding box*/
    int16_t ofs_x;                  /**< x offset of the bounding box*/
    int16_t ofs_y;                  /**< y offset of the bounding box. Measured from the top of the line*/
} lv_font_fmt_txt_glyph_dsc_t;

typedef struct 
{
    /** First Unicode character for this range*/
    uint32_t range_start;

    /** Number of Unicode characters related to this range.
     * Last Unicode character = range_start + range_length - 1*/
    uint16_t range_length;

    /** First glyph ID (array index of `glyph_dsc`) for this range*/
    uint16_t glyph_id_start;

    const uint16_t * unicode_list;

    /** if(type == LV_FONT_FMT_TXT_CMAP_FORMAT0_...) it's `uint8_t *`
     * if(type == LV_FONT_FMT_TXT_CMAP_SPARSE_...)  it's `uint16_t *`
     */
    const void * glyph_id_ofs_list;

    /** Length of `unicode_list` and/or `glyph_id_ofs_list`*/
    uint16_t list_length;

    /** Type of this character map*/
    lv_font_fmt_txt_cmap_type_t type;
} lv_font_fmt_txt_cmap_t;


typedef struct 
{
    uint32_t last_letter;
    uint32_t last_glyph_id;
} lv_font_fmt_txt_glyph_cache_t;

/*Describe store additional data for fonts*/
typedef struct 
{
    /*The bitmaps of all glyphs*/
    const uint8_t * glyph_bitmap;

    /*Describe the glyphs*/
    const lv_font_fmt_txt_glyph_dsc_t * glyph_dsc;

    /*Map the glyphs to Unicode characters.
     *Array of `lv_font_cmap_fmt_txt_t` variables*/
    const lv_font_fmt_txt_cmap_t * cmaps;

    /**
     * Store kerning values.
     * Can be `lv_font_fmt_txt_kern_pair_t *  or `lv_font_kern_classes_fmt_txt_t *`
     * depending on `kern_classes`
     */
    const void * kern_dsc;

    /*Scale kern values in 12.4 format*/
    uint16_t kern_scale;

    /*Number of cmap tables*/
    uint16_t cmap_num       : 9;

    /*Bit per pixel: 1, 2, 3, 4, 8*/
    uint16_t bpp            : 4;

    /*Type of `kern_dsc`*/
    uint16_t kern_classes   : 1;

    /*
     * storage format of the bitmap
     * from `lv_font_fmt_txt_bitmap_format_t`
     */
    uint16_t bitmap_format  : 2;

    /*Cache the last letter and is glyph id*/
    lv_font_fmt_txt_glyph_cache_t * cache;
} lv_font_fmt_txt_dsc_t;


typedef struct 
{
    const struct _lv_font_t *resolved_font; /**< Pointer to a font where the glyph was actually found after handling fallbacks*/
    uint16_t adv_w; /**< The glyph needs this space. Draw the next glyph after this width.*/
    uint16_t box_w; /**< Width of the glyph's bounding box*/
    uint16_t box_h; /**< Height of the glyph's bounding box*/
    int16_t ofs_x;  /**< x offset of the bounding box*/
    int16_t ofs_y;  /**< y offset of the bounding box*/
    uint8_t bpp: 4;  /**< Bit-per-pixel: 1, 2, 4, 8*/
    uint8_t is_placeholder: 1; /** Glyph is missing. But placeholder will still be displayed */
} lv_font_glyph_dsc_t;

/** Describe the properties of a font*/
typedef struct _lv_font_t 
{
    /** Get a glyph's descriptor from a font*/
    bool (*get_glyph_dsc)(const struct _lv_font_t *, lv_font_glyph_dsc_t *, uint32_t letter, uint32_t letter_next);

    /** Get a glyph's bitmap from a font*/
    const uint8_t * (*get_glyph_bitmap)(const struct _lv_font_t *, uint32_t);

    /*Pointer to the font in a font pack (must have the same line height)*/
    lv_coord_t line_height;         /**< The real line height where any text fits*/
    lv_coord_t base_line;           /**< Base line measured from the top of the line_height*/
    uint8_t subpx  : 2;             /**< An element of `lv_font_subpx_t`*/

    int8_t underline_position;      /**< Distance between the top of the underline and base line (< 0 means below the base line)*/
    int8_t underline_thickness;     /**< Thickness of the underline*/
    const void * dsc;               /**< Store implementation specific or run_time data or caching here*/
    const struct _lv_font_t * fallback;   /**< Fallback font for missing glyph. Resolved recursively */
#if LV_USE_USER_DATA
    void * user_data;               /**< Custom user data for font.*/
#endif
    int static_bitmap;              //????????
} lv_font_t;

typedef struct {
    /*To get a kern value of two code points:
       1. Get the `glyph_id_left` and `glyph_id_right` from `lv_font_fmt_txt_cmap_t
       2. for(i = 0; i < pair_cnt * 2; i += 2)
             if(gylph_ids[i] == glyph_id_left &&
                gylph_ids[i+1] == glyph_id_right)
                 return values[i / 2];
     */
    const void * glyph_ids;
    const int8_t * values;
    uint32_t pair_cnt   : 30;
    uint32_t glyph_ids_size : 2;    /*0: `glyph_ids` is stored as `uint8_t`; 1: as `uint16_t`*/
} lv_font_fmt_txt_kern_pair_t;


/** More complex but more optimal class based kern value storage*/
typedef struct {
    /*To get a kern value of two code points:
          1. Get the `glyph_id_left` and `glyph_id_right` from `lv_font_fmt_txt_cmap_t
          2. Get the class of the left and right glyphs as `left_class` and `right_class`
              left_class = left_class_mapping[glyph_id_left];
              right_class = right_class_mapping[glyph_id_right];
          3. value = class_pair_values[(left_class-1)*right_class_cnt + (right_class-1)]
        */

    const int8_t * class_pair_values;    /*left_class_cnt * right_class_cnt value*/
    const uint8_t * left_class_mapping;   /*Map the glyph_ids to classes: index -> glyph_id -> class_id*/
    const uint8_t * right_class_mapping;  /*Map the glyph_ids to classes: index -> glyph_id -> class_id*/
    uint8_t left_class_cnt;
    uint8_t right_class_cnt;
} lv_font_fmt_txt_kern_classes_t;

bool lv_font_get_glyph_dsc_fmt_txt(const lv_font_t * font, lv_font_glyph_dsc_t * dsc_out, uint32_t unicode_letter, uint32_t unicode_letter_next);
const uint8_t * lv_font_get_bitmap_fmt_txt(const lv_font_t * font, uint32_t letter);

#endif /*LVGL_H*/
