#ifndef _ERROR_H_
#define _ERROR_H_
#define MAX_ERRCTX_LEN 255
#define MAX_ERRMSG_LEN 223

// MAX_ERRSTR_LEN = 32 (max size of type string) + 223 (max size of errmsg) + 255 + 255 (context and space for context)
#define MAX_ERRSTR_LEN 765
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
    ERR_RUNTIME,
    ERR_RUNTIME_TYPE,
    ERR_RUNTIME_NAME,
} ErrorType;

static const char *ErrorTypeString[] = {
    "Tokenisation Error",
    "Syntax Error",
    "Runtime Error",
    "Type Error",
    "Name Error"
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
