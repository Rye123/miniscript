#include <stdio.h>
#include <string.h>
#include "../error/error.h"
#include "symbol.h"
#include "parser.h"

/* Returns EOF if idx exceeds the token length, otherwise returns the token. */
Token *getToken(Token **tokens, size_t tokensLen, size_t idx) {
    if (idx >= tokensLen)
        return *(tokens + tokensLen - 1);
    return *(tokens + idx);
}

Error *getParseError(ErrorType type, Token **tokens, size_t tokensLen, size_t *curIdx) {
    size_t idx = *curIdx;
    Token *tok = getToken(tokens, tokensLen, idx);
    Error *err;

    while (tok->type == TOKEN_NL || tok->type == TOKEN_EOF) {
        idx--;
        if (idx < 0)
            break;
        tok = getToken(tokens, tokensLen, idx);
    }
    err = error_new(type, tok->lineNum, tok->colNum);
    return err;
}

Error *parseTerminal(ASTNode *parent, Token **tokens, size_t tokensLen, size_t *curIdx, TokenType expectedTokenType) {
    Token *tok;
    Error *err;

    tok = getToken(tokens, tokensLen, *curIdx);
    if (expectedTokenType != tok->type) {
        err = getParseError(ERR_SYNTAX, tokens, tokensLen, curIdx);
        snprintf(err->message, MAX_ERRMSG_LEN, "Expected token %s, instead got %s.", TokenTypeString[expectedTokenType],
                 TokenTypeString[tok->type]);
        return err;
    }
    astnode_addChild(parent, SYM_TERMINAL, tok);
    *curIdx = *curIdx + 1;
    return NULL;
}

Error *parsePrimary(ASTNode *parent, Token **tokens, size_t tokensLen, size_t *curIdx) {
    ASTNode *self;
    Token *lookahead;
    Token *lookahead2;
    Error *err;

    self = astnode_new(SYM_PRIMARY, NULL);
    /* 1. Parse lookahead */
    lookahead = getToken(tokens, tokensLen, *curIdx);
    switch (lookahead->type) {
        case TOKEN_PAREN_L: {
            parseTerminal(self, tokens, tokensLen, curIdx, TOKEN_PAREN_L);

            err = parseExpr(self, tokens, tokensLen, curIdx);
            if (err) {
                error_free(err);
                return err;
            }

            err = parseTerminal(self, tokens, tokensLen, curIdx, TOKEN_PAREN_R);
            if (err) {
                err = getParseError(ERR_SYNTAX, tokens, tokensLen, curIdx);
                snprintf(err->message, MAX_ERRMSG_LEN, "Expected a closing parentheses.");
                error_free(err);
                return err;
            }
            break;
        }
        case TOKEN_IDENTIFIER: {
            lookahead2 = getToken(tokens, tokensLen, *curIdx + 1);
            if (lookahead2->type == TOKEN_PAREN_L) {
                err = parseFnCall(self, tokens, tokensLen, curIdx);
                if (err) {
                    astnode_free(self);
                    return err;
                }
                break;
            }
        }
        case TOKEN_STRING:
        case TOKEN_NUMBER:
        case TOKEN_NULL:
        case TOKEN_TRUE:
        case TOKEN_FALSE: {
            err = parseTerminal(self, tokens, tokensLen, curIdx, lookahead->type);
            if (err)
                return err;
            break;
        }
        default: {
            err = getParseError(ERR_SYNTAX, tokens, tokensLen, curIdx);
            snprintf(err->message, MAX_ERRMSG_LEN,
                     "Expecting either a terminal or a starting parentheses, instead got token %s.",
                     TokenTypeString[lookahead->type]);
            return err;
        }
    }

    astnode_addChildNode(parent, self);
    return NULL;
}

