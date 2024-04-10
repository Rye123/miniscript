#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define _DEBUG_PARSER_ 0
#include "../error/error.h"
#include "../logger/logger.h"
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
        log_message(&executionLogger, "%s: Token at index %lu, type %s, lexeme \"%s\"\n", str, *curIdx, TokenTypeString[tokens[*curIdx]->type], tokens[*curIdx]->lexeme);
}

Error *getParseError(ErrorType type, Token **tokens, size_t tokensLen, size_t *curIdx)
{
    // We use the PREVIOUS token if this one is a newline
    size_t idx = *curIdx;
    Token *tok = getToken(tokens, tokensLen, idx);
    while (tok->type == TOKEN_NL || tok->type == TOKEN_EOF) {
        idx--;
        if (idx < 0)
            break;
        tok = getToken(tokens, tokensLen, idx);
    }
    Error *err = error_new(type, tok->lineNum, tok->colNum);
    return err;
}

Error *parseTerminal(ASTNode *parent, Token **tokens, size_t tokensLen, size_t *curIdx, TokenType expectedTokenType)
{
    printParse("parseTerminal", tokens, curIdx);
    Token *tok = getToken(tokens, tokensLen, *curIdx);

    if (expectedTokenType != tok->type) {
        Error *err = getParseError(ERR_SYNTAX, tokens, tokensLen, curIdx);
        snprintf(err->message, MAX_ERRMSG_LEN, "Expected token %s, instead got %s.", TokenTypeString[expectedTokenType], TokenTypeString[tok->type]);
        return err;
    }
    astnode_addChild(parent, SYM_TERMINAL, tok);
    *curIdx = *curIdx + 1;
    return NULL;
}

Error *parsePrimary(ASTNode *parent, Token **tokens, size_t tokensLen, size_t *curIdx)
{
    printParse("parsePrimary", tokens, curIdx);
    ASTNode *self = astnode_new(SYM_PRIMARY, NULL);
    // 1. Parse lookahead
    Token *lookahead = getToken(tokens, tokensLen, *curIdx);
    switch (lookahead->type) {
    case TOKEN_PAREN_L: {
        parseTerminal(self, tokens, tokensLen, curIdx, TOKEN_PAREN_L);
        Error *exprError = parseExpr(self, tokens, tokensLen, curIdx);
        Error *hasEOFError = parseTerminal(self, tokens, tokensLen, curIdx, TOKEN_PAREN_R);
        if (exprError) {
            error_free(exprError);
            return exprError;
        }
        if (hasEOFError) {
            Error *eofError = getParseError(ERR_SYNTAX, tokens, tokensLen, curIdx);     
            snprintf(eofError->message, MAX_ERRMSG_LEN, "Expected a closing parentheses.");
            error_free(hasEOFError);
            return eofError;
        }
        break;
    }
    case TOKEN_IDENTIFIER:
    case TOKEN_STRING:
    case TOKEN_NUMBER:
    case TOKEN_NULL:
    case TOKEN_TRUE:
    case TOKEN_FALSE: {
        Error *err = parseTerminal(self, tokens, tokensLen, curIdx, lookahead->type);
        if (err)
            return err;
        break;
    }
    default: {
        Error *err = getParseError(ERR_SYNTAX, tokens, tokensLen, curIdx);
        snprintf(err->message, MAX_ERRMSG_LEN, "Expecting either a terminal or a starting parentheses, instead got token %s.", TokenTypeString[lookahead->type]);
        return err;
    }
    }

    astnode_addChildNode(parent, self);
    return NULL;
}

Error *parsePower(ASTNode *parent, Token **tokens, size_t tokensLen, size_t *curIdx)
{
    printParse("parsePower", tokens, curIdx);
    ASTNode *self = astnode_new(SYM_POWER, NULL);
    // 1. Parse PRIMARY
    Error *priErr = parsePrimary(self, tokens, tokensLen, curIdx);
    if (priErr)
        return priErr;

    // 2. Parse lookahead (INVARIANT: idx now points to after the first comparison)
    Token *lookahead = getToken(tokens, tokensLen, *curIdx);
    switch (lookahead->type) {
    case TOKEN_CARET: {
        parseTerminal(self, tokens, tokensLen, curIdx, TOKEN_CARET);
        Error *unaryErr = parseUnary(self, tokens, tokensLen, curIdx);
        if (unaryErr)
            return unaryErr;
        break;
    }
    default:
        break;
    }
    
    astnode_addChildNode(parent, self);
    return NULL;
}

