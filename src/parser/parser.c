#include <stdio.h>
#include <stdlib.h>
#include "symbol.h"
#include "parser.h"

void parse(Token **tokens, size_t tokenCount)
{
    return;
}

int main()
{
    ASTNode* nUnary = astnode_new(SYM_UNARY, NULL);
    astnode_addChild(nUnary, SYM_TERMINAL, token_new(TOKEN_PLUS, "+", 1, 0, 0));
    astnode_addChild(nUnary, SYM_TERMINAL, token_new(TOKEN_NUMBER, "2", 1, 0, 1));

    for (size_t i = 0; i < nUnary->numChildren; i++) {
	printf("Child %lu: \n", i);
	printf("\tSymbol type %d\n", nUnary->children[i]->type);
	astnode_print(nUnary->children[i]);
    }
}
