#ifndef PROGRESSBAR_H
#define	PROGRESSBAR_H

#include "GUI_GSP/Fonts/LVGL/GSP_lvgl.h"
#include "GUI_GSP.h"

enum
{
    BAR_HOR = 0,
    BAR_VERT,
    
};
// Максимальное количество маркеров на ProgressBar
#define PROGRESSBAR_MAX_MARKERS 4

// Виджет ProgressBar
typedef struct
{
    unsigned char   Type;             // Тип виджета
    int             ID;               // ID виджета
    short           x;                // координата x левого верхнего угла (абсолютная)
    short           y;                // координата y левого верхнего угла (абсолютная)
    short           rel_x;            // относительная координата x (для пересчета при смене выравнивания)
    short           rel_y;            // относительная координата y (для пересчета при смене выравнивания)
    short           lenx;             // длинна виджетп
    short           leny;             // высота виджета
    short           WidthBorder;      // ширина бордюра
    short           R;                // Радиус закругления углов
    short           Pos;              // текущая позиция  
    short           Pos_Min;          // минимальная позиция
    short           Pos_Max;          // максимальная позиция 
    short           Pos_Start;       // начальное значение для двустороннего режима (левое значение)
    bool            BidirectionalMode; // Режим двустороннего бара (полоска от Pos_Start до Pos)
    short           Orient;            // Ориентация  
    short           WidgetAlign;       // Выравнивание виджета относительно Screen (GSP_WIDGET_ALIGN_*)
    WORD_COLOR      BackColor;        // цвет основы
    WORD_COLOR      BarColor;         // Цвет столбика
    WORD_COLOR      BorderColor;      // Цвет юордюра
    bool            Visible;          // признак видимости

    // Маркеры (до PROGRESSBAR_MAX_MARKERS штук)
    short           MarkerValues[PROGRESSBAR_MAX_MARKERS]; // значения маркеров в тех же единицах, что и Pos
    bool            MarkerPassed[PROGRESSBAR_MAX_MARKERS]; // признак: 1, если Pos превысил значение маркера
    short           MarkerCount;                           // текущее количество маркеров
} _GSP_ProgressBar;


_GSP_ProgressBar *Crate_ProgressBar(_GSP_Screen *Screen, int x, int y, int lenx, int leny, int ID);
void DrawingBar(_GSP_ProgressBar *Bar);
void ProgressBarSetWidgetAlign(_GSP_ProgressBar *Bar, int align);
void ProgressBarSetPos(_GSP_ProgressBar *Bar, int x, int y);
void ProgressBarSetSize(_GSP_ProgressBar *Bar, int lenx, int leny);
void ProgressBarSetValue(_GSP_ProgressBar *Bar, int value);
void ProgressBarSetRange(_GSP_ProgressBar *Bar, int min, int max);
void ProgressBarIncrement(_GSP_ProgressBar *Bar, int increment);
int ProgressBarGetValue(_GSP_ProgressBar *Bar);

// Работа с маркерами
void ProgressBarClearMarkers(_GSP_ProgressBar *Bar);
bool ProgressBarAddMarker(_GSP_ProgressBar *Bar, int value);
bool ProgressBarRemoveMarker(_GSP_ProgressBar *Bar, int index);
bool ProgressBarGetMarkerState(_GSP_ProgressBar *Bar, int index);

// Работа с двусторонним режимом
void ProgressBarSetBidirectionalMode(_GSP_ProgressBar *Bar, bool enable);
void ProgressBarSetStartValue(_GSP_ProgressBar *Bar, int start_value);

#endif	/* PROGRESSBAR_H */

