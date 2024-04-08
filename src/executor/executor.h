#ifndef _EXECUTOR_H_
#define _EXECUTOR_H_
#include "../lexer/token.h"
#include "../parser/symbol.h"
#include "symboltable.h"

ExecValue* value_newNull();
ExecValue* value_newString(char* strValue);
ExecValue* value_newNumber(double numValue);
void value_free(ExecValue *value);

/**
execSymbol:
- Returns NULL, or the evaluated ExecValue*.
 */

ExecValue* execStart(Context* ctx, ASTNode *start);
ExecValue* execLine(Context* ctx, ASTNode *line);
ExecValue* execStmt(Context* ctx, ASTNode *stmt);
ExecValue* execExprStmt(Context* ctx, ASTNode *exprStmt);
ExecValue* execPrntStmt(Context* ctx, ASTNode *prntStmt);
ExecValue* execExpr(Context* ctx, ASTNode* expr);
ExecValue* execEquality(Context* ctx, ASTNode *equality);
ExecValue* execEqualityR(Context* ctx, ASTNode *equalityR);
ExecValue* execComparison(Context* ctx, ASTNode *comparison);
ExecValue* execComparisonR(Context* ctx, ASTNode *comparisonR);
ExecValue* execSum(Context* ctx, ASTNode *sum);
ExecValue* execSumR(Context* ctx, ASTNode *sumR);
ExecValue* execTerm(Context* ctx, ASTNode *term);
ExecValue* execTermR(Context* ctx, ASTNode *termR);
ExecValue* execUnary(Context* ctx, ASTNode *unary);
ExecValue* execPower(Context* ctx, ASTNode *power);
ExecValue* execPrimary(Context* ctx, ASTNode *primary);
ExecValue* execTerminal(Context* ctx, ASTNode *terminal);

#endif
