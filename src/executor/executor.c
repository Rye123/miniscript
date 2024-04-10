#include <stdio.h>
#include <string.h>
#include "../lexer/token.h"
#include "../parser/symbol.h"
#include "../logger/logger.h"
#include "executor.h"
#include "symboltable.h"

// Returns the actual value of `val` if it's an identifier, otherwise just returns `val`.
// If it's an identifier, `val` is freed.
ExecValue *unpackValue(Context *ctx, ExecValue *val)
{
    if (val->type == TYPE_IDENTIFIER) {
        // Extract symbol value
        ExecValue *newVal = context_getValue(ctx, val);
        if (newVal == NULL)
            return executionError("Undeclared identifier.");
        value_free(val);
        val = newVal;
    }
    return val;
}

ExecValue *execTerminal(Context* ctx, ASTNode *terminal)
{
    if (terminal->type != SYM_TERMINAL)
        return criticalError("terminal: Invalid symbol type, expected SYM_TERMINAL");

    Token *tok = terminal->tok;

    switch (tok->type) {
    case TOKEN_NULL:
        return value_newNull();
    case TOKEN_TRUE:
        return value_newNumber(1.0);
    case TOKEN_FALSE:
        return value_newNumber(0.0);
    case TOKEN_NUMBER:
        return value_newNumber(tok->literal.literal_num);
    case TOKEN_STRING:
        return value_newString(tok->literal.literal_str);
    case TOKEN_IDENTIFIER:
        return value_newIdentifier(tok->lexeme);
    default:
        return criticalError("terminal: Invalid token for execTerminal");
    }
}

ExecValue *execPrimary(Context* ctx, ASTNode *primary)
{
    if (primary->type != SYM_PRIMARY)
        return criticalError("primary: Invalid symbol type, expected SYM_PRIMARY");
    
    // Check if primary is ( EXPR ) or TERMINAL
    if (primary->numChildren == 1)
        return execTerminal(ctx, primary->children[0]);
    if (primary->numChildren == 3) {
        if (primary->children[0]->tok->type != TOKEN_PAREN_L ||
            primary->children[2]->tok->type != TOKEN_PAREN_R)
            return criticalError("primary: Expected ( EXPR ), instead given invalid expression with 3 children.");
        return execExpr(ctx, primary->children[1]);
    }
    
    return criticalError("primary: Expected 1 or 3 children.");
}

ExecValue *execPower(Context* ctx, ASTNode *power)
{
    if (power->type != SYM_POWER)
        return criticalError("power: Invalid symbol type, expected SYM_POWER");
    
    // Check if power is PRIMARY ^ UNARY or PRIMARY
    if (power->numChildren == 1)
        return execPrimary(ctx, power->children[0]);
    if (power->numChildren == 3) {
        ExecValue *lVal = execPrimary(ctx, power->children[0]);
        ExecValue *rVal = execUnary(ctx, power->children[2]);
        ExecValue *retVal;

        lVal = unpackValue(ctx, lVal);
        rVal = unpackValue(ctx, rVal);

        TokenType op = power->children[1]->tok->type;
        if (op != TOKEN_CARET)
            return criticalError("power: Unexpected operator, expected ^.");
        retVal = value_opPow(lVal, rVal);
        value_free(lVal); value_free(rVal);
        return retVal;
    }

    return criticalError("power: Expected 1 or 3 children.");
}

ExecValue *execUnary(Context* ctx, ASTNode *unary)
{
    if (unary->type != SYM_UNARY)
        return criticalError("unary: Invalid symbol type, expected SYM_UNARY");
    
    // Check if unary is +/- UNARY or POWER
    if (unary->numChildren == 1)
        return execPower(ctx, unary->children[0]);
    if (unary->numChildren == 2) {
        ExecValue *rVal = execUnary(ctx, unary->children[1]);
        ExecValue *retVal;
        TokenType op = unary->children[0]->tok->type;
        
        rVal = unpackValue(ctx, rVal);

        switch (op) {
        case TOKEN_PLUS:  retVal = value_opUnaryPos(rVal); break;
        case TOKEN_MINUS: retVal = value_opUnaryNeg(rVal); break;
        default:
            retVal = criticalError("unary: Unexpected operator.");
        }
        value_free(rVal);
        return retVal;
    }

    return criticalError("unary: Expected 1 or 2 children.");
}

