#include <p32xxxx.h>
#include <xc.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include "GUI_GSP.h"
#include "GSP_Label.h"
#include "GSP_DrawText.h"
#include "GSP_mem_monitor.h"


#ifdef USED_LABEL

#define PERIOD_BLINC_CURSOR 300
//*************************************************
// Вспомогательная функция: обновление ширины Label в режиме AutoWidth
static void LabelUpdateAutoWidth(_GSP_Label *Label)
{
	if(!Label) return;
	if(!Label->AutoWidth) return;
	if(!Label->Text || !Label->Font) return;

	// Вычисляем ширину текста с учетом межбуквенного интервала
	int text_width = GetTextWidthUTF8(Label->Font, (char *)Label->Text, Label->LetterSpacing);
	if(text_width <= 0) return;

	Label->lenx = (short)text_width;

	// Пересчитываем абсолютные координаты с учетом нового размера и текущего выравнивания
	_GSP_Screen *Screen = Get_Active_Screen();
	if(Screen)
	{
		int abs_x = Label->rel_x;
		int abs_y = Label->rel_y;
		CalculateWidgetPosition(Screen, &abs_x, &abs_y, Label->lenx, Label->leny, Label->WidgetAlign);
		Label->x = abs_x;
		Label->y = abs_y;
	}
}

