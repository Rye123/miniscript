#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "executor.h"
#include "symboltable.h"

Context *context_new(Context *parent, Context *global)
{
    Context *ctx = malloc(sizeof(Context));
    ctx->global = global;
    ctx->parent = parent;
    ctx->symbols = malloc(0);
    ctx->symbolCount = 0;
    return ctx;
}

void context_addSymbol(Context *ctx, ExecValue *identifier)
{
    ExecSymbol *sym = malloc(sizeof(ExecSymbol));
    if (identifier->type != TYPE_IDENTIFIER) {
	printf("Tried to add a symbol for a non-identifier.\n");
	exit(1);
    }
	
    char *identifierName = identifier->value.identifier_name;
    sym->symbolName = strdup(identifierName);
    sym->value = malloc(sizeof(ExecValue));

    ctx->symbolCount = ctx->symbolCount + 1;
    ctx->symbols = realloc(ctx->symbols, ctx->symbolCount * sizeof(ExecSymbol*));
    ctx->symbols[ctx->symbolCount-1] = sym;
    printf("Current count: %lu\n", ctx->symbolCount);
}


ExecValue *criticalError(char *msg)
{
    printf("Critical Error: %s\n", msg);
    exit(1);
    return value_newNull();
}

ExecValue *executionError(char *msg) //TODO: return an ERROR value with a copy of the string, and have checks in each execution function
{
    printf("Runtime Error: %s\n", msg);
    exit(1); //TODO: remove when the above is implemented
    return value_newNull();
}

ExecValue *value_newNull()
{
    ExecValue* val = malloc(sizeof(ExecValue));
    val->type = TYPE_NULL;
    val->value.literal_null = NULL;
    val->metadata = -1;
    return val;
}

ExecValue *value_newString(char *strValue)
{
    ExecValue* val = malloc(sizeof(ExecValue));
    val->type = TYPE_STRING;

    // Create null-terminated copy of the string
    char *str_cpy = strdup(strValue);
    val->value.literal_str = str_cpy;
    val->metadata = -1;
    return val;
}

ExecValue *value_newNumber(double numValue)
{
    ExecValue* val = malloc(sizeof(ExecValue));
    val->type = TYPE_NUMBER;
    val->value.literal_num = numValue;
    val->metadata = -1;
    return val;
}

ExecValue *value_newIdentifier(char *identifierName)
{
    ExecValue* val = malloc(sizeof(ExecValue));
    val->type = TYPE_IDENTIFIER;

    // Create null-terminated copy of the identifier name
    char *name_cpy = strdup(identifierName);
    val->value.identifier_name = name_cpy;
    return val;
}

ExecSymbol *context_getSymbol(Context *ctx, ExecValue *identifier)
{
    if (identifier->type != TYPE_IDENTIFIER) {
	printf("Tried to get a symbol for a non-identifier.\n");
	exit(1);
    }
    char *symbolName = identifier->value.identifier_name;
    size_t expLen = strlen(symbolName);
    for (size_t i = 0; i < ctx->symbolCount; i++) {
	if (strlen(ctx->symbols[i]->symbolName) != expLen)
	    continue;
        if (strncmp(ctx->symbols[i]->symbolName, symbolName, expLen) == 0)
	    return ctx->symbols[i];
    }
    return NULL;
}

ExecSymbol *context_getSymbolWalk(Context *ctx, ExecValue *identifier)
{
    ExecSymbol *sym = context_getSymbol(ctx, identifier);
    if (sym == NULL && ctx->global != NULL)
	sym = context_getSymbol(ctx->global, identifier);
    return NULL;
}

ExecValue *context_getValue(Context *ctx, ExecValue *identifier)
{
    ExecSymbol *sym = context_getSymbol(ctx, identifier);
    if (sym == NULL)
	return NULL;
    ExecValue *val = malloc(sizeof(ExecValue));
    memcpy(val, sym->value, sizeof(ExecValue));
    return val;
}