Error *parseUnary(ASTNode *parent, Token **tokens, size_t tokensLen, size_t *curIdx)
{
    printParse("parseUnary", tokens, curIdx);
    ASTNode *self = astnode_new(SYM_UNARY, NULL);
    Token *lookahead = getToken(tokens, tokensLen, *curIdx);
    switch (lookahead->type) {
    case TOKEN_PLUS:
    case TOKEN_MINUS: {
        parseTerminal(self, tokens, tokensLen, curIdx, lookahead->type);
        Error *unaryErr = parseUnary(self, tokens, tokensLen, curIdx);
        if (unaryErr)
            return unaryErr;
        break;
    }
    default: {
        Error *powerErr = parsePower(self, tokens, tokensLen, curIdx);
        if (powerErr)
            return powerErr;
        break;
    }
    }

    astnode_addChildNode(parent, self);
    return NULL;
}

Error *parseTermR(ASTNode *parent, Token **tokens, size_t tokensLen, size_t *curIdx)
{
    printParse("parseTermR", tokens, curIdx);
    ASTNode *self = astnode_new(SYM_TERM_R, NULL);
    Token *lookahead = getToken(tokens, tokensLen, *curIdx);
    switch (lookahead->type) {
    case TOKEN_STAR:
    case TOKEN_SLASH:
    case TOKEN_PERCENT: {
        parseTerminal(self, tokens, tokensLen, curIdx, lookahead->type);
        Error *termErr = parseTerm(self, tokens, tokensLen, curIdx);
        if (termErr)
            return termErr;
        Error *termRErr = parseTermR(self, tokens, tokensLen, curIdx);
        if (termRErr)
            return termRErr;
        break;
    }
    default:
        break;
    }
    
    astnode_addChildNode(parent, self);
    return NULL;
}

Error *parseTerm(ASTNode *parent, Token **tokens, size_t tokensLen, size_t *curIdx)
{
    printParse("parseTerm", tokens, curIdx);
    ASTNode *self = astnode_new(SYM_TERM, NULL);
    Error *unaryErr = parseUnary(self, tokens, tokensLen, curIdx);
    if (unaryErr)
        return unaryErr;
    Error *termRErr = parseTermR(self, tokens, tokensLen, curIdx);
    if (termRErr)
        return termRErr;

    astnode_addChildNode(parent, self);
    return NULL;
}

Error *parseSumR(ASTNode *parent, Token **tokens, size_t tokensLen, size_t *curIdx)
{
    printParse("parseSumR", tokens, curIdx);
    ASTNode *self = astnode_new(SYM_SUM_R, NULL);
    Token *lookahead = getToken(tokens, tokensLen, *curIdx);
    
    switch (lookahead->type) {
    case TOKEN_PLUS:
    case TOKEN_MINUS:
        parseTerminal(self, tokens, tokensLen, curIdx, lookahead->type);
        Error *sumErr = parseSum(self, tokens, tokensLen, curIdx);
        if (sumErr)
            return sumErr;
        Error *sumRErr = parseSumR(self, tokens, tokensLen, curIdx);
        if (sumRErr)
            return sumRErr;
        break;
    default:
        break;
    }
    
    astnode_addChildNode(parent, self);
    return NULL;
}

Error *parseSum(ASTNode *parent, Token **tokens, size_t tokensLen, size_t *curIdx)
{
    printParse("parseSum", tokens, curIdx);
    ASTNode *self = astnode_new(SYM_SUM, NULL);
    Error *termErr = parseTerm(self, tokens, tokensLen, curIdx);
    if (termErr)
        return termErr;
    Error *sumRErr = parseSumR(self, tokens, tokensLen, curIdx);
    if (sumRErr)
        return sumRErr;

    astnode_addChildNode(parent, self);
    return NULL;
}

Error *parseComparisonR(ASTNode *parent, Token **tokens, size_t tokensLen, size_t *curIdx)
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
        Error *compErr = parseComparison(self, tokens, tokensLen, curIdx);
        if (compErr)
            return compErr;
        Error *compRErr = parseComparisonR(self, tokens, tokensLen, curIdx);
        if (compRErr)
            return compRErr;
        break;
    default:
        break;
    }
    
    astnode_addChildNode(parent, self);
    return NULL;
}

Error *parseComparison(ASTNode *parent, Token **tokens, size_t tokensLen, size_t *curIdx)
{
    printParse("parseComparison", tokens, curIdx);
    ASTNode *self = astnode_new(SYM_COMPARISON, NULL);
    Error *sumErr = parseSum(self, tokens, tokensLen, curIdx);
    if (sumErr)
        return sumErr;
    Error *compRErr = parseComparisonR(self, tokens, tokensLen, curIdx);
    if (compRErr)
        return compRErr;

    astnode_addChildNode(parent, self);
    return NULL;
}

