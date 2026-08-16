#include <p32xxxx.h>
#include <xc.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "GUI_GSP.h"
#include "GSP_Image.h"
#include "GSP_mem_monitor.h"


#ifdef USED_IMAGE

//*****************************************************************
_GSP_Image *Crate_Image(_GSP_Screen *Screen, int x, int y, int lenx, int leny, int ID)
{
	_GSP_Image *img = user_malloc(sizeof(_GSP_Image));
	IncrementListWidgets(Screen, (int *)img);
	
	img->Type = GSP_IMAGE;
	img->lenx = lenx;
	img->leny = leny;
	img->ID = ID;
	
	// Инициализируем выравнивание виджета по умолчанию
	img->WidgetAlign = GSP_WIDGET_ALIGN_TOP_LEFT;
	
	// Сохраняем относительные координаты для будущего пересчета
	img->rel_x = x;
	img->rel_y = y;
	
	// Вычисляем абсолютные координаты с учетом выравнивания
	int abs_x = x;
	int abs_y = y;
	CalculateWidgetPosition(Screen, &abs_x, &abs_y, lenx, leny, img->WidgetAlign);
	img->x = abs_x;
	img->y = abs_y;
	
	img->Bitmaps = NULL;
	img->tmp_Bitmap = NULL;
	
	img->BackColor = TRANSPARENT;
	img->BorderColor = DEFAULT_COLOR_BACK;
	img->R = 0;
	img->WidthBorder = 0;
	img->Visible = true;
	
	// Инициализация полей анимации
	img->CountFrame = 0;
	img->Count_Time = 0;
	img->N_Frames = 0;
	img->Time = -1;  // -1 означает отсутствие анимации
	img->AnimMode = GSP_ANIM_MODE_LOOP;
	img->AnimState = GSP_ANIM_STATE_STOPPED;
	img->AnimDirection = 1;
	img->last_timer_update = 0;
	
	// Инициализация полей мерцания
	img->Time_Blinc = 0;  // 0 означает отсутствие мерцания
	img->BlincTogle = true;
	img->old_timer = 0;
	return(img);
}
//***********************************************
void ImageSetBMP(_GSP_Image *img, const void *bitmap)
{
	if(img)
	{
		img->Bitmaps = (void *)bitmap;  // Приводим к void * для сохранения в структуре
		img->Time = -1;  // Отключаем анимацию
		img->CountFrame = 0;
		img->tmp_Bitmap = NULL;
	}
}
//***********************************************
void ImageSetFrames(_GSP_Image *img, void *frames, int n, int time)
{
	if (img && frames && n > 0 && time > 0)
	{
		img->Bitmaps = frames;
		img->N_Frames = n;
		img->CountFrame = 0;
		img->Count_Time = 0;
		img->Time = time;
		img->tmp_Bitmap = NULL;
		img->AnimMode = GSP_ANIM_MODE_LOOP;
		img->AnimState = GSP_ANIM_STATE_PLAYING;
		img->AnimDirection = 1;
		img->last_timer_update = Timer_GUI;
	}
}
//***********************************************
void ImageSetFramesEx(_GSP_Image *img, void *frames, int n, int time, int mode)
{
	if (img && frames && n > 0 && time > 0 && 
		mode >= GSP_ANIM_MODE_LOOP && mode <= GSP_ANIM_MODE_REVERSE)
	{
		img->Bitmaps = frames;
		img->N_Frames = n;
		img->CountFrame = (mode == GSP_ANIM_MODE_REVERSE) ? n - 1 : 0;
		img->Count_Time = 0;
		img->Time = time;
		img->tmp_Bitmap = NULL;
		img->AnimMode = mode;
		img->AnimState = GSP_ANIM_STATE_PLAYING;
		img->AnimDirection = (mode == GSP_ANIM_MODE_REVERSE) ? -1 : 1;
		img->last_timer_update = Timer_GUI;
	}
}
//***********************************************
void ImageDeleteAnimation(_GSP_Image *img)
{
	if(img)
	{
		img->Time = -1;  // Отключаем анимацию
		img->tmp_Bitmap = NULL;
		img->AnimState = GSP_ANIM_STATE_STOPPED;
	}
}
//***********************************************
void ImagePauseAnimation(_GSP_Image *img)
{
	if(img && img->AnimState == GSP_ANIM_STATE_PLAYING)
	{
		img->AnimState = GSP_ANIM_STATE_PAUSED;
	}
}
//***********************************************
void ImageResumeAnimation(_GSP_Image *img)
{
	if(img && img->AnimState == GSP_ANIM_STATE_PAUSED)
	{
		img->AnimState = GSP_ANIM_STATE_PLAYING;
		img->last_timer_update = Timer_GUI; // Сброс таймера для точного возобновления
	}
}
//***********************************************
void ImageRestartAnimation(_GSP_Image *img)
{
	if(img && img->Time > 0)
	{
		img->CountFrame = (img->AnimMode == GSP_ANIM_MODE_REVERSE) ? img->N_Frames - 1 : 0;
		img->Count_Time = 0;
		img->AnimDirection = (img->AnimMode == GSP_ANIM_MODE_REVERSE) ? -1 : 1;
		img->AnimState = GSP_ANIM_STATE_PLAYING;
		img->last_timer_update = Timer_GUI;
	}
}
//***********************************************
void ImageSetAnimationMode(_GSP_Image *img, int mode)
{
	if(img && mode >= GSP_ANIM_MODE_LOOP && mode <= GSP_ANIM_MODE_REVERSE)
	{
		img->AnimMode = mode;
		if(img->Time > 0) // Если анимация активна, перезапускаем с новым режимом
		{
			ImageRestartAnimation(img);
		}
	}
}
//***********************************************
int ImageGetCurrentFrame(_GSP_Image *img)
{
	if(img)
	{
		return img->CountFrame;
	}
	return -1;
}
//***********************************************
bool ImageIsAnimationPlaying(_GSP_Image *img)
{
	if(img)
	{
		return (img->AnimState == GSP_ANIM_STATE_PLAYING && img->Time > 0);
	}
	return false;
}
//***********************************************
void ImageStopAnimationAtFrame(_GSP_Image *img, int frame)
{
	if(img && img->Time > 0 && img->N_Frames > 0)
	{
		// Ограничиваем номер кадра в допустимых пределах
		if(frame < 0) frame = 0;
		if(frame >= img->N_Frames) frame = img->N_Frames - 1;
		
		// Устанавливаем кадр и останавливаем анимацию
		img->CountFrame = frame;
		img->AnimState = GSP_ANIM_STATE_STOPPED;
		
		// Устанавливаем текущий кадр для отображения
		if(img->Bitmaps)
		{
			int *ptr = (int*)img->Bitmaps;
			if(frame >= 0 && frame < img->N_Frames)
			{
				img->tmp_Bitmap = (void *)ptr[frame];
			}
		}
	}
}
//***********************************************
void ImageStopAnimation(_GSP_Image *img)
{
	if(img && img->Time > 0)
	{
		// Останавливаем анимацию на текущем кадре
		img->AnimState = GSP_ANIM_STATE_STOPPED;
		
		// Убеждаемся, что текущий кадр установлен для отображения
		if(img->Bitmaps && img->N_Frames > 0)
		{
			// Ограничиваем текущий кадр в допустимых пределах
			if(img->CountFrame < 0) img->CountFrame = 0;
			if(img->CountFrame >= img->N_Frames) img->CountFrame = img->N_Frames - 1;
			
			int *ptr = (int*)img->Bitmaps;
			if(img->CountFrame >= 0 && img->CountFrame < img->N_Frames)
			{
				img->tmp_Bitmap = (void *)ptr[img->CountFrame];
			}
		}
	}
}
//***********************************************
void ImageStartAnimation(_GSP_Image *img)
{
	if(img && img->Time > 0 && img->N_Frames > 0)
	{
		// Запускаем анимацию с текущего кадра
		img->AnimState = GSP_ANIM_STATE_PLAYING;
		img->last_timer_update = Timer_GUI; // Сброс таймера для точного запуска
		
		// Устанавливаем начальное направление в зависимости от режима
		if(img->AnimMode == GSP_ANIM_MODE_REVERSE)
		{
			img->AnimDirection = -1;
		}
		else
		{
			img->AnimDirection = 1;
		}
	}
}
//***********************************************
void ImageSetBlink(_GSP_Image *img, int blink_time)
{
	if(img)
	{
		img->Time_Blinc = blink_time;
		img->old_timer = 0;  // Сброс таймера
	}
}
//***********************************************
void ImageDisableBlink(_GSP_Image *img)
{
	if(img)
	{
		img->Time_Blinc = 0;  // Отключаем мерцание
		img->BlincTogle = true;  // Устанавливаем видимость
	}
}
//*************************************************
void ImageSetWidgetAlign(_GSP_Image *img, int align)
{
	if(!img) return;
	img->WidgetAlign = align;
	
	// Пересчитываем координаты относительно Active_Screen
	_GSP_Screen *Screen = Get_Active_Screen();
	if(Screen)
	{
		// Используем сохраненные относительные координаты для пересчета
		int abs_x = img->rel_x;
		int abs_y = img->rel_y;
		CalculateWidgetPosition(Screen, &abs_x, &abs_y, img->lenx, img->leny, align);
		img->x = abs_x;
		img->y = abs_y;
	}
}
//*****************************************************
void ImageSetPos(_GSP_Image *img, int x, int y)
{
	if(!img) return;
	
	// Обновляем относительные координаты
	img->rel_x = x;
	img->rel_y = y;
	
	// Пересчитываем абсолютные координаты с учетом текущего выравнивания
	_GSP_Screen *Screen = Get_Active_Screen();
	if(Screen)
	{
		int abs_x = x;
		int abs_y = y;
		CalculateWidgetPosition(Screen, &abs_x, &abs_y, img->lenx, img->leny, img->WidgetAlign);
		img->x = abs_x;
		img->y = abs_y;
	}
}
//*****************************************************
void ImageSetSize(_GSP_Image *img, int lenx, int leny)
{
	if(!img) return;
	
	// Обновляем размер виджета
	img->lenx = lenx;
	img->leny = leny;
	
	// Пересчитываем абсолютные координаты с учетом текущего выравнивания
	// (при изменении размера позиция может измениться в зависимости от выравнивания)
	_GSP_Screen *Screen = Get_Active_Screen();
	if(Screen)
	{
		int abs_x = img->rel_x;
		int abs_y = img->rel_y;
		CalculateWidgetPosition(Screen, &abs_x, &abs_y, lenx, leny, img->WidgetAlign);
		img->x = abs_x;
		img->y = abs_y;
	}
}
//****************************************************************
int ImageGetWidth(_GSP_Image *img)
{
	return(img->lenx);
}
//*************************************************
static void ImageUpdateAnimation(_GSP_Image *img)
{
	if (!img || img->Time <= 0 || img->AnimState != GSP_ANIM_STATE_PLAYING) return;
	
	// Проверяем безопасность массива
	if (!img->Bitmaps || img->N_Frames <= 0) return;
	
	// Более точный тайминг на основе реального времени
	int elapsed_time = Timer_GUI - img->last_timer_update;
	if (elapsed_time < img->Time) return; // Еще не время для смены кадра
	
	img->last_timer_update = Timer_GUI;
	
	// Обработка различных режимов анимации
	switch (img->AnimMode)
	{
		case GSP_ANIM_MODE_LOOP:
			img->CountFrame = (img->CountFrame + 1) % img->N_Frames;
			break;
			
		case GSP_ANIM_MODE_ONCE:
			if (img->CountFrame < img->N_Frames - 1)
			{
				img->CountFrame++;
			}
			else
			{
				img->AnimState = GSP_ANIM_STATE_STOPPED; // Остановка после одного прохода
			}
			break;
			
		case GSP_ANIM_MODE_PINGPONG:
			img->CountFrame += img->AnimDirection;
			if (img->CountFrame >= img->N_Frames - 1)
			{
				img->CountFrame = img->N_Frames - 1;
				img->AnimDirection = -1; // Меняем направление
			}
			else if (img->CountFrame <= 0)
			{
				img->CountFrame = 0;
				img->AnimDirection = 1; // Меняем направление
			}
			break;
			
		case GSP_ANIM_MODE_REVERSE:
			img->CountFrame--;
			if (img->CountFrame < 0)
			{
				img->CountFrame = img->N_Frames - 1;
			}
			break;
	}
	
	// Проверка границ массива кадров
	if (img->CountFrame >= 0 && img->CountFrame < img->N_Frames)
	{
		int *ptr = (int*)img->Bitmaps;
		img->tmp_Bitmap = (void *)ptr[img->CountFrame];
	}
}
//*************************************************
static void ImageRunBlinc(_GSP_Image *img)
{
	if (img->Time_Blinc)
	{
		if (labs(Timer_GUI - img->old_timer) >= img->Time_Blinc)
		{
			img->old_timer = Timer_GUI;
			if (img->BlincTogle) img->BlincTogle = false;
			else img->BlincTogle = true;
		}
	}
	else img->BlincTogle = true;
}
//*************************************************
void DrawingImage(_GSP_Image *img)
{
	if(img->Visible)
	{
SetColor(img->BackColor);
		int lenx = img->lenx;
		int leny = img->leny;
		int x1 = img->x;
		int y1 = img->y;
		int x2 = img->x + lenx;
		int y2 = img->y + leny;
		
		int a_x = Active_Screen->x;
		int a_len_x = Active_Screen->lenx;
		int a_y = Active_Screen->y;
		int a_len_y = Active_Screen->leny;
		
		if(x2 > a_x + a_len_x) x2 = a_x + a_len_x;
		if(y2 > a_y + a_len_y) y2 = a_y + a_len_y;

SetClipRgn(x1, y1, x2, y2);
SetClip(true);
		
		if(img->BackColor != TRANSPARENT)
		{
SetColor(img->BackColor);
FillBevel(x1, y1, x2, y2, img->R);
		}

		// Обработка мерцания
		ImageRunBlinc(img);
		
		if(img->BlincTogle)  // Показывать изображение только если не мигает или находится в видимом состоянии
		{
			if(img->Bitmaps)
			{
				// Обновляем анимацию (если активна)
				ImageUpdateAnimation(img);
				
				// Выбираем изображение для отрисовки
				if(img->Time > 0 && img->tmp_Bitmap) // анимированное изображение
				{
					// tmp_Bitmap уже установлен в ImageUpdateAnimation
				}
				else if(img->Time <= 0) // статическое изображение
				{
					img->tmp_Bitmap = img->Bitmaps;
				}

				// Отрисовка изображения с проверкой безопасности
				if(img->tmp_Bitmap)
				{
					short h = GetImageHeight(img->tmp_Bitmap);
					short w = GetImageWidth(img->tmp_Bitmap);
					
					if(h > 0 && w > 0) // Дополнительная проверка корректности изображения
					{
						short y = y1 + (leny - h) / 2; // центровка по вертикали
						lenx = x2 - x1;	
						int x = x1 + (lenx - w) / 2;   // центровка по горизонтали
						
if (img->tmp_Bitmap)
								PutImage((short)x, (short)y, img->tmp_Bitmap, 0);
					}
				}
			}
		}

		// Рисуем бордюр
		if(img->WidthBorder)
		{
SetLineThickness(img->WidthBorder - 1);		
SetColor(img->BorderColor);
Bevel(x1, y1, x2, y2, img->R);
		}
SetClip(false);
	}
}
#endif
