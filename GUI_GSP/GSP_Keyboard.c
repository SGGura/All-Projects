#include <xc.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "GUI_GSP.h"
#include "GSP_Keyboard.h"
#include "GSP_DrawText.h"
#include "GSP_mem_monitor.h"

#ifdef USED_KEYBOARD
#define MAX_ROWS_KEYBOARD 6

//*************************************************
// Вспомогательная функция для получения индекса кнопки в массиве buttons по row и col
static int GetButtonArrayIndex(_GSP_Keyboard *Keyboard, int row, int col)
{
    if(!Keyboard || !Keyboard->row_lengths || !Keyboard->buttons) return -1;
    if(row < 0 || row >= Keyboard->rows) return -1;
    if(col < 0 || col >= Keyboard->row_lengths[row]) return -1;
    
    int current_row = 0;
    int current_col = 0;
    
    for(int i = 0; Keyboard->buttons[i] != NULL && Keyboard->buttons[i][0] != '\0'; i++)
    {
        const char *btn = Keyboard->buttons[i];
        
        if(strcmp(btn, "\n") == 0)
        {
            // Новая строка
            current_row++;
            current_col = 0;
        }
        else
        {
            // Это кнопка
            if(current_row == row && current_col == col)
            {
                return i;
            }
            current_col++;
        }
    }
    
    return -1;
}

//*************************************************
// Вспомогательная функция для парсинга массива кнопок
// Подсчитывает количество строк и кнопок в каждой строке
static void ParseButtons(_GSP_Keyboard *Keyboard)
{
    if(!Keyboard || !Keyboard->buttons) return;
    
    // Сохраняем старые указатели на случай ошибки
    int *old_row_lengths = Keyboard->row_lengths;
    float *old_button_width_multipliers = Keyboard->button_width_multipliers;
    
    // Обнуляем указатели перед выделением новой памяти
    Keyboard->row_lengths = NULL;
    Keyboard->button_width_multipliers = NULL;
    Keyboard->rows = 0;
    Keyboard->button_count = 0;
    
    // Первый проход: подсчитываем количество строк и кнопок
    int current_row_buttons = 0;
    int max_rows = MAX_ROWS_KEYBOARD;										// максимальное количество строк (можно увеличить при необходимости)
    int *temp_row_lengths = user_malloc(sizeof(int) * max_rows);
    if(!temp_row_lengths)
    {
        // Восстанавливаем старые указатели при ошибке
        Keyboard->row_lengths = old_row_lengths;
        Keyboard->button_width_multipliers = old_button_width_multipliers;
        return;
    }
    
    for(int i = 0; Keyboard->buttons[i] != NULL && Keyboard->buttons[i][0] != '\0'; i++)
    {
        const char *btn = Keyboard->buttons[i];
        
        // Проверяем, является ли это символом новой строки
        if(strcmp(btn, "\n") == 0)
        {
            // Конец текущей строки
            if(Keyboard->rows < max_rows)
            {
                temp_row_lengths[Keyboard->rows] = current_row_buttons;
                Keyboard->rows++;
            }
            current_row_buttons = 0;
        }
        else
        {
            // Это кнопка
            current_row_buttons++;
            Keyboard->button_count++;
        }
    }
    
    // Добавляем последнюю строку, если она не пустая
    if(current_row_buttons > 0 && Keyboard->rows < max_rows)
    {
        temp_row_lengths[Keyboard->rows] = current_row_buttons;
        Keyboard->rows++;
    }
    
    // Выделяем память для массива длин строк
    if(Keyboard->rows > 0)
    {
        Keyboard->row_lengths = user_malloc(sizeof(int) * Keyboard->rows);
        if(!Keyboard->row_lengths)
        {
            // Ошибка выделения памяти - освобождаем временный массив и восстанавливаем старые данные
            user_free(temp_row_lengths);
            Keyboard->row_lengths = old_row_lengths;
            Keyboard->button_width_multipliers = old_button_width_multipliers;
            return;
        }
        
        for(int i = 0; i < Keyboard->rows; i++)
        {
            Keyboard->row_lengths[i] = temp_row_lengths[i];
        }
    }
    
    // Выделяем память для массива множителей ширины и инициализируем все = 1.0f
    if(Keyboard->button_count > 0)
    {
        Keyboard->button_width_multipliers = user_malloc(sizeof(float) * Keyboard->button_count);
        if(!Keyboard->button_width_multipliers)
        {
            // Ошибка выделения памяти - освобождаем уже выделенное и восстанавливаем старые данные
            user_free(temp_row_lengths);
            if(Keyboard->row_lengths)
            {
                user_free(Keyboard->row_lengths);
                Keyboard->row_lengths = NULL;
            }
            Keyboard->row_lengths = old_row_lengths;
            Keyboard->button_width_multipliers = old_button_width_multipliers;
            return;
        }
        
        for(int i = 0; i < Keyboard->button_count; i++)
        {
            Keyboard->button_width_multipliers[i] = 1.0f; // по умолчанию все кнопки имеют множитель 1.0
        }
    }
    
    // Все успешно - освобождаем временный массив и старые данные
    user_free(temp_row_lengths);
    
    // Теперь безопасно освобождаем старые данные
    if(old_row_lengths) user_free(old_row_lengths);
    if(old_button_width_multipliers) user_free(old_button_width_multipliers);
    
    // Сбрасываем выбранную позицию
    Keyboard->selected_row = 0;
    Keyboard->selected_col = 0;
    if(Keyboard->rows > 0 && Keyboard->row_lengths && Keyboard->row_lengths[0] > 0)
    {
        Keyboard->selected_index = GetButtonArrayIndex(Keyboard, 0, 0);
    }
    else
    {
        Keyboard->selected_index = -1;
    }
}

