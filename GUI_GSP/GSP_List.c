#include <xc.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "GUI_GSP.h"
#include "GSP_List.h"
#include "GSP_DrawText.h"
#include "GSP_mem_monitor.h"

#ifdef USED_LIST

//*****************************************************************
_GSP_List *Crate_List(_GSP_Screen *Screen, int x, int y, int lenx, int leny, int ID)
{
    _GSP_List *List = user_malloc(sizeof(_GSP_List));
    IncrementListWidgets(Screen, (int *)List);

    List->Type = GSP_LIST; // widget type is GSP_LIST
    List->lenx = lenx;
    List->leny = leny;
    List->ID = ID;
    
    // Инициализируем выравнивание виджета по умолчанию
    List->WidgetAlign = GSP_WIDGET_ALIGN_TOP_LEFT;
    
    // Сохраняем относительные координаты для будущего пересчета
    List->rel_x = x;
    List->rel_y = y;
    
    // Вычисляем абсолютные координаты с учетом выравнивания
    int abs_x = x;
    int abs_y = y;
    CalculateWidgetPosition(Screen, &abs_x, &abs_y, lenx, leny, List->WidgetAlign);
    List->x = abs_x;
    List->y = abs_y;
    List->BackColor = TRANSPARENT;
    List->TextColor = DEFAULT_COLOR_TEXT;
    List->BorderColor = DEFAULT_COLOR_TEXT;
    List->SelColor = DEFAULT_COLOR_BAR;
    List->R = 0;
    List->WidthBorder = 1;
    List->BorderSides = GSP_BORDER_ALL;  // по умолчанию все стороны
    List->Visible = true;
    List->ShowStatus = true;
    List->ShowTriangle = false;  // треугольник выключен по умолчанию
    List->ShowScrollbar = true;  // скроллбар включен по умолчанию
    List->checkmarks = NULL;     // массив флагов галочек (инициализируется при добавлении элементов)
    List->Font = DEFAULT_FONT;
    List->Allign = ALLIGN_TEXT_LEFT;  // выравнивание текста по умолчанию
    List->En_Shift = true;           // бегущая строка выключена по умолчанию
    List->Mode_Shift = GSP_SHIFT_INFINITY; // режим бесконечной прокрутки
    List->dx = 0;
    List->sh = false;
    List->delay = 0;
    List->item_count = 0;
    List->items = NULL;
    List->item_static = NULL;
    List->selected = -1;
    List->top_index = 0;
    List->item_height = GetTextHightUTF8(List->Font);
    List->gap_between_items = 1; // зазор между строками по умолчанию
    List->text_padding = 1; // зазор текста сверху элемента по умолчанию
    List->triangle_gap = 0; // зазор между треугольником и текстом по умолчанию
    List->ShowSelection = true;
    List->CenterItems = false;  // центровка выключена по умолчанию

    return List;
}

//*************************************************
void ListAddItem(_GSP_List *List, const char *text)
{
    if(!List || !text) return;
    
    // Всегда выделяем память и копируем строку
    char *copy = user_malloc(strlen(text)+1);
    if(!copy) return; // Не удалось выделить память для строки
    
    strcpy(copy, text);
    
    // Выделяем память для массива указателей на строки
    char **new_items = user_realloc(List->items, sizeof(char *) * (List->item_count + 1));
    if(!new_items)
    {
        // Не удалось выделить память для массива - освобождаем копию строки
        user_free(copy);
        return;
    }
    List->items = new_items;
    List->items[List->item_count] = copy;
    
    // Выделяем память для массива флагов галочек
    bool *new_checkmarks = user_realloc(List->checkmarks, sizeof(bool) * (List->item_count + 1));
    if(!new_checkmarks)
    {
        user_free(copy);
        List->items = user_realloc(List->items, sizeof(char *) * List->item_count);
        return;
    }
    List->checkmarks = new_checkmarks;
    List->checkmarks[List->item_count] = false;
    
    // Флаг: строка динамическая (копия)
    bool *new_item_static = user_realloc(List->item_static, sizeof(bool) * (List->item_count + 1));
    if(!new_item_static)
    {
        user_free(copy);
        List->items = user_realloc(List->items, sizeof(char *) * List->item_count);
        List->checkmarks = user_realloc(List->checkmarks, sizeof(bool) * List->item_count);
        return;
    }
    List->item_static = new_item_static;
    List->item_static[List->item_count] = false;
    
    List->item_count++;
}

