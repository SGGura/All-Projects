#include <p32xxxx.h>
#include <xc.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include "GUI_GSP.h"
#include "GSP_DrawText.h"
#include "GSP_mem_monitor.h"
#include "GSP_JSON.h"
#ifdef USED_MESSAGE
#include "GSP_Message.h"
#endif



_GSP_Screen	*Active_Screen = NULL;	

#ifdef USED_TIMER	
_GSP_Timers *Active_Timers = NULL;
#endif

lv_font_fmt_txt_dsc_t	*Active_Font = NULL;

unsigned int Timer_GUI = 0;

// Механизм безопасного переключения экранов
static struct
{
	bool need_switch;              // Флаг необходимости переключения экрана
	ScreenCreateFunc create_func;   // Функция создания нового экрана
} ScreenSwitch = {false, NULL};
//*****************************************************************
void GUI_TimerClock(int ms)
{
	Timer_GUI += ms;
}
//****************************************************
void Set_Active_Screen(_GSP_Screen *Screen)
{
	Active_Screen = Screen;
}
//****************************************************
_GSP_Screen *Get_Active_Screen(void)
{
	return(Active_Screen);
}
//****************************************************
// Вспомогательная функция для вычисления координат виджета с учетом выравнивания
void CalculateWidgetPosition(_GSP_Screen *Screen, int *x, int *y, int lenx, int leny, int widget_align)
{
	int rel_x = *x;
	int rel_y = *y;
	
	// Получаем параметры Screen
	int screen_x = Screen->x;
	int screen_y = Screen->y;
	int screen_w = Screen->lenx;
	int screen_h = Screen->leny;
	
	// Если координаты 0,0, то позиционируем по выравниванию (без смещения)
	if(rel_x == 0 && rel_y == 0)
	{
		switch(widget_align)
		{
			case GSP_WIDGET_ALIGN_TOP_LEFT:
				*x = screen_x;
				*y = screen_y;
				break;
			case GSP_WIDGET_ALIGN_TOP_CENTER:
				*x = screen_x + (screen_w - lenx) / 2;
				*y = screen_y;
				break;
			case GSP_WIDGET_ALIGN_TOP_RIGHT:
				*x = screen_x + screen_w - lenx;
				*y = screen_y;
				break;
			case GSP_WIDGET_ALIGN_MIDDLE_LEFT:
				*x = screen_x;
				*y = screen_y + (screen_h - leny) / 2;
				break;
			case GSP_WIDGET_ALIGN_MIDDLE_CENTER:
				*x = screen_x + (screen_w - lenx) / 2;
				*y = screen_y + (screen_h - leny) / 2;
				break;
			case GSP_WIDGET_ALIGN_MIDDLE_RIGHT:
				*x = screen_x + screen_w - lenx;
				*y = screen_y + (screen_h - leny) / 2;
				break;
			case GSP_WIDGET_ALIGN_BOTTOM_LEFT:
				*x = screen_x;
				*y = screen_y + screen_h - leny;
				break;
			case GSP_WIDGET_ALIGN_BOTTOM_CENTER:
				*x = screen_x + (screen_w - lenx) / 2;
				*y = screen_y + screen_h - leny;
				break;
			case GSP_WIDGET_ALIGN_BOTTOM_RIGHT:
				*x = screen_x + screen_w - lenx;
				*y = screen_y + screen_h - leny;
				break;
			default: // По умолчанию TOP_LEFT
				*x = screen_x;
				*y = screen_y;
				break;
		}
	}
	else
	{
		// Используем координаты как смещение от точки выравнивания
		switch(widget_align)
		{
			case GSP_WIDGET_ALIGN_TOP_LEFT:
				*x = screen_x + rel_x;
				*y = screen_y + rel_y;
				break;
			case GSP_WIDGET_ALIGN_TOP_CENTER:
				*x = screen_x + (screen_w - lenx) / 2 + rel_x;
				*y = screen_y + rel_y;
				break;
			case GSP_WIDGET_ALIGN_TOP_RIGHT:
				*x = screen_x + screen_w - lenx + rel_x;
				*y = screen_y + rel_y;
				break;
			case GSP_WIDGET_ALIGN_MIDDLE_LEFT:
				*x = screen_x + rel_x;
				*y = screen_y + (screen_h - leny) / 2 + rel_y;
				break;
			case GSP_WIDGET_ALIGN_MIDDLE_CENTER:
				*x = screen_x + (screen_w - lenx) / 2 + rel_x;
				*y = screen_y + (screen_h - leny) / 2 + rel_y;
				break;
			case GSP_WIDGET_ALIGN_MIDDLE_RIGHT:
				*x = screen_x + screen_w - lenx + rel_x;
				*y = screen_y + (screen_h - leny) / 2 + rel_y;
				break;
			case GSP_WIDGET_ALIGN_BOTTOM_LEFT:
				*x = screen_x + rel_x;
				*y = screen_y + screen_h - leny + rel_y;
				break;
			case GSP_WIDGET_ALIGN_BOTTOM_CENTER:
				*x = screen_x + (screen_w - lenx) / 2 + rel_x;
				*y = screen_y + screen_h - leny + rel_y;
				break;
			case GSP_WIDGET_ALIGN_BOTTOM_RIGHT:
				*x = screen_x + screen_w - lenx + rel_x;
				*y = screen_y + screen_h - leny + rel_y;
				break;
			default: // По умолчанию TOP_LEFT (старое поведение)
				*x = screen_x + rel_x;
				*y = screen_y + rel_y;
				break;
		}
	}
}
//****************************************************
void ScreenSetKeyHandler(_GSP_Screen *Screen, void (*KeyHandler)(void))
{
	if(Screen != NULL)
	{
		Screen->KeyHandler = KeyHandler;
	}
}
//*****************************************************************
_GSP_Screen *Crate_Screen(int x, int y, int lenx, int leny, int ID)
{
	_GSP_Screen *Screen = user_malloc(sizeof(_GSP_Screen));
	Screen->x = x;
	Screen->y = y;
	Screen->lenx = lenx;
	Screen->leny = leny;
	if((Screen->x+Screen->lenx) > (SCREEN_HOR_SIZE-1)) Screen->lenx = (SCREEN_HOR_SIZE-1) -  Screen->x;
	if((Screen->y+Screen->leny) > (SCREEN_VER_SIZE-1)) Screen->leny = (SCREEN_VER_SIZE-1) -  Screen->y;
	Screen->ID = ID;
	Screen->N = 0;
	Screen->BorderColor = WHITE;
	Screen->WidthBorder = 0;
	Screen->Type = GSP_SCREEN;
	Screen->BackColor = DEFAULT_COLOR_SCREEN;
	Screen->KeyHandler = NULL;  // По умолчанию указатель равен NULL
	
#ifdef USED_TIMER
	// Инициализация полей для привязанных таймеров
	Screen->N_Timers = 0;
	Screen->ListTimers = NULL;
#endif
	Active_Screen = Screen;
	return(Screen);
}
//***************************************
static bool GUI_Update(unsigned int period)
{
	bool update = false;
	static unsigned int Old_Timer = 0;
	if(labs(Timer_GUI - Old_Timer) >= period)
	{
		Old_Timer = Timer_GUI;
		update = true;
	}
	return(update);
}
//*************************************************
int GUI_DeleteScreen(_GSP_Screen *Screen)
{
	int err;
	if(Screen == NULL) return(GSP_ERR_WIDGET);
	
#ifdef USED_TIMER
	// НОВОЕ: Автоматическое удаление всех привязанных таймеров
	DeleteAllTimers(Screen);
#endif
	
	do
	{
		err = GUI_DeleteWidget(Screen, Screen->ListWidgets[0]);	
//	}while(err!= GSP_NO_WIDGET);
	}while(Screen->N);
	if(Active_Screen == Screen) Active_Screen = NULL;
	user_free(Screen->ListWidgets);
	Screen->ListWidgets = NULL;
	user_free(Screen);
	Screen = NULL;
	
	// Освобождаем память, выделенную через Parse_JSON
	GSP_JSON_FreeResult();
	
	return(GSP_OK);
}
//*************************************************
int GUI_DeleteWidget(_GSP_Screen *Screen, void *Widget)
{
	int f_n = -1;
	if(Widget == NULL)	return(GSP_ERR_WIDGET);
	
	for(int i = 0; i<Screen->N;++i)
	{
		if(Widget == Screen->ListWidgets[i]) {f_n = i; break;}
	}
	if(f_n >=0)
	{
		unsigned char type = *(char *)Widget; 
		switch(type)
		{
#ifdef USED_RULER
			case GSP_RULER:
			{
				_GSP_Ruler *Ruler = Widget;
				RulerDelete(Ruler);
				user_free(Widget);	
				Widget = NULL;
				break;
			}
#endif			 
#ifdef USED_LABEL
			case GSP_LABEL:
			{
				_GSP_Label *Label = Widget;
				if(!Label->Static)	user_free((char *)Label->Text);
				user_free(Widget);	
				Widget = NULL;
				break;
			}
#endif
#ifdef USED_MESSAGE
			case GSP_MESSAGE:
			{
				_GSP_Message *Message = Widget;
				if(!Message->Static) user_free((char *)Message->Text);
				user_free(Widget);
				Widget = NULL;
				break;
			}
#endif
		#ifdef USED_LIST
			case GSP_LIST:
			{
				_GSP_List *List = Widget;
				// Используем ListClear для освобождения всей памяти элементов
				ListClear(List);
				// Освобождаем память для самой структуры виджета
				user_free(Widget);
				Widget = NULL;
				break;
			}
		#endif
		#ifdef USED_KEYBOARD
			case GSP_KEYBOARD:
			{
				_GSP_Keyboard *Keyboard = Widget;
				// Освобождаем память для массива длин строк
				if(Keyboard->row_lengths) user_free(Keyboard->row_lengths);
				// Освобождаем память для массива множителей ширины кнопок
				if(Keyboard->button_width_multipliers) user_free(Keyboard->button_width_multipliers);
				// Примечание: Keyboard->buttons не освобождается, так как это внешний указатель
				// Освобождаем память для самой структуры виджета
				user_free(Widget);
				Widget = NULL;
				break;
			}
		#endif
#ifdef USED_CHART
			case GSP_CHART:
			{
				_GSP_Chart *Chart = Widget;
				user_free(Chart->DataView);
				if(Chart->BarData) user_free(Chart->BarData);
				user_free(Widget);	
				Widget = NULL;
				break;
			}
#endif
#ifdef USED_TABLE
			case GSP_TABLE:
			{
				_GSP_Table *Table = Widget;
				TableDelete(Table);
				user_free(Widget);	
				Widget = NULL;
				break;
			}
#endif			 
#ifdef USED_IMAGE
			case GSP_IMAGE:
			{
				_GSP_Image *Image = Widget;
				user_free(Widget);
				Widget = NULL;
				break;
			}
#endif
#ifdef USED_LINE
			case GSP_LINE:
			{
				_GSP_PolyLine *Line = Widget;
				user_free(Line->points);
				user_free(Widget);	
				Widget = NULL;
				break;
			}
#endif
#ifdef USED_PROGRESSBAR
			case GSP_BAR:
			{
				_GSP_ProgressBar *Bar = Widget;
				user_free(Widget);	
				Widget = NULL;
				break;
			}
#endif
#ifdef USED_SPINNER
			case GSP_SPINNER:
			{
				user_free(Widget);
				Widget = NULL;
				break;
			}
#endif
#ifdef USED_PANEL
			case GSP_PANEL:
			{
				user_free(Widget);
				Widget = NULL;
				break;
			}
#endif
		}
		for(int i = f_n;i<Screen->N;++i) Screen->ListWidgets[i] = Screen->ListWidgets[i+1];		// перестроить список виджетов 
		Screen->N--;
		if(Screen->N > 0)
		{
			int **new_list = user_realloc(Screen->ListWidgets, sizeof(int *)*Screen->N);
			if(new_list)
			{
				Screen->ListWidgets = new_list;
			}
		}
		else
		{
			// Если виджетов не осталось, освобождаем массив
			user_free(Screen->ListWidgets);
			Screen->ListWidgets = NULL;
		}
	}
	else return(GSP_NO_WIDGET);
	return(GSP_OK);
}
#ifdef USED_TIMER
//*************************************************
int GUI_DeleteTimer(void *Timer)
{
	int f_n = -1;
	if(Timer == NULL)	return(GSP_ERR_WIDGET);
	
	for(int i = 0; i<Active_Timers->N;++i)
	{
		if(Timer == Active_Timers->ListTimers[i]) {f_n = i; break;}
	}
	if(f_n >=0)
	{
		user_free(Timer);	
		for(int i = f_n;i<Active_Timers->N;++i) Active_Timers->ListTimers[i] = Active_Timers->ListTimers[i+1];	// перестроить список 
		Active_Timers->N--;
		if(Active_Timers->N > 0)
		{
			int **new_list = user_realloc(Active_Timers->ListTimers, sizeof(int *)*Active_Timers->N);
			if(new_list)
			{
				Active_Timers->ListTimers = new_list;
			}
			// Если new_list == NULL, оставляем старый указатель (память не освобождается, но это лучше чем потерять указатель)
		}
		else
		{
			// Если таймеров не осталось, освобождаем список
			if(Active_Timers->ListTimers)
			{
				user_free(Active_Timers->ListTimers);
				Active_Timers->ListTimers = NULL;
			}
		}
	}
	else return(GSP_NO_WIDGET);
	return(GSP_OK);
}
#endif

