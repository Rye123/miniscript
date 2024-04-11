#ifndef _EXECUTOR_H_
#define _EXECUTOR_H_
#include "../lexer/token.h"
#include "../parser/symbol.h"
#include "symboltable.h"

/**
execSymbol:
- Returns NULL, or the evaluated ExecValue*.
 */

ExecValue* execStart(Context* ctx, ASTNode *start);
ExecValue* execLine(Context* ctx, ASTNode *line);
ExecValue* execStmt(Context* ctx, ASTNode *stmt);
ExecValue* execContinue(Context* ctx, ASTNode *stmt);
ExecValue* execBreak(Context* ctx, ASTNode *stmt);
ExecValue* execWhileStmt(Context* ctx, ASTNode *stmt);
ExecValue* execIfStmt(Context* ctx, ASTNode *stmt);
ExecValue* execElseIf(Context* ctx, ASTNode *stmt);
ExecValue* execElse(Context* ctx, ASTNode *stmt);
ExecValue* execBlock(Context* ctx, ASTNode *stmt);
ExecValue* execReturn(Context* ctx, ASTNode *stmt);
ExecValue* execExprStmt(Context* ctx, ASTNode *exprStmt);
ExecValue* execPrntStmt(Context* ctx, ASTNode *prntStmt);
ExecValue* execExpr(Context* ctx, ASTNode* expr);
ExecValue* execFnExpr(Context* ctx, ASTNode* expr);
ExecValue* execArgList(Context* ctx, ASTNode* expr);
ExecValue* execArg(Context* ctx, ASTNode* expr);
ExecValue* execOrExpr(Context* ctx, ASTNode* orExpr);
ExecValue* execAndExpr(Context* ctx, ASTNode* andExpr);
ExecValue* execLogUnary(Context* ctx, ASTNode* logUnary);
ExecValue* execEquality(Context* ctx, ASTNode *equality);
ExecValue* execComparison(Context* ctx, ASTNode *comparison);
ExecValue* execSum(Context* ctx, ASTNode *sum);
ExecValue* execTerm(Context* ctx, ASTNode *term);
ExecValue* execUnary(Context* ctx, ASTNode *unary);
ExecValue* execPower(Context* ctx, ASTNode *power);
ExecValue* execPrimary(Context* ctx, ASTNode *primary);
ExecValue* execFnCall(Context* ctx, ASTNode *fnCall);
ExecValue* execFnArgs(Context* ctx, ASTNode *fnArgs);
ExecValue* execTerminal(Context* ctx, ASTNode *terminal);

#endif