ExecValue *execTerm(Context* ctx, ASTNode *term)
{
    if (term->type != SYM_TERM)
        return criticalError("term: Invalid symbol type, expected SYM_TERM.");

    // Check if term is TERM OP UNARY or UNARY
    if (term->numChildren == 1)
        return execUnary(ctx, term->children[0]);
    if (term->numChildren == 3) {
        ExecValue *lVal = execTerm(ctx, term->children[0]);
        ExecValue *rVal = execUnary(ctx, term->children[2]);
        ExecValue *retVal;

        lVal = unpackValue(ctx, lVal);
        rVal = unpackValue(ctx, rVal);

        TokenType op = term->children[1]->tok->type;

        switch (op) {
        case TOKEN_STAR:    retVal = value_opMul(lVal, rVal); break;
        case TOKEN_SLASH:   retVal = value_opDiv(lVal, rVal); break;
        case TOKEN_PERCENT: retVal = value_opMod(lVal, rVal); break;
        default:
            retVal = criticalError("term: Unexpected operator.");
        }
        value_free(lVal); value_free(rVal);
        return retVal;
    }
    return criticalError("term: Expected 1 or 3 children.");
}

ExecValue *execSum(Context* ctx, ASTNode *sum)
{
    if (sum->type != SYM_SUM)
        return criticalError("sum: Invalid symbol type, expected SYM_SUM.");

    // Check if sum is SUM OP TERM or SUM
    if (sum->numChildren == 1)
        return execTerm(ctx, sum->children[0]);
    if (sum->numChildren == 3) {
        ExecValue *lVal = execSum(ctx, sum->children[0]);
        ExecValue *rVal = execTerm(ctx, sum->children[2]);
        ExecValue *retVal;

        lVal = unpackValue(ctx, lVal);
        rVal = unpackValue(ctx, rVal);

        TokenType op = sum->children[1]->tok->type;

        switch (op) {
        case TOKEN_PLUS:  retVal = value_opAdd(lVal, rVal); break;
        case TOKEN_MINUS: retVal = value_opSub(lVal, rVal); break;
        default:
            retVal = criticalError("sum: Unexpected operator.");
        }
        value_free(lVal); value_free(rVal);
        return retVal;
    }
    return criticalError("sum: Expected 1 or 3 children.");
}

ExecValue *execComparison(Context* ctx, ASTNode *comparison)
{
    if (comparison->type != SYM_COMPARISON)
        return criticalError("comparison: Invalid symbol type, expected SYM_COMPARISON.");

    // Check if comparison is SUM OP COMPARISON or SUM
    if (comparison->numChildren == 1)
        return execSum(ctx, comparison->children[0]);
    if (comparison->numChildren == 3) {
        ExecValue *lVal = execComparison(ctx, comparison->children[0]);
        ExecValue *rVal = execSum(ctx, comparison->children[2]);
        ExecValue *retVal;

        lVal = unpackValue(ctx, lVal);
        rVal = unpackValue(ctx, rVal);

        TokenType op = comparison->children[1]->tok->type;

        switch (op) {
        case TOKEN_GREATER:       retVal = value_opGt(lVal, rVal); break;
        case TOKEN_GREATER_EQUAL: retVal = value_opGEq(lVal, rVal); break;
        case TOKEN_LESS:          retVal = value_opLt(lVal, rVal); break;
        case TOKEN_LESS_EQUAL:    retVal = value_opLEq(lVal, rVal); break;
        default:
            retVal = criticalError("comparison: Unexpected operator.");
        }
        value_free(lVal); value_free(rVal);
        return retVal;
    }
    return criticalError("comparison: Expected 1 or 3 children.");
}

