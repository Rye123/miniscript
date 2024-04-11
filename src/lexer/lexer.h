#ifndef _LEXER_H_
#define _LEXER_H_
#include <stdlib.h>
#include "../error/error.h"
#include "token.h"

typedef struct {
    int hasError;
    char errorMessage[MAX_ERRMSG_LEN];
    int lineNum;
    int colNum;
} LexResult;

void initLexResult(LexResult *lexResult);
void lexResultUpdate(LexResult *lexerResult, int hasError, const char *errStr, int lineNum, int colNum);


// Adds a lexer error to the list.
void lexError(const char *errStr, int lineNum, int colNum, const Error ***errorsPtr, size_t *errorCount);

/*
Performs lexical analysis on `source`, storing the tokens in the array `*(tokensPtr)`
- `tokensPtr`: Takes the ADDRESS of a tokens array of size 0. (sorry)
- `tokenCount`: Takes the ADDRESS of the size of that array.
- `source`: Takes a string of source code.
*/
void lex(const Token ***tokensPtr, size_t *tokenCount, const char *source, LexResult *lexerResult);
#endif
