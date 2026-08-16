#include <p32xxxx.h>
#include <xc.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "GUI_GSP.h"
#include "GSP_Panel.h"
#include "GSP_mem_monitor.h"

#ifdef USED_PANEL

_GSP_Panel *CreatePanel(_GSP_Screen *Screen, int x, int y, int lenx, int leny, int ID)
{
	_GSP_Panel *Panel = user_malloc(sizeof(_GSP_Panel));
	if (!Panel) return NULL;

	memset(Panel, 0, sizeof(_GSP_Panel));
	IncrementListWidgets(Screen, (int *)Panel);

	if (lenx < 2) lenx = 2;
	if (leny < 2) leny = 2;
	Panel->Type = GSP_PANEL;
	Panel->ID = ID;
	Panel->lenx = (short)lenx;
	Panel->leny = (short)leny;
	Panel->rel_x = (short)x;
	Panel->rel_y = (short)y;
	Panel->WidgetAlign = GSP_WIDGET_ALIGN_TOP_LEFT;
	Panel->BackColor = DEFAULT_COLOR_BACK;
	Panel->BorderColor = DEFAULT_COLOR_BAR;
	Panel->ShowBorder = true;
	Panel->Radius = 0;
	Panel->Visible = true;

	{
		int abs_x = x;
		int abs_y = y;
		CalculateWidgetPosition(Screen, &abs_x, &abs_y, Panel->lenx, Panel->leny, Panel->WidgetAlign);
		Panel->x = (short)abs_x;
		Panel->y = (short)abs_y;
	}

	return Panel;
}
//*******************************************************************************
void DrawingPanel(_GSP_Panel *Panel)
{
	if (!Panel->Visible) return;

	int x1 = Panel->x;
	int y1 = Panel->y;
	int x2 = Panel->x + Panel->lenx;
	int y2 = Panel->y + Panel->leny;
	int a_x = Active_Screen->x;
	int a_len_x = Active_Screen->lenx;
	int a_y = Active_Screen->y;
	int a_len_y = Active_Screen->leny;
	if (x2 > a_x + a_len_x) x2 = a_x + a_len_x;
	if (y2 > a_y + a_len_y) y2 = a_y + a_len_y;

SetClipRgn(x1, y1, x2, y2);
SetClip(true);

	/* Заливка фона (если не TRANSPARENT) */
	if (Panel->BackColor != TRANSPARENT)
	{
SetColor(Panel->BackColor);
		if (Panel->Radius > 0)
FillBevel((short)x1, (short)y1, (short)x2, (short)y2, (short)Panel->Radius);
		else
Bar((short)x1, (short)y1, (short)x2, (short)y2);
	}

	/* Контур */
	if (Panel->ShowBorder)
	{
SetColor(Panel->BorderColor);
SetLineThickness(GSP_NORMAL_LINE);
		if (Panel->Radius > 0)
Bevel((short)x1, (short)y1, (short)x2, (short)y2, (short)Panel->Radius);
		else
Bevel((short)x1, (short)y1, (short)x2, (short)y2, 0);
SetLineThickness(GSP_NORMAL_LINE);
	}

SetClip(false);
}
//**************************************************************************
void PanelSetPosition(_GSP_Panel *Panel, int x, int y)
{
	if (!Panel) return;
	Panel->rel_x = (short)x;
	Panel->rel_y = (short)y;
	_GSP_Screen *Screen = Get_Active_Screen();
	if (Screen)
	{
		int abs_x = x;
		int abs_y = y;
		CalculateWidgetPosition(Screen, &abs_x, &abs_y, Panel->lenx, Panel->leny, Panel->WidgetAlign);
		Panel->x = (short)abs_x;
		Panel->y = (short)abs_y;
	}
}
//****************************************************************************
void PanelSetSize(_GSP_Panel *Panel, int lenx, int leny)
{
	if (!Panel) return;
	if (lenx < 2) lenx = 2;
	if (leny < 2) leny = 2;
	Panel->lenx = (short)lenx;
	Panel->leny = (short)leny;
	_GSP_Screen *Screen = Get_Active_Screen();
	if (Screen)
	{
		int abs_x = Panel->rel_x;
		int abs_y = Panel->rel_y;
		CalculateWidgetPosition(Screen, &abs_x, &abs_y, Panel->lenx, Panel->leny, Panel->WidgetAlign);
		Panel->x = (short)abs_x;
		Panel->y = (short)abs_y;
	}
}
//***************************************************************************
void PanelSetBackColor(_GSP_Panel *Panel, WORD_COLOR color)
{
	if (Panel)
		Panel->BackColor = color;
}
//***************************************************************************
void PanelSetBorder(_GSP_Panel *Panel, bool show)
{
	if (Panel)
		Panel->ShowBorder = show;
}
//***************************************************************************
void PanelSetBorderColor(_GSP_Panel *Panel, WORD_COLOR color)
{
	if (Panel)
		Panel->BorderColor = color;
}
//***************************************************************************
void PanelSetRadius(_GSP_Panel *Panel, int radius)
{
	if (Panel)
		Panel->Radius = (short)(radius >= 0 ? radius : 0);
}
//***************************************************************************
void PanelSetVisible(_GSP_Panel *Panel, bool visible)
{
	if (Panel)
		Panel->Visible = visible;
}
//***************************************************************************
void PanelSetWidgetAlign(_GSP_Panel *Panel, int align)
{
	if (!Panel) return;
	Panel->WidgetAlign = (short)align;
	_GSP_Screen *Screen = Get_Active_Screen();
	if (Screen)
	{
		int abs_x = Panel->rel_x;
		int abs_y = Panel->rel_y;
		CalculateWidgetPosition(Screen, &abs_x, &abs_y, Panel->lenx, Panel->leny, Panel->WidgetAlign);
		Panel->x = (short)abs_x;
		Panel->y = (short)abs_y;
	}
}

#endif /* USED_PANEL */