void context_setSymbol(Context *ctx, ExecValue *identifier, ExecValue *value)
{
    ExecSymbol *sym = context_getSymbol(ctx, identifier);
    if (sym == NULL) {
	printf("Execution Error: Could not set value of undeclared variable %s.\n", sym->symbolName);
	exit(1);
    }

    // Copy the contents of the value
    sym->value = memcpy(sym->value, value, sizeof(ExecValue));
    printf("Symbol %s set to %f\n", sym->symbolName, sym->value->value.literal_num);
}

void context_free(Context *ctx)
{
    for (size_t i = 0; i < ctx->symbolCount; i++)
	free(ctx->symbols[i]);
    free(ctx->symbols);
    free(ctx);
}

ExecValue* value_opUnaryPos(ExecValue *e)
{
    if (e == NULL)
	return criticalError("pos: e1 or e2 is null");
    if (e->type == TYPE_NULL || e->type == TYPE_STRING)
	return executionError("TypeError: unary positive expects a number.");
    // Don't handle identifiers, so that the caller is responsible for managing memory of e1 and e2
    if (e->type == TYPE_IDENTIFIER)
	return criticalError("pos: pass the VALUE of the identifier into this function with context_getValue(), instead of the identifier itself.");

    double result = e->value.literal_num;
    return value_newNumber(result);
}

ExecValue* value_opUnaryNeg(ExecValue *e)
{
    if (e == NULL)
	return criticalError("neg: e1 or e2 is null");
    if (e->type == TYPE_NULL || e->type == TYPE_STRING)
	return executionError("TypeError: unary negative expects a number.");
    // Don't handle identifiers, so that the caller is responsible for managing memory of e1 and e2
    if (e->type == TYPE_IDENTIFIER)
	return criticalError("neg: pass the VALUE of the identifier into this function with context_getValue(), instead of the identifier itself.");

    double result = -e->value.literal_num;
    return value_newNumber(result);
}

ExecValue* value_opAdd(ExecValue *e1, ExecValue *e2)
{
    if (e1 == NULL || e2 == NULL)
	return criticalError("add: e1 or e2 is null");
    if (e1->type == TYPE_NULL || e2->type == TYPE_NULL)
	return executionError("TypeError: addition expects no null values.");
    // Don't handle identifiers, so that the caller is responsible for managing memory of e1 and e2
    if (e1->type == TYPE_IDENTIFIER || e2->type == TYPE_IDENTIFIER)
	return criticalError("add: pass the VALUE of the identifier into this function with context_getValue(), instead of the identifier itself.");

    if (e1->type == TYPE_NUMBER && e2->type == TYPE_NUMBER) {
	double result = e1->value.literal_num + e2->value.literal_num;
	return value_newNumber(result);
    } else if (e1->type == TYPE_STRING && e2->type == TYPE_STRING) {
	char *s1 = e1->value.literal_str;
	char *s2 = e2->value.literal_str;
	size_t resultLen = strlen(s1) + strlen(s2);
	char *result = malloc(resultLen + 1);
	strncpy(result, s1, strlen(s1));
	strncpy(result + strlen(s1), s2, strlen(s2));
	result[resultLen] = '\0';
	ExecValue *resultVal = value_newString(result);
	free(result);
	return resultVal;
    } 
    return executionError("TypeError: addition expects two strings or two numbers.");
}

ExecValue* value_opSub(ExecValue *e1, ExecValue *e2)
{
    if (e1 == NULL || e2 == NULL)
	return criticalError("sub: e1 or e2 is null");
    if (e1->type == TYPE_NULL || e2->type == TYPE_NULL)
	return executionError("TypeError: subtraction expects no null values.");
    if (e1->type == TYPE_STRING || e2->type == TYPE_STRING)
	return executionError("TypeError: subtraction expects no string values.");
    // Don't handle identifiers, so that the caller is responsible for managing memory of e1 and e2
    if (e1->type == TYPE_IDENTIFIER || e2->type == TYPE_IDENTIFIER)
	return criticalError("sub: pass the VALUE of the identifier into this function with context_getValue(), instead of the identifier itself.");

    if (e1->type == TYPE_NUMBER && e2->type == TYPE_NUMBER) {
	double result = e1->value.literal_num - e2->value.literal_num;
	return value_newNumber(result);
    }
    return executionError("TypeError: subtraction expects two numbers.");
}

