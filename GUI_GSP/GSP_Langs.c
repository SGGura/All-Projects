/**
 * @file GSP_Langs.c
 * @brief Реализация работы с языковыми пакетами (функции).
 *        Состояние (Head_Lang_Pack, Ext_Fonts, AllLangFont) хранится здесь.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "GSP_Langs.h"
#include "GSP_JSON.h"
#include "Fonts/LVGL/GSP_lv_font_loader.h"

HEAD_LANG_PACK_ Head_Lang_Pack;
lv_font_t *Ext_Fonts[MAX_FONTS] = {NULL};
lv_font_t *AllLangFont = NULL;

//********************************************************************
int Init_Langs(void *pack_lang, int Lang)
{
	int error = -1;
	if (!pack_lang) return error;
	const IMAGE_EXTERNAL *p = (const IMAGE_EXTERNAL *)pack_lang;
	// Загрузить заголовок языкового пакета
	ExternalMemoryCallback((void *)pack_lang, 0, sizeof(HEAD_LANG_PACK_), &Head_Lang_Pack);
	if(Head_Lang_Pack.Ver == VER_LAGN_PACK)
	{
		// скоректировать адреса относительно начала FLASH
		for(int i = 0; i< Head_Lang_Pack.N_Langs;++i)
		{
			Head_Lang_Pack.LangPack[i].Adr_JSON += p->address;
			Head_Lang_Pack.LangPack[i].Adr_Icon_Lang += p->address;
			for(int j = 0;j<Head_Lang_Pack.LangPack[i].N_Fonts;++j)
			{
				Head_Lang_Pack.LangPack[i].Adr_Fonts[j] += p->address;
			}
		}
		Head_Lang_Pack.Adr_Font_Name_Langs += p->address;
		error = 1;
	}
	printf("Init langs..........");
	if(error < 0) {printf("ERROR version!\r\n");}
	else	      
	{
		printf("OK\r\n"); 
		Load_Langs(Lang);
	} 
	return(error);
}
//*******************************************************************
int Load_Langs(int Lang)
{
	int lang = Lang;
	// очистить загруженные фонты
	for(int i = 0;i<MAX_FONTS;++i)
	{
		if(Ext_Fonts[i]) Ext_Fonts[i] =  NULL;
	}
	if(AllLangFont) lv_font_free(AllLangFont);
	// Загрузить шрифты
	memset(Ext_Fonts, 0, sizeof(Ext_Fonts));
	AllLangFont = NULL;
	EXTDATA tmp;
	tmp.type = (EXTERNAL | IMAGE_JPEG | COMP_NONE);
	for(int i = 0;i<Head_Lang_Pack.LangPack[lang].N_Fonts;++i)
	{
		tmp.address = Head_Lang_Pack.LangPack[lang].Adr_Fonts[i];
									//printf("Adr font: %08X\r\n", tmp.address);
		Ext_Fonts[i] = lv_font_load((void *)&tmp);
	}
	tmp.address = Head_Lang_Pack.Adr_Font_Name_Langs;
	AllLangFont = lv_font_load((void *)&tmp);
	
	printf("Load langs................OK\r\n");
	return(0);
}
//********************************************************************
//процедура поиска значения ключа и шрифта 
char *Get_String_From_LangPack(int Lang, KEY_NAME_ *Key, int n)
{
	static char *str = NULL; 
	static NAME_NEW_ name = {0};	
	str = NULL;
	Search_String_Font(Lang, Key->key_name, &name);
	if(name.valid)
	{
		str = name.name[n];
	}
	
	return(str);
}
//************************************************************
//процедура поиска значения ключа и шрифта 
void Search_String_Font(int Lang, char *key, NAME_NEW_ *name_new)
{
	if(Lang < 0 || Lang >= Head_Lang_Pack.N_Langs) return;
	GSP_JSON_Parse(Head_Lang_Pack.LangPack[Lang].Adr_JSON,
	               Head_Lang_Pack.LangPack[Lang].Size_JSON, key);
	
	if(GSP_JSON_Result.count >= 1)
	{
		name_new->N = GSP_JSON_Result.count-1;
		name_new->name = (char **)GSP_JSON_Result.values;
		name_new->name_font = (char *)GSP_JSON_Result.values[GSP_JSON_Result.count-1];
		name_new->name_valid = true;				// действительные только имена
									//printf("%s\r\n", name_new->name_font);
		
		// найти шрифт
		if(strcmp(name_new->name_font, "AllLangFont") == 0) 
		{
			name_new->valid = true;
			name_new->font = AllLangFont;
		}
		else
		{
			int n;
		// Нати соответсвие имени фотта и его номера
			for(n = 0;n<Head_Lang_Pack.LangPack[Lang].N_Fonts;++n)
			{
				if(name_new->name_font != NULL)
				{
					if(strcmp(name_new->name_font, &Head_Lang_Pack.LangPack[Lang].List_Name_Font[n][0]) == 0) break;
				}
			}

			if(n < Head_Lang_Pack.LangPack[Lang].N_Fonts)
			{
				name_new->valid = true;
				name_new->font = Ext_Fonts[n];
			}
			else name_new->valid = false; // Если шрифта нет
		}
	}
}
//********************************************************
// поиск в JSON текста сообщения заданного его именем 
lv_font_t *Set_Text_Font_Label(int Lang, _GSP_Label *Label, char *NameText, int n)
{
	NAME_NEW_ name = {0};	
	Search_String_Font(Lang, NameText, &name);
	if(name.valid && (Label != NULL))
	{
		LabelSetText(Label,  name.name[n]);
		LabelSetFont(Label, name.font);
	}
	if(name.font == NULL) name.valid = false; 
	return(name.font);
}
//************************************************************
// Процедура подсчета числа строк в ключе JSON заданным NAMES_*Key
int Get_Count_Strings_From_LangPack(int Lang, KEY_NAME_ *Key)
{
	int count = 0;
	
	if(Key && Key->key_name && Lang >= 0 && Lang < Head_Lang_Pack.N_Langs)
	{
		GSP_JSON_Parse(Head_Lang_Pack.LangPack[Lang].Adr_JSON,
		               Head_Lang_Pack.LangPack[Lang].Size_JSON, Key->key_name);
		if(GSP_JSON_Result.count > 0)
			count = GSP_JSON_Result.count - 1;
	}
	
	return(count);
}
