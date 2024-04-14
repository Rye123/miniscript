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

        if (newVal == NULL && ctx->global != NULL) {
            // Check global scope
            newVal = context_getValue(ctx->global, val);
        }

        if (newVal == NULL) {
            Error *nameErr = error_new(ERR_RUNTIME_NAME, -1, -1);
            snprintf(nameErr->message, MAX_ERRMSG_LEN, "Undeclared identifier \"%s\"", val->value.identifier_name);
            ExecValue *errVal = value_newError(nameErr, val->tok);
            value_free(val);
            return errVal;
        }
        value_free(val);
        val = newVal;
    }
    return val;
}

ExecValue *execTerminal(Context* ctx, ASTNode *terminal)
{
    if (terminal->type != SYM_TERMINAL)
        criticalError("terminal: Invalid symbol type, expected SYM_TERMINAL");

    Token *tok = terminal->tok;

    switch (tok->type) {
    case TOKEN_NULL:
        return value_newNull();
    case TOKEN_TRUE:
        return value_newNumber(1.0, tok);
    case TOKEN_FALSE:
        return value_newNumber(0.0, tok);
    case TOKEN_NUMBER:
        return value_newNumber(tok->literal.literal_num, tok);
    case TOKEN_STRING:
        return value_newString(tok->literal.literal_str, tok);
    case TOKEN_IDENTIFIER:
        return value_newIdentifier(tok->lexeme, tok);
    default:
        criticalError("terminal: Invalid token for execTerminal");
    }
    return NULL;
}

ExecValue *execFnArgs(Context* ctx, ASTNode *fnArgs)
{
    if (fnArgs->type != SYM_FN_ARGS)
        criticalError("fnArgs: Invalid symbol type, expected SYM_FN_ARGS");

    size_t curFnArgCount = 0;
    for (size_t i = 0; i < fnArgs->numChildren; i++) {
        ASTNode *child = fnArgs->children[i];
        if (child->type == SYM_EXPR) {
            curFnArgCount += 1;
            if (curFnArgCount > ctx->argCount) {
              Error *szError = error_new(ERR_RUNTIME, -1, -1);
              snprintf(szError->message, MAX_ERRMSG_LEN,
                       "Too many arguments provided to function.");
              return value_newError(szError, child->tok);
            }
            // the PARENT context is used to get the value.
            ExecValue *value = execExpr(ctx->parent, child);
            if (value->type == TYPE_ERROR)
              return value;

            value = unpackValue(ctx->parent, value);

            // Assign the value within the function context
            ExecSymbol *fnSym = ctx->symbols[curFnArgCount - 1];
            value_free(fnSym->value);
            fnSym->value = value_clone(value);
            value_free(value);
        } else if (child->type == SYM_TERMINAL && child->tok->type == TOKEN_COMMA) {
            continue;
        } else {
            criticalError("fnArgs: Unexpected child in arglist.");
        }
    }

    // Check if all values in ctx have been assigned
    for (size_t i = 0; i < ctx->symbolCount; i++) {
        ExecSymbol *sym = ctx->symbols[i];
        if (sym->value->type == TYPE_UNASSIGNED) {
            Error *unasErr = error_new(ERR_RUNTIME, -1, -1);
            snprintf(unasErr->message, MAX_ERRMSG_LEN, "Too little arguments provided to function.");
            return value_newError(unasErr, fnArgs->parent->children[0]->tok);
        }
    }

    return value_newNull();
}

