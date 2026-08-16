#ifndef LINE_H
#define	LINE_H

#include "GUI_GSP/Fonts/LVGL/GSP_lvgl.h"
#include "GUI_GSP.h"

/*
// структура точки
typedef struct
{
    int             x;                
    int             y;                
}  _GSP_Point;
*/

// Виджет Line
typedef struct
{
    unsigned char   Type;             // Тип виджета
    int             ID;               // ID виджета
    short           N;                // Чисто точек координат  
    short           *points;            // указатель на координаты точек перегиба       
    short           Width;              // ширина
    short           Style;          // тип. спшная или пунктир
    WORD_COLOR      Color;              // цвет
    short           Time_Blinc;       // Период мигания в мс
    bool            Visible;            // признак видимости
    // служебные параметры
    int             old_timer;       // ИСПРАВЛЕНО: изменено с short на int для избежания переполнения
    bool            BlincTogle;      // флаг мигания
} _GSP_PolyLine;


_GSP_PolyLine *Crate_PolyLine(_GSP_Screen *Screen, int n, int ID);
void PolyLineAddPoint(_GSP_PolyLine *PolyLine, int n, int x, int y);
void DrawingPolyLine(_GSP_PolyLine *Line);
void PolyLineRunBlinc(_GSP_PolyLine *Line);


#endif	/* LINE_H */

