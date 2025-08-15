#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// Memory statistics structure
typedef struct {
    size_t total_heap;
    size_t total_allocated;
    size_t total_free;
    size_t allocation_count;
} memory_stats_t;

// Memory management functions
void memory_init(void* start_addr, size_t size);
void* kmalloc(size_t size);
void kfree(void* ptr);
void* krealloc(void* ptr, size_t new_size);
void* kcalloc(size_t num_elements, size_t element_size);

// Memory utility functions
void memory_set(void* ptr, int value, size_t size);
void memory_copy(void* dest, const void* src, size_t size);
int memory_compare(const void* ptr1, const void* ptr2, size_t size);

// Memory management utilities
void coalesce_free_blocks(void);
memory_stats_t get_memory_stats(void);
bool is_memory_initialized(void);

// Convenience macros
#define KMALLOC(size) kmalloc(size)
#define KFREE(ptr) do { kfree(ptr); ptr = NULL; } while(0)
#define KCALLOC(count, size) kcalloc(count, size)
#define KREALLOC(ptr, size) krealloc(ptr, size)

// Memory alignment macros
#define ALIGN_UP(addr, align) (((addr) + (align) - 1) & ~((align) - 1))
#define ALIGN_DOWN(addr, align) ((addr) & ~((align) - 1))
#define IS_ALIGNED(addr, align) (((addr) & ((align) - 1)) == 0)

// Common memory sizes
#define KB(x) ((x) * 1024)
#define MB(x) ((x) * 1024 * 1024)
#define GB(x) ((x) * 1024ULL * 1024 * 1024)

#endif
