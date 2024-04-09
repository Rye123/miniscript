#include <stdio.h>
#include <math.h>
#include <string.h>
#include "../lexer/token.h"
#include "../parser/symbol.h"
#include "executor.h"
#include "symboltable.h"

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
	
	if (lVal->type == TYPE_IDENTIFIER) {
	    // Extract symbol value
	    ExecValue *newVal = context_getValue(ctx, lVal);
	    if (newVal == NULL)
		return criticalError("Undeclared identifier.");
	    value_free(lVal);
	    lVal = newVal;
	}

	if (rVal->type == TYPE_IDENTIFIER) {
	    // Extract symbol value
	    ExecValue *newVal = context_getValue(ctx, lVal);
	    if (newVal == NULL)
		return criticalError("Undeclared identifier.");
	    value_free(rVal);
	    rVal = newVal;
	}

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
	
	if (rVal->type == TYPE_IDENTIFIER) {
	    ExecValue *val = context_getValue(ctx, rVal);
	    if (val == NULL)
		return executionError("Undeclared identifier.");
	    value_free(rVal);
	    rVal = val;
	}

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

	if (lVal->type == TYPE_IDENTIFIER) {
	    ExecValue *val = context_getValue(ctx, lVal);
	    if (val == NULL)
		return executionError("Undeclared identifier.");
	    value_free(lVal);
	    lVal = val;
	}
	
	if (rVal->type == TYPE_IDENTIFIER) {
	    ExecValue *val = context_getValue(ctx, rVal);
	    if (val == NULL)
		return executionError("Undeclared identifier.");
	    value_free(rVal);
	    rVal = val;
	}

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

	if (lVal->type == TYPE_IDENTIFIER) {
	    ExecValue *val = context_getValue(ctx, lVal);
	    if (val == NULL)
		return executionError("Undeclared identifier.");
	    value_free(lVal);
	    lVal = val;
	}
	
	if (rVal->type == TYPE_IDENTIFIER) {
	    ExecValue *val = context_getValue(ctx, rVal);
	    if (val == NULL)
		return executionError("Undeclared identifier.");
	    value_free(rVal);
	    rVal = val;
	}

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

	if (lVal->type == TYPE_IDENTIFIER) {
	    ExecValue *val = context_getValue(ctx, lVal);
	    if (val == NULL)
		return executionError("Undeclared identifier.");
	    value_free(lVal);
	    lVal = val;
	}
	
	if (rVal->type == TYPE_IDENTIFIER) {
	    ExecValue *val = context_getValue(ctx, rVal);
	    if (val == NULL)
		return executionError("Undeclared identifier.");
	    value_free(rVal);
	    rVal = val;
	}

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

	if (lVal->type == TYPE_IDENTIFIER) {
	    ExecValue *val = context_getValue(ctx, lVal);
	    if (val == NULL)
		return executionError("Undeclared identifier.");
	    value_free(lVal);
	    lVal = val;
	}
	
	if (rVal->type == TYPE_IDENTIFIER) {
	    ExecValue *val = context_getValue(ctx, rVal);
	    if (val == NULL)
		return executionError("Undeclared identifier.");
	    value_free(rVal);
	    rVal = val;
	}

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

ExecValue *execExpr(Context* ctx, ASTNode *expr)
{
    if (expr->type != SYM_EXPR)
	return criticalError("Given invalid value for execExpr.");

    if (expr->numChildren != 1)
	return criticalError("Number of children of expr not 1.");

    return execEquality(ctx, expr->children[0]);
}

ExecValue *execPrntStmt(Context* ctx, ASTNode *prntStmt)
{
    if (prntStmt->numChildren == 3 &&
	prntStmt->children[0]->tok->type == TOKEN_PRINT &&
	prntStmt->children[1]->type == SYM_EXPR &&
	prntStmt->children[2]->tok->type == TOKEN_NL) {
	ExecValue *exprResult = execExpr(ctx, prntStmt->children[1]);

	if (exprResult->type == TYPE_IDENTIFIER) {
	    // Get symbol value
	    ExecValue *value = context_getValue(ctx, exprResult);
	    value_free(exprResult);
	    exprResult = value;
	}
	
	switch (exprResult->type) {
	case TYPE_IDENTIFIER:
	    printf("ERROR: Identifier value was another identifier\n");
	    exit(1);
	    break;
        case TYPE_STRING:
	    printf("%s", exprResult->value.literal_str);
	    break;
	case TYPE_NUMBER:
	    printf("%f", exprResult->value.literal_num);
	    break;
	case TYPE_NULL:
	    printf("(null)");
	    break;
	}
	value_free(exprResult);
	return value_newNull();
    }
    return criticalError("Invalid print statement.");
}

ExecValue *execExprStmt(Context* ctx, ASTNode *exprStmt)
{
    if (exprStmt->numChildren == 2 &&
	exprStmt->children[0]->type == SYM_EXPR &&
	exprStmt->children[1]->tok->type == TOKEN_NL) {
	ExecValue *exprResult = execExpr(ctx, exprStmt->children[0]);
	value_free(exprResult);
	return value_newNull();
    }
    return criticalError("Invalid expr statement.");
}

ExecValue *execStmt(Context* ctx, ASTNode *stmt)
{
    if (stmt->numChildren == 1) {
	if (stmt->children[0]->type == SYM_EXPR_STMT)
	    return execExprStmt(ctx, stmt->children[0]);
	if (stmt->children[0]->type == SYM_PRNT_STMT)
	    return execPrntStmt(ctx, stmt->children[0]);
    }
    return criticalError("Invalid statement.");
}

ExecValue *execAsmt(Context* ctx, ASTNode *asmt)
{
    if (asmt->numChildren == 4 &&
	asmt->children[0]->tok->type == TOKEN_IDENTIFIER &&
	asmt->children[1]->tok->type == TOKEN_EQUAL &&
	asmt->children[2]->type == SYM_EXPR &&
	asmt->children[3]->tok->type == TOKEN_NL) {
	ExecValue *lvalue = execTerminal(ctx, asmt->children[0]);
	ExecValue *rvalue = execExpr(ctx, asmt->children[2]);
	// TODO: implement symbol table assignment
	// There's no explicit declaration in Miniscript, so we check the symbol table -- if it isn't there, we declare it
	ExecSymbol *sym = context_getSymbol(ctx, lvalue);
        if (sym == NULL)
	    context_addSymbol(ctx, lvalue);

	if (rvalue->type == TYPE_IDENTIFIER) {
	    // Get symbol value
	    ExecValue *value = context_getValue(ctx, rvalue);
	    value_free(rvalue);
	    rvalue = value;
	}
        
	context_setSymbol(ctx, lvalue, rvalue);
        value_free(lvalue); value_free(rvalue);
	return value_newNull();
    }
    return criticalError("Invalid assignment.");
}

ExecValue *execLine(Context* ctx, ASTNode *line)
{
    if (line->numChildren == 1) {
	if (line->children[0]->type == SYM_ASMT)
	    return execAsmt(ctx, line->children[0]);
	if (line->children[0]->type == SYM_STMT)
	    return execStmt(ctx, line->children[0]);
    }
    return criticalError("Invalid line.");
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