ExecValue *execFnCall(Context* ctx, ASTNode *fnCall)
{
    if (fnCall->type != SYM_FN_CALL)
        criticalError("fnCall: Invalid symbol type, expected SYM_FN_CALL");

    if (fnCall->numChildren != 4)
        criticalError("fnCall: Expected 4 children.");
    if (fnCall->children[0]->type != SYM_TERMINAL || fnCall->children[0]->tok->type != TOKEN_IDENTIFIER)
        criticalError("fnCall: Invalid child 1, expected an identifier.");
    
    // Get identifier, check in ctx
    ExecValue *identifier = execTerminal(ctx, fnCall->children[0]);
    ExecValue *val = context_getValue(ctx, identifier);
    if (val == NULL) {
        Error *typeErr = error_new(ERR_RUNTIME_TYPE, -1, -1);
        snprintf(typeErr->message, MAX_ERRMSG_LEN, "No such identifier %s", identifier->value.identifier_name);
        value_free(identifier);
        return value_newError(typeErr, identifier->tok);
    }
    if (val->type != TYPE_FUNCTION) {
        Error *typeErr = error_new(ERR_RUNTIME_TYPE, -1, -1);
        snprintf(typeErr->message, MAX_ERRMSG_LEN, "Identifier %s is not a function.", identifier->value.identifier_name);
        value_free(identifier);
        value_free(val);
        return value_newError(typeErr, identifier->tok);
    }
    FunctionRef *fnRef = val->value.function_ref;
    ASTNode *fnArgList = fnRef->argList;
    ASTNode *fnBlk = fnRef->fnBlk;
    ASTNode *fnArgs = fnCall->children[2];
    if (fnArgList->type != SYM_ARG_LIST)
        criticalError("fnCall: Referenced function ref's arg list is not SYM_ARG_LIST");
    if (fnBlk->type != SYM_BLOCK)
        criticalError("fnCall: Referenced function ref's block is not SYM_BLOCK");
    if (fnArgs->type != SYM_FN_ARGS)
        criticalError("fnCall: Arguments for function call not of type SYM_FN_ARGS");
    
    // if exists, create new context with specific arg count
    Context *fnCtx = context_new(ctx, ctx->global);
    if (ctx->global == NULL)
        fnCtx->global = ctx;
    
    fnCtx->argCount = 0;
    
    // Call linked arglist
    ExecValue *errVal = execArgList(fnCtx, fnArgList);
    if (errVal->type == TYPE_ERROR) {
        value_free(identifier);
        value_free(val);
        return errVal;
    }
    value_free(errVal);
    
    // Call fnargs
    errVal = execFnArgs(fnCtx, fnArgs);
    if (errVal->type == TYPE_ERROR) {
        value_free(identifier);
        value_free(val);
        return errVal;
    }
    value_free(errVal);
    value_free(identifier);
    
    // Call linked block until return
    ExecValue *retVal = execBlock(fnCtx, fnBlk);
    return retVal;
}

ExecValue *execPrimary(Context* ctx, ASTNode *primary)
{
    if (primary->type != SYM_PRIMARY)
        criticalError("primary: Invalid symbol type, expected SYM_PRIMARY");
    
    // Check if primary is ( EXPR ) or TERMINAL or FN_CALL
    if (primary->numChildren == 1) {
        ASTNode *child = primary->children[0];
        if (child->type == SYM_TERMINAL)
            return execTerminal(ctx, child);
        else if (child->type == SYM_FN_CALL)
            return execFnCall(ctx, child);
        
        criticalError("primary: Expected a TERMINAL or FN_CALL.");
        return NULL;
    }
    
    if (primary->numChildren == 3) {
      if (primary->children[0]->tok->type != TOKEN_PAREN_L ||
          primary->children[2]->tok->type != TOKEN_PAREN_R)
        criticalError("primary: Expected ( EXPR ), instead given invalid "
                      "expression with 3 children.");
      return execExpr(ctx, primary->children[1]);
    }

    criticalError("primary: Expected 1 or 3 children.");
    return NULL;
}

ExecValue *execPower(Context* ctx, ASTNode *power)
{
    if (power->type != SYM_POWER)
        criticalError("power: Invalid symbol type, expected SYM_POWER");
    
    // Check if power is PRIMARY ^ UNARY or PRIMARY
    if (power->numChildren == 1)
        return execPrimary(ctx, power->children[0]);
    if (power->numChildren == 3) {
        ExecValue *lVal = execPrimary(ctx, power->children[0]);
        ExecValue *rVal = execUnary(ctx, power->children[2]);
        ExecValue *retVal = NULL;

        lVal = unpackValue(ctx, lVal);
        rVal = unpackValue(ctx, rVal);
        if (lVal->type == TYPE_ERROR) {
            value_free(rVal);
            return lVal;
        } else if (rVal->type == TYPE_ERROR) {
            value_free(lVal);
            return rVal;
        }

        TokenType op = power->children[1]->tok->type;
        if (op != TOKEN_CARET)
            criticalError("power: Unexpected operator, expected ^.");
        retVal = value_opPow(lVal, rVal);
        value_free(lVal); value_free(rVal);
        return retVal;
    }

    criticalError("power: Expected 1 or 3 children.");
    return NULL;
}

