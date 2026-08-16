#include <p32xxxx.h>
#include <xc.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include "GUI_GSP.h"
#include "GSP_Label.h"
#include "GSP_DrawText.h"
#include "GSP_ProgressBar.h"
#include "GSP_mem_monitor.h"


#ifdef USED_PROGRESSBAR

//*****************************************************************
_GSP_ProgressBar *Crate_ProgressBar(_GSP_Screen *Screen, int x, int y, int lenx, int leny, int ID)
{
	_GSP_ProgressBar *Bar = user_malloc(sizeof(_GSP_ProgressBar));
	IncrementListWidgets(Screen, (int *)Bar);
	
	Bar->ID = ID;
	Bar->Type = GSP_BAR;
	Bar->lenx = lenx;
	Bar->leny = leny;
	
	// Инициализируем выравнивание виджета по умолчанию
	Bar->WidgetAlign = GSP_WIDGET_ALIGN_TOP_LEFT;
	
	// Сохраняем относительные координаты для будущего пересчета
	Bar->rel_x = x;
	Bar->rel_y = y;
	
	// Вычисляем абсолютные координаты с учетом выравнивания
	int abs_x = x;
	int abs_y = y;
	CalculateWidgetPosition(Screen, &abs_x, &abs_y, lenx, leny, Bar->WidgetAlign);
	Bar->x = abs_x;
	Bar->y = abs_y;
	
	Bar->Orient = BAR_HOR;
	Bar->BackColor = TRANSPARENT;
	Bar->BorderColor = DEFAULT_COLOR_TEXT;
	Bar->BarColor =    DEFAULT_COLOR_BAR;
	Bar->R = 0;
	Bar->WidthBorder = 1;
	Bar->Visible = true;
	Bar->Pos_Min = 0;
	Bar->Pos_Max = 1000;
	Bar->Pos = 500;
	Bar->Pos_Start = 0;
	Bar->BidirectionalMode = false;  // Двусторонний режим выключен по умолчанию

	// Инициализируем маркеры
	Bar->MarkerCount = 0;
	for(int i = 0; i < PROGRESSBAR_MAX_MARKERS; i++)
	{
		Bar->MarkerValues[i] = 0;
		Bar->MarkerPassed[i] = false;
	}

	return(Bar);
}
//*****************************************************
void ProgressBarSetWidgetAlign(_GSP_ProgressBar *Bar, int align)
{
	if(!Bar) return;
	
	Bar->WidgetAlign = align;
	
	// Находим родительский Screen для пересчета координат
	_GSP_Screen *Screen = Get_Active_Screen();
	if(!Screen) return;
	
	// Пересчитываем абсолютные координаты с учетом нового выравнивания
	int abs_x = Bar->rel_x;
	int abs_y = Bar->rel_y;
	CalculateWidgetPosition(Screen, &abs_x, &abs_y, Bar->lenx, Bar->leny, align);
	Bar->x = abs_x;
	Bar->y = abs_y;
}
//*****************************************************
void ProgressBarSetPos(_GSP_ProgressBar *Bar, int x, int y)
{
	if(!Bar) return;
	
	// Обновляем относительные координаты
	Bar->rel_x = x;
	Bar->rel_y = y;
	
	// Пересчитываем абсолютные координаты с учетом текущего выравнивания
	_GSP_Screen *Screen = Get_Active_Screen();
	if(Screen)
	{
		int abs_x = x;
		int abs_y = y;
		CalculateWidgetPosition(Screen, &abs_x, &abs_y, Bar->lenx, Bar->leny, Bar->WidgetAlign);
		Bar->x = abs_x;
		Bar->y = abs_y;
	}
}
//*****************************************************
void ProgressBarSetValue(_GSP_ProgressBar *Bar, int value)
{
	if(!Bar) return;
	
	// Ограничиваем значение в пределах Min..Max
	if(Bar->Pos_Min < Bar->Pos_Max)
	{
		if(value < Bar->Pos_Min) value = Bar->Pos_Min;
		if(value > Bar->Pos_Max) value = Bar->Pos_Max;
	}
	
	// В двустороннем режиме проверяем, что Value не меньше StartValue
	if(Bar->BidirectionalMode && value < Bar->Pos_Start)
	{
		value = Bar->Pos_Start;
	}
	
	Bar->Pos = value;

	// Обновляем признаки превышения маркеров
	for(int i = 0; i < Bar->MarkerCount && i < PROGRESSBAR_MAX_MARKERS; i++)
	{
		Bar->MarkerPassed[i] = (Bar->Pos > Bar->MarkerValues[i]);
	}
}
//*****************************************************
void ProgressBarSetRange(_GSP_ProgressBar *Bar, int min, int max)
{
	if(!Bar) return;
	
	// Устанавливаем Min и Max
	if(min <= max)
	{
		Bar->Pos_Min = min;
		Bar->Pos_Max = max;
	}
	else
	{
		// Если min > max, меняем местами
		Bar->Pos_Min = max;
		Bar->Pos_Max = min;
	}
	
	// Ограничиваем текущее значение в новом диапазоне
	if(Bar->Pos < Bar->Pos_Min) Bar->Pos = Bar->Pos_Min;
	if(Bar->Pos > Bar->Pos_Max) Bar->Pos = Bar->Pos_Max;
	
	// В двустороннем режиме проверяем и корректируем StartValue
	if(Bar->BidirectionalMode)
	{
		// Ограничиваем StartValue в новом диапазоне
		if(Bar->Pos_Start < Bar->Pos_Min) Bar->Pos_Start = Bar->Pos_Min;
		if(Bar->Pos_Start > Bar->Pos_Max) Bar->Pos_Start = Bar->Pos_Max;
		
		// Гарантируем, что StartValue не больше текущего Value
		if(Bar->Pos_Start > Bar->Pos) Bar->Pos_Start = Bar->Pos;
	}

	// Обновляем признаки превышения маркеров после смены диапазона
	for(int i = 0; i < Bar->MarkerCount && i < PROGRESSBAR_MAX_MARKERS; i++)
	{
		Bar->MarkerPassed[i] = (Bar->Pos > Bar->MarkerValues[i]);
	}
}
//*****************************************************
void ProgressBarIncrement(_GSP_ProgressBar *Bar, int increment)
{
	if(!Bar) return;
	
	// Увеличиваем значение
	int new_value = Bar->Pos + increment;
	
	// Ограничиваем в пределах Min..Max
	if(Bar->Pos_Min < Bar->Pos_Max)
	{
		if(new_value < Bar->Pos_Min) new_value = Bar->Pos_Min;
		if(new_value > Bar->Pos_Max) new_value = Bar->Pos_Max;
	}

	Bar->Pos = new_value;

	// Обновляем признаки превышения маркеров
	for(int i = 0; i < Bar->MarkerCount && i < PROGRESSBAR_MAX_MARKERS; i++)
	{
		Bar->MarkerPassed[i] = (Bar->Pos > Bar->MarkerValues[i]);
	}
}
//*****************************************************
int ProgressBarGetValue(_GSP_ProgressBar *Bar)
{
	if(!Bar) return (0);
	return (Bar->Pos);
}
//*****************************************************
void ProgressBarClearMarkers(_GSP_ProgressBar *Bar)
{
	if(!Bar) return;
	Bar->MarkerCount = 0;
	for(int i = 0; i < PROGRESSBAR_MAX_MARKERS; i++)
	{
		Bar->MarkerValues[i] = 0;
		Bar->MarkerPassed[i] = false;
	}
}
//*****************************************************
bool ProgressBarAddMarker(_GSP_ProgressBar *Bar, int value)
{
	if(!Bar) return false;

	// Нельзя добавить больше, чем PROGRESSBAR_MAX_MARKERS
	if(Bar->MarkerCount >= PROGRESSBAR_MAX_MARKERS) return false;

	// Ограничиваем значение в пределах Min..Max (по той же логике, что и ProgressBarSetValue)
	if(Bar->Pos_Min < Bar->Pos_Max)
	{
		if(value < Bar->Pos_Min) value = Bar->Pos_Min;
		if(value > Bar->Pos_Max) value = Bar->Pos_Max;
	}

	Bar->MarkerValues[Bar->MarkerCount] = (short)value;
	Bar->MarkerPassed[Bar->MarkerCount] = (Bar->Pos > Bar->MarkerValues[Bar->MarkerCount]);
	Bar->MarkerCount++;

	return true;
}
//*****************************************************
bool ProgressBarRemoveMarker(_GSP_ProgressBar *Bar, int index)
{
	if(!Bar) return false;

	// Проверяем валидность индекса
	if(index < 0 || index >= Bar->MarkerCount) return false;

	// Сдвигаем все маркеры после удаляемого влево
	for(int i = index; i < Bar->MarkerCount - 1; i++)
	{
		Bar->MarkerValues[i] = Bar->MarkerValues[i + 1];
		Bar->MarkerPassed[i] = Bar->MarkerPassed[i + 1];
	}

	// Уменьшаем счетчик и обнуляем последний элемент
	Bar->MarkerCount--;
	Bar->MarkerValues[Bar->MarkerCount] = 0;
	Bar->MarkerPassed[Bar->MarkerCount] = false;

	return true;
}
//*****************************************************
bool ProgressBarGetMarkerState(_GSP_ProgressBar *Bar, int index)
{
	if(!Bar) return false;

	// Проверяем валидность индекса
	if(index < 0 || index >= Bar->MarkerCount) return false;

	return Bar->MarkerPassed[index];
}
//*****************************************************
void ProgressBarSetSize(_GSP_ProgressBar *Bar, int lenx, int leny)
{
	if(!Bar) return;
	
	// Обновляем размер виджета
	Bar->lenx = lenx;
	Bar->leny = leny;
	
	// Пересчитываем абсолютные координаты с учетом текущего выравнивания
	// (при изменении размера позиция может измениться в зависимости от выравнивания)
	_GSP_Screen *Screen = Get_Active_Screen();
	if(Screen)
	{
		int abs_x = Bar->rel_x;
		int abs_y = Bar->rel_y;
		CalculateWidgetPosition(Screen, &abs_x, &abs_y, lenx, leny, Bar->WidgetAlign);
		Bar->x = abs_x;
		Bar->y = abs_y;
	}
}
//*************************************************
//**************************************************
void DrawingBar(_GSP_ProgressBar *Bar)
{
	if(Bar->Visible)
	{
		int lenx = Bar->lenx;
		int leny = Bar->leny;
		int x1 = Bar->x;
		int y1 = Bar->y;
		int x2 = Bar->x +lenx;
		int y2 = Bar->y +leny;
		int a_x = Active_Screen->x;
		int a_len_x = Active_Screen->lenx;
		int a_y = Active_Screen->y;
		int a_len_y = Active_Screen->leny;
		
		if( x2 > a_x+a_len_x) x2 = a_x+a_len_x;
		if( y2 > a_y+a_len_y) y2 = a_y+a_len_y;

SetClipRgn(x1, y1, x2, y2);
SetClip(true);
		if(Bar->BackColor != TRANSPARENT)
		{
SetColor(Bar->BackColor);
FillBevel(x1,y1,x2,y2, Bar->R);
		}		

		// Отрисовка залитой части бара
SetColor(Bar->BarColor);
		int filled_end_x = x1;
		int filled_start_x = x1;
		int filled_start_y = y2;
		int filled_end_y = y2;
		switch(Bar->Orient)
		{
			case BAR_HOR:
			{
				int range = Bar->Pos_Max - Bar->Pos_Min;
				if(range > 0)
				{
					if(Bar->BidirectionalMode)
					{
						// Двусторонний режим: полоска от Pos_Start до Pos
						// Вычисляем позиции начала и конца полоски
						int start_x = ((Bar->Pos_Start - Bar->Pos_Min) * Bar->lenx) / range + Bar->x;
						int end_x = ((Bar->Pos - Bar->Pos_Min) * Bar->lenx) / range + Bar->x;
						
						// Ограничиваем в пределах виджета
						if(start_x < x1) start_x = x1;
						if(start_x > x2) start_x = x2;
						if(end_x < x1) end_x = x1;
						if(end_x > x2) end_x = x2;
						
						// Определяем левую и правую границы полоски
						if(start_x < end_x)
						{
							filled_start_x = start_x;
							filled_end_x = end_x;
						}
						else
						{
							filled_start_x = end_x;
							filled_end_x = start_x;
						}
						
						// Гарантируем минимальную ширину полоски 2 пикселя
						if(filled_end_x - filled_start_x < 2)
						{
							// Если полоска слишком узкая, расширяем её до 2 пикселей
							if(filled_start_x + 2 <= x2)
							{
								filled_end_x = filled_start_x + 2;
							}
							else if(filled_end_x - 2 >= x1)
							{
								filled_start_x = filled_end_x - 2;
							}
							else
							{
								// Если виджет слишком узкий, используем всю ширину
								filled_start_x = x1;
								filled_end_x = x2;
							}
						}
						
						// Рисуем полоску от начального значения до текущего
FillBevel(filled_start_x, y1, filled_end_x, y2, Bar->R);
					}
					else
					{
						// Обычный режим: полоска от начала до текущей позиции
						int x = ((Bar->Pos - Bar->Pos_Min) * Bar->lenx) / range + Bar->x;
						filled_end_x = x;
FillBevel(x1, y1, x, y2, Bar->R);
					}
				}
				else
				{
					filled_end_x = Bar->x;
FillBevel(x1, y1, Bar->x, y2, Bar->R);
				}
				break;
			}
			case BAR_VERT:
			{
				int range = Bar->Pos_Max - Bar->Pos_Min;
				if(range > 0)
				{
					if(Bar->BidirectionalMode)
					{
						// Двусторонний режим: полоска от Pos_Start до Pos
						// Вычисляем позиции начала и конца полоски
						int start_y = (Bar->y + Bar->leny) - ((Bar->Pos_Start - Bar->Pos_Min) * Bar->leny) / range;
						int end_y = (Bar->y + Bar->leny) - ((Bar->Pos - Bar->Pos_Min) * Bar->leny) / range;
						
						// Ограничиваем в пределах виджета
						if(start_y < y1) start_y = y1;
						if(start_y > y2) start_y = y2;
						if(end_y < y1) end_y = y1;
						if(end_y > y2) end_y = y2;
						
						// Определяем верхнюю и нижнюю границы полоски
						if(start_y > end_y)
						{
							filled_start_y = end_y;
							filled_end_y = start_y;
						}
						else
						{
							filled_start_y = start_y;
							filled_end_y = end_y;
						}
						
						// Гарантируем минимальную высоту полоски 2 пикселя
						if(filled_end_y - filled_start_y < 2)
						{
							// Если полоска слишком узкая, расширяем её до 2 пикселей
							if(filled_start_y + 2 <= y2)
							{
								filled_end_y = filled_start_y + 2;
							}
							else if(filled_end_y - 2 >= y1)
							{
								filled_start_y = filled_end_y - 2;
							}
							else
							{
								// Если виджет слишком узкий, используем всю высоту
								filled_start_y = y1;
								filled_end_y = y2;
							}
						}
						
						// Рисуем полоску от начального значения до текущего
FillBevel(x1, filled_start_y, x2, filled_end_y, Bar->R);
					}
					else
					{
						// Обычный режим: полоска от низа до текущей позиции
						int y = (Bar->y + Bar->leny) - ((Bar->Pos - Bar->Pos_Min) * Bar->leny) / range;
						filled_start_y = y;
FillBevel(x1, y, x2, y2, Bar->R);
					}
				}
				else
				{
					filled_start_y = Bar->y;
FillBevel(x1, Bar->y, x2, y2, Bar->R);
				}
				break;
			}
		}

		// Отрисовка маркеров (пунктирные линии поперёк бара)
		if(Bar->MarkerCount > 0)
		{
			for(int i = 0; i < Bar->MarkerCount; i++)
			{
				int m_val = Bar->MarkerValues[i];

				// Пропускаем маркер, если некорректный диапазон
				int range = Bar->Pos_Max - Bar->Pos_Min;
				if(range <= 0) continue;

				switch(Bar->Orient)
				{
					case BAR_HOR:
					{
						// Позиция маркера по X (аналогично заполнению бара)
						int mx;
						int range = Bar->Pos_Max - Bar->Pos_Min;
						if(range > 0)
						{
							mx = ((m_val - Bar->Pos_Min) * Bar->lenx) / range + Bar->x;
						}
						else
						{
							mx = Bar->x;
						}

						// Маркер пунктиром через XOR — виден и на закрашенной, и на незакрашенной области
						{
							int y;
							for (y = y1; y <= y2; y += 2)
								PutPixelXOR(mx, y);
						}
						break;
					}
					case BAR_VERT:
					{
						// Позиция маркера по Y (аналогично заполнению бара)
						int my;
						int range = Bar->Pos_Max - Bar->Pos_Min;
						if(range > 0)
						{
							my = (Bar->y + Bar->leny) - ((m_val - Bar->Pos_Min) * Bar->leny) / range;
						}
						else
						{
							my = Bar->y;
						}

						// Маркер пунктиром через XOR — виден и на закрашенной, и на незакрашенной области
						{
							int x;
							for (x = x1; x <= x2; x += 2)
								PutPixelXOR(x, my);
						}
						break;
					}
				}
			}
		}
		
		if(Bar->WidthBorder)
		{
SetLineThickness(Bar->WidthBorder-1);		
SetColor(Bar->BorderColor);
Bevel(x1,y1,x2,y2, Bar->R);
		}
SetClip(false);
	}
}
//*****************************************************
void ProgressBarSetBidirectionalMode(_GSP_ProgressBar *Bar, bool enable)
{
	if(!Bar) return;
	Bar->BidirectionalMode = enable;
	
	// При включении режима инициализируем начальное значение текущим значением
	if(enable)
	{
		Bar->Pos_Start = Bar->Pos;
	}
	else
	{
		// При выключении режима сбрасываем начальное значение
		Bar->Pos_Start = Bar->Pos_Min;
	}
}
//*****************************************************
void ProgressBarSetStartValue(_GSP_ProgressBar *Bar, int start_value)
{
	if(!Bar) return;
	
	// Ограничиваем значение в пределах Min..Max
	if(Bar->Pos_Min < Bar->Pos_Max)
	{
		if(start_value < Bar->Pos_Min) start_value = Bar->Pos_Min;
		if(start_value > Bar->Pos_Max) start_value = Bar->Pos_Max;
	}
	
	// В двустороннем режиме проверяем, что StartValue не больше текущего Value
	if(Bar->BidirectionalMode && start_value > Bar->Pos)
	{
		start_value = Bar->Pos;
	}
	
	Bar->Pos_Start = start_value;
}
#endif
