#include <p32xxxx.h>
#include <xc.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "GUI_GSP.h"
#include "GSP_Spinner.h"
#include "GSP_mem_monitor.h"

#ifdef USED_SPINNER
//*************************************************************
// Обновление анимации по Timer_GUI (как у Image)
static void SpinnerUpdateAnimation(_GSP_Spinner *Spinner)
{
	if (!Spinner || Spinner->period_ms <= 0) return;
	int elapsed = (int)(Timer_GUI - Spinner->last_timer_update);
	if (elapsed < Spinner->period_ms) return;

	Spinner->last_timer_update = Timer_GUI;
	Spinner->phase = (short)((Spinner->phase + 1) % (Spinner->style == GSP_SPINNER_STYLE_CIRCLE ? SPINNER_CIRCLE_BOXES : SPINNER_NUM_BOXES));
}

_GSP_Spinner *CreateSpinner(_GSP_Screen *Screen, int x, int y, int box_size, int ID)
{
	_GSP_Spinner *Spinner = user_malloc(sizeof(_GSP_Spinner));
	if (!Spinner) return NULL;

	memset(Spinner, 0, sizeof(_GSP_Spinner));
	IncrementListWidgets(Screen, (int *)Spinner);

	if (box_size < 2) box_size = 2;
	if (box_size > 8) box_size = 8;
	Spinner->box_big = (short)box_size;
	Spinner->style = GSP_SPINNER_STYLE_ROW;
	Spinner->gap_circle = 0;
	Spinner->circle_show_empty = true;

	Spinner->Type = GSP_SPINNER;
	Spinner->ID = ID;
	Spinner->lenx = (short)(SPINNER_NUM_BOXES * (Spinner->box_big + SPINNER_GAP) - SPINNER_GAP);
	Spinner->leny = Spinner->box_big;
	Spinner->rel_x = (short)x;
	Spinner->rel_y = (short)y;
	Spinner->WidgetAlign = GSP_WIDGET_ALIGN_TOP_LEFT;
	Spinner->period_ms = 50;
	Spinner->phase = 0;
	Spinner->last_timer_update = Timer_GUI;
	Spinner->Color = DEFAULT_COLOR_BAR;
	Spinner->BackColor = TRANSPARENT;
	Spinner->Visible = true;

	{
		int abs_x = x;
		int abs_y = y;
		CalculateWidgetPosition(Screen, &abs_x, &abs_y, Spinner->lenx, Spinner->leny, Spinner->WidgetAlign);
		Spinner->x = (short)abs_x;
		Spinner->y = (short)abs_y;
	}

	return Spinner;
}
//*******************************************************************************
void SpinnerSetStyleCircle(_GSP_Spinner *Spinner, int gap_pixels)
{
	if (!Spinner) return;
	if (gap_pixels < 0) gap_pixels = 0;
	Spinner->style = GSP_SPINNER_STYLE_CIRCLE;
	Spinner->gap_circle = (short)gap_pixels;
	Spinner->circle_show_empty = true;
	Spinner->lenx = (short)(2 * Spinner->box_big + gap_pixels);
	Spinner->leny = Spinner->lenx;
	Spinner->phase = 0;
	{
		_GSP_Screen *Screen = Get_Active_Screen();
		if (Screen)
		{
			int abs_x = Spinner->rel_x;
			int abs_y = Spinner->rel_y;
			CalculateWidgetPosition(Screen, &abs_x, &abs_y, Spinner->lenx, Spinner->leny, Spinner->WidgetAlign);
			Spinner->x = (short)abs_x;
			Spinner->y = (short)abs_y;
		}
	}
}
//*******************************************************************************
static void DrawRectOutline(short left, short top, short size)
{
	short r = (short)(left + size);
	short b = (short)(top + size);
Line(left, top, r, top);
Line(r, top, r, b);
Line(r, b, left, b);
Line(left, b, left, top);
}
//*******************************************************************************
void DrawingSpinner(_GSP_Spinner *Spinner)
{
	if (!Spinner->Visible) return;

	SpinnerUpdateAnimation(Spinner);

	int x1 = Spinner->x;
	int y1 = Spinner->y;
	int x2 = Spinner->x + Spinner->lenx;
	int y2 = Spinner->y + Spinner->leny;
	int a_x = Active_Screen->x;
	int a_len_x = Active_Screen->lenx;
	int a_y = Active_Screen->y;
	int a_len_y = Active_Screen->leny;
	if (x2 > a_x + a_len_x) x2 = a_x + a_len_x;
	if (y2 > a_y + a_len_y) y2 = a_y + a_len_y;

SetClipRgn(x1, y1, x2, y2);
SetClip(true);

	if (Spinner->BackColor != TRANSPARENT)
	{
SetColor(Spinner->BackColor);
Bar((short)x1, (short)y1, (short)x2, (short)y2);
	}

SetColor(Spinner->Color);

	if (Spinner->style == GSP_SPINNER_STYLE_CIRCLE)
	{
		/* Сетка 2x2: один квадрат заполнен, остальные контур. Обход по кругу: вл→вп→нп→нл→вл */
		static const short circle_order[] = { 0, 1, 3, 2 };  /* ячейки по часовой стрелке */
		short base_x = Spinner->x;
		short base_y = Spinner->y;
		short sz = Spinner->box_big;
		short gap = Spinner->gap_circle;
		short step = (short)(sz + gap);
		short phase = Spinner->phase;
		short filled_cell = circle_order[phase % SPINNER_CIRCLE_BOXES];
		short i;
SetLineThickness(GSP_NORMAL_LINE);
		for (i = 0; i < SPINNER_CIRCLE_BOXES; i++)
		{
			short left = (short)(base_x + (i % 2) * step);
			short top = (short)(base_y + (i / 2) * step);
			if (i == filled_cell)
Bar(left, top, (short)(left + sz), (short)(top + sz));
			else if (Spinner->circle_show_empty)
				DrawRectOutline(left, top, sz);
		}
SetLineThickness(GSP_NORMAL_LINE);
	}
	else
	{
		short left = Spinner->x;
		short top = Spinner->y;
		short box_big = Spinner->box_big;
		short gap = (short)SPINNER_GAP;
		short slot = (short)(box_big + gap);
		short phase = Spinner->phase;
		short i;
		for (i = 0; i < SPINNER_NUM_BOXES; i++)
		{
			/* Градация: 0 = большой, 1 = 1/2, 2 = 1/4, 3 = 1/8. (i - phase) даёт движение слева направо */
			short grade = (short)((i - phase + SPINNER_NUM_BOXES) % SPINNER_NUM_BOXES);
			short size;
			if (grade == 0) size = box_big;
			else if (grade == 1) size = (short)(box_big / 2);
			else if (grade == 2) size = (short)(box_big / 4);
			else size = (short)(box_big / 8);
			if (size < 1) size = 1;
			short cell_left = (short)(left + i * slot);
			short off = (short)((slot - size) / 2);
			short bx = (short)(cell_left + off);
			short by = (short)(top + off);
Bar(bx, by, (short)(bx + size), (short)(by + size));
		}
	}

SetClip(false);
}
//**************************************************************************
void SpinnerSetPosition(_GSP_Spinner *Spinner, int x, int y)
{
	if (!Spinner) return;

	Spinner->rel_x = (short)x;
	Spinner->rel_y = (short)y;
	_GSP_Screen *Screen = Get_Active_Screen();
	if (Screen)
	{
		int abs_x = x;
		int abs_y = y;
		CalculateWidgetPosition(Screen, &abs_x, &abs_y, Spinner->lenx, Spinner->leny, Spinner->WidgetAlign);
		Spinner->x = (short)abs_x;
		Spinner->y = (short)abs_y;
	}
}
//****************************************************************************
void SpinnerSetSize(_GSP_Spinner *Spinner, int box_big)
{
	if (!Spinner) return;

	if (box_big < 2) box_big = 2;
	if (box_big > 8) box_big = 8;
	Spinner->box_big = (short)box_big;
	if (Spinner->style == GSP_SPINNER_STYLE_CIRCLE)
	{
		Spinner->lenx = (short)(2 * Spinner->box_big + Spinner->gap_circle);
		Spinner->leny = Spinner->lenx;
	}
	else
	{
		Spinner->lenx = (short)(SPINNER_NUM_BOXES * (Spinner->box_big + SPINNER_GAP) - SPINNER_GAP);
		Spinner->leny = Spinner->box_big;
	}
	_GSP_Screen *Screen = Get_Active_Screen();
	if (Screen)
	{
		int abs_x = Spinner->rel_x;
		int abs_y = Spinner->rel_y;
		CalculateWidgetPosition(Screen, &abs_x, &abs_y, Spinner->lenx, Spinner->leny, Spinner->WidgetAlign);
		Spinner->x = (short)abs_x;
		Spinner->y = (short)abs_y;
	}
}
//***************************************************************************
void SpinnerSetPeriod(_GSP_Spinner *Spinner, int period_ms)
{
	if (!Spinner || period_ms < 10) return;

	Spinner->period_ms = period_ms;
}
//***************************************************************************
void SpinnerSetVisible(_GSP_Spinner *Spinner, bool visible)
{
	if (Spinner)
		Spinner->Visible = visible;
}
//***************************************************************************
void SpinnerSetColor(_GSP_Spinner *Spinner, WORD_COLOR color)
{
	if (Spinner)
		Spinner->Color = color;
}
//***************************************************************************
void SpinnerSetBackColor(_GSP_Spinner *Spinner, WORD_COLOR color)
{
	if (Spinner)
		Spinner->BackColor = color;  /* TRANSPARENT (0x55) — не заливать своим цветом, рисовать поверх фона экрана */
}
//***************************************************************************
void SpinnerSetCircleShowEmpty(_GSP_Spinner *Spinner, bool show)
{
	if (Spinner)
		Spinner->circle_show_empty = show;  /* false = только заполненный квадрат, пустые не рисуются */
}
//***************************************************************************
void SpinnerSetWidgetAlign(_GSP_Spinner *Spinner, int align)
{
	if (!Spinner) return;

	Spinner->WidgetAlign = (short)align;
	_GSP_Screen *Screen = Get_Active_Screen();
	if (Screen)
	{
		int abs_x = Spinner->rel_x;
		int abs_y = Spinner->rel_y;
		CalculateWidgetPosition(Screen, &abs_x, &abs_y, Spinner->lenx, Spinner->leny, Spinner->WidgetAlign);
		Spinner->x = (short)abs_x;
		Spinner->y = (short)abs_y;
	}
}

#endif /* USED_SPINNER */
