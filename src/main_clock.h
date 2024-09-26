#ifndef _CLOCK_H_
#define _CLOCK_H_
#include <stdint.h>

#include "rbtree.h"
#include "list.h"
#include "time_base.h"
#include "memory_pool.h"

#define MILLISECONDS 1000000
#define NANOSECONDS 1000000000

typedef void(*onUpdate) (slave_clock_t*, void **);  // 主时钟tick回调
typedef void(*onTimer) (void **);                   // 计时器回调

// 目前的时钟设计参考vlc, 有一个主时钟负责通过系统调用根据系统的分辨率来计算从程序启动到当前的tick数
// 每个主时钟可以有不同的从时钟，从时钟有一个start tick，有自己的速度，根据主时钟 和速度计算自己时钟的tick
// 在同步的时候，每个时钟有自己的delay， 表示自己的流比tick慢多少。在同步的时候，一个主动同步的从时钟
// 会把自己的delay 送到 主时钟上，并且主时钟会把其他所有从时钟的delay 减少该delay，并且主时钟的start
// 如果不溢出的情况下会增加 delay，然后发送给所有从时钟的所属部件一个同步消息。
// 比方说音频为主同步的部件，那么视频或者其他部件在被动同步之后，其delay不为0的情况下。
// 就需要相应的同步处理，比如，视频流可以加速播放 直到这次delay消失等等，后续有网络流的情况更加复杂
typedef struct slave_clock_t {
    master_clock_t  *master;
    uint64_t        start;  
    utime_base_t    speed; // default = 1.0
    LIST_NODE_LIST
} slave_clock_t;

typedef struct timer_t {
    slave_clock_t       *ck; // 所属的从时钟
    uint64_t            date; // 到期时对应的系统时钟 tick
    onTimer             callback; // 回调
    void                **priv; // 参数
    ngx_rbtree_node_t   link; //红黑树节点
} timer_t;

typedef struct clock_listener_t {
    onUpdate            call_back;
    slave_clock_t       *ck;
    void                **priv;
    LIST_NODE_LIST
} clock_listener_t;

typedef struct master_clock {
    uint64_t            start;
    uint64_t            frequency;
    uint64_t            tick;
    slave_clock_t       *slaves;
    clock_listener_t    *listener;
    ngx_rbtree_t        timer_tree;
} master_clock_t;

/// @brief 获取全局时钟
/// @return 
master_clock_t* global_clock();

/// @brief 初始化主时钟
void init_master_clock(master_clock_t *mc);

/// @brief 创建一个从时钟
/// @param clock 从时钟
/// @param pool 内存池
/// @return 0成功 其他失败
int create_clock(slave_clock_t **clock, mem_pool_t *pool);

/// @brief 添加/删除 一个从时钟
/// @param mc 主时钟
/// @param clock 从时钟
void add_clock(master_clock_t *mc, slave_clock_t *clock);
void remove_clock(master_clock_t *mc, slave_clock_t *clock);

/// @brief 添加/删除 一个主时钟 tick之后的回调， 一般很少使用（除非需要跟主时钟tempo一致） 
/// @param mc 主时钟
/// @param listener 监听结构
void add_listener(master_clock_t *mc, clock_listener_t *listener);
void remove_listerner(master_clock_t *mc, clock_listener_t *listener);

inline uint64_t get_mc_tick(master_clock_t *mc);
inline double get_mc_tick_secs(master_clock_t *mc);
inline uint64_t get_mc_tick_ms(master_clock_t *mc);
inline uint64_t get_mc_tick_100ms(master_clock_t *mc);

uint64_t get_slave_tick(slave_clock_t *slave);

void tick(master_clock_t* clock);

void set_timer(master_clock_t* mc, uint64_t date, timer_t *tm); // 主时钟有一个 红黑树存储所有timer

timer_t* get_recent_timer(master_clock_t* mc);  //从红黑树种获取最近的timer


#define init_sys_clock()                init_master_clock(global_clock())
#define add_clock_sys(clock)            add_clock(global_clock(), clock)
#define remove_clock_sys(clock)         remove_clock(global_clock(), clock)
#define add_listener_sys(clock)         add_listener(global_clock(), clock)
#define remove_listener_sys(clock)      remove_listener(global_clock(), clock)
#define set_sys_timer(date, tm)         set_timer(global_clock(), date, tm)
#define get_sys_recent_timer()          get_recent_timer(global_clock())
#define get_sys_tick()                  get_mc_tick(global_clock())
#define get_sys_tick_secs()             get_mc_tick_secs(global_clock())
#define get_sys_tick_ms()               get_mc_tick_ms(global_clock())
#define get_sys_tick_100ms()            get_mc_tick_100ms(global_clock())

#endif