//*************************************************
// Вспомогательная функция для получения текста кнопки по row и col
static const char *GetButtonTextByPosition(_GSP_Keyboard *Keyboard, int row, int col)
{
    if(!Keyboard || !Keyboard->buttons) return NULL;
    
    int array_index = GetButtonArrayIndex(Keyboard, row, col);
    if(array_index < 0) return NULL;
    
    return Keyboard->buttons[array_index];
}

//*****************************************************************
_GSP_Keyboard *Crate_Keyboard(_GSP_Screen *Screen, int x, int y, int lenx, int leny, int ID)
{
    _GSP_Keyboard *Keyboard = user_malloc(sizeof(_GSP_Keyboard));
    if(!Keyboard) return NULL;  // Проверка на ошибку выделения памяти
    
    // Сохраняем количество виджетов до добавления
    int old_N = Screen->N;
    IncrementListWidgets(Screen, (int *)Keyboard);
    
    // Проверяем, был ли виджет добавлен в список
    // Если N не увеличилось, значит не удалось выделить память для списка
    if(Screen->N == old_N)
    {
        // Не удалось добавить в список - освобождаем память и возвращаем NULL
        user_free(Keyboard);
        return NULL;
    }
    
    Keyboard->Type = GSP_KEYBOARD;
    Keyboard->lenx = lenx;
    Keyboard->leny = leny;
    Keyboard->ID = ID;
    
    // Инициализируем выравнивание виджета по умолчанию
    Keyboard->WidgetAlign = GSP_WIDGET_ALIGN_TOP_LEFT;
    
    // Сохраняем относительные координаты для будущего пересчета
    Keyboard->rel_x = x;
    Keyboard->rel_y = y;
    
    // Вычисляем абсолютные координаты с учетом выравнивания
    int abs_x = x;
    int abs_y = y;
    CalculateWidgetPosition(Screen, &abs_x, &abs_y, lenx, leny, Keyboard->WidgetAlign);
    Keyboard->x = abs_x;
    Keyboard->y = abs_y;
    
    Keyboard->BackColor = TRANSPARENT;
    Keyboard->TextColor = DEFAULT_COLOR_TEXT;
    Keyboard->SelColor = DEFAULT_COLOR_BAR;
    Keyboard->ButtonBorderColor = DEFAULT_COLOR_TEXT;
    Keyboard->Visible = true;
    Keyboard->Font = DEFAULT_FONT;
    
    // Инициализация данных кнопок
    Keyboard->buttons = NULL;
    Keyboard->button_count = 0;
    Keyboard->rows = 0;
    Keyboard->row_lengths = NULL;
    Keyboard->button_width_multipliers = NULL;
    
    // Параметры кнопок по умолчанию
    Keyboard->button_width = 20;
    Keyboard->button_height = 20;
    Keyboard->button_gap_x = -1;  // отрицательный зазор для перекрытия границ
    Keyboard->button_gap_y = -1;  // отрицательный зазор для перекрытия границ
    Keyboard->width_remainder = 0;
    Keyboard->height_remainder = 0;
    Keyboard->text_offset_y = 0;  // смещение текста по умолчанию (нет смещения)
    
    // Навигация
    Keyboard->selected_row = 0;
    Keyboard->selected_col = 0;
    Keyboard->selected_index = 0;
    
    return Keyboard;
}

