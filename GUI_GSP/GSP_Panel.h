#ifndef PANEL_H
#define	PANEL_H

#include "GUI_GSP/Fonts/LVGL/GSP_lvgl.h"
#include "GUI_GSP.h"

// Виджет Panel — прямоугольник с контуром или без, задаваемым цветом фона, может иметь скруглённые углы
typedef struct
{
    unsigned char   Type;             // Тип виджета (GSP_PANEL)
    int             ID;               // ID виджета
    short           x;                // координата x левого верхнего угла
    short           y;                // координата y левого верхнего угла
    short           rel_x;            // относительная координата x (для пересчёта при смене выравнивания)
    short           rel_y;            // относительная координата y
    short           lenx;             // ширина виджета
    short           leny;             // высота виджета
    short           WidgetAlign;      // выравнивание (GSP_WIDGET_ALIGN_*)
    WORD_COLOR      BackColor;        // цвет фона (TRANSPARENT = не заливать)
    WORD_COLOR      BorderColor;      // цвет контура
    bool            ShowBorder;       // рисовать контур (true) или нет (false)
    short           Radius;           // радиус скругления углов (0 = прямые углы)
    bool            Visible;          // признак видимости
} _GSP_Panel;

#ifdef USED_PANEL
_GSP_Panel *CreatePanel(_GSP_Screen *Screen, int x, int y, int lenx, int leny, int ID);
void DrawingPanel(_GSP_Panel *Panel);
void PanelSetPosition(_GSP_Panel *Panel, int x, int y);
void PanelSetSize(_GSP_Panel *Panel, int lenx, int leny);
void PanelSetBackColor(_GSP_Panel *Panel, WORD_COLOR color);
void PanelSetBorder(_GSP_Panel *Panel, bool show);           // true = рисовать контур
void PanelSetBorderColor(_GSP_Panel *Panel, WORD_COLOR color);
void PanelSetRadius(_GSP_Panel *Panel, int radius);         // 0 = прямые углы
void PanelSetVisible(_GSP_Panel *Panel, bool visible);
void PanelSetWidgetAlign(_GSP_Panel *Panel, int align);
#endif

#endif	/* PANEL_H */
