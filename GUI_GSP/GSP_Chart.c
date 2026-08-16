#include <p32xxxx.h>
#include <xc.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include "GUI_GSP.h"
#include "GSP_Chart.h"
#include "GSP_mem_monitor.h"

static void ChartPackedData(short *In_Data, int len_in, short *Out_Data, int len_out);

#ifdef USED_CHART

//*****************************************************************
_GSP_Chart *Chart_Crate(_GSP_Screen *Screen, int x, int y, int lenx, int leny, int ID)
{
	_GSP_Chart *Chart = user_malloc(sizeof(_GSP_Chart));
	IncrementListWidgets(Screen, (int *)Chart);
	
	Chart->Type = GSP_CHART;
	Chart->lenx = lenx;
	Chart->leny = leny;
	Chart->ID = ID;
	
	// Инициализируем выравнивание виджета по умолчанию
	Chart->WidgetAlign = GSP_WIDGET_ALIGN_TOP_LEFT;
	
	// Сохраняем относительные координаты для будущего пересчета
	Chart->rel_x = x;
	Chart->rel_y = y;
	
	// Вычисляем абсолютные координаты с учетом выравнивания
	int abs_x = x;
	int abs_y = y;
	CalculateWidgetPosition(Screen, &abs_x, &abs_y, lenx, leny, Chart->WidgetAlign);
	Chart->x = abs_x;
	Chart->y = abs_y;

	Chart->BackColor = DEFAULT_COLOR_BACK;
	Chart->BorderColor = DEFAULT_COLOR_ACTIVE;
	Chart->WidthBorder = 0;
	Chart->R = 0;
	Chart->Visible = true;
	Chart->Greed = GREED_DOTE;
	Chart->GreedColor = DEFAULT_COLOR_ACTIVE;
	Chart->CurveColor = DEFAULT_COLOR_ACTIVE;	
	Chart->ThincCurve	 = 1;
	Chart->N_X = 10;
	Chart->N_Y = 5;
	Chart->Data = NULL;
	Chart->DataView = user_malloc(Chart->lenx*2 * sizeof(short));		
	if(!Chart->DataView)
	{
		// Не удалось выделить память - освобождаем виджет
		user_free(Chart);
		return NULL;
	}
	Chart->Max = 1000;
	Chart->Min = 0;
	Chart->Fill = 0;
	Chart->Mode = CHART_MODE_GRAPH;  // Режим графика по умолчанию
	Chart->BarData = NULL;
	Chart->BarDataCount = 0;
	Chart->HistogramBins = lenx;  // По умолчанию количество bins = ширина виджета
	Chart->AutoBins = false;  // Автоматический расчет количества bins выключен по умолчанию
	Chart->MinBarWidth = 3;  // Минимальная ширина столбика по умолчанию (3 пикселя)
	Chart->InvertY = false;  // По умолчанию Min внизу, Max вверху
	Chart->AutoRange = false;  // Автоматический расчет диапазона выключен по умолчанию
	Chart->AutoMin = 0;
	Chart->AutoMax = 0;
	Chart->AutoRangeMargin = 5;  // 5% отступ по умолчанию
	for(int i=0; i<CHART_MARCERS;++i)
	{
		Chart->Marker[i].Enable = false;
		Chart->Marker[i].Pos = 0;
		Chart->Marker[i].Color = DEFAULT_COLOR_ACTIVE;
		Chart->Marker[i].Line = false;
		Chart->Marker[i].H = 5;
		Chart->Marker[i].LineType = SOLID_LINE;  // По умолчанию сплошная линия
	}
	for(int i=0; i<CHART_MARCERS;++i)
	{
		Chart->Strob[i].Enable = false;
		Chart->Strob[i].PosX = (Chart->lenx/2);
		Chart->Strob[i].PosY = ((Chart->Max-Chart->Min)*3)/4;
		Chart->Strob[i].Len = 20;
		Chart->Strob[i].H = 2;
		Chart->Strob[i].Color = DEFAULT_COLOR_ACTIVE;
	}
	
	Chart->packaging = ChartPackedData;			// Процедура упаковки по умолчанию
	return(Chart);
}
//***********************************************
void DrawingChart(_GSP_Chart *Chart)
{
	if(Chart->Visible)
	{
SetColor(Chart->BackColor);
		int lenx = Chart->lenx;
		int leny = Chart->leny;
		int x1 = Chart->x;
		int y1 = Chart->y;
		int x2 = Chart->x +lenx;
		int y2 = Chart->y +leny;
		int a_x = Active_Screen->x;
		int a_len_x = Active_Screen->lenx;
		int a_y = Active_Screen->y;
		int a_len_y = Active_Screen->leny;
		
		if( x2 > a_x+a_len_x) x2 = a_x+a_len_x;
		if( y2 > a_y+a_len_y) y2 = a_y+a_len_y;

SetClipRgn(x1, y1, x2, y2);
SetClip(true);
		if(Chart->BackColor != TRANSPARENT)
		{
SetColor(Chart->BackColor);
FillBevel(x1,y1,x2,y2, Chart->R);
		}
		// Вычисляем коэффициент масштабирования (используется в обоих режимах и для маркеров/стробов)
		int K = ((int)Chart->leny *10000)/(Chart->Max - Chart->Min);
		
		// Отрисовка в зависимости от режима
		if(Chart->Mode == CHART_MODE_HISTOGRAM_DYNAMIC || Chart->Mode == CHART_MODE_BAR_SHIFT)
		{
			// Режим столбчатой диаграммы
SetColor(Chart->CurveColor);
			int thinc = Chart->ThincCurve-1;
			
			int zerro = 0  + -Chart->Min;
			zerro = Chart->y + Chart->leny - (zerro*K)/10000+thinc;
			
			if(Chart->BarData && Chart->BarDataCount > 0)
			{
				if(Chart->Mode == CHART_MODE_HISTOGRAM_DYNAMIC)
				{
					// Режим динамической гистограммы: BarData содержит частоты попадания в каждый bin
					int num_bins = Chart->HistogramBins;
					if(num_bins <= 0) num_bins = Chart->lenx;  // Защита от некорректного значения
					
					// Находим максимальную частоту для нормализации
					short max_count = 0;
					for(int i = 0; i < num_bins; i++)
					{
						if(Chart->BarData[i] > max_count)
							max_count = Chart->BarData[i];
					}
					
					// Вычисляем ширину одного столбика: ширина виджета / количество bins
					int bar_width = (num_bins > 0) ? (Chart->lenx / num_bins) : 1;
					if(bar_width < 1) bar_width = 1;
					
					// Рисуем гистограмму: каждый столбец = частота попадания в соответствующий bin
					for(int i = 0; i < num_bins; i++)
					{
						// Вычисляем позицию X столбика
						// Каждый столбик занимает равную долю ширины виджета
						int bar_x_left = x1 + (i * Chart->lenx) / num_bins;
						int bar_x_right = x1 + ((i + 1) * Chart->lenx) / num_bins;
						
						// Проверяем границы
						if(bar_x_left >= x2) break;
						if(bar_x_right > x2) bar_x_right = x2;
						
						// Вычисляем высоту столбца на основе частоты
						short count = Chart->BarData[i];
						int bar_y, bar_bottom;
						
						if(max_count > 0)
						{
							// Нормализуем частоту к высоте виджета
							int bar_height = (count * Chart->leny) / max_count;
							
							if(Chart->InvertY)
							{
								// Инверсия оси Y: Min вверху, Max внизу
								bar_bottom = y1;  // Верх виджета
								bar_y = y1 + bar_height;  // Высота от верха
								
								// Ограничиваем высоту столбца
								if(bar_y > y2) bar_y = y2;
								
								// Рисуем заполненный столбец от верха вниз
								if(bar_y > bar_bottom)
								{
									// Используем Bar для рисования заполненного прямоугольника
Bar(bar_x_left, bar_bottom, bar_x_right - 1, bar_y);
								}
							}
							else
							{
								// Обычный режим: Min внизу, Max вверху
								bar_bottom = y2;  // Низ виджета
								bar_y = y2 - bar_height;  // Высота от низа
								
								// Ограничиваем высоту столбца
								if(bar_y < y1) bar_y = y1;
								
								// Рисуем заполненный столбец от низа вверх
								if(bar_y < bar_bottom)
								{
									// Используем Bar для рисования заполненного прямоугольника
Bar(bar_x_left, bar_y, bar_x_right - 1, bar_bottom);
								}
							}
						}
					}
				}
				else if(Chart->Mode == CHART_MODE_BAR_SHIFT)
				{
					// Режим со сдвигом: каждый столбец = 1 пиксель, данные сдвигаются влево
					int start_index = 0;
					int end_index = Chart->BarDataCount;
					
					// Если данных больше чем lenx, показываем только последние lenx
					if(Chart->BarDataCount > Chart->lenx)
					{
						start_index = Chart->BarDataCount - Chart->lenx;
						end_index = Chart->BarDataCount;
					}
					
					// Рисуем столбцы по одному пикселю на каждый
					for(int i = start_index; i < end_index; i++)
					{
						short value = Chart->BarData[i];
						int bar_x = x1 + (i - start_index);  // Позиция X = индекс от начала видимой области
						
						// Проверяем границы
						if(bar_x >= x2) break;
						
						// Вычисляем высоту столбца
						int bar_value = value + -Chart->Min;
						int bar_y, bar_bottom;
						
						if(Chart->InvertY)
						{
							// Инверсия оси Y: Min вверху, Max внизу (для BSCAN)
							bar_y = Chart->y + (bar_value*K)/10000+thinc;
							bar_bottom = y1;  // Верх виджета (Min)
							
							// Ограничиваем высоту столбца
							if(bar_y < y1) bar_y = y1;
							if(bar_y > y2) bar_y = y2;
							
							// Рисуем столбец от верха (Min) вниз к значению
							if(bar_y > bar_bottom)
							{
FastV_Line(bar_x, bar_bottom, bar_y, 1);
							}
						}
						else
						{
							// Обычный режим: Min внизу, Max вверху
							bar_y = Chart->y + Chart->leny - (bar_value*K)/10000+thinc;
							bar_bottom = (Chart->Min <= 0) ? zerro : y2;  // Низ виджета (Min)
							
							// Ограничиваем высоту столбца
							if(bar_y < y1) bar_y = y1;
							if(bar_y > y2) bar_y = y2;
							
							// Рисуем столбец от низа (Min) вверх к значению
							if(bar_y < bar_bottom)
							{
								// Значение положительное - рисуем вверх от нуля
FastV_Line(bar_x, bar_y, bar_bottom, 1);
							}
							else if(bar_y > bar_bottom)
							{
								// Значение отрицательное - рисуем вниз от нуля
FastV_Line(bar_x, bar_bottom, bar_y, 1);
							}
						}
					}
				}
			}
		}
		else
		{
			// Режим графика (по умолчанию)
			// Нарисовать сетку (только для графика)
		if(Chart->Greed)
		{
			int dx = Chart->lenx/Chart->N_X; 
			int dy = Chart->leny/Chart->N_Y; 
SetColor(Chart->GreedColor);
			for(int x = x1;x<x2;x+=dx) FastV_Line(x, y1, y2,Chart->Greed);
			for(int y = y1;y<y2;y+=dy) FastH_Line(y, x1, x2,Chart->Greed);			
		}
		// Упаковать данные
		if(Chart->packaging)
		{
			Chart->packaging(Chart->Data, Chart->LenData, Chart->DataView, Chart->lenx);
		}
		
		// Отрисовка графика
		int Max, Min;
SetColor(Chart->CurveColor);
		int thinc = Chart->ThincCurve-1;
		
		int old_max = Chart->DataView[0]  + -Chart->Min;
		old_max = Chart->y + Chart->leny - (old_max*K)/10000+thinc;
		int old_min = Chart->DataView[1]  + -Chart->Min;
		old_min = Chart->y + Chart->leny - (old_min*K)/10000+thinc;
		
		int zerro = 0  + -Chart->Min;
		zerro = Chart->y + Chart->leny - (zerro*K)/10000+thinc;
		if(Chart->DataView)
		{
			int i = 0;
			for(int x = x1;x<x2;++x)
			{
				Max = Chart->DataView[i++]  + -Chart->Min;
				Min = Chart->DataView[i++]  + -Chart->Min;
				Max = Chart->y + Chart->leny - (Max*K)/10000+thinc;
				Min = Chart->y + Chart->leny - (Min*K)/10000 -thinc;
FastV_Line(x, Min, Max,1);
				if(Chart->Fill) FastV_Line(x, Min, zerro,1);
				else
				{
FastV_Line(x, Max, old_min,1);
				}
				old_max = Max;
				old_min = Min;
			   	//Line(x,Min,x,Max);
				}
			}
		}
		// Отрисовка маркера
		for(int n = 0; n< CHART_MARCERS;++n)
		{
			if(Chart->Marker[n].Enable)
			{
SetColor(Chart->Marker[n].Color);
				if(Chart->Mode == CHART_MODE_HISTOGRAM_DYNAMIC || Chart->Mode == CHART_MODE_BAR_SHIFT)
				{
					// В режиме диаграммы маркеры горизонтальные
					// Marker[n].Pos интерпретируется как значение (Min..Max)
					int marker_value = Chart->Marker[n].Pos + -Chart->Min;
					int y_pos;
					
					if(Chart->InvertY)
					{
						// Инверсия оси Y: Min вверху, Max внизу
						y_pos = Chart->y + (marker_value*K)/10000;
					}
					else
					{
						// Обычный режим: Min внизу, Max вверху
						y_pos = Chart->y + Chart->leny - (marker_value*K)/10000;
					}
					
					// Ограничиваем позицию маркера границами виджета
					if(y_pos < y1) y_pos = y1;
					if(y_pos > y2) y_pos = y2;
					
					int h;
					int H2 = Chart->Marker[n].H*2;
					int y = y_pos - y1 - Chart->Marker[n].H;  // Относительная позиция от y1
					
					for(int i = 0;i<H2;++i) 
					{
						if(i<Chart->Marker[n].H) h = i;
						else				     h = H2-i;
FastH_Line(y1+y+i, x1, x1+h,1);
					}
					if(Chart->Marker[n].Line)
					{
SetLineType(Chart->Marker[n].LineType);
FastH_Line(y1+(y_pos-y1), x1, x2,2);
SetLineType(GSP_SOLID_LINE);  // Восстанавливаем сплошную линию
					}
				}
				else
				{
					// В режиме графика маркеры вертикальные
				int h;
				int H2 = Chart->Marker[n].H*2;
				int pos = (Chart->lenx * Chart->Marker[n].Pos)/Chart->LenData; 
				int x = pos - Chart->Marker[n].H;
					
				for(int i = 0;i<H2;++i) 
				{
					if(i<Chart->Marker[n].H) h = i;
					else				     h = H2-i;
FastV_Line(x1+x+i, y1, y1+h,1);
				}
					if(Chart->Marker[n].Line)
					{
SetLineType(Chart->Marker[n].LineType);
FastV_Line(x1+pos, y1, y2,2);
SetLineType(GSP_SOLID_LINE);  // Восстанавливаем сплошную линию
					}
				}
			}
		}
		// Стробы только для режима графика
		if(Chart->Mode == CHART_MODE_GRAPH)
		{
		for(int n = 0; n< CHART_MARCERS;++n)
		{
			if(Chart->Strob[n].Enable)
			{
SetColor(Chart->Strob[n].Color);
				int x = x1+Chart->Strob[n].PosX;
				int y = Chart->Strob[n].PosY + -Chart->Min;
				y = y1 + Chart->leny - (y*K)/10000;
				for(int i = 0; i<Chart->Strob[n].H;++i)
				{
FastH_Line(y+i, x, x+Chart->Strob[n].Len,1);
					}
				}
			}
		}
		// 
		if(Chart->WidthBorder)
		{
SetLineThickness(Chart->WidthBorder-1);
SetColor(Chart->BorderColor);
Bevel(x1,y1,x2,y2, Chart->R);
		}
SetClip(false);
	}
}
// *************************************************
void ChartUpdateViewData(_GSP_Chart *Chart, void (*proc)(short *, int, short *, int))
{
	if(Chart->DataView)
	{
		proc(Chart->Data, Chart->LenData, Chart->DataView, Chart->lenx);
	}
}
// *************************************************
void ChartAddData(_GSP_Chart *Chart, short *Data, int len)
{
	Chart->LenData = len;											
	Chart->Data = Data;												
}
//*****************************************************
void ChartSetProcPack(_GSP_Chart *Chart, void (*proc)(short *, int, short *, int))
{
	Chart->packaging = proc;
}
// *************************************************
int  ChartGetLenX(_GSP_Chart *Chart)
{
	if(Chart) return(Chart->lenx);
	else	  return(0);	
}
//*****************************************************
void ChartSetMode(_GSP_Chart *Chart, short mode)
{
	if(!Chart) return;
	
	Chart->Mode = mode;
	
	// При переключении в режим столбчатой диаграммы инициализируем массив данных
	if((mode == CHART_MODE_HISTOGRAM_DYNAMIC || mode == CHART_MODE_BAR_SHIFT) && Chart->BarData == NULL)
	{
		// Для гистограммы используем HistogramBins, для BAR_SHIFT - lenx
		int data_size = (mode == CHART_MODE_HISTOGRAM_DYNAMIC) ? Chart->HistogramBins : Chart->lenx;
		if(data_size <= 0) data_size = Chart->lenx;  // Защита от некорректного значения
		
		Chart->BarData = user_malloc(data_size * sizeof(short));
		if(Chart->BarData)
		{
			memset(Chart->BarData, 0, data_size * sizeof(short));
			Chart->BarDataCount = 0;
		}
	}
}
//*****************************************************
void ChartSetWidgetAlign(_GSP_Chart *Chart, int align)
{
	if(!Chart) return;
	
	Chart->WidgetAlign = align;
	
	// Находим родительский Screen для пересчета координат
	_GSP_Screen *Screen = Get_Active_Screen();
	if(!Screen) return;
	
	// Пересчитываем абсолютные координаты с учетом нового выравнивания
	int abs_x = Chart->rel_x;
	int abs_y = Chart->rel_y;
	CalculateWidgetPosition(Screen, &abs_x, &abs_y, Chart->lenx, Chart->leny, align);
	Chart->x = abs_x;
	Chart->y = abs_y;
}
//*****************************************************
void ChartSetPos(_GSP_Chart *Chart, int x, int y)
{
	if(!Chart) return;
	
	// Обновляем относительные координаты
	Chart->rel_x = x;
	Chart->rel_y = y;
	
	// Пересчитываем абсолютные координаты с учетом текущего выравнивания
	_GSP_Screen *Screen = Get_Active_Screen();
	if(Screen)
	{
		int abs_x = x;
		int abs_y = y;
		CalculateWidgetPosition(Screen, &abs_x, &abs_y, Chart->lenx, Chart->leny, Chart->WidgetAlign);
		Chart->x = abs_x;
		Chart->y = abs_y;
	}
}
//*****************************************************
void ChartSetSize(_GSP_Chart *Chart, int lenx, int leny)
{
	if(!Chart) return;
	
	// Обновляем размер виджета
	Chart->lenx = lenx;
	Chart->leny = leny;
	
	// Если включен автоматический расчет bins, пересчитываем количество bins
	if(Chart->AutoBins && Chart->Mode == CHART_MODE_HISTOGRAM_DYNAMIC)
	{
		int num_bins = (Chart->MinBarWidth > 0) ? (lenx / Chart->MinBarWidth) : lenx;
		if(num_bins < 1) num_bins = 1;
		if(num_bins > lenx) num_bins = lenx;
		
		// Если количество bins изменилось, пересоздаем массив данных
		if(Chart->HistogramBins != num_bins && Chart->BarData)
		{
			user_free(Chart->BarData);
			Chart->BarData = user_malloc(num_bins * sizeof(short));
			if(Chart->BarData)
			{
				memset(Chart->BarData, 0, num_bins * sizeof(short));
				Chart->BarDataCount = 0;
			}
		}
		Chart->HistogramBins = num_bins;
	}
	
	// Пересчитываем абсолютные координаты с учетом текущего выравнивания
	// (при изменении размера позиция может измениться в зависимости от выравнивания)
	_GSP_Screen *Screen = Get_Active_Screen();
	if(Screen)
	{
		int abs_x = Chart->rel_x;
		int abs_y = Chart->rel_y;
		CalculateWidgetPosition(Screen, &abs_x, &abs_y, lenx, leny, Chart->WidgetAlign);
		Chart->x = abs_x;
		Chart->y = abs_y;
	}
}
//*****************************************************
void ChartSetInvertY(_GSP_Chart *Chart, bool invert)
{
	if(!Chart) return;
	Chart->InvertY = invert;
}
//*****************************************************
void ChartSetAutoRange(_GSP_Chart *Chart, bool enable, short margin_percent)
{
	if(!Chart) return;
	
	Chart->AutoRange = enable;
	
	// Ограничиваем margin_percent в разумных пределах (0-50%)
	if(margin_percent < 0) margin_percent = 0;
	if(margin_percent > 50) margin_percent = 50;
	Chart->AutoRangeMargin = margin_percent;
	
	// При включении автоматического режима сбрасываем отслеживаемые значения
	if(enable)
	{
		Chart->AutoMin = 0;
		Chart->AutoMax = 0;
		// Очищаем гистограмму для пересчета с новым диапазоном
		if(Chart->BarData && Chart->Mode == CHART_MODE_HISTOGRAM_DYNAMIC)
		{
			int data_size = Chart->HistogramBins;
			if(data_size <= 0) data_size = Chart->lenx;
			memset(Chart->BarData, 0, data_size * sizeof(short));
			Chart->BarDataCount = 0;
		}
	}
}
//*****************************************************
void ChartResetHistogram(_GSP_Chart *Chart)
{
	if(!Chart) return;
	
	// Сбрасываем гистограмму: очищаем все счетчики
	if(Chart->BarData && Chart->Mode == CHART_MODE_HISTOGRAM_DYNAMIC)
	{
		int data_size = Chart->HistogramBins;
		if(data_size <= 0) data_size = Chart->lenx;
		memset(Chart->BarData, 0, data_size * sizeof(short));
		Chart->BarDataCount = 0;
	}
	
	// Также сбрасываем отслеживаемые значения для AutoRange
	if(Chart->AutoRange)
	{
		Chart->AutoMin = 0;
		Chart->AutoMax = 0;
	}
}
//*****************************************************
void ChartSetMinMax(_GSP_Chart *Chart, short min, short max)
{
	if(!Chart) return;
	
	// Проверяем корректность диапазона
	if(min >= max)
	{
		// Некорректный диапазон - используем значения по умолчанию
		Chart->Min = 0;
		Chart->Max = 1000;
		return;
	}
	
	// Проверяем, изменился ли диапазон
	bool range_changed = (Chart->Min != min || Chart->Max != max);
	
	Chart->Min = min;
	Chart->Max = max;
	
	// Если диапазон изменился и включен режим гистограммы, очищаем данные
	if(range_changed && Chart->BarData && Chart->Mode == CHART_MODE_HISTOGRAM_DYNAMIC)
	{
		int data_size = Chart->HistogramBins;
		if(data_size <= 0) data_size = Chart->lenx;
		memset(Chart->BarData, 0, data_size * sizeof(short));
		Chart->BarDataCount = 0;
	}
}
//*****************************************************
void ChartSetHistogramBins(_GSP_Chart *Chart, short num_bins)
{
	if(!Chart) return;
	
	// Если включен автоматический режим, отключаем его при ручной установке
	if(Chart->AutoBins)
	{
		Chart->AutoBins = false;
	}
	
	// Проверяем корректность количества bins
	if(num_bins <= 0) num_bins = Chart->lenx;  // По умолчанию = ширина виджета
	if(num_bins > Chart->lenx) num_bins = Chart->lenx;  // Не больше ширины виджета
	
	// Проверяем, изменилось ли количество bins
	bool bins_changed = (Chart->HistogramBins != num_bins);
	
	Chart->HistogramBins = num_bins;
	
	// Если количество bins изменилось и массив уже создан, пересоздаем его
	if(bins_changed && Chart->BarData && Chart->Mode == CHART_MODE_HISTOGRAM_DYNAMIC)
	{
		user_free(Chart->BarData);
		Chart->BarData = user_malloc(num_bins * sizeof(short));
		if(Chart->BarData)
		{
			memset(Chart->BarData, 0, num_bins * sizeof(short));
			Chart->BarDataCount = 0;
		}
	}
}
//*****************************************************
void ChartSetAutoBins(_GSP_Chart *Chart, bool enable, short min_bar_width)
{
	if(!Chart) return;
	
	Chart->AutoBins = enable;
	
	// Ограничиваем минимальную ширину столбика в разумных пределах (1-20 пикселей)
	if(min_bar_width < 1) min_bar_width = 1;
	if(min_bar_width > 20) min_bar_width = 20;
	Chart->MinBarWidth = min_bar_width;
	
	// Если включаем автоматический режим, рассчитываем количество bins
	if(enable && Chart->Mode == CHART_MODE_HISTOGRAM_DYNAMIC)
	{
		int num_bins = (min_bar_width > 0) ? (Chart->lenx / min_bar_width) : Chart->lenx;
		if(num_bins < 1) num_bins = 1;
		if(num_bins > Chart->lenx) num_bins = Chart->lenx;
		
		// Если количество bins изменилось, пересоздаем массив данных
		if(Chart->HistogramBins != num_bins && Chart->BarData)
		{
			user_free(Chart->BarData);
			Chart->BarData = user_malloc(num_bins * sizeof(short));
			if(Chart->BarData)
			{
				memset(Chart->BarData, 0, num_bins * sizeof(short));
				Chart->BarDataCount = 0;
			}
		}
		Chart->HistogramBins = num_bins;
	}
}
//*****************************************************
void ChartAddBarData(_GSP_Chart *Chart, short value)
{
	if(!Chart || (Chart->Mode != CHART_MODE_HISTOGRAM_DYNAMIC && Chart->Mode != CHART_MODE_BAR_SHIFT)) return;
	
	// Автоматический расчет Min/Max при включенном режиме AutoRange
	if(Chart->AutoRange)
	{
		if(Chart->BarDataCount == 0)
		{
			// Первое значение - инициализируем диапазон
			Chart->AutoMin = value;
			Chart->AutoMax = value;
			
			// Устанавливаем начальный диапазон с небольшим отступом
			// Для первого значения создаем небольшой диапазон вокруг него
			short initial_range = 10;  // Минимальный начальный диапазон
			if(value != 0)
			{
				// Вычисляем отступ от значения (используем безопасный способ)
				short abs_value = (value < 0) ? -value : value;
				initial_range = (abs_value * Chart->AutoRangeMargin) / 100;
				if(initial_range < 10) initial_range = 10;  // Минимальный диапазон
			}
			
			Chart->Min = value - initial_range;
			Chart->Max = value + initial_range;
		}
		else
		{
			// Обновляем отслеживаемый диапазон
			if(value < Chart->AutoMin) Chart->AutoMin = value;
			if(value > Chart->AutoMax) Chart->AutoMax = value;
			
			// Вычисляем новый диапазон с учетом отступа
			short range = Chart->AutoMax - Chart->AutoMin;
			if(range > 0)
			{
				short margin = (range * Chart->AutoRangeMargin) / 100;
				short new_min = Chart->AutoMin - margin;
				short new_max = Chart->AutoMax + margin;
				
				// Проверяем, изменился ли диапазон значительно (более чем на 10% от старого диапазона)
				short old_range = Chart->Max - Chart->Min;
				bool range_changed = false;
				
				if(old_range <= 0)
				{
					// Старый диапазон некорректен - обновляем
					range_changed = true;
				}
				else
				{
					// Проверяем изменение границ (более 10% от старого диапазона)
					short min_change = (new_min < Chart->Min) ? (Chart->Min - new_min) : 0;
					short max_change = (new_max > Chart->Max) ? (new_max - Chart->Max) : 0;
					
					if(min_change * 100 / old_range > 10 || max_change * 100 / old_range > 10)
					{
						range_changed = true;
					}
				}
				
				// Если диапазон изменился значительно, обновляем Min/Max и очищаем гистограмму
				if(range_changed)
				{
					Chart->Min = new_min;
					Chart->Max = new_max;
					
					// Очищаем гистограмму, так как bins изменились
					if(Chart->BarData && Chart->Mode == CHART_MODE_HISTOGRAM_DYNAMIC)
					{
						int data_size = Chart->HistogramBins;
						if(data_size <= 0) data_size = Chart->lenx;
						memset(Chart->BarData, 0, data_size * sizeof(short));
						Chart->BarDataCount = 0;
					}
				}
			}
		}
	}
	
	if(Chart->BarData == NULL)
	{
		// Инициализируем массив, если еще не создан
		// Для режима BAR и HISTOGRAM_DYNAMIC нужен массив
		// Для BAR: хранит отдельные значения (размер = lenx)
		// Для HISTOGRAM_DYNAMIC: хранит частоты попадания в каждый bin (размер = HistogramBins)
		int data_size = (Chart->Mode == CHART_MODE_HISTOGRAM_DYNAMIC) ? Chart->HistogramBins : Chart->lenx;
		if(data_size <= 0) data_size = Chart->lenx;  // Защита от некорректного значения
		
		Chart->BarData = user_malloc(data_size * sizeof(short));
		if(Chart->BarData == NULL) return;
		memset(Chart->BarData, 0, data_size * sizeof(short));
		Chart->BarDataCount = 0;
	}
	
	if(Chart->Mode == CHART_MODE_HISTOGRAM_DYNAMIC)
	{
		// Режим динамической гистограммы: BarData содержит частоты попадания в каждый bin
		int num_bins = Chart->HistogramBins;
		if(num_bins <= 0) num_bins = Chart->lenx;  // Защита от некорректного значения
		
		// Определяем в какой bin попадает значение
		int range = Chart->Max - Chart->Min;
		if(range > 0 && num_bins > 0)
		{
			// Ограничиваем значение в пределах Min..Max
			if(value < Chart->Min) value = Chart->Min;
			if(value > Chart->Max) value = Chart->Max;
			
			// Вычисляем индекс bin: (value - Min) / range * num_bins
			int bin_index = ((value - Chart->Min) * num_bins) / range;
			
			// Ограничиваем индекс
			if(bin_index < 0) bin_index = 0;
			if(bin_index >= num_bins) bin_index = num_bins - 1;
			
			// Увеличиваем счетчик соответствующего bin
			// Накопление происходит постоянно без затухания
			if(Chart->BarData[bin_index] < 32767)  // Защита от переполнения
			{
				Chart->BarData[bin_index]++;
			}
			
			// Увеличиваем общий счетчик добавленных значений
			Chart->BarDataCount++;
		}
	}
	else if(Chart->Mode == CHART_MODE_BAR_SHIFT)
	{
		// Режим со сдвигом: каждый столбец = 1 пиксель, данные сдвигаются влево
		// Если данных уже lenx, сдвигаем все влево и добавляем новое справа
		if(Chart->BarDataCount >= Chart->lenx)
		{
			// Массив заполнен - сдвигаем все данные влево на одну позицию
			for(int i = 0; i < Chart->lenx - 1; i++)
			{
				Chart->BarData[i] = Chart->BarData[i + 1];
			}
			// Добавляем новое значение в конец (позиция lenx-1)
			Chart->BarData[Chart->lenx - 1] = value;
			// Количество не меняется, так как массив уже заполнен
		}
		else
		{
			// Массив еще не заполнен, просто добавляем новое значение
			Chart->BarData[Chart->BarDataCount] = value;
			Chart->BarDataCount++;
		}
	}
}
//*****************************************************
// МАКСИМАЛЬНО ОПТИМИЗИРОВАННАЯ версия ChartPackedData (приоритет: скорость)
// Агрессивные оптимизации:
// 1. Предвычисление всех границ групп - один проход вместо проверок на каждой итерации
// 2. Указатели вместо индексов - быстрее доступ к памяти
// 3. else if вместо двух if - ~50% экономии сравнений
// 4. Развертка циклов для малых групп (1-4 элемента) - убираем цикл полностью
// 5. Локальные переменные - лучше для кэша процессора
// 6. Минимизация проверок - предвычисляем все заранее
// 7. Кэширование значений - избегаем повторного чтения из памяти
static void ChartPackedData(short *In_Data, int len_in, short *Out_Data, int len_out)
{
	if(!In_Data || !Out_Data || len_in <= 0 || len_out <= 0) return;
	
	const int Len_Group = len_in / len_out;
	const int remainder = len_in % len_out;
	const int Koef = (remainder * 100) / len_out;
	
	short *in_ptr = In_Data;
	short *out_ptr = Out_Data;
	short *in_end = In_Data + len_in;
	int Sum_Koef = 0;
	
	// Обрабатываем каждую группу
	for(int group = 0; group < len_out && in_ptr < in_end; group++)
	{
		// Вычисляем размер текущей группы
		int group_size = Len_Group;
		Sum_Koef += Koef;
		if(Sum_Koef >= 100)
		{
			group_size++;
			Sum_Koef -= 100;
		}
		
		// Ограничиваем размер группы оставшимися данными
		int remaining = (int)(in_end - in_ptr);
		if(group_size > remaining)
			group_size = remaining;
		
		// Оптимизация: развертка цикла для малых групп (убираем цикл полностью)
		short max_val, min_val;
		
		if(group_size == 1)
		{
			// Группа из 1 элемента - нет циклов, нет сравнений
			max_val = min_val = *in_ptr++;
		}
		else if(group_size == 2)
		{
			// Группа из 2 элементов - развернутый код
			short v1 = *in_ptr++;
			short v2 = *in_ptr++;
			max_val = (v1 > v2) ? v1 : v2;
			min_val = (v1 < v2) ? v1 : v2;
		}
		else if(group_size == 3)
		{
			// Группа из 3 элементов - развернутый код
			short v1 = *in_ptr++;
			short v2 = *in_ptr++;
			short v3 = *in_ptr++;
			max_val = (v1 > v2) ? v1 : v2;
			if(v3 > max_val) max_val = v3;
			min_val = (v1 < v2) ? v1 : v2;
			if(v3 < min_val) min_val = v3;
		}
		else if(group_size == 4)
        {
			// Группа из 4 элементов - развернутый код
			short v1 = *in_ptr++;
			short v2 = *in_ptr++;
			short v3 = *in_ptr++;
			short v4 = *in_ptr++;
			max_val = (v1 > v2) ? v1 : v2;
			short max34 = (v3 > v4) ? v3 : v4;
			if(max34 > max_val) max_val = max34;
			min_val = (v1 < v2) ? v1 : v2;
			short min34 = (v3 < v4) ? v3 : v4;
			if(min34 < min_val) min_val = min34;
		}
		else
		{
			// Группа из 5+ элементов - используем оптимизированный цикл
			max_val = *in_ptr;
			min_val = *in_ptr;
			short *group_end = in_ptr + group_size;
			
			// Обработка группы с оптимизацией сравнений
			while(++in_ptr < group_end)
			{
				short val = *in_ptr;  // Кэшируем значение
				if(val > max_val) 
					max_val = val;
				else if(val < min_val)  // else if - экономия проверок
					min_val = val;
			}
		}
		
		// Записываем результат
		*out_ptr++ = max_val;
		*out_ptr++ = min_val;
	}
}

#endif