ExecValue *execEquality(Context* ctx, ASTNode *equality)
{
    if (equality->type != SYM_EQUALITY)
        return criticalError("equality: Invalid symbol type, expected SYM_EQUALITY.");

    // Check if equality is COMPARISON OP EQUALITY or COMPARISON
    if (equality->numChildren == 1)
        return execComparison(ctx, equality->children[0]);
    if (equality->numChildren == 3) {
        ExecValue *lVal = execEquality(ctx, equality->children[0]);
        ExecValue *rVal = execComparison(ctx, equality->children[2]);
        ExecValue *retVal;

        lVal = unpackValue(ctx, lVal);
        rVal = unpackValue(ctx, rVal);

        TokenType op = equality->children[1]->tok->type;

        switch (op) {
        case TOKEN_EQUAL_EQUAL: retVal = value_opEqEq(lVal, rVal); break;
        case TOKEN_BANG_EQUAL:  retVal = value_opNEq(lVal, rVal); break;
        default:
            retVal = criticalError("equality: Unexpected operator.");
        }
        value_free(lVal); value_free(rVal);
        return retVal;
    }
    return criticalError("equality: Expected 1 or 3 children.");
}
ExecValue* execLogUnary(Context* ctx, ASTNode* logUnary)
{
    if (logUnary->type != SYM_LOG_UNARY)
        return criticalError("logUnary: Invalid symbol type, expected SYM_LOG_UNARY");

    // Check if expr is not LOG_UNARY | EQUALITY
    if (logUnary->numChildren == 1)
        return execEquality(ctx, logUnary->children[0]);
    if (logUnary->numChildren == 2) {
        ExecValue *rVal = execLogUnary(ctx, logUnary->children[1]);
        ExecValue *retVal;
        TokenType op = logUnary->children[0]->tok->type;
        
        rVal = unpackValue(ctx, rVal);

        if (op == TOKEN_NOT)
            retVal = value_opNot(rVal);
        else
            retVal = criticalError("logUnary: Unexpected operator.");

        value_free(rVal);
        return retVal;
    }

    return criticalError("logUnary: Expected 1 or 2 children.");
    
}

ExecValue* execAndExpr(Context* ctx, ASTNode* andExpr)
{
    if (andExpr->type != SYM_AND_EXPR)
        return criticalError("andExpr: Invalid symbol type, expected SYM_AND_EXPR");

    // Check if expr is AND_EXPR and LOG_UNARY | LOG_UNARY
    if (andExpr->numChildren == 1)
        return execLogUnary(ctx, andExpr->children[0]);
    if (andExpr->numChildren == 3) {
        ExecValue *lVal = execAndExpr(ctx, andExpr->children[0]);
        ExecValue *rVal = execLogUnary(ctx, andExpr->children[2]);
        ExecValue *retVal;

        lVal = unpackValue(ctx, lVal);
        rVal = unpackValue(ctx, rVal);

        retVal = value_opAnd(lVal, rVal);
        value_free(lVal); value_free(rVal);

        return retVal;
    }
    return criticalError("andExpr: Expected 1 or 3 children.");
}

ExecValue* execOrExpr(Context* ctx, ASTNode* orExpr)
{
    if (orExpr->type != SYM_OR_EXPR)
        return criticalError("orExpr: Invalid symbol type, expected SYM_OR_EXPR");

    // Check if expr is OR_EXPR or AND_EXPR  |  AND_EXPR
    if (orExpr->numChildren == 1)
        return execAndExpr(ctx, orExpr->children[0]);
    if (orExpr->numChildren == 3) {
        ExecValue *lVal = execOrExpr(ctx, orExpr->children[0]);
        ExecValue *rVal = execAndExpr(ctx, orExpr->children[2]);
        ExecValue *retVal;

        lVal = unpackValue(ctx, lVal);
        rVal = unpackValue(ctx, rVal);

        retVal = value_opOr(lVal, rVal);
        value_free(lVal); value_free(rVal);

        return retVal;
    }
    return criticalError("orExpr: Expected 1 or 3 children.");
}

ExecValue *execExpr(Context* ctx, ASTNode *expr)
{
    if (expr->type != SYM_EXPR)
        return criticalError("expr: Invalid symbol type, expected SYM_EXPR");

    if (expr->numChildren != 1)
        return criticalError("expr: Expected 1 child.");

    return execOrExpr(ctx, expr->children[0]);
}

