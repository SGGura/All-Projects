#ifndef GSP_CONFIG_H
#define	GSP_CONFIG_H

#include <stdlib.h>

/* ==================== Размеры и ориентация дисплея (только здесь) ==================== */
#define ORIENT_NORMAL       0
#define ORIENT_REVERS       1

#ifndef SCREEN_HOR_SIZE
#define SCREEN_HOR_SIZE    128
#endif
#ifndef SCREEN_VER_SIZE
#define SCREEN_VER_SIZE    64
#endif

#define SIZE_DISP           ((SCREEN_HOR_SIZE * SCREEN_VER_SIZE) / 8)

#define CLIP_DISABLE        0
#define CLIP_ENABLE         1

/* ==================== Опции дисплея (из бывшего GraphicsConfig) ==================== */
#define DISPLAY_CONTROLLER          SH1107
#define DISP_HOR_RESOLUTION         128
#define DISP_VER_RESOLUTION         64
#define COLOR_DEPTH                 1
#define DISP_ORIENTATION            0
#define USE_NONBLOCKING_CONFIG
#define USE_FONT_FLASH
#define USE_BITMAP_EXTERNAL

/* ==================== Графика: версия, цвета, типы (из бывшего Graphics) ==================== */
#define GRAPHICS_LIBRARY_VERSION    0x0306
#define GFX_FONT_SPACE              const

#ifndef __WORD_COLOR_DEFINED__
#define __WORD_COLOR_DEFINED__
typedef unsigned char WORD_COLOR;
#endif

#define TRANSPARENT         0x55
#define BLACK               0x00
#define WHITE               0xff

typedef enum
{
    MEM_FLASH   = 0,
    MEM_EXTERNAL= 1,
    MEM_RAM     = 2,
    MEM_VIDEOBUF= 3
} TYPE_MEMORY;

typedef struct
{
    TYPE_MEMORY type;
    unsigned short ID;
    unsigned long  address;
} EXTDATA;

unsigned short ExternalMemoryCallback(EXTDATA *memory, unsigned int offset, unsigned int nCount, void *buffer);

typedef struct
{
    unsigned char compression;
    unsigned char colorDepth;
    short   height;
    short   width;
} BITMAP_HEADER;

#define FLASH_BYTE          const unsigned char
#define FLASH_WORD          const unsigned short

typedef struct
{
    TYPE_MEMORY type;
    FLASH_BYTE  *address;
} BITMAP_FLASH;

#define IMAGE_EXTERNAL      EXTDATA
#define EXTERNAL            0x0001
#define IMAGE_JPEG          0x0000
#define IMAGE_MBITMAP       0x0000
#define COMP_NONE           0x0000

#define RGB565CONVERT(red, green, blue)  (unsigned short) (((red >> 3) << 11) | ((green >> 2) << 5) | (blue >> 3))
#define RGB332CONVERT(red, green, blue) (unsigned char) ((red & 0xE0) | ((green & 0xE0) >> 3) | (blue >> 6))

/* ==================== GSP: драйвер, шрифты, таймер, клавиатура ==================== */
void SetBright(short light);

#include "../Drivers/GSP_Driver.h"
#include "../Fonts/Fonts.h"
#include "../../SYS_Timer.h"
#include "../../Keyboard.h"

/* GetMaxX(), GetMaxY() — прямые вызовы функций из GSP_Driver.h */

#define	Refresh_GSP()   GSP_Display_Refresh()
#define	Keyboard_GSP    GSP_KeyPoll

#define MAX_WIDGETS 15
#define GUI_REFR_PERIOD 67

#define DEFAULT_COLOR_SCREEN    WHITE
#define DEFAULT_COLOR_BACK      WHITE
#define DEFAULT_COLOR_TEXT      BLACK
#define DEFAULT_COLOR_ACTIVE    BLACK
#define DEFAULT_COLOR_BAR       BLACK

#define USED_TABLE
#define USED_LABEL
#define USED_CHART
#define CHART_ENABLE_GRAPH 0
#define CHART_ENABLE_HISTOGRAM 1
#define CHART_ENABLE_BAR_SHIFT 0
#define USED_IMAGE
#define USED_PROGRESSBAR
#define USED_SPINNER
#define USED_PANEL
#define USED_LINE
#define USED_TIMER
#define USED_LIST
#define USED_MESSAGE
#define USED_KEYBOARD

#define DEFAULT_FONT  &lv_font_unscii_8
#define DEFAULT_TEXT  "Text"

#endif	/* GSP_CONFIG_H */
