#ifndef KEYBOARD_GSP_H
#define KEYBOARD_GSP_H

#include <stdbool.h>
#include "GUI_GSP/Fonts/LVGL/GSP_lvgl.h"
#include "GUI_GSP.h"


// Виджет Keyboard с сеткой кнопок
typedef struct
{
    unsigned char   Type;              // тип виджета (GSP_KEYBOARD)
    int             ID;                // уникальный идентификатор виджета
    short           x;                 // координата X левого верхнего угла виджета (абсолютная)
    short           y;                 // координата Y левого верхнего угла виджета (абсолютная)
    short           rel_x;             // относительная координата x (для пересчета при смене выравнивания)
    short           rel_y;             // относительная координата y (для пересчета при смене выравнивания)
    short           lenx;              // ширина виджета в пикселях
    short           leny;              // высота виджета в пикселях
    WORD_COLOR      BackColor;         // цвет фона виджета
    WORD_COLOR      TextColor;         // цвет текста невыбранных кнопок
    WORD_COLOR      SelColor;          // цвет фона выбранной кнопки
    WORD_COLOR      ButtonBorderColor; // цвет рамки кнопок
    bool            Visible;           // видимость виджета (true - видимый, false - скрытый)
    lv_font_t       *Font;             // указатель на шрифт для отображения текста
    short           WidgetAlign;       // Выравнивание виджета относительно Screen (GSP_WIDGET_ALIGN_*)
    
    // Массив строк для кнопок (заканчивается на NULL или пустой строке)
    // "\n" означает конец текущей строки кнопок
    const char      **buttons;         // массив указателей на строки кнопок
    int             button_count;      // общее количество кнопок (без учета "\n")
    int             rows;              // количество строк кнопок
    int             *row_lengths;      // массив длин строк (количество кнопок в каждой строке)
    
    // Параметры кнопок
    int             button_width;      // ширина одной кнопки в пикселях (базовая ширина)
    int             button_height;     // высота одной кнопки в пикселях
    int             button_gap_x;      // зазор между кнопками по горизонтали
    int             button_gap_y;      // зазор между кнопками по вертикали
    int             width_remainder;    // остаток от деления ширины (распределяется между кнопками)
    int             height_remainder;  // остаток от деления высоты (распределяется между строками)
    int             text_offset_y;     // смещение текста по вертикали (положительное - вниз, отрицательное - вверх)
    float           *button_width_multipliers; // массив множителей ширины для каждой кнопки (по умолчанию все = 1.0f)
    
    // Навигация
    int             selected_row;      // индекс выбранной строки (0-based)
    int             selected_col;     // индекс выбранной кнопки в строке (0-based)
    int             selected_index;   // общий индекс выбранной кнопки (для быстрого доступа)
    
}_GSP_Keyboard;

// Функции создания и управления
_GSP_Keyboard *Crate_Keyboard(_GSP_Screen *Screen, int x, int y, int lenx, int leny, int ID);
void DrawingKeyboard(_GSP_Keyboard *Keyboard);
void KeyboardSetButtons(_GSP_Keyboard *Keyboard, const char **buttons);
void KeyboardMoveNext(_GSP_Keyboard *Keyboard);
void KeyboardMovePrev(_GSP_Keyboard *Keyboard);
void KeyboardMoveUp(_GSP_Keyboard *Keyboard);
void KeyboardMoveDown(_GSP_Keyboard *Keyboard);
void KeyboardSetSelected(_GSP_Keyboard *Keyboard, int row, int col);
int KeyboardGetSelectedIndex(_GSP_Keyboard *Keyboard);
const char *KeyboardGetSelectedText(_GSP_Keyboard *Keyboard);
void KeyboardSetFont(_GSP_Keyboard *Keyboard, void *font);
void KeyboardSetWidgetAlign(_GSP_Keyboard *Keyboard, int align);
void KeyboardSetPos(_GSP_Keyboard *Keyboard, int x, int y);
void KeyboardSetSize(_GSP_Keyboard *Keyboard, int lenx, int leny);
void KeyboardSetTextOffset(_GSP_Keyboard *Keyboard, int offset_y);
void KeyboardSetButtonWidthMultiplier(_GSP_Keyboard *Keyboard, int row, int col, float multiplier);

#endif // KEYBOARD_GSP_H

