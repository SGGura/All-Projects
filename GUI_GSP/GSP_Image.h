#ifndef BITMAP_H
#define	BITMAP_H

#include "GUI_GSP/Fonts/LVGL/GSP_lvgl.h"
#include "GUI_GSP.h"

// Режимы анимации
#define GSP_ANIM_MODE_LOOP     0  // Циклическая анимация (по умолчанию)
#define GSP_ANIM_MODE_ONCE     1  // Анимация один раз, затем остановка
#define GSP_ANIM_MODE_PINGPONG 2  // Анимация туда-обратно
#define GSP_ANIM_MODE_REVERSE  3  // Обратная циклическая анимация

// Состояния анимации
#define GSP_ANIM_STATE_STOPPED 0
#define GSP_ANIM_STATE_PLAYING 1
#define GSP_ANIM_STATE_PAUSED  2

// Виджет BitMAp
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
    void            *Bitmaps;         // Массив картинок для анимации или одна картинка
    void            *tmp_Bitmap;      // Временный указатель на текущий кадр
    short           WidgetAlign;        // Выравнивание виджета относительно Screen (GSP_WIDGET_ALIGN_*)
    bool            Visible;            // признак видимости
    
    // Поля для анимации
    int             CountFrame;       // Текущий кадр анимации
    int             Count_Time;       // Счетчик времени для анимации
    int             N_Frames;         // Количество кадров в анимации
    int             Time;             // Время между кадрами (-1 = без анимации)
    unsigned char   AnimMode;         // Режим анимации (GSP_ANIM_MODE_*)
    unsigned char   AnimState;        // Состояние анимации (GSP_ANIM_STATE_*)
    int             AnimDirection;    // Направление анимации (1 или -1 для ping-pong)
    int             last_timer_update;// Последнее обновление таймера для более точного тайминга
    
    // Поля для мерцания
    int             Time_Blinc;       // Время мерцания (0 = без мерцания)
    bool            BlincTogle;       // Флаг состояния мерцания
    int             old_timer;        // Старое значение таймера для мерцания
} _GSP_Image;


_GSP_Image *Crate_Image(_GSP_Screen *Screen, int x, int y, int lenx, int leny, int ID);
void DrawingImage(_GSP_Image *img);
void ImageSetBMP(_GSP_Image *img, const void *bitmap);
void ImageSetWidgetAlign(_GSP_Image *img, int align);
void ImageSetPos(_GSP_Image *img, int x, int y);
void ImageSetSize(_GSP_Image *img, int lenx, int leny);
int ImageGetWidth(_GSP_Image *img);

// Функции для работы с анимацией
void ImageSetFrames(_GSP_Image *img, void *frames, int n, int time);
void ImageSetFramesEx(_GSP_Image *img, void *frames, int n, int time, int mode);
void ImageDeleteAnimation(_GSP_Image *img);
void ImagePauseAnimation(_GSP_Image *img);
void ImageResumeAnimation(_GSP_Image *img);
void ImageRestartAnimation(_GSP_Image *img);
void ImageSetAnimationMode(_GSP_Image *img, int mode);
int ImageGetCurrentFrame(_GSP_Image *img);
bool ImageIsAnimationPlaying(_GSP_Image *img);
void ImageStopAnimationAtFrame(_GSP_Image *img, int frame);
void ImageStopAnimation(_GSP_Image *img);
void ImageStartAnimation(_GSP_Image *img);

// Функции для работы с мерцанием
void ImageSetBlink(_GSP_Image *img, int blink_time);
void ImageDisableBlink(_GSP_Image *img);


#endif	/* LABEL_H */

