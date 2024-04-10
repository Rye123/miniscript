#include <stdlib.h>
#include <string.h>
#include "../logger/logger.h"
#include "error.h"

ErrorContext *errorContext = {NULL};

void criticalError(const char *msg)
{
    log_message(&executionLogger, "Critical Error: %s", msg);
    exit(1);
}

void initErrorContext(const char *source)
{
    if (errorContext != NULL)
        free(errorContext);
    errorContext = malloc(sizeof(ErrorContext));
    if (errorContext == NULL)
        criticalError("initErrorContext: Could not allocate memory for error context.\n");
    errorContext->source = source;
}

void _checkErrorContext()
{
    if (errorContext != NULL)
        return;
    criticalError("Error context not initialised.");
}

Error *error_new(ErrorType type, int lineNum, int colNum)
{
    _checkErrorContext();
    Error *err = malloc(sizeof(Error));
    err->type = type;
    err->ctx = errorContext;
    err->message[0] = '\0';
    err->lineNum = lineNum;
    err->colNum = colNum;
    return err;
}

void error_string(Error *error, char *dest, size_t destLen)
{
    _checkErrorContext();
    if (destLen < MAX_ERRSTR_LEN)
        criticalError("Size of buffer for error_string less than maximum error message length.");

    size_t i = 0;
    // 1. Copy error type in
    size_t typeSize = strnlen(ErrorTypeString[error->type], MAX_ERRSTR_LEN);
    strncpy(dest, ErrorTypeString[error->type], typeSize);
    i += typeSize;

    dest[i] = ':';
    dest[i+1] = ' ';
    i += 2;

    // 2. Copy error message in
    size_t msgSize = strnlen(error->message, MAX_ERRMSG_LEN);
    strncpy(dest + i, error->message, msgSize);
    i += msgSize;

    // 3. Add context
    if (error->lineNum == -1 || error->colNum == -1) {
        dest[i] = '\0';
        return;
    }

    dest[i] = '\n';
    i++;
    
    // 3.1. Identify the correct line
    int curLine = 0; size_t targetLineLen = 0;
    char c; size_t j = 0;
    do {
        if (curLine == error->lineNum) {
            int start_j = j;
            do {
                c = *(error->ctx->source + j);
                targetLineLen = j - start_j;
                j++;
            } while (c != '\n' && c != '\0');
            break;
        }
        c = *(error->ctx->source + j);
        if (c == '\n')
            curLine++;
        j++;
    } while (c != '\0');
    
    // 3.2. Copy line in
    if (targetLineLen > MAX_ERRCTX_LEN) {
        // Don't bother printing, too long
        dest[i-1] = '\0';
        return;
    }
        
    strncpy(dest + i, (error->ctx->source + j - targetLineLen - 1), targetLineLen);
    i += targetLineLen;
    
    // 3.3. Add the caret
    dest[i] = '\n';
    i++;

    for (size_t k = 0; k < error->colNum; k++) {
        dest[i+k] = ' ';
    }
    i += error->colNum;
    dest[i] = '^';
    dest[i+1] = '\0';
}

void error_free(Error *err)
{
    free(err);
}
