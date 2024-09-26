#include <windows.h>
#include "event_handle.h"
#include "utils.h"

struct event_handle_t {
    HANDLE hEvent;
};

int create_event(event_handle_t **ev) {
    *ev = (event_handle_t *)malloc(sizeof(event_handle_t));

    (*ev)->hEvent = CreateEventEx(NULL, NULL, 0, EVENT_ALL_ACCESS);
    if ((*ev)->hEvent == NULL) {
        blog(LOG_ERROR, "CreateEventEx failed: \n");
        handleError(blog);
        return -1;
    }

    return 0;
};

int wait_single_event(event_handle_t *ev, uint64_t duration) {
    DWORD ret = WaitForSingleObjectEx(ev->hEvent, (DWORD)duration, FALSE);
    if (ret == WAIT_FAILED) {
        blog(LOG_ERROR, "WaitForSingleObjectEx failed: \n");
        handleError(blog);
    }
    
    return ret;
};

int wait_multiple_event(event_handle_t **ev, int n, uint64_t duration, int wait_all) {
    HANDLE *hArr = malloc(sizeof(HANDLE) * n);
    BOOL wa = wait_all;

    for (int i = 0; i < n; i++) {
        hArr[i] = (*(ev + i))->hEvent;
    }

    DWORD ret = WaitForMultipleObjectsEx(n, hArr, wa, (DWORD)duration, FALSE);
    if (ret == WAIT_FAILED) {
        blog(LOG_ERROR, "WaitForMultipleObjectsEx failed\n");
        handleError(blog);
    }

    return ret;
};

int signal_event(event_handle_t *ev) {
    BOOL ret = SetEvent(ev->hEvent);
    if (ret) {
        blog(LOG_ERROR, "SetEvent failed\n");
        handleError(blog);
    }
    return ret;
};