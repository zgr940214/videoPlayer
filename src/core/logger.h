#ifndef _MY_LOGGER_H
#define _MY_LOGGER_H
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

enum {
    LOG_DEBUG = 0x0100,
    LOG_INFO = 0x0200,
    LOG_WARNING = 0x0300,
    LOG_ERROR = 0x0400
};

typedef void (*log_handler_t)(int lvl, char *msg, va_list, void* meta);

void base_get_log_handler(log_handler_t *handler, void **params);

void base_set_log_handler(log_handler_t *handler, void* params);

int blogva(int lvl, char *format, va_list args);

int blog(int lvl, char *format, ...);

#endif