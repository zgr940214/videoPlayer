#ifndef _MY_MEM_POOL_H_
#define _MY_MEM_POOL_H_
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define align_size(size, alignment)     \
    ((size) + (alignment) - 1) & ((alignment) - 1)
    
typedef struct mem_pool_t {
    int a;
} mem_pool_t;

static inline void* mem_alloc(mem_pool_t *p, uint64_t size, uint64_t alignment) {
    uint64_t aligned_size = align_size(size, alignment);
    return malloc(aligned_size);
}

static inline void mem_free(mem_pool_t *p, void* addr) {
    free(addr);
}

#endif