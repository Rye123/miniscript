#ifndef _SYMBOL_H_
#define _SYMBOL_H_
#include <stdlib.h>
#include "../lexer/token.h"

/**
Standard -- this is used by the executor
     START      -> LINE* EOF
     LINE       -> ASMT  | STMT | EOL      (EOL is for empty line)
     ASMT       -> IDENTIFIER = EXPR EOL   (Used for assignment or declaration)
     STMT       -> EXPR_STMT | PRNT_STMT | IF_STMT | WHILE_STMT | BREAK | CONTINUE | RETURN
     EXPR_STMT  -> EXPR EOL | EOL
     PRNT_STMT  -> print EXPR EOL | print EOL
     WHILE_STMT -> while EXPR EOL BLOCK end while EOL
     BREAK      -> break EOL
     CONTINUE   -> continue EOL
     RETURN     -> return EXPR EOL | return EOL
     IF_STMT    -> if EXPR then EOL BLOCK ELSEIF_STMT end if EOL |
                   if EXPR then EOL BLOCK ELSE_STMT   end if EOL |
                   if EXPR then EOL BLOCK             end if EOL
     ELSEIF_STMT-> else if EXPR then EOL BLOCK ELSEIF_STMT | 
                   else if EXPR then EOL BLOCK ELSE_STMT   |
                   else if EXPR then EOL BLOCK
     ELSE_STMT  -> else EOL BLOCK 
     BLOCK      -> LINE*
     EXPR       -> OR_EXPR | FN_EXPR | empty
     FN_EXPR    -> function ( ARG_LIST ) EOL BLOCK end function
     ARG_LIST   -> ARG,* ARG
     ARG        -> IDENTIFIER | IDENTIFIER = STRING | IDENTIFIER = NUMBER | IDENTIFIER = NULL
(LR) OR_EXPR    -> OR_EXPR or AND_EXPR | AND_EXPR
(LR) AND_EXPR   -> AND_EXPR and LOG_UNARY | LOG_UNARY
(RR) LOG_UNARY  -> not LOG_UNARY | EQUALITY
(LR) EQUALITY   -> EQUALITY == COMPARISON | EQUALITY != COMPARISON | COMPARISON
(LR) COMPARISON -> SUM > COMPARISON | SUM >= COMPARISON | SUM < COMPARISON | SUM <= COMPARISON | SUM
(LR) SUM        -> SUM + TERM | SUM - TERM | TERM
(LR) TERM       -> TERM * UNARY | TERM / UNARY | TERM % UNARY | UNARY
(RR) UNARY      -> +UNARY | -UNARY | POWER
(RR) POWER      -> PRIMARY ^ UNARY | PRIMARY
     PRIMARY    -> TERMINAL | ( EXPR ) | FN_CALL
     FN_CALL    -> IDENTIFIER ( FN_ARGS )
     FN_ARGS    -> EXPR,* EXPR
     TERMINAL   -> IDENTIFIER | STRING | NUMBER | NULL

Left-Recursion Fixed -- this is used by the parser, then converted to the above afterwards to re-introduce left-recursion
START        -> LINE* EOF
LINE         -> ASMT | STMT | EOL
ASMT         -> IDENTIFIER = EXPR EOL
STMT         -> EXPR_STMT | PRNT_STMT | IF_STMT | WHILE_STMT | BREAK | CONTINUE | RETURN
EXPR_STMT    -> EXPR EOL | EOL
PRNT_STMT    -> TOKEN_PRINT EXPR EOL | EOL
WHILE_STMT   -> while EXPR EOL BLOCK end while EOL
BREAK        -> break EOL
CONTINUE     -> continue EOL
RETURN       -> return EXPR EOL | return EOL
IF_STMT      -> if EXPR then EOL BLOCK ELSEIF_STMT end if EOL |
                if EXPR then EOL BLOCK ELSE_STMT   end if EOL |
                if EXPR then EOL BLOCK             end if EOL
ELSEIF_STMT  -> else if EXPR then EOL BLOCK ELSEIF_STMT | 
                else if EXPR then EOL BLOCK ELSE_STMT   |
                else if EXPR then EOL BLOCK
ELSE_STMT    -> else EOL BLOCK 
EXPR         -> OR_EXPR | FN_EXPR | empty
FN_EXPR      -> function ( ARG_LIST ) EOL BLOCK end function EOL
ARG_LIST     -> ARG,* ARG
ARG          -> IDENTIFIER | IDENTIFIER = STRING | IDENTIFIER = NUMBER | IDENTIFIER = NULL
OR_EXPR      -> AND_EXPR OR_EXPR_R
OR_EXPR_R    -> or OR_EXPR OR_EXPR_R | empty
AND_EXPR     -> LOG_UNARY AND_EXPR_R
AND_EXPR_R   -> and AND_EXPR AND_EXPR_R | empty
LOG_UNARY    -> not LOG_UNARY | EQUALITY
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
PRIMARY      -> TERMINAL | ( EXPR ) | FN_CALL
FN_CALL      -> IDENTIFIER ( FN_ARGS ) | TERMINAL
FN_ARGS      -> EXPR,* EXPR
TERMINAL     -> IDENTIFIER | STRING | NUMBER | NULL
 **/

