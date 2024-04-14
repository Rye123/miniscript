#include "logger/logger.h"
#include "interpreter.h"

int main(int argc, char **argv) {
    init_loggers();

    if (argc == 1)
        runREPL();
    else if (argc == 2)
        runFile(argv[1]);
    else {
        log_message(&consoleLogger, "Usage: ./miniscript [file]\n");
        cleanup_loggers();
        return 1;
    }
    cleanup_loggers();
    return 0;
}