Error *parseEqualityR(ASTNode *parent, Token **tokens, size_t tokensLen, size_t *curIdx)
{
    printParse("parseEqualityR", tokens, curIdx);
    ASTNode *self = astnode_new(SYM_EQUALITY_R, NULL);
    Token *lookahead = getToken(tokens, tokensLen, *curIdx);
    switch (lookahead->type) {
    case TOKEN_EQUAL_EQUAL:
    case TOKEN_BANG_EQUAL:
        parseTerminal(self, tokens, tokensLen, curIdx, lookahead->type);
        Error *eqErr = parseEquality(self, tokens, tokensLen, curIdx);
        if (eqErr)
            return eqErr;
        Error *eqRErr = parseEqualityR(self, tokens, tokensLen, curIdx);
        if (eqRErr)
            return eqRErr;
        break;
    default:
        break;
    }
    astnode_addChildNode(parent, self);
    return NULL;
}

Error *parseEquality(ASTNode *parent, Token **tokens, size_t tokensLen, size_t *curIdx)
{
    printParse("parseEquality", tokens, curIdx);
    ASTNode *self = astnode_new(SYM_EQUALITY, NULL);
    Error *compErr = parseComparison(self, tokens, tokensLen, curIdx);
    if (compErr)
        return compErr;
    Error *eqRErr = parseEqualityR(self, tokens, tokensLen, curIdx);
    if (eqRErr)
        return eqRErr;

    astnode_addChildNode(parent, self);
    return NULL;
}

Error *parseLogUnary(ASTNode *parent, Token **tokens, size_t tokensLen, size_t *curIdx)
{
    printParse("parseLogUnary", tokens, curIdx);
    ASTNode *self = astnode_new(SYM_LOG_UNARY, NULL);
    Token *lookahead = getToken(tokens, tokensLen, *curIdx);
    Error *err = NULL;
    if (lookahead->type == TOKEN_NOT) {
        parseTerminal(self, tokens, tokensLen, curIdx, lookahead->type);
        err = parseLogUnary(self, tokens, tokensLen, curIdx);
    } else {
        err = parseEquality(self, tokens, tokensLen, curIdx);
    }
    if (err)
        return err;
    astnode_addChildNode(parent, self);
    return NULL;
}

Error *parseAndExprR(ASTNode *parent, Token **tokens, size_t tokensLen, size_t *curIdx)
{
    printParse("parseAndExprR", tokens, curIdx);
    ASTNode *self = astnode_new(SYM_AND_EXPR_R, NULL);
    Token *lookahead = getToken(tokens, tokensLen, *curIdx);
    Error *err = NULL;
    if (lookahead->type == TOKEN_AND) {
        parseTerminal(self, tokens, tokensLen, curIdx, lookahead->type);
        err = parseAndExpr(self, tokens, tokensLen, curIdx);
        if (err)
            return err;
        err = parseAndExprR(self, tokens, tokensLen, curIdx);
        if (err)
            return err;
    }
    astnode_addChildNode(parent, self);
    return NULL;
}

Error *parseAndExpr(ASTNode *parent, Token **tokens, size_t tokensLen, size_t *curIdx)
{
    printParse("parseAndExpr", tokens, curIdx);
    ASTNode *self = astnode_new(SYM_AND_EXPR, NULL);
    Error *err = NULL;
    err = parseLogUnary(self, tokens, tokensLen, curIdx);
    if (err)
        return err;
    err = parseAndExprR(self, tokens, tokensLen, curIdx);
    if (err)
        return err;
    astnode_addChildNode(parent, self);
    return NULL;
}

Error *parseOrExprR(ASTNode *parent, Token **tokens, size_t tokensLen, size_t *curIdx)
{
    printParse("parseOrExprR", tokens, curIdx);
    ASTNode *self = astnode_new(SYM_OR_EXPR_R, NULL);
    Token *lookahead = getToken(tokens, tokensLen, *curIdx);
    Error *err = NULL;
    if (lookahead->type == TOKEN_OR) {
        parseTerminal(self, tokens, tokensLen, curIdx, lookahead->type);
        err = parseOrExpr(self, tokens, tokensLen, curIdx);
        if (err)
            return err;
        err = parseOrExprR(self, tokens, tokensLen, curIdx);
        if (err)
            return err;
    }
    astnode_addChildNode(parent, self);
    return NULL;
}