ExecValue *execPrntStmt(Context* ctx, ASTNode *prntStmt)
{
    if (prntStmt->type != SYM_PRNT_STMT)
        return criticalError("prntStmt: Invalid symbol type, expected SYM_PRNT_STMT");
    if (prntStmt->numChildren == 3 &&
	prntStmt->children[0]->tok->type == TOKEN_PRINT &&
	prntStmt->children[1]->type == SYM_EXPR &&
	prntStmt->children[2]->tok->type == TOKEN_NL) {
		ExecValue *exprResult = execExpr(ctx, prntStmt->children[1]);

        exprResult = unpackValue(ctx, exprResult);
		
		switch (exprResult->type) {
			case TYPE_IDENTIFIER:
				log_message(&executionLogger,"ERROR: Identifier value was another identifier\n");
				exit(1);
				break;
				case TYPE_STRING:
                    log_message(&consoleLogger,"%s\n", exprResult->value.literal_str);
                log_message(&executionLogger,"%s\n", exprResult->value.literal_str);
                log_message(&resultLogger,"%s\n", exprResult->value.literal_str);
				break;
			case TYPE_NUMBER:
                log_message(&consoleLogger,"%g\n", exprResult->value.literal_num);
                log_message(&executionLogger,"%g\n", exprResult->value.literal_num);
				log_message(&resultLogger,"%g\n", exprResult->value.literal_num);
				break;
			case TYPE_NULL:
                log_message(&consoleLogger,"null\n");
                log_message(&executionLogger,"null\n");
				log_message(&resultLogger,"null\n");
				break;
			}
		value_free(exprResult);
		return value_newNull();
    }
    return criticalError("prntstmt: Invalid print statement.");
}

ExecValue *execExprStmt(Context* ctx, ASTNode *exprStmt)
{
    if (exprStmt->type != SYM_EXPR_STMT)
        return criticalError("exprStmt: Invalid symbol type, expected SYM_EXPR_STMT");
    if (exprStmt->numChildren == 2 &&
        exprStmt->children[0]->type == SYM_EXPR &&
        exprStmt->children[1]->tok->type == TOKEN_NL) {
        ExecValue *exprResult = execExpr(ctx, exprStmt->children[0]);
        value_free(exprResult);
        return value_newNull();
    }
    return criticalError("exprStmt: Invalid exprStmt.");
}

ExecValue * execBlock(Context* ctx, ASTNode *block){
	for (int i=0; i<block->numChildren; i++){
		execLine(ctx, block->children[i]);
	}
	return value_newNull();
}

ExecValue *execElse(Context* ctx, ASTNode *elseStmt){
    if (elseStmt->type != SYM_ELSE)
        return criticalError("elsestmt: Invalid symbol type, expected SYM_ELSE");
    
	if (elseStmt->numChildren == 3)
		return execBlock(ctx, elseStmt->children[2]);
	else
		return criticalError("execElse: Invalid line.");
	
}

ExecValue *execElseIf(Context* ctx, ASTNode *elseIfStmt){
    if (elseIfStmt->type != SYM_ELSEIF)
        return criticalError("elseifstmt: Invalid symbol type, expected SYM_ELSEIF");

    ExecValue *expr = execExpr(ctx, elseIfStmt->children[2]);
    expr = unpackValue(ctx, expr);
    if (value_falsiness(expr) == 1) { // true branch
        return execBlock(ctx, elseIfStmt->children[5]);
    } else {
        if (elseIfStmt->numChildren == 7) {
            if (elseIfStmt->children[6]->type == SYM_ELSEIF)
                return execElseIf(ctx, elseIfStmt->children[6]);
            else if (elseIfStmt->children[6]->type == SYM_ELSE)
                return execElse(ctx, elseIfStmt->children[6]);

            return criticalError("execElseIf: Invalid branch -- not else if, or else");
        }
        return value_newNull();
    }
}