Error *parseFnCall(ASTNode *parent, Token **tokens, size_t tokensLen, size_t *curIdx) {
    ASTNode *self;
    Token *lookahead;
    Token *lookahead2;
    Error *err;

    self = astnode_new(SYM_FN_CALL, NULL);
    lookahead = getToken(tokens, tokensLen, *curIdx);
    lookahead2 = getToken(tokens, tokensLen, *curIdx + 1);
    if (lookahead->type != TOKEN_IDENTIFIER || lookahead2->type != TOKEN_PAREN_L) {
        err = getParseError(ERR_SYNTAX, tokens, tokensLen, curIdx);
        snprintf(err->message, MAX_ERRMSG_LEN, "Expecting a function call.");
        astnode_free(self);
        return err;
    }
    parseTerminal(self, tokens, tokensLen, curIdx, TOKEN_IDENTIFIER);
    parseTerminal(self, tokens, tokensLen, curIdx, TOKEN_PAREN_L);
    err = parseFnArgs(self, tokens, tokensLen, curIdx);
    if (err) {
        astnode_free(self);
        return err;
    }
    lookahead = getToken(tokens, tokensLen, *curIdx);
    if (lookahead->type != TOKEN_PAREN_R) {
        err = getParseError(ERR_SYNTAX, tokens, tokensLen, curIdx);
        snprintf(err->message, MAX_ERRMSG_LEN, "Expected function call to end with right parentheses.");
        astnode_free(self);
        return err;
    }
    parseTerminal(self, tokens, tokensLen, curIdx, TOKEN_PAREN_R);
    astnode_print(self);
    astnode_addChildNode(parent, self);
    return NULL;
}

Error *parseFnArgs(ASTNode *parent, Token **tokens, size_t tokensLen, size_t *curIdx) {
    ASTNode *self;
    Token *lookahead;
    Error *err;

    self = astnode_new(SYM_FN_ARGS, NULL);
    lookahead = getToken(tokens, tokensLen, *curIdx);
    while (lookahead->type != TOKEN_PAREN_R) {
        err = parseExpr(self, tokens, tokensLen, curIdx);
        if (err) {
            astnode_free(self);
            return err;
        }
        parseTerminal(self, tokens, tokensLen, curIdx, TOKEN_COMMA);
        lookahead = getToken(tokens, tokensLen, *curIdx);
    }
    astnode_addChildNode(parent, self);
    return NULL;
}

Error *parsePower(ASTNode *parent, Token **tokens, size_t tokensLen, size_t *curIdx) {
    ASTNode *self;
    Token *lookahead;
    Error *err;

    self = astnode_new(SYM_POWER, NULL);
    /* 1. Parse PRIMARY */
    err = parsePrimary(self, tokens, tokensLen, curIdx);
    if (err)
        return err;

    /* 2. Parse lookahead (INVARIANT: idx now points to after the first comparison) */
    lookahead = getToken(tokens, tokensLen, *curIdx);
    switch (lookahead->type) {
        case TOKEN_CARET: {
            parseTerminal(self, tokens, tokensLen, curIdx, TOKEN_CARET);
            err = parseUnary(self, tokens, tokensLen, curIdx);
            if (err)
                return err;
            break;
        }
        default:
            break;
    }

    astnode_addChildNode(parent, self);
    return NULL;
}

Error *parseUnary(ASTNode *parent, Token **tokens, size_t tokensLen, size_t *curIdx) {
    ASTNode *self;
    Token *lookahead;
    Error *err;

    self = astnode_new(SYM_UNARY, NULL);
    lookahead = getToken(tokens, tokensLen, *curIdx);
    switch (lookahead->type) {
        case TOKEN_PLUS:
        case TOKEN_MINUS: {
            parseTerminal(self, tokens, tokensLen, curIdx, lookahead->type);
            err = parseUnary(self, tokens, tokensLen, curIdx);
            if (err)
                return err;
            break;
        }
        default: {
            err = parsePower(self, tokens, tokensLen, curIdx);
            if (err)
                return err;
            break;
        }
    }

    astnode_addChildNode(parent, self);
    return NULL;
}

