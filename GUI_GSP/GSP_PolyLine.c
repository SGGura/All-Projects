#include <p32xxxx.h>
#include <xc.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include "GUI_GSP.h"
#include "GSP_PolyLine.h"
#include "GSP_DrawText.h"
#include "GSP_mem_monitor.h"


#ifdef USED_LINE

//*****************************************************************
_GSP_PolyLine *Crate_PolyLine(_GSP_Screen *Screen, int n, int ID)
{
	_GSP_PolyLine *PolyLine = user_malloc(sizeof(_GSP_PolyLine));
	IncrementListWidgets(Screen, (int *)PolyLine);
	
	PolyLine->Type = GSP_LINE;
	PolyLine->N = n;
	PolyLine->points = (short *)user_malloc(n*sizeof(short)*2);
	if(!PolyLine->points)
	{
		// Не удалось выделить память - освобождаем виджет
		user_free(PolyLine);
		return NULL;
	}
	PolyLine->ID = ID;
	
	PolyLine->Color = DEFAULT_COLOR_TEXT;
	PolyLine->Style = SOLID_LINE;
	PolyLine->Width = 1;
	PolyLine->Time_Blinc = 0;
	PolyLine->BlincTogle = true;
	PolyLine->old_timer = 0;  // ИСПРАВЛЕНО: инициализируем таймер
	PolyLine->Visible = true;
	return(PolyLine);
}
//*****************************************************************
void PolyLineAddPoint(_GSP_PolyLine *PolyLine, int n, int x, int y)
{
	n*=2;
	PolyLine->points[n] = x;
	PolyLine->points[n+1] = y;
}
//*************************************************
void DrawingPolyLine(_GSP_PolyLine *PolyLine)
{
	if(PolyLine->Visible)
	{
		if(PolyLine->points)
		{
			// обработка мерцания
			PolyLineRunBlinc(PolyLine);
			if(PolyLine->BlincTogle)
			{
SetColor(PolyLine->Color);
SetLineThickness(PolyLine->Width);	
				for(int i = 0;i<(PolyLine->N*2 - 2); i+=2)
				{
					int x1 = PolyLine->points[i];
					int y1 = PolyLine->points[i+1];
					int x2 = PolyLine->points[i+2];
					int y2 = PolyLine->points[i+3];
					int a_x = Active_Screen->x;
					int a_len_x = Active_Screen->lenx;
					int a_y = Active_Screen->y;
					int a_len_y = Active_Screen->leny;

					if( x2 > a_x+a_len_x) x2 = a_x+a_len_x;
					if( y2 > a_y+a_len_y) y2 = a_y+a_len_y;
SetClipRgn(x1, y1, x2, y2);
SetLineType(PolyLine->Style);
Line(x1,y1,x2,y2);
				}
			}
		}
SetClip(false);
SetLineType(GSP_SOLID_LINE);
	}
	
}
//*********************************************
void PolyLineRunBlinc(_GSP_PolyLine *PolyLine)
{
	if(PolyLine->Time_Blinc)
	{
		if(labs(Timer_GUI - PolyLine->old_timer) >= PolyLine->Time_Blinc)
		{
			PolyLine->old_timer = Timer_GUI;
			if(PolyLine->BlincTogle) PolyLine->BlincTogle = false;
			else				     PolyLine->BlincTogle = true;
		}
	}
	else PolyLine->BlincTogle = true;
}

#endif