Error *parseOrExpr(ASTNode *parent, Token **tokens, size_t tokensLen, size_t *curIdx)
{
    printParse("parseOrExpr", tokens, curIdx);
    ASTNode *self = astnode_new(SYM_OR_EXPR, NULL);
    Error *err = NULL;
    err = parseAndExpr(self, tokens, tokensLen, curIdx);
    if (err)
        return err;
    err = parseOrExprR(self, tokens, tokensLen, curIdx);
    if (err)
        return err;
    astnode_addChildNode(parent, self);
    return NULL;
}

Error* parseArg(ASTNode *parent, Token **tokens, size_t tokensLen, size_t *curIdx)
{
    printParse("parseArg", tokens, curIdx);
    ASTNode *self = astnode_new(SYM_ARG, NULL);
    Token *lookahead2 = getToken(tokens, tokensLen, *curIdx + 1);
    Error *err = NULL;
    if (lookahead2->type == TOKEN_EQUAL) {
        // IDENTIFIER = STRING or NUMBER or NULL
        err = parseTerminal(self, tokens, tokensLen, curIdx, TOKEN_IDENTIFIER);
        if (err) {
            astnode_free(self);
            return err;
        }
        parseTerminal(self, tokens, tokensLen, curIdx, TOKEN_EQUAL);
        Token *lookahead = getToken(tokens, tokensLen, *curIdx);
        switch (lookahead->type) {
        case TOKEN_STRING: parseTerminal(self, tokens, tokensLen, curIdx, TOKEN_STRING); break;
        case TOKEN_NUMBER: parseTerminal(self, tokens, tokensLen, curIdx, TOKEN_NUMBER); break;
        case TOKEN_NULL: parseTerminal(self, tokens, tokensLen, curIdx, TOKEN_NULL); break;
        default:
            // error
            err = getParseError(ERR_SYNTAX, tokens, tokensLen, curIdx);
            snprintf(err->message, MAX_ERRMSG_LEN, "Invalid function parameter definition, should be \"arg\" or \"arg = value\", where value is a string, number or null.");
            astnode_free(self);
            return err;
            break;
        }
    } else {
        // IDENTIFIER
        err = parseTerminal(self, tokens, tokensLen, curIdx, TOKEN_IDENTIFIER);
        if (err) {
            astnode_free(self);
            return err;
        }
    }
    astnode_addChildNode(parent, self);
    return NULL;
}

Error* parseArgList(ASTNode *parent, Token **tokens, size_t tokensLen, size_t *curIdx)
{
    printParse("parseArgList", tokens, curIdx);
    ASTNode *self = astnode_new(SYM_ARG_LIST, NULL);
    Error *err = parseArg(self, tokens, tokensLen, curIdx);
    if (err) {
        astnode_free(self);
        return err;
    }
    
    Token *lookahead = getToken(tokens, tokensLen, *curIdx);
    while (lookahead->type == TOKEN_COMMA) {
        parseTerminal(self, tokens, tokensLen, curIdx, TOKEN_COMMA);
        err = parseArg(self, tokens, tokensLen, curIdx);
        if (err) {
            astnode_free(self);
            return err;
        }
        lookahead = getToken(tokens, tokensLen, *curIdx);
    }
    astnode_addChildNode(parent, self);
    return NULL;
}

Error *parseReturn(ASTNode *parent, Token **tokens, size_t tokensLen, size_t *curIdx)
{
    printParse("parseReturn", tokens, curIdx);
    ASTNode *self = astnode_new(SYM_CONTINUE, NULL);
    Error *err = NULL;
    err = parseTerminal(self, tokens, tokensLen, curIdx, TOKEN_RETURN);
    if (err) {
        astnode_free(self);
        return err;
    }

    Token *lookahead = getToken(tokens, tokensLen, *curIdx);
    if (lookahead->type != TOKEN_NL) {
        err = parseExpr(self, tokens, tokensLen, curIdx);
        if (err) {
            astnode_free(self);
            return err;
        }
    }
    
    err = parseTerminal(self, tokens, tokensLen, curIdx, TOKEN_NL);
    if (err) {
        astnode_free(self);
        return err;
    }
    astnode_addChildNode(parent, self);
    return NULL;
}

