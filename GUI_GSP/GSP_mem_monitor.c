#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "GSP_mem_monitor.h"

// Если USE_MEM_MONITOR отключен, компилируем только заглушки
#if !USE_MEM_MONITOR
// Мониторинг отключен - ничего не компилируем
#else

// Отменяем макросы переопределения для вызова реальных функций
#undef malloc
#undef free
#undef realloc
#undef calloc

// ============================================================================
// КОНФИГУРАЦИЯ ХЕШ-ТАБЛИЦЫ
// ============================================================================
#define HASH_SIZE 64  // Должно быть степенью 2 для быстрого хеширования
#define MAX_CHAIN_LENGTH 4  // Максимальная длина цепочки коллизий

// ============================================================================
// СТРУКТУРЫ ДАННЫХ
// ============================================================================

typedef struct MemBlock
{
	void *ptr;
	size_t size;
	uint32_t timestamp;  // Для обнаружения "старых" блоков (утечек)
	
#if USE_DEBUG_ALLOC_INFO
	const char* file;
	int line;
#endif
	
	struct MemBlock *next;  // Для цепочки коллизий
} MemBlock;

// Статистические счетчики
typedef struct
{
	size_t current_usage;      // Текущее использование памяти
	size_t peak_usage;         // Пиковое использование
	size_t total_allocated;    // Всего выделено
	size_t total_freed;        // Всего освобождено
	
	uint32_t malloc_count;     // Количество malloc
	uint32_t calloc_count;     // Количество calloc
	uint32_t realloc_count;    // Количество realloc
	uint32_t free_count;       // Количество free
	
	uint32_t failed_malloc;    // Неудачные malloc
	uint32_t failed_calloc;    // Неудачные calloc
	uint32_t failed_realloc;   // Неудачные realloc
	uint32_t double_free;      // Попытки double free
	uint32_t invalid_free;     // Освобождение неизвестных указателей
	
	uint32_t tracked_blocks;   // Количество отслеживаемых блоков
	uint32_t untracked_allocs; // Выделения, не попавшие в таблицу (переполнение)
} MemStats;

// ============================================================================
// ГЛОБАЛЬНЫЕ ПЕРЕМЕННЫЕ
// ============================================================================

static MemBlock* hashTable[HASH_SIZE];  // Хеш-таблица
static MemStats stats = {0};            // Статистика
static uint32_t system_ticks = 0;       // Счетчик времени (должен обновляться извне)

// Пул блоков для хранения информации (избегаем malloc внутри malloc)
#define BLOCK_POOL_SIZE 64
static MemBlock blockPool[BLOCK_POOL_SIZE];
static uint16_t poolUsed = 0;

// ============================================================================
// ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ
// ============================================================================

// Функция хеширования указателя
static inline uint32_t hash_ptr(void *ptr)
{
	// Используем старшие биты адреса (младшие обычно 0 из-за выравнивания)
	uintptr_t addr = (uintptr_t)ptr;
	return (addr >> 4) & (HASH_SIZE - 1);
}

// Получить блок из пула
static MemBlock* alloc_block(void)
{
	if (poolUsed >= BLOCK_POOL_SIZE)
	{
		return NULL;  // Пул переполнен
	}
	return &blockPool[poolUsed++];
}

// Вернуть блок в пул (простая реализация - просто уменьшаем счетчик)
static void free_block(MemBlock* block)
{
	// Помечаем блок как свободный
	block->ptr = NULL;
	
	// Если это последний блок в пуле, уменьшаем счетчик
	if (block == &blockPool[poolUsed - 1])
	{
		poolUsed--;
		
		// Продолжаем уменьшать, пока находим освобожденные блоки
		while (poolUsed > 0 && blockPool[poolUsed - 1].ptr == NULL)
		{
			poolUsed--;
		}
	}
}

