#include <p32xxxx.h>
#include <xc.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include "GUI_GSP.h"
#include "GSP_Message.h"
#include "GSP_DrawText.h"
#include "GSP_mem_monitor.h"

#ifdef USED_MESSAGE

//*****************************************************************
_GSP_Message *Crate_Message(_GSP_Screen *Screen, int x, int y, int lenx, int leny, int ID)
{
	_GSP_Message *Message = user_malloc(sizeof(_GSP_Message));
	IncrementListWidgets(Screen, (int *)Message);
	
	Message->Type = GSP_MESSAGE;
	Message->lenx = lenx;
	Message->leny = leny;
	Message->ID = ID;
	
	// Инициализируем выравнивание виджета по умолчанию
	Message->WidgetAlign = GSP_WIDGET_ALIGN_TOP_LEFT;
	
	// Сохраняем относительные координаты для будущего пересчета
	Message->rel_x = x;
	Message->rel_y = y;
	
	// Вычисляем абсолютные координаты с учетом выравнивания
	int abs_x = x;
	int abs_y = y;
	CalculateWidgetPosition(Screen, &abs_x, &abs_y, lenx, leny, Message->WidgetAlign);
	Message->x = abs_x;
	Message->y = abs_y;
	
	Message->BackColor = TRANSPARENT;
	Message->TextColor = DEFAULT_COLOR_TEXT;
	Message->BorderColor = DEFAULT_COLOR_TEXT;
	Message->Static = false;
	Message->Text = user_malloc(strlen(DEFAULT_TEXT) + 1);
	strcpy((char *)Message->Text, DEFAULT_TEXT);
	Message->Font = DEFAULT_FONT;
	Message->R = 0;
	Message->AllignH = ALLIGN_TEXT_CENTER;
	Message->AllignV = ALLIGN_VERT_CENTER;
	Message->WidthBorder = 1;
	Message->Visible = true;
	Message->lifetime_ms = 0;  // По умолчанию бесконечное время жизни
	Message->creation_time = Timer_GUI;  // Запоминаем время создания
	Message->expired = false;  // Флаг истечения времени жизни
	return(Message);
}

//***********************************************
void MessageSetText(_GSP_Message *Message, const char *text)
{
	if(Message && text)
	{
		if(!Message->Static)
		{
			char *new_text = user_realloc((char *)Message->Text, strlen(text) + 1);
			if(new_text)
			{
				Message->Text = new_text;
				strcpy((char *)Message->Text, text);
			}
		}
		else Message->Text = text;
	}
}

//***********************************************
void MessageSetFont(_GSP_Message *Message, void *font)
{
	if(Message) Message->Font = font;
}

//*************************************************
void MessageSetAllignH(_GSP_Message *Message, int allign)
{
	if(Message) Message->AllignH = allign;
}

//*************************************************
void MessageSetAllignV(_GSP_Message *Message, int allign)
{
	if(Message) Message->AllignV = allign;
}

//*************************************************
// Установка выравнивания виджета относительно Screen
void MessageSetWidgetAlign(_GSP_Message *Message, int align)
{
	if(!Message) return;
	
	Message->WidgetAlign = align;
	
	// Пересчитываем координаты относительно Active_Screen
	_GSP_Screen *Screen = Get_Active_Screen();
	if(Screen)
	{
		// Используем сохраненные относительные координаты для пересчета
		int abs_x = Message->rel_x;
		int abs_y = Message->rel_y;
		CalculateWidgetPosition(Screen, &abs_x, &abs_y, Message->lenx, Message->leny, align);
		Message->x = abs_x;
		Message->y = abs_y;
	}
}

//*************************************************
// Установка позиции виджета с пересчетом координат
void MessageSetPos(_GSP_Message *Message, int x, int y)
{
	if(!Message) return;
	
	// Обновляем относительные координаты
	Message->rel_x = x;
	Message->rel_y = y;
	
	// Пересчитываем абсолютные координаты с учетом текущего выравнивания
	_GSP_Screen *Screen = Get_Active_Screen();
	if(Screen)
	{
		int abs_x = x;
		int abs_y = y;
		CalculateWidgetPosition(Screen, &abs_x, &abs_y, Message->lenx, Message->leny, Message->WidgetAlign);
		Message->x = abs_x;
		Message->y = abs_y;
	}
}