ExecValue* value_opMul(ExecValue *e1, ExecValue *e2)
{
    if (e1 == NULL || e2 == NULL)
	return criticalError("mul: e1 or e2 is null");
    if (e1->type == TYPE_NULL || e2->type == TYPE_NULL)
	return executionError("TypeError: multiplication expects no null values.");
    if (e1->type == TYPE_STRING || e2->type == TYPE_STRING)
	return executionError("TypeError: multiplication expects no string values.");
    // Don't handle identifiers, so that the caller is responsible for managing memory of e1 and e2
    if (e1->type == TYPE_IDENTIFIER || e2->type == TYPE_IDENTIFIER)
	return criticalError("mul: pass the VALUE of the identifier into this function with context_getValue(), instead of the identifier itself.");

    if (e1->type == TYPE_NUMBER && e2->type == TYPE_NUMBER) {
	double result = e1->value.literal_num * e2->value.literal_num;
	return value_newNumber(result);
    }
    return executionError("TypeError: multiplication expects two numbers.");
}

ExecValue* value_opDiv(ExecValue *e1, ExecValue *e2)
{
    if (e1 == NULL || e2 == NULL)
	return criticalError("div: e1 or e2 is null");
    if (e1->type == TYPE_NULL || e2->type == TYPE_NULL)
	return executionError("TypeError: division expects no null values.");
    if (e1->type == TYPE_STRING || e2->type == TYPE_STRING)
	return executionError("TypeError: division expects no string values.");
    // Don't handle identifiers, so that the caller is responsible for managing memory of e1 and e2
    if (e1->type == TYPE_IDENTIFIER || e2->type == TYPE_IDENTIFIER)
	return criticalError("div: pass the VALUE of the identifier into this function with context_getValue(), instead of the identifier itself.");

    if (e1->type == TYPE_NUMBER && e2->type == TYPE_NUMBER) {
	if (e2->value.literal_num == 0.0)
	    return executionError("ValueError: division by zero");
	double result = e1->value.literal_num / e2->value.literal_num;
	return value_newNumber(result);
    }
    return executionError("TypeError: division expects two numbers.");
}

ExecValue* value_opMod(ExecValue *e1, ExecValue *e2)
{
    if (e1 == NULL || e2 == NULL)
	return criticalError("mod: e1 or e2 is null");
    if (e1->type == TYPE_NULL || e2->type == TYPE_NULL)
	return executionError("TypeError: modulo expects no null values.");
    if (e1->type == TYPE_STRING || e2->type == TYPE_STRING)
	return executionError("TypeError: modulo expects no string values.");
    // Don't handle identifiers, so that the caller is responsible for managing memory of e1 and e2
    if (e1->type == TYPE_IDENTIFIER || e2->type == TYPE_IDENTIFIER)
	return criticalError("mod: pass the VALUE of the identifier into this function with context_getValue(), instead of the identifier itself.");

    if (e1->type == TYPE_NUMBER && e2->type == TYPE_NUMBER) {
	if (e2->value.literal_num == 0.0)
	    return executionError("ValueError: modulo by zero");
	double result = fmod(e1->value.literal_num, e2->value.literal_num);
	return value_newNumber(result);
    }
    return executionError("TypeError: modulo expects two numbers.");
}