//*************************************************
void KeyboardSetButtons(_GSP_Keyboard *Keyboard, const char **buttons)
{
    if(!Keyboard) return;
    
    Keyboard->buttons = buttons;
    
    // Если buttons == NULL, освобождаем старые данные и обнуляем счетчики
    if(!buttons)
    {
        if(Keyboard->row_lengths)
        {
            user_free(Keyboard->row_lengths);
            Keyboard->row_lengths = NULL;
        }
        if(Keyboard->button_width_multipliers)
        {
            user_free(Keyboard->button_width_multipliers);
            Keyboard->button_width_multipliers = NULL;
        }
        Keyboard->rows = 0;
        Keyboard->button_count = 0;
        Keyboard->selected_row = 0;
        Keyboard->selected_col = 0;
        Keyboard->selected_index = -1;
        return;
    }
    
    ParseButtons(Keyboard); // ParseButtons создает массив множителей и инициализирует все = 1
    
    // Автоматически рассчитываем размеры кнопок пропорционально размеру виджета
    // Размер рассчитывается от размера виджета с учетом множителей ширины кнопок
    if(Keyboard->rows > 0 && Keyboard->row_lengths && Keyboard->button_width_multipliers)
    {
        // Находим максимальную сумму множителей в строке (самая широкая строка)
        float max_total_multipliers = 0.0f;
        for(int row = 0; row < Keyboard->rows; row++)
        {
            float row_total_multipliers = 0.0f;
            int button_index = 0;
            // Подсчитываем сумму множителей для текущей строки
            for(int i = 0; i < row; i++)
            {
                button_index += Keyboard->row_lengths[i];
            }
            for(int col = 0; col < Keyboard->row_lengths[row]; col++)
            {
                if(button_index + col < Keyboard->button_count)
                {
                    row_total_multipliers += Keyboard->button_width_multipliers[button_index + col];
                }
            }
            if(row_total_multipliers > max_total_multipliers)
            {
                max_total_multipliers = row_total_multipliers;
            }
        }
        
        if(max_total_multipliers > 0.0f)
        {
            // Рассчитываем базовую ширину кнопки пропорционально:
            // ширина виджета / максимальная сумма множителей
            // Учитываем отрицательный зазор для перекрытия границ
            // Количество зазоров = количество кнопок - 1 в самой длинной строке
            int max_cols = 0;
            for(int i = 0; i < Keyboard->rows; i++)
            {
                if(Keyboard->row_lengths[i] > max_cols)
                {
                    max_cols = Keyboard->row_lengths[i];
                }
            }
            int total_gap_x = Keyboard->button_gap_x * (max_cols - 1);
            int total_width_needed = Keyboard->lenx - total_gap_x;
            // Используем float для точности, затем округляем
            float button_width_float = (float)total_width_needed / max_total_multipliers;
            Keyboard->button_width = (int)(button_width_float + 0.5f); // округление до ближайшего целого
            // Сохраняем остаток от деления для распределения между кнопками
            Keyboard->width_remainder = total_width_needed - (int)(max_total_multipliers * button_width_float + 0.5f);
            
            // Рассчитываем высоту кнопки пропорционально:
            // высота виджета / количество строк
            // Учитываем отрицательный зазор для перекрытия границ
            int total_gap_y = Keyboard->button_gap_y * (Keyboard->rows - 1);
            int total_height_needed = Keyboard->leny - total_gap_y;
            Keyboard->button_height = total_height_needed / Keyboard->rows;
            // Сохраняем остаток от деления для распределения между строками
            Keyboard->height_remainder = total_height_needed % Keyboard->rows;
            
            // Гарантируем минимальный размер кнопки (хотя бы 1 пиксель)
            if(Keyboard->button_width < 1) Keyboard->button_width = 1;
            if(Keyboard->button_height < 1) Keyboard->button_height = 1;
        }
    }
    
    // Сбрасываем выбранную позицию
    Keyboard->selected_row = 0;
    Keyboard->selected_col = 0;
    Keyboard->selected_index = 0;
}

