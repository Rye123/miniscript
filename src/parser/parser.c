#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define _DEBUG_PARSER_ 0
#include "symbol.h"
#include "parser.h"

// Returns EOF if idx exceeds the token length, otherwise returns the token.
Token *getToken(Token **tokens, size_t tokensLen, size_t idx)
{
    if (idx >= tokensLen)
	return *(tokens + tokensLen - 1);
    return *(tokens + idx);
}

void printParse(char* str, Token **tokens, size_t *curIdx)
{
    if (_DEBUG_PARSER_)
	printf("%s: Token at index %lu, type %s, lexeme \"%s\"\n", str, *curIdx, TokenTypeString[tokens[*curIdx]->type], tokens[*curIdx]->lexeme);
}

void parseTerminal(ASTNode *parent, Token **tokens, size_t tokensLen, size_t *curIdx, TokenType expectedTokenType)
{
    printParse("parseTerminal", tokens, curIdx);
    Token *tok = getToken(tokens, tokensLen, *curIdx);
    if (expectedTokenType != tok->type) {
	printf("SYNTAX ERROR: Unexpected token type %s, expected %s.\n", TokenTypeString[tok->type], TokenTypeString[expectedTokenType]);
    }
    astnode_addChild(parent, SYM_TERMINAL, tok);
    *curIdx = *curIdx + 1;
}

void parsePrimary(ASTNode *parent, Token **tokens, size_t tokensLen, size_t *curIdx)
{
    printParse("parsePrimary", tokens, curIdx);
    ASTNode *self = astnode_new(SYM_PRIMARY, NULL);
    // 1. Parse lookahead
    Token *lookahead = getToken(tokens, tokensLen, *curIdx);
    switch (lookahead->type) {
    case TOKEN_PAREN_L:
	parseTerminal(self, tokens, tokensLen, curIdx, TOKEN_PAREN_L);
	parseExpr(self, tokens, tokensLen, curIdx);
	parseTerminal(self, tokens, tokensLen, curIdx, TOKEN_PAREN_R);
	break;
    case TOKEN_IDENTIFIER:
    case TOKEN_STRING:
    case TOKEN_NUMBER:
        parseTerminal(self, tokens, tokensLen, curIdx, lookahead->type);
	break;
    default:
	printf("SYNTAX ERROR: Unexpected token type %s for parsePrimary.\n", TokenTypeString[lookahead->type]);
    }

    astnode_addChildNode(parent, self);
}

void parsePower(ASTNode *parent, Token **tokens, size_t tokensLen, size_t *curIdx)
{
    printParse("parsePower", tokens, curIdx);
    ASTNode *self = astnode_new(SYM_POWER, NULL);
    // 1. Parse PRIMARY
    parsePrimary(self, tokens, tokensLen, curIdx);

    // 2. Parse lookahead (INVARIANT: idx now points to after the first comparison)
    Token *lookahead = getToken(tokens, tokensLen, *curIdx);
    switch (lookahead->type) {
    case TOKEN_CARET:
	parseTerminal(self, tokens, tokensLen, curIdx, TOKEN_CARET);
	parseUnary(self, tokens, tokensLen, curIdx);
	break;
    default:
	break;
    }
    
    astnode_addChildNode(parent, self);
}

void parseUnary(ASTNode *parent, Token **tokens, size_t tokensLen, size_t *curIdx)
{
    printParse("parseUnary", tokens, curIdx);
    ASTNode *self = astnode_new(SYM_UNARY, NULL);
    Token *lookahead = getToken(tokens, tokensLen, *curIdx);
    switch (lookahead->type) {
    case TOKEN_PLUS:
    case TOKEN_MINUS:
	parseTerminal(self, tokens, tokensLen, curIdx, lookahead->type);
	parseUnary(self, tokens, tokensLen, curIdx);
	break;
    default:
	parsePower(self, tokens, tokensLen, curIdx);
	break;
    }

    astnode_addChildNode(parent, self);
}

void parseTermR(ASTNode *parent, Token **tokens, size_t tokensLen, size_t *curIdx)
{
    printParse("parseTermR", tokens, curIdx);
    ASTNode *self = astnode_new(SYM_TERM_R, NULL);
    Token *lookahead = getToken(tokens, tokensLen, *curIdx);
    switch (lookahead->type) {
    case TOKEN_STAR:
    case TOKEN_SLASH:
    case TOKEN_PERCENT:
	parseTerminal(self, tokens, tokensLen, curIdx, lookahead->type);
	parseTerm(self, tokens, tokensLen, curIdx);
	parseTermR(self, tokens, tokensLen, curIdx);
	break;
    default:
	break;
    }
    
    astnode_addChildNode(parent, self);
}

