#include "memory.h"
#include "../text/text_utils.h"
#include <stddef.h>

// Simple heap allocator implementation
#define HEAP_START 0x100000  // 1MB
#define HEAP_SIZE  0x400000  // 4MB
#define BLOCK_MAGIC 0xDEADBEEF

typedef struct HeapBlock {
    uint32_t magic;
    uint32_t size;
    uint32_t is_free;
    struct HeapBlock* next;
    struct HeapBlock* prev;
} HeapBlock;

static HeapBlock* heap_start = NULL;
static uint8_t* heap_memory = (uint8_t*)HEAP_START;
static uint32_t heap_initialized = 0;
static uint32_t total_allocated = 0;
static uint32_t total_freed = 0;

void memory_init(void) {
    if (heap_initialized) return;

    // Initialize the first block
    heap_start = (HeapBlock*)heap_memory;
    heap_start->magic = BLOCK_MAGIC;
    heap_start->size = HEAP_SIZE - sizeof(HeapBlock);
    heap_start->is_free = 1;
    heap_start->next = NULL;
    heap_start->prev = NULL;

    heap_initialized = 1;

    print("Memory manager initialized\n", 0x0A);
    print("Heap at: ", COLOR_DEFAULT);
    print_hex(HEAP_START, COLOR_DEFAULT);
    print(" Size: ", COLOR_DEFAULT);
    print_hex(HEAP_SIZE, COLOR_DEFAULT);
    print(" bytes\n", COLOR_DEFAULT);
}

void* kmalloc(uint32_t size) {
    if (!heap_initialized) {
        memory_init();
    }

    if (size == 0) return NULL;

    // Align size to 8 bytes
    size = (size + 7) & ~7;

    HeapBlock* current = heap_start;
    while (current) {
        if (current->magic != BLOCK_MAGIC) {
            print("Heap corruption detected!\n", 0x0C);
            return NULL;
        }

        if (current->is_free && current->size >= size) {
            // Found a suitable block
            if (current->size > size + sizeof(HeapBlock) + 8) {
                // Split the block
                HeapBlock* new_block = (HeapBlock*)((uint8_t*)current + sizeof(HeapBlock) + size);
                new_block->magic = BLOCK_MAGIC;
                new_block->size = current->size - size - sizeof(HeapBlock);
                new_block->is_free = 1;
                new_block->next = current->next;
                new_block->prev = current;

                if (current->next) {
                    current->next->prev = new_block;
                }
                current->next = new_block;
                current->size = size;
            }

            current->is_free = 0;
            total_allocated += size;
            return (void*)((uint8_t*)current + sizeof(HeapBlock));
        }

        current = current->next;
    }

    print("Out of memory!\n", 0x0C);
    return NULL;
}

void kfree(void* ptr) {
    if (!ptr) return;

    HeapBlock* block = (HeapBlock*)((uint8_t*)ptr - sizeof(HeapBlock));

    if (block->magic != BLOCK_MAGIC) {
        print("Invalid free - corrupted block!\n", 0x0C);
        return;
    }

    if (block->is_free) {
        print("Double free detected!\n", 0x0C);
        return;
    }

    block->is_free = 1;
    total_freed += block->size;

    // Coalesce with next block if it's free
    if (block->next && block->next->is_free) {
        block->size += block->next->size + sizeof(HeapBlock);
        HeapBlock* next_next = block->next->next;
        if (next_next) {
            next_next->prev = block;
        }
        block->next = next_next;
    }

    // Coalesce with previous block if it's free
    if (block->prev && block->prev->is_free) {
        block->prev->size += block->size + sizeof(HeapBlock);
        HeapBlock* next = block->next;
        if (next) {
            next->prev = block->prev;
        }
        block->prev->next = next;
    }
}

uint32_t get_total_allocated(void) {
    return total_allocated;
}

uint32_t get_total_freed(void) {
    return total_freed;
}

uint32_t get_heap_usage(void) {
    if (!heap_initialized) return 0;

    uint32_t used = 0;
    HeapBlock* current = heap_start;

    while (current) {
        if (!current->is_free) {
            used += current->size + sizeof(HeapBlock);
        }
        current = current->next;
    }

    return used;
}

uint32_t get_free_memory(void) {
    if (!heap_initialized) return 0;

    uint32_t free = 0;
    HeapBlock* current = heap_start;

    while (current) {
        if (current->is_free) {
            free += current->size;
        }
        current = current->next;
    }

    return free;
}

// Memory test function
int memory_test(void) {
    print("Running memory test...\n", 0x0E);

    void* ptrs[10];
    uint32_t sizes[] = {32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384};

    // Test allocation
    for (int i = 0; i < 10; i++) {
        ptrs[i] = kmalloc(sizes[i]);
        if (!ptrs[i]) {
            print("Failed to allocate ", 0x0C);
            print_dec(sizes[i], 0x0C);
            print(" bytes\n", 0x0C);
            return 0;
        }

        // Write test pattern
        uint8_t* mem = (uint8_t*)ptrs[i];
        for (uint32_t j = 0; j < sizes[i]; j++) {
            mem[j] = (uint8_t)(i + j);
        }
    }

    // Test read back
    for (int i = 0; i < 10; i++) {
        uint8_t* mem = (uint8_t*)ptrs[i];
        for (uint32_t j = 0; j < sizes[i]; j++) {
            if (mem[j] != (uint8_t)(i + j)) {
                print("Memory corruption detected at block ", 0x0C);
                print_dec(i, 0x0C);
                print(" offset ", 0x0C);
                print_dec(j, 0x0C);
                print("\n", 0x0C);
                return 0;
            }
        }
    }

    // Free all blocks
    for (int i = 0; i < 10; i++) {
        kfree(ptrs[i]);
    }

    print("Memory test passed!\n", 0x0A);
    return 1;
}
