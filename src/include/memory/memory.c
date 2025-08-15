#include "memory.h"

// Simple memory allocator state
static void* heap_start = NULL;
static void* heap_end = NULL;
static size_t heap_size = 0;
static bool memory_initialized = false;

// Simple free block structure
typedef struct free_block {
    size_t size;
    struct free_block* next;
} free_block_t;

static free_block_t* free_list = NULL;

// Memory statistics
static size_t total_allocated = 0;
static size_t total_free = 0;
static size_t allocation_count = 0;

void memory_init(void* start_addr, size_t size) {
    heap_start = start_addr;
    heap_end = (void*)((char*)start_addr + size);
    heap_size = size;

    // Initialize the free list with the entire heap as one big free block
    free_list = (free_block_t*)heap_start;
    free_list->size = size - sizeof(free_block_t);
    free_list->next = NULL;

    total_free = free_list->size;
    total_allocated = 0;
    allocation_count = 0;
    memory_initialized = true;
}

// Align size to 8-byte boundary
static size_t align_size(size_t size) {
    return (size + 7) & ~7;
}

void* kmalloc(size_t size) {
    if (!memory_initialized || size == 0) {
        return NULL;
    }

    size = align_size(size);

    // Search for a suitable free block
    free_block_t* prev = NULL;
    free_block_t* current = free_list;

    while (current != NULL) {
        if (current->size >= size) {
            // Found a suitable block
            void* allocated_ptr = (void*)((char*)current + sizeof(free_block_t));

            // If the block is much larger than needed, split it
            if (current->size > size + sizeof(free_block_t) + 16) {
                // Create a new free block from the remainder
                free_block_t* new_block = (free_block_t*)((char*)allocated_ptr + size);
                new_block->size = current->size - size - sizeof(free_block_t);
                new_block->next = current->next;

                // Update current block
                current->size = size;
                current->next = new_block;
            }

            // Remove current block from free list
            if (prev == NULL) {
                free_list = current->next;
            } else {
                prev->next = current->next;
            }

            // Update statistics
            total_allocated += current->size;
            total_free -= current->size;
            allocation_count++;

            return allocated_ptr;
        }

        prev = current;
        current = current->next;
    }

    // No suitable block found
    return NULL;
}

void kfree(void* ptr) {
    if (!memory_initialized || ptr == NULL) {
        return;
    }

    // Check if pointer is within heap bounds
    if (ptr < heap_start || ptr >= heap_end) {
        return;
    }

    // Get the block header
    free_block_t* block = (free_block_t*)((char*)ptr - sizeof(free_block_t));

    // Update statistics
    total_allocated -= block->size;
    total_free += block->size;
    allocation_count--;

    // Insert block back into free list (simple insertion at beginning)
    block->next = free_list;
    free_list = block;

    // Try to coalesce adjacent free blocks
    coalesce_free_blocks();
}

void* krealloc(void* ptr, size_t new_size) {
    if (!memory_initialized) {
        return NULL;
    }

    if (ptr == NULL) {
        return kmalloc(new_size);
    }

    if (new_size == 0) {
        kfree(ptr);
        return NULL;
    }

    new_size = align_size(new_size);

    // Get current block size
    free_block_t* block = (free_block_t*)((char*)ptr - sizeof(free_block_t));
    size_t old_size = block->size;

    if (new_size <= old_size) {
        // Shrinking or same size - no need to move
        return ptr;
    }

    // Need to allocate new block
    void* new_ptr = kmalloc(new_size);
    if (new_ptr == NULL) {
        return NULL;
    }

    // Copy old data
    memory_copy(new_ptr, ptr, old_size);

    // Free old block
    kfree(ptr);

    return new_ptr;
}

void* kcalloc(size_t num_elements, size_t element_size) {
    size_t total_size = num_elements * element_size;

    // Check for overflow
    if (num_elements != 0 && total_size / num_elements != element_size) {
        return NULL;
    }

    void* ptr = kmalloc(total_size);
    if (ptr != NULL) {
        memory_set(ptr, 0, total_size);
    }

    return ptr;
}

void coalesce_free_blocks(void) {
    if (free_list == NULL) {
        return;
    }

    // Simple coalescing: merge adjacent blocks
    free_block_t* current = free_list;

    while (current != NULL && current->next != NULL) {
        char* current_end = (char*)current + sizeof(free_block_t) + current->size;

        if (current_end == (char*)current->next) {
            // Adjacent blocks - merge them
            free_block_t* next_block = current->next;
            current->size += sizeof(free_block_t) + next_block->size;
            current->next = next_block->next;
        } else {
            current = current->next;
        }
    }
}

void memory_set(void* ptr, int value, size_t size) {
    unsigned char* byte_ptr = (unsigned char*)ptr;
    unsigned char byte_value = (unsigned char)value;

    for (size_t i = 0; i < size; i++) {
        byte_ptr[i] = byte_value;
    }
}

void memory_copy(void* dest, const void* src, size_t size) {
    unsigned char* dest_ptr = (unsigned char*)dest;
    const unsigned char* src_ptr = (const unsigned char*)src;

    for (size_t i = 0; i < size; i++) {
        dest_ptr[i] = src_ptr[i];
    }
}

int memory_compare(const void* ptr1, const void* ptr2, size_t size) {
    const unsigned char* p1 = (const unsigned char*)ptr1;
    const unsigned char* p2 = (const unsigned char*)ptr2;

    for (size_t i = 0; i < size; i++) {
        if (p1[i] < p2[i]) {
            return -1;
        } else if (p1[i] > p2[i]) {
            return 1;
        }
    }

    return 0;
}

memory_stats_t get_memory_stats(void) {
    memory_stats_t stats;
    stats.total_heap = heap_size;
    stats.total_allocated = total_allocated;
    stats.total_free = total_free;
    stats.allocation_count = allocation_count;
    return stats;
}

bool is_memory_initialized(void) {
    return memory_initialized;
}
