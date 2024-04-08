#ifndef _SYMBOLTABLE_H_
#define _SYMBOLTABLE_H_
#include "executor.h"

// A unique identifier in the current scope, not to be confused with the Symbol from parsing.
typedef struct {
    const char* symbolName;
    ExecValue*  value;
} ExecSymbol;  

typedef struct _context {
    struct _context *global;  // Points to the global scope/context
    struct _context *parent;  // Points to the parent scope/context
    ExecSymbol** symbols;      // Symbols defined in this context. 
    size_t symbolCount;
} Context;

// Defines a new context. The global context would have parent = NULL and global = NULL.
Context* context_new(Context* parent, Context* global);

// Adds a new identifier to the context. This adds a COPY of the ExecValue.
void context_addSymbol(Context* ctx, ExecValue* identifier);

// Returns the ExecSymbol associated with an identifier, or NULL if there is no such identifier.
ExecSymbol *context_getSymbol(Context *ctx, ExecValue *identifier);

// Modifies the ExecSymbol associated with an identifier.
void context_setSymbol(Context* ctx, ExecValue* identifier, ExecValue* value);

// Frees the context, including all copied symbols.
void context_free(Context* ctx);

#endif
