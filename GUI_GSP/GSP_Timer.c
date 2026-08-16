#include <p32xxxx.h>
#include <xc.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include "GUI_GSP.h"
#include "GSP_Timer.h"
#include "Config/GUI_GSP_Config.h"
#include "GSP_mem_monitor.h"

#ifdef USED_TIMER

//************************************************
// Функция для добавления таймера в список экрана
static void IncrementListScreenTimers(_GSP_Screen *Screen, void *Timer)
{
	int **new_list;
	if(Screen->N_Timers == 0)
	{
		new_list = user_malloc(sizeof(int *) * (Screen->N_Timers + 1));
	}
	else
	{
		new_list = user_realloc(Screen->ListTimers, sizeof(int *) * (Screen->N_Timers + 1));
	}
	
	if(new_list)
	{
		Screen->ListTimers = new_list;
		Screen->ListTimers[Screen->N_Timers++] = (int *)Timer;
	}
	// Если new_list == NULL, таймер не добавляется, но это не критично
}
//************************************************
// Создание таймера, привязанного к экрану
_GSP_Timer *CreateTimer(_GSP_Screen *Screen, void (*TimerFuncPtr)(void *), int period_ms, int count)
{
	if(!Screen) return NULL;
	
	// Создаем таймер напрямую
	_GSP_Timer *Timer = user_malloc(sizeof(_GSP_Timer));
	if(!Timer) return NULL;
	
	Timer->Type = GSP_TIMER;
	Timer->TimerFuncPtr = TimerFuncPtr;
	Timer->Old_Timer = Timer_GUI;  /* первый срабатывание — через period_ms от создания */
	Timer->count = count;
	Timer->period_ms = period_ms;
	
	// Добавляем в глобальный список таймеров
	IncrementListTimers((int *)Timer);
	
	// Добавляем в список таймеров экрана
	IncrementListScreenTimers(Screen, Timer);
	
	return Timer;
}
//************************************************
// Удаление всех таймеров экрана
void DeleteAllTimers(_GSP_Screen *Screen)
{
	if(!Screen || !Screen->ListTimers) return;
	
	// Удаляем все таймеры из глобального списка
	for(int i = 0; i < Screen->N_Timers; i++)
	{
		_GSP_Timer *Timer = (_GSP_Timer *)Screen->ListTimers[i];
		if(Timer)
		{
			GUI_DeleteTimer(Timer);
		}
	}
	
	// Освобождаем список таймеров экрана
	if(Screen->ListTimers)
	{
		user_free(Screen->ListTimers);
		Screen->ListTimers = NULL;
	}
	Screen->N_Timers = 0;
}
//************************************************
// Приостановка всех таймеров экрана
void PauseAllTimers(_GSP_Screen *Screen)
{
	if(!Screen || !Screen->ListTimers) return;
	
	for(int i = 0; i < Screen->N_Timers; i++)
	{
		_GSP_Timer *Timer = (_GSP_Timer *)Screen->ListTimers[i];
		if(Timer)
		{
			// Устанавливаем Old_Timer в -1 как флаг паузы
			Timer->Old_Timer = -1;
		}
	}
}
//************************************************
// Возобновление всех таймеров экрана
void ResumeAllTimers(_GSP_Screen *Screen)
{
	if(!Screen || !Screen->ListTimers) return;
	
	for(int i = 0; i < Screen->N_Timers; i++)
	{
		_GSP_Timer *Timer = (_GSP_Timer *)Screen->ListTimers[i];
		if(Timer && Timer->Old_Timer == -1)  // Был на паузе
		{
			Timer->Old_Timer = Timer_GUI;  // Сброс таймера для возобновления
		}
	}
}
//************************************************
// Получение количества таймеров экрана
int GetTimersCount(_GSP_Screen *Screen)
{
	if(!Screen) return 0;
	return Screen->N_Timers;
}
//************************************************
// Установка нового периода для таймера
void Set_GSP_Timer_Period(_GSP_Timer *Timer, int period)
{
	if(Timer)
	{
		Timer->period_ms = period;
	}
}

#endif