//*****************************************************
void IncrementListWidgets(_GSP_Screen *Screen, void *Widget)
{
	if(!Screen || !Widget) return;
	
	int **new_list;
	if(Screen->N == 0)
	{
		new_list = user_malloc(sizeof(int *));
	}
	else
	{
		new_list = user_realloc(Screen->ListWidgets, sizeof(int *)*(Screen->N+1));
	}
	
	if(new_list)
	{
		Screen->ListWidgets = new_list;
	Screen->ListWidgets[Screen->N++] = (int *)Widget;
	}
}
#ifdef USED_TIMER
//*****************************************************
void IncrementListTimers(void *Timer)
{
	if(!Active_Timers || !Timer) return;
	
	int **new_list;
	if(Active_Timers->N == 0)
	{
		new_list = user_malloc(sizeof(int *));
	}
	else
	{
		new_list = user_realloc(Active_Timers->ListTimers, sizeof(int *)*(Active_Timers->N+1));
	}
	
	if(new_list)
	{
		Active_Timers->ListTimers = new_list;
	Active_Timers->ListTimers[Active_Timers->N++] = (int *)Timer;
	}
}
#endif
//************************************************
// Функция безопасного переключения экранов
// Устанавливает флаг для переключения экрана, которое будет выполнено в безопасном месте
// Всегда удаляет текущий активный экран (Active_Screen)
void GUI_SwitchScreen(ScreenCreateFunc create_func)
{
	if(create_func == NULL) return;
	
	ScreenSwitch.need_switch = true;
	ScreenSwitch.create_func = create_func;
}
//************************************************
// Проверка, запланировано ли переключение экрана
bool GUI_IsScreenSwitchPending(void)
{
	return ScreenSwitch.need_switch;
}