Error* parseFnExpr(ASTNode *parent, Token **tokens, size_t tokensLen, size_t *curIdx)
{
    printParse("parseFnExpr", tokens, curIdx);
    ASTNode *self = astnode_new(SYM_FN_EXPR, NULL);
    Error *err = NULL;

    Token *lookahead = getToken(tokens, tokensLen, *curIdx);
    Token *lookahead2 = getToken(tokens, tokensLen, *curIdx + 1);

    // Parse function(
    if (lookahead->type != TOKEN_FUNCTION || lookahead2->type != TOKEN_PAREN_L) {
        printf("%s, %s\n", TokenTypeString[lookahead->type], TokenTypeString[lookahead2->type]);
        Error *fnError = getParseError(ERR_SYNTAX, tokens, tokensLen, curIdx);
        snprintf(fnError->message, MAX_ERRMSG_LEN, "Function definition should start with \"function\" and left parentheses: function(arg1, arg2, ...)");
        astnode_free(self);
        return fnError;
    }
    parseTerminal(self, tokens, tokensLen, curIdx, TOKEN_FUNCTION);
    parseTerminal(self, tokens, tokensLen, curIdx, TOKEN_PAREN_L);

    // Parse arg list
    err = parseArgList(self, tokens, tokensLen, curIdx);
    if (err)
        return err;

    // Parse )\n
    lookahead = getToken(tokens, tokensLen, *curIdx);
    lookahead2 = getToken(tokens, tokensLen, *curIdx + 1);
    if (lookahead->type != TOKEN_PAREN_R || lookahead2->type != TOKEN_NL) {
        Error *fnError = getParseError(ERR_SYNTAX, tokens, tokensLen, curIdx);
        snprintf(fnError->message, MAX_ERRMSG_LEN, "Function definition should end with right parentheses and a new line: function(arg1, arg2, ...)");
        astnode_free(self);
        return fnError;
    }
    parseTerminal(self, tokens, tokensLen, curIdx, TOKEN_PAREN_R);
    parseTerminal(self, tokens, tokensLen, curIdx, TOKEN_NL);

    // Parse Block
    ASTNode *block = astnode_new(SYM_BLOCK, NULL);
    lookahead = getToken(tokens, tokensLen, *curIdx);
    while (lookahead->type != TOKEN_END) {
        if (lookahead->type == TOKEN_EOF) {
            Error *eofError = getParseError(ERR_SYNTAX_EOF, tokens, tokensLen, curIdx);
            snprintf(eofError->message, MAX_ERRMSG_LEN, "Function block not terminated with \"end function\".");
            astnode_free(self);
            astnode_free(block);
            return eofError;
        }
        Error *lineErr = parseLine(block, tokens, tokensLen, curIdx);
        if (lineErr)
            return lineErr;
        lookahead = getToken(tokens, tokensLen, *curIdx);
    }
    astnode_addChildNode(self, block);

    // End
    lookahead = getToken(tokens, tokensLen, *curIdx);
    lookahead2 = getToken(tokens, tokensLen, *curIdx + 1);
    if (lookahead->type != TOKEN_END || lookahead2->type != TOKEN_FUNCTION) {
        Error *eofError = getParseError(ERR_SYNTAX_EOF, tokens, tokensLen, curIdx);
        snprintf(eofError->message, MAX_ERRMSG_LEN, "Function block not terminated with \"end function\"");
        astnode_free(self);
        astnode_free(block);
    }
    parseTerminal(self, tokens, tokensLen, curIdx, TOKEN_END);
    parseTerminal(self, tokens, tokensLen, curIdx, TOKEN_FUNCTION);

    astnode_addChildNode(parent, self);
    return NULL;
}

Error *parseExpr(ASTNode *parent, Token **tokens, size_t tokensLen, size_t *curIdx)
{
    printParse("parseExpr", tokens, curIdx);
    ASTNode *self = astnode_new(SYM_EXPR, NULL);
    Token *lookahead = getToken(tokens, tokensLen, *curIdx);
    Error *err = NULL;

    if (lookahead->type == TOKEN_FUNCTION)
        err = parseFnExpr(self, tokens, tokensLen, curIdx);
    else
        err = parseOrExpr(self, tokens, tokensLen, curIdx);
    
    if (err)
        return err;
    astnode_addChildNode(parent, self);
    return NULL;
}

Error *parsePrntStmt(ASTNode *parent, Token **tokens, size_t tokensLen, size_t *curIdx)
{
    printParse("parsePrntStmt", tokens, curIdx);
    ASTNode *self = astnode_new(SYM_PRNT_STMT, NULL);
    
    Error *err = NULL;
    err = parseTerminal(self, tokens, tokensLen, curIdx, TOKEN_PRINT);
    if (err)
        return err;

    Token *lookahead = getToken(tokens, tokensLen, *curIdx);
    if (lookahead->type == TOKEN_NL) {
        astnode_addChildNode(parent, self);
        return NULL;
    }
    
    err = parseExpr(self, tokens, tokensLen, curIdx);
    if (err)
        return err;
    err = parseTerminal(self, tokens, tokensLen, curIdx, TOKEN_NL);
    if (err)
        return err;
    astnode_addChildNode(parent, self);
    return NULL;
}

