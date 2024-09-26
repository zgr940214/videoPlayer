#include <assert.h>
#include <stdio.h>

#include "main_clock.h"
#include "time_base.h"
#include "base/utils.h"
#include "core/logger.h"

static int              initialized = 0;
static master_clock_t   sys_clock;

inline master_clock_t* global_clock() {
    return &sys_clock;
};

void init_master_clock(master_clock_t *mc) {
    LARGE_INTEGER freq, count;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&count);
    mc->frequency = freq.QuadPart;
    mc->start = count.QuadPart;
    mc->tick = 0;
    mc->slaves = NULL;
    mc->listener = NULL;

    ngx_rbtree_node_t *sentinel = malloc(sizeof(sentinel));
    memset(sentinel, 0, sizeof(ngx_rbtree_node_t));
    ngx_rbtree_init(&mc->timer_tree, 
        sentinel, 
        ngx_rbtree_insert_timer_value);

    initialized = 1;
};

int create_clock(slave_clock_t **clock, mem_pool_t *pool) {
    assert(initialized == 1 && pool);
    *clock = (slave_clock_t *)mem_alloc(pool, sizeof(slave_clock_t), 1);
    memset(*clock, 0, sizeof(slave_clock_t));
    if (!*clock) {
        blog(LOG_WARNING, "master clock is not initialized!\n");
        return -1;
    }
    return 0;
};

inline void add_clock(master_clock_t *mc, slave_clock_t *clock) {
    slave_clock_t         *ck = mc->slaves;
    ASRT_NEQ(clock, NULL);
    ASRT_NEQ(ck, NULL);
    if (!ck) {
        ck = clock;
        return;
    }
    LIST_NODE_APPEND(&ck->lnode, &clock->lnode);
};

void remove_clock(master_clock_t *mc, slave_clock_t *clock) {
    slave_clock_t       *ck = mc->slaves;
    ASRT_NEQ(clock, NULL);
    ASRT_NEQ(ck, NULL);
    if (clock == ck) {
        if (ck->lnode.next) {
            slave_clock_t *next = LIST_CONTIANER(slave_clock_t, ck->lnode.next);
            LIST_REMOVE(&ck->lnode);
            mc->slaves = next;
        }
        return;
    }
    LIST_REMOVE(&clock->lnode);
};

void add_listener(master_clock_t *mc, clock_listener_t *listener) {
    clock_listener_t    *listener_head = mc->listener;
    ASRT_NEQ(listener_head, NULL);
    ASRT_NEQ(listener, NULL);
    if (!listener_head) {
        listener_head = listener;
        return; 
    }
    LIST_NODE_APPEND(&listener_head->lnode, &listener->lnode);
};

void remove_listerner(master_clock_t *mc, clock_listener_t *listener) {
    clock_listener_t    *listener_head = mc->listener;
    ASRT_NEQ(listener_head, NULL);
    ASRT_NEQ(listener, NULL);
    if (listener_head == listener) {
        clock_listener_t *next = 
            LIST_CONTIANER(clock_listener_t, listener->lnode.next);
        LIST_REMOVE(&listener->lnode);
        mc->listener = next;
        return;
    }
    LIST_REMOVE(&listener->lnode);
};

inline uint64_t get_mc_tick(master_clock_t *mc) {
    return mc->tick;
};

inline double get_mc_tick_secs(master_clock_t *mc) {
    double secs = (double)get_mc_tick(mc) / (double)mc->frequency;
    return secs;
};

inline uint64_t get_mc_tick_ms(master_clock_t *mc) {    
    return uint64_scale(get_mc_tick(mc), MILLISECONDS, mc->frequency);
};

inline uint64_t get_mc_tick_100ms(master_clock_t *mc) {
    return uint64_scale(get_mc_tick(mc), MILLISECONDS * 100, mc->frequency);
};

/// @brief 获取slave 时钟的tick
/// @param slave slave时钟
/// @return tick
inline uint64_t get_slave_tick(slave_clock_t *slave) {
    master_clock_t *mc = slave->master;
    ASRT_NEQ(mc, NULL);

    uint64_t tick = get_mc_tick(mc);
    
    return uint64_scale_tb(tick, slave->speed); 
};

/// @brief 主时钟的ticking
/// @param clock 主时钟
void tick(master_clock_t* clock) {
    LARGE_INTEGER t;
    QueryPerformanceCounter(&t);
    clock->tick = t.QuadPart - clock->start;
};

/// @brief 设置定时器任务，存储到rbtree中
/// @param date 日期tick
/// @param tm 定时器
void set_timer(master_clock_t* mc, uint64_t date, timer_t *tm) {
    ngx_rbtree_node_t *node = &tm->link;

    ngx_rbtree_insert(&mc->timer_tree, node);
};

timer_t* get_recent_timer(master_clock_t *mc) {
    ngx_rbtree_t *tree = &mc->timer_tree;
    ngx_rbtree_node_t *pre, *cur;
    pre = NULL;
    cur = tree->root;

    while(cur != tree->sentinel) {
        pre = cur;
        cur = cur->left;
    }
    if (pre)
        return ngx_rbtree_data(pre, timer_t, link);
    return NULL;
};