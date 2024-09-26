#ifndef _EVENT_H_
#define _EVENT_H_
#include <stdint.h>
#include "list.h"
#include "main_clock.h"

typedef void(*event_cb)(event_t *, void** priv, uint64_t time_stamp);

typedef enum {
    EVENT_NONE = 0,
    EVENT_VIDEO_PLAY = 0x1,
    EVENT_VIDEO_UPDATE = 0x1,
    EVENT_VIDEO_PAUSE = 0x2,
    EVENT_VIDEO_STOP = 0x4,

    EVENT_AUDIO_PLAY = 0xf1,
    EVENT_AUDIO_UPDATE = 0xf2,
    EVENT_AUDIO_PAUSE = 0xf3,
    EVENT_AUDIO_STOP = 0xf4,

    EVENT_DECODER_START = 0xff1,
    EVENT_DECODER_STOP = 0xff2

} event_id;

struct event {
    event_cb call_back;
    void **priv;
    LIST_NODE_LIST
}

static event_t  *priority_event_queue;
static event_t  *event_queue;

int merge_event_queue() {
    if (!event_queue) {// nothing to merge
        return 0;
    }

    if (!priority_event_queue) {// no new event
        priority_event_queue = event_queue;
        event_queue = NULL;
        return 0;
    }

    event_t *p = priority_event_queue;
    list_node_t *t =  LIST_PPREV(priority_event_queue);
    event_t *p_tail = LIST_CONTIANER(event_t, t); 
    priority_event_queue = event_queue;
    
    LIST_PPREV(event_queue)->next = p;
    LIST_PPREV(p)->next = event_queue;
    LIST_PPREV(p) = LIST_PPREV(event_queue);
    LIST_PPREV(event_queue) = &p_tail->lnode;
    
    event_queue = NULL;

    return 0;
}


int process_event() {
    merge_event_queue();
    event_t *p = priority_event_queue;
    event_t *del = p;

    while(p) {
        if (p->call_back) {
            uint64_t ts = get_sys_tick();
            p->call_back(p, p->priv, ts);
        }
        p = LIST_CONTIANER(event_t, LIST_PNEXT(p));
    }
}


#endif