typedef enum {
    SYM_START,
    SYM_LINE,
    SYM_ASMT, SYM_STMT,
    SYM_EXPR_STMT, SYM_PRNT_STMT,
    SYM_IFSTMT, SYM_ELSEIF, SYM_ELSE, SYM_BLOCK, SYM_WHILE,
    SYM_BREAK, SYM_CONTINUE, SYM_RETURN,
    SYM_EXPR,
    SYM_FN_EXPR, SYM_ARG_LIST, SYM_ARG,
    SYM_OR_EXPR, SYM_OR_EXPR_R,
    SYM_AND_EXPR, SYM_AND_EXPR_R,
    SYM_LOG_UNARY,
    SYM_EQUALITY, SYM_EQUALITY_R,
    SYM_COMPARISON, SYM_COMPARISON_R,
    SYM_SUM, SYM_SUM_R,
    SYM_TERM, SYM_TERM_R,
    SYM_UNARY, SYM_POWER, SYM_PRIMARY,
    SYM_FN_CALL, SYM_FN_ARGS,
    SYM_TERMINAL,
} SymbolType;

static const char* SymbolTypeString[] = {
    "SYM_START",
    "SYM_LINE",
    "SYM_ASMT", "SYM_STMT",
    "SYM_EXPR_STMT", "SYM_PRNT_STMT",
    "SYM_IFSTMT", "SYM_ELSEIF", "SYM_ELSE", "SYM_BLOCK", "SYM_WHILE",
    "SYM_BREAK", "SYM_CONTINUE", "SYM_RETURN",
    "SYM_EXPR",
    "SYM_FN_EXPR", "SYM_ARG_LIST", "SYM_ARG",
    "SYM_OR_EXPR", "SYM_OR_EXPR_R",
    "SYM_AND_EXPR", "SYM_AND_EXPR_R",
    "SYM_LOG_UNARY",
    "SYM_EQUALITY", "SYM_EQUALITY_R",
    "SYM_COMPARISON", "SYM_COMPARISON_R",
    "SYM_SUM", "SYM_SUM_R",
    "SYM_TERM", "SYM_TERM_R",
    "SYM_UNARY", "SYM_POWER", "SYM_PRIMARY",
    "SYM_FN_CALL", "SYM_FN_ARGS",
    "SYM_TERMINAL",
};

typedef struct _astnode {
    SymbolType type;
    Token *tok;
    size_t numChildren;
    struct _astnode *parent;
    struct _astnode **children;
} ASTNode;

ASTNode *astnode_new(SymbolType type, Token *tok);
ASTNode *astnode_clone(ASTNode *node);
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
