#ifndef RULER_H
#define	RULER_H

#include "GUI_GSP.h"
#include "GUI_GSP/Fonts/LVGL/GSP_lvgl.h"

#define Y_NAME_SCALE        50
#define SEP_RULER_H         0xff
#define SEP_RULER_M         0x3f
#define SEP_RULER_S         0x1f
#define N_STEPS_RULER        12           // число дискретов
#define MAX_DISCRET_RULER       3           // максимальное число делений линейки 
#define MIN_DISCRET_RULER       2           // минимальное число делений линейки 

// Р’РёРґР¶РµС‚ Ruler
typedef struct
{
    int Steps[N_STEPS_RULER];                   // дискретность больших делений линейки в мм*10
    int Step;                                   // тукущая дискретность в мм*10
    int Start;                                  //  Начальное значение шкалы в мм*10
    int Len;                                    // длинна шкалы мм*10
    int   N;                                    // Число больших делений линейки
	unsigned char *Flags;                      // Признаки рисок размер должен совпадать с экраном
    char Name_Div[MAX_DISCRET_RULER+5][10];     // Подписи к шкалам линейки
    char Name[2][MAX_DISCRET_RULER];            // Подписи к общей шкале
    bool Update;                                // Признак обновления
} R_Param_;

typedef struct
{
    unsigned char   Type;             // Тип виджета
    int             ID;               // ID виджета
    int             x;                // координата x леыого верхнего угла
    int             y;                // координата y леыого верхнего угла
    int             lenx;             // длинна виджетп
    int             leny;             // высота виджета
    WORD_COLOR      BackColor;        // цвет основы
    WORD_COLOR      TextColor;        // Цвет текста
    lv_font_t       *Font;            // Отображаемый шрифт 
    bool            Visible;          // признак видимости
    R_Param_        Params;            // параметры линейки 
} _GSP_Ruler;


_GSP_Ruler *RulerCrate(_GSP_Screen *Screen, int x, int y, int lenx, int leny, int ID);
void RulerDrawing(_GSP_Ruler *Ruler);
void RulerReCalc(_GSP_Ruler *Ruler);
void RulerDelete(_GSP_Ruler *Ruler);
void RulerSetScale(_GSP_Ruler *Ruler, int start, int len);

#endif	/* LABEL_H */

