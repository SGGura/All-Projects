#ifndef SPINNER_H
#define	SPINNER_H

#include "GUI_GSP/Fonts/LVGL/GSP_lvgl.h"
#include "GUI_GSP.h"

// Виджет Spinner — два вида: ряд квадратов с градацией размеров / сетка 2x2 (один заполнен, остальные контур)
#define SPINNER_NUM_BOXES  4
#define SPINNER_GAP        1
#define SPINNER_CIRCLE_BOXES 4

#define GSP_SPINNER_STYLE_ROW    0  // ряд: большой, 1/2, 1/4, 1/8
#define GSP_SPINNER_STYLE_CIRCLE 1  // сетка 2x2: один заполнен, остальные только контур

typedef struct
{
    unsigned char   Type;             // Тип виджета (GSP_SPINNER)
    int             ID;               // ID виджета
    short           x;                // координата x левого верхнего угла ограничивающего прямоугольника
    short           y;                // координата y левого верхнего угла
    short           rel_x;            // относительная координата x (для пересчёта при смене выравнивания)
    short           rel_y;            // относительная координата y
    short           lenx;             // ширина виджета
    short           leny;             // высота виджета
    short           WidgetAlign;      // выравнивание (GSP_WIDGET_ALIGN_*)
    short           style;            // GSP_SPINNER_STYLE_ROW или GSP_SPINNER_STYLE_CIRCLE
    short           box_big;          // размер квадрата (пиксели); для ROW — большой, остальные 1/2,1/4,1/8
    short           gap_circle;       // зазор между квадратами в пикселях (только для STYLE_CIRCLE, сетка 2x2)
    bool            circle_show_empty; // для сетки 2x2: true = рисовать контур пустых, false = только заполненный
    int             period_ms;        // период анимации (мс), как у Image
    short           phase;            // индекс текущего «активного» квадрата (0..N-1)
    unsigned int    last_timer_update; // момент последнего обновления фазы (Timer_GUI), как у Image
    WORD_COLOR      Color;            // цвет квадратов
    WORD_COLOR      BackColor;        // цвет фона (TRANSPARENT = не заливать)
    bool            Visible;          // признак видимости
} _GSP_Spinner;

#ifdef USED_SPINNER
_GSP_Spinner *CreateSpinner(_GSP_Screen *Screen, int x, int y, int box_size, int ID);  /* по умолчанию режим бегающих квадратов */
void DrawingSpinner(_GSP_Spinner *Spinner);
void SpinnerSetPosition(_GSP_Spinner *Spinner, int x, int y);
void SpinnerSetSize(_GSP_Spinner *Spinner, int box_big);
void SpinnerSetStyleCircle(_GSP_Spinner *Spinner, int gap_pixels);   /* переключить на сетку 2x2, зазор в пикселях */
void SpinnerSetPeriod(_GSP_Spinner *Spinner, int period_ms);
void SpinnerSetVisible(_GSP_Spinner *Spinner, bool visible);
void SpinnerSetColor(_GSP_Spinner *Spinner, WORD_COLOR color);
void SpinnerSetBackColor(_GSP_Spinner *Spinner, WORD_COLOR color);  // TRANSPARENT — прозрачный фон
void SpinnerSetCircleShowEmpty(_GSP_Spinner *Spinner, bool show);   // сетка 2x2: false = только заполненный квадрат
void SpinnerSetWidgetAlign(_GSP_Spinner *Spinner, int align);
#endif

#endif	/* SPINNER_H */