//*************************************************
void KeyboardMoveNext(_GSP_Keyboard *Keyboard)
{
    if(!Keyboard || !Keyboard->row_lengths) return;
    if(Keyboard->rows == 0) return;
    
    Keyboard->selected_col++;
    if(Keyboard->selected_col >= Keyboard->row_lengths[Keyboard->selected_row])
    {
        // Переходим на следующую строку
        Keyboard->selected_row++;
        if(Keyboard->selected_row >= Keyboard->rows)
        {
            Keyboard->selected_row = 0; // Циклически переходим к первой строке
        }
        Keyboard->selected_col = 0;
    }
    
    // Обновляем общий индекс
    Keyboard->selected_index = GetButtonArrayIndex(Keyboard, Keyboard->selected_row, Keyboard->selected_col);
}

//*************************************************
void KeyboardMovePrev(_GSP_Keyboard *Keyboard)
{
    if(!Keyboard || !Keyboard->row_lengths) return;
    if(Keyboard->rows == 0) return;
    
    Keyboard->selected_col--;
    if(Keyboard->selected_col < 0)
    {
        // Переходим на предыдущую строку
        Keyboard->selected_row--;
        if(Keyboard->selected_row < 0)
        {
            Keyboard->selected_row = Keyboard->rows - 1; // Циклически переходим к последней строке
        }
        Keyboard->selected_col = Keyboard->row_lengths[Keyboard->selected_row] - 1;
    }
    
    // Обновляем общий индекс
    Keyboard->selected_index = GetButtonArrayIndex(Keyboard, Keyboard->selected_row, Keyboard->selected_col);
}

//*************************************************
void KeyboardMoveDown(_GSP_Keyboard *Keyboard)
{
    if(!Keyboard || !Keyboard->row_lengths) return;
    if(Keyboard->rows == 0) return;
    
    Keyboard->selected_row++;
    if(Keyboard->selected_row >= Keyboard->rows)
    {
        Keyboard->selected_row = 0; // Циклически переходим к первой строке
    }
    
    // Проверяем, что выбранная колонка существует в новой строке
    if(Keyboard->selected_col >= Keyboard->row_lengths[Keyboard->selected_row])
    {
        Keyboard->selected_col = Keyboard->row_lengths[Keyboard->selected_row] - 1;
        if(Keyboard->selected_col < 0) Keyboard->selected_col = 0;
    }
    
    // Обновляем общий индекс
    Keyboard->selected_index = GetButtonArrayIndex(Keyboard, Keyboard->selected_row, Keyboard->selected_col);
}

//*************************************************
void KeyboardMoveUp(_GSP_Keyboard *Keyboard)
{
    if(!Keyboard || !Keyboard->row_lengths) return;
    if(Keyboard->rows == 0) return;
    
    Keyboard->selected_row--;
    if(Keyboard->selected_row < 0)
    {
        Keyboard->selected_row = Keyboard->rows - 1; // Циклически переходим к последней строке
    }
    
    // Проверяем, что выбранная колонка существует в новой строке
    if(Keyboard->selected_col >= Keyboard->row_lengths[Keyboard->selected_row])
    {
        Keyboard->selected_col = Keyboard->row_lengths[Keyboard->selected_row] - 1;
        if(Keyboard->selected_col < 0) Keyboard->selected_col = 0;
    }
    
    // Обновляем общий индекс
    Keyboard->selected_index = GetButtonArrayIndex(Keyboard, Keyboard->selected_row, Keyboard->selected_col);
}

//*************************************************
void KeyboardSetSelected(_GSP_Keyboard *Keyboard, int row, int col)
{
    if(!Keyboard || !Keyboard->row_lengths) return;
    if(row < 0 || row >= Keyboard->rows) return;
    if(col < 0 || col >= Keyboard->row_lengths[row]) return;
    
    Keyboard->selected_row = row;
    Keyboard->selected_col = col;
    Keyboard->selected_index = GetButtonArrayIndex(Keyboard, row, col);
}