//*****************************************************************
_GSP_Label *Crate_Label(_GSP_Screen *Screen, int x, int y, int lenx, int leny, int ID)
{
	_GSP_Label *Label = user_malloc(sizeof(_GSP_Label));
	IncrementListWidgets(Screen, (int *)Label);
	
	Label->Type = GSP_LABEL;
	Label->lenx = lenx;
	Label->leny = leny;
	Label->ID = ID;
	
	// Инициализируем выравнивание виджета по умолчанию
	Label->WidgetAlign = GSP_WIDGET_ALIGN_TOP_LEFT;
	
	// Сохраняем относительные координаты для будущего пересчета
	Label->rel_x = x;
	Label->rel_y = y;
	
	// Вычисляем абсолютные координаты с учетом выравнивания
	int abs_x = x;
	int abs_y = y;
	CalculateWidgetPosition(Screen, &abs_x, &abs_y, lenx, leny, Label->WidgetAlign);
	Label->x = abs_x;
	Label->y = abs_y;
	
	Label->BackColor = TRANSPARENT;
	Label->TextColor = DEFAULT_COLOR_TEXT;
	Label->BorderColor = DEFAULT_COLOR_TEXT;//DEFAULT_COLOR_BACK;
	Label->En_Shift = false;
	Label->Static = false;
	Label->Text = user_malloc(strlen(DEFAULT_TEXT) + 1);	// первичное выделение памяти пов текст (+1 для нулевого терминатора)
	strcpy((char *)Label->Text, DEFAULT_TEXT);	
	Label->Font = DEFAULT_FONT;
	Label->R = 0;
	Label->Allign = ALLIGN_TEXT_CENTER;
	Label->WidthBorder = 0;
	Label->Time_Blinc = 0;
	Label->BlincTogle = true;
	Label->old_timer = 0;  // ИСПРАВЛЕНО: инициализируем таймер
	Label->Mode_Shift = GSP_SHIFT_INFINITY;
	Label->dx = 0;
	Label->dy = 0;  // По умолчанию вертикальное смещение = 0
	Label->sh = false;
	Label->delay = 0;
	Label->old_let_text = -1;
	Label->LetterSpacing = 0;  // По умолчанию межбуквенный интервал = 0
	Label->AutoWidth = false;  // По умолчанию ширина фиксированная
	Label->en_edit = false;    // По умолчанию редактирование выключено
	Label->max_text_length = 0; // По умолчанию без ограничений длины
	Label->cursor_timer = 0;   // Инициализация таймера курсора
	Label->cursor_visible = true; // Инициализация видимости курсора
	Label->TextInvert = false;    // По умолчанию текст без инвертирования
	Label->Visible = true;
	return(Label);
}
//***********************************************
void LabelSetText(_GSP_Label * Label, const char * text)
{
	if(Label)
	{
		if(!Label->Static)
		{
			// Проверяем, что text не NULL
			if(!text) text = "";
			
			int text_len = strlen(text);
			int alloc_size;
			
			// Если включен режим редактирования и задана максимальная длина
			if(Label->en_edit && Label->max_text_length > 0)
			{
				// Ограничиваем длину текста максимальным значением
				if(text_len > Label->max_text_length)
				{
					text_len = Label->max_text_length;
				}
				// Выделяем память с учетом max_text_length для будущих операций редактирования
				alloc_size = Label->max_text_length + 1;
			}
			else
			{
				// Старое поведение - выделяем ровно столько, сколько нужно
				// Минимум 1 байт для нулевого терминатора (даже для пустой строки)
				alloc_size = (text_len > 0) ? (text_len + 1) : 1;
			}
			
			char *old_text = (char *)Label->Text;
			char *new_text = user_realloc((char *)Label->Text, alloc_size);
			if(new_text)
			{
				Label->Text = new_text;
				if(Label->en_edit && Label->max_text_length > 0 && text_len < Label->max_text_length)
				{
					// Копируем с ограничением длины
					if(text_len > 0)
					{
						strncpy((char *)Label->Text, text, text_len);
						((char *)Label->Text)[text_len] = '\0';  // гарантируем нулевой терминатор
					}
					else
					{
						// Пустая строка
						((char *)Label->Text)[0] = '\0';
					}
				}
				else
				{
					// Обычное копирование
					if(text_len > 0)
					{
						strcpy((char *)Label->Text, text);
					}
					else
					{
						// Пустая строка
						((char *)Label->Text)[0] = '\0';
					}
				}
			}
			else
			{
				// user_realloc вернул NULL - восстанавливаем старый указатель
				// (realloc не освобождает старый блок при ошибке)
				Label->Text = old_text;
			}
		//	printf("%s %s\r\n", text, (char *)Label->Text);
		}
		else Label->Text = text;

		// При изменении текста можно обновить ширину в режиме AutoWidth
		LabelUpdateAutoWidth(Label);
	}
}
//***********************************************
void LabelSetTextFmt(_GSP_Label *Label, const char *format, ...)
{
	if(!Label) return;
	
	char buffer[256];  // Буфер для форматированной строки
	va_list args;
	va_start(args, format);
	vsnprintf(buffer, sizeof(buffer), format, args);
	va_end(args);
	
	LabelSetText(Label, buffer);
}
//***********************************************
void LabelSetFont(_GSP_Label *Label, void *font)
{
	if(Label)
	{
		Label->Font = font;
		// При изменении шрифта обновляем ширину в режиме AutoWidth
		LabelUpdateAutoWidth(Label);
	}
}
//*************************************************
void LabelSetTextAllign(_GSP_Label *Label, int allign)
{
	if(Label) Label->Allign = allign;
}
//*************************************************
void LabelSetWidgetAlign(_GSP_Label *Label, int align)
{
	if(!Label) return;
	Label->WidgetAlign = align;
	
	// Пересчитываем координаты относительно Active_Screen
	_GSP_Screen *Screen = Get_Active_Screen();
	if(Screen)
	{
		// Используем сохраненные относительные координаты для пересчета
		int abs_x = Label->rel_x;
		int abs_y = Label->rel_y;
		CalculateWidgetPosition(Screen, &abs_x, &abs_y, Label->lenx, Label->leny, align);
		Label->x = abs_x;
		Label->y = abs_y;
	}
}
//*****************************************************
void LabelSetPos(_GSP_Label *Label, int x, int y)
{
	if(!Label) return;
	
	// Обновляем относительные координаты
	Label->rel_x = x;
	Label->rel_y = y;
	
	// Пересчитываем абсолютные координаты с учетом текущего выравнивания
	_GSP_Screen *Screen = Get_Active_Screen();
	if(Screen)
	{
		int abs_x = x;
		int abs_y = y;
		CalculateWidgetPosition(Screen, &abs_x, &abs_y, Label->lenx, Label->leny, Label->WidgetAlign);
		Label->x = abs_x;
		Label->y = abs_y;
	}
}
//*****************************************************
void LabelSetSize(_GSP_Label *Label, int lenx, int leny)
{
	if(!Label) return;
	
	// Обновляем размер виджета
	Label->lenx = lenx;
	Label->leny = leny;
	
	// Пересчитываем абсолютные координаты с учетом текущего выравнивания
	// (при изменении размера позиция может измениться в зависимости от выравнивания)
	_GSP_Screen *Screen = Get_Active_Screen();
	if(Screen)
	{
		int abs_x = Label->rel_x;
		int abs_y = Label->rel_y;
		CalculateWidgetPosition(Screen, &abs_x, &abs_y, lenx, leny, Label->WidgetAlign);
		Label->x = abs_x;
		Label->y = abs_y;
	}
}
//*************************************************
void DrawingLable(_GSP_Label *Label)
{
	if(Label->Visible)
	{
		int lenx = Label->lenx;
		int leny = Label->leny;
		int x1 = Label->x;
		int y1 = Label->y;
		int x2 = Label->x +lenx;
		int y2 = Label->y +leny;
		int a_x = Active_Screen->x;
		int a_len_x = Active_Screen->lenx;
		int a_y = Active_Screen->y;
		int a_len_y = Active_Screen->leny;
		
		if( x2 > a_x+a_len_x) x2 = a_x+a_len_x;
		if( y2 > a_y+a_len_y) y2 = a_y+a_len_y;

SetClipRgn(x1, y1, x2, y2);
SetClip(true);
		if(Label->BackColor != TRANSPARENT)
		{
SetColor(Label->BackColor);
FillBevel(x1,y1,x2,y2, Label->R);
		}
		
		// Проверяем, что Text не NULL
		if(!Label->Text)
		{
			// Text не инициализирован - ничего не рисуем
SetClip(false);
			return;
		}
		
		// Проверяем, что Text не пустой (если не режим редактирования)
		bool is_empty = (Label->Text[0] == '\0');
		if(is_empty && !Label->en_edit)
		{
			// Пустая строка и не режим редактирования - ничего не рисуем
SetClip(false);
			return;
		}
		
		// аосчитать число строк
		int n_str = 1;
		int len = is_empty ? 0 : strlen((char *)Label->Text);
		for(int i = 0;i<len;++i)
		{
			if(Label->Text[i] == '\n') n_str++;
		}
		
		// обработка сдвига если одна строка
		if(n_str < 2) { LabelRunShift(Label);}
		
		// обработка мерцания
		LabelRunBlinc(Label);
		
		// обработка курсора в режиме редактирования
		if(Label->en_edit)
		{
			LabelRunCursor(Label);
		}
		
		if(Label->BlincTogle)
		{
			short h_str = GetTextHightUTF8(Label->Font);
			short h =  h_str*n_str;
			short y = y1 + (leny - h)/2 + Label->dy;			// центровка по вертиками + вертикальное смещение
			lenx = x2-x1;
			int x = x1+1+Label->dx;
			
			// Рисуем текст, если он не пустой
			if(!is_empty)
			{
				char *string = (char *) strtok((char *)Label->Text, (const char *) "\n");
				
				// Проверяем, что strtok вернул валидную строку
				if(string)
				{
					int len_str = GetTextWidthUTF8(Label->Font, string, Label->LetterSpacing);

					do
					{
						if(string && string[0] != '\0')
						{
							DrawAllignTextUTF8(x, y,  lenx, Label->Allign, string , Label->Font, Label->TextColor, Label->LetterSpacing, Label->TextInvert ? 1 : 0);
							int len = GetTextWidthUTF8(Label->Font, string, Label->LetterSpacing);
							if((Label->En_Shift) && (len > Label->lenx))	
							{
								DrawAllignTextUTF8(x+len_str+Label->lenx/2, y,  lenx, Label->Allign, string , Label->Font, Label->TextColor, Label->LetterSpacing, Label->TextInvert ? 1 : 0);
							}
							if(n_str > 1 && string) *(string+strlen(string)) = '\n';			// вернуть символ удаленный  strtok
						}
						string = (char *)strtok(NULL, (const char *) "\n");
						y+=h_str;
					}while(--n_str);
				}
			}
			
			// Рисуем курсор в режиме редактирования
			if(Label->en_edit && Label->cursor_visible)
			{
				short h_str = GetTextHightUTF8(Label->Font);
				short y_cursor = y1 + (leny - h_str) / 2 + Label->dy;  // Вертикальная центровка курсора + смещение
				int x_cursor;
				
				// Вычисляем позицию курсора (в конце текста)
				if(is_empty || !Label->Text || Label->Text[0] == '\0')
				{
					// Пустая строка - курсор в начале
					switch(Label->Allign)
					{
						case ALLIGN_TEXT_LEFT:
							x_cursor = x1 + 1;
							break;
						case ALLIGN_TEXT_CENTER:
							x_cursor = x1 + lenx / 2;
							break;
						case ALLIGN_TEXT_RIGHT:
							x_cursor = x2 - 1;
							break;
						default:
							x_cursor = x1 + 1;
							break;
					}
				}
				else
				{
					// Вычисляем ширину текста
					int text_width = GetTextWidthUTF8(Label->Font, (char *)Label->Text, Label->LetterSpacing);
					
					// Вычисляем позицию X в зависимости от выравнивания
					switch(Label->Allign)
					{
						case ALLIGN_TEXT_LEFT:
							x_cursor = x1 + 1 + text_width;
							break;
						case ALLIGN_TEXT_CENTER:
							x_cursor = x1 + (lenx - text_width) / 2 + text_width;
							break;
						case ALLIGN_TEXT_RIGHT:
							x_cursor = x2 - 1 - text_width + text_width;  // Это упрощается до x2 - 1, но оставим для ясности
							x_cursor = x1 + lenx - text_width;
							break;
						default:
							x_cursor = x1 + 1 + text_width;
							break;
					}
					
					// Учитываем смещение для сдвига
					x_cursor += Label->dx;
				}
				
				// Ограничиваем позицию курсора границами виджета
				if(x_cursor < x1) x_cursor = x1;
				if(x_cursor >= x2) x_cursor = x2 - 1;
				
				// Рисуем вертикальную линию (курсор)
SetColor(Label->TextColor);
SetLineThickness(0);  // 1 пиксель
SetLineType(GSP_SOLID_LINE);  // Сплошная линия
Line(x_cursor, y_cursor, x_cursor, y_cursor + h_str - 1);
			}
		}

		if(Label->WidthBorder)
		{
SetLineThickness(Label->WidthBorder-1);		
SetColor(Label->BorderColor);
Bevel(x1,y1,x2,y2, Label->R);
		}
SetClip(false);
	}
}
//*********************************************
void LabelRunShift(_GSP_Label *Label)
{
	if(Label->En_Shift)
	{
		GSP_RunShift(Label->Text, Label->Font, Label->lenx, Label->Mode_Shift,
		             &Label->old_let_text, &Label->dx, &Label->sh, &Label->delay, Label->LetterSpacing);
	}
}
//*********************************************
void LabelRunBlinc(_GSP_Label *Label)
{
	if(Label->Time_Blinc)
	{
		if(labs(Timer_GUI - Label->old_timer) >= Label->Time_Blinc)
		{
			Label->old_timer = Timer_GUI;
			if(Label->BlincTogle) Label->BlincTogle = false;
			else				  Label->BlincTogle = true;
		}
	}
	else Label->BlincTogle = true;
}
//*********************************************
void LabelRunCursor(_GSP_Label *Label)
{
	if(!Label->en_edit) return;
	
	// Период мигания курсора: 500ms (Timer_GUI измеряется в миллисекундах)
	const int cursor_period = PERIOD_BLINC_CURSOR;
	
	if(labs(Timer_GUI - Label->cursor_timer) >= cursor_period)
	{
		Label->cursor_timer = Timer_GUI;
		Label->cursor_visible = !Label->cursor_visible;
	}
}

