#ifndef _ERROR_H_
#define _ERROR_H_

// Max size of the string: " [error type] (Line [line], Column [col])"
#define MAX_ERRTYPE_LEN 64

// Max size of the error message itself
#define MAX_ERRMSG_LEN 255

// Max size of the line of code printed out as context for the error
#define MAX_ERRCTX_LEN 255

// Max size of the entire error string
#define MAX_ERRSTR_LEN 574
#include <stdlib.h>

// Denotes an error that is with the interpreter itself.
void criticalError(const char *msg);

typedef struct {
    const char *source;
} ErrorContext;

extern ErrorContext* errorContext;

// Initialises the error context for future errors
void initErrorContext(const char* source);

typedef enum {
    ERR_TOKEN,
    ERR_SYNTAX,
    ERR_SYNTAX_EOF,
    ERR_RUNTIME,
    ERR_RUNTIME_TYPE,
    ERR_RUNTIME_NAME,
} ErrorType;

static const char *ErrorTypeString[] = {
    "Tokenization Error", // original Lexer Error
    "Syntax Error",       // original Compiler Error
    "Unexpected EOF Error",
    "Runtime Error",
    "Type Error",
    "Undefined Identifier"
};

typedef struct {
    ErrorType type;
    ErrorContext* ctx;
    char message[MAX_ERRMSG_LEN];
    int lineNum;
    int colNum;
} Error;

// New error -- set lineNum and colNum to -1 to avoid adding context
Error* error_new(ErrorType type, int lineNum, int colNum);

// Outputs the error into the given string.
void error_string(Error* error, char* dest, size_t destLen);

// Frees the error
void error_free(Error* error);

#endif
