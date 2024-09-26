#ifndef _CIRCULAR_BUF_H_
#define _CIRCULAR_BUF_H_
#include <stdint.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "memory_pool.h"
#include "atomic.h"

#define PAGESIZE        0x00001000       

typedef struct cqueue_elem {
    uint32_t    index;
    char        data;
} cq_elem_t;

typedef struct circular_queue {
    void                    *buf;
    uint32_t                capacity;
    
    uint32_t                elem_size;
    uint32_t                step_size;
    vp_atomic_uint32_t      count;
    vp_atomic_uint32_t      free;

    vp_atomic_uint32_t      first;
    vp_atomic_uint32_t      last;
    uint32_t                end;
} cqueue_t;

#define RoundAlignment(size, align) (((size) + (align) - 1) & ~((align) - 1))        

#define cq_element(cq, index) \
    (cq_elem_t*)((uintptr_t)(cq)->buf + (index) * (cq)->step_size)

#define cq_elem_data(cq, type) ((type *)(&(cq)->data))

#define cq_last_inc(q)                              \
    do {                                            \
        (q)->last++;                                \
        if ((q)->last >= (q)->capacity) {           \
            (q)->last -= (q)->capacity;             \
        }                                           \
    } while(0);
    
#define cq_first_inc(q)                             \
    do {                                            \
        (q)->first++;                               \
        if ((q)->first >= (q)->capacity) {          \
            (q)->first -= (q)->capacity;            \
        }                                           \
    } while(0);


static inline cqueue_t* create_circular_queue(int elem_size, 
    int elem_capacity, mem_pool_t *mem_pool) {
        (void)mem_pool;
        assert(elem_size > 0 && elem_capacity > 0);

        cqueue_t *q = (cqueue_t *)malloc(sizeof(cqueue_t));
        if (!q)
            return NULL;

        q->step_size = elem_size + sizeof(uint32_t);
        q->step_size = RoundAlignment(q->step_size, 64);
        
        uint32_t size = q->step_size * elem_capacity;
        size = RoundAlignment(size, PAGESIZE);

        q->buf = malloc(size);
        if (!q->buf) {
            free(q);
            return NULL;
        }

        q->capacity = elem_capacity;
        q->elem_size = elem_size;
        q->count = 0;
        q->free = elem_capacity;
        q->first = 0;
        q->last = 0;
        q->end = elem_capacity - 1;
        
        for (int i = 0; i < elem_capacity; i++) {
            cq_elem_t *elem = cq_element(q, i);
            elem->index = i;
        }

        return q;
    }

    cq_elem_t* circular_queue_get_next(cqueue_t *q) {
        if (q->free == 0) {
            return NULL;
        }

        cq_elem_t* elem = cq_element(q, q->last++);
        if (q->last >= q->capacity) {
            q->last -= q->capacity;
        }
        q->free--;

        return elem;
    }

    int circular_queue_copy_insert(cqueue_t *q, void* data) {
        if (q->free == 0) {
            return -1;
        }

        cq_elem_t* elem = cq_element(q, q->last++);
        memcpy(elem->data, data, q->elem_size);
        return 0;
    }

    int circular_queue_pop(cqueue_t *q) {
        q->free++;
        cq_first_inc(q);
        return 0;
    }

    int circular_queue_copy_pop(cqueue_t *q, void* data) {
        cq_elem_t *elem = cq_element(q, q->first);
        memcpy(data, elem->data, q->elem_size);
        q->free++;
        cq_first_inc(q);
        return 0;
    }

    int circular_queue_top(cqueue_t *q, void** data) {
        if (data = NULL) {
            return -1;
        }
        cq_elem_t* elem = cq_element(q, q->first);
        *data = elem->data;
        return 0;
    }

#endif