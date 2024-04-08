#include <stdio.h>
#include <math.h>
#include <string.h>
#include "../lexer/token.h"
#include "../parser/symbol.h"
#include "executor.h"
#include "symboltable.h"

ExecValue *criticalError(char *msg)
{
    printf("Critical Error: %s\n", msg);
    exit(1);
    return value_newNull();
}

ExecValue *value_newNull()
{
    ExecValue* val = malloc(sizeof(ExecValue));
    val->type = TYPE_NULL;
    val->value.literal_null = NULL;
    val->metadata = -1;
    return val;
}

ExecValue *value_newString(char *strValue)
{
    ExecValue* val = malloc(sizeof(ExecValue));
    val->type = TYPE_STRING;

    // Create null-terminated copy of the string
    char *str_cpy = malloc(strlen(strValue) * sizeof(char));
    if (strValue != NULL)
	memcpy(str_cpy, strValue, strlen(strValue));
    val->value.literal_str = str_cpy;
    val->metadata = -1;
    return val;
}

ExecValue *value_newNumber(double numValue)
{
    ExecValue* val = malloc(sizeof(ExecValue));
    val->type = TYPE_NUMBER;
    val->value.literal_num = numValue;
    val->metadata = -1;
    return val;
}

ExecValue *value_newIdentifier(char *identifierName)
{
    ExecValue* val = malloc(sizeof(ExecValue));
    val->type = TYPE_IDENTIFIER;

    // Create null-terminated copy of the identifier name
    char *name_cpy = malloc(strlen(identifierName) * sizeof(char));
    if (identifierName != NULL)
	memcpy(name_cpy, identifierName, strlen(identifierName));
    val->value.identifier_name = name_cpy;
}

void value_free(ExecValue *value)
{
    if (value->type == TYPE_STRING)
	free(value->value.literal_str);
    if (value->type == TYPE_IDENTIFIER)
	free(value->value.identifier_name);
    free(value);
}

ExecValue *execTerminal(Context* ctx, ASTNode *terminal)
{
    if (terminal->type != SYM_TERMINAL)
	return criticalError("Given invalid value for execTerminal.");

    Token *tok = terminal->tok;

    switch (tok->type) {
    case TOKEN_NULL:
	return value_newNull();
    case TOKEN_TRUE:
	return value_newNumber(1.0);
    case TOKEN_FALSE:
	return value_newNumber(0.0);
    case TOKEN_NUMBER:
	return value_newNumber(tok->literal.literal_num);
    case TOKEN_STRING:
	return value_newString(tok->literal.literal_str);
    case TOKEN_IDENTIFIER:
	return value_newIdentifier(tok->lexeme);
    default:
	return criticalError("Invalid token for execTerminal");
    }
}

ExecValue *execPrimary(Context* ctx, ASTNode *primary)
{
    if (primary->type != SYM_PRIMARY)
	return criticalError("Given invalid value for execPrimary.");
    if (primary->numChildren == 0)
	return criticalError("Given empty primary.");

    // Check if primary is ( EXPR ) or TERMINAL
    if (primary->numChildren == 3) {
	if (primary->children[0]->tok->type != TOKEN_PAREN_L ||
	    primary->children[2]->tok->type != TOKEN_PAREN_R)
	    return criticalError("Expected ( EXPR ), instead given invalid expression with 3 children.");
	return execExpr(ctx, primary->children[1]);
    } else if (primary->numChildren == 1) {
	return execTerminal(ctx, primary->children[0]);
    }
    
    return criticalError("Invalid number of children for primary.");
}

ExecValue *execPower(Context* ctx, ASTNode *power)
{
    if (power->type != SYM_POWER)
	return criticalError("Given invalid value for execPower.");
    
    // Check if power is PRIMARY ^ UNARY or PRIMARY
    if (power->numChildren == 1)
	return execPrimary(ctx, power->children[0]);
    if (power->numChildren == 3) {
	if (power->children[1]->tok->type != TOKEN_CARET)
	    criticalError("Invalid middle token for execPower.");
	ExecValue *lVal = execPrimary(ctx, power->children[0]);
	ExecValue *rVal = execUnary(ctx, power->children[2]);

	if (lVal->type != TYPE_NUMBER || rVal->type != TYPE_NUMBER) {
	    printf("Semantic Error: Operation ^ is valid only between numbers.\n");
	    value_free(lVal); value_free(rVal);
	    return value_newNull();
	}
	
	double lvalue = lVal->value.literal_num;
	double rvalue = rVal->value.literal_num;
	double value = pow(lvalue, rvalue); //TODO: pow() errors
	value_free(lVal); value_free(rVal);
	return value_newNumber(value);
    }

    return criticalError("Invalid children for power.");
}