//*************************************************
int KeyboardGetSelectedIndex(_GSP_Keyboard *Keyboard)
{
    if(!Keyboard) return -1;
    return Keyboard->selected_index;
}

//*************************************************
const char *KeyboardGetSelectedText(_GSP_Keyboard *Keyboard)
{
    if(!Keyboard) return NULL;
    return GetButtonTextByPosition(Keyboard, Keyboard->selected_row, Keyboard->selected_col);
}

//*************************************************
void KeyboardSetFont(_GSP_Keyboard *Keyboard, void *font)
{
    if(!Keyboard) return;
    Keyboard->Font = font;
}

//*************************************************
void KeyboardSetWidgetAlign(_GSP_Keyboard *Keyboard, int align)
{
    if(!Keyboard) return;
    Keyboard->WidgetAlign = align;
    
    // Пересчитываем координаты относительно Active_Screen
    _GSP_Screen *Screen = Get_Active_Screen();
    if(Screen)
    {
        int abs_x = Keyboard->rel_x;
        int abs_y = Keyboard->rel_y;
        CalculateWidgetPosition(Screen, &abs_x, &abs_y, Keyboard->lenx, Keyboard->leny, align);
        Keyboard->x = abs_x;
        Keyboard->y = abs_y;
    }
}

//*************************************************
void KeyboardSetPos(_GSP_Keyboard *Keyboard, int x, int y)
{
    if(!Keyboard) return;
    
    Keyboard->rel_x = x;
    Keyboard->rel_y = y;
    
    // Пересчитываем абсолютные координаты с учетом текущего выравнивания
    _GSP_Screen *Screen = Get_Active_Screen();
    if(Screen)
    {
        int abs_x = x;
        int abs_y = y;
        CalculateWidgetPosition(Screen, &abs_x, &abs_y, Keyboard->lenx, Keyboard->leny, Keyboard->WidgetAlign);
        Keyboard->x = abs_x;
        Keyboard->y = abs_y;
    }
}

//*************************************************
void KeyboardSetTextOffset(_GSP_Keyboard *Keyboard, int offset_y)
{
    if(!Keyboard) return;
    Keyboard->text_offset_y = offset_y;
}

//*************************************************
void KeyboardSetButtonWidthMultiplier(_GSP_Keyboard *Keyboard, int row, int col, float multiplier)
{
    if(!Keyboard || !Keyboard->button_width_multipliers) return;
    // Массив множителей должен быть создан после вызова KeyboardSetButtons
    if(row < 0 || row >= Keyboard->rows) return;
    if(col < 0 || col >= Keyboard->row_lengths[row]) return;
    if(multiplier < 0.1f) multiplier = 0.1f; // минимальный множитель = 0.1
    
    // Вычисляем индекс кнопки в массиве
    int button_index = 0;
    for(int i = 0; i < row; i++)
    {
        button_index += Keyboard->row_lengths[i];
    }
    button_index += col;
    
    if(button_index < Keyboard->button_count)
    {
        Keyboard->button_width_multipliers[button_index] = multiplier;
        
        // Пересчитываем базовую ширину кнопки с учетом новых множителей
        if(Keyboard->rows > 0 && Keyboard->row_lengths)
        {
            // Находим максимальную сумму множителей в строке (самая широкая строка)
            int max_total_multipliers = 0;
            for(int r = 0; r < Keyboard->rows; r++)
            {
                int row_total_multipliers = 0;
                int btn_idx = 0;
                // Подсчитываем сумму множителей для текущей строки
                for(int i = 0; i < r; i++)
                {
                    btn_idx += Keyboard->row_lengths[i];
                }
                for(int c = 0; c < Keyboard->row_lengths[r]; c++)
                {
                    if(btn_idx + c < Keyboard->button_count)
                    {
                        row_total_multipliers += Keyboard->button_width_multipliers[btn_idx + c];
                    }
                }
                if(row_total_multipliers > max_total_multipliers)
                {
                    max_total_multipliers = row_total_multipliers;
                }
            }
            
            if(max_total_multipliers > 0)
            {
                // Рассчитываем базовую ширину кнопки пропорционально:
                // ширина виджета / максимальная сумма множителей
                int max_cols = 0;
                for(int i = 0; i < Keyboard->rows; i++)
                {
                    if(Keyboard->row_lengths[i] > max_cols)
                    {
                        max_cols = Keyboard->row_lengths[i];
                    }
                }
                int total_gap_x = Keyboard->button_gap_x * (max_cols - 1);
                int total_width_needed = Keyboard->lenx - total_gap_x;
                Keyboard->button_width = total_width_needed / max_total_multipliers;
                // Сохраняем остаток от деления для распределения между кнопками
                Keyboard->width_remainder = total_width_needed % max_total_multipliers;
                
                // Гарантируем минимальный размер кнопки
                if(Keyboard->button_width < 1) Keyboard->button_width = 1;
            }
        }
    }
}

