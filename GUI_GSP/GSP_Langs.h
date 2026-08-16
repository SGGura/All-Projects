/**
 * @file GSP_Langs.h
 * @brief Объявления типов, констант и функций работы с языковыми пакетами.
 *        Состояние (Head_Lang_Pack, Ext_Fonts, AllLangFont) — в GSP_Langs.c. Ключи NAMES_ — в проекте (Langs.c).
 */
#ifndef GSP_LANGS_H
#define GSP_LANGS_H

#include <stdbool.h>
#include "GUI_GSP.h"

#define MAX_NUMB_LANG       10                  // максимальное число языков
#define MAX_FONTS           5                  // Максимальное число применяемых шрифтов
#define MAX_LEN_NAME_FONT   10                  // Максимальная длинна имени шрифта
#define VER_LAGN_PACK       100                 // Версия пакета   

typedef struct
{
	int N;                  // число значений
	char *name_font;		// имя шпифта 
	lv_font_t *font;		// указатель на шрифт
	char **name;           // указатель на строки 
    bool valid;             // зкшянак досоверности
    bool name_valid;       // признак достовености только имени
}NAME_NEW_;

typedef struct
{
    unsigned int Adr_JSON;                                          // адрес JSON
    unsigned int Size_JSON;                                         // длинна JSON
    unsigned int Adr_Icon_Lang;                                     // Адрес иконкя языка
    unsigned char N_Fonts;                                                // число шрифтов языка
    unsigned char List_Name_Font[MAX_FONTS][MAX_LEN_NAME_FONT];     // Строки назнаний шрифтов
    unsigned int Adr_Fonts[MAX_FONTS];                              // Адреса шрифтов
} __attribute__((packed)) HEAD_JSON_;

// Структура заголовка общего языкового файла
typedef struct
{
    unsigned int Ver;                        // номер версии
    unsigned char N_Langs;                        // число пакетов зыков
    unsigned int Adr_Font_Name_Langs;      // Адреса шрифта для всех названий языков
    HEAD_JSON_ LangPack[MAX_NUMB_LANG];
} __attribute__((packed)) HEAD_LANG_PACK_;

#define FIELD_NAME(f) (sizeof(#f)
// Структура текстовых сообщений
typedef const struct  
{
    char *key_name;                 // Имя структуры для поиска в JSON
} __attribute__((packed)) KEY_NAME_;

// Новая Структура текстовых сообщений в формате JSON
// Структура заголовка общего языкового пакета
typedef const struct  
{
    unsigned char N;                    // число языков
    char Ver[20];                       // версия пакета
    unsigned int Pack[256];             // Адреса начал каждого пакета
} __attribute__((packed)) LANGS_FILE_;

// Структура заголовка языкового пакета
typedef const struct  
{
    char Name[20];                          // имя пакета
    unsigned int Fonts[MAX_FONTS];          // Адреса начала файлов шрифтов
} __attribute__((packed)) LANG_PACK_;

/* Состояние языкового пакета (хранится в библиотеке) */
extern HEAD_LANG_PACK_ Head_Lang_Pack;
extern lv_font_t *Ext_Fonts[MAX_FONTS];
extern lv_font_t *AllLangFont;

int Init_Langs(void *pack_lang, int Lang);
int Load_Langs(int Lang);
void Search_String_Font(int Lang, char *key, NAME_NEW_ *name_new);
char *Get_String_From_LangPack(int Lang, KEY_NAME_ *Key, int n);
lv_font_t *Set_Text_Font_Label(int Lang, _GSP_Label *Label, char *NameText, int n);
int Get_Count_Strings_From_LangPack(int Lang, KEY_NAME_ *Key);

#endif /* GSP_LANGS_H */