//****************************************************************
int LabelGetWidth(_GSP_Label *Label)
{
	return(Label->lenx);
}
//****************************************************************
int LabelGetHigth(_GSP_Label *Label)
{
	return(Label->leny);
}
//*************************************************
const char *LabelGetText(_GSP_Label *Label)
{
	if(!Label) return NULL;
	return Label->Text;
}
//*************************************************
void LabelSetLetterSpacing(_GSP_Label *Label, int spacing)
{
	if(Label)
	{
		Label->LetterSpacing = spacing;
		// При изменении межбуквенного интервала обновляем ширину в режиме AutoWidth
		LabelUpdateAutoWidth(Label);
	}
}
//*************************************************
void LabelSetVerticalOffset(_GSP_Label *Label, int offset)
{
	if(Label)
	{
		Label->dy = offset;
	}
}
//*************************************************
void LabelSetAutoWidth(_GSP_Label *Label, bool enable)
{
	if(!Label) return;
	Label->AutoWidth = enable;
	// При включении режима автоширины сразу обновляем размер
	if(enable)
	{
		LabelUpdateAutoWidth(Label);
	}
}

//*************************************************
void LabelSetEditMode(_GSP_Label *Label, bool enable)
{
	if(!Label) return;
	Label->en_edit = enable;
	
	// Если включаем режим редактирования и задана максимальная длина,
	// перевыделяем память для текста с учетом max_text_length
	if(enable && Label->max_text_length > 0 && !Label->Static)
	{
		int current_len = (Label->Text && Label->Text[0] != '\0') ? strlen((char *)Label->Text) : 0;
		if(current_len <= Label->max_text_length)
		{
			// Перевыделяем память под максимальную длину
			char *old_text = (char *)Label->Text;
			Label->Text = user_realloc((char *)Label->Text, Label->max_text_length + 1);
			if(!Label->Text)
			{
				// user_realloc вернул NULL - восстанавливаем старый указатель
				Label->Text = old_text;
			}
			else if(!old_text || (old_text && old_text[0] == '\0'))
			{
				// Если текст был NULL или пустой, гарантируем нулевой терминатор
				((char *)Label->Text)[0] = '\0';
			}
		}
	}
}