ExecValue *execUnary(Context* ctx, ASTNode *unary)
{
    if (unary->type != SYM_UNARY)
        criticalError("unary: Invalid symbol type, expected SYM_UNARY");
    
    // Check if unary is +/- UNARY or POWER
    if (unary->numChildren == 1)
        return execPower(ctx, unary->children[0]);
    if (unary->numChildren == 2) {
        ExecValue *rVal = execUnary(ctx, unary->children[1]);
        ExecValue *retVal = NULL;
        TokenType op = unary->children[0]->tok->type;
        
        rVal = unpackValue(ctx, rVal);
        if (rVal->type == TYPE_ERROR)
            return rVal;

        switch (op) {
        case TOKEN_PLUS:  retVal = value_opUnaryPos(rVal); break;
        case TOKEN_MINUS: retVal = value_opUnaryNeg(rVal); break;
        default:
            criticalError("unary: Unexpected operator.");
        }
        value_free(rVal);
        return retVal;
    }

    criticalError("unary: Expected 1 or 2 children.");
    return NULL;
}

ExecValue *execTerm(Context* ctx, ASTNode *term)
{
    if (term->type != SYM_TERM)
        criticalError("term: Invalid symbol type, expected SYM_TERM.");

    // Check if term is TERM OP UNARY or UNARY
    if (term->numChildren == 1)
        return execUnary(ctx, term->children[0]);
    if (term->numChildren == 3) {
        ExecValue *lVal = execTerm(ctx, term->children[0]);
        ExecValue *rVal = execUnary(ctx, term->children[2]);
        ExecValue *retVal = NULL;

        lVal = unpackValue(ctx, lVal);
        rVal = unpackValue(ctx, rVal);
        if (lVal->type == TYPE_ERROR) {
            value_free(rVal);
            return lVal;
        } else if (rVal->type == TYPE_ERROR) {
            value_free(lVal);
            return rVal;
        }

        TokenType op = term->children[1]->tok->type;

        switch (op) {
        case TOKEN_STAR:    retVal = value_opMul(lVal, rVal); break;
        case TOKEN_SLASH:   retVal = value_opDiv(lVal, rVal); break;
        case TOKEN_PERCENT: retVal = value_opMod(lVal, rVal); break;
        default:
            criticalError("term: Unexpected operator.");
        }
        value_free(lVal); value_free(rVal);
        return retVal;
    }
    criticalError("term: Expected 1 or 3 children.");
    return NULL;
}

ExecValue *execSum(Context* ctx, ASTNode *sum)
{
    if (sum->type != SYM_SUM)
        criticalError("sum: Invalid symbol type, expected SYM_SUM.");

    // Check if sum is SUM OP TERM or SUM
    if (sum->numChildren == 1)
        return execTerm(ctx, sum->children[0]);
    if (sum->numChildren == 3) {
        ExecValue *lVal = execSum(ctx, sum->children[0]);
        ExecValue *rVal = execTerm(ctx, sum->children[2]);
        ExecValue *retVal = NULL;

        lVal = unpackValue(ctx, lVal);
        rVal = unpackValue(ctx, rVal);
        if (lVal->type == TYPE_ERROR) {
            value_free(rVal);
            return lVal;
        } else if (rVal->type == TYPE_ERROR) {
            value_free(lVal);
            return rVal;
        }

        TokenType op = sum->children[1]->tok->type;

        switch (op) {
        case TOKEN_PLUS:  retVal = value_opAdd(lVal, rVal); break;
        case TOKEN_MINUS: retVal = value_opSub(lVal, rVal); break;
        default:
            criticalError("sum: Unexpected operator.");
        }
        value_free(lVal); value_free(rVal);
        return retVal;
    }
    criticalError("sum: Expected 1 or 3 children.");
    return NULL;
}