ExecValue* value_opPow(ExecValue *e1, ExecValue *e2)
{
    if (e1 == NULL || e2 == NULL)
	return criticalError("pow: e1 or e2 is null");
    if (e1->type == TYPE_NULL || e2->type == TYPE_NULL)
	return executionError("TypeError: power expects no null values.");
    if (e1->type == TYPE_STRING || e2->type == TYPE_STRING)
	return executionError("TypeError: power expects no string values.");
    // Don't handle identifiers, so that the caller is responsible for managing memory of e1 and e2
    if (e1->type == TYPE_IDENTIFIER || e2->type == TYPE_IDENTIFIER)
	return criticalError("pow: pass the VALUE of the identifier into this function with context_getValue(), instead of the identifier itself.");

    if (e1->type == TYPE_NUMBER && e2->type == TYPE_NUMBER) {
	double result = pow(e1->value.literal_num, e2->value.literal_num);
	return value_newNumber(result);
    }
    return executionError("TypeError: power expects two numbers.");
}

ExecValue* value_opEqEq(ExecValue *e1, ExecValue *e2)
{
    if (e1 == NULL || e2 == NULL)
	return criticalError("eq: e1 or e2 is null");
    if (e1->type == TYPE_NULL || e2->type == TYPE_NULL)
	return executionError("TypeError: equality expects no null values.");
    // Don't handle identifiers, so that the caller is responsible for managing memory of e1 and e2
    if (e1->type == TYPE_IDENTIFIER || e2->type == TYPE_IDENTIFIER)
	return criticalError("eq: pass the VALUE of the identifier into this function with context_getValue(), instead of the identifier itself.");

    if (e1->type == TYPE_NUMBER && e2->type == TYPE_NUMBER) {
	double result = e1->value.literal_num == e2->value.literal_num;
	return value_newNumber(result);
    } else if (e1->type == TYPE_STRING && e2->type == TYPE_STRING) {
	char *s1 = e1->value.literal_str;
	char *s2 = e2->value.literal_str;
	double result = 1;

	if (strlen(s1) != strlen(s2))
	    result = 0;
	else
	    result = (strncmp(s1, s2, strlen(s1)) == 0) ? 1 : 0;
	return value_newNumber(result);
    } 
    return executionError("TypeError: equality expects two strings or two numbers.");
}

ExecValue* value_opNEq(ExecValue *e1, ExecValue *e2)
{
    if (e1 == NULL || e2 == NULL)
	return criticalError("neq: e1 or e2 is null");
    if (e1->type == TYPE_NULL || e2->type == TYPE_NULL)
	return executionError("TypeError: inequality expects no null values.");
    // Don't handle identifiers, so that the caller is responsible for managing memory of e1 and e2
    if (e1->type == TYPE_IDENTIFIER || e2->type == TYPE_IDENTIFIER)
	return criticalError("neq: pass the VALUE of the identifier into this function with context_getValue(), instead of the identifier itself.");

    if ((e1->type == TYPE_NUMBER && e2->type == TYPE_NUMBER) ||
	(e1->type == TYPE_STRING && e2->type == TYPE_STRING)) {
	ExecValue *eqeqRes = value_opEqEq(e1, e2);
	eqeqRes->value.literal_num = !eqeqRes->value.literal_num;
	return eqeqRes;
    }
    return executionError("TypeError: inequality expects two strings or two numbers.");
}

ExecValue* value_opGt(ExecValue *e1, ExecValue *e2)
{
    if (e1 == NULL || e2 == NULL)
	return criticalError("gt: e1 or e2 is null");
    if (e1->type == TYPE_NULL || e2->type == TYPE_NULL)
	return executionError("TypeError: greaterThan expects no null values.");
    // Don't handle identifiers, so that the caller is responsible for managing memory of e1 and e2
    if (e1->type == TYPE_IDENTIFIER || e2->type == TYPE_IDENTIFIER)
	return criticalError("gt: pass the VALUE of the identifier into this function with context_getValue(), instead of the identifier itself.");

    if (e1->type == TYPE_NUMBER && e2->type == TYPE_NUMBER) {
	double result = e1->value.literal_num > e2->value.literal_num;
	return value_newNumber(result);
    } else if (e1->type == TYPE_STRING && e2->type == TYPE_STRING) {
	char *s1 = e1->value.literal_str;
	char *s2 = e2->value.literal_str;
	return value_newNumber(strlen(s1) > strlen(s2));
    } 
    return executionError("TypeError: greaterThan expects two strings or two numbers.");
}

