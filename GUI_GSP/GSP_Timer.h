#ifndef GSP_TIMER_H
#define	GSP_TIMER_H

// Виджет Timer
typedef struct
{
    unsigned char  Type;                       // Тип виджета
    int            ID;                         // ID виджета
    int            period_ms;                     // Периодичность таймера в mc
    int            count;                      // число повторений срабатывания (-1 вечный )  
    //Слюжебные
    int Old_Timer;
    // Вызываемая функция
    void (*TimerFuncPtr) (void *);              //указатель на вызываемую по таймеру фуккцию
}_GSP_Timer;

#ifdef USED_TIMER
// Функции для работы с таймерами, привязанными к экрану
_GSP_Timer *CreateTimer(_GSP_Screen *Screen, void (*TimerFuncPtr)(void *), int period_ms, int count);
void DeleteAllTimers(_GSP_Screen *Screen);
void PauseAllTimers(_GSP_Screen *Screen);
void ResumeAllTimers(_GSP_Screen *Screen);
int GetTimersCount(_GSP_Screen *Screen);

// Дополнительные функции для работы с таймерами
void Set_GSP_Timer_Period(_GSP_Timer *Timer, int period);
#endif

#endif	/* GSP_TIMER_H */

