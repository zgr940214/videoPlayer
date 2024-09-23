#include <assert.h>
#include <stdio.h>

#include "base/utils.h"
#include "clock.h"
#include "logger.h"

static master_clock_t   sys_clock;
static bool             initialized = false;

inline master_clock_t* global_clock() {
    return &sys_clock;
};

void init_master_clock() {
    LARGE_INTEGER freq, count;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&count);
    sys_clock.frequency = freq.QuadPart;
    sys_clock.tick = count.QuadPart;
    sys_clock.slaves = NULL;
    sys_clock.listener = NULL;
    initialized = true;
};

int create_clock(clock_t **clock, mem_pool_t *pool) {
    assert(initialized && pool);
    *clock = (clock_t *)mem_alloc(pool, sizeof(clock_t), 1);
    if (!*clock) {
        blog(LOG_WARNING, "master clock is not initialized!\n");
        return -1;
    }
};

inline void add_clock(clock_t *clock) {
    master_clock_t  *sys_ck = global_clock();
    clock_t         *ck = sys_ck->slaves;
    ASRT_NEQ(clock, NULL);
    ASRT_NEQ(ck, NULL);
    if (!ck) {
        ck = clock;
        return;
    }
    LIST_NODE_APPEND(&ck->lnode, &clock->lnode);
};

void remove_clock(clock_t *clock) {
    master_clock_t  *sys_ck = global_clock();
    clock_t         *ck = sys_ck->slaves;
    ASRT_NEQ(clock, NULL);
    ASRT_NEQ(ck, NULL);
    if (clock == ck) {
        if (ck->lnode.next) {
            clock_t *next = LIST_CONTIANER(clock_t, ck->lnode.next);
            LIST_REMOVE(&ck->lnode);
            sys_ck->slaves = next;
        }
        return;
    }
    LIST_REMOVE(&clock->lnode);
};

void add_listener(clock_listener_t *listener) {
    master_clock_t      *sys_ck = global_clock();
    clock_listener_t    *listener_head = sys_ck->listener;
    ASRT_NEQ(listener_head, NULL);
    ASRT_NEQ(listener, NULL);
    if (!listener_head) {
        listener_head = listener;
        return; 
    }
    LIST_NODE_APPEND(&listener_head->lnode, &listener->lnode);
};

void remove_listerner(clock_listener_t *listener) {
    master_clock_t      *sys_ck = global_clock();
    clock_listener_t    *listener_head = sys_ck->listener;
    ASRT_NEQ(listener_head, NULL);
    ASRT_NEQ(listener, NULL);
    if (listener_head == listener) {
        clock_listener_t *next = 
            LIST_CONTIANER(clock_listener_t, listener->lnode.next);
        LIST_REMOVE(&listener->lnode);
        sys_ck->listener = next;
        return;
    }
    LIST_REMOVE(&listener->lnode);
}
