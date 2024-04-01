#ifndef _EXECUTOR_H_
#define _EXECUTOR_H_
#include "../lexer/token.h"
#include "../parser/symbol.h"

typedef enum {
    TYPE_NUMBER,
    TYPE_STRING,
    TYPE_BOOL,
    TYPE_NULL
} ExecValueType;

typedef struct {
    ExecValueType type;
    union {
	void* literal_null;
	int literal_bool;
	double literal_num;
	char* literal_str;
    } literal;
    int metadata; // dependent on the node, typically this is used by *_R nodes to store data for their parents.
} ExecValue;

ExecValue* value_newNull();
ExecValue* value_newBool(int boolValue);
ExecValue* value_newString(char* strValue);
ExecValue* value_newNumber(double numValue);
void value_free(ExecValue *value);

/**
execSymbol:
- Returns NULL, or the evaluated Token*.
 */

ExecValue* execExpr(ASTNode* expr);
ExecValue* execEquality(ASTNode *equality);
ExecValue* execEqualityR(ASTNode *equalityR);
ExecValue* execComparison(ASTNode *comparison);
ExecValue* execComparisonR(ASTNode *comparisonR);
ExecValue* execSum(ASTNode *sum);
ExecValue* execSumR(ASTNode *sumR);
ExecValue* execTerm(ASTNode *term);
ExecValue* execTermR(ASTNode *termR);
ExecValue* execUnary(ASTNode *unary);
ExecValue* execPower(ASTNode *power);
ExecValue* execPrimary(ASTNode *primary);
ExecValue* execTerminal(ASTNode *terminal);

#endif
