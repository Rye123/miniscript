#ifndef _SYMBOLTABLE_H_
#define _SYMBOLTABLE_H_
#include "../error/error.h"
#include "executor.h"
#include "execvalue.h"

/* A unique identifier in the current scope, not to be confused with the Symbol from parsing. */
typedef struct {
    char* symbolName;
    ExecValue* value;
} ExecSymbol;

/* Data about the current execution scope. */
typedef struct _context {
    struct _context *global;   /* Points to the global scope/context */
    struct _context *parent;   /* Points to the parent scope/context */
    size_t argCount;           /* Number of arguments in this context. */
    ExecSymbol** symbols;      /* Symbols defined in this context. */
    size_t symbolCount;
    int hasBreakOrContinue;    /* True if it is exiting break */
    int hasReturn;             /* True if has return */
} Context;

/* Defines a new context. The global context would have parent = NULL and global = NULL. */
Context* context_new(Context* parent, Context* global);

/* Adds a new identifier to the context. This adds a COPY of the ExecValue. */
void context_addSymbol(Context* ctx, ExecValue* identifier);


/* Returns the ExecSymbol associated with an identifier, or NULL if there is no such identifier. */
ExecSymbol *context_getSymbol(Context *ctx, ExecValue *identifier);
/* Returns a COPY of the ExecValue associated with identifier, or NULL if there is no such identifier */
ExecValue *context_getValue(Context *ctx, ExecValue *identifier);

/* Modifies the ExecSymbol associated with an identifier. */
void context_setSymbol(Context* ctx, ExecValue* identifier, ExecValue* value);

/* Frees the context, including all copied symbols. */
void context_free(Context* ctx);

#endif
