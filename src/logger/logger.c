#include "logger.h"

Logger resultLogger = {NULL};
Logger executionLogger = {NULL};
Logger consoleLogger = {NULL};

void log_message(Logger *logger, const char *format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(logger->out, format, args);
    va_end(args);
    // fflush(logger->out);
}

void init_loggers() {
    executionLogger.out = fopen("execution.log", "w");
    resultLogger.out = fopen("result.log", "w");
    consoleLogger.out = stdout;
}

void cleanup_loggers() {
    if (executionLogger.out != NULL && executionLogger.out != stdout && executionLogger.out != stderr) {
        fclose(executionLogger.out);
    }
    if (resultLogger.out != NULL && resultLogger.out != stdout && resultLogger.out != stderr) {
        fclose(resultLogger.out);
    }
}
