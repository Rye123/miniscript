#ifndef _EXECVALUE_H
#define _EXECVALUE_H
#include "../parser/symbol.h"
#include "../error/error.h"

typedef enum {
    TYPE_NUMBER,
    TYPE_STRING,
    TYPE_NULL,
    TYPE_IDENTIFIER,
    TYPE_FUNCTION,
    TYPE_ERROR,
    TYPE_UNASSIGNED
} ValueType;

static const char* ValueTypeString[] = {"TYPE_NUMBER", "TYPE_STRING", "TYPE_NULL", "TYPE_IDENTIFIER", "TYPE_FUNCTION", "TYPE_ERROR", "TYPE_UNASSIGNED"};

// A function reference
typedef struct {
    ASTNode* argList;
    ASTNode* fnBlk;
} FunctionRef;

// A value that is assigned, or an identifier name.
typedef struct {
    ValueType type;
    union {
        void* literal_null;
        double literal_num;
        char* literal_str;
        char* identifier_name;
        FunctionRef* function_ref;
        Error* error_ptr;
    } value;
    Token* tok; // Used to add context for the ExecValue
} ExecValue;


// Defines new ExecValues
ExecValue* value_newNull();
ExecValue* value_newString(char* strValue, Token* tokPtr);
ExecValue* value_newNumber(double numValue, Token* tokPtr);
ExecValue* value_newIdentifier(char *identifierName, Token* tokPtr);
ExecValue* value_newError(Error *err, Token* tokPtr);
ExecValue* value_newFunction(ASTNode* argList, ASTNode* block, Token* tokPtr);

// Clones an ExecValue
ExecValue* value_clone(ExecValue *val);

// Frees an ExecValue
void value_free(ExecValue *value);

// Returns 0 if the value is FALSE, 1 if TRUE. -1 for an invalid type.
int value_falsiness(ExecValue *);

/* Returns a NEW ExecValue* with the result of these operations.*/
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