Error *parseExprStmt(ASTNode *parent, Token **tokens, size_t tokensLen, size_t *curIdx)
{
    printParse("parseExprStmt", tokens, curIdx);
    ASTNode *self = astnode_new(SYM_EXPR_STMT, NULL);
    Token *lookahead = getToken(tokens, tokensLen, *curIdx);
    if (lookahead->type == TOKEN_NL) {
        astnode_addChildNode(parent, self);
        return NULL;
    }
    
    Error *err = NULL;
    err = parseExpr(self, tokens, tokensLen, curIdx);
    if (err)
        return err;
    err = parseTerminal(self, tokens, tokensLen, curIdx, TOKEN_NL);
    if (err)
        return err;
    astnode_addChildNode(parent, self);
    return NULL;
}

Error *parseElseStmt(ASTNode *parent, Token **tokens, size_t tokensLen, size_t *curIdx){
    printParse("parseElseStmt", tokens, curIdx);
    ASTNode *self = astnode_new(SYM_ELSE, NULL);
    Error *err = NULL;
    err = parseTerminal(self, tokens, tokensLen, curIdx, TOKEN_ELSE);
    if (err)
        return err;
    parseTerminal(self, tokens, tokensLen, curIdx, TOKEN_NL);

    // Block
    ASTNode *block = astnode_new(SYM_BLOCK, NULL);
    Token *lookahead = getToken(tokens, tokensLen, *curIdx);
    while (lookahead->type != TOKEN_END) {
        // Still line in the block
        if (lookahead->type == TOKEN_EOF) {
            Error *eofError = getParseError(ERR_SYNTAX_EOF, tokens, tokensLen, curIdx);
            snprintf(eofError->message, MAX_ERRMSG_LEN, "Else block not terminated with \"end if\"."); //TODO: might confuse with nested if
            astnode_free(self);
            astnode_free(block);
            return eofError;
        }
        Error *lineErr = parseLine(block, tokens, tokensLen, curIdx);
        if (lineErr)
            return lineErr;
        lookahead = getToken(tokens, tokensLen, *curIdx);
    }
    astnode_addChildNode(self, block);
    astnode_addChildNode(parent, self);
    return NULL;
}

Error *parseElseIfStmt(ASTNode *parent, Token **tokens, size_t tokensLen, size_t *curIdx){
    printParse("parseElseIfStmt", tokens, curIdx);
    ASTNode *self = astnode_new(SYM_ELSEIF, NULL);
    Error *err = NULL;
    parseTerminal(self, tokens, tokensLen, curIdx, TOKEN_ELSE);
    parseTerminal(self, tokens, tokensLen, curIdx, TOKEN_IF);
    err = parseExpr(self, tokens, tokensLen, curIdx);
    if (err)
        return err;
    err = parseTerminal(self, tokens, tokensLen, curIdx, TOKEN_THEN);
    if (err)
        return err;
    err = parseTerminal(self, tokens, tokensLen, curIdx, TOKEN_NL);
    if (err)
        return err;

    // Block
    ASTNode *block = astnode_new(SYM_BLOCK, NULL);
    Token *lookahead = getToken(tokens, tokensLen, *curIdx);
    while (lookahead->type != TOKEN_END && lookahead->type != TOKEN_ELSE) {
        // Still line in the block
        if (lookahead->type == TOKEN_EOF) {
            Error *eofError = getParseError(ERR_SYNTAX_EOF, tokens, tokensLen, curIdx);
            snprintf(eofError->message, MAX_ERRMSG_LEN, "Else If block not terminated with \"end if\" or \"else\"."); //TODO: might confuse with nested if
            astnode_free(self);
            astnode_free(block);
            return eofError;
        }
        Error *lineErr = parseLine(block, tokens, tokensLen, curIdx);
        if (lineErr)
            return lineErr;
        lookahead = getToken(tokens, tokensLen, *curIdx);
    }
    astnode_addChildNode(self, block);

    // Parse else or end if
    lookahead = getToken(tokens, tokensLen, *curIdx);
    Token *lookahead2 = getToken(tokens, tokensLen, (*curIdx) + 1);
    if (lookahead->type == TOKEN_ELSE) {
        if (lookahead2->type == TOKEN_IF)
            err = parseElseIfStmt(self, tokens, tokensLen, curIdx);
        else
            err = parseElseStmt(self, tokens, tokensLen, curIdx);
    }
    if (err)
        return err;
    
    astnode_addChildNode(parent, self);
    return NULL;
}

