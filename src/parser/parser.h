#ifndef _PARSER_H_
#define _PARSER_H_

#include <stdlib.h>
#include "../error/error.h"
#include "../lexer/token.h"
#include "symbol.h"

Error* parseLine(ASTNode *parent, Token **tokens, size_t tokensLen, size_t *curIdx);
Error* parseAsmt(ASTNode *parent, Token **tokens, size_t tokensLen, size_t *curIdx);
Error* parseStmt(ASTNode *parent, Token **tokens, size_t tokensLen, size_t *curIdx);
Error* parseWhile(ASTNode *parent, Token **tokens, size_t tokensLen, size_t *curIdx);
Error* parseBreak(ASTNode *parent, Token **tokens, size_t tokensLen, size_t *curIdx);
Error* parseContinue(ASTNode *parent, Token **tokens, size_t tokensLen, size_t *curIdx);
Error* parseReturn(ASTNode *parent, Token **tokens, size_t tokensLen, size_t *curIdx);
Error* parseIfStmt(ASTNode *parent, Token **tokens, size_t tokensLen, size_t *curIdx);
Error* parseElseIfStmt(ASTNode *parent, Token **tokens, size_t tokensLen, size_t *curIdx);
Error* parseElseStmt(ASTNode *parent, Token **tokens, size_t tokensLen, size_t *curIdx);
Error* parseExprStmt(ASTNode *parent, Token **tokens, size_t tokensLen, size_t *curIdx);
Error* parsePrntStmt(ASTNode *parent, Token **tokens, size_t tokensLen, size_t *curIdx);
Error* parseExpr(ASTNode *parent, Token **tokens, size_t tokensLen, size_t *curIdx);
Error* parseFnExpr(ASTNode *parent, Token **tokens, size_t tokensLen, size_t *curIdx);
Error* parseArgList(ASTNode *parent, Token **tokens, size_t tokensLen, size_t *curIdx);
Error* parseArg(ASTNode *parent, Token **tokens, size_t tokensLen, size_t *curIdx);
Error* parseOrExpr(ASTNode *parent, Token **tokens, size_t tokensLen, size_t *curIdx);
Error* parseOrExprR(ASTNode *parent, Token **tokens, size_t tokensLen, size_t *curIdx);
Error* parseAndExpr(ASTNode *parent, Token **tokens, size_t tokensLen, size_t *curIdx);
Error* parseAndExprR(ASTNode *parent, Token **tokens, size_t tokensLen, size_t *curIdx);
Error* parseLogUnary(ASTNode *parent, Token **tokens, size_t tokensLen, size_t *curIdx);
Error* parseEquality(ASTNode *parent, Token **tokens, size_t tokensLen, size_t *curIdx);
Error* parseEqualityR(ASTNode *parent, Token **tokens, size_t tokensLen, size_t *curIdx);
Error* parseComparison(ASTNode *parent, Token **tokens, size_t tokensLen, size_t *curIdx);
Error* parseComparisonR(ASTNode *parent, Token **tokens, size_t tokensLen, size_t *curIdx);
Error* parseSum(ASTNode *parent, Token **tokens, size_t tokensLen, size_t *curIdx);
Error* parseSumR(ASTNode *parent, Token **tokens, size_t tokensLen, size_t *curIdx);
Error* parseTerm(ASTNode *parent, Token **tokens, size_t tokensLen, size_t *curIdx);
Error* parseTermR(ASTNode *parent, Token **tokens, size_t tokensLen, size_t *curIdx);
Error* parseUnary(ASTNode *parent, Token **tokens, size_t tokensLen, size_t *curIdx);
Error* parsePower(ASTNode *parent, Token **tokens, size_t tokensLen, size_t *curIdx);
Error* parsePrimary(ASTNode *parent, Token **tokens, size_t tokensLen, size_t *curIdx);
Error* parseTerminal(ASTNode *parent, Token **tokens, size_t tokensLen, size_t *curIdx, TokenType expectedTokenType);
Error* parseLine(ASTNode *parent, Token **tokens, size_t tokensLen, size_t *curIdx);

/*
Performs syntax analysis on `tokens`, storing the AST in `root`.
*/
Error* parse(ASTNode *root, Token **tokens, size_t tokensCount);

#endif