//*************************************************
void LabelSetMaxTextLength(_GSP_Label *Label, int max_length)
{
	if(!Label) return;
	
	if(max_length < 0) max_length = 0;  // 0 означает без ограничений
	
	Label->max_text_length = max_length;
	
	// Если режим редактирования включен и задана максимальная длина,
	// перевыделяем память для текста
	if(Label->en_edit && max_length > 0 && !Label->Static)
	{
		int current_len = (Label->Text && Label->Text[0] != '\0') ? strlen((char *)Label->Text) : 0;
		char *old_text = (char *)Label->Text;
		
		if(current_len <= max_length)
		{
			// Перевыделяем память под максимальную длину
			Label->Text = user_realloc((char *)Label->Text, max_length + 1);
			if(!Label->Text)
			{
				// user_realloc вернул NULL - восстанавливаем старый указатель
				Label->Text = old_text;
			}
			else if(!old_text || (old_text && old_text[0] == '\0'))
			{
				// Если текст был NULL или пустой, гарантируем нулевой терминатор
				((char *)Label->Text)[0] = '\0';
			}
		}
		else
		{
			// Если текущий текст длиннее нового максимума, обрезаем его
			Label->Text = user_realloc((char *)Label->Text, max_length + 1);
			if(!Label->Text)
			{
				// user_realloc вернул NULL - восстанавливаем старый указатель
				Label->Text = old_text;
			}
			else
			{
				((char *)Label->Text)[max_length] = '\0';
			}
		}
	}
}