ExecValue *execComparison(Context* ctx, ASTNode *comparison)
{
    if (comparison->type != SYM_COMPARISON)
        criticalError("comparison: Invalid symbol type, expected SYM_COMPARISON.");

    // Check if comparison is SUM OP COMPARISON or SUM
    if (comparison->numChildren == 1)
        return execSum(ctx, comparison->children[0]);
    if (comparison->numChildren == 3) {
        ExecValue *lVal = execComparison(ctx, comparison->children[0]);
        ExecValue *rVal = execSum(ctx, comparison->children[2]);
        ExecValue *retVal = NULL;

        lVal = unpackValue(ctx, lVal);
        rVal = unpackValue(ctx, rVal);
        if (lVal->type == TYPE_ERROR) {
            value_free(rVal);
            return lVal;
        } else if (rVal->type == TYPE_ERROR) {
            value_free(lVal);
            return rVal;
        }

        TokenType op = comparison->children[1]->tok->type;

        switch (op) {
        case TOKEN_GREATER:       retVal = value_opGt(lVal, rVal); break;
        case TOKEN_GREATER_EQUAL: retVal = value_opGEq(lVal, rVal); break;
        case TOKEN_LESS:          retVal = value_opLt(lVal, rVal); break;
        case TOKEN_LESS_EQUAL:    retVal = value_opLEq(lVal, rVal); break;
        default:
            criticalError("comparison: Unexpected operator.");
        }
        value_free(lVal); value_free(rVal);
        return retVal;
    }
    criticalError("comparison: Expected 1 or 3 children.");
    return NULL;
}

ExecValue *execEquality(Context* ctx, ASTNode *equality)
{
    if (equality->type != SYM_EQUALITY)
        criticalError("equality: Invalid symbol type, expected SYM_EQUALITY.");

    // Check if equality is COMPARISON OP EQUALITY or COMPARISON
    if (equality->numChildren == 1)
        return execComparison(ctx, equality->children[0]);
    if (equality->numChildren == 3) {
        ExecValue *lVal = execEquality(ctx, equality->children[0]);
        ExecValue *rVal = execComparison(ctx, equality->children[2]);
        ExecValue *retVal = NULL;

        lVal = unpackValue(ctx, lVal);
        rVal = unpackValue(ctx, rVal);
        if (lVal->type == TYPE_ERROR) {
            value_free(rVal);
            return lVal;
        } else if (rVal->type == TYPE_ERROR) {
            value_free(lVal);
            return rVal;
        }

        TokenType op = equality->children[1]->tok->type;

        switch (op) {
        case TOKEN_EQUAL_EQUAL: retVal = value_opEqEq(lVal, rVal); break;
        case TOKEN_BANG_EQUAL:  retVal = value_opNEq(lVal, rVal); break;
        default:
            criticalError("equality: Unexpected operator.");
        }
        value_free(lVal); value_free(rVal);
        return retVal;
    }
    criticalError("equality: Expected 1 or 3 children.");
    return NULL;
}
ExecValue* execLogUnary(Context* ctx, ASTNode* logUnary)
{
    if (logUnary->type != SYM_LOG_UNARY)
        criticalError("logUnary: Invalid symbol type, expected SYM_LOG_UNARY");

    // Check if expr is not LOG_UNARY | EQUALITY
    if (logUnary->numChildren == 1)
        return execEquality(ctx, logUnary->children[0]);
    if (logUnary->numChildren == 2) {
        ExecValue *rVal = execLogUnary(ctx, logUnary->children[1]);
        ExecValue *retVal = NULL;
        TokenType op = logUnary->children[0]->tok->type;
        
        rVal = unpackValue(ctx, rVal);
        if (rVal->type == TYPE_ERROR)
            return rVal;

        if (op == TOKEN_NOT)
            retVal = value_opNot(rVal);
        else
            criticalError("logUnary: Unexpected operator.");

        value_free(rVal);
        return retVal;
    }

    criticalError("logUnary: Expected 1 or 2 children.");
    return NULL;
}