Error *parseIfStmt(ASTNode *parent, Token **tokens, size_t tokensLen, size_t *curIdx)
{
    printParse("parseIfStmt", tokens, curIdx);
    ASTNode *self = astnode_new(SYM_IFSTMT, NULL);
    Error *err;
    err = parseTerminal(self, tokens, tokensLen, curIdx, TOKEN_IF);
    if (err)
        return err;
    err = parseExpr(self, tokens, tokensLen, curIdx);
    if (err)
        return err;
    err = parseTerminal(self, tokens, tokensLen, curIdx, TOKEN_THEN);
    if (err)
        return err;
    err = parseTerminal(self, tokens, tokensLen, curIdx, TOKEN_NL);
    if (err)
        return err;

    // Block
    ASTNode *block = astnode_new(SYM_BLOCK, NULL);
    Token *lookahead = getToken(tokens, tokensLen, *curIdx);
    while (lookahead->type != TOKEN_END && lookahead->type != TOKEN_ELSE) {
        // Still line in the block
        if (lookahead->type == TOKEN_EOF) {
            Error *eofError = getParseError(ERR_SYNTAX_EOF, tokens, tokensLen, curIdx);
            snprintf(eofError->message, MAX_ERRMSG_LEN, "If block not terminated with \"end if\" or \"else\"."); //TODO: might confuse with nested if
            astnode_free(self);
            astnode_free(block);
            return eofError;
        }
        Error *lineErr = parseLine(block, tokens, tokensLen, curIdx);
        if (lineErr)
            return lineErr;
        lookahead = getToken(tokens, tokensLen, *curIdx);
    }
    astnode_addChildNode(self, block);

    // Parse else or end if
    lookahead = getToken(tokens, tokensLen, *curIdx);
    Token *lookahead2 = getToken(tokens, tokensLen, (*curIdx) + 1);
    if (lookahead->type == TOKEN_ELSE) {
        if (lookahead2->type == TOKEN_IF)
            err = parseElseIfStmt(self, tokens, tokensLen, curIdx);
        else
            err = parseElseStmt(self, tokens, tokensLen, curIdx);
    }
    if (err)
        return err;
    
    err = parseTerminal(self, tokens, tokensLen, curIdx, TOKEN_END);
    if (err)
        return err;
    err = parseTerminal(self, tokens, tokensLen, curIdx, TOKEN_IF); //NL parsed on Line level
    if (err)
        return err;
    err = parseTerminal(self, tokens, tokensLen, curIdx, TOKEN_NL);
    if (err)
        return err;

    astnode_addChildNode(parent, self);
    return NULL;
}

Error *parseWhile(ASTNode *parent, Token ** tokens, size_t tokensLen, size_t *curIdx)
{
    printParse("parseWhile", tokens, curIdx);
    ASTNode* self = astnode_new(SYM_WHILE, NULL);

    Error *err;
    err = parseTerminal(self, tokens, tokensLen, curIdx, TOKEN_WHILE);
    if (err)
        return err;
    err = parseExpr(self, tokens, tokensLen, curIdx);
    if (err)
        return err;
    err = parseTerminal(self, tokens, tokensLen, curIdx, TOKEN_NL);
    if (err)
        return err;

    // Block
    ASTNode *block = astnode_new(SYM_BLOCK, NULL);
    Token *lookahead = getToken(tokens, tokensLen, *curIdx);
    while (lookahead->type != TOKEN_END) {
        // Still line in the block
        if (lookahead->type == TOKEN_EOF) {
            Error *eofError = getParseError(ERR_SYNTAX_EOF, tokens, tokensLen, curIdx);
            snprintf(eofError->message, MAX_ERRMSG_LEN, "While loop not terminated with \"end while\"."); //TODO: end while or end? might confuse with nested if
            astnode_free(self);
            astnode_free(block);
            return eofError;
        }
        Error *lineErr = parseLine(block, tokens, tokensLen, curIdx);
        if (lineErr)
            return lineErr;
        lookahead = getToken(tokens, tokensLen, *curIdx);
    }
    astnode_addChildNode(self, block);

    err = parseTerminal(self, tokens, tokensLen, curIdx, TOKEN_END);
    if (err)
        return err;
    err = parseTerminal(self, tokens, tokensLen, curIdx, TOKEN_WHILE);
    if (err)
        return err;
    err = parseTerminal(self, tokens, tokensLen, curIdx, TOKEN_NL);
    if (err)
        return err;
    astnode_addChildNode(parent, self);
    return NULL;
}