void parseTerm(ASTNode *parent, Token **tokens, size_t tokensLen, size_t *curIdx)
{
    printParse("parseTerm", tokens, curIdx);
    ASTNode *self = astnode_new(SYM_TERM, NULL);
    parseUnary(self, tokens, tokensLen, curIdx);
    parseTermR(self, tokens, tokensLen, curIdx);

    astnode_addChildNode(parent, self);
}

void parseSumR(ASTNode *parent, Token **tokens, size_t tokensLen, size_t *curIdx)
{
    printParse("parseSumR", tokens, curIdx);
    ASTNode *self = astnode_new(SYM_SUM_R, NULL);
    Token *lookahead = getToken(tokens, tokensLen, *curIdx);
    
    switch (lookahead->type) {
    case TOKEN_PLUS:
    case TOKEN_MINUS:
	parseTerminal(self, tokens, tokensLen, curIdx, lookahead->type);
	parseSum(self, tokens, tokensLen, curIdx);
	parseSumR(self, tokens, tokensLen, curIdx);
	break;
    default:
	break;
    }
    
    astnode_addChildNode(parent, self);
}

void parseSum(ASTNode *parent, Token **tokens, size_t tokensLen, size_t *curIdx)
{
    printParse("parseSum", tokens, curIdx);
    ASTNode *self = astnode_new(SYM_SUM, NULL);
    parseTerm(self, tokens, tokensLen, curIdx);
    parseSumR(self, tokens, tokensLen, curIdx);

    astnode_addChildNode(parent, self);
}

void parseComparisonR(ASTNode *parent, Token **tokens, size_t tokensLen, size_t *curIdx)
{
    printParse("parseComparisonR", tokens, curIdx);
    ASTNode *self = astnode_new(SYM_COMPARISON_R, NULL);
    Token *lookahead = getToken(tokens, tokensLen, *curIdx);
    switch (lookahead->type) {
    case TOKEN_GREATER:
    case TOKEN_GREATER_EQUAL:
    case TOKEN_LESS:
    case TOKEN_LESS_EQUAL:
	parseTerminal(self, tokens, tokensLen, curIdx, lookahead->type);
	parseComparison(self, tokens, tokensLen, curIdx);
	parseComparisonR(self, tokens, tokensLen, curIdx);
	break;
    default:
	break;
    }
    
    astnode_addChildNode(parent, self);
}

void parseComparison(ASTNode *parent, Token **tokens, size_t tokensLen, size_t *curIdx)
{
    printParse("parseComparison", tokens, curIdx);
    ASTNode *self = astnode_new(SYM_COMPARISON, NULL);
    parseSum(self, tokens, tokensLen, curIdx);
    parseComparisonR(self, tokens, tokensLen, curIdx);

    astnode_addChildNode(parent, self);
}

void parseEqualityR(ASTNode *parent, Token **tokens, size_t tokensLen, size_t *curIdx)
{
    printParse("parseEqualityR", tokens, curIdx);
    ASTNode *self = astnode_new(SYM_EQUALITY_R, NULL);
    Token *lookahead = getToken(tokens, tokensLen, *curIdx);
    switch (lookahead->type) {
    case TOKEN_EQUAL_EQUAL:
    case TOKEN_BANG_EQUAL:
	parseTerminal(self, tokens, tokensLen, curIdx, lookahead->type);
	parseEquality(self, tokens, tokensLen, curIdx);
	parseEqualityR(self, tokens, tokensLen, curIdx);
	break;
    default:
	break;
    }
    astnode_addChildNode(parent, self);
}

void parseEquality(ASTNode *parent, Token **tokens, size_t tokensLen, size_t *curIdx)
{
    printParse("parseEquality", tokens, curIdx);
    ASTNode *self = astnode_new(SYM_EQUALITY, NULL);
    parseComparison(self, tokens, tokensLen, curIdx);
    parseEqualityR(self, tokens, tokensLen, curIdx);

    astnode_addChildNode(parent, self);
}

