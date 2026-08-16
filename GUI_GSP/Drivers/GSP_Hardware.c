/**
 * @file GSP_Hardware.c
 * @brief Одна структура GSP_Hardware: все обращения к железу через поля GSP_HW.
 *        Рисование — только через GSP_Driver; дисплей — Refresh(GSP буфер), Init(LCD).
 */
#include "GSP_Driver.h"
#include "../../Graphics/ERC12864.h"
#include "../Config/GUI_GSP_Config.h"
#include "../../Flash.h"
#include "../../Main_SPI.h"
#include "../../Keyboard.h"
#include <stdlib.h>

#if USE_MEM_MONITOR
#include "../GSP_mem_monitor.h"
#endif


//*****************************************************
GSP_Hardware GSP_HW = 
{
	.display =
	{
		.Refresh = (void (*)(void))GSP_Display_Refresh,
		.Init = (void (*)(void))Init_LCD
	},
	.stream_reader =
	{
		.start = Start_Stream_Flash,
		.read_byte = Read_Stream_Flash,
		.stop = Stop_Stream_Flash
	},
	.keyboard =
	{
		.key_poll = KeyboardPoll,
		.read_key = Read_key,
	},
	.memory_read = (GSP_MemoryReadFn)ExternalMemoryCallback,
#if USE_MEM_MONITOR
	.alloc   = user_malloc,
	.free    = user_free,
#else
	.alloc   = malloc,
	.free    = free,
#endif
};
