/**
 * @file GSP_Driver.h
 * @brief Интерфейсы драйверов для переносимой библиотеки GUI_GSP.
 *        Библиотека работает с разными дисплеями, FLASH и клавиатурами
 *        через регистрацию драйверов при инициализации.
 */
#ifndef GSP_DRIVER_H
#define GSP_DRIVER_H

#include <stddef.h>
#include <stdbool.h>

/* Цвета BLACK, WHITE, TRANSPARENT — в GSP_Config.h */

/* Константы линий */
#define GSP_SOLID_LINE   0
#define GSP_DOTTED_LINE  1
#define GSP_DASHED_LINE  4
#define GSP_NORMAL_LINE  0
#define GSP_THICK_LINE   1

/* Выравнивание текста */
#define GSP_ALLIGN_TEXT_LEFT   0
#define GSP_ALLIGN_TEXT_RIGHT  1
#define GSP_ALLIGN_TEXT_CENTER  2

/* Типы и константы для совместимости; рисование — прямыми вызовами функций GSP_Driver */
#ifndef __WORD_COLOR_DEFINED__
#define __WORD_COLOR_DEFINED__
typedef unsigned char WORD_COLOR;
#endif
typedef short SHORT;
typedef unsigned short WORD;
#ifndef SOLID_LINE
#define SOLID_LINE   GSP_SOLID_LINE
#endif
#ifndef DOTTED_LINE
#define DOTTED_LINE  GSP_DOTTED_LINE
#endif
#ifndef NORMAL_LINE
#define NORMAL_LINE  GSP_NORMAL_LINE
#endif
#ifndef THICK_LINE
#define THICK_LINE   GSP_THICK_LINE
#endif
#ifndef DASHED_LINE
#define DASHED_LINE  GSP_DASHED_LINE
#endif
#ifndef ALLIGN_TEXT_LEFT
#define ALLIGN_TEXT_LEFT   GSP_ALLIGN_TEXT_LEFT
#endif
#ifndef ALLIGN_TEXT_RIGHT
#define ALLIGN_TEXT_RIGHT  GSP_ALLIGN_TEXT_RIGHT
#endif
#ifndef ALLIGN_TEXT_CENTER
#define ALLIGN_TEXT_CENTER GSP_ALLIGN_TEXT_CENTER
#endif

/* ==================== Драйвер дисплея (только железо) ==================== */
typedef struct GSP_DisplayDriver GSP_DisplayDriver;

struct GSP_DisplayDriver
{
    /* Обновление экрана (вывод буфера на дисплей) */
    void (*Refresh)(void);
    /* Инициализация LCD */
    void (*Init)(void);
};

/* ==================== Чтение памяти (FLASH/внешняя память) ==================== */
/* Для загрузки шрифтов и данных. ext_data - указатель на структуру проекта (EXTDATA и т.п.) */
typedef unsigned short (*GSP_MemoryReadFn)(void *ext_data, unsigned int offset, unsigned int nCount, void *buffer);

/* ==================== Потоковое чтение (для JSON) ==================== */
/* Прямые вызовы по указателям, без ctx. read_byte возвращает байт (0..255) или -1 при EOF */
typedef struct
{
    void (*start)(long address);
    unsigned char  (*read_byte)(void);
    void (*stop)(void);
} GSP_StreamReader;

/* ==================== Клавиатура (прямые указатели на процедуры) ==================== */
typedef struct
{
	int (*key_poll)(void);
	int (*read_key)(void);
} GSP_Keyboard;

/* ==================== Аллокатор (типы для полей GSP_Hardware) ==================== */
typedef void* (*GSP_AllocFn)(size_t size);
typedef void  (*GSP_FreeFn)(void *ptr);
typedef int (*GSP_KeyPollFn)(void);

