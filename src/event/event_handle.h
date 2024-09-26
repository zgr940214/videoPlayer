#ifndef _EVENT_H_
#define _EVENT_H_
#include <stdint.h>

typedef struct event_handle_t event_handle_t;

int create_event(event_handle_t **ev);

int wait_single_event(event_handle_t *ev, uint64_t duration);

int wait_multiple_event(event_handle_t **ev, int n, uint64_t duration, int wait_all);

int signal_event(event_handle_t *ev);

#endif