#ifndef GUI_GSP_H
#define	GUI_GSP_H

#include "Config/GUI_GSP_Config.h"
#include "../GUI_GSP/Fonts/LVGL/GSP_lvgl.h"
#include <stdlib.h>

#define VERSION_GUIGSP      "1.2"

extern lv_font_t GSP_Calibri_10;
extern lv_font_t Days_one_8;
extern lv_font_t GSP_Arial_16;


enum
{
    GSP_OK = 0,
    GSP_NO_WIDGET, 
    GSP_ERR_WIDGET
};
// виджеты
enum
{
    GSP_SCREEN = 0,
    GSP_LABEL,
    GSP_CHART,
    GSP_RULER,
    GSP_TABLE,
    GSP_IMAGE,
    GSP_LINE,
    GSP_BAR,
    GSP_SPINNER,
    GSP_PANEL,
    GSP_TIMER,
    GSP_LIST,
    GSP_MESSAGE,
    GSP_KEYBOARD,
};
// атрибуту lable 
enum
{
    GSP_BLINC = 1,
    GSP_SHIFT = 2,
    GSP_ENABLE = 4,
};

// режимы бегущей строки (для Label и List)
enum
{
    GSP_SHIFT_CICL = 0,      // циклическая прокрутка
    GSP_SHIFT_INFINITY,      // бесконечная прокрутка
};

// флаги для сторон рамки
#define GSP_BORDER_TOP    0x01    // верхняя сторона
#define GSP_BORDER_BOTTOM 0x02    // нижняя сторона
#define GSP_BORDER_LEFT   0x04    // левая сторона
#define GSP_BORDER_RIGHT  0x08    // правая сторона
#define GSP_BORDER_ALL    0x0F    // все стороны

// Выравнивание виджета относительно Screen (для Label)
#define GSP_WIDGET_ALIGN_TOP_LEFT      0   // левый верхний угол
#define GSP_WIDGET_ALIGN_TOP_CENTER    1   // верхний центр
#define GSP_WIDGET_ALIGN_TOP_RIGHT     2   // правый верхний угол
#define GSP_WIDGET_ALIGN_MIDDLE_LEFT   3   // левый центр
#define GSP_WIDGET_ALIGN_MIDDLE_CENTER 4   // центр
#define GSP_WIDGET_ALIGN_MIDDLE_RIGHT  5   // правый центр
#define GSP_WIDGET_ALIGN_BOTTOM_LEFT   6   // левый нижний угол
#define GSP_WIDGET_ALIGN_BOTTOM_CENTER 7   // нижний центр
#define GSP_WIDGET_ALIGN_BOTTOM_RIGHT  8   // правый нижний угол

// структура Screen
typedef struct
{
    unsigned char   Type;             // тир
    int             ID;               // ID°
    short           x;                // координата x леыого верхнего угла
    short           y;                // координата y леыого верхнего угла
    short           lenx;             // длинна виджетп
    short           leny;             // высота виджета
    WORD_COLOR      BackColor;        // цвет основы
    short           N;                // Число подключенных виджетов 
    short           WidthBorder;      // ширина бордюра
    WORD_COLOR      BorderColor;      // Цвет юордюра

    int             **ListWidgets;   // Масив указателей подключенных виждетов
    void            (*KeyHandler)(void);  // Указатель на процедуру обработки кнопок (NULL по умолчанию)
    
#ifdef USED_TIMER
    // Поля для привязанных таймеров
    short           N_Timers;         // Количество привязанных таймеров
    int             **ListTimers;     // Массив указателей на привязанные таймеры
#endif
} _GSP_Screen;

#ifdef USED_TIMER
// структура Обслуживания таймеров
typedef struct
{
    unsigned char   Type;             // тир
    int             ID;               // ID°
    int             N;                // Число подключенных таймеров 
    int             **ListTimers;   // Масив указателей подключенные таймеры
} __attribute__((packed)) _GSP_Timers;
#endif


extern unsigned int Timer_GUI;
extern _GSP_Screen *Active_Screen;			// указатель на актичное окно


void GUI_TimerClock(int ms);
_GSP_Screen *Crate_Screen(int x, int y, int lenx, int leny, int ID);
void Set_Active_Screen(_GSP_Screen *Screen);
void ScreenSetKeyHandler(_GSP_Screen *Screen, void (*KeyHandler)(void));

void GUI_Run(void);
int GUI_DeleteWidget(_GSP_Screen *Screen, void *Widget);
int GUI_DeleteScreen(_GSP_Screen *Screen);

void IncrementListWidgets(_GSP_Screen *Screen, void *Widget);

#ifdef USED_TIMER
int GUI_DeleteTimer(void *Timer);
void IncrementListTimers(void *Timer);
#endif
void GUI_SetBright(unsigned short br);
void GUI_Delay(int ms);
_GSP_Screen *Get_Active_Screen(void);
void GUI_GSP_Init(void);

// Механизм безопасного переключения экранов
typedef void (*ScreenCreateFunc)(void);
void GUI_SwitchScreen(ScreenCreateFunc create_func);
bool GUI_IsScreenSwitchPending(void);  // Проверка, запланировано ли переключение экрана

// Вспомогательная функция для вычисления координат виджета с учетом выравнивания
void CalculateWidgetPosition(_GSP_Screen *Screen, int *x, int *y, int lenx, int leny, int widget_align);

// общая функция для обработки бегущей строки
void GSP_RunShift(const char *text, lv_font_t *font, int available_width, int mode_shift,
                  short *old_let_text, short *dx, bool *sh, short *delay, int letter_spacing);

// Подключение заголовочных файлов виджетов
#include "GSP_Label.h"         // Базовый виджет (используется в Table.h, поэтому должен быть первым)
#include "GSP_List.h"
#include "GSP_Message.h"
#include "GSP_Image.h"
#include "GSP_Chart.h"
#include "GSP_Ruler.h"
#include "GSP_ProgressBar.h"
#include "GSP_Spinner.h"
#include "GSP_Panel.h"
#include "GSP_PolyLine.h"
#include "GSP_Table.h"         // Использует Label.h, поэтому после него
#include "GSP_Timer.h"     // Таймеры
#include "GSP_Keyboard.h"  // Виджет клавиатуры


#endif	/* GUI_GSP_H */