// Добавить блок в хеш-таблицу
static int hash_add(void *ptr, size_t size)
{
	if (ptr == NULL) return 0;
	
	uint32_t hash = hash_ptr(ptr);
	
	// Проверяем длину цепочки
	int chain_len = 0;
	MemBlock* current = hashTable[hash];
	while (current != NULL)
	{
		chain_len++;
		if (chain_len >= MAX_CHAIN_LENGTH)
		{
			// Цепочка слишком длинная - возможно, нужно увеличить HASH_SIZE
			return 0;
		}
		current = current->next;
	}
	
	// Выделяем блок из пула
	MemBlock* block = alloc_block();
	if (block == NULL)
	{
		return 0;  // Пул переполнен
	}
	
	// Заполняем информацию
	block->ptr = ptr;
	block->size = size;
	block->timestamp = system_ticks;
	block->next = hashTable[hash];
	
#if USE_DEBUG_ALLOC_INFO
	block->file = NULL;
	block->line = 0;
#endif
	
	// Добавляем в начало цепочки
	hashTable[hash] = block;
	stats.tracked_blocks++;
	
	return 1;
}

// Найти блок в хеш-таблице
static MemBlock* hash_find(void *ptr)
{
	if (ptr == NULL) return NULL;
	
	uint32_t hash = hash_ptr(ptr);
	MemBlock* current = hashTable[hash];
	
	while (current != NULL)
	{
		if (current->ptr == ptr)
		{
			return current;
		}
		current = current->next;
	}
	
	return NULL;
}

// Удалить блок из хеш-таблицы
static size_t hash_remove(void *ptr)
{
	if (ptr == NULL) return 0;
	
	uint32_t hash = hash_ptr(ptr);
	MemBlock* current = hashTable[hash];
	MemBlock* prev = NULL;
	
	while (current != NULL)
	{
		if (current->ptr == ptr)
		{
			size_t size = current->size;
			
			// Удаляем из цепочки
			if (prev == NULL)
			{
				hashTable[hash] = current->next;
			}
			else
			{
				prev->next = current->next;
			}
			
			// Освобождаем блок
			free_block(current);
			stats.tracked_blocks--;
			
			return size;
		}
		prev = current;
		current = current->next;
	}
	
	return 0;  // Блок не найден
}

// Обновить адрес блока (для realloc)
static int hash_update(void *old_ptr, void *new_ptr, size_t new_size)
{
	if (old_ptr == NULL || new_ptr == NULL) return 0;
	
	// Если адрес не изменился
	if (old_ptr == new_ptr)
	{
		MemBlock* block = hash_find(old_ptr);
		if (block != NULL)
		{
			block->size = new_size;
			block->timestamp = system_ticks;
			return 1;
		}
		return 0;
	}
	
	// Адрес изменился - удаляем старый и добавляем новый
	size_t old_size = hash_remove(old_ptr);
	if (old_size > 0)
	{
		return hash_add(new_ptr, new_size);
	}
	
	return 0;
}

// Функция для обновления системного времени (должна вызываться периодически)
void mm_tick(void)
{
	system_ticks++;
}

// ============================================================================
// ОСНОВНЫЕ ФУНКЦИИ МОНИТОРИНГА
// ============================================================================

#if USE_DEBUG_ALLOC_INFO
void* user_malloc_debug(size_t size, const char* file, int line)
#else
void* user_malloc(size_t size)
#endif
{
	// Вызываем реальный malloc
	void *p = malloc(size);
	
	if (p != NULL)
	{
		// Пытаемся добавить в таблицу отслеживания
		if (hash_add(p, size))
		{
			// Успешно добавлен в таблицу - обновляем счетчики
			stats.current_usage += size;
			stats.total_allocated += size;
			stats.malloc_count++;
			
#if USE_DEBUG_ALLOC_INFO
			MemBlock* block = hash_find(p);
			if (block != NULL)
			{
				block->file = file;
				block->line = line;
			}
#endif
			
			if (stats.current_usage > stats.peak_usage)
			{
				stats.peak_usage = stats.current_usage;
			}
		}
		else
		{
			// Не удалось добавить в таблицу (переполнение)
			stats.untracked_allocs++;
			
#if USE_ERR_MEM_MONITOR
			printf("MEM WARNING: malloc(%u) table full, ptr %p untracked\n",
			       (unsigned int)size, p);
#endif
			
			// ВАЖНО: Всё равно обновляем счетчики для корректности
			stats.current_usage += size;
			stats.total_allocated += size;
			stats.malloc_count++;
			
			if (stats.current_usage > stats.peak_usage)
			{
				stats.peak_usage = stats.current_usage;
			}
		}
	}
	else
	{
		stats.failed_malloc++;
	}
	
	return p;
}