//*************************************************
void ListAddItemStatic(_GSP_List *List, const char *text)
{
    if(!List || !text) return;
    
    char **new_items = user_realloc(List->items, sizeof(char *) * (List->item_count + 1));
    if(!new_items) return;
    List->items = new_items;
    List->items[List->item_count] = (char *)text;
    
    bool *new_checkmarks = user_realloc(List->checkmarks, sizeof(bool) * (List->item_count + 1));
    if(!new_checkmarks)
    {
        List->items = user_realloc(List->items, sizeof(char *) * List->item_count);
        return;
    }
    List->checkmarks = new_checkmarks;
    List->checkmarks[List->item_count] = false;
    
    bool *new_item_static = user_realloc(List->item_static, sizeof(bool) * (List->item_count + 1));
    if(!new_item_static)
    {
        List->items = user_realloc(List->items, sizeof(char *) * List->item_count);
        List->checkmarks = user_realloc(List->checkmarks, sizeof(bool) * List->item_count);
        return;
    }
    List->item_static = new_item_static;
    List->item_static[List->item_count] = true;
    
    List->item_count++;
}

//*************************************************
void ListClear(_GSP_List *List)
{
    if(!List) return;
    if(List->items)
    {
        for(int i = 0; i < List->item_count; i++)
        {
            if(List->items[i] && (!List->item_static || !List->item_static[i]))
                user_free(List->items[i]);
        }
        user_free(List->items);
    }
    List->items = NULL;
    
    if(List->checkmarks) user_free(List->checkmarks);
    List->checkmarks = NULL;
    if(List->item_static) user_free(List->item_static);
    List->item_static = NULL;
    List->dx = 0;
    List->sh = false;
    List->delay = 0;
    List->item_count = 0;
}

//*************************************************
void ListSetSelected(_GSP_List *List, int idx)
{
    if(!List) return;
    if(idx < 0) { List->selected = -1; return; }
    if(idx >= List->item_count) idx = List->item_count - 1;
    List->selected = idx;
    
    // ensure selected is visible - расчет должен совпадать с DrawingList
    int status_h = 0; // status bar removed, reserve full height for items
    int avail_h = List->leny - status_h;
    int th = GetTextHightUTF8(List->Font);
    // Учитываем зазор между строками: n элементов = n * th + (n-1) * gap
    int visible = ((avail_h + List->gap_between_items) / (th + List->gap_between_items) > 0) ? 
                  ((avail_h + List->gap_between_items) / (th + List->gap_between_items)) : 1;
    
    // Сбрасываем параметры сдвига при смене выбранного элемента
    List->dx = 0;
    List->sh = false;
    List->delay = 0;
    
    // прокручиваем список, чтобы выбранный элемент был виден
    if(List->selected < List->top_index) 
    {
        // выбранный элемент выше видимой области - прокручиваем вверх
        List->top_index = List->selected;
    }
    else if(List->selected >= List->top_index + visible) 
    {
        // выбранный элемент ниже видимой области - прокручиваем вниз
        List->top_index = List->selected - visible + 1;
        // убеждаемся, что top_index не отрицательный
        if(List->top_index < 0) List->top_index = 0;
    }
}

//*************************************************
int ListScrollUp(_GSP_List *List)
{
    if(!List) return -1;
    if(List->item_count == 0) return -1;
    
    // Уменьшаем номер выбранного элемента
    int new_selected = List->selected - 1;
    if(new_selected < 0) new_selected = 0;
    
    // Устанавливаем новый выбранный элемент (ListSetSelected автоматически прокрутит список, если нужно)
    ListSetSelected(List, new_selected);
    
    // Возвращаем новый выбранный индекс
    return new_selected;
}

//*************************************************
int ListScrollDown(_GSP_List *List)
{
    if(!List) return -1;
    if(List->item_count == 0) return -1;
    
    // Увеличиваем номер выбранного элемента
    int new_selected = List->selected + 1;
    if(new_selected >= List->item_count) new_selected = List->item_count - 1;
    
    // Устанавливаем новый выбранный элемент (ListSetSelected автоматически прокрутит список, если нужно)
    ListSetSelected(List, new_selected);
    
    // Возвращаем новый выбранный индекс
    return new_selected;
}

//*************************************************
void ListSetShowStatus(_GSP_List *List, bool show)
{
    if(!List) return;
    List->ShowStatus = show;
}