//*************************************************
void KeyboardSetSize(_GSP_Keyboard *Keyboard, int lenx, int leny)
{
    if(!Keyboard) return;
    
    Keyboard->lenx = lenx;
    Keyboard->leny = leny;
    
    // Пересчитываем абсолютные координаты с учетом текущего выравнивания
    _GSP_Screen *Screen = Get_Active_Screen();
    if(Screen)
    {
        int abs_x = Keyboard->rel_x;
        int abs_y = Keyboard->rel_y;
        CalculateWidgetPosition(Screen, &abs_x, &abs_y, lenx, leny, Keyboard->WidgetAlign);
        Keyboard->x = abs_x;
        Keyboard->y = abs_y;
    }
    
    // Пересчитываем размеры кнопок пропорционально новому размеру виджета
    if(Keyboard->rows > 0 && Keyboard->row_lengths && Keyboard->button_width_multipliers)
    {
        // Находим максимальную сумму множителей в строке (самая широкая строка)
        float max_total_multipliers = 0.0f;
        for(int row = 0; row < Keyboard->rows; row++)
        {
            float row_total_multipliers = 0.0f;
            int button_index = 0;
            // Подсчитываем сумму множителей для текущей строки
            for(int i = 0; i < row; i++)
            {
                button_index += Keyboard->row_lengths[i];
            }
            for(int col = 0; col < Keyboard->row_lengths[row]; col++)
            {
                if(button_index + col < Keyboard->button_count)
                {
                    row_total_multipliers += Keyboard->button_width_multipliers[button_index + col];
                }
            }
            if(row_total_multipliers > max_total_multipliers)
            {
                max_total_multipliers = row_total_multipliers;
            }
        }
        
        if(max_total_multipliers > 0.0f)
        {
            // Рассчитываем базовую ширину кнопки пропорционально:
            // ширина виджета / максимальная сумма множителей
            // Учитываем отрицательный зазор для перекрытия границ
            int max_cols = 0;
            for(int i = 0; i < Keyboard->rows; i++)
            {
                if(Keyboard->row_lengths[i] > max_cols)
                {
                    max_cols = Keyboard->row_lengths[i];
                }
            }
            int total_gap_x = Keyboard->button_gap_x * (max_cols - 1);
            int total_width_needed = Keyboard->lenx - total_gap_x;
            // Используем float для точности, затем округляем
            float button_width_float = (float)total_width_needed / max_total_multipliers;
            Keyboard->button_width = (int)(button_width_float + 0.5f); // округление до ближайшего целого
            // Сохраняем остаток от деления для распределения между кнопками
            Keyboard->width_remainder = total_width_needed - (int)(max_total_multipliers * button_width_float + 0.5f);
            
            // Рассчитываем высоту кнопки пропорционально:
            // высота виджета / количество строк
            // Учитываем отрицательный зазор для перекрытия границ
            int total_gap_y = Keyboard->button_gap_y * (Keyboard->rows - 1);
            int total_height_needed = Keyboard->leny - total_gap_y;
            Keyboard->button_height = total_height_needed / Keyboard->rows;
            // Сохраняем остаток от деления для распределения между строками
            Keyboard->height_remainder = total_height_needed % Keyboard->rows;
            
            // Гарантируем минимальный размер кнопки (хотя бы 1 пиксель)
            if(Keyboard->button_width < 1) Keyboard->button_width = 1;
            if(Keyboard->button_height < 1) Keyboard->button_height = 1;
        }
    }
}

