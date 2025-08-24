#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>
#include <stddef.h>

// Memory management functions
void memory_init(void);
void* kmalloc(uint32_t size);
void kfree(void* ptr);

// Memory statistics
uint32_t get_total_allocated(void);
uint32_t get_total_freed(void);
uint32_t get_heap_usage(void);
uint32_t get_free_memory(void);

// Memory testing
int memory_test(void);

#endif