ExecValue* execAndExpr(Context* ctx, ASTNode* andExpr)
{
    if (andExpr->type != SYM_AND_EXPR)
        criticalError("andExpr: Invalid symbol type, expected SYM_AND_EXPR");

    // Check if expr is AND_EXPR and LOG_UNARY | LOG_UNARY
    if (andExpr->numChildren == 1)
        return execLogUnary(ctx, andExpr->children[0]);
    if (andExpr->numChildren == 3) {
        ExecValue *lVal = execAndExpr(ctx, andExpr->children[0]);
        ExecValue *rVal = execLogUnary(ctx, andExpr->children[2]);
        ExecValue *retVal = NULL;

        lVal = unpackValue(ctx, lVal);
        rVal = unpackValue(ctx, rVal);
        if (lVal->type == TYPE_ERROR) {
            value_free(rVal);
            return lVal;
        } else if (rVal->type == TYPE_ERROR) {
            value_free(lVal);
            return rVal;
        }

        retVal = value_opAnd(lVal, rVal);
        value_free(lVal); value_free(rVal);

        return retVal;
    }
    criticalError("andExpr: Expected 1 or 3 children.");
    return NULL;
}

ExecValue* execOrExpr(Context* ctx, ASTNode* orExpr)
{
    if (orExpr->type != SYM_OR_EXPR)
        criticalError("orExpr: Invalid symbol type, expected SYM_OR_EXPR");

    // Check if expr is OR_EXPR or AND_EXPR  |  AND_EXPR
    if (orExpr->numChildren == 1)
        return execAndExpr(ctx, orExpr->children[0]);
    if (orExpr->numChildren == 3) {
        ExecValue *lVal = execOrExpr(ctx, orExpr->children[0]);
        ExecValue *rVal = execAndExpr(ctx, orExpr->children[2]);
        ExecValue *retVal = NULL;

        lVal = unpackValue(ctx, lVal);
        rVal = unpackValue(ctx, rVal);
        if (lVal->type == TYPE_ERROR) {
            value_free(rVal);
            return lVal;
        } else if (rVal->type == TYPE_ERROR) {
            value_free(lVal);
            return rVal;
        }

        retVal = value_opOr(lVal, rVal);
        value_free(lVal); value_free(rVal);

        return retVal;
    }
    criticalError("orExpr: Expected 1 or 3 children.");
    return NULL;
}

ExecValue* execArg(Context* ctx, ASTNode* arg)
{
    if (arg->type != SYM_ARG)
        criticalError("arg: Invalid symbol type, expected SYM_ARG");

    // Could be IDENTIFIER or IDENTIFIER = TERMINAL
    if (arg->numChildren == 1) {
        ExecValue *identifier = execTerminal(ctx, arg->children[0]);
        
        if (context_getSymbol(ctx, identifier) != NULL) {
            Error *execError = error_new(ERR_RUNTIME, -1, -1);
            snprintf(execError->message, MAX_ERRMSG_LEN, "Function parameter has the same identifier name \"%s\"", identifier->value.identifier_name);
            ExecValue *errVal = value_newError(execError, identifier->tok);
            value_free(identifier);
            return errVal;
        }
        context_addSymbol(ctx, identifier);
    } else if (arg->numChildren == 3) {
        ExecValue *identifier = execTerminal(ctx, arg->children[0]);
        ExecValue *defaultValue = execTerminal(ctx, arg->children[1]);
        if (arg->children[1]->type != SYM_TERMINAL || arg->children[1]->tok->type != TOKEN_EQUAL)
            criticalError("arg: Second child of assignment not an equals.");
        
        if (context_getSymbol(ctx, identifier) != NULL) {
            Error *execError = error_new(ERR_RUNTIME, -1, -1);
            snprintf(execError->message, MAX_ERRMSG_LEN, "Function parameter has the same identifier name \"%s\"", identifier->value.identifier_name);
            ExecValue *errVal = value_newError(execError, identifier->tok);
            value_free(identifier);
            return errVal;
        }
        context_addSymbol(ctx, identifier);
        context_setSymbol(ctx, identifier, defaultValue);
        value_free(defaultValue);
    }
    
    return value_newNull();
}

// Called by the function call to add the context variables
ExecValue* execArgList(Context* ctx, ASTNode* argList)
{
    if (argList->type != SYM_ARG_LIST)
        criticalError("argList: Invalid symbol type, expected SYM_ARG_LIST");

    // Add each symbol to the list
    for (size_t i = 0; i < argList->numChildren; i++) {
        ASTNode *child = argList->children[i];
        ExecValue *errVal;
        if (child->type == SYM_ARG) {
            errVal = execArg(ctx, child);
            ctx->argCount += 1;
        } else if (child->type == SYM_TERMINAL && child->tok->type == TOKEN_COMMA) {
            continue;
        } else {
            criticalError("argList: Unexpected child in arglist.");
        }
        if (errVal->type == TYPE_ERROR)
            return errVal;
        
        value_free(errVal);
    }
    
    return value_newNull();
}