// Parse an expression of tokens, starting from position `start`.
void parseExpr(ASTNode *parent, Token **tokens, size_t tokensLen, size_t *curIdx)
{
    printParse("parseExpr", tokens, curIdx);
    ASTNode *self = astnode_new(SYM_EXPR, NULL);
    parseEquality(self, tokens, tokensLen, curIdx);
    astnode_addChildNode(parent, self);
}

void parsePrntStmt(ASTNode *parent, Token **tokens, size_t tokensLen, size_t *curIdx)
{
    printParse("parsePrntStmt", tokens, curIdx);
    ASTNode *self = astnode_new(SYM_PRNT_STMT, NULL);
    parseTerminal(self, tokens, tokensLen, curIdx, TOKEN_PRINT);
    parseExpr(self, tokens, tokensLen, curIdx);
    parseTerminal(self, tokens, tokensLen, curIdx, TOKEN_NL); //TODO: ENSURE a program ENDS with NL and then EOF.

    astnode_addChildNode(parent, self);
}

void parseExprStmt(ASTNode *parent, Token **tokens, size_t tokensLen, size_t *curIdx)
{
    printParse("parseExprStmt", tokens, curIdx);
    ASTNode *self = astnode_new(SYM_EXPR_STMT, NULL);
    parseExpr(self, tokens, tokensLen, curIdx);
    parseTerminal(self, tokens, tokensLen, curIdx, TOKEN_NL); //TODO: ENSURE a program ENDS with NL and then EOF.

    astnode_addChildNode(parent, self);
}

void parseStmt(ASTNode *parent, Token **tokens, size_t tokensLen, size_t *curIdx)
{
    printParse("parseStmt", tokens, curIdx);
    ASTNode *self = astnode_new(SYM_STMT, NULL);
    Token *lookahead = getToken(tokens, tokensLen, *curIdx);
    if (lookahead->type == TOKEN_PRINT) {
	parsePrntStmt(self, tokens, tokensLen, curIdx);
    } else {
	parseExprStmt(self, tokens, tokensLen, curIdx);
    }

    astnode_addChildNode(parent, self);
}

void parseAsmt(ASTNode *parent, Token **tokens, size_t tokensLen, size_t *curIdx)
{
    printParse("parseAsmt", tokens, curIdx);
    ASTNode *self = astnode_new(SYM_ASMT, NULL);
    Token *lookahead = getToken(tokens, tokensLen, *curIdx);
    Token *lookahead2 = getToken(tokens, tokensLen, (*curIdx) + 1);
    if (lookahead->type != TOKEN_IDENTIFIER || lookahead2->type != TOKEN_EQUAL) {
	printf("Invalid assignment parsing.\n");
	exit(1);
    }
    
    parseTerminal(self, tokens, tokensLen, curIdx, TOKEN_IDENTIFIER);
    parseTerminal(self, tokens, tokensLen, curIdx, TOKEN_EQUAL);
    parseExpr(self, tokens, tokensLen, curIdx);
    parseTerminal(self, tokens, tokensLen, curIdx, TOKEN_NL);

    astnode_addChildNode(parent, self);
}

void parseLine(ASTNode *parent, Token **tokens, size_t tokensLen, size_t *curIdx)
{
    printParse("parseLine", tokens, curIdx);
    ASTNode *self = astnode_new(SYM_LINE, NULL);

    // If lookahead is an IDENTIFIER, we check if the follow-up is an equal sign. If so, then it is an assignment.
    Token *lookahead = getToken(tokens, tokensLen, *curIdx);
    Token *lookahead2 = getToken(tokens, tokensLen, (*curIdx) + 1);
    if (lookahead->type == TOKEN_IDENTIFIER && lookahead2->type == TOKEN_EQUAL)
	parseAsmt(self, tokens, tokensLen, curIdx);
    else
	parseStmt(self, tokens, tokensLen, curIdx);
    
    astnode_addChildNode(parent, self);
}


void parse(ASTNode *root, Token **tokens, size_t tokenCount)
{
    // Identify length of tokens array
    size_t tokensLen = tokenCount;

    // Create the tree
    size_t curIdx = 0;
    Token *lookahead;
    while (1) {
	lookahead = getToken(tokens, tokensLen, curIdx);
	if (lookahead->type == TOKEN_EOF)
	    break;
	parseLine(root, tokens, tokensLen, &curIdx);
    }
    return;
}
