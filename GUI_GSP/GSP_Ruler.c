#include <p32xxxx.h>
#include <xc.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include "GUI_GSP.h"
#include "GSP_Ruler.h"
#include "GSP_DrawText.h"
#include "GSP_mem_monitor.h"

#ifdef USED_RULER

void RulerCalc(_GSP_Ruler *Ruler, unsigned char marker)
{
	if(!Ruler || !Ruler->Params.Flags) return;
	
	bool flag = true;
	int step = Ruler->Params.Step;
	
	switch(marker)
	{
		case SEP_RULER_H: break;
		case SEP_RULER_M: step = Ruler->Params.Step / 2;   break;
		case SEP_RULER_S: step = Ruler->Params.Step / 10; break;
	}
	
	if(step <= 0) return; // защита от деления на ноль
	
	int x = Ruler->Params.Start % step;
	if(x >= 0)
	{
		x = step - x;
	}
	else
	{
		x = -x;
		flag = false;
	}
	
	int tmp = Ruler->Params.Start / step;
	tmp = (tmp * step) / 100;
	int st = step / 100;
	int s = 0;
	const int max_s = MAX_DISCRET_RULER + 4; // размер массива Name_Div
	
	while(x < Ruler->Params.Len)
	{
		int i = (x * Ruler->lenx) / Ruler->Params.Len;
		if(i >= 0 && i < Ruler->lenx)
		{
			Ruler->Params.Flags[i] |= marker;
			if(marker == SEP_RULER_H)
			{
				if(flag) { tmp += st; }
				if(s < max_s)
				{
					sprintf(&Ruler->Params.Name_Div[s][0], "%d", tmp);
				}
				++s;
				if(!flag) { tmp += st; }
			}
		}
		x = x + step;
	}
}
//****************************************************************
void RulerReCalc(_GSP_Ruler *Ruler)
{
	if(!Ruler || !Ruler->Params.Flags) return;
	
	memset(Ruler->Params.Flags, 0, Ruler->lenx);					// очистить линейку
	int s = 0;
	// определить число интервалов
	int interval = MIN_DISCRET_RULER;
	do
	{
		if(s >= N_STEPS_RULER) break;
		Ruler->Params.Step = Ruler->Params.Steps[s];
		if(Ruler->Params.Step <= 0) break; // защита от деления на ноль
		Ruler->Params.N = Ruler->Params.Len / Ruler->Params.Step;			// число делений линейки
		++s;
	}while((Ruler->Params.N < interval) || (Ruler->Params.N > (MAX_DISCRET_RULER-1)));
	
	// пересчитать основные разделители
	RulerCalc(Ruler, SEP_RULER_H);
	// пересчитать половинные разделители
	//RulerCalc(Ruler, SEP_RULER_M);
}
//*****************************************************************
_GSP_Ruler *RulerCrate(_GSP_Screen *MainScreen, int x, int y, int lenx, int leny, int ID)
{
	_GSP_Ruler *Ruler = user_malloc(sizeof(_GSP_Ruler));
	IncrementListWidgets(MainScreen, (int *)Ruler);

	Ruler->ID = ID;
	Ruler->Type = GSP_RULER;
	Ruler->x = x+MainScreen->x;
	Ruler->y = y+MainScreen->y;
	Ruler->lenx = lenx;
	Ruler->leny = leny;
	Ruler->BackColor = TRANSPARENT;
	Ruler->TextColor = DEFAULT_COLOR_TEXT;
	Ruler->Visible = true;
	Ruler->Params.Flags = user_malloc(lenx);
	if(!Ruler->Params.Flags)
	{
		user_free(Ruler);
		return NULL;
	}
	
	Ruler->Font = DEFAULT_FONT;
	memset(Ruler->Params.Flags, 0, lenx);					// очистить линейку
	Ruler->Params.Len = 5000; 								
	Ruler->Params.Start = 0;
	Ruler->Params.Steps[0] = 100;	
	Ruler->Params.Steps[1] = 200;	
	Ruler->Params.Steps[2] = 500;	
	Ruler->Params.Steps[3] = 1000;	
	Ruler->Params.Steps[4] = 2000;	
	Ruler->Params.Steps[5] = 5000;	
	Ruler->Params.Steps[6] = 10000;	
	Ruler->Params.Steps[7] = 25000;	
	Ruler->Params.Steps[8] = 50000;	
	Ruler->Params.Steps[9] = 60000;	
	Ruler->Params.Steps[10] = 70000;	
	Ruler->Params.Steps[11] = 100000;	
	
	RulerReCalc(Ruler);
	return(Ruler);
}
//***********************************************
void RulerDrawing(_GSP_Ruler *Ruler)
{
	if(Ruler->Visible)
	{
		int lenx = Ruler->lenx;
		int leny = Ruler->leny;
		int x1 = Ruler->x;
		int y1 = Ruler->y;
		int x2 = Ruler->x +lenx;
		int y2 = Ruler->y +leny;
		int a_x = Active_Screen->x;
		int a_len_x = Active_Screen->lenx;
		int a_y = Active_Screen->y;
		int a_len_y = Active_Screen->leny;
		
		if( x2 > a_x+a_len_x) x2 = a_x+a_len_x;
		if( y2 > a_y+a_len_y) y2 = a_y+a_len_y;

SetClipRgn(x1, y1, x2, y2);
SetClip(true);
		if(Ruler->BackColor != TRANSPARENT)
		{
SetColor(Ruler->BackColor);
FillBevel(x1,y1,x2,y2, 0);
		}
		
SetColor(Ruler->TextColor);
		int s = 0;
		const int max_s = MAX_DISCRET_RULER + 4;
		
		for(int i = 0; i < Ruler->lenx; ++i)
		{
			// проверка основных делений (SEP_RULER_H)
			if(Ruler->Params.Flags[i] & SEP_RULER_H)
			{
				if(s < max_s)
				{
					DrawTextUTF8(x1+i+2, y1, Ruler->Font, &Ruler->Params.Name_Div[s][0], Ruler->TextColor, 0, 0);
				}
FastV_Line(x1+i, y1, y1+3, 1);
				if(++s >= max_s) s = max_s - 1;
			}
			// проверка средних делений (SEP_RULER_M)
			else if(Ruler->Params.Flags[i] & SEP_RULER_M)
			{
FastV_Line(x1+i, y1, y1+2, 1);
			}
			// проверка малых делений (SEP_RULER_S)
			else if(Ruler->Params.Flags[i] & SEP_RULER_S)
			{
FastV_Line(x1+i, y1, y1+1, 1);
			}
		}
	}
}
//***********************************************
void RulerDelete(_GSP_Ruler *Ruler)
{
	if(!Ruler) return;
	if(Ruler->Params.Flags)
	{
		user_free(Ruler->Params.Flags);
		Ruler->Params.Flags = NULL;
	}
}
//***********************************************
void RulerSetScale(_GSP_Ruler *Ruler, int start, int len)
{
	Ruler->Params.Len = len;   
	Ruler->Params.Start = start;
	RulerReCalc(Ruler);	
}

#endif