// Returns the function reference variable, with the argList and block. This will be stored in the current context
ExecValue* execFnExpr(Context* ctx, ASTNode* fnExpr)
{
    if (fnExpr->type != SYM_FN_EXPR)
        criticalError("fnExpr: Invalid symbol type, expected SYM_FN_EXPR");

    // Validate the various symbols first
    if (fnExpr->numChildren != 8)
        criticalError("fnExpr: Expected 8 children.");
    if (fnExpr->children[2]->type != SYM_ARG_LIST || fnExpr->children[5]->type != SYM_BLOCK)
        criticalError("fnExpr: Expected child 2 to be ARG_LIST, child 5 to be BLOCK.");
    
    ASTNode *argList = fnExpr->children[2];
    ASTNode *block = fnExpr->children[5];
    
    return value_newFunction(argList, block, fnExpr->tok);
}

ExecValue *execExpr(Context* ctx, ASTNode *expr)
{
    if (expr->type != SYM_EXPR)
        criticalError("expr: Invalid symbol type, expected SYM_EXPR");

    if (expr->numChildren != 1)
        criticalError("expr: Expected 1 child.");

    if (expr->children[0]->type == SYM_OR_EXPR)
        return execOrExpr(ctx, expr->children[0]);
    else if(expr->children[0]->type == SYM_FN_EXPR)
        return execFnExpr(ctx, expr->children[0]);
    criticalError("expr: Expected a child.");
    return NULL;
}

ExecValue *execPrntStmt(Context* ctx, ASTNode *prntStmt)
{
    if (prntStmt->type != SYM_PRNT_STMT)
        criticalError("prntStmt: Invalid symbol type, expected SYM_PRNT_STMT");
    if (prntStmt->numChildren == 1 && prntStmt->children[0]->tok->type == TOKEN_PRINT) {
        log_message(&consoleLogger,"\n");
        log_message(&executionLogger,"\n");
        log_message(&resultLogger,"\n");
        return value_newNull();
    }
    if (prntStmt->numChildren == 3 &&
	prntStmt->children[0]->tok->type == TOKEN_PRINT &&
	prntStmt->children[1]->type == SYM_EXPR &&
	prntStmt->children[2]->tok->type == TOKEN_NL) {
		ExecValue *exprResult = execExpr(ctx, prntStmt->children[1]);
        
        exprResult = unpackValue(ctx, exprResult);
        if (exprResult->type == TYPE_ERROR)
            return exprResult;
		
		switch (exprResult->type) {
        case TYPE_IDENTIFIER:
            criticalError("prntStmt: Identifier's value was an identifier.");
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
        default:
            criticalError("prntStmt: Unexpected type in exprResult.");
        }
		value_free(exprResult);
		return value_newNull();
    }
    criticalError("prntstmt: Invalid print statement.");
    return NULL;
}

ExecValue *execExprStmt(Context* ctx, ASTNode *exprStmt)
{
    if (exprStmt->type != SYM_EXPR_STMT)
        criticalError("exprStmt: Invalid symbol type, expected SYM_EXPR_STMT");
    if (exprStmt->numChildren == 2 &&
        exprStmt->children[0]->type == SYM_EXPR &&
        exprStmt->children[1]->tok->type == TOKEN_NL) {
        ExecValue *exprResult = execExpr(ctx, exprStmt->children[0]);
        if (exprResult->type == TYPE_ERROR)
            return exprResult;
        value_free(exprResult);
        return value_newNull();
    }
    criticalError("exprStmt: Invalid exprStmt.");
    return NULL;
}

ExecValue *execReturn(Context *ctx, ASTNode *ret)
{
    if (ret->type != SYM_RETURN)
        criticalError("return: Invalid symbol type, expected SYM_RETURN");

    ctx->hasReturn = 1;

    if (ret->numChildren == 2)
        return value_newNull();
    if (ret->numChildren == 3)
        return execExpr(ctx, ret->children[1]);

    criticalError("return: Expected 2 or 3 children.");
    return NULL;
}

