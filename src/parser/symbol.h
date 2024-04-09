#ifndef _SYMBOL_H_
#define _SYMBOL_H_
#include <stdlib.h>
#include "../lexer/token.h"

/**
Standard -- this is used by the executor
     START      -> LINE* EOF
     LINE       -> ASMT  | STMT | EOL      (EOL is for empty line)
     ASMT       -> IDENTIFIER = EXPR EOL   (Used for assignment or declaration)
     STMT       -> EXPR_STMT | PRNT_STMT
     EXPR_STMT  -> EXPR EOL
     PRNT_STMT  -> TOKEN_PRINT EXPR EOL
     EXPR       -> EQUALITY
(LR) EQUALITY   -> EQUALITY == COMPARISON | EQUALITY != COMPARISON | COMPARISON
(LR) COMPARISON -> SUM > COMPARISON | SUM >= COMPARISON | SUM < COMPARISON | SUM <= COMPARISON | SUM
(LR) SUM        -> SUM + TERM | SUM - TERM | TERM
(LR) TERM       -> TERM * UNARY | TERM / UNARY | TERM % UNARY | UNARY
(RR) UNARY      -> +UNARY | -UNARY | POWER
(RR) POWER      -> PRIMARY ^ UNARY | PRIMARY
     PRIMARY    -> TERMINAL | ( EXPR )

Left-Recursion Fixed -- this is used by the parser, then converted to the above afterwards to re-introduce left-recursion
START        -> LINE* EOF
LINE         -> ASMT | STMT | EOL
ASMT         -> IDENTIFIER = EXPR EOL
STMT         -> EXPR_STMT | PRNT_STMT
EXPR_STMT    -> EXPR EOL
PRNT_STMT    -> TOKEN_PRINT EXPR EOL
EXPR         -> EQUALITY
EQUALITY     -> COMPARISON EQUALITY_R
EQUALITY_R   -> == EQUALITY EQUALITY_R |
                != EQUALITY EQUALITY_R | 
                empty
COMPARISON   -> SUM COMPARISON_R
COMPARISON_R -> >  COMPARISON COMPARISON_R | 
                >= COMPARISON COMPARISON_R | 
                <  COMPARISON COMPARISON_R | 
                <= COMPARISON COMPARISON_R | 
                empty
SUM          -> TERM SUM_R
SUM_R        -> + SUM SUM_R | - SUM SUM_R | empty
TERM         -> UNARY TERM_R
TERM_R       -> * TERM TERM_R | / TERM TERM_R | % TERM TERM_R | empty
UNARY        -> +UNARY | -UNARY | POWER
POWER        -> PRIMARY ^ UNARY | PRIMARY
PRIMARY      -> TERMINAL | ( EXPR )
TERMINAL     -> IDENTIFIER | STRING | NUMBER
 **/

typedef enum {
    SYM_START,
    SYM_LINE,
    SYM_ASMT, SYM_STMT,
    SYM_EXPR_STMT, SYM_PRNT_STMT,    
    SYM_EXPR,
    SYM_EQUALITY, SYM_EQUALITY_R,
    SYM_COMPARISON, SYM_COMPARISON_R,
    SYM_SUM, SYM_SUM_R,
    SYM_TERM, SYM_TERM_R,
    SYM_UNARY, SYM_POWER, SYM_PRIMARY, SYM_TERMINAL,
} SymbolType;

static const char* SymbolTypeString[] = {
    "SYM_START",
    "SYM_LINE",
    "SYM_ASMT", "SYM_STMT",
    "SYM_EXPR_STMT", "SYM_PRNT_STMT",
    "SYM_EXPR",
    "SYM_EQUALITY", "SYM_EQUALITY_R",
    "SYM_COMPARISON", "SYM_COMPARISON_R",
    "SYM_SUM", "SYM_SUM_R",
    "SYM_TERM", "SYM_TERM_R",
    "SYM_UNARY", "SYM_POWER", "SYM_PRIMARY", "SYM_TERMINAL"
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

// Adds token with type as a child of this node.
void astnode_addChild(ASTNode *node, const SymbolType type, Token *tok);

void astnode_addChildNode(ASTNode *parent, ASTNode *child);

// Adds a symbol with type `expectedType` as a child of this node.
void astnode_addChildExp(ASTNode *node, const SymbolType expectedType);

void astnode_clearChildren(ASTNode *node);

// Converts from parse tree to AST (i.e. removes *_R nodes)
ASTNode* astnode_gen(ASTNode *node);

#endif