//*************************************************
// Установка времени жизни виджета в миллисекундах
// lifetime_ms = 0 означает бесконечное время жизни
void MessageSetLifetime(_GSP_Message *Message, unsigned int lifetime_ms)
{
	if(Message)
	{
		Message->lifetime_ms = lifetime_ms;
		Message->creation_time = Timer_GUI;  // Сбрасываем время создания при установке времени жизни
		Message->expired = false;  // Сбрасываем флаг истечения
	}
}

//*************************************************
// Проверка и удаление истекших виджетов Message
// Должна вызываться после отрисовки всех виджетов
// Безопасна для вызова даже если Screen был удален (проверяет валидность Screen)
void MessageCheckAndDeleteExpired(_GSP_Screen *Screen)
{
	if(!Screen) return;
	
	// Дополнительная проверка: убеждаемся, что Screen все еще валиден
	// Проверяем, что Screen->ListWidgets не NULL (может быть NULL если Screen был удален)
	if(!Screen->ListWidgets) return;
	
	// Проходим по списку виджетов в обратном порядке для безопасного удаления
	for(int i = Screen->N - 1; i >= 0; i--)
	{
		// Проверяем валидность указателя перед использованием
		if(Screen->ListWidgets[i] && i < Screen->N)
		{
			unsigned char widget_type = *(unsigned char *)Screen->ListWidgets[i];
			if(widget_type == GSP_MESSAGE)
			{
				_GSP_Message *Message = (_GSP_Message *)Screen->ListWidgets[i];
				if(Message && Message->expired)
				{
					// Удаляем истекший виджет
					// GUI_DeleteWidget безопасна, так как проверяет принадлежность виджета экрану
					GUI_DeleteWidget(Screen, Message);
				}
			}
		}
	}
}

//*************************************************
// Функция для разбиения строки на строки с автоматическим переносом
// Возвращает количество строк и заполняет массив указателей на строки
static int WrapText(const char *text, int max_width, lv_font_t *font, char ***lines)
{
	if(!text || !font || max_width <= 0)
	{
		*lines = NULL;
		return 0;
	}
	
	int max_lines = 100;
	char **result_lines = user_malloc(sizeof(char *) * max_lines);
	if(!result_lines)
	{
		*lines = NULL;
		return 0;
	}
	int line_idx = 0;
	int text_len = strlen(text);
	int line_start = 0;
	
	// Проходим по тексту и разбиваем на строки
	for(int i = 0; i <= text_len; i++)
	{
		if(text[i] == '\n' || text[i] == '\0')
		{
			int line_len = i - line_start;
			if(line_len > 0)
			{
				// Обрабатываем строку: копируем символы и добавляем \n при переполнении
				char *wrapped = user_malloc(line_len + 50);
				if(!wrapped) continue; // Пропускаем эту строку, если не удалось выделить память
				int wrapped_pos = 0;
				int current_start = 0;
				
				for(int j = 0; j < line_len; j++)
				{
					char ch = text[line_start + j];
					
					// Проверяем ширину текущей строки перед добавлением символа
					if(wrapped_pos > current_start)
					{
						wrapped[wrapped_pos] = '\0';
						int current_width = GetTextWidthUTF8(font, wrapped + current_start, 0);
						
						char test_char[2] = {ch, '\0'};
						int char_width = GetTextWidthUTF8(font, test_char, 0);
						
						if(current_width + char_width >= max_width && current_width > 0)
						{
							// Добавляем перенос
							wrapped[wrapped_pos] = '\n';
							wrapped_pos++;
							current_start = wrapped_pos;
							
							// Если текущий символ - пробел, пропускаем его
							if(ch == ' ' || ch == '\t')
							{
								continue; // Пропускаем пробел после переноса
							}
						}
					}
					
					wrapped[wrapped_pos++] = ch;
				}
				wrapped[wrapped_pos] = '\0';
				
				// Разбиваем по \n и создаем строки
				int substr_start = 0;
				for(int j = 0; j <= wrapped_pos; j++)
				{
					if(wrapped[j] == '\n' || wrapped[j] == '\0')
					{
						int substr_len = j - substr_start;
						if(line_idx < max_lines)
						{
							result_lines[line_idx] = user_malloc(substr_len + 1);
							if(result_lines[line_idx])
							{
								strncpy(result_lines[line_idx], wrapped + substr_start, substr_len);
								result_lines[line_idx][substr_len] = '\0';
								line_idx++;
							}
						}
						substr_start = j + 1;
					}
				}
				user_free(wrapped);
			}
			else if(line_idx < max_lines)
			{
				// Пустая строка
				result_lines[line_idx] = user_malloc(1);
				if(result_lines[line_idx])
				{
					result_lines[line_idx][0] = '\0';
					line_idx++;
				}
			}
			line_start = i + 1;
		}
	}
	
	if(line_idx == 0)
	{
		result_lines[0] = user_malloc(1);
		if(result_lines[0])
		{
			result_lines[0][0] = '\0';
			line_idx = 1;
		}
		else
		{
			// Не удалось выделить память - освобождаем массив и возвращаем ошибку
			user_free(result_lines);
			*lines = NULL;
			return 0;
		}
	}
	
	*lines = result_lines;
	return line_idx;
}