#if USE_DEBUG_ALLOC_INFO
void* user_calloc_debug(size_t nmemb, size_t size, const char* file, int line)
#else
void* user_calloc(size_t nmemb, size_t size)
#endif
{
	// Проверка переполнения при умножении
	if (nmemb != 0 && size > SIZE_MAX / nmemb)
	{
		stats.failed_calloc++;
		return NULL;
	}
	
	size_t total_size = nmemb * size;
	
	// Вызываем реальный calloc
	void *p = calloc(nmemb, size);
	
	if (p != NULL)
	{
		// Пытаемся добавить в таблицу отслеживания
		if (hash_add(p, total_size))
		{
			stats.current_usage += total_size;
			stats.total_allocated += total_size;
			stats.calloc_count++;
			
#if USE_DEBUG_ALLOC_INFO
			MemBlock* block = hash_find(p);
			if (block != NULL)
			{
				block->file = file;
				block->line = line;
			}
#endif
			
			if (stats.current_usage > stats.peak_usage)
			{
				stats.peak_usage = stats.current_usage;
			}
		}
		else
		{
			stats.untracked_allocs++;
			
#if USE_ERR_MEM_MONITOR
			printf("MEM WARNING: calloc(%u,%u) table full, ptr %p untracked\n",
			       (unsigned int)nmemb, (unsigned int)size, p);
#endif
			
			// Всё равно обновляем счетчики
			stats.current_usage += total_size;
			stats.total_allocated += total_size;
			stats.calloc_count++;
			
			if (stats.current_usage > stats.peak_usage)
			{
				stats.peak_usage = stats.current_usage;
			}
		}
	}
	else
	{
		stats.failed_calloc++;
	}
	
	return p;
}