//*************************************************
void ListSetFont(_GSP_List *List, void *font)
{
    if(!List) return;
    List->Font = font;
    // обновляем высоту элемента при изменении шрифта
    int new_th = GetTextHightUTF8(List->Font);
    List->item_height = new_th;
    
    // Проверяем, что gap все еще валиден с новым шрифтом
    // Если gap слишком отрицательный для нового шрифта, корректируем его
    int min_gap = -(new_th - 1);
    if(List->gap_between_items < min_gap)
    {
        List->gap_between_items = min_gap;
    }
}

//*************************************************
void ListSetAllign(_GSP_List *List, int allign)
{
    if(!List) return;
    List->Allign = allign;
}

//*************************************************
void ListSetBorderSides(_GSP_List *List, int sides)
{
    if(!List) return;
    List->BorderSides = sides;
}

//*************************************************
void ListSetShowTriangle(_GSP_List *List, bool show)
{
    if(!List) return;
    List->ShowTriangle = show;
}

//*************************************************
void ListSetShowSelection(_GSP_List *List, bool show)
{
    if(!List) return;
    List->ShowSelection = show;
}

//*************************************************
void ListSetShowScrollbar(_GSP_List *List, bool show)
{
    if(!List) return;
    List->ShowScrollbar = show;
}

//*************************************************
void ListSetItemCheckmark(_GSP_List *List, int idx, bool show)
{
    if(!List) return;
    if(idx < 0 || idx >= List->item_count) return;
    
    // Выделяем память для массива checkmarks, если еще не выделена
    if(!List->checkmarks && List->item_count > 0)
    {
        List->checkmarks = user_malloc(sizeof(bool) * List->item_count);
        // Инициализируем все элементы как false
        for(int i = 0; i < List->item_count; i++)
        {
            List->checkmarks[i] = false;
        }
    }
    
    if(!List->checkmarks) return;
    List->checkmarks[idx] = show;
}

//*************************************************
void ListSetTriangleGap(_GSP_List *List, int gap)
{
    if(!List) return;
    List->triangle_gap = gap;
}

//*************************************************
//*************************************************
void ListSetWidgetAlign(_GSP_List *List, int align)
{
	if(!List) return;
	List->WidgetAlign = align;
	
	// Пересчитываем координаты относительно Active_Screen
	_GSP_Screen *Screen = Get_Active_Screen();
	if(Screen)
	{
		// Используем сохраненные относительные координаты для пересчета
		int abs_x = List->rel_x;
		int abs_y = List->rel_y;
		CalculateWidgetPosition(Screen, &abs_x, &abs_y, List->lenx, List->leny, align);
		List->x = abs_x;
		List->y = abs_y;
	}
}
//*****************************************************
void ListSetPos(_GSP_List *List, int x, int y)
{
	if(!List) return;
	
	// Обновляем относительные координаты
	List->rel_x = x;
	List->rel_y = y;
	
	// Пересчитываем абсолютные координаты с учетом текущего выравнивания
	_GSP_Screen *Screen = Get_Active_Screen();
	if(Screen)
	{
		int abs_x = x;
		int abs_y = y;
		CalculateWidgetPosition(Screen, &abs_x, &abs_y, List->lenx, List->leny, List->WidgetAlign);
		List->x = abs_x;
		List->y = abs_y;
	}
}
//*****************************************************
void ListSetSize(_GSP_List *List, int lenx, int leny)
{
	if(!List) return;
	
	// Обновляем размер виджета
	List->lenx = lenx;
	List->leny = leny;
	
	// Пересчитываем абсолютные координаты с учетом текущего выравнивания
	// (при изменении размера позиция может измениться в зависимости от выравнивания)
	_GSP_Screen *Screen = Get_Active_Screen();
	if(Screen)
	{
		int abs_x = List->rel_x;
		int abs_y = List->rel_y;
		CalculateWidgetPosition(Screen, &abs_x, &abs_y, lenx, leny, List->WidgetAlign);
		List->x = abs_x;
		List->y = abs_y;
	}
}
//*************************************************
void ListSetCenterItems(_GSP_List *List, bool center)
{
    if(!List) return;
    List->CenterItems = center;
}
//*************************************************
void ListSetGapBetweenItems(_GSP_List *List, int gap)
{
    if(!List) return;
    
    // Получаем высоту текущего шрифта для валидации
    int th = GetTextHightUTF8(List->Font);
    
    // Ограничиваем минимальное значение, чтобы избежать деления на ноль
    // gap >= -(th - 1), чтобы гарантировать (th + gap) >= 1
    // Это предотвращает деление на ноль в формуле расчета видимых элементов
    int min_gap = -(th - 1);
    if(gap < min_gap)
    {
        gap = min_gap;
    }
    
    List->gap_between_items = gap;
}