/* ==================== Единая структура железа: все обращения к железу через её поля ==================== */
typedef struct
{
	GSP_DisplayDriver  display;
	GSP_StreamReader   stream_reader;
	GSP_Keyboard       keyboard;
	GSP_MemoryReadFn   memory_read;
	GSP_AllocFn        alloc;
	GSP_FreeFn         free;
} GSP_Hardware;

extern GSP_Hardware GSP_HW;

/* Доступ к железу только через GSP_HW; макросы для краткости (прямые поля структуры) */
//#define GSP_GetDisplayDriver()       (&(GSP_HW.display))  
#define GSP_GetStreamReader()        (&(GSP_HW.stream_reader))
#define GSP_GetKeyPollCallback()      (GSP_HW.keyboard.key_poll)
#define GSP_GetReadKeyCallback()      (GSP_HW.keyboard.read_key)
#define GSP_GetMemoryReadCallback()   (GSP_HW.memory_read)
#define GSP_GetAlloc()                (GSP_HW.alloc)
#define GSP_GetFree()                 (GSP_HW.free)
/* Инициализация библиотеки (использует GSP_HW) */
int GSP_Init(void);

/* Опрос клавиатуры через GSP_HW.keyboard.key_poll */
int GSP_KeyPoll(void);

/* Чтение кода клавиши через GSP_HW.keyboard.read_key (оболочка над указателем) */
int GSP_ReadKey(void);

/* ==================== Состояние рисования (для макросов) ==================== */
#ifndef GSP_DRIVER_BUILD
extern unsigned char _color;
extern short _clipRgn, _clipLeft, _clipTop, _clipRight, _clipBottom;
extern short _cursorX, _cursorY;
extern short _lineType;
extern unsigned char _lineThickness;
#endif

/* ==================== Макросы вместо функций (размеры, цвет, клип, курсор, линия) ==================== */
#ifndef SCREEN_HOR_SIZE
#define SCREEN_HOR_SIZE 128
#endif
#ifndef SCREEN_VER_SIZE
#define SCREEN_VER_SIZE 64
#endif
#define GetMaxX()           ((short)(SCREEN_HOR_SIZE - 1))
#define GetMaxY()           ((short)(SCREEN_VER_SIZE - 1))
#define SetColor(c)         (_color = (unsigned char)(c))
#define GetColor()          (_color)
#define MoveTo(x,y)         (_cursorX = (short)(x), _cursorY = (short)(y))
#define GetX()              (_cursorX)
#define GetY()              (_cursorY)
#define SetClipRgn(l,t,r,b) (_clipLeft = (short)(l), _clipTop = (short)(t), _clipRight = (short)(r), _clipBottom = (short)(b))
#define SetClip(e)          (_clipRgn = (short)(e))
#define GetClipLeft()       (_clipLeft)
#define GetClipRight()      (_clipRight)
#define GetClipTop()        (_clipTop)
#define GetClipBottom()     (_clipBottom)
#define SetLineType(t)      (_lineType = (short)(t))
#define SetLineThickness(t) (_lineThickness = (unsigned char)(t))

/* ==================== Функции дисплея (рисование в буфер) ==================== */
void PutPixel(short x, short y);
void PutPixelXOR(short x, short y);
unsigned short GetPixel(short x, short y);
void Bar(short left, short top, short right, short bottom);
unsigned short Line(short x1, short y1, short x2, short y2);
unsigned short Bevel(short x1, short y1, short x2, short y2, short rad);
unsigned short FillBevel(short x1, short y1, short x2, short y2, short rad);
void FastV_Line(int x, int y0, int y1, int interval);
void FastH_Line(int y, int x0, int x1, int interval);
unsigned short PutImage(short left, short top, void *bitmap, unsigned char stretch);
short GetImageWidth(void *bitmap);
short GetImageHeight(void *bitmap);
void GSP_Display_Refresh(void);
void GSP_Display_Clear(void);
void GSP_Display_Init(void);

#endif /* GSP_DRIVER_H */
