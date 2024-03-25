#ifndef _SYMBOL_H_
#define _SYMBOL_H_
#include <stdlib.h>
#include "../lexer/token.h"

/**
Standard
EXPR       -> EQUALITY
EQUALITY   -> COMPARISON == EQUALITY | COMPARISON != EQUALITY | COMPARISON
COMPARISON -> SUM > COMPARISON | SUM >= COMPARISON | SUM < COMPARISON | SUM <= COMPARISON | SUM
SUM        -> SUM + TERM | SUM - TERM | TERM
TERM       -> TERM * UNARY | TERM / UNARY | TERM % UNARY | UNARY
UNARY      -> +UNARY | -UNARY | POWER
POWER      -> PRIMARY ^ UNARY | PRIMARY
PRIMARY    -> TERMINAL | ( EXPR )

Left-Recursion Fixed
EXPR       -> EQUALITY
EQUALITY   -> COMPARISON == EQUALITY | COMPARISON != EQUALITY | COMPARISON
COMPARISON -> SUM > COMPARISON | SUM >= COMPARISON | SUM < COMPARISON | SUM <= COMPARISON | SUM
SUM        -> TERM SUM_R
SUM_R      -> + SUM SUM_R | empty
TERM       -> UNARY TERM_R
TERM_R     -> * TERM TERM_R | / TERM TERM_R | % TERM TERM_R | empty
UNARY      -> +UNARY | -UNARY | POWER
POWER      -> PRIMARY ^ UNARY | PRIMARY
PRIMARY    -> TERMINAL | ( EXPR )
TERMINAL   -> IDENTIFIER | STRING | NUMBER
 **/

typedef enum {
    SYM_EXPR,
    SYM_EQUALITY,
    SYM_COMPARISON,
    SYM_SUM,
    SYM_SUM_R,
    SYM_TERM,
    SYM_TERM_R,
    SYM_UNARY,
    SYM_POWER,
    SYM_PRIMARY,
    SYM_TERMINAL,
} SymbolType;

static const char* SymbolTypeString[] = {
    "SYM_EXPR", "SYM_EQUALITY", "SYM_COMPARISON", "SYM_SUM",
    "SYM_SUM_R", "SYM_TERM", "SYM_TERM_R", "SYM_UNARY",
    "SYM_POWER", "SYM_PRIMARY", "SYM_TERMINAL"
};


// Returns 0 if the token is unimplemented, 1 if it's intended to be implemented.
int isTokenUnimplemented(Token tok);


typedef struct _astnode {
    SymbolType type;
    Token *tok;
    size_t numChildren;
    struct _astnode *parent;
    struct _astnode **children;
} ASTNode;

ASTNode *astnode_new(SymbolType type, Token *tok);
void astnode_free(ASTNode *node);
void astnode_print(ASTNode *node);

// Returns if this node is expanded (i.e. is either a terminal, or a nonterminal with expanded children)
int astnode_isExpanded(ASTNode *node);

// Adds token with type as a child of this node.
void astnode_addChild(ASTNode *node, const SymbolType type, Token *tok);

// Adds a symbol with type `expectedType` as a child of this node.
void astnode_addChildExp(ASTNode *node, const SymbolType expectedType);

// Evaluates all non-expanded children of this node (i.e. children who are nonterminals and aren't expanded)
void astnode_eval(ASTNode *node);

#endif
