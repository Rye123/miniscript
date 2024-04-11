#ifndef _INTERPRETER_H_
#define _INTERPRETER_H_

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include "logger/logger.h"
#include "error/error.h"
#include "lexer/token.h"
#include "lexer/lexer.h"
#include "parser/parser.h"
#include "executor/executor.h"
#include "executor/symboltable.h"
#define REPL_BUF_MAX 2000
#define LINE_MAX 1000

typedef enum {
    INIT,
    LEXING,
    PARSING,
    EXECUTING,
    CLEANING,
    ERROR,
    STOP
} State;

typedef struct {
    State current_state;
} FSM;

void transition(FSM *fsm, int success);

int runLine(const char *source, Context *executionContext, int asREPL);
void runFile(const char* fname);
void runREPL();

#endif

