#ifndef _LEXER_H_
#define _LEXER_H_
#include <stdlib.h>
#include "../error/error.h"
#include "token.h"

// Adds a lexing error to the list.
void lexError(const char *errStr, int lineNum, int colNum, const Error ***errorsPtr, size_t *errorCount);

/*
Performs lexical analysis on `source`, storing the tokens in the array `*(tokensPtr)`
- `tokensPtr`: Takes the ADDRESS of a tokens array of size 0. (sorry)
- `tokenCount`: Takes the ADDRESS of the size of that array.
- `source`: Takes a string of source code.
*/
int lex(const Token ***tokensPtr, size_t *tokenCount, const Error ***errorsPtr, size_t *errorCount, const char *source);
#endif