//*************************************************
void DrawingKeyboard(_GSP_Keyboard *Keyboard)
{
    if(!Keyboard) return;
    if(!Keyboard->Visible) return;
    if(!Keyboard->buttons) return;
    if(Keyboard->rows == 0 || !Keyboard->row_lengths) return;
    
    int lenx = Keyboard->lenx;
    int leny = Keyboard->leny;
    int x1 = Keyboard->x;
    int y1 = Keyboard->y;
    int x2 = Keyboard->x + lenx;
    int y2 = Keyboard->y + leny;
    int a_x = Active_Screen->x;
    int a_len_x = Active_Screen->lenx;
    int a_y = Active_Screen->y;
    int a_len_y = Active_Screen->leny;
    
    if(x2 > a_x + a_len_x) x2 = a_x + a_len_x;
    if(y2 > a_y + a_len_y) y2 = a_y + a_len_y;
    
SetClipRgn(x1, y1, x2, y2);
SetClip(true);
    
    // Рисуем фон виджета
    if(Keyboard->BackColor != TRANSPARENT)
    {
SetColor(Keyboard->BackColor);
FillBevel(x1, y1, x2, y2, 0);
    }
    
    // Парсим массив кнопок и рисуем их
    int current_row = 0;
    int current_col = 0;
    int current_button_index = 0; // индекс текущей кнопки в массиве множителей
    
    int start_y = y1;
    int available_width = Keyboard->lenx;
    
    for(int i = 0; Keyboard->buttons[i] != NULL && Keyboard->buttons[i][0] != '\0'; i++)
    {
        const char *btn_text = Keyboard->buttons[i];
        
        // Проверяем, является ли это символом новой строки
        if(strcmp(btn_text, "\n") == 0)
        {
            // Конец текущей строки - переходим на следующую
            current_row++;
            current_col = 0;
            continue;
        }
        
        // Получаем множитель ширины для текущей кнопки
        float width_multiplier = 1.0f;
        if(Keyboard->button_width_multipliers && current_button_index < Keyboard->button_count)
        {
            width_multiplier = Keyboard->button_width_multipliers[current_button_index];
        }
        
        // Это кнопка - вычисляем смещение для центрирования текущей строки
        int row_offset_x = 0;
        if(current_row < Keyboard->rows && Keyboard->row_lengths && Keyboard->button_width_multipliers)
        {
            int buttons_in_row = Keyboard->row_lengths[current_row];
            if(buttons_in_row > 0)
            {
                // Вычисляем ширину строки с кнопками, учитывая множители и распределение остатков
                int row_width = 0;
                int row_button_index = current_button_index - current_col; // индекс первой кнопки в строке
                for(int col = 0; col < buttons_in_row; col++)
                {
                    float col_multiplier = 1.0f;
                    if(row_button_index + col < Keyboard->button_count)
                    {
                        col_multiplier = Keyboard->button_width_multipliers[row_button_index + col];
                    }
                    int col_width = (int)(Keyboard->button_width * col_multiplier + 0.5f); // округление
                    // Распределяем остаток пропорционально множителям (упрощенная версия)
                    if(col < Keyboard->width_remainder && col_multiplier > 0.0f)
                    {
                        col_width++; // первые кнопки получают дополнительный пиксель
                    }
                    row_width += col_width;
                    if(col < buttons_in_row - 1)
                    {
                        row_width += Keyboard->button_gap_x;
                    }
                }
                // Вычисляем смещение для центрирования
                row_offset_x = (available_width - row_width) / 2;
            }
        }
        
        // Вычисляем позицию кнопки с учетом центрирования строки и множителя
        int btn_width = (int)(Keyboard->button_width * width_multiplier + 0.5f); // округление
        // Распределяем остаток пропорционально (упрощенная версия - первые кнопки получают +1)
        if(current_col < Keyboard->width_remainder && width_multiplier > 0.0f)
        {
            btn_width++;  // первые кнопки получают дополнительный пиксель
        }
        
        // Все ряды имеют одинаковую высоту
        int btn_height = Keyboard->button_height;
        
        // Вычисляем смещение по X с учетом множителей предыдущих кнопок
        int btn_x_offset = 0;
        if(Keyboard->button_width_multipliers)
        {
            int row_start_index = current_button_index - current_col; // индекс первой кнопки в строке
            for(int col = 0; col < current_col; col++)
            {
                float col_multiplier = 1.0f;
                if(row_start_index + col < Keyboard->button_count)
                {
                    col_multiplier = Keyboard->button_width_multipliers[row_start_index + col];
                }
                int col_width = (int)(Keyboard->button_width * col_multiplier + 0.5f); // округление
                if(col < Keyboard->width_remainder && col_multiplier > 0.0f)
                {
                    col_width++; // первые кнопки получают дополнительный пиксель
                }
                btn_x_offset += col_width + Keyboard->button_gap_x;
            }
        }
        
        // Вычисляем смещение по Y (все ряды имеют одинаковую высоту)
        int btn_y_offset = 0;
        for(int i = 0; i < current_row; i++)
        {
            btn_y_offset += Keyboard->button_height + Keyboard->button_gap_y;
        }
        
        int btn_x = x1 + row_offset_x + btn_x_offset;
        int btn_y = start_y + btn_y_offset;
        int btn_x2 = btn_x + btn_width - 1;
        int btn_y2 = btn_y + btn_height - 1;
        
        // Ограничиваем координаты кнопки границами виджета, чтобы граница не обрезалась
        int btn_x_clipped = btn_x;
        int btn_y_clipped = btn_y;
        int btn_x2_clipped = btn_x2;
        int btn_y2_clipped = btn_y2;
        
        if(btn_x < x1) btn_x_clipped = x1;
        if(btn_y < y1) btn_y_clipped = y1;
        if(btn_x2 > x2) btn_x2_clipped = x2;
        if(btn_y2 > y2) btn_y2_clipped = y2;
        
        // Проверяем, является ли эта кнопка выбранной
        bool is_selected = (current_row == Keyboard->selected_row && current_col == Keyboard->selected_col);
        
        // Рисуем фон кнопки (используем ограниченные координаты)
        WORD_COLOR btn_bg_color = is_selected ? Keyboard->SelColor : Keyboard->BackColor;
        if(btn_bg_color != TRANSPARENT)
        {
SetColor(btn_bg_color);
FillBevel(btn_x_clipped, btn_y_clipped, btn_x2_clipped, btn_y2_clipped, 0);
        }
        
        // Рисуем рамку кнопки (1 пиксель) - используем оригинальные координаты для полной границы
        // Временно расширяем область клиппинга, чтобы граница не обрезалась
        int old_x2 = x2;
        int old_y2 = y2;
        if(btn_x2 > x2) x2 = btn_x2;
        if(btn_y2 > y2) y2 = btn_y2;
SetClipRgn(x1, y1, x2, y2);
SetClip(true);
        
SetColor(Keyboard->ButtonBorderColor);
SetLineThickness(0);  // 0 означает толщину 1 пиксель (как WidthBorder-1 при WidthBorder=1)
Bevel(btn_x, btn_y, btn_x2, btn_y2, 0);
        
        // Восстанавливаем область клиппинга
SetClipRgn(x1, y1, old_x2, old_y2);
SetClip(true);
        
        // Рисуем текст кнопки
        WORD_COLOR text_color = is_selected ? 
            ((Keyboard->SelColor == BLACK) ? WHITE : BLACK) : Keyboard->TextColor;
        
        // Вычисляем позицию текста (центрируем)
        // Используем реальные размеры кнопки (btn_width и btn_height), а не базовые
        int text_width = GetTextWidthUTF8(Keyboard->Font, (char*)btn_text, 0);
        int text_height = GetTextHightUTF8(Keyboard->Font);
        int text_x = btn_x + (btn_width - text_width) / 2;
        // Вычисляем позицию текста (центрируем) с учетом смещения
        int text_y = btn_y + (btn_height - text_height) / 2 + Keyboard->text_offset_y;
        
        // Ограничиваем область отрисовки текста границами кнопки
SetClipRgn(btn_x, btn_y, btn_x2, btn_y2);
SetClip(true);
        DrawTextUTF8(text_x, text_y, Keyboard->Font, (char*)btn_text, text_color, 0, 0);
SetClip(false);
        
        current_col++;
        current_button_index++; // увеличиваем индекс кнопки в массиве множителей
    }
    
SetClip(false);
}

#endif // USED_KEYBOARD

