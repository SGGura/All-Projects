#include <p32xxxx.h>
#include <xc.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include "GUI_GSP.h"
#include "GSP_Table.h"
#include "GSP_DrawText.h"
#include "GSP_mem_monitor.h"

#ifdef USED_TABLE

//*******************************************************************
static _Table_Cell *GetCell(_GSP_Table *Table, int r, int c)
{
	if(!Table || !Table->Cells) return(NULL);
	if(r < 0 || r >= Table->Rows || c < 0 || c >= Table->Colls) return(NULL);
	int n = r*Table->Colls + c;
	return((_Table_Cell *)*(Table->Cells+n));  
}
//************************************
static void InitCell(_Table_Cell *Cell, _GSP_Table *Table, int r, int c)
{
	Cell->TextColor = DEFAULT_COLOR_TEXT;
	Cell->BackColor = TRANSPARENT;
	Cell->En_Shift = false;
	Cell->Font = Table->Font;					// фонт одина на все ячейки
	Cell->lenx = Table->lencoll[c]; 
	Cell->BorderColor = Table->BorderColor;
	Cell->WidthBorder = Table->WidthBorder;
	char buf[20];
	sprintf(buf, "Cell:%d %d  ", r, c);
	Cell->Text = user_malloc(strlen(buf)+1);		// начальное выделение памяти строки (+1 для '\0')
	strcpy((char *)Cell->Text, buf);	
	Cell->Allign = ALLIGN_TEXT_CENTER;
	Cell->Time_Blinc = 0;
	Cell->BlincTogle = true;
	Cell->dx = 0;
	Cell->sh = false;
	Cell->delay = 0;
	Cell->Static = false;					// ячейка владеет своей памятью
	Cell->old_let_text = -1;				// инициализация для сдвига
	Cell->Mode_Shift = GSP_SHIFT_INFINITY;
}
//*****************************************************************
_GSP_Table *Crate_Table(_GSP_Screen *MainScreen, int x, int y, int lenx, int leny, int ID)
{
	_GSP_Table *Table = user_malloc(sizeof(_GSP_Table));
	IncrementListWidgets(MainScreen, (int *)Table);
	
	Table->Type = GSP_TABLE;
	Table->ID = ID;
	Table->lenx = lenx;
	Table->leny = leny;
	
	// Инициализируем выравнивание виджета по умолчанию
	Table->WidgetAlign = GSP_WIDGET_ALIGN_TOP_LEFT;
	
	// Сохраняем относительные координаты для будущего пересчета
	Table->rel_x = x;
	Table->rel_y = y;
	
	// Вычисляем абсолютные координаты с учетом выравнивания
	int abs_x = x;
	int abs_y = y;
	CalculateWidgetPosition(MainScreen, &abs_x, &abs_y, lenx, leny, Table->WidgetAlign);
	Table->x = abs_x;
	Table->y = abs_y;
	
	Table->BackColor = WHITE;
	Table->BorderColor = BLACK;
	Table->WidthBorder = 0;
	Table->R = 0;
	Table->Visible = true;
	Table->Font = DEFAULT_FONT;
	Table->Rows = 0;
	Table->Colls = 0;
	Table->SelectedRow = -1;
	Table->SelectedColl = -1;
	Table->SelectedBackColor = BLACK;
	Table->SelectedTextColor = WHITE;
	
	// Таблица создается пустой, без строк и колонок
	// Ячейки будут созданы при добавлении строк и колонок через TableAddColl и TableInsertRow
	Table->Cells = NULL;
	Table->lencoll = NULL;
	
	return(Table);
}
//***********************************************
void TableSetText(_GSP_Table *Table, int row, int coll, const char * text)
{
	_Table_Cell *Cell = GetCell(Table, row, coll);
	if(!Cell) return;
	
	if(!Cell->Static)
	{
		Cell->Text = user_realloc((char *)Cell->Text, strlen(text)+1);
		if(Cell->Text)
		{
			strcpy((char *)Cell->Text, text);
		}
	}
	else
	{
		Cell->Text = text;  // статический текст - просто присваиваем указатель
	}
}
//***********************************************
void TableSetTextFmt(_GSP_Table *Table, int row, int coll, const char *format, ...)
{
	_Table_Cell *Cell = GetCell(Table, row, coll);
	if(!Cell) return;
	
	char buffer[256];  // Буфер для форматированной строки
	va_list args;
	va_start(args, format);
	vsnprintf(buffer, sizeof(buffer), format, args);
	va_end(args);
	
	if(!Cell->Static)
	{
		Cell->Text = user_realloc((char *)Cell->Text, strlen(buffer)+1);
		if(Cell->Text)
		{
			strcpy((char *)Cell->Text, buffer);
		}
	}
	else
	{
		// Если ячейка статическая, нельзя изменять текст
		// В этом случае просто игнорируем
	}
}
//***********************************************
void TableSetFontCell(_GSP_Table *Table, int row, int coll, void *font)
{
	_Table_Cell *Cell = GetCell(Table, row, coll);
	if(!Cell) return;
	Cell->Font = font;  
}
//***********************************************
void TableSetTextBlinc(_GSP_Table *Table, int row, int coll, int time)
{
	_Table_Cell *Cell = GetCell(Table, row, coll);
	if(!Cell) return;
	Cell->Time_Blinc = time;  
}
//***********************************************
void TableSetTextShift(_GSP_Table *Table, int row, int coll, bool en)
{
	_Table_Cell *Cell = GetCell(Table, row, coll);
	if(!Cell) return;
	Cell->En_Shift = en;  
}
//***********************************************
void TableSetColorBK(_GSP_Table *Table, int row, int coll, WORD_COLOR color)
{
	_Table_Cell *Cell = GetCell(Table, row, coll);
	if(!Cell) return;
	Cell->BackColor = color;
}
//***********************************************
void TableSetColorText(_GSP_Table *Table, int row, int coll, WORD_COLOR color)
{
	_Table_Cell *Cell = GetCell(Table, row, coll);
	if(!Cell) return;
	Cell->TextColor = color;
}
//***********************************************
void TableSetBorder(_GSP_Table *Table, int row, int coll, WORD_COLOR color, int Width)
{
	_Table_Cell *Cell = GetCell(Table, row, coll);
	if(!Cell) return;
	Cell->BorderColor = color;
	Cell->WidthBorder = Width;
}
//***********************************************
//***********************************************
void TableAddColl(_GSP_Table *Table, int len)
{
	TableInsertColl(Table, len, Table->Colls);
}
//***********************************************
void TableInsertColl(_GSP_Table *Table, int len, int coll)
{
	Table->Colls+=1;
	// Если таблица была пустой, выделяем память впервые
	if(Table->Cells == NULL)
	{
		Table->Cells = user_malloc(Table->Rows*Table->Colls*sizeof(int));
		if(!Table->Cells)
		{
			Table->Colls--; // Откатываем изменение
			return;
		}
		// Инициализируем все ячейки как NULL
		for(int i = 0; i < Table->Rows*Table->Colls; i++)
		{
			*(Table->Cells + i) = 0;
		}
	}
	else
	{
		int *new_cells = user_realloc(Table->Cells, Table->Rows*Table->Colls*sizeof(int));
		if(!new_cells)
		{
			Table->Colls--; // Откатываем изменение
			return;
		}
		Table->Cells = new_cells;
	}
	
	if(Table->lencoll == NULL)
	{
		Table->lencoll = user_malloc(Table->Colls*sizeof(short));
		if(!Table->lencoll)
		{
			// Откатываем изменения
			Table->Colls--;
			if(Table->Cells && Table->Colls == 0)
			{
				user_free(Table->Cells);
				Table->Cells = NULL;
			}
			return;
		}
	}
	else
	{
		short *new_lencoll = user_realloc(Table->lencoll, Table->Colls*sizeof(short));
		if(!new_lencoll)
		{
			// Откатываем изменения
			Table->Colls--;
			if(Table->Cells)
			{
				int *new_cells = user_realloc(Table->Cells, Table->Rows*Table->Colls*sizeof(int));
				if(new_cells) Table->Cells = new_cells;
			}
			return;
		}
		Table->lencoll = new_lencoll;
	}
	// Сместить массив длин колонок ( расширить)
	if(Table->Colls > 1)
	{
	for(int i = Table->Colls-2;i>=coll;--i)
	{
		*(Table->lencoll+i+1) = *(Table->lencoll + i);
		}
	}
	Table->lencoll[coll] = len;
	
	// Переместить существующие ячейки, если они есть
	if(Table->Rows > 0 && Table->Colls > 1)
	{
	int n_src = Table->Rows*(Table->Colls-1)-1;
	int n_dst = Table->Rows*Table->Colls-1;
	int n_col= coll;
	for(int r=Table->Rows; r>0; r--)
	{
		for(int c=Table->Colls; c>0; c--)
		{
			if(n_dst % Table->Colls == n_col)
			{
				*(Table->Cells + n_dst) = 0;
				n_dst--;
			}
			else
			{
				*(Table->Cells + n_dst) = *(Table->Cells + n_src);
				n_dst--;
				n_src--;
				}
			}
		}
	}	
	
	// Инициализировать новые ячейки
	for(int r = 0;r<Table->Rows;++r)
	{
		int n = r*Table->Colls + coll;
		_Table_Cell *Cell = (_Table_Cell *)user_malloc(sizeof(_Table_Cell));
		*(Table->Cells + n) = (int)Cell;
		InitCell(Cell, Table, r, coll);
	}
}
//***********************************************
void TableDelColl(_GSP_Table *Table, int coll)
{
	// Освободить память
	for(int r = 0;r<Table->Rows;++r)
	{
		int n = r*Table->Colls + coll;
		_Table_Cell *Cell = (_Table_Cell *)*(Table->Cells + n);
		if(Cell && !Cell->Static && Cell->Text)
		{
			user_free((char *)Cell->Text);
		}
		user_free(Cell);
	}
	
	int n_tbl = Table->Rows*Table->Colls;
	int n_col=coll, k=0;
	for(int n=0; n<n_tbl; n++)
	{
		if(n % Table->Colls != n_col)
		{
			*(Table->Cells + k) = *(Table->Cells + n);
			k++;
		}
	}	
	// Сместить массив длин колонок ( сжать)
	for(int i = coll;i<Table->Colls;++i)
	{
		*(Table->lencoll+i) = *(Table->lencoll + i+1);
	}
	
	Table->Colls -= 1;
	// Изменяем размер массива ячеек
	int *new_cells = user_realloc(Table->Cells, Table->Rows*Table->Colls*sizeof(int));
	if(new_cells)
	{
		Table->Cells = new_cells;
	}
	// Изменяем размер массива длин колонок
	if(Table->lencoll)
	{
		short *new_lencoll = user_realloc(Table->lencoll, Table->Colls*sizeof(short));
		if(new_lencoll)
		{
			Table->lencoll = new_lencoll;
		}
	}
}
//*************************************************
void DrawingTable(_GSP_Table *Table)
{
	if(Table->Visible)
	{
SetColor(Table->BackColor);
		int lenx = Table->lenx;
		int leny = Table->leny;
		int x1 = Table->x;
		int y1 = Table->y;
		int x2 = Table->x +lenx;
		int y2 = Table->y +leny;
		int a_x = Active_Screen->x;
		int a_len_x = Active_Screen->lenx;
		int a_y = Active_Screen->y;
		int a_len_y = Active_Screen->leny;
		
		if( x2 > a_x+a_len_x) x2 = a_x+a_len_x;
		if( y2 > a_y+a_len_y) y2 = a_y+a_len_y;

SetClipRgn(x1, y1, x2, y2);
SetClip(true);
		if(Table->BackColor != TRANSPARENT)
		{
SetColor(Table->BackColor);
FillBevel(x1,y1,x2,y2, Table->R);
		}
		
		int n = 0;
		short h =  GetTextHightUTF8(Table->Font);
		short row_height = Table->leny / Table->Rows;  // Высота одной строки
		for(int r = 0;r<Table->Rows; ++r)
		{
			for(int c = 0;c<Table->Colls; ++c)
			{
				_Table_Cell *Cell = (_Table_Cell *)*(Table->Cells+n++); 
				int lencoll = Table->lencoll[c];
				// обработка сдвига - используем ту же процедуру, что в Label и List
				if(Cell->En_Shift)
				{
					GSP_RunShift(Cell->Text, Cell->Font, lencoll, Cell->Mode_Shift,
					              &Cell->old_let_text, &Cell->dx, &Cell->sh, &Cell->delay, 0);
				}
				// обработка мигания
				LabelRunBlinc(Cell);
				
				// отрисовка ячейки
				short y = y1 + row_height * r;  // Позиция строки
				short y_cell = y1 + row_height * (r + 1);  // Нижняя граница строки
				int shift = 0;
				for(int i = 0; i < c; i++)
				{
					shift += Table->lencoll[i];
				}
				int x = x1 + shift;
				int x_cell =  x+lencoll;
				if(x_cell > x2) x_cell = x2; 
				if(y_cell > y2) y_cell = y2;
SetClipRgn(x, y, x_cell, y_cell);
				if(Cell->BlincTogle)
				{
					// Проверка, является ли ячейка выбранной
					bool is_selected = (Table->SelectedRow == r && Table->SelectedColl == c);
					
					if(is_selected)
					{
						// Для выделенной ячейки: 1. стереть ячейку цветом фона таблицы
SetColor(Table->BackColor);
FillBevel(x, y, x_cell, y_cell, 0);
						
						// 2. закрасить область с отступом 2 пикселя со всех сторон
						WORD_COLOR cell_back_color = Table->SelectedBackColor;
						WORD_COLOR cell_text_color = Table->SelectedTextColor;
						
						if(cell_back_color != TRANSPARENT)
					{
SetColor(cell_back_color);
FillBevel(x+2, y+2, x_cell-2, y_cell-2, 0);
					}
						// 3. вывести текст
						short text_y = y + (row_height - h) / 2;
						DrawAllignTextUTF8(x+Cell->dx+1, text_y,  lencoll, Cell->Allign, (char *)Cell->Text , Cell->Font, cell_text_color, 0, 0);
					}
					else
					{
						// Для обычной ячейки - стандартная закраска без отступа
						WORD_COLOR cell_back_color = Cell->BackColor;
						WORD_COLOR cell_text_color = Cell->TextColor;
						
						if(cell_back_color != TRANSPARENT)
						{
SetColor(cell_back_color);
FillBevel(x, y, x_cell, y_cell, 0);
						}
						// Текст по центру строки
						short text_y = y + (row_height - h) / 2;
						DrawAllignTextUTF8(x+Cell->dx+1, text_y,  lencoll, Cell->Allign, (char *)Cell->Text , Cell->Font, cell_text_color, 0, 0);
					}
				}
				if(Cell->WidthBorder)
				{
SetLineThickness(Cell->WidthBorder-1);		
SetColor(Cell->BorderColor);
Bevel(x, y, x_cell, y_cell, 0);
				}
			}
		}
SetClipRgn(x1, y1, x2, y2);

		if(Table->WidthBorder)
		{
SetLineThickness(Table->WidthBorder-1);		
SetColor(Table->BorderColor);
Bevel(x1,y1,x2,y2, Table->R);
		}
SetClip(false);
	}
}
//*************************************************
void TableDelRow(_GSP_Table *Table, int row)
{
	// Освободить память
	for(int c = 0;c<Table->Colls;++c)
	{
		int n = row*Table->Colls + c;
		_Table_Cell *Cell = (_Table_Cell *)*(Table->Cells + n);
		if(Cell && !Cell->Static && Cell->Text)
		{
			user_free((char *)Cell->Text);
		}
		user_free(Cell);
	}
	
	int n_tbl = Table->Rows*Table->Colls;
	int n_row=row, k=0;
	for(int n=0; n<n_tbl; n++)
	{
		if(n >= n_row*Table->Colls && n < (n_row+1)*Table->Colls)
		{
			// Пропускаем удаляемую строку
			continue;
		}
		*(Table->Cells + k) = *(Table->Cells + n);
		k++;
	}
	
	Table->Rows -= 1;
	// Изменяем размер массива ячеек
	int *new_cells = user_realloc(Table->Cells, Table->Rows*Table->Colls*sizeof(int));
	if(new_cells)
	{
		Table->Cells = new_cells;
	} 
}
//***********************************************
void TableInsertRow(_GSP_Table *Table, int row)
{
	Table->Rows+=1;
	// Если таблица была пустой, выделяем память впервые
	if(Table->Cells == NULL)
	{
		Table->Cells = user_malloc(Table->Rows*Table->Colls*sizeof(int));
		if(!Table->Cells)
		{
			Table->Rows--; // Откатываем изменение
			return;
		}
		// Инициализируем все ячейки как NULL
		for(int i = 0; i < Table->Rows*Table->Colls; i++)
		{
			*(Table->Cells + i) = 0;
		}
	}
	else
	{
		int *new_cells = user_realloc(Table->Cells, Table->Rows*Table->Colls*sizeof(int));
		if(!new_cells)
		{
			Table->Rows--; // Откатываем изменение
			return;
		}
		Table->Cells = new_cells;
	}
	
	// Переместить существующие ячейки, если они есть
	if(Table->Colls > 0)
	{
	int n_tbl = Table->Rows*Table->Colls - 1;
	int n_row=row, k;
	k = n_tbl - Table->Colls;
	for(int n=n_tbl; n>=0; n--)
	{
		if(n < n_row*Table->Colls) n=-1;
		else
		{
			if(n < (n_row + 1)*Table->Colls) *(Table->Cells + n) = 0;
			else
			{
				*(Table->Cells + n) = *(Table->Cells + k);
				k--;
			}
		}
	}	
	}
	
	// Инициализировать новые ячейки
	for(int c = 0;c<Table->Colls;++c)
	{
		int n = row*Table->Colls + c;
		_Table_Cell *Cell = (_Table_Cell *)user_malloc(sizeof(_Table_Cell));
		*(Table->Cells + n) = (int)Cell;
		InitCell(Cell, Table, row, c);
	}
}
//***********************************************
void TableDelete(_GSP_Table *Table)
{
	if(!Table) return;
	
	if(Table->Cells && Table->Rows > 0 && Table->Colls > 0)
	{
	for(int n=0; n<(Table->Rows*Table->Colls); ++n)
	{
		_Table_Cell *Cell = (_Table_Cell *)*(Table->Cells+n);
		if(Cell)
		{
			if(!Cell->Static && Cell->Text)
			{
				user_free((char *)Cell->Text);
			}
			user_free(Cell);
			}
		}
	}
	if(Table->lencoll) user_free(Table->lencoll);
	if(Table->Cells) user_free(Table->Cells);
}
//***********************************************
void TableSetSelected(_GSP_Table *Table, int row, int coll)
{
	if(!Table) return;
	if(row < 0 || row >= Table->Rows) row = -1;
	if(coll < 0 || coll >= Table->Colls) coll = -1;
	Table->SelectedRow = row;
	Table->SelectedColl = coll;
}
//***********************************************
void TableSetSelectedColors(_GSP_Table *Table, WORD_COLOR back_color, WORD_COLOR text_color)
{
	if(!Table) return;
	Table->SelectedBackColor = back_color;
	Table->SelectedTextColor = text_color;
}
//***********************************************
void TableGetSelected(_GSP_Table *Table, int *row, int *coll)
{
	if(!Table || !row || !coll) return;
	*row = Table->SelectedRow;
	*coll = Table->SelectedColl;
}
//***********************************************
void TableSetWidgetAlign(_GSP_Table *Table, int align)
{
	if(!Table) return;
	
	Table->WidgetAlign = align;
	
	// Находим родительский Screen для пересчета координат
	_GSP_Screen *Screen = Get_Active_Screen();
	if(!Screen) return;
	
	// Пересчитываем абсолютные координаты с учетом нового выравнивания
	int abs_x = Table->rel_x;
	int abs_y = Table->rel_y;
	CalculateWidgetPosition(Screen, &abs_x, &abs_y, Table->lenx, Table->leny, align);
	Table->x = abs_x;
	Table->y = abs_y;
}
//***********************************************
void TableSetTextAlign(_GSP_Table *Table, int row, int coll, int align)
{
	_Table_Cell *Cell = GetCell(Table, row, coll);
	if(!Cell) return;
	
	// Проверяем, что align является допустимым значением
	if(align != ALLIGN_TEXT_LEFT && align != ALLIGN_TEXT_CENTER && align != ALLIGN_TEXT_RIGHT)
		return;
	
	Cell->Allign = align;
}

#endif