ExecValue *execBlock(Context* ctx, ASTNode *block)
{
    if (block->type != SYM_BLOCK)
        criticalError("block: Invalid symbol type, expected SYM_BLOCK");
    
	for (int i = 0; i < block->numChildren; i++){
        ExecValue *result = execLine(ctx, block->children[i]);
        if (result->type == TYPE_ERROR)
            return result;

        if (ctx->hasReturn) {
            result = unpackValue(ctx, result);
            return result;
        }
        value_free(result);
	}
	return value_newNull();
}

ExecValue *execElse(Context* ctx, ASTNode *elseStmt){
    if (elseStmt->type != SYM_ELSE)
        criticalError("elsestmt: Invalid symbol type, expected SYM_ELSE");
    
	if (elseStmt->numChildren == 3)
		return execBlock(ctx, elseStmt->children[2]);
	else
		criticalError("execElse: Invalid else.");

    return value_newNull();
}

ExecValue *execElseIf(Context* ctx, ASTNode *elseIfStmt){
    if (elseIfStmt->type != SYM_ELSEIF)
        criticalError("elseifstmt: Invalid symbol type, expected SYM_ELSEIF");

    ExecValue *expr = execExpr(ctx, elseIfStmt->children[2]);
    expr = unpackValue(ctx, expr);
    if (expr->type == TYPE_ERROR)
        return expr;
    if (value_falsiness(expr) == 1) { // true branch
        return execBlock(ctx, elseIfStmt->children[5]);
    } else {
        if (elseIfStmt->numChildren == 7) {
            if (elseIfStmt->children[6]->type == SYM_ELSEIF)
                return execElseIf(ctx, elseIfStmt->children[6]);
            else if (elseIfStmt->children[6]->type == SYM_ELSE)
                return execElse(ctx, elseIfStmt->children[6]);

            criticalError("execElseIf: Invalid branch -- not else if, or else");
        }
    }
    return value_newNull();
}

ExecValue *execIfStmt(Context* ctx, ASTNode *ifStmt)
{
    if (ifStmt->type != SYM_IFSTMT)
        criticalError("ifstmt: Invalid symbol type, expected SYM_IFSTMT");

    ExecValue *expr = execExpr(ctx, ifStmt->children[1]);
    expr = unpackValue(ctx, expr);
    if (expr->type == TYPE_ERROR)
        return expr;
    if (value_falsiness(expr) == 1) { // true branch
        value_free(expr);
        return execBlock(ctx, ifStmt->children[4]);
    } else {
        value_free(expr);
        if (ifStmt->children[5]->type == SYM_ELSEIF)
            return execElseIf(ctx, ifStmt->children[5]);
        else if (ifStmt->children[5]->type == SYM_ELSE)
            return execElse(ctx, ifStmt->children[5]);
    }
    return value_newNull();
}

ExecValue *execWhileStmt(Context* ctx, ASTNode *whileStmt)
{
    if (whileStmt->type != SYM_WHILE)
        criticalError("ifstmt: Invalid symbol type, expected SYM_WHILE");
    ExecValue *expr = execExpr(ctx, whileStmt->children[1]);
    expr = unpackValue(ctx, expr);
    if (expr->type == TYPE_ERROR)
        return expr;
    while (value_falsiness(expr) == 1){
        ExecValue *blockErr = execBlock(ctx, whileStmt->children[3]);
        if (blockErr->type == TYPE_ERROR) {
            value_free(expr);
            return blockErr;
        }
        value_free(expr);
        expr = execExpr(ctx, whileStmt->children[1]);
        expr = unpackValue(ctx, expr);
        if (expr->type == TYPE_ERROR) {
            value_free(blockErr);
            return expr;
        }
        if (ctx->hasBreakOrContinue == 1){
            ctx->hasBreakOrContinue = 0;
            break;
        } else if (ctx->hasBreakOrContinue == 2)
            ctx->hasBreakOrContinue = 0;

        value_free(blockErr);
    }
    value_free(expr);
    return value_newNull();
}

ExecValue *execBreak(Context* ctx, ASTNode *breakStmt)
{
    if (breakStmt->type != SYM_BREAK)
        criticalError("ifstmt: Invalid symbol type, expected SYM_BREAK");
    ctx->hasBreakOrContinue = 1;
    return value_newNull();
}