//*************************************************
// Символ галочки ☑ в UTF-8 (U+2611)
static const char checkmark_symbol[] = "✓";

//*************************************************
// Рисует заполненный треугольник, направленный вправо
// Ширина треугольника всегда равна его высоте (size)
// Каждая следующая горизонтальная линия на 1 пиксель длиннее/короче предыдущей
// От середины длина уменьшается по 1 пикселю (симметрично)
static void DrawTriangleRight(int x, int y, int size, WORD_COLOR color)
{
SetColor(color);
    
    // size всегда нечетное, поэтому треугольник симметричен и равносторонний
    // Высота = size, ширина = size
    // Каждая линия: ширина = 1 + min(i, size-1-i)
    // Это дает: 1, 2, 3, ..., size, ..., 3, 2, 1
    int x_left = x;
    
    for(int i = 0; i < size; i++)
    {
        int fill_y = y + i;
        // Вычисляем расстояние до ближайшего края
        int dist_to_edge = (i < size - 1 - i) ? i : (size - 1 - i);
        
        // Ширина линии: от 1 на краях до size в центре
        // Каждая следующая линия на 1 пиксель длиннее/короче
        int line_width = 1 + dist_to_edge;
        
        int fill_x2 = x_left + line_width - 1;
Line(x_left, fill_y, fill_x2, fill_y);
    }
}

//*************************************************
void ListSetShift(_GSP_List *List, bool enable, int mode)
{
    if(!List) return;
    List->En_Shift = enable;
    List->Mode_Shift = mode;
    
    // Сбрасываем параметры сдвига
    List->dx = 0;
    List->sh = false;
    List->delay = 0;
}

//*************************************************
// Вспомогательная функция для расчета ширины текстовой области
// item_idx: индекс строки для проверки галочки (если < 0, используется старая логика для обратной совместимости)
static int CalculateTextAreaWidth(_GSP_List *List, int x1, int x2, int visible, int item_idx)
{
    const int gap_between_text_and_scrollbar = 2;
    const int knob_w = 3;
    const int gap_knob_to_right = 1;
    const int left_padding = 2; // отступ текста от левого края
    const int gap_between_text_and_checkmark = 2; // зазор между текстом и галочкой
    
    bool need_sb = (List->item_count > visible) && List->ShowScrollbar;
    
    // Проверяем, есть ли галочка у конкретной строки или хотя бы одна строка с галочкой в видимой области
    bool has_checkmark = false;
    if(item_idx >= 0 && item_idx < List->item_count)
    {
        // Проверяем галочку для конкретной строки
        if(List->checkmarks && List->checkmarks[item_idx])
        {
            has_checkmark = true;
        }
    }
    else if(item_idx < 0)
    {
        // Старая логика: проверяем, есть ли хотя бы одна строка с галочкой в видимой области
        if(List->checkmarks)
        {
            for(int i = 0; i < visible && (List->top_index + i) < List->item_count; i++)
            {
                if(List->checkmarks[List->top_index + i])
                {
                    has_checkmark = true;
                    break;
                }
            }
        }
    }
    
    // Вычисляем ширину символа галочки, если строка имеет галочку
    int checkmark_width = 0;
    if(has_checkmark)
    {
        // Вычисляем ширину символа ☑ через GetTextWidthUTF8
        checkmark_width = GetTextWidthUTF8(List->Font, (char*)checkmark_symbol, 0);
        checkmark_width += gap_between_text_and_checkmark;  // добавляем зазор
    }
    
    if(need_sb)
    {
        int knob_x2 = x2 - gap_knob_to_right;
        int knob_x1 = knob_x2 - knob_w + 1;
        int text_right = knob_x1 - gap_between_text_and_scrollbar;
        return text_right - (x1 + left_padding) - checkmark_width;
    }
    else
    {
        // Учитываем зазор с обеих сторон (левый и правый) и ширину галочки
        return List->lenx - left_padding * 2 - checkmark_width;
    }
}

