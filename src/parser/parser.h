#ifndef _PARSER_H_
#define _PARSER_H_

#include <stdlib.h>
#include "../lexer/token.h"

/*
Parses an array of tokens into a syntax tree.
TODO: add syntax tree for parsing into the arguments, this should be allocated by the caller.
*/
void parse(Token** tokens, size_t tokenCount);

#endif