ExecValue* value_opGEq(ExecValue *e1, ExecValue *e2)
{
    if (e1 == NULL || e2 == NULL)
	return criticalError("geq: e1 or e2 is null");
    if (e1->type == TYPE_NULL || e2->type == TYPE_NULL)
	return executionError("TypeError: greaterThanOrEqual expects no null values.");
    // Don't handle identifiers, so that the caller is responsible for managing memory of e1 and e2
    if (e1->type == TYPE_IDENTIFIER || e2->type == TYPE_IDENTIFIER)
	return criticalError("geq: pass the VALUE of the identifier into this function with context_getValue(), instead of the identifier itself.");

    if (e1->type == TYPE_NUMBER && e2->type == TYPE_NUMBER) {
	double result = e1->value.literal_num >= e2->value.literal_num;
	return value_newNumber(result);
    } else if (e1->type == TYPE_STRING && e2->type == TYPE_STRING) {
	char *s1 = e1->value.literal_str;
	char *s2 = e2->value.literal_str;
	return value_newNumber(strlen(s1) >= strlen(s2));
    } 
    return executionError("TypeError: greaterThanOrEqual expects two strings or two numbers.");
}

ExecValue* value_opLt(ExecValue *e1, ExecValue *e2)
{
    if (e1 == NULL || e2 == NULL)
	return criticalError("lt: e1 or e2 is null");
    if (e1->type == TYPE_NULL || e2->type == TYPE_NULL)
	return executionError("TypeError: lessThan expects no null values.");
    // Don't handle identifiers, so that the caller is responsible for managing memory of e1 and e2
    if (e1->type == TYPE_IDENTIFIER || e2->type == TYPE_IDENTIFIER)
	return criticalError("lt: pass the VALUE of the identifier into this function with context_getValue(), instead of the identifier itself.");

    if (e1->type == TYPE_NUMBER && e2->type == TYPE_NUMBER) {
	double result = e1->value.literal_num < e2->value.literal_num;
	return value_newNumber(result);
    } else if (e1->type == TYPE_STRING && e2->type == TYPE_STRING) {
	char *s1 = e1->value.literal_str;
	char *s2 = e2->value.literal_str;
	return value_newNumber(strlen(s1) < strlen(s2));
    } 
    return executionError("TypeError: lessThan expects two strings or two numbers.");
}

ExecValue* value_opLEq(ExecValue *e1, ExecValue *e2)
{
    if (e1 == NULL || e2 == NULL)
	return criticalError("leq: e1 or e2 is null");
    if (e1->type == TYPE_NULL || e2->type == TYPE_NULL)
	return executionError("TypeError: lessThanOrEqual expects no null values.");
    // Don't handle identifiers, so that the caller is responsible for managing memory of e1 and e2
    if (e1->type == TYPE_IDENTIFIER || e2->type == TYPE_IDENTIFIER)
	return criticalError("leq: pass the VALUE of the identifier into this function with context_getValue(), instead of the identifier itself.");

    if (e1->type == TYPE_NUMBER && e2->type == TYPE_NUMBER) {
	double result = e1->value.literal_num <= e2->value.literal_num;
	return value_newNumber(result);
    } else if (e1->type == TYPE_STRING && e2->type == TYPE_STRING) {
	char *s1 = e1->value.literal_str;
	char *s2 = e2->value.literal_str;
	return value_newNumber(strlen(s1) <= strlen(s2));
    } 
    return executionError("TypeError: lessThanOrEqual expects two strings or two numbers.");
}
