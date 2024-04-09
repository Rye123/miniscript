#ifndef _PARSER_H_
#define _PARSER_H_

#include <stdlib.h>
#include "../lexer/token.h"
#include "symbol.h"

void parse(ASTNode *root, Token **tokens, size_t tokensCount);
void parseLine(ASTNode *parent, Token **tokens, size_t tokensLen, size_t *curIdx);
void parseAsmt(ASTNode *parent, Token **tokens, size_t tokensLen, size_t *curIdx);
void parseStmt(ASTNode *parent, Token **tokens, size_t tokensLen, size_t *curIdx);
void parseExprStmt(ASTNode *parent, Token **tokens, size_t tokensLen, size_t *curIdx);
void parsePrntStmt(ASTNode *parent, Token **tokens, size_t tokensLen, size_t *curIdx);
void parseExpr(ASTNode *parent, Token **tokens, size_t tokensLen, size_t *curIdx);
void parseOrExpr(ASTNode *parent, Token **tokens, size_t tokensLen, size_t *curIdx);
void parseOrExprR(ASTNode *parent, Token **tokens, size_t tokensLen, size_t *curIdx);
void parseAndExpr(ASTNode *parent, Token **tokens, size_t tokensLen, size_t *curIdx);
void parseAndExprR(ASTNode *parent, Token **tokens, size_t tokensLen, size_t *curIdx);
void parseLogUnary(ASTNode *parent, Token **tokens, size_t tokensLen, size_t *curIdx);
void parseEquality(ASTNode *parent, Token **tokens, size_t tokensLen, size_t *curIdx);
void parseEqualityR(ASTNode *parent, Token **tokens, size_t tokensLen, size_t *curIdx);
void parseComparison(ASTNode *parent, Token **tokens, size_t tokensLen, size_t *curIdx);
void parseComparisonR(ASTNode *parent, Token **tokens, size_t tokensLen, size_t *curIdx);
void parseSum(ASTNode *parent, Token **tokens, size_t tokensLen, size_t *curIdx);
void parseSumR(ASTNode *parent, Token **tokens, size_t tokensLen, size_t *curIdx);
void parseTerm(ASTNode *parent, Token **tokens, size_t tokensLen, size_t *curIdx);
void parseTermR(ASTNode *parent, Token **tokens, size_t tokensLen, size_t *curIdx);
void parseUnary(ASTNode *parent, Token **tokens, size_t tokensLen, size_t *curIdx);
void parsePower(ASTNode *parent, Token **tokens, size_t tokensLen, size_t *curIdx);
void parsePrimary(ASTNode *parent, Token **tokens, size_t tokensLen, size_t *curIdx);
void parseTerminal(ASTNode *parent, Token **tokens, size_t tokensLen, size_t *curIdx, TokenType expectedTokenType);
void parseLine(ASTNode *parent, Token **tokens, size_t tokensLen, size_t *curIdx);

/*
Performs syntax analysis on `tokens`, storing the AST in `root`.
*/
void parse(ASTNode *root, Token** tokens, size_t tokenCount);

#endif