ExecValue *execUnary(Context* ctx, ASTNode *unary)
{
    if (unary->type != SYM_UNARY)
	return criticalError("Given invalid value for execUnary.");
    
    // Check if unary is +/- UNARY or POWER
    if (unary->numChildren == 1)
	return execPower(ctx, unary->children[0]);
    if (unary->numChildren == 2) {
	int is_pos = 0;
	if (unary->children[0]->tok->type == TOKEN_PLUS)
	    is_pos = 1;
	else if (unary->children[0]->tok->type == TOKEN_MINUS)
	    is_pos = 0;
	else
	    return criticalError("Invalid first child of unary.");
	ExecValue *rVal = execUnary(ctx, unary->children[1]);
	if (rVal->type != TYPE_NUMBER) {
	    printf("Semantic Error: Only a number can be unary.\n");
	    value_free(rVal);
	    return value_newNull();
	}
	double value = rVal->value.literal_num;
	if (!is_pos)
	    value = -value;
	value_free(rVal);
	return value_newNumber(value);
    }

    return criticalError("Invalid children for unary.");
}

ExecValue *execTermR(Context* ctx, ASTNode *termR)
{
    if (termR->type != SYM_TERM_R)
	return criticalError("Given invalid value for execTermR.");
    
    // Check if termR is op TERM TERM_R or EMPTY
    if (termR->numChildren == 0)
	return value_newNull();
    if (termR->numChildren == 3) {
	int op = -1; // 0 for *, 1 for /, 2 for %
	switch (termR->children[0]->tok->type) {
	case TOKEN_STAR: op = 0; break;
	case TOKEN_SLASH: op = 1; break;
	case TOKEN_PERCENT: op = 2; break;
	default: return criticalError("Invalid operation for termR.");
	}

	if (termR->children[1]->type != SYM_TERM || termR->children[2]->type != SYM_TERM_R)
	    return criticalError("Invalid child type for termR.");

	ExecValue *termVal = execTerm(ctx, termR->children[1]);
	ExecValue *termRVal = execTermR(ctx, termR->children[2]);
	double value;
	
	if (termRVal->type == TYPE_NULL && termVal->type == TYPE_NUMBER) {
	    value = termVal->value.literal_num;
	    value_free(termVal); value_free(termRVal);
	} else if (termRVal->type == TYPE_NUMBER && termVal->type == TYPE_NUMBER) {
	    // Apply the operation based on termRVal's metadata
	    value = termVal->value.literal_num;
	    double rvalue = termRVal->value.literal_num;
	    int child_op = termRVal->metadata;
	    if (child_op == 0)
		value = value * rvalue; //TODO: handle errors
	    else if (child_op == 1)
		value = value / rvalue; //TODO: handle errors
	    else if (child_op == 2)
		value = fmod(value, rvalue); //TODO: handle errors
	    else {
		value_free(termVal); value_free(termRVal);
		return criticalError("Expected operation from child termR.");
	    }
	    value_free(termVal); value_free(termRVal);
	} else {
	    value_free(termVal);
	    value_free(termRVal);
	    return criticalError("Invalid type of child, expected numbers for both symbols.");
	}
	
	ExecValue *intermVal = value_newNumber(value);
	intermVal->metadata = op;
	return intermVal;
    }

    return criticalError("Invalid children for termR.");
}

