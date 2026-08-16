#ifndef LABEL_H
#define	LABEL_H

#include "GUI_GSP/Fonts/LVGL/GSP_lvgl.h"
#include "GUI_GSP.h"

// Виджет Lable
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
    WORD_COLOR      TextColor;        // Цвет текста
    WORD_COLOR      BorderColor;      // Цвет юордюра
    bool            En_Shift;         // Разрешение сдвига , если длинная строка
    const char      *Text;            // Отображаемый текст 
    lv_font_t       *Font;            // Отображаемый шрифт 
    short           Allign;             // Позиционирование текста по горизонтали (ALLIGN_TEXT_LEFT, ALLIGN_TEXT_CENTER, ALLIGN_TEXT_RIGHT)
    short           WidgetAlign;        // Выравнивание виджета относительно Screen (GSP_WIDGET_ALIGN_*)
    short           Time_Blinc;       // Период мигания в мс
    bool            Visible;            // признак видимости
    short           Mode_Shift;         // Режим бегущей строки
    // служебные параметры
    short           old_let_text;
    int             old_timer;       // ИСПРАВЛЕНО: изменено с short на int для избежания переполнения
    bool            BlincTogle;      // флаг мигания
    short           dx;                 // смещение для сдвига
    short           dy;                 // смещение текста по вертикали (в пикселях, может быть отрицательным)
    bool            sh;              // локальное разрешегие сдвига
    short           delay;              // задержка сдвига в начале
    bool            Static;            // признак что не нужно выжелять память. Text указывает на статичный буфер
    short           LetterSpacing;      // Межбуквенный интервал (в пикселях, может быть отрицательным)
    bool            AutoWidth;          // Режим автоширины: lenx зависит от ширины текста
    bool            en_edit;            // разрешение редактирования текста (добавление/удаление символов)
    int             max_text_length;    // максимальная длина текста в режиме редактирования (0 = без ограничений)
    // Параметры курсора для режима редактирования
    int             cursor_timer;       // таймер для мигания курсора
    bool            cursor_visible;      // видимость курсора (для мигания)
    bool            TextInvert;          // инвертирование только пикселей текста (XOR)
}_GSP_Label;


_GSP_Label *Crate_Label(_GSP_Screen *Screen, int x, int y, int lenx, int leny, int ID);
void DrawingLable(_GSP_Label *Label);
void LabelSetText(_GSP_Label *Label, const char * text);
void LabelSetTextFmt(_GSP_Label *Label, const char *format, ...);
void LabelSetFont(_GSP_Label * Label, void *font);
void LabelSetTextAllign(_GSP_Label *Label, int allign);
void LabelSetWidgetAlign(_GSP_Label *Label, int align);
void LabelSetPos(_GSP_Label *Label, int x, int y);
void LabelSetSize(_GSP_Label *Label, int lenx, int leny);
void LabelSetAutoWidth(_GSP_Label *Label, bool enable);

void LabelRunShift(_GSP_Label *Label);
void LabelRunBlinc(_GSP_Label *Label);
void LabelRunCursor(_GSP_Label *Label);
int LabelGetWidth(_GSP_Label *Label);
int LabelGetHigth(_GSP_Label *Label);
const char *LabelGetText(_GSP_Label *Label);
void LabelSetLetterSpacing(_GSP_Label *Label, int spacing);
void LabelSetVerticalOffset(_GSP_Label *Label, int offset);
void LabelSetEditMode(_GSP_Label *Label, bool enable);
void LabelSetMaxTextLength(_GSP_Label *Label, int max_length);
void LabelAppendChar(_GSP_Label *Label, const char *utf8_char);
void LabelRemoveLastChar(_GSP_Label *Label);
void LabelSetTextInvert(_GSP_Label *Label, bool invert);

#endif	/* LABEL_H */