ExecValue *execContinue(Context* ctx, ASTNode *breakStmt)
{
    if (breakStmt->type != SYM_CONTINUE)
        criticalError("ifstmt: Invalid symbol type, expected SYM_CONTINUE");
    ctx->hasBreakOrContinue = 2;
    return value_newNull();
}

ExecValue *execStmt(Context* ctx, ASTNode *stmt)
{
    if (stmt->type != SYM_STMT)
        criticalError("stmt: Invalid symbol type, expected SYM_STMT");
    if (ctx->hasBreakOrContinue)
        return value_newNull();
	if (stmt->numChildren == 1) {
		if (stmt->children[0]->type == SYM_EXPR_STMT)
			return execExprStmt(ctx, stmt->children[0]);
		if (stmt->children[0]->type == SYM_PRNT_STMT)
			return execPrntStmt(ctx, stmt->children[0]);
		if (stmt->children[0]->type == SYM_IFSTMT)
			return execIfStmt(ctx, stmt->children[0]);
		if (stmt->children[0]->type == SYM_BREAK)
			return execBreak(ctx, stmt->children[0]);
        if (stmt->children[0]->type == SYM_CONTINUE)
			return execContinue(ctx, stmt->children[0]);
		if (stmt->children[0]->type == SYM_WHILE)
			return execWhileStmt(ctx, stmt->children[0]);
		if (stmt->children[0]->type == SYM_RETURN)
			return execReturn(ctx, stmt->children[0]);
		
    }
    criticalError("stmt: Invalid statement.");
    return NULL;
}

ExecValue *execAsmt(Context* ctx, ASTNode *asmt)
{
    if (asmt->type != SYM_ASMT)
        criticalError("asmt: Invalid symbol type, expected SYM_ASMT");
    if (ctx->hasBreakOrContinue)
        return value_newNull();
    if (asmt->numChildren == 4 &&
        asmt->children[0]->tok->type == TOKEN_IDENTIFIER &&
        asmt->children[1]->tok->type == TOKEN_EQUAL &&
        asmt->children[2]->type == SYM_EXPR &&
        asmt->children[3]->tok->type == TOKEN_NL) {
        ExecValue *lvalue = execTerminal(ctx, asmt->children[0]);
        ExecValue *rvalue = execExpr(ctx, asmt->children[2]);
        rvalue = unpackValue(ctx, rvalue);

        if (lvalue->type == TYPE_ERROR) {
            value_free(rvalue);
            return lvalue;
        } else if (rvalue->type == TYPE_ERROR) {
            value_free(lvalue);
            return rvalue;
        }
        
        // There's no explicit declaration in Miniscript, so we check the symbol table -- if it isn't there, we declare it
        ExecSymbol *sym = context_getSymbol(ctx, lvalue);
        if (sym == NULL)
            context_addSymbol(ctx, lvalue);

        context_setSymbol(ctx, lvalue, rvalue);
        value_free(lvalue); value_free(rvalue);
        return value_newNull();
    }
    criticalError("asmt: Invalid assignment.");
    return NULL;
}

ExecValue *execLine(Context* ctx, ASTNode *line)
{
    if (line->type != SYM_LINE)
        criticalError("line: Invalid symbol type, expected SYM_LINE");
    if (ctx->hasBreakOrContinue)
        return value_newNull();
    if (line->numChildren == 1) {
        if (line->children[0]->type == SYM_ASMT)
            return execAsmt(ctx, line->children[0]);
        if (line->children[0]->type == SYM_STMT)
            return execStmt(ctx, line->children[0]);
        criticalError("line: Line has invalid children.");
    }
    criticalError("line: Expected line to have 1 child.");
    return NULL;
}

ExecValue *execStart(Context* ctx, ASTNode *start)
{
    // Returns the execution exit code
    int exitCode = 0;
    for (size_t i = 0; i < start->numChildren; i++) {
        ASTNode *child = start->children[i];
        ExecValue *result;
        if (child->type == SYM_LINE)
            result = execLine(ctx, child);
        else if (child->tok->type == TOKEN_EOF)
            break;
        else
            criticalError("Unexpected symbol.");

        if (result->type == TYPE_ERROR)
            return result;

        value_free(result);
    }

    return value_newNull();
}
