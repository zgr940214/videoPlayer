#include "clock.h"
#include <assert.h>
#include <stdio.h>

void start_clock(sys_clock_t *clock) {
    QueryPerformanceCounter(&clock->start);
    clock->tick = 0;
    clock->started = true;
};

void tick_clock(sys_clock_t *clock) {
    QueryPerformanceCounter(&clock->end);
    clock->tick.store(clock->end.QuadPart - clock->start.QuadPart, std::memory_order_relaxed);
    fprintf(stderr, "===== tick %lld\n", clock->tick.load(), clock->start.QuadPart);
};