//*************************************************
void ListRunShift(_GSP_List *List)
{
    if(!List || !List->En_Shift || !List->items) return;
    if(List->selected < 0 || List->selected >= List->item_count) return;
    
    int idx = List->selected;
    if(!List->items[idx]) return;
    
    int th = GetTextHightUTF8(List->Font);
    int avail_h = List->leny;
    // Учитываем зазор между строками: n элементов = n * th + (n-1) * gap
    int visible = ((avail_h + List->gap_between_items) / (th + List->gap_between_items) > 0) ? 
                  ((avail_h + List->gap_between_items) / (th + List->gap_between_items)) : 1;
    
    int list_x2 = List->x + List->lenx;
    int text_area_w = CalculateTextAreaWidth(List, List->x, list_x2, visible, -1);
    
    if(text_area_w <= 0) return;
    
    // Вычисляем доступную ширину для текста с учетом треугольника (как в DrawingList)
    int available_text_width = text_area_w;
    if(List->ShowTriangle)
    {
        int triangle_size = th - 3;
        if(triangle_size % 2 == 0 && triangle_size > 0) triangle_size--;
        if(triangle_size < 1) triangle_size = 1;
        available_text_width = text_area_w - (triangle_size + List->triangle_gap);
    }
    
    if(available_text_width <= 0) return;
    
        GSP_RunShift(List->items[idx], List->Font, available_text_width, List->Mode_Shift,
                 NULL, &List->dx, &List->sh, &List->delay, 0);
}

//*************************************************
void ListDelItem(_GSP_List *List, int idx)
{
    if(!List) return;
    if(List->item_count <= 0) return;
    if(idx < 0 || idx >= List->item_count) return;

    if(List->items && List->items[idx] && (!List->item_static || !List->item_static[idx]))
    {
        user_free(List->items[idx]);
    }
    List->items[idx] = NULL;

    for(int i = idx; i < List->item_count - 1; ++i)
    {
        List->items[i] = List->items[i+1];
        if(List->checkmarks) List->checkmarks[i] = List->checkmarks[i+1];
        if(List->item_static) List->item_static[i] = List->item_static[i+1];
    }
    List->item_count--;

    if(List->item_count == 0)
    {
        user_free(List->items);
        List->items = NULL;
        if(List->checkmarks) { user_free(List->checkmarks); List->checkmarks = NULL; }
        List->selected = -1;
        List->top_index = 0;
        // Сбрасываем параметры сдвига
        List->dx = 0;
        List->sh = false;
        List->delay = 0;
        return;
    }
    else
    {
        // Изменяем размер массива указателей на строки
        char **new_items = user_realloc(List->items, sizeof(char *) * List->item_count);
        if(new_items)
        {
            List->items = new_items;
        }
        // Изменяем размер массива флагов галочек
        if(List->checkmarks)
        {
            bool *new_checkmarks = user_realloc(List->checkmarks, sizeof(bool) * List->item_count);
            if(new_checkmarks) List->checkmarks = new_checkmarks;
        }
        if(List->item_static)
        {
            bool *new_item_static = user_realloc(List->item_static, sizeof(bool) * List->item_count);
            if(new_item_static) List->item_static = new_item_static;
        }
    }
    
    // Сбрасываем параметры сдвига при удалении элемента
    List->dx = 0;
    List->sh = false;
    List->delay = 0;

    // adjust selected and top_index
    if(List->selected >= List->item_count) List->selected = List->item_count - 1;
    if(List->selected < 0) List->selected = -1;

    int th = GetTextHightUTF8(List->Font);
    int status_h = List->ShowStatus ? List->item_height : 0;
    int avail_h = List->leny - status_h;
    // Учитываем зазор между строками: n элементов = n * th + (n-1) * gap
    int visible = ((avail_h + List->gap_between_items) / (th + List->gap_between_items) > 0) ? 
                  ((avail_h + List->gap_between_items) / (th + List->gap_between_items)) : 1;

    if(List->top_index + visible > List->item_count)
    {
        int new_top = List->item_count - visible;
        if(new_top < 0) new_top = 0;
        List->top_index = new_top;
    }
    if(List->top_index < 0) List->top_index = 0;
}

//*************************************************
int ListGetSelected(_GSP_List *List)
{
    if(!List) return -1;
    return List->selected;
}