#if USE_DEBUG_ALLOC_INFO
void* user_realloc_debug(void *ptr, size_t size, const char* file, int line)
#else
void* user_realloc(void *ptr, size_t size)
#endif
{
	// Случай realloc(NULL, size) эквивалентен malloc(size)
	if (ptr == NULL)
	{
#if USE_DEBUG_ALLOC_INFO
		return user_malloc_debug(size, file, line);
#else
		return user_malloc(size);
#endif
	}
	
	// Случай realloc(ptr, 0) эквивалентен free(ptr)
	if (size == 0)
	{
#if USE_DEBUG_ALLOC_INFO
		user_free_debug(ptr, file, line);
#else
		user_free(ptr);
#endif
		return NULL;
	}
	
	// Находим информацию о старом блоке
	MemBlock* old_block = hash_find(ptr);
	size_t old_size = (old_block != NULL) ? old_block->size : 0;
	int was_tracked = (old_block != NULL);
	
	// Вызываем реальный realloc
	void *p = realloc(ptr, size);
	
	if (p != NULL)
	{
		// realloc успешен
		stats.realloc_count++;
		
		if (was_tracked)
		{
			// Старый блок был отслеживаем
			if (p != ptr)
			{
				// Адрес изменился
				hash_remove(ptr);
				if (hash_add(p, size))
				{
					// Успешно добавлен новый блок
#if USE_DEBUG_ALLOC_INFO
					MemBlock* block = hash_find(p);
					if (block != NULL)
					{
						block->file = file;
						block->line = line;
					}
#endif
				}
				else
				{
					stats.untracked_allocs++;
#if USE_ERR_MEM_MONITOR
					printf("MEM WARNING: realloc() new ptr %p untracked\n", p);
#endif
				}
			}
			else
			{
				// Адрес не изменился - просто обновляем размер
				old_block->size = size;
				old_block->timestamp = system_ticks;
#if USE_DEBUG_ALLOC_INFO
				old_block->file = file;
				old_block->line = line;
#endif
			}
			
			// Обновляем счетчики
			stats.current_usage = stats.current_usage - old_size + size;
			stats.total_allocated += size;
			stats.total_freed += old_size;
			
			if (stats.current_usage > stats.peak_usage)
			{
				stats.peak_usage = stats.current_usage;
			}
		}
		else
		{
			// Старый блок НЕ был отслеживаем (неотслеживаемое выделение)
			// Считаем это как новое выделение
			if (hash_add(p, size))
			{
#if USE_DEBUG_ALLOC_INFO
				MemBlock* block = hash_find(p);
				if (block != NULL)
				{
					block->file = file;
					block->line = line;
				}
#endif
			}
			else
			{
				stats.untracked_allocs++;
			}
			
			// Обновляем счетчики (старый размер неизвестен, считаем только новый)
			stats.current_usage = stats.current_usage - old_size + size;
			stats.total_allocated += size;
			
			if (stats.current_usage > stats.peak_usage)
			{
				stats.peak_usage = stats.current_usage;
			}
		}
	}
	else
	{
		// realloc вернул NULL - старый блок остается нетронутым
		stats.failed_realloc++;
	}
	
	return p;
}

#if USE_DEBUG_ALLOC_INFO
void user_free_debug(void *ptr, const char* file, int line)
#else
void user_free(void *ptr)
#endif
{
	if (ptr == NULL) return;
	
	// Ищем блок в таблице
	size_t size = hash_remove(ptr);
	
	if (size > 0)
	{
		// Блок найден и удален из таблицы
		stats.current_usage -= size;
		stats.total_freed += size;
		stats.free_count++;
		
		// Освобождаем память
		free(ptr);
	}
	else
	{
		// Блок не найден в таблице
		// Это может быть:
		// 1. Double free (уже освобожденный блок)
		// 2. Неотслеживаемое выделение (переполнение таблицы)
		// 3. Неверный указатель
		
		// Проверяем, не пытаемся ли мы освободить NULL
		// (хотя это проверено выше, на всякий случай)
		
		// Пытаемся освободить память (если это double free, система должна обработать)
		// ВНИМАНИЕ: double free - это UB, но мы пытаемся обнаружить это
		
		// Простая эвристика: проверим, есть ли этот указатель в пуле освобожденных блоков
		int found_freed = 0;
		for (int i = 0; i < BLOCK_POOL_SIZE; i++)
		{
			// Если нашли блок с NULL ptr, который раньше был ptr
			// (это сложно проверить без дополнительной структуры данных)
		}
		
		// Для упрощения считаем это invalid free
		stats.invalid_free++;
		
#if USE_ERR_MEM_MONITOR
#if USE_DEBUG_ALLOC_INFO
		printf("MEM ERROR: free() untracked ptr %p at %s:%d\n", ptr, file, line);
#else
		printf("MEM ERROR: free() untracked ptr %p (double free or invalid ptr)\n", ptr);
#endif
#endif
		
		// Всё равно вызываем free (может быть неотслеживаемое выделение)
		// ОСТОРОЖНО: если это double free, будет crash
		free(ptr);
	}
}

// ============================================================================
// ФУНКЦИИ СТАТИСТИКИ И ДИАГНОСТИКИ
// ============================================================================

