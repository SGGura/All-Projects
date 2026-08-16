#ifndef TABLE_H
#define	TABLE_H


#include "GUI_GSP/Fonts/LVGL/GSP_lvgl.h"
#include "GUI_GSP.h"
#include "GSP_Label.h"

// Структура элемента 
#define _Table_Cell _GSP_Label
// Виджет Table
typedef struct
{
    unsigned char   Type;             // Тип виджета
    int             ID;               // ID виджета
    short           x;                // координата x леыого верхнего угла (абсолютная)
    short           y;                // координата y леыого верхнего угла (абсолютная)
    short           rel_x;            // относительная координата x (для пересчета при смене выравнивания)
    short           rel_y;            // относительная координата y (для пересчета при смене выравнивания)
    short           lenx;             // длинна виджетп
    short           leny;             // высота виджета
    short           *lencoll;         // eказатель на массив длин каждой колонки
    short           WidthBorder;      // ширина бордюра
    short           R;                // Радиус закругления углов
    bool            Visible;           // Видимость виджета  
    lv_font_t       *Font;            // Отображаемый шрифт 
    WORD_COLOR      BackColor;        // цвет основа всего виджкта
    WORD_COLOR      BorderColor;      // Цвет юордюра
    short           Rows;             // Число строк
    short           Colls;            // Число колонок     
    int             *Cells;           // Указатель на массив элементов
    short           SelectedRow;      // Выбранная строка (-1 = нет выбора)
    short           SelectedColl;     // Выбранная колонка (-1 = нет выбора)
    WORD_COLOR      SelectedBackColor; // Цвет фона выбранной ячейки
    WORD_COLOR      SelectedTextColor; // Цвет текста выбранной ячейки
    short           WidgetAlign;      // Выравнивание виджета относительно Screen (GSP_WIDGET_ALIGN_*)
} _GSP_Table;


_GSP_Table *Crate_Table(_GSP_Screen *Screen, int x, int y, int lenx, int leny, int ID);
void DrawingTable(_GSP_Table *Table);
void TableSetText(_GSP_Table *Table, int row, int coll, const char * text);
void TableSetTextFmt(_GSP_Table *Table, int row, int coll, const char *format, ...);
void TableSetColorBK(_GSP_Table *Table, int row, int coll, WORD_COLOR color);
void TableSetColorText(_GSP_Table *Table, int row, int coll, WORD_COLOR color);
void TableSetTextBlinc(_GSP_Table *Table, int row, int coll, int time);
void TableSetTextShift(_GSP_Table *Table, int row, int coll, bool en);
void TableAddColl(_GSP_Table *Table, int len);
void TableDelColl(_GSP_Table *Table, int col);
void TableInsertColl(_GSP_Table *Table, int len, int coll);
void TableInsertRow(_GSP_Table *Table, int row);
void TableDelRow(_GSP_Table *Table, int row);
void TableSetBorder(_GSP_Table *Table, int row, int coll, WORD_COLOR color, int Width);
void TableSetFontCell(_GSP_Table *Table, int row, int coll, void *font);
void TableSetSelected(_GSP_Table *Table, int row, int coll);
void TableSetSelectedColors(_GSP_Table *Table, WORD_COLOR back_color, WORD_COLOR text_color);
void TableGetSelected(_GSP_Table *Table, int *row, int *coll);
void TableSetWidgetAlign(_GSP_Table *Table, int align);
void TableSetTextAlign(_GSP_Table *Table, int row, int coll, int align);

void TableDelete(_GSP_Table *Table);

#endif	

