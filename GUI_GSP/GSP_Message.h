#ifndef MESSAGE_H
#define	MESSAGE_H

#include "GUI_GSP/Fonts/LVGL/GSP_lvgl.h"
#include "GUI_GSP.h"

// Константы выравнивания по вертикали
#define ALLIGN_VERT_TOP     0
#define ALLIGN_VERT_CENTER  1
#define ALLIGN_VERT_BOTTOM  2

// Виджет Message - многострочный текст с выравниванием
typedef struct
{
    unsigned char   Type;             // Тип виджета
    int             ID;               // ID виджета
    short           x;                // координата x левого верхнего угла (абсолютная)
    short           y;                // координата y левого верхнего угла (абсолютная)
    short           rel_x;            // относительная координата x (для пересчета при смене выравнивания)
    short           rel_y;            // относительная координата y (для пересчета при смене выравнивания)
    short           lenx;             // длина виджета
    short           leny;             // высота виджета
    short           WidthBorder;      // ширина бордюра
    short           R;                // Радиус закругления углов
    WORD_COLOR      BackColor;        // цвет основы
    WORD_COLOR      TextColor;        // Цвет текста
    WORD_COLOR      BorderColor;      // Цвет бордюра
    const char      *Text;            // Отображаемый текст (многострочный, разделитель \n)
    lv_font_t       *Font;            // Отображаемый шрифт 
    short           AllignH;          // Выравнивание по горизонтали (ALLIGN_TEXT_LEFT, ALLIGN_TEXT_CENTER, ALLIGN_TEXT_RIGHT)
    short           AllignV;          // Выравнивание по вертикали (ALLIGN_VERT_TOP, ALLIGN_VERT_CENTER, ALLIGN_VERT_BOTTOM)
    short           WidgetAlign;      // Выравнивание виджета относительно Screen (GSP_WIDGET_ALIGN_*)
    bool            Visible;          // признак видимости
    bool            Static;           // признак что не нужно выделять память. Text указывает на статичный буфер
    unsigned int    lifetime_ms;      // время жизни виджета в миллисекундах (0 = бесконечное)
    unsigned int    creation_time;    // время создания виджета (из Timer_GUI)
    bool            expired;          // флаг истечения времени жизни (для отложенного удаления)
} _GSP_Message;


_GSP_Message *Crate_Message(_GSP_Screen *Screen, int x, int y, int lenx, int leny, int ID);
void DrawingMessage(_GSP_Message *Message);
void MessageSetText(_GSP_Message *Message, const char *text);
void MessageSetFont(_GSP_Message *Message, void *font);
void MessageSetAllignH(_GSP_Message *Message, int allign);
void MessageSetAllignV(_GSP_Message *Message, int allign);
void MessageSetWidgetAlign(_GSP_Message *Message, int align);
void MessageSetPos(_GSP_Message *Message, int x, int y);
void MessageSetLifetime(_GSP_Message *Message, unsigned int lifetime_ms);
void MessageCheckAndDeleteExpired(_GSP_Screen *Screen);

#endif	/* MESSAGE_H */