ExecValue *execTerm(Context* ctx, ASTNode *term)
{
    if (term->type != SYM_TERM)
	return criticalError("Given invalid value for execTermR.");
    
    // Check if term is UNARY TERM_R
    if (term->numChildren == 0)
	return criticalError("Expected children for term.");
    if (term->numChildren == 2) {
	if (term->children[0]->type != SYM_UNARY || term->children[1]->type != SYM_TERM_R)
	    return criticalError("Invalid child type for TERM.");

	ExecValue *unaryVal = execUnary(ctx, term->children[0]);
	ExecValue *termRVal = execTermR(ctx, term->children[1]);
	double value;

	if (unaryVal->type == TYPE_STRING && termRVal->type == TYPE_NULL) {
	    value_free(termRVal); return unaryVal;
	} else if (unaryVal->type == TYPE_NUMBER && termRVal->type == TYPE_NULL) {
	    value = unaryVal->value.literal_num;
	    value_free(unaryVal); value_free(termRVal);
	} else if (unaryVal->type == TYPE_NUMBER && termRVal->type == TYPE_NUMBER) {
	    // Apply operation, based on termRVal's metadata
	    value = unaryVal->value.literal_num;
	    double rvalue = termRVal->value.literal_num;
	    int child_op = termRVal->metadata;
	    if (child_op == 0)
		value = value * rvalue; //TODO: handle errors
	    else if (child_op == 1)
		value = value / rvalue; //TODO: handle errors
	    else if (child_op == 2)
		value = fmod(value, rvalue); //TODO: handle errors
	    else {
		value_free(unaryVal); value_free(termRVal);
		return criticalError("Expected operation from child termR.");
	    }
	} else {
	    value_free(unaryVal); value_free(termRVal);
	    return criticalError("Invalid type of child, expected numbers for both symbols.");
	}

        return value_newNumber(value);
    }

    return criticalError("Invalid children for term.");
}

ExecValue *execSumR(Context* ctx, ASTNode *sumR)
{
    if (sumR->type != SYM_SUM_R)
	return criticalError("Given invalid value for execSumR.");
    
    // Check if sumR is op SUM SUM_R or EMPTY
    if (sumR->numChildren == 0)
	return value_newNull();
    if (sumR->numChildren == 3) {
	int op = -1; // 0 for +, 1 for -
	switch (sumR->children[0]->tok->type) {
	case TOKEN_PLUS: op = 0; break;
	case TOKEN_MINUS: op = 1; break;
	default: return criticalError("Invalid operation for sumR.");
	}

	if (sumR->children[1]->type != SYM_SUM || sumR->children[2]->type != SYM_SUM_R)
	    return criticalError("Invalid child type for sumR.");

	ExecValue *sumVal = execSum(ctx, sumR->children[1]);
	ExecValue *sumRVal = execSumR(ctx, sumR->children[2]);
	double value;
	
	if (sumRVal->type == TYPE_NULL && sumVal->type == TYPE_NUMBER) {
	    value = sumVal->value.literal_num;
	    value_free(sumVal); value_free(sumRVal);
	} else if (sumRVal->type == TYPE_NUMBER && sumVal->type == TYPE_NUMBER) {
	    // Apply the operation based on sumRVal's metadata
	    value = sumVal->value.literal_num;
	    double rvalue = sumRVal->value.literal_num;
	    int child_op = sumRVal->metadata;
	    if (child_op == 0)
		value = value + rvalue; //TODO: handle errors
	    else if (child_op == 1)
		value = value - rvalue; //TODO: handle errors
	    else {
		value_free(sumVal); value_free(sumRVal);
		return criticalError("Expected operation from child sumR.");
	    }
	    value_free(sumVal); value_free(sumRVal);
	} else {
	    value_free(sumVal);
	    value_free(sumRVal);
	    return criticalError("Invalid type of child, expected numbers for both symbols.");
	}
	
	ExecValue *insumVal = value_newNumber(value);
	insumVal->metadata = op;
	return insumVal;
    }

    return criticalError("Invalid children for sumR.");
}

ExecValue *execSum(Context* ctx, ASTNode *sum)
{
    if (sum->type != SYM_SUM)
	return criticalError("Given invalid value for execSumR.");
    
    // Check if sum is TERM SUM_R
    if (sum->numChildren == 0)
	return criticalError("Expected children for sum.");
    if (sum->numChildren == 2) {
	if (sum->children[0]->type != SYM_TERM || sum->children[1]->type != SYM_SUM_R)
	    return criticalError("Invalid child type for SUM.");

	ExecValue *termVal = execTerm(ctx, sum->children[0]);
	ExecValue *sumRVal = execSumR(ctx, sum->children[1]);
	double value;

	if (termVal->type == TYPE_NUMBER && sumRVal->type == TYPE_NULL) {
	    value = termVal->value.literal_num;
	    value_free(termVal); value_free(sumRVal);
	} else if (termVal->type == TYPE_NUMBER && sumRVal->type == TYPE_NUMBER) {
	    // Apply operation, based on sumRVal's metadata
	    value = termVal->value.literal_num;
	    double rvalue = sumRVal->value.literal_num;
	    int child_op = sumRVal->metadata;
	    if (child_op == 0)
		value = value + rvalue; //TODO: handle errors
	    else if (child_op == 1)
		value = value - rvalue; //TODO: handle errors
	    else {
		value_free(termVal); value_free(sumRVal);
		return criticalError("Expected operation from child sumR.");
	    }
	} else {
	    value_free(termVal); value_free(sumRVal);
	    return criticalError("Invalid type of child, expected numbers for both symbols.");
	}

        return value_newNumber(value);
    }

    return criticalError("Invalid children for sum.");
}