//*************************************************
void LabelSetTextInvert(_GSP_Label *Label, bool invert)
{
	if(Label) Label->TextInvert = invert;
}

//*************************************************
void LabelAppendChar(_GSP_Label *Label, const char *utf8_char)
{
	if(!Label || !utf8_char || !utf8_char[0]) return;
	if(Label->Static) return;  // нельзя изменять статический текст
	if(!Label->en_edit) return;  // режим редактирования не включен
	
	if(!Label->Text) return;
	
	int current_len = strlen((char *)Label->Text);
	
	// Вычисляем размер добавляемого символа (для UTF-8 может быть 1-4 байта)
	int char_size = 0;
	unsigned char first_byte = (unsigned char)utf8_char[0];
	if((first_byte & 0x80) == 0)
	{
		// ASCII символ (1 байт)
		char_size = 1;
	}
	else if((first_byte & 0xE0) == 0xC0)
	{
		// 2 байта
		char_size = 2;
	}
	else if((first_byte & 0xF0) == 0xE0)
	{
		// 3 байта
		char_size = 3;
	}
	else if((first_byte & 0xF8) == 0xF0)
	{
		// 4 байта
		char_size = 4;
	}
	else
	{
		char_size = 1;  // по умолчанию 1 байт
	}
	
	// Проверяем ограничение максимальной длины
	if(Label->max_text_length > 0)
	{
		if(current_len + char_size > Label->max_text_length)
		{
			return;  // превышена максимальная длина
		}
	}
	
	// Убеждаемся, что память выделена достаточно
	if(Label->max_text_length > 0)
	{
		// Память уже должна быть выделена под max_text_length
	}
	else
	{
		// Выделяем память динамически
		Label->Text = user_realloc((char *)Label->Text, current_len + char_size + 1);
		if(!Label->Text) return;
	}
	
	// Добавляем символ в конец
	memcpy((char *)Label->Text + current_len, utf8_char, char_size);
	((char *)Label->Text)[current_len + char_size] = '\0';
	
	// Обновляем ширину в режиме AutoWidth
	LabelUpdateAutoWidth(Label);
}

//*************************************************
void LabelRemoveLastChar(_GSP_Label *Label)
{
	if(!Label) return;
	if(Label->Static) return;  // нельзя изменять статический текст
	if(!Label->en_edit) return;  // режим редактирования не включен
	
	if(!Label->Text) return;
	
	int current_len = strlen((char *)Label->Text);
	if(current_len == 0) return;  // строка пустая
	
	// Находим начало последнего UTF-8 символа
	// Идем назад от конца строки, пока не найдем начало символа
	int char_start = current_len - 1;
	
	// Ищем начало UTF-8 символа (байт, который не является продолжением)
	while(char_start > 0 && ((unsigned char)((char *)Label->Text)[char_start] & 0xC0) == 0x80)
	{
		char_start--;
	}
	
	// Вычисляем размер последнего символа
	int char_size = current_len - char_start;
	
	// Удаляем символ (просто ставим нулевой терминатор)
	((char *)Label->Text)[char_start] = '\0';
	
	// Обновляем ширину в режиме AutoWidth
	LabelUpdateAutoWidth(Label);
}
#endif