ExecValue *execIfStmt(Context* ctx, ASTNode *ifStmt){
    if (ifStmt->type != SYM_IFSTMT)
        return criticalError("ifstmt: Invalid symbol type, expected SYM_IFSTMT");

    ExecValue *expr = execExpr(ctx, ifStmt->children[1]);
    expr = unpackValue(ctx, expr);
    if (value_falsiness(expr) == 1) { // true branch
        return execBlock(ctx, ifStmt->children[4]);
    } else {
        if (ifStmt->children[5]->type == SYM_ELSEIF)
            return execElseIf(ctx, ifStmt->children[5]);
        else if (ifStmt->children[5]->type == SYM_ELSE)
            return execElse(ctx, ifStmt->children[5]);
        return value_newNull();
    }
}

ExecValue *execWhileStmt(Context* ctx, ASTNode *whileStmt){
    if (whileStmt->type != SYM_WHILE)
        return criticalError("ifstmt: Invalid symbol type, expected SYM_WHILE");
    ExecValue *expr = execExpr(ctx, whileStmt->children[1]);
    while (value_falsiness(unpackValue(ctx, expr)) == 1){
        execBlock(ctx, whileStmt->children[3]);
        expr = execExpr(ctx, whileStmt->children[1]);
    }
    return value_newNull();
}

ExecValue *execStmt(Context* ctx, ASTNode *stmt)
{
    if (stmt->type != SYM_STMT)
        return criticalError("stmt: Invalid symbol type, expected SYM_STMT");
	if (stmt->numChildren == 1) {
		if (stmt->children[0]->type == SYM_EXPR_STMT)
			return execExprStmt(ctx, stmt->children[0]);
		if (stmt->children[0]->type == SYM_PRNT_STMT)
			return execPrntStmt(ctx, stmt->children[0]);
		if (stmt->children[0]->type == SYM_IFSTMT){
			return execIfStmt(ctx, stmt->children[0]);
		}
		if (stmt->children[0]->type == SYM_WHILE){
			return execWhileStmt(ctx, stmt->children[0]);
		}
    }
    return criticalError("stmt: Invalid statement.");
}

ExecValue *execAsmt(Context* ctx, ASTNode *asmt)
{
    if (asmt->type != SYM_ASMT)
        return criticalError("asmt: Invalid symbol type, expected SYM_ASMT");
    if (asmt->numChildren == 4 &&
        asmt->children[0]->tok->type == TOKEN_IDENTIFIER &&
        asmt->children[1]->tok->type == TOKEN_EQUAL &&
        asmt->children[2]->type == SYM_EXPR &&
        asmt->children[3]->tok->type == TOKEN_NL) {
        ExecValue *lvalue = execTerminal(ctx, asmt->children[0]);
        ExecValue *rvalue = execExpr(ctx, asmt->children[2]);
        // There's no explicit declaration in Miniscript, so we check the symbol table -- if it isn't there, we declare it
        ExecSymbol *sym = context_getSymbol(ctx, lvalue);
        if (sym == NULL)
            context_addSymbol(ctx, lvalue);

        rvalue = unpackValue(ctx, rvalue);
        
        context_setSymbol(ctx, lvalue, rvalue);
        value_free(lvalue); value_free(rvalue);
        return value_newNull();
    }
    return criticalError("asmt: Invalid assignment.");
}

ExecValue *execLine(Context* ctx, ASTNode *line)
{
    if (line->type != SYM_LINE)
        return criticalError("line: Invalid symbol type, expected SYM_LINE");
    if (line->numChildren == 1) {
        if (line->children[0]->type == SYM_ASMT)
            return execAsmt(ctx, line->children[0]);
        if (line->children[0]->type == SYM_STMT)
            return execStmt(ctx, line->children[0]);
    }
    return criticalError("line: Invalid line.");
}

ExecValue *execStart(Context* ctx, ASTNode *start)
{
    // Returns the execution exit code
    //TODO: all runtime errors here
    int exitCode = 0;
    for (size_t i = 0; i < start->numChildren; i++) {
        ASTNode *child = start->children[i];
        ExecValue *result;
        if (child->type == SYM_LINE)
            result = execLine(ctx, child);
        else if (child->tok->type == TOKEN_EOF)
            break;
        else
            return criticalError("Unexpected symbol.");

        value_free(result);
    }

    return value_newNumber(exitCode);
}