Error *parseTermR(ASTNode *parent, Token **tokens, size_t tokensLen, size_t *curIdx) {
    ASTNode *self;
    Token *lookahead;
    Error *err;

    self = astnode_new(SYM_TERM_R, NULL);
    lookahead = getToken(tokens, tokensLen, *curIdx);
    switch (lookahead->type) {
        case TOKEN_STAR:
        case TOKEN_SLASH:
        case TOKEN_PERCENT: {
            parseTerminal(self, tokens, tokensLen, curIdx, lookahead->type);
            err = parseTerm(self, tokens, tokensLen, curIdx);
            if (err)
                return err;
            err = parseTermR(self, tokens, tokensLen, curIdx);
            if (err)
                return err;
            break;
        }
        default:
            break;
    }

    astnode_addChildNode(parent, self);
    return NULL;
}

Error *parseTerm(ASTNode *parent, Token **tokens, size_t tokensLen, size_t *curIdx) {
    ASTNode *self;
    Error *err;

    self = astnode_new(SYM_TERM, NULL);
    err = parseUnary(self, tokens, tokensLen, curIdx);
    if (err)
        return err;
    err = parseTermR(self, tokens, tokensLen, curIdx);
    if (err)
        return err;

    astnode_addChildNode(parent, self);
    return NULL;
}

Error *parseSumR(ASTNode *parent, Token **tokens, size_t tokensLen, size_t *curIdx) {
    ASTNode *self;
    Token *lookahead;
    Error *err;

    self = astnode_new(SYM_SUM_R, NULL);
    lookahead = getToken(tokens, tokensLen, *curIdx);
    switch (lookahead->type) {
        case TOKEN_PLUS:
        case TOKEN_MINUS:
            parseTerminal(self, tokens, tokensLen, curIdx, lookahead->type);
            err = parseSum(self, tokens, tokensLen, curIdx);
            if (err)
                return err;
            err = parseSumR(self, tokens, tokensLen, curIdx);
            if (err)
                return err;
            break;
        default:
            break;
    }

    astnode_addChildNode(parent, self);
    return NULL;
}

Error *parseSum(ASTNode *parent, Token **tokens, size_t tokensLen, size_t *curIdx) {
    ASTNode *self;
    Error *err;

    self = astnode_new(SYM_SUM, NULL);
    err = parseTerm(self, tokens, tokensLen, curIdx);
    if (err)
        return err;
    err = parseSumR(self, tokens, tokensLen, curIdx);
    if (err)
        return err;

    astnode_addChildNode(parent, self);
    return NULL;
}

Error *parseComparisonR(ASTNode *parent, Token **tokens, size_t tokensLen, size_t *curIdx) {
    ASTNode *self;
    Token *lookahead;
    Error *err;

    self = astnode_new(SYM_COMPARISON_R, NULL);
    lookahead = getToken(tokens, tokensLen, *curIdx);
    switch (lookahead->type) {
        case TOKEN_GREATER:
        case TOKEN_GREATER_EQUAL:
        case TOKEN_LESS:
        case TOKEN_LESS_EQUAL:
            parseTerminal(self, tokens, tokensLen, curIdx, lookahead->type);
            err = parseComparison(self, tokens, tokensLen, curIdx);
            if (err)
                return err;
            err = parseComparisonR(self, tokens, tokensLen, curIdx);
            if (err)
                return err;
            break;
        default:
            break;
    }

    astnode_addChildNode(parent, self);
    return NULL;
}

Error *parseComparison(ASTNode *parent, Token **tokens, size_t tokensLen, size_t *curIdx) {
    ASTNode *self;
    Error *err;

    self = astnode_new(SYM_COMPARISON, NULL);
    err = parseSum(self, tokens, tokensLen, curIdx);
    if (err)
        return err;
    err = parseComparisonR(self, tokens, tokensLen, curIdx);
    if (err)
        return err;

    astnode_addChildNode(parent, self);
    return NULL;
}

