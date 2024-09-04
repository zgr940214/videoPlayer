#pragma once

#define align_size(size, alignment)     \
    ((size) + (alignment) - 1) & ((alignment) - 1)
    
typedef struct memory_pool {
    
} mem_pool_t;

static inline void* mem_alloc(memory_pool *p, size_t size, size_t alignment) {
    (void)p;
    size_t aligned_size = align_size(size, alignment);
    return malloc(aligned_size);
}

static inline void mem_free(memory_pool *p, void* addr) {
    (void)p;
    free(addr);
}