ExecValue *execComparisonR(Context* ctx, ASTNode *comparisonR)
{
    if (comparisonR->type != SYM_COMPARISON_R)
	return criticalError("Given invalid value for execComparisonR.");
    
    // Check if comparisonR is op COMPARISON COMPARISON_R or EMPTY
    if (comparisonR->numChildren == 0)
	return value_newNull();
    if (comparisonR->numChildren == 3) {
	int op = -1; // 0 for >, 1 for >=, 2 for <, 3 for <=
	switch (comparisonR->children[0]->tok->type) {
	case TOKEN_GREATER: op = 0; break;
	case TOKEN_GREATER_EQUAL: op = 1; break;
	case TOKEN_LESS: op = 2; break;
	case TOKEN_LESS_EQUAL: op = 3; break;
	default: return criticalError("Invalid operation for comparisonR.");
	}

	if (comparisonR->children[1]->type != SYM_COMPARISON || comparisonR->children[2]->type != SYM_COMPARISON_R)
	    return criticalError("Invalid child type for comparisonR.");

	ExecValue *comparisonVal = execComparison(ctx, comparisonR->children[1]);
	ExecValue *comparisonRVal = execComparisonR(ctx, comparisonR->children[2]);
	double value;
	
	if (comparisonRVal->type == TYPE_NULL && comparisonVal->type == TYPE_NUMBER) {
	    value = comparisonVal->value.literal_num;
	    value_free(comparisonVal); value_free(comparisonRVal);
	} else if (comparisonRVal->type == TYPE_NUMBER && comparisonVal->type == TYPE_NUMBER) {
	    // Apply the operation based on comparisonRVal's metadata
	    value = comparisonVal->value.literal_num;
	    double rvalue = comparisonRVal->value.literal_num;
	    int child_op = comparisonRVal->metadata;
	    if (child_op == 0)
		value = value > rvalue; //TODO: handle errors
	    else if (child_op == 1)
		value = value >= rvalue; //TODO: handle errors
	    else if (child_op == 2)
		value = value < rvalue; //TODO: handle errors
	    else if (child_op == 3)
		value = value <= rvalue; //TODO: handle errors
	    else {
		value_free(comparisonVal); value_free(comparisonRVal);
		return criticalError("Expected operation from child comparisonR.");
	    }
	    value_free(comparisonVal); value_free(comparisonRVal);
	} else {
	    value_free(comparisonVal);
	    value_free(comparisonRVal);
	    return criticalError("Invalid type of child, expected numbers for both symbols.");
	}
	
	ExecValue *intermediateVal = value_newNumber(value);
	intermediateVal->metadata = op;
	return intermediateVal;
    }

    return criticalError("Invalid children for comparisonR.");
}

ExecValue *execComparison(Context* ctx, ASTNode *comparison)
{
    if (comparison->type != SYM_COMPARISON)
	return criticalError("Given invalid value for execComparisonR.");
    
    // Check if comparison is SUM COMPARISON_R
    if (comparison->numChildren == 0)
	return criticalError("Expected children for comparison.");
    if (comparison->numChildren == 2) {
	if (comparison->children[0]->type != SYM_SUM || comparison->children[1]->type != SYM_COMPARISON_R)
	    return criticalError("Invalid child type for COMPARISON.");

	ExecValue *sumVal = execSum(ctx, comparison->children[0]);
	ExecValue *comparisonRVal = execComparisonR(ctx, comparison->children[1]);
	double value;

	if (sumVal->type == TYPE_NUMBER && comparisonRVal->type == TYPE_NULL) {
	    value = sumVal->value.literal_num;
	    value_free(sumVal); value_free(comparisonRVal);
	    return value_newNumber(value);
	} else if (sumVal->type == TYPE_NUMBER && comparisonRVal->type == TYPE_NUMBER) {
	    // Apply operation, based on comparisonRVal's metadata
	    value = sumVal->value.literal_num;
	    double rvalue = comparisonRVal->value.literal_num;
	    int child_op = comparisonRVal->metadata;
	    if (child_op == 0)
		value = value > rvalue; //TODO: handle errors
	    else if (child_op == 1)
		value = value >= rvalue; //TODO: handle errors
	    else if (child_op == 2)
		value = value < rvalue; //TODO: handle errors
	    else if (child_op == 3)
		value = value <= rvalue; //TODO: handle errors
	    else {
		value_free(sumVal); value_free(comparisonRVal);
		return criticalError("Expected operation from child comparisonR.");
	    }
	    return value_newNumber(value);
	} else {
	    value_free(sumVal); value_free(comparisonRVal);
	    return criticalError("Invalid type of child, expected numbers for both symbols.");
	}
    }

    return criticalError("Invalid children for comparison.");
}