Error *parseEqualityR(ASTNode *parent, Token **tokens, size_t tokensLen, size_t *curIdx) {
    ASTNode *self;
    Token *lookahead;
    Error *err;

    self = astnode_new(SYM_EQUALITY_R, NULL);
    lookahead = getToken(tokens, tokensLen, *curIdx);
    switch (lookahead->type) {
        case TOKEN_EQUAL_EQUAL:
        case TOKEN_BANG_EQUAL:
            parseTerminal(self, tokens, tokensLen, curIdx, lookahead->type);
            err = parseEquality(self, tokens, tokensLen, curIdx);
            if (err)
                return err;
            err = parseEqualityR(self, tokens, tokensLen, curIdx);
            if (err)
                return err;
            break;
        default:
            break;
    }
    astnode_addChildNode(parent, self);
    return NULL;
}

Error *parseEquality(ASTNode *parent, Token **tokens, size_t tokensLen, size_t *curIdx) {
    ASTNode *self;
    Error *err;

    self = astnode_new(SYM_EQUALITY, NULL);
    err = parseComparison(self, tokens, tokensLen, curIdx);
    if (err)
        return err;
    err = parseEqualityR(self, tokens, tokensLen, curIdx);
    if (err)
        return err;

    astnode_addChildNode(parent, self);
    return NULL;
}