//************************************************
// Внутренняя функция для выполнения переключения экрана
static void ExecuteScreenSwitch(void)
{
	if(!ScreenSwitch.need_switch) return;
	
	ScreenCreateFunc create_func = ScreenSwitch.create_func;
	
	// Сбрасываем флаг перед выполнением (на случай рекурсии)
	ScreenSwitch.need_switch = false;
	ScreenSwitch.create_func = NULL;
	
	// Удаляем текущий активный экран
	if(Active_Screen) GUI_DeleteScreen(Active_Screen);
	
	// Создаем новый экран
	if(create_func) create_func();
}
//************************************************
void GUI_Run(void)
{
	unsigned char type;
#ifdef USED_TIMER
	// Обработка таймеров
	if(Active_Timers)
	{
		_GSP_Timer *Cur_Timer = NULL;
		for(int i = 0; i<Active_Timers->N;++i)		// 
		{
			Cur_Timer = (_GSP_Timer *)Active_Timers->ListTimers[i];
			if(labs(Timer_GUI - Cur_Timer->Old_Timer) >= Cur_Timer->period_ms)
			{
				Cur_Timer->Old_Timer = Timer_GUI;
				Cur_Timer->TimerFuncPtr(Cur_Timer);
				if(Cur_Timer->count > 0) 
				{
					if(--Cur_Timer->count == 0)	GUI_DeleteTimer(Cur_Timer);
				}
			}
		}
	}
#endif
	
	// Безопасное переключение экранов (после обработки таймеров, до обработки виджетов)
	ExecuteScreenSwitch();
	
	// Обработка виджетов
	if(GUI_Update(GUI_REFR_PERIOD))
	{
		Keyboard_GSP();		
		if(Active_Screen)
		{
SetColor(Active_Screen->BackColor);
SetClipRgn(0, 0, SCREEN_HOR_SIZE-1, SCREEN_VER_SIZE-1);
SetClip(true);
Bar(Active_Screen->x, Active_Screen->y, Active_Screen->x + Active_Screen->lenx, Active_Screen->y + Active_Screen->leny);
SetClip(false);
			for(int i = 0; i<Active_Screen->N;++i)		// 
			{
				type = *Active_Screen->ListWidgets[i];
				switch(type)
				{
#ifdef USED_LABEL
					case GSP_LABEL:
					{
						DrawingLable((_GSP_Label *)Active_Screen->ListWidgets[i]);
						break;
					}
#endif
#ifdef USED_CHART
					case GSP_CHART:
					{
						DrawingChart((_GSP_Chart *)Active_Screen->ListWidgets[i]);
						break;
					}
#endif
#ifdef USED_RULER
					case GSP_RULER:
					{
						RulerDrawing((_GSP_Ruler *)Active_Screen->ListWidgets[i]);
						break;
					}
#endif					
#ifdef USED_TABLE
					case GSP_TABLE:
					{
						DrawingTable((_GSP_Table *)Active_Screen->ListWidgets[i]);
						break;
					}
#endif					
#ifdef USED_IMAGE
					case GSP_IMAGE:
					{
						DrawingImage((_GSP_Image *)Active_Screen->ListWidgets[i]);
						break;
					}
#endif					
#ifdef USED_LINE
					case GSP_LINE:
					{
						DrawingPolyLine((_GSP_PolyLine *)Active_Screen->ListWidgets[i]);
						break;
					}
#endif					
#ifdef USED_PROGRESSBAR
					case GSP_BAR:
					{
						DrawingBar((_GSP_ProgressBar *)Active_Screen->ListWidgets[i]);
						break;
					}
#endif
#ifdef USED_SPINNER
					case GSP_SPINNER:
					{
						DrawingSpinner((_GSP_Spinner *)Active_Screen->ListWidgets[i]);
						break;
					}
#endif
#ifdef USED_PANEL
					case GSP_PANEL:
					{
						DrawingPanel((_GSP_Panel *)Active_Screen->ListWidgets[i]);
						break;
					}
#endif
#ifdef USED_LIST
					case GSP_LIST:
					{
						DrawingList((_GSP_List *)Active_Screen->ListWidgets[i]);
						break;
					}
#endif
#ifdef USED_MESSAGE
					case GSP_MESSAGE:
					{
						DrawingMessage((_GSP_Message *)Active_Screen->ListWidgets[i]);
						break;
					}
#endif
#ifdef USED_KEYBOARD
					case GSP_KEYBOARD:
					{
						DrawingKeyboard((_GSP_Keyboard *)Active_Screen->ListWidgets[i]);
						break;
					}
#endif
				}
			}
			if(Active_Screen->WidthBorder)
			{
SetLineThickness(Active_Screen->WidthBorder-1);		
SetColor(Active_Screen->BorderColor);
Bevel(Active_Screen->x,Active_Screen->y,Active_Screen->x+Active_Screen->lenx,Active_Screen->y+Active_Screen->leny, 0);
			}
#ifdef USED_MESSAGE
			// Проверка и удаление истекших виджетов Message после отрисовки
			MessageCheckAndDeleteExpired(Active_Screen);
#endif
			Refresh_GSP();
			// Вызов функции обработки кнопок после обработки всего окна
			if(Active_Screen->KeyHandler != NULL)
			{
				Active_Screen->KeyHandler();
			}
		}
	}		
}
//***************************************************************
void GUI_SetBright(unsigned short br)
{
	SetBright(br);
}
//************************************
void GUI_Delay(int ms)
{
	do
	{
		WDT_Clear();							
		GUI_Run();
		if(GUI_Update(GUI_REFR_PERIOD)) ms -= GUI_REFR_PERIOD;
	}while(ms > 0);
}
//*************************************
void GUI_GSP_Init(void)
{
#ifdef USED_TIMER
	Active_Timers = user_malloc(sizeof(_GSP_Timers));
	Active_Timers->ListTimers = NULL;
	Active_Timers->N = 0;
	Active_Timers->Type = GSP_TIMER;
#endif
}
//*************************************************
// Общая функция для обработки бегущей строки (используется в Label и List)
//*****************************************************************
void GSP_RunShift(const char *text, lv_font_t *font, int available_width, int mode_shift,
                  short *old_let_text, short *dx, bool *sh, short *delay, int letter_spacing)
{
    if(!text || !font || available_width <= 0) return;
    
    int max = GetTextWidthUTF8(font, (char *)text, letter_spacing);
    
    // проверяем, изменилась ли длина текста или это первая инициализация
    if(old_let_text)
    {
        int old_value = *old_let_text;
        if(old_value == -1 || old_value != max)
        {
            *old_let_text = max;
            if(dx) *dx = 0;
            if(sh) *sh = false;
            if(delay) *delay = 0;
        }
    }
    
    if(max > available_width)
    {
        switch(mode_shift)
        {
            case GSP_SHIFT_CICL:
            {
                if(dx && *dx == 0)
                {
                    if(sh && !*sh)
                    {
                        if(delay && ++*delay > 30) 
                            *sh = true;
                    }
                }
                if(sh && *sh && dx)
                {
                    // Сдвиг происходит до 1/5 с конца длины виджета (4/5 от начала)
                    int stop_position = available_width * 4 / 5;
                    if(--*dx == -(max - stop_position))
                    {
                        *dx = 0;
                        *sh = false;
                        if(delay) *delay = 0;
                    }
                }
                break;
            }
            case GSP_SHIFT_INFINITY:
            {
                if(dx)
                {
                    // Сдвигаем текст влево на 1 пиксель каждый вызов
                    if(--*dx <= -(max + available_width/2))
                    {
                        *dx = 0; // Возвращаем в начало для бесконечного цикла
                    }
                }
                break;
            }
        }
    }
    else
    {
        // Если текст помещается, сбрасываем состояние сдвига
        if(dx) *dx = 0;
        if(sh) *sh = false;
        if(delay) *delay = 0;
    }
}