//*************************************************
void DrawingMessage(_GSP_Message *Message)
{
	if(!Message || !Message->Visible) return;
	
	// Проверка, что виджет все еще принадлежит активному экрану
	// Это защита от ситуации, когда Screen был удален, а виджет еще не удален
	_GSP_Screen *Screen = Get_Active_Screen();
	if(!Screen) return;  // Если нет активного экрана, не рисуем
	
	// Проверяем, что виджет принадлежит активному экрану
	bool widget_found = false;
	for(int i = 0; i < Screen->N; i++)
	{
		if(Screen->ListWidgets[i] == (int *)Message)
		{
			widget_found = true;
			break;
		}
	}
	if(!widget_found) return;  // Виджет не принадлежит активному экрану
	
	// Проверка времени жизни виджета
	if(Message->lifetime_ms > 0 && !Message->expired)
	{
		unsigned int elapsed = Timer_GUI - Message->creation_time;
		if(elapsed >= Message->lifetime_ms)
		{
			// Время жизни истекло - помечаем для удаления
			Message->expired = true;
			Message->Visible = false;  // Скрываем виджет
			return;  // Не рисуем, так как виджет истек
		}
	}
	
	// Если виджет истек, не рисуем его
	if(Message->expired) return;
	
	int lenx = Message->lenx;
	int leny = Message->leny;
	int x1 = Message->x;
	int y1 = Message->y;
	int x2 = Message->x + lenx;
	int y2 = Message->y + leny;
	int a_x = Active_Screen->x;
	int a_len_x = Active_Screen->lenx;
	int a_y = Active_Screen->y;
	int a_len_y = Active_Screen->leny;
	
	if(x2 > a_x + a_len_x) x2 = a_x + a_len_x;
	if(y2 > a_y + a_len_y) y2 = a_y + a_len_y;

SetClipRgn(x1, y1, x2, y2);
SetClip(true);
	if(Message->BackColor != TRANSPARENT)
	{
SetColor(Message->BackColor);
FillBevel(x1, y1, x2, y2, Message->R);
	}
	
	// Разбиваем текст на строки с автоматическим переносом
	// Учитываем возможные отступы (минус 2 пикселя для безопасности)
	int available_width = lenx - 2;
	if(available_width < 10) available_width = lenx; // Минимальная ширина
	
	char **lines = NULL;
	int n_str = WrapText(Message->Text, available_width, Message->Font, &lines);
	
	if(n_str == 0 || !lines)
	{
SetClip(false);
		return;
	}
	
	// Вычисление высоты одной строки и общей высоты текста
	short h_str = GetTextHightUTF8(Message->Font);
	short total_h = h_str * n_str;
	
	// Вычисление начальной позиции Y с учетом вертикального выравнивания
	int start_y;
	switch(Message->AllignV)
	{
		case ALLIGN_VERT_TOP:
			start_y = y1;
			break;
		case ALLIGN_VERT_CENTER:
			start_y = y1 + (leny - total_h) / 2;
			break;
		case ALLIGN_VERT_BOTTOM:
			start_y = y1 + leny - total_h;
			break;
		default:
			start_y = y1 + (leny - total_h) / 2;
			break;
	}
	
	// Отрисовка строк
	int y = start_y;
	for(int i = 0; i < n_str; i++)
	{
		if(lines[i])
		{
			DrawAllignTextUTF8(x1, y, lenx, Message->AllignH, lines[i], Message->Font, Message->TextColor, 0, 0);
			user_free(lines[i]);
		}
		y += h_str;
	}
	user_free(lines);
	
	if(Message->WidthBorder)
	{
SetLineThickness(Message->WidthBorder - 1);
SetColor(Message->BorderColor);
Bevel(x1, y1, x2, y2, Message->R);
	}
SetClip(false);
}

#endif

