#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
