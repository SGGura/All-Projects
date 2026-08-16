#ifndef LIST_H
#define LIST_H

#include <stdbool.h>
#include "GUI_GSP/Fonts/LVGL/GSP_lvgl.h"
#include "GUI_GSP.h"

// Виджет List с прокруткой и статус-баром
typedef struct
{
    unsigned char   Type;              // тип виджета (GSP_LIST)
    int             ID;                // уникальный идентификатор виджета
    short             x;                 // координата X левого верхнего угла виджета (абсолютная)
    short             y;                 // координата Y левого верхнего угла виджета (абсолютная)
    short             rel_x;             // относительная координата x (для пересчета при смене выравнивания)
    short             rel_y;             // относительная координата y (для пересчета при смене выравнивания)
    short             lenx;              // ширина виджета в пикселях
    short             leny;              // высота виджета в пикселях
    short             WidthBorder;        // толщина рамки в пикселях (0 - без рамки)
    short             BorderSides;       // флаги сторон рамки (GSP_BORDER_TOP, GSP_BORDER_BOTTOM, GSP_BORDER_LEFT, GSP_BORDER_RIGHT, GSP_BORDER_ALL)
    short             R;                 // радиус скругления углов (0 - без скругления)
    WORD_COLOR      BackColor;         // цвет фона виджета
    WORD_COLOR      TextColor;         // цвет текста невыбранных элементов
    WORD_COLOR      BorderColor;        // цвет рамки виджета
    WORD_COLOR      SelColor;          // цвет фона выбранного элемента
    bool            Visible;           // видимость виджета (true - видимый, false - скрытый)
    bool            ShowStatus;        // показывать строку статуса внизу (true - показывать, false - скрыть)
    bool            ShowTriangle;      // показывать треугольник перед текстом (true - показывать, false - скрыть)
    bool            ShowScrollbar;     // показывать скроллбар (true - показывать, false - скрыть, по умолчанию true)
    bool            *checkmarks;       // массив флагов отображения галочки для каждой строки (true - показывать, false - скрыть)
    lv_font_t       *Font;             // указатель на шрифт для отображения текста
    short             Allign;            // выравнивание текста по горизонтали (ALLIGN_TEXT_LEFT, ALLIGN_TEXT_CENTER, ALLIGN_TEXT_RIGHT)
    bool            En_Shift;          // разрешение бегущей строки для длинного текста (true - включено, false - выключено)
    short             Mode_Shift;        // режим бегущей строки (GSP_SHIFT_CICL - циклический, GSP_SHIFT_INFINITY - бесконечный)
    // служебные параметры для сдвига текста (только для выбранного элемента)
    short           dx;                // смещение по X для сдвига выбранного элемента при бегущей строке
    bool            sh;                // локальное разрешение сдвига для выбранного элемента
    short           delay;             // задержка сдвига в начале для выбранного элемента
    
    short             item_count;        // количество элементов в списке
    char            **items;          // массив указателей на строки (динамические копии или статические)
    bool            *item_static;     // true — строка статическая (не освобождать при Clear/DelItem)
    short             selected;          // индекс выбранного элемента (-1 - ничего не выбрано)
    short             top_index;         // индекс верхнего видимого элемента (для прокрутки)
    short             item_height;       // вычисляемая высота одной строки в пикселях (на основе шрифта)
    short             gap_between_items; // зазор между строками элементов в пикселях (по умолчанию 1)
    short             text_padding;      // зазор текста сверху элемента в пикселях (по умолчанию 1)
    short             triangle_gap;     // зазор между треугольником и началом текста в пикселях (по умолчанию 0)
    bool            ShowSelection;     // показывать визуальное выделение выбранного элемента (true - показывать)
    bool            CenterItems;       // центрировать строки по вертикали, если их меньше чем помещается (по умолчанию false)
    short             WidgetAlign;       // Выравнивание виджета относительно Screen (GSP_WIDGET_ALIGN_*)

}_GSP_List;

_GSP_List *Crate_List(_GSP_Screen *Screen, int x, int y, int lenx, int leny, int ID);
void DrawingList(_GSP_List *List);

void ListAddItem(_GSP_List *List, const char *text);       // копирует строку в кучу
void ListAddItemStatic(_GSP_List *List, const char *text); // сохраняет указатель на статическую строку (не копирует, не освобождает)
void ListClear(_GSP_List *List);
void ListSetSelected(_GSP_List *List, int idx);
int ListScrollUp(_GSP_List *List);      // Возвращает новый выбранный индекс
int ListScrollDown(_GSP_List *List);    // Возвращает новый выбранный индекс
void ListSetShowStatus(_GSP_List *List, bool show);
void ListSetFont(_GSP_List *List, void *font);
void ListSetAllign(_GSP_List *List, int allign);
void ListSetShift(_GSP_List *List, bool enable, int mode);
void ListSetBorderSides(_GSP_List *List, int sides);
void ListSetShowTriangle(_GSP_List *List, bool show);
void ListSetShowScrollbar(_GSP_List *List, bool show);
void ListSetShowSelection(_GSP_List *List, bool show);
void ListSetItemCheckmark(_GSP_List *List, int idx, bool show);
void ListSetTriangleGap(_GSP_List *List, int gap);
void ListSetGapBetweenItems(_GSP_List *List, int gap);
void ListSetWidgetAlign(_GSP_List *List, int align);
void ListSetPos(_GSP_List *List, int x, int y);
void ListSetSize(_GSP_List *List, int lenx, int leny);
void ListSetCenterItems(_GSP_List *List, bool center);
void ListRunShift(_GSP_List *List);
void ListDelItem(_GSP_List *List, int idx);
int ListGetSelected(_GSP_List *List);

#endif // LIST_H
