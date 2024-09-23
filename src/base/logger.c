#include "logger.h"

static void *meta = NULL;
static log_handler_t log_handler = def_log_handler;

void def_log_handler(int lvl, char *msg, va_list args, void* meta) {
    char out[8192];
    vsprintf(out, msg, args);
    switch (lvl)
    {
    case LOG_DEBUG:
        fprintf(stdout, "debug: %s\n", out);
        break;

    case LOG_INFO:
        fprintf(stdout, "info: %s\n", out);
        break;

    case LOG_WARNING:
        fprintf(stdout, "warning: %s\n", out);
        break;

    case LOG_ERROR:
        fprintf(stderr, "error: %s\n", out);
        fflush(stderr);
    }

    (void)meta;
}

void base_get_log_handler(log_handler_t *handler, void **params) {
    if (handler)
        *handler = log_handler;

    if (params) {
        *params = meta;
    }
}

void base_set_log_handler(log_handler_t *handler, void* params) {
    if (!handler)
        log_handler = def_log_handler;
    else 
        log_handler = *handler;

    if (params)
        meta = params;
}

void blogva(int lvl, char *format, va_list args) {
    log_handler(lvl, format, args, meta);
}

void blog(int lvl, char *format, ...) {
    va_list args;

    va_start(args, format);
    blogva(lvl, format, args);
    va_end(args);
}