//*************************************************
void DrawingList(_GSP_List *List)
{
    if(!List) return;
    if(!List->Visible) return;

    int lenx = List->lenx;
    int leny = List->leny;
    int x1 = List->x;
    int y1 = List->y;
    int x2 = List->x + lenx;
    int y2 = List->y + leny;
    int a_x = Active_Screen->x;
    int a_len_x = Active_Screen->lenx;
    int a_y = Active_Screen->y;
    int a_len_y = Active_Screen->leny;

    if( x2 > a_x+a_len_x) x2 = a_x+a_len_x;
    if( y2 > a_y+a_len_y) y2 = a_y+a_len_y;

SetClipRgn(x1, y1, x2, y2);
SetClip(true);
    if(List->BackColor != TRANSPARENT)
    {
SetColor(List->BackColor);
FillBevel(x1,y1,x2,y2, List->R);
    }

    // обработка сдвига для бегущей строки
    if(List->En_Shift) ListRunShift(List);

    int th = GetTextHightUTF8(List->Font);
    // Используем доступную высоту с учётом верхнего внутреннего отступа в 2 пикселя
    int avail_h = leny - 1;
    if(avail_h < 1) avail_h = 1;
    // Учитываем зазор между строками: n элементов = n * th + (n-1) * gap
    // Решаем: avail_h >= n * th + (n-1) * gap
    // avail_h >= n * (th + gap) - gap
    // n <= (avail_h + gap) / (th + gap)
    int visible = ((avail_h + List->gap_between_items) / (th + List->gap_between_items) > 0) ? 
                  ((avail_h + List->gap_between_items) / (th + List->gap_between_items)) : 1;
    
    // Вычисляем, есть ли частично видимая последняя строка
    int used_height = visible * (th + List->gap_between_items) - List->gap_between_items;
    int remaining_height = avail_h - used_height;
    bool has_partial_item = false;
    int partial_visible_height = 0;
    
    // Проверяем, есть ли частично видимая строка
    if(remaining_height > 0 && List->item_count > 0 && (List->top_index + visible) < List->item_count) {
        // Есть частично видимая строка
        has_partial_item = true;
        partial_visible_height = remaining_height;
        // Минимальная видимая высота - хотя бы 1 пиксель текста
        if(partial_visible_height >= List->text_padding + 1) {
            // Можно отобразить частично видимую строку
        } else {
            // Слишком мало места - не отображаем
            has_partial_item = false;
        }
    }
    
    // Количество строк для отрисовки (включая частично видимую)
    int items_to_draw = visible;
    if(has_partial_item) {
        items_to_draw = visible + 1;
    }
    
    bool need_sb = (List->item_count > visible) && List->ShowScrollbar;

    // вычисляем размер треугольника один раз (одинаков для всех элементов)
    int triangle_size = th - 3;
    if(triangle_size % 2 == 0 && triangle_size > 0) triangle_size--;
    if(triangle_size < 1) triangle_size = 1;
    const int left_padding = 2; // отступ текста от левого края
    int base_text_x = x1 + left_padding;

    // Вычисляем начальную позицию Y с учетом центровки
    // Базовая позиция первой строки: y = y1 + 2
    int y = y1 + 2;
    if(List->CenterItems && List->item_count > 0 && List->item_count < visible)
    {
        // Вычисляем общую высоту всех строк
        int total_height = List->item_count * (th + List->gap_between_items) - List->gap_between_items;
        // Вычисляем свободное пространство
        int free_space = avail_h - total_height;
        // Добавляем отступ сверху для центровки
        int y_offset = free_space / 2;
        y = y1 + 2 + y_offset;
    }

    int idx = List->top_index;
    for(int i=0; i<items_to_draw; i++)
    {
        if(idx >= List->item_count) break;
        
        // Определяем, является ли это частично видимой строкой
        bool is_partial = (has_partial_item && i == visible);
        
        // рассчитываем ширину текстовой области для конкретной строки
        int text_area_w = CalculateTextAreaWidth(List, x1, x2, visible, idx);
        
        // вычисляем смещения для текста с учетом треугольника
        int text_start_x = base_text_x;
        int text_w = text_area_w;
        if(List->ShowTriangle)
        {
            text_start_x = base_text_x + triangle_size + List->triangle_gap;
            text_w = text_area_w - (triangle_size + List->triangle_gap);
        }
        
        int item_y1 = y;
        int item_y2;
        int text_x = text_start_x; // позиция текста (может изменяться при выравнивании и сдвиге)
        
        // Для частично видимой строки используем обрезанную высоту
        if(is_partial) {
            item_y2 = y1 + avail_h; // Обрезаем по доступной высоте
        } else {
            item_y2 = y + th; // Полная высота текста
        }
        
        // определяем цвета и параметры в зависимости от выбора
        bool is_selected = (idx == List->selected);
        bool is_visually_selected = is_selected && List->ShowSelection;
        WORD_COLOR text_color = is_visually_selected ? ((List->SelColor == BLACK) ? WHITE : BLACK) : List->TextColor;
        WORD_COLOR triangle_color = is_visually_selected ? text_color : List->TextColor;
        
        // применяем сдвиг для бегущей строки только для выбранного элемента
        if(is_selected && List->En_Shift)
        {
            text_x = text_start_x + List->dx;
        }
        
        // вычисляем позицию текста с учетом отступа сверху
        int text_y = item_y1 + List->text_padding; // добавляем отступ сверху
        
        // закрашиваем фон для выбранного элемента (с таким же смещением как текст)
        // Делаем фон на 2 пикселя выше высоты шрифта: по 1 пикселю сверху и снизу текста
        if(is_visually_selected)
        {
SetColor(List->SelColor);
            int fill_y1 = text_y - 1; // фон начинается на 1 пиксель выше текста
            int fill_y2;

            // Не выходим выше границы виджета
            if(fill_y1 < y1) fill_y1 = y1;
            
            // Для частично видимой строки обрезаем фон по видимой области
            if(is_partial) {
                fill_y2 = y1 + avail_h - 1; // Обрезаем по доступной высоте
            } else {
                fill_y2 = text_y + th; // высота фона = высота текста + 2 пикселя
            }
            
            // Закрашиваем на полную ширину виджета, без зазоров слева и справа
            // Зазор справа только если есть скроллбар
            int fill_x1 = x1; // без зазора слева
            int fill_x2;
            if(need_sb)
            {
                // Если есть скроллбар, заканчиваем перед ним
                const int gap_between_text_and_scrollbar = 2;
                const int knob_w = 3;
                const int gap_knob_to_right = 1;
                int knob_x2 = x2 - gap_knob_to_right;
                int knob_x1 = knob_x2 - knob_w + 1;
                fill_x2 = knob_x1 - gap_between_text_and_scrollbar - 1;
            }
            else
            {
                fill_x2 = x2 - 1; // до правого края виджета
            }
FillBevel(fill_x1, fill_y1, fill_x2, fill_y2, 0);
        }
        
        // рисуем треугольник (для частично видимой строки рисуем, если он помещается в видимую область)
        if(List->ShowTriangle)
        {
            int triangle_y = text_y + (th - triangle_size) / 2; // выравниваем относительно текста
            int triangle_bottom = triangle_y + triangle_size;
            
            // Для частично видимой строки проверяем, помещается ли треугольник в видимую область
            if(is_partial) {
                int visible_bottom = y1 + avail_h;
                // Рисуем треугольник только если он хотя бы частично виден
                if(triangle_y < visible_bottom && triangle_bottom > item_y1) {
                    // Ограничиваем область отрисовки треугольника видимой областью
SetClipRgn(base_text_x, item_y1, base_text_x + triangle_size, visible_bottom);
SetClip(true);
                    DrawTriangleRight(base_text_x, triangle_y, triangle_size, triangle_color);
SetClip(false);
                }
            } else {
                // Для полностью видимой строки рисуем треугольник без ограничений
                DrawTriangleRight(base_text_x, triangle_y, triangle_size, triangle_color);
            }
        }
        
        // ограничиваем область отрисовки текста
        int clip_left = List->ShowTriangle ? text_start_x : base_text_x;
        int text_right_bound = clip_left + text_w;
SetClipRgn(clip_left, item_y1, text_right_bound, item_y2);
SetClip(true);
        DrawAllignTextUTF8(text_x, text_y, text_w, List->Allign, List->items[idx], List->Font, text_color, 0, 0);
        
        // рисуем дубликат текста для бегущей строки только для выбранного элемента
        // Для частично видимой строки не рисуем дубликат, так как он может быть обрезан
        if(is_selected && List->En_Shift && List->items[idx] && !is_partial)
        {
            int text_width = GetTextWidthUTF8(List->Font, List->items[idx], 0);
            if(text_width > text_w)
            {
                DrawAllignTextUTF8(text_x + text_width + text_w/2, text_y, text_w, List->Allign, List->items[idx], List->Font, text_color, 0, 0);
            }
        }
SetClip(false);
        
        // рисуем символ галочки ☑ справа в конце строки, если включено для данной строки
        // Для частично видимой строки показываем галочку только если она видна
        if(List->checkmarks && List->checkmarks[idx] && !is_partial)
        {
            const int gap_between_text_and_checkmark = 2;
            
            // Вычисляем ширину символа галочки
            int checkmark_width = GetTextWidthUTF8(List->Font, (char*)checkmark_symbol, 0);
            
            // Вычисляем позицию галочки справа от текста
            int checkmark_x;
            if(need_sb)
            {
                // Если есть скроллбар, позиционируем галочку перед ним
                const int gap_between_text_and_scrollbar = 2;
                const int knob_w = 3;
                const int gap_knob_to_right = 1;
                int knob_x2 = x2 - gap_knob_to_right;
                int knob_x1 = knob_x2 - knob_w + 1;
                checkmark_x = knob_x1 - gap_between_text_and_scrollbar - checkmark_width;
            }
            else
            {
                // Если нет скроллбара, позиционируем галочку у правого края с отступом
                const int right_padding = 2;
                checkmark_x = x2 - right_padding - checkmark_width;
            }
            
            // Выравниваем галочку по вертикали относительно текста
            int checkmark_y = text_y;
            
            // Используем цвет текста для галочки
            WORD_COLOR checkmark_color = text_color;
            DrawTextUTF8(checkmark_x, checkmark_y, List->Font, (char*)checkmark_symbol, checkmark_color, 0, 0);
        }

        // Для частично видимой строки не добавляем зазор, так как это последняя строка
        if(!is_partial) {
            y += th + List->gap_between_items; // Добавляем зазор между строками
        }
        idx++;
    }

    // status bar removed — no drawing here

    // draw scrollbar if needed
    if(need_sb)
    {
        int sb_y1 = y1 + 1;
        int sb_y2 = y1 + avail_h - 1;

        // compute knob size and position
        int sb_h = sb_y2 - sb_y1 + 1;
        int knob_h = (List->item_count > 0) ? (visible * sb_h) / List->item_count : sb_h;
        if(knob_h < 6) knob_h = 6;
        if(knob_h > sb_h) knob_h = sb_h;

        int max_scroll_pos = List->item_count - visible;
        int knob_pos;
        if(max_scroll_pos <= 0) knob_pos = sb_y1;
        else knob_pos = sb_y1 + (List->top_index * (sb_h - knob_h)) / max_scroll_pos;

        // draw knob (rectangle) - ширина 3 пикселя, прижат к правому краю с зазором в 1 пиксель
        const int knob_w = 3;
        const int gap_knob_to_right = 1;
        int knob_x2 = x2 - gap_knob_to_right;
        int knob_x1 = knob_x2 - knob_w + 1;
        int sb_center_x = (knob_x1 + knob_x2) / 2;  // центр ползунка
        
        // draw vertical dotted line for scrollbar (центрирована относительно ползунка)
SetColor(List->BorderColor);
SetLineType(GSP_DOTTED_LINE);
SetLineThickness(GSP_NORMAL_LINE);
Line(sb_center_x, sb_y1, sb_center_x, sb_y2);
SetLineType(GSP_SOLID_LINE); // restore solid line type
        
        // draw knob
SetColor(List->SelColor);
FillBevel(knob_x1, knob_pos, knob_x2, knob_pos + knob_h, 2);
    }

    if(List->WidthBorder)
    {
SetLineThickness(List->WidthBorder-1);
SetColor(List->BorderColor);
        
        // рисуем рамку только с заданных сторон
        if(List->R == 0)
        {
            // прямоугольная рамка - рисуем линии для каждой стороны
            if(List->BorderSides & GSP_BORDER_TOP)
            {
Line(x1, y1, x2, y1);  // верхняя сторона
            }
            if(List->BorderSides & GSP_BORDER_BOTTOM)
            {
Line(x1, y2, x2, y2);  // нижняя сторона
            }
            if(List->BorderSides & GSP_BORDER_LEFT)
            {
Line(x1, y1, x1, y2);  // левая сторона
            }
            if(List->BorderSides & GSP_BORDER_RIGHT)
            {
Line(x2, y1, x2, y2);  // правая сторона
            }
        }
        else
        {
            // для закругленных углов используем Bevel, но это ограничение
            // можно улучшить, если нужно рисовать только определенные стороны с закруглением
Bevel(x1,y1,x2,y2, List->R);
        }
    }
SetClip(false);
}

#endif
