#ifndef CHART_H
#define	CHART_H

#include "GUI_GSP.h"
enum
{
    GREED_NO = 0,
    GREED_LINE,
    GREED_DOTE = 3,
    GREED_DASH,
};

// Режимы отображения Chart
enum
{
    CHART_MODE_GRAPH = 0,    // Режим графика (по умолчанию)
    CHART_MODE_HISTOGRAM_DYNAMIC,    // Режим динамической гистограммы (частота попадания значений в диапазоны)
    CHART_MODE_BAR_SHIFT,    // Режим столбчатой диаграммы со сдвигом (фиксированная ширина столбцов)
};

// Виджет Chart
#define CHART_MARCERS   2
typedef struct
{
    bool            Enable;             // включение
    short           Pos;          // положение маркера
    short           H;                  // Высота маркера
    WORD_COLOR      Color;              // цвет маркера
    bool            Line;               // линия маркера
    short           LineType;           // тип линии (SOLID_LINE, DOTTED_LINE, DASHED_LINE)
} _Chart_Marker;

typedef struct
{
    bool            Enable;             // включение
    short           PosX;                // положение строба
    short           PosY;                // положение строба
    short           Len;                  // Высота строба
    short           H;                  //  Толщина строба
    WORD_COLOR      Color;              // цвет строба
} _Chart_Strob;

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
    WORD_COLOR      BackColor;        // цвет основы
    WORD_COLOR      BorderColor;      // Цвет юордюра
    bool            Visible;          // признак видимости
    bool            Fill;               // Заполненность
    short           Greed;            // тип сетки
    WORD_COLOR      GreedColor;        //   Цвет сетки
    WORD_COLOR      CurveColor;        //   Цвет кривой
    short           ThincCurve;         // Тощтна кривой
    short           N_X;               // Число делений по горизонтали
    short           N_Y;               // Число делений по вертикали
    short          *Data;              // указатель на данные  для отображеня
    short           LenData;            // Число данных для отображения
    short           *DataView;           // указатель на массив готовый для отображения ( упакованные данные)
    short           Max;                // максимальное значение
    short           Min;                // минимальное
    _Chart_Marker   Marker[CHART_MARCERS];// Маркеры 
    _Chart_Strob    Strob[CHART_MARCERS];// Стробы 
    
    void (*packaging)(short *In_Data, int len_in, short *Out_Data, int len_out); // указатель на процедурц упаковки
    
    short           Mode;               // Режим отображения (CHART_MODE_GRAPH, CHART_MODE_HISTOGRAM_DYNAMIC или CHART_MODE_BAR_SHIFT)
    short           *BarData;           // Массив данных для столбчатой диаграммы (размер = HistogramBins)
    short           BarDataCount;       // Текущее количество данных в BarData
    short           HistogramBins;      // Количество bins (столбиков) в гистограмме (по умолчанию = lenx)
    bool            AutoBins;           // Автоматический расчет количества bins
    short           MinBarWidth;        // Минимальная ширина столбика в пикселях (для AutoBins)
    short           WidgetAlign;        // Выравнивание виджета относительно Screen (GSP_WIDGET_ALIGN_*)
    bool            InvertY;            // Инверсия оси Y: true = Min вверху, Max внизу (для BSCAN)
    bool            AutoRange;          // Автоматический расчет Min/Max из входящих данных
    short           AutoMin;            // Автоматически вычисленное минимальное значение
    short           AutoMax;            // Автоматически вычисленное максимальное значение
    short           AutoRangeMargin;    // Отступ от краев при автоматическом расчете (в процентах, 0-50)
} _GSP_Chart;


_GSP_Chart *Chart_Crate(_GSP_Screen *Screen, int x, int y, int lenx, int leny, int ID);
void ChartAddData(_GSP_Chart *Chart, short *Data, int len);
void DrawingChart(_GSP_Chart *Chart);
void ChartSetProcPack(_GSP_Chart *Chart, void (*proc)(short *, int, short *, int));
void ChartUpdateViewData(_GSP_Chart *Chart, void (*proc)(short *, int, short *, int));
int  ChartGetLenX(_GSP_Chart *Chart);
void ChartSetMode(_GSP_Chart *Chart, short mode);
void ChartAddBarData(_GSP_Chart *Chart, short value);
void ChartSetWidgetAlign(_GSP_Chart *Chart, int align);
void ChartSetPos(_GSP_Chart *Chart, int x, int y);
void ChartSetSize(_GSP_Chart *Chart, int lenx, int leny);
void ChartSetInvertY(_GSP_Chart *Chart, bool invert);
void ChartSetAutoRange(_GSP_Chart *Chart, bool enable, short margin_percent);
void ChartSetMinMax(_GSP_Chart *Chart, short min, short max);
void ChartResetHistogram(_GSP_Chart *Chart);
void ChartSetHistogramBins(_GSP_Chart *Chart, short num_bins);
void ChartSetAutoBins(_GSP_Chart *Chart, bool enable, short min_bar_width);

#endif	/* LABEL_H */

