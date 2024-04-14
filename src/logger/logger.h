#ifndef LOGGER_H
#define LOGGER_H

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

typedef struct {
    FILE* out;
} Logger;

extern Logger resultLogger;
extern Logger executionLogger;
extern Logger consoleLogger;

void log_message(Logger *logger, const char *format, ...);
void init_loggers(void);
void cleanup_loggers(void);

#endif
