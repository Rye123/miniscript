#include <stdlib.h>
#include <string.h>
#include "../logger/logger.h"
#include "error.h"

ErrorContext *errorContext = NULL;

void criticalError(const char *msg) {
    log_message(&executionLogger, "Critical Error: %s", msg);
    exit(1);
}

void initErrorContext(const char *source) {
    if (errorContext != NULL)
        free(errorContext);
    errorContext = malloc(sizeof(ErrorContext));
    if (errorContext == NULL)
        criticalError("initErrorContext: Could not allocate memory for error context.\n");
    errorContext->source = source;
}

void _checkErrorContext(void) {
    if (errorContext != NULL)
        return;
    if (errorContext->source != NULL)
        return;
    criticalError("Error context not initialised.");
}

Error *error_new(ErrorType type, int lineNum, int colNum) {
    Error *err;
    _checkErrorContext();
    err = malloc(sizeof(Error));
    err->type = type;
    err->ctx = errorContext;
    err->message[0] = '\0';
    err->lineNum = lineNum;
    err->colNum = colNum;
    return err;
}

void error_string(Error *error, char *dest, size_t destLen) {
    size_t i = 0;
    size_t j;
    size_t k;
    size_t typeSize;
    size_t locSize;
    size_t msgSize;
    const char *source;
    size_t sourceLen;
    size_t tgtIdxStart;
    size_t tgtIdxEnd;
    size_t curLine;
    char c;
    size_t tgtLineSz;

    _checkErrorContext();
    if (destLen < MAX_ERRSTR_LEN)
        criticalError("Size of buffer for error_string less than maximum error message length.");


    /* 1. Copy error type in */
    typeSize = strnlen(ErrorTypeString[error->type], MAX_ERRSTR_LEN);
    strncpy(dest, ErrorTypeString[error->type], typeSize);
    i += typeSize;

    /* 2. Error location */
    locSize = snprintf(dest + i, MAX_ERRTYPE_LEN - typeSize, " (Line %d, Column %d)", error->lineNum + 1,
                       error->colNum + 1);
    i += locSize;
    dest[i] = ':';
    dest[i + 1] = ' ';
    i += 2;

    /* 2. Copy error message in */
    msgSize = strnlen(error->message, MAX_ERRMSG_LEN);
    strncpy(dest + i, error->message, msgSize);
    i += msgSize;

    /* 3. Add context */
    if (error->lineNum == -1 || error->colNum == -1) {
        /* If no context, return */
        dest[i] = '\0';
        return;
    }

    dest[i] = '\n';
    i++;

    /* 3.1. Identify the correct line */
    source = error->ctx->source;
    sourceLen = strlen(source);
    tgtIdxStart = 0;
    tgtIdxEnd = 0;
    curLine = 0;

    for (j = 0; j < sourceLen; j++) {
        c = *(source + j);
        if (curLine == error->lineNum) {
            /* At the desired line. */
            /* Need to identify the size of the line */
            tgtIdxStart = j;
            tgtIdxEnd = j;
            for (k = j; k < sourceLen; k++) {
                c = *(source + k);
                if (c == '\n' || c == '\0')
                    break;
                tgtIdxEnd++;
            }
            break;
        }

        if (c == '\n')
            curLine++;
    }

    /* 3.2. Copy the entire line in */
    tgtLineSz = tgtIdxEnd - tgtIdxStart;
    if (tgtLineSz > MAX_ERRCTX_LEN) {
        /* Don't bother printing, too long */
        dest[i - 1] = '\0';
        return;
    }
    strncpy(dest + i, source + tgtIdxStart, tgtLineSz);
    i += tgtLineSz;

    /* 3.3. Add the caret */
    dest[i] = '\n';
    i++;

    for (k = 0; k < error->colNum; k++) {
        dest[i + k] = ' ';
    }
    i += error->colNum;
    dest[i] = '^';
    dest[i + 1] = '\0';
}

void error_free(Error *err) {
    if (err)
        free(err);
}
