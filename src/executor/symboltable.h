#ifndef _SYMBOLTABLE_H_
#define _SYMBOLTABLE_H_
#include "executor.h"

typedef enum {
    TYPE_NUMBER,
    TYPE_STRING,
    TYPE_NULL,
    TYPE_IDENTIFIER
} ValueType;

static const char* ValueTypeString[] = {"TYPE_NUMBER", "TYPE_STRING", "TYPE_NULL", "TYPE_IDENTIFIER"};

typedef struct {
    ValueType type;
    union {
	void* literal_null;
	double literal_num;
	char* literal_str;
	char* identifier_name;
    } value;
} ExecValue;

// A unique identifier in the current scope, not to be confused with the Symbol from parsing.
typedef struct {
    char* symbolName;
    ExecValue*  value;
} ExecSymbol;  

typedef struct _context {
    struct _context *global;  // Points to the global scope/context
    struct _context *parent;  // Points to the parent scope/context
    ExecSymbol** symbols;      // Symbols defined in this context. 
    size_t symbolCount;
    int hasBreakOrContinue; // True if it is exiting break
} Context;

// Defines a new context. The global context would have parent = NULL and global = NULL.
Context* context_new(Context* parent, Context* global);

// Adds a new identifier to the context. This adds a COPY of the ExecValue.
void context_addSymbol(Context* ctx, ExecValue* identifier);

// Defines new ExecValues
ExecValue* value_newNull();
ExecValue* value_newString(char* strValue);
ExecValue* value_newNumber(double numValue);
ExecValue* value_newIdentifier(char *identifierName);

// Clones an ExecValue
ExecValue* value_clone(ExecValue *val);

// Frees an ExecValue
void value_free(ExecValue *value);


// Returns the ExecSymbol associated with an identifier, or NULL if there is no such identifier.
ExecSymbol *context_getSymbol(Context *ctx, ExecValue *identifier);

// Returns a COPY of the ExecValue associated with identifier, or NULL if there is no such identifier
ExecValue *context_getValue(Context *ctx, ExecValue *identifier);

// Returns the ExecSymbol in the local or the GLOBAL scope.
ExecSymbol *context_getSymbolWalk(Context *ctx, ExecValue *identifier);

// Modifies the ExecSymbol associated with an identifier.
void context_setSymbol(Context* ctx, ExecValue* identifier, ExecValue* value);

// Frees the context, including all copied symbols.
void context_free(Context* ctx);

ExecValue* criticalError(char *msg);
ExecValue* executionError(char *msg);

// Returns 0 if the value is FALSE, 1 if TRUE.
int value_falsiness(ExecValue *);

/* Returns a NEW ExecValue* with the result of these operations. */
ExecValue* value_opUnaryPos(ExecValue*);
ExecValue* value_opUnaryNeg(ExecValue*);
ExecValue* value_opNot(ExecValue*);
ExecValue* value_opAnd(ExecValue*, ExecValue*);
ExecValue* value_opOr(ExecValue*, ExecValue*);
ExecValue* value_opAdd(ExecValue*, ExecValue*);
ExecValue* value_opSub(ExecValue*, ExecValue*);
ExecValue* value_opMul(ExecValue*, ExecValue*);
ExecValue* value_opDiv(ExecValue*, ExecValue*);
ExecValue* value_opMod(ExecValue*, ExecValue*);
ExecValue* value_opPow(ExecValue*, ExecValue*);
ExecValue* value_opEqEq(ExecValue*, ExecValue*);
ExecValue* value_opNEq(ExecValue*, ExecValue*);
ExecValue* value_opGt(ExecValue*, ExecValue*);
ExecValue* value_opGEq(ExecValue*, ExecValue*);
ExecValue* value_opLt(ExecValue*, ExecValue*);
ExecValue* value_opLEq(ExecValue*, ExecValue*);

#endif