Error *parseBreak(ASTNode *parent, Token ** tokens, size_t tokensLen, size_t *curIdx)
{
    printParse("parseBreak", tokens, curIdx);
    ASTNode *self = astnode_new(SYM_BREAK, NULL);
    Error *err = NULL;
    err = parseTerminal(self, tokens, tokensLen, curIdx, TOKEN_BREAK);
    if (err)
        return err;
    err = parseTerminal(self, tokens, tokensLen, curIdx, TOKEN_NL);
    if (err)
        return err;
    astnode_addChildNode(parent, self);
    return NULL;
}

Error *parseContinue(ASTNode *parent, Token **tokens, size_t tokensLen, size_t *curIdx)
{
    printParse("parseContinue", tokens, curIdx);
    ASTNode *self = astnode_new(SYM_CONTINUE, NULL);
    Error *err = NULL;
    err = parseTerminal(self, tokens, tokensLen, curIdx, TOKEN_CONTINUE);
    if (err)
        return err;
    err = parseTerminal(self, tokens, tokensLen, curIdx, TOKEN_NL);
    if (err)
        return err;
    astnode_addChildNode(parent, self);
    return NULL;
}

Error *parseStmt(ASTNode *parent, Token **tokens, size_t tokensLen, size_t *curIdx)
{
    printParse("parseStmt", tokens, curIdx);
    ASTNode *self = astnode_new(SYM_STMT, NULL);
    Token *lookahead = getToken(tokens, tokensLen, *curIdx);
    Error *err = NULL;
    if (lookahead->type == TOKEN_PRINT)
	    err = parsePrntStmt(self, tokens, tokensLen, curIdx);
    else if (lookahead->type == TOKEN_WHILE)
        err = parseWhile(self, tokens, tokensLen, curIdx);
    else if (lookahead->type == TOKEN_IF)
        err = parseIfStmt(self, tokens, tokensLen, curIdx);
    else if (lookahead->type == TOKEN_BREAK)
        err = parseBreak(self, tokens, tokensLen, curIdx);
    else if (lookahead->type == TOKEN_CONTINUE)
        err = parseContinue(self, tokens, tokensLen, curIdx);
    else if (lookahead->type == TOKEN_RETURN)
        err = parseReturn(self, tokens, tokensLen, curIdx);
    else
	    err = parseExprStmt(self, tokens, tokensLen, curIdx);

    if (err)
        return err;

    astnode_addChildNode(parent, self);
    return NULL;
}

Error *parseAsmt(ASTNode *parent, Token **tokens, size_t tokensLen, size_t *curIdx)
{
    printParse("parseAsmt", tokens, curIdx);
    ASTNode *self = astnode_new(SYM_ASMT, NULL);
    Token *lookahead = getToken(tokens, tokensLen, *curIdx);
    Token *lookahead2 = getToken(tokens, tokensLen, (*curIdx) + 1);

    Error *err = NULL;
    err = parseTerminal(self, tokens, tokensLen, curIdx, TOKEN_IDENTIFIER);
    if (err)
        return err;
    err = parseTerminal(self, tokens, tokensLen, curIdx, TOKEN_EQUAL);
    if (err)
        return err;
    err = parseExpr(self, tokens, tokensLen, curIdx);
    if (err)
        return err;
    err = parseTerminal(self, tokens, tokensLen, curIdx, TOKEN_NL);
    if (err)
        return err;

    astnode_addChildNode(parent, self);
    return NULL;
}

Error *parseLine(ASTNode *parent, Token **tokens, size_t tokensLen, size_t *curIdx)
{
    printParse("parseLine", tokens, curIdx);
    ASTNode *self = astnode_new(SYM_LINE, NULL);
    
    Token *lookahead = getToken(tokens, tokensLen, *curIdx);

    // Check if empty line
    if (lookahead->type == TOKEN_NL) {
        parseTerminal(self, tokens, tokensLen, curIdx, TOKEN_NL);
        // Don't add the node, we just ignore the newline
        astnode_free(self);
        return NULL;
    } else {
        // (Cheating method, by right should fix the CFG for assignment)
        // Lookahead TWICE to see if it's an assignment
        Token *lookahead2 = getToken(tokens, tokensLen, (*curIdx) + 1);
        Error *err = NULL;
        if (lookahead->type == TOKEN_IDENTIFIER && lookahead2->type == TOKEN_EQUAL)
            err = parseAsmt(self, tokens, tokensLen, curIdx);
        else
            err = parseStmt(self, tokens, tokensLen, curIdx);
        if (err)
            return err;
    }
    
    astnode_addChildNode(parent, self);
    return NULL;
}

Error *parse(ASTNode *root, Token **tokens, size_t tokenCount)
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
        Error *err = parseLine(root, tokens, tokensLen, &curIdx);
        if (err)
            return err;
    }
    
    return NULL;
}
