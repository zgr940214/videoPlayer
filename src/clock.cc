#include "clock.h"
#include <assert.h>
#include <stdio.h>

void start_clock(sys_clock_t *clock) {
    QueryPerformanceCounter(&clock->start);
    clock->tick = 0;
    clock->started = true;
};

void tick_clock(sys_clock_t *clock) {
    if (false == clock->started) {
        start_clock(clock);
        getchar();
        fprintf(stderr, "~~~~~ tick %lld, start %llu\n", clock->tick.load(), clock->start.QuadPart);
        return;
    }
    QueryPerformanceCounter(&clock->end);
    clock->tick.store(clock->end.QuadPart - clock->start.QuadPart, std::memory_order_relaxed);
    fprintf(stderr, "===== tick %lld\n", clock->tick.load(), clock->start.QuadPart);
};