Error *parseLogUnary(ASTNode *parent, Token **tokens, size_t tokensLen, size_t *curIdx) {
    ASTNode *self;
    Token *lookahead;
    Error *err;

    self = astnode_new(SYM_LOG_UNARY, NULL);
    lookahead = getToken(tokens, tokensLen, *curIdx);
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

Error *parseAndExprR(ASTNode *parent, Token **tokens, size_t tokensLen, size_t *curIdx) {
    ASTNode *self;
    Token *lookahead;
    Error *err;

    self = astnode_new(SYM_AND_EXPR_R, NULL);
    lookahead = getToken(tokens, tokensLen, *curIdx);
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

Error *parseAndExpr(ASTNode *parent, Token **tokens, size_t tokensLen, size_t *curIdx) {
    ASTNode *self;
    Error *err;

    self = astnode_new(SYM_AND_EXPR, NULL);
    err = parseLogUnary(self, tokens, tokensLen, curIdx);
    if (err)
        return err;
    err = parseAndExprR(self, tokens, tokensLen, curIdx);
    if (err)
        return err;
    astnode_addChildNode(parent, self);
    return NULL;
}

Error *parseOrExprR(ASTNode *parent, Token **tokens, size_t tokensLen, size_t *curIdx) {
    ASTNode *self;
    Token *lookahead;
    Error *err;

    self = astnode_new(SYM_OR_EXPR_R, NULL);
    lookahead = getToken(tokens, tokensLen, *curIdx);
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

Error *parseOrExpr(ASTNode *parent, Token **tokens, size_t tokensLen, size_t *curIdx) {
    ASTNode *self;
    Error *err;

    self = astnode_new(SYM_OR_EXPR, NULL);
    err = parseAndExpr(self, tokens, tokensLen, curIdx);
    if (err)
        return err;
    err = parseOrExprR(self, tokens, tokensLen, curIdx);
    if (err)
        return err;
    astnode_addChildNode(parent, self);
    return NULL;
}

Error *parseArg(ASTNode *parent, Token **tokens, size_t tokensLen, size_t *curIdx) {
    ASTNode *self;
    Token *lookahead;
    Token *lookahead2;
    Error *err = NULL;

    self = astnode_new(SYM_ARG, NULL);
    lookahead2 = getToken(tokens, tokensLen, *curIdx + 1);
    if (lookahead2->type == TOKEN_EQUAL) {
        /* IDENTIFIER = STRING or NUMBER or NULL */
        err = parseTerminal(self, tokens, tokensLen, curIdx, TOKEN_IDENTIFIER);
        if (err) {
            astnode_free(self);
            return err;
        }
        parseTerminal(self, tokens, tokensLen, curIdx, TOKEN_EQUAL);
        lookahead = getToken(tokens, tokensLen, *curIdx);
        switch (lookahead->type) {
            case TOKEN_STRING:
                parseTerminal(self, tokens, tokensLen, curIdx, TOKEN_STRING);
                break;
            case TOKEN_NUMBER:
                parseTerminal(self, tokens, tokensLen, curIdx, TOKEN_NUMBER);
                break;
            case TOKEN_NULL:
                parseTerminal(self, tokens, tokensLen, curIdx, TOKEN_NULL);
                break;
            default:
                /* error */
                err = getParseError(ERR_SYNTAX, tokens, tokensLen, curIdx);
                snprintf(err->message, MAX_ERRMSG_LEN,
                         "Invalid function parameter definition, should be \"arg\" or \"arg = value\", where value is a string, number or null.");
                astnode_free(self);
                return err;
                break;
        }
    } else {
        /* IDENTIFIER */
        err = parseTerminal(self, tokens, tokensLen, curIdx, TOKEN_IDENTIFIER);
        if (err) {
            astnode_free(self);
            return err;
        }
    }
    astnode_addChildNode(parent, self);
    return NULL;
}

Error *parseArgList(ASTNode *parent, Token **tokens, size_t tokensLen, size_t *curIdx) {
    ASTNode *self;
    Token *lookahead;
    Error *err;

    self = astnode_new(SYM_ARG_LIST, NULL);
    lookahead = getToken(tokens, tokensLen, *curIdx);
    while (lookahead->type != TOKEN_PAREN_R) {
        err = parseArg(self, tokens, tokensLen, curIdx);
        if (err) {
            astnode_free(self);
            return err;
        }
        parseTerminal(self, tokens, tokensLen, curIdx, TOKEN_COMMA);
        lookahead = getToken(tokens, tokensLen, *curIdx);
    }
    astnode_addChildNode(parent, self);
    return NULL;
}

Error *parseReturn(ASTNode *parent, Token **tokens, size_t tokensLen, size_t *curIdx) {
    ASTNode *self;
    Token *lookahead;
    Error *err;

    self = astnode_new(SYM_RETURN, NULL);
    err = parseTerminal(self, tokens, tokensLen, curIdx, TOKEN_RETURN);
    if (err) {
        astnode_free(self);
        return err;
    }

    lookahead = getToken(tokens, tokensLen, *curIdx);
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

Error *parseFnExpr(ASTNode *parent, Token **tokens, size_t tokensLen, size_t *curIdx) {
    ASTNode *self;
    ASTNode *block;
    Token *lookahead;
    Token *lookahead2;
    Error *err;

    self = astnode_new(SYM_FN_EXPR, NULL);
    lookahead = getToken(tokens, tokensLen, *curIdx);
    lookahead2 = getToken(tokens, tokensLen, *curIdx + 1);
    /* Parse function( */
    if (lookahead->type != TOKEN_FUNCTION || lookahead2->type != TOKEN_PAREN_L) {
        printf("%s, %s\n", TokenTypeString[lookahead->type], TokenTypeString[lookahead2->type]);
        err = getParseError(ERR_SYNTAX, tokens, tokensLen, curIdx);
        snprintf(err->message, MAX_ERRMSG_LEN,
                 "Function definition should start with \"function\" and left parentheses: function(arg1, arg2, ...)");
        astnode_free(self);
        return err;
    }
    parseTerminal(self, tokens, tokensLen, curIdx, TOKEN_FUNCTION);
    parseTerminal(self, tokens, tokensLen, curIdx, TOKEN_PAREN_L);

    /* Parse arg list */
    err = parseArgList(self, tokens, tokensLen, curIdx);
    if (err)
        return err;

    /* Parse )\n */
    lookahead = getToken(tokens, tokensLen, *curIdx);
    lookahead2 = getToken(tokens, tokensLen, *curIdx + 1);
    if (lookahead->type != TOKEN_PAREN_R || lookahead2->type != TOKEN_NL) {
        err = getParseError(ERR_SYNTAX, tokens, tokensLen, curIdx);
        snprintf(err->message, MAX_ERRMSG_LEN,
                 "Function definition should end with right parentheses and a new line: function(arg1, arg2, ...)");
        astnode_free(self);
        return err;
    }
    parseTerminal(self, tokens, tokensLen, curIdx, TOKEN_PAREN_R);
    parseTerminal(self, tokens, tokensLen, curIdx, TOKEN_NL);

    /* Parse Block */
    block = astnode_new(SYM_BLOCK, NULL);
    lookahead = getToken(tokens, tokensLen, *curIdx);
    while (lookahead->type != TOKEN_END) {
        if (lookahead->type == TOKEN_EOF) {
            Error *eofError = getParseError(ERR_SYNTAX_EOF, tokens, tokensLen, curIdx);
            snprintf(eofError->message, MAX_ERRMSG_LEN, "Function block not terminated with \"end function\".");
            astnode_free(self);
            astnode_free(block);
            return eofError;
        }
        err = parseLine(block, tokens, tokensLen, curIdx);
        if (err)
            return err;
        lookahead = getToken(tokens, tokensLen, *curIdx);
    }
    astnode_addChildNode(self, block);

    /* End */
    lookahead = getToken(tokens, tokensLen, *curIdx);
    lookahead2 = getToken(tokens, tokensLen, *curIdx + 1);
    if (lookahead->type != TOKEN_END || lookahead2->type != TOKEN_FUNCTION) {
        err = getParseError(ERR_SYNTAX_EOF, tokens, tokensLen, curIdx);
        snprintf(err->message, MAX_ERRMSG_LEN, "Function block not terminated with \"end function\"");
        astnode_free(self);
        astnode_free(block);
        return err;
    }
    parseTerminal(self, tokens, tokensLen, curIdx, TOKEN_END);
    parseTerminal(self, tokens, tokensLen, curIdx, TOKEN_FUNCTION);

    astnode_addChildNode(parent, self);
    return NULL;
}

Error *parseExpr(ASTNode *parent, Token **tokens, size_t tokensLen, size_t *curIdx) {
    ASTNode *self;
    Token *lookahead;
    Error *err;

    self = astnode_new(SYM_EXPR, NULL);
    lookahead = getToken(tokens, tokensLen, *curIdx);
    if (lookahead->type == TOKEN_FUNCTION)
        err = parseFnExpr(self, tokens, tokensLen, curIdx);
    else
        err = parseOrExpr(self, tokens, tokensLen, curIdx);

    if (err)
        return err;
    astnode_addChildNode(parent, self);
    return NULL;
}

Error *parsePrntStmt(ASTNode *parent, Token **tokens, size_t tokensLen, size_t *curIdx) {
    ASTNode *self;
    Error *err;
    Token *lookahead;

    self = astnode_new(SYM_PRNT_STMT, NULL);
    err = parseTerminal(self, tokens, tokensLen, curIdx, TOKEN_PRINT);
    if (err)
        return err;

    lookahead = getToken(tokens, tokensLen, *curIdx);
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

Error *parseExprStmt(ASTNode *parent, Token **tokens, size_t tokensLen, size_t *curIdx) {
    ASTNode *self;
    Token *lookahead;
    Error *err;

    self = astnode_new(SYM_EXPR_STMT, NULL);
    lookahead = getToken(tokens, tokensLen, *curIdx);
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

Error *parseElseStmt(ASTNode *parent, Token **tokens, size_t tokensLen, size_t *curIdx) {
    ASTNode *self;
    ASTNode *block;
    Token *lookahead;
    Error *err;

    self = astnode_new(SYM_ELSE, NULL);
    err = parseTerminal(self, tokens, tokensLen, curIdx, TOKEN_ELSE);
    if (err)
        return err;
    parseTerminal(self, tokens, tokensLen, curIdx, TOKEN_NL);

    /* Block */
    block = astnode_new(SYM_BLOCK, NULL);
    lookahead = getToken(tokens, tokensLen, *curIdx);
    while (lookahead->type != TOKEN_END) {
        /* Still line in the block */
        if (lookahead->type == TOKEN_EOF) {
            err = getParseError(ERR_SYNTAX_EOF, tokens, tokensLen, curIdx);
            snprintf(err->message, MAX_ERRMSG_LEN, "Else block not terminated with \"end if\".");
            astnode_free(self);
            astnode_free(block);
            return err;
        }
        err = parseLine(block, tokens, tokensLen, curIdx);
        if (err)
            return err;
        lookahead = getToken(tokens, tokensLen, *curIdx);
    }
    astnode_addChildNode(self, block);
    astnode_addChildNode(parent, self);
    return NULL;
}

Error *parseElseIfStmt(ASTNode *parent, Token **tokens, size_t tokensLen, size_t *curIdx) {
    ASTNode *self;
    ASTNode *block;
    Token *lookahead;
    Token *lookahead2;
    Error *err = NULL;

    self = astnode_new(SYM_ELSEIF, NULL);
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

    /* Block */
    block = astnode_new(SYM_BLOCK, NULL);
    lookahead = getToken(tokens, tokensLen, *curIdx);
    while (lookahead->type != TOKEN_END && lookahead->type != TOKEN_ELSE) {
        /* Still line in the block */
        if (lookahead->type == TOKEN_EOF) {
            Error *eofError = getParseError(ERR_SYNTAX_EOF, tokens, tokensLen, curIdx);
            snprintf(eofError->message, MAX_ERRMSG_LEN, "Else If block not terminated with \"end if\" or \"else\".");
            astnode_free(self);
            astnode_free(block);
            return eofError;
        }
        err = parseLine(block, tokens, tokensLen, curIdx);
        if (err)
            return err;
        lookahead = getToken(tokens, tokensLen, *curIdx);
    }
    astnode_addChildNode(self, block);

    /* Parse else or end if */
    lookahead = getToken(tokens, tokensLen, *curIdx);
    lookahead2 = getToken(tokens, tokensLen, (*curIdx) + 1);
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

Error *parseIfStmt(ASTNode *parent, Token **tokens, size_t tokensLen, size_t *curIdx) {
    ASTNode *self;
    ASTNode *block;
    Token *lookahead;
    Token *lookahead2;
    Error *err;

    self = astnode_new(SYM_IFSTMT, NULL);
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

    /* Block */
    block = astnode_new(SYM_BLOCK, NULL);
    lookahead = getToken(tokens, tokensLen, *curIdx);
    while (lookahead->type != TOKEN_END && lookahead->type != TOKEN_ELSE) {
        /* Still line in the block */
        if (lookahead->type == TOKEN_EOF) {
            err = getParseError(ERR_SYNTAX_EOF, tokens, tokensLen, curIdx);
            snprintf(err->message, MAX_ERRMSG_LEN, "If block not terminated with \"end if\" or \"else\".");
            astnode_free(self);
            astnode_free(block);
            return err;
        }
        err = parseLine(block, tokens, tokensLen, curIdx);
        if (err)
            return err;
        lookahead = getToken(tokens, tokensLen, *curIdx);
    }
    astnode_addChildNode(self, block);

    /* Parse else or end if */
    lookahead = getToken(tokens, tokensLen, *curIdx);
    lookahead2 = getToken(tokens, tokensLen, (*curIdx) + 1);
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
    err = parseTerminal(self, tokens, tokensLen, curIdx, TOKEN_IF); /*NL parsed on Line level */
    if (err)
        return err;
    err = parseTerminal(self, tokens, tokensLen, curIdx, TOKEN_NL);
    if (err)
        return err;

    astnode_addChildNode(parent, self);
    return NULL;
}

Error *parseWhile(ASTNode *parent, Token **tokens, size_t tokensLen, size_t *curIdx) {
    ASTNode *self;
    ASTNode *block;
    Token *lookahead;
    Error *err;

    self = astnode_new(SYM_WHILE, NULL);
    err = parseTerminal(self, tokens, tokensLen, curIdx, TOKEN_WHILE);
    if (err)
        return err;
    err = parseExpr(self, tokens, tokensLen, curIdx);
    if (err)
        return err;
    err = parseTerminal(self, tokens, tokensLen, curIdx, TOKEN_NL);
    if (err)
        return err;

    /* Block */
    block = astnode_new(SYM_BLOCK, NULL);
    lookahead = getToken(tokens, tokensLen, *curIdx);
    while (lookahead->type != TOKEN_END) {
        /* Still line in the block */
        if (lookahead->type == TOKEN_EOF) {
            err = getParseError(ERR_SYNTAX_EOF, tokens, tokensLen, curIdx);
            snprintf(err->message, MAX_ERRMSG_LEN, "While loop not terminated with \"end while\".");
            astnode_free(self);
            astnode_free(block);
            return err;
        }
        err = parseLine(block, tokens, tokensLen, curIdx);
        if (err)
            return err;
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

Error *parseBreak(ASTNode *parent, Token **tokens, size_t tokensLen, size_t *curIdx) {
    ASTNode *self;
    Error *err;

    self = astnode_new(SYM_BREAK, NULL);
    err = parseTerminal(self, tokens, tokensLen, curIdx, TOKEN_BREAK);
    if (err)
        return err;
    err = parseTerminal(self, tokens, tokensLen, curIdx, TOKEN_NL);
    if (err)
        return err;
    astnode_addChildNode(parent, self);
    return NULL;
}

Error *parseContinue(ASTNode *parent, Token **tokens, size_t tokensLen, size_t *curIdx) {
    ASTNode *self;
    Error *err;

    self = astnode_new(SYM_CONTINUE, NULL);
    err = parseTerminal(self, tokens, tokensLen, curIdx, TOKEN_CONTINUE);
    if (err)
        return err;
    err = parseTerminal(self, tokens, tokensLen, curIdx, TOKEN_NL);
    if (err)
        return err;
    astnode_addChildNode(parent, self);
    return NULL;
}

Error *parseStmt(ASTNode *parent, Token **tokens, size_t tokensLen, size_t *curIdx) {
    ASTNode *self;
    Token *lookahead;
    Error *err;

    self = astnode_new(SYM_STMT, NULL);
    lookahead = getToken(tokens, tokensLen, *curIdx);
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

Error *parseAsmt(ASTNode *parent, Token **tokens, size_t tokensLen, size_t *curIdx) {
    ASTNode *self;
    Error *err;

    self = astnode_new(SYM_ASMT, NULL);
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

Error *parseLine(ASTNode *parent, Token **tokens, size_t tokensLen, size_t *curIdx) {
    ASTNode *self;
    Token *lookahead;
    Token *lookahead2;
    Error *err;

    self = astnode_new(SYM_LINE, NULL);
    lookahead = getToken(tokens, tokensLen, *curIdx);
    /* Check if empty line */
    if (lookahead->type == TOKEN_NL) {
        parseTerminal(self, tokens, tokensLen, curIdx, TOKEN_NL);
        /* Don't add the node, we just ignore the newline */
        astnode_free(self);
        return NULL;
    } else {
        /* (Cheating method, by right should fix the CFG for assignment) */
        /* Lookahead TWICE to see if it's an assignment */
        lookahead2 = getToken(tokens, tokensLen, (*curIdx) + 1);
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

Error *parse(ASTNode *root, Token **tokens, size_t tokenCount) {
    size_t tokensLen = tokenCount;
    size_t curIdx = 0;
    Token *lookahead;
    Error *err;

    /* Create the tree */
    while (1) {
        lookahead = getToken(tokens, tokensLen, curIdx);
        if (lookahead->type == TOKEN_EOF)
            break;
        err = parseLine(root, tokens, tokensLen, &curIdx);
        if (err)
            return err;
    }

    return NULL;
}