void mm_print_stat(void)
{
	printf("\n========== MEMORY STATISTICS ==========\n");
	printf("Memory Usage:\n");
	printf("  Current:       %7u bytes\n", (unsigned int)stats.current_usage);
	printf("  Peak:          %7u bytes\n", (unsigned int)stats.peak_usage);
	printf("  Total alloc:   %7u bytes\n", (unsigned int)stats.total_allocated);
	printf("  Total freed:   %7u bytes\n", (unsigned int)stats.total_freed);
	
	printf("\nOperation Counts:\n");
	printf("  malloc():      %7u (failed: %u)\n", stats.malloc_count, stats.failed_malloc);
	printf("  calloc():      %7u (failed: %u)\n", stats.calloc_count, stats.failed_calloc);
	printf("  realloc():     %7u (failed: %u)\n", stats.realloc_count, stats.failed_realloc);
	printf("  free():        %7u\n", stats.free_count);
	
	printf("\nTracking Info:\n");
	printf("  Tracked blocks: %u / %u (%.1f%%)\n",
	       stats.tracked_blocks,
	       BLOCK_POOL_SIZE,
	       (stats.tracked_blocks * 100.0f) / BLOCK_POOL_SIZE);
	printf("  Untracked allocs: %u\n", stats.untracked_allocs);
	
	if (stats.invalid_free > 0 || stats.double_free > 0)
	{
		printf("\nERRORS:\n");
		printf("  Invalid free:  %u\n", stats.invalid_free);
		printf("  Double free:   %u\n", stats.double_free);
	}
	
	printf("=======================================\n");
}

void mm_print_leaks(void)
{
	printf("\n========== ACTIVE MEMORY BLOCKS ==========\n");
	
	if (stats.tracked_blocks == 0)
	{
		printf("No active blocks (no leaks detected).\n");
		printf("==========================================\n");
		return;
	}
	
	printf("Total: %u blocks, %u bytes\n\n",
	       stats.tracked_blocks,
	       (unsigned int)stats.current_usage);
	
	int count = 0;
	for (int hash = 0; hash < HASH_SIZE; hash++)
	{
		MemBlock* current = hashTable[hash];
		while (current != NULL)
		{
			uint32_t age = system_ticks - current->timestamp;
			
			printf("  [%3d] Addr: 0x%08X, Size: %6u bytes, Age: %u ticks",
			       count++,
			       (unsigned int)current->ptr,
			       (unsigned int)current->size,
			       age);
			
#if USE_DEBUG_ALLOC_INFO
			if (current->file != NULL)
			{
				printf(", %s:%d", current->file, current->line);
			}
#endif
			
			if (age > LEAK_SUSPECT_TICKS)
			{
				printf(" [LEAK SUSPECT]");
			}
			
			printf("\n");
			
			current = current->next;
		}
	}
	
	printf("==========================================\n");
}

void mm_print_detailed_stat(void)
{
	printf("\n========== DETAILED MEMORY STATISTICS ==========\n");
	
	mm_print_stat();
	
	printf("\nHash Table Statistics:\n");
	
	int used_buckets = 0;
	int max_chain = 0;
	int total_chain = 0;
	
	for (int i = 0; i < HASH_SIZE; i++)
	{
		int chain_len = 0;
		MemBlock* current = hashTable[i];
		
		while (current != NULL)
		{
			chain_len++;
			current = current->next;
		}
		
		if (chain_len > 0)
		{
			used_buckets++;
			total_chain += chain_len;
			if (chain_len > max_chain)
			{
				max_chain = chain_len;
			}
		}
	}
	
	printf("  Hash buckets:  %d / %d used (%.1f%%)\n",
	       used_buckets, HASH_SIZE,
	       (used_buckets * 100.0f) / HASH_SIZE);
	printf("  Max chain:     %d\n", max_chain);
	printf("  Avg chain:     %.2f\n",
	       used_buckets > 0 ? (float)total_chain / used_buckets : 0.0f);
	
	if (max_chain > MAX_CHAIN_LENGTH)
	{
		printf("  WARNING: Chain length exceeds MAX_CHAIN_LENGTH!\n");
	}
	
	printf("\nMemory Pool:\n");
	printf("  Pool used:     %u / %u (%.1f%%)\n",
	       poolUsed, BLOCK_POOL_SIZE,
	       (poolUsed * 100.0f) / BLOCK_POOL_SIZE);
	
	printf("================================================\n");
}

