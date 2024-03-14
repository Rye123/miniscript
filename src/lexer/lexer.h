#ifndef _LEXER_H_
#define _LEXER_H_

#include "token.h"
#include "../../lib/list.h"

/*
Performs lexical analysis on `source`, storing the tokens in the dynamic list `tokenLs`.
*/
void lex(const char *source, List* tokenLs);

#endif