ExecValue *execEqualityR(Context* ctx, ASTNode *equalityR)
{
    if (equalityR->type != SYM_EQUALITY_R)
	return criticalError("Given invalid value for execEqualityR.");
    
    // Check if equalityR is op EQUALITY EQUALITY_R or EMPTY
    if (equalityR->numChildren == 0)
	return value_newNull();
    if (equalityR->numChildren == 3) {
	int op = -1; // 0 for ==, 1 for !=
	switch (equalityR->children[0]->tok->type) {
	case TOKEN_EQUAL_EQUAL: op = 0; break;
	case TOKEN_BANG_EQUAL: op = 1; break;
	default: return criticalError("Invalid operation for equalityR.");
	}

	if (equalityR->children[1]->type != SYM_EQUALITY || equalityR->children[2]->type != SYM_EQUALITY_R)
	    return criticalError("Invalid child type for equalityR.");

	ExecValue *equalityVal = execEquality(ctx, equalityR->children[1]);
	ExecValue *equalityRVal = execEqualityR(ctx, equalityR->children[2]);
	double value;
	
	if (equalityRVal->type == TYPE_NULL && equalityVal->type == TYPE_NUMBER) {
	    value = equalityVal->value.literal_num;
	    value_free(equalityVal); value_free(equalityRVal);
	} else if (equalityRVal->type == TYPE_NUMBER && equalityVal->type == TYPE_NUMBER) {
	    // Apply the operation based on equalityRVal's metadata
	    value = equalityVal->value.literal_num;
	    double rvalue = equalityRVal->value.literal_num;
	    int child_op = equalityRVal->metadata;
	    if (child_op == 0)
		value = value == rvalue; //TODO: handle errors
	    else if (child_op == 1)
		value = value != rvalue; //TODO: handle errors
	    else {
		value_free(equalityVal); value_free(equalityRVal);
		return criticalError("Expected operation from child equalityR.");
	    }
	    value_free(equalityVal); value_free(equalityRVal);
	} else {
	    value_free(equalityVal);
	    value_free(equalityRVal);
	    return criticalError("Invalid type of child, expected numbers for both symbols.");
	}
	
	ExecValue *intermediateVal = value_newNumber(value);
	intermediateVal->metadata = op;
	return intermediateVal;
    }

    return criticalError("Invalid children for equalityR.");
}

ExecValue *execEquality(Context* ctx, ASTNode *equality)
{
    if (equality->type != SYM_EQUALITY)
	return criticalError("Given invalid value for execEqualityR.");
    
    // Check if equality is COMPARISON EQUALITY_R
    if (equality->numChildren == 0)
	return criticalError("Expected children for equality.");
    if (equality->numChildren == 2) {
	if (equality->children[0]->type != SYM_COMPARISON || equality->children[1]->type != SYM_EQUALITY_R)
	    return criticalError("Invalid child type for EQUALITY.");

	ExecValue *comparisonVal = execComparison(ctx, equality->children[0]);
	ExecValue *equalityRVal = execEqualityR(ctx, equality->children[1]);
	double value;
	
	if (equalityRVal->type == TYPE_NULL && comparisonVal->type == TYPE_NUMBER) {
	    value = comparisonVal->value.literal_num;
	    value_free(comparisonVal); value_free(equalityRVal);
	    return value_newNumber(value);
	} else if (equalityRVal->type == TYPE_NUMBER && comparisonVal->type == TYPE_NUMBER) {
	    // Apply the operation based on equalityRVal's metadata
	    value = comparisonVal->value.literal_num;
	    double rvalue = equalityRVal->value.literal_num;
	    int child_op = equalityRVal->metadata;
	    if (child_op == 0)
		value = value == rvalue; //TODO: handle errors
	    else if (child_op == 1)
		value = value != rvalue; //TODO: handle errors
	    else {
		value_free(comparisonVal); value_free(equalityRVal);
		return criticalError("Expected operation from child equalityR.");
	    }
	    value_free(comparisonVal); value_free(equalityRVal);
	} else {
	    value_free(comparisonVal); value_free(equalityRVal);
	    return criticalError("Invalid type of child, expected numbers for both symbols.");
	}

        return value_newNumber(value);
    }

    return criticalError("Invalid children for equality.");
}

