#include <stdlib.h>
#include <string.h>
#include "../logger/logger.h"
#include "../error/error.h"
#include "execvalue.h"
#include "executor.h"
#include "symboltable.h"

Context *context_new(Context *parent, Context *global) {
    Context *ctx = malloc(sizeof(Context));
    ctx->global = global;
    ctx->parent = parent;
    ctx->argCount = 0;
    ctx->symbols = malloc(0);
    ctx->symbolCount = 0;
    ctx->hasBreakOrContinue = 0;
    ctx->hasReturn = 0;
    return ctx;
}

void context_addSymbol(Context *ctx, ExecValue *identifier) {
    char *identifierName;
    ExecSymbol *sym = malloc(sizeof(ExecSymbol));

    if (identifier->type != TYPE_IDENTIFIER) {
        log_message(&executionLogger,
                    "Tried to add an ExecSymbol with an 'identifier' of type %s, expected TYPE_IDENTIFIER.\n",
                    ValueTypeString[identifier->type]);
        exit(1);
    }

    identifierName = identifier->value.identifier_name;
    sym->symbolName = strdup(identifierName);
    sym->value = malloc(sizeof(ExecValue));
    sym->value->type = TYPE_UNASSIGNED;

    ctx->symbolCount = ctx->symbolCount + 1;
    ctx->symbols = realloc(ctx->symbols, ctx->symbolCount * sizeof(ExecSymbol *));
    ctx->symbols[ctx->symbolCount - 1] = sym;
}

ExecSymbol *context_getSymbol(Context *ctx, ExecValue *identifier) {
    char *symbolName;
    size_t expLen;
    size_t i;

    if (identifier->type != TYPE_IDENTIFIER) {
        log_message(&executionLogger,
                    "Tried to get an ExecSymbol with 'identifier' of type %s, expected TYPE_IDENTIFIER.\n",
                    ValueTypeString[identifier->type]);
        exit(1);
    }

    symbolName = identifier->value.identifier_name;
    expLen = strlen(symbolName);
    for (i = 0; i < ctx->symbolCount; i++) {
        if (strlen(ctx->symbols[i]->symbolName) != expLen)
            continue;
        if (strncmp(ctx->symbols[i]->symbolName, symbolName, expLen) == 0)
            return ctx->symbols[i];
    }
    return NULL;
}

ExecValue *context_getValue(Context *ctx, ExecValue *identifier) {
    ExecSymbol *sym = context_getSymbol(ctx, identifier);
    if (sym == NULL)
        return NULL;
    return value_clone(sym->value);
}

void context_setSymbol(Context *ctx, ExecValue *identifier, ExecValue *value) {
    ExecValue *newValue;
    ExecSymbol *sym = context_getSymbol(ctx, identifier);


    if (sym == NULL) {
        log_message(&executionLogger, "Execution Error: Could not set value of undeclared variable %s.\n",
                    sym->symbolName);
        exit(1);
    }

    /* Define a new ExecValue -- this is to allow the given value to be deallocated later */
    newValue = value_clone(value);

    /* Set the new value */
    value_free(sym->value);
    sym->value = newValue;
}

void context_free(Context *ctx) {
    size_t i;
    for (i = 0; i < ctx->symbolCount; i++)
        free(ctx->symbols[i]);
    free(ctx->symbols);
    free(ctx);
}
