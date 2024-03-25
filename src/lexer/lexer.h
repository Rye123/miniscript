#ifndef _LEXER_H_
#define _LEXER_H_
#include <stdlib.h>
#include "token.h"

/*
Performs lexical analysis on `source`, storing the tokens in the array `*(tokensPtr)`
- `tokensPtr`: Takes the ADDRESS of a tokens array of size 0. (sorry)
- `tokenCount`: Takes the ADDRESS of the size of that array.
- `source`: Takes a string of source code.
*/
void lex(const Token ***tokensPtr, size_t *tokenCount, const char *source);
#endif
