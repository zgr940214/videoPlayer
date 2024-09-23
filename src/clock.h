#ifndef _CLOCK_H_
#define _CLOCK_H_
#include <windows.h>
#include <stdint.h>
#include <atomic>

#include "time_base.h"
#include "memory_pool.h"
#include "list.h"

// central clock tick ..
typedef void(*onUpdate) (clock_t *, void **priv);

typedef struct clock_listener {
    onUpdate        call_back;
    clock_t         *ck;
    void            **priv;
    LIST_NODE_LIST
} clock_listener_t;

typedef struct clock {
    uint64_t        freq;  // can have its own frequency
    uint64_t        start;
    time_base_t     speed; // default = 1.0
    LIST_NODE_LIST
} clock_t;

typedef struct master_clock {
    uint64_t            frequency;
    uint64_t            tick;
    clock_t             *slaves;
    clock_listener_t    *listener;
} master_clock_t;

master_clock_t* global_clock();

void init_master_clock();

int create_clock(clock_t **clock, mem_pool_t *pool);

void add_clock(clock_t *clock);

void remove_clock(clock_t *clock);

void add_listener(clock_listener_t *listener);

void remove_listerner(clock_listener_t *listener);

void tick();

void set_timer(uint64_t date, clock_listener_t *cl);

#endif