ExecValue *execExpr(Context* ctx, ASTNode *expr)
{
    if (expr->type != SYM_EXPR)
	return criticalError("Given invalid value for execExpr.");

    if (expr->numChildren != 1)
	return criticalError("Number of children of expr not 1.");

    return execEquality(ctx, expr->children[0]);
}

ExecValue *execPrntStmt(Context* ctx, ASTNode *prntStmt)
{
    if (prntStmt->numChildren == 3 &&
	prntStmt->children[0]->tok->type == TOKEN_PRINT &&
	prntStmt->children[1]->type == SYM_EXPR &&
	prntStmt->children[2]->tok->type == TOKEN_NL) {
	ExecValue *exprResult = execExpr(ctx, prntStmt->children[1]);

	switch (exprResult->type) {
	case TYPE_STRING:
	    printf("%s", exprResult->value.literal_str);
	    break;
	case TYPE_NUMBER:
	    printf("%f", exprResult->value.literal_num);
	    break;
	case TYPE_NULL:
	    printf("(null)");
	    break;
	}
	value_free(exprResult);
	return value_newNull();
    }
    return criticalError("Invalid print statement.");
}

ExecValue *execExprStmt(Context* ctx, ASTNode *exprStmt)
{
    if (exprStmt->numChildren == 2 &&
	exprStmt->children[0]->type == SYM_EXPR &&
	exprStmt->children[1]->tok->type == TOKEN_NL) {
	ExecValue *exprResult = execExpr(ctx, exprStmt->children[0]);
	value_free(exprResult);
	return value_newNull();
    }
    return criticalError("Invalid expr statement.");
}

ExecValue *execStmt(Context* ctx, ASTNode *stmt)
{
    if (stmt->numChildren == 1) {
	if (stmt->children[0]->type == SYM_EXPR_STMT)
	    return execExprStmt(ctx, stmt->children[0]);
	if (stmt->children[0]->type == SYM_PRNT_STMT)
	    return execPrntStmt(ctx, stmt->children[0]);
    }
    return criticalError("Invalid statement.");
}

ExecValue *execAsmt(Context* ctx, ASTNode *asmt)
{
    if (asmt->numChildren == 4 &&
	asmt->children[0]->tok->type == TOKEN_IDENTIFIER &&
	asmt->children[1]->tok->type == TOKEN_EQUAL &&
	asmt->children[2]->type == SYM_EXPR &&
	asmt->children[3]->tok->type == TOKEN_NL) {
	ExecValue *lvalue = execTerminal(ctx, asmt->children[0]);
	ExecValue *rvalue = execExpr(ctx, asmt->children[2]);
	// TODO: implement symbol table assignment
	value_free(rvalue);
	return value_newNull();
    }
    return criticalError("Invalid assignment.");
}

ExecValue *execLine(Context* ctx, ASTNode *line)
{
    if (line->numChildren == 1) {
	if (line->children[0]->type == SYM_ASMT)
	    return execAsmt(ctx, line->children[0]);
	if (line->children[0]->type == SYM_STMT)
	    return execStmt(ctx, line->children[0]);
    }
    return criticalError("Invalid line.");
}

ExecValue *execStart(ASTNode *start)
{
    // Returns the execution exit code
    //TODO: all runtime errors here
    int exitCode = 0;
    Context *globalContext = context_new(NULL, NULL);
    for (size_t i = 0; i < start->numChildren; i++) {
	ASTNode *child = start->children[0];
	ExecValue *result;
	if (child->type == SYM_LINE)
	    result = execLine(globalContext, child);
	else if (child->tok->type == TOKEN_EOF)
	    break;
	else
	    return criticalError("Unexpected symbol.");

	value_free(result);
    }

    return value_newNumber(exitCode);
}