void mm_periodic_check(void)
{
	// Проверка критических уровней памяти
	if (stats.current_usage >= MEM_CRITICAL_THRESHOLD)
	{
		printf("CRITICAL: Memory usage %u bytes!\n", (unsigned int)stats.current_usage);
		mm_print_leaks();
	}
	else if (stats.current_usage >= MEM_WARNING_THRESHOLD)
	{
		printf("WARNING: High memory usage %u bytes\n", (unsigned int)stats.current_usage);
	}
	
	// Проверка заполнения пула
	if (poolUsed >= BLOCK_POOL_SIZE * 0.9f)
	{
		printf("WARNING: Block pool almost full (%u/%u)\n", poolUsed, BLOCK_POOL_SIZE);
	}
	
	// Проверка баланса выделения/освобождения
	uint32_t total_allocs = stats.malloc_count + stats.calloc_count;
	int32_t balance = total_allocs - stats.free_count;
	
	if (balance > 50)  // Порог - настраиваемый параметр
	{
		printf("WARNING: Alloc/free imbalance: %u allocs, %u frees (diff: %d)\n",
		       total_allocs, stats.free_count, balance);
	}
	
	// Проверка ошибок
	if (stats.failed_malloc > 0 || stats.failed_calloc > 0 || stats.failed_realloc > 0)
	{
		printf("ERROR: Failed allocations detected! (m:%u c:%u r:%u)\n",
		       stats.failed_malloc, stats.failed_calloc, stats.failed_realloc);
	}
	
	if (stats.invalid_free > 0 || stats.double_free > 0)
	{
		printf("ERROR: Invalid free operations! (invalid:%u double:%u)\n",
		       stats.invalid_free, stats.double_free);
	}
}

int mm_verify_integrity(void)
{
	// Проверка целостности: считаем размер всех отслеживаемых блоков
	size_t calculated_size = 0;
	int calculated_blocks = 0;
	
	for (int hash = 0; hash < HASH_SIZE; hash++)
	{
		MemBlock* current = hashTable[hash];
		while (current != NULL)
		{
			calculated_size += current->size;
			calculated_blocks++;
			current = current->next;
		}
	}
	
	int integrity_ok = 1;
	
	if (calculated_blocks != stats.tracked_blocks)
	{
		printf("INTEGRITY ERROR: Block count mismatch! Calculated: %d, Stats: %u\n",
		       calculated_blocks, stats.tracked_blocks);
		integrity_ok = 0;
	}
	
	if (calculated_size != stats.current_usage)
	{
		printf("INTEGRITY ERROR: Size mismatch! Calculated: %u, Stats: %u\n",
		       (unsigned int)calculated_size, (unsigned int)stats.current_usage);
		integrity_ok = 0;
	}
	
	return integrity_ok;
}

void mm_reset_stats(void)
{
	// Очищаем хеш-таблицу
	for (int i = 0; i < HASH_SIZE; i++)
	{
		hashTable[i] = NULL;
	}
	
	// Очищаем пул блоков
	memset(blockPool, 0, sizeof(blockPool));
	poolUsed = 0;
	
	// Сбрасываем статистику
	memset(&stats, 0, sizeof(stats));
	
	printf("Memory monitor statistics reset.\n");
}

// Функции получения статистики
size_t mm_get_current_usage(void)
{
	return stats.current_usage;
}

size_t mm_get_peak_usage(void)
{
	return stats.peak_usage;
}

size_t mm_get_total_allocated(void)
{
	return stats.total_allocated;
}

int mm_get_active_blocks(void)
{
	return stats.tracked_blocks;
}

#endif // USE_MEM_MONITOR
