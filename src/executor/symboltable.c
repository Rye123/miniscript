#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "symboltable.h"
#include "executor.h"

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
    sym->symbolName = identifier->value.identifier_name;
    sym->value = NULL;

    ctx->symbolCount++;
    ctx->symbols = realloc(ctx->symbols, ctx->symbolCount);
    ctx->symbols[ctx->symbolCount-1] = sym;
}

ExecSymbol *_ctx_getSymFromCtx(Context *ctx, char *symbolName)
{
    size_t expLen = strlen(symbolName);
    for (int i = 0; i < ctx->symbolCount; i++) {
	if (strlen(ctx->symbols[i]->symbolName) != expLen)
	    continue;
	if (strncmp(ctx->symbols[i]->symbolName, symbolName, expLen) == 0)
	    return ctx->symbols[i];
    }
    return NULL;
}

ExecSymbol *context_getSymbol(Context *ctx, ExecValue *identifier)
{
    char *symbolName = identifier->value.identifier_name;
    ExecSymbol *sym = _ctx_getSymFromCtx(ctx, symbolName);

    // Search global scope if not found in local
    if (sym == NULL && ctx->global != NULL)
	sym = _ctx_getSymFromCtx(ctx->global, symbolName);
    
    return sym;
}

void context_setSymbol(Context *ctx, ExecValue *identifier, ExecValue *value)
{
    ExecSymbol *sym = context_getSymbol(ctx, identifier);
    if (sym == NULL) {
	printf("Execution Error: Could not set value of undeclared variable %s.\n", sym->symbolName);
	exit(1);
    }
    sym->value = value;
}

void context_free(Context *ctx)
{
    for (size_t i = 0; i < ctx->symbolCount; i++)
	free(ctx->symbols[i]);
    free(ctx->symbols);
    free(ctx);
}
