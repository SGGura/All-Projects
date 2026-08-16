#ifndef GSP_MEM_MONITOR_H
#define GSP_MEM_MONITOR_H

#include <stddef.h>
#include <stdint.h>

/* Флаг для включения/выключения мониторинга памяти */
#ifndef USE_MEM_MONITOR
#define USE_MEM_MONITOR 0
#endif

#ifndef USE_ERR_MEM_MONITOR
#define USE_ERR_MEM_MONITOR 0
#endif

#ifndef USE_DEBUG_ALLOC_INFO
#define USE_DEBUG_ALLOC_INFO 0
#endif

#ifndef MEM_WARNING_THRESHOLD
#define MEM_WARNING_THRESHOLD (8 * 1024)
#endif

#ifndef MEM_CRITICAL_THRESHOLD
#define MEM_CRITICAL_THRESHOLD (12 * 1024)
#endif

#ifndef LEAK_SUSPECT_TICKS
#define LEAK_SUSPECT_TICKS 60000
#endif

#if USE_MEM_MONITOR

#if USE_DEBUG_ALLOC_INFO
void* user_malloc_debug(size_t size, const char* file, int line);
void* user_calloc_debug(size_t nmemb, size_t size, const char* file, int line);
void* user_realloc_debug(void *ptr, size_t size, const char* file, int line);
void  user_free_debug(void *ptr, const char* file, int line);

#define malloc(s)      user_malloc_debug(s, __FILE__, __LINE__)
#define calloc(n, s)   user_calloc_debug(n, s, __FILE__, __LINE__)
#define realloc(p, s)  user_realloc_debug(p, s, __FILE__, __LINE__)
#define free(p)        user_free_debug(p, __FILE__, __LINE__)
#else
void* user_malloc(size_t size);
void* user_calloc(size_t nmemb, size_t size);
void* user_realloc(void *ptr, size_t size);
void  user_free(void *ptr);

#define malloc(s)      user_malloc(s)
#define calloc(n, s)   user_calloc(n, s)
#define realloc(p, s)  user_realloc(p, s)
#define free(p)        user_free(p)
#endif

void mm_print_stat(void);
void mm_print_leaks(void);
void mm_print_detailed_stat(void);
void mm_periodic_check(void);
int  mm_verify_integrity(void);
void mm_reset_stats(void);
void mm_tick(void);

size_t mm_get_current_usage(void);
size_t mm_get_peak_usage(void);
size_t mm_get_total_allocated(void);
int    mm_get_active_blocks(void);

#else
#include <stdlib.h>
#define user_malloc(s)      malloc(s)
#define user_calloc(n, s)   calloc(n, s)
#define user_realloc(p, s)  realloc(p, s)
#define user_free(p)        free(p)

static inline void mm_print_stat(void) {}
static inline void mm_print_leaks(void) {}
static inline void mm_print_detailed_stat(void) {}
static inline void mm_periodic_check(void) {}
static inline int  mm_verify_integrity(void) { return 1; }
static inline void mm_reset_stats(void) {}
static inline void mm_tick(void) {}
static inline size_t mm_get_current_usage(void) { return 0; }
static inline size_t mm_get_peak_usage(void) { return 0; }
static inline size_t mm_get_total_allocated(void) { return 0; }
static inline int    mm_get_active_blocks(void) { return 0; }
#endif

#endif /* GSP_MEM_MONITOR_H */
