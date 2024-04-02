#include <stdio.h>
#include <math.h>
#include <string.h>
#include "../lexer/token.h"
#include "../parser/symbol.h"
#include "executor.h"

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
    val->literal.literal_null = NULL;
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
    val->literal.literal_str = str_cpy;
    val->metadata = -1;
    return val;
}

ExecValue *value_newNumber(double numValue)
{
    ExecValue* val = malloc(sizeof(ExecValue));
    val->type = TYPE_NUMBER;
    val->literal.literal_num = numValue;
    val->metadata = -1;
    return val;
}

void value_free(ExecValue *value)
{
    if (value->type == TYPE_STRING)
	free(value->literal.literal_str);
    free(value);
}

ExecValue *execTerminal(ASTNode *terminal)
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
	return criticalError("Identifier not implemented."); //TODO: this should return a reference to something in the symbol table, where the table is possibly provided through a Context struct.
    default:
	return criticalError("Invalid token for execTerminal");
    }
}

ExecValue *execPrimary(ASTNode *primary)
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
	return execExpr(primary->children[1]);
    } else if (primary->numChildren == 1) {
	return execTerminal(primary->children[0]);
    }
    
    return criticalError("Invalid number of children for primary.");
}

ExecValue *execPower(ASTNode *power)
{
    if (power->type != SYM_POWER)
	return criticalError("Given invalid value for execPower.");
    
    // Check if power is PRIMARY ^ UNARY or PRIMARY
    if (power->numChildren == 1)
	return execPrimary(power->children[0]);
    if (power->numChildren == 3) {
	if (power->children[1]->tok->type != TOKEN_CARET)
	    criticalError("Invalid middle token for execPower.");
	ExecValue *lVal = execPrimary(power->children[0]);
	ExecValue *rVal = execUnary(power->children[2]);

	if (lVal->type != TYPE_NUMBER || rVal->type != TYPE_NUMBER) {
	    printf("Semantic Error: Operation ^ is valid only between numbers.\n");
	    value_free(lVal); value_free(rVal);
	    return value_newNull();
	}
	
	double lvalue = lVal->literal.literal_num;
	double rvalue = rVal->literal.literal_num;
	double value = pow(lvalue, rvalue); //TODO: pow() errors
	value_free(lVal); value_free(rVal);
	return value_newNumber(value);
    }

    return criticalError("Invalid children for power.");
}

ExecValue *execUnary(ASTNode *unary)
{
    if (unary->type != SYM_UNARY)
	return criticalError("Given invalid value for execUnary.");
    
    // Check if unary is +/- UNARY or POWER
    if (unary->numChildren == 1)
	return execPower(unary->children[0]);
    if (unary->numChildren == 2) {
	int is_pos = 0;
	if (unary->children[0]->tok->type == TOKEN_PLUS)
	    is_pos = 1;
	else if (unary->children[0]->tok->type == TOKEN_MINUS)
	    is_pos = 0;
	else
	    return criticalError("Invalid first child of unary.");
	ExecValue *rVal = execUnary(unary->children[1]);
	if (rVal->type != TYPE_NUMBER) {
	    printf("Semantic Error: Only a number can be unary.\n");
	    value_free(rVal);
	    return value_newNull();
	}
	double value = rVal->literal.literal_num;
	if (!is_pos)
	    value = -value;
	value_free(rVal);
	return value_newNumber(value);
    }

    return criticalError("Invalid children for unary.");
}

ExecValue *execTermR(ASTNode *termR)
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

	ExecValue *termVal = execTerm(termR->children[1]);
	ExecValue *termRVal = execTermR(termR->children[2]);
	double value;
	
	if (termRVal->type == TYPE_NULL && termVal->type == TYPE_NUMBER) {
	    value = termVal->literal.literal_num;
	    value_free(termVal); value_free(termRVal);
	} else if (termRVal->type == TYPE_NUMBER && termVal->type == TYPE_NUMBER) {
	    // Apply the operation based on termRVal's metadata
	    value = termVal->literal.literal_num;
	    double rvalue = termRVal->literal.literal_num;
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

ExecValue *execTerm(ASTNode *term)
{
    if (term->type != SYM_TERM)
	return criticalError("Given invalid value for execTermR.");
    
    // Check if term is UNARY TERM_R
    if (term->numChildren == 0)
	return criticalError("Expected children for term.");
    if (term->numChildren == 2) {
	if (term->children[0]->type != SYM_UNARY || term->children[1]->type != SYM_TERM_R)
	    return criticalError("Invalid child type for TERM.");

	ExecValue *unaryVal = execUnary(term->children[0]);
	ExecValue *termRVal = execTermR(term->children[1]);
	double value;

	if (unaryVal->type == TYPE_NUMBER && termRVal->type == TYPE_NULL) {
	    value = unaryVal->literal.literal_num;
	    value_free(unaryVal); value_free(termRVal);
	} else if (unaryVal->type == TYPE_NUMBER && termRVal->type == TYPE_NUMBER) {
	    // Apply operation, based on termRVal's metadata
	    value = unaryVal->literal.literal_num;
	    double rvalue = termRVal->literal.literal_num;
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

ExecValue *execSumR(ASTNode *sumR)
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

	ExecValue *sumVal = execSum(sumR->children[1]);
	ExecValue *sumRVal = execSumR(sumR->children[2]);
	double value;
	
	if (sumRVal->type == TYPE_NULL && sumVal->type == TYPE_NUMBER) {
	    value = sumVal->literal.literal_num;
	    value_free(sumVal); value_free(sumRVal);
	} else if (sumRVal->type == TYPE_NUMBER && sumVal->type == TYPE_NUMBER) {
	    // Apply the operation based on sumRVal's metadata
	    value = sumVal->literal.literal_num;
	    double rvalue = sumRVal->literal.literal_num;
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

ExecValue *execSum(ASTNode *sum)
{
    if (sum->type != SYM_SUM)
	return criticalError("Given invalid value for execSumR.");
    
    // Check if sum is TERM SUM_R
    if (sum->numChildren == 0)
	return criticalError("Expected children for sum.");
    if (sum->numChildren == 2) {
	if (sum->children[0]->type != SYM_TERM || sum->children[1]->type != SYM_SUM_R)
	    return criticalError("Invalid child type for SUM.");

	ExecValue *termVal = execTerm(sum->children[0]);
	ExecValue *sumRVal = execSumR(sum->children[1]);
	double value;

	if (termVal->type == TYPE_NUMBER && sumRVal->type == TYPE_NULL) {
	    value = termVal->literal.literal_num;
	    value_free(termVal); value_free(sumRVal);
	} else if (termVal->type == TYPE_NUMBER && sumRVal->type == TYPE_NUMBER) {
	    // Apply operation, based on sumRVal's metadata
	    value = termVal->literal.literal_num;
	    double rvalue = sumRVal->literal.literal_num;
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

ExecValue *execComparisonR(ASTNode *comparisonR)
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

	ExecValue *comparisonVal = execComparison(comparisonR->children[1]);
	ExecValue *comparisonRVal = execComparisonR(comparisonR->children[2]);
	double value;
	
	if (comparisonRVal->type == TYPE_NULL && comparisonVal->type == TYPE_NUMBER) {
	    value = comparisonVal->literal.literal_num;
	    value_free(comparisonVal); value_free(comparisonRVal);
	} else if (comparisonRVal->type == TYPE_NUMBER && comparisonVal->type == TYPE_NUMBER) {
	    // Apply the operation based on comparisonRVal's metadata
	    value = comparisonVal->literal.literal_num;
	    double rvalue = comparisonRVal->literal.literal_num;
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

ExecValue *execComparison(ASTNode *comparison)
{
    if (comparison->type != SYM_COMPARISON)
	return criticalError("Given invalid value for execComparisonR.");
    
    // Check if comparison is SUM COMPARISON_R
    if (comparison->numChildren == 0)
	return criticalError("Expected children for comparison.");
    if (comparison->numChildren == 2) {
	if (comparison->children[0]->type != SYM_SUM || comparison->children[1]->type != SYM_COMPARISON_R)
	    return criticalError("Invalid child type for COMPARISON.");

	ExecValue *sumVal = execSum(comparison->children[0]);
	ExecValue *comparisonRVal = execComparisonR(comparison->children[1]);
	double value;

	if (sumVal->type == TYPE_NUMBER && comparisonRVal->type == TYPE_NULL) {
	    value = sumVal->literal.literal_num;
	    value_free(sumVal); value_free(comparisonRVal);
	    return value_newNumber(value);
	} else if (sumVal->type == TYPE_NUMBER && comparisonRVal->type == TYPE_NUMBER) {
	    // Apply operation, based on comparisonRVal's metadata
	    value = sumVal->literal.literal_num;
	    double rvalue = comparisonRVal->literal.literal_num;
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

ExecValue *execEqualityR(ASTNode *equalityR)
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

	ExecValue *equalityVal = execEquality(equalityR->children[1]);
	ExecValue *equalityRVal = execEqualityR(equalityR->children[2]);
	double value;
	
	if (equalityRVal->type == TYPE_NULL && equalityVal->type == TYPE_NUMBER) {
	    value = equalityVal->literal.literal_num;
	    value_free(equalityVal); value_free(equalityRVal);
	} else if (equalityRVal->type == TYPE_NUMBER && equalityVal->type == TYPE_NUMBER) {
	    // Apply the operation based on equalityRVal's metadata
	    value = equalityVal->literal.literal_num;
	    double rvalue = equalityRVal->literal.literal_num;
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

ExecValue *execEquality(ASTNode *equality)
{
    if (equality->type != SYM_EQUALITY)
	return criticalError("Given invalid value for execEqualityR.");
    
    // Check if equality is COMPARISON EQUALITY_R
    if (equality->numChildren == 0)
	return criticalError("Expected children for equality.");
    if (equality->numChildren == 2) {
	if (equality->children[0]->type != SYM_COMPARISON || equality->children[1]->type != SYM_EQUALITY_R)
	    return criticalError("Invalid child type for EQUALITY.");

	ExecValue *comparisonVal = execComparison(equality->children[0]);
	ExecValue *equalityRVal = execEqualityR(equality->children[1]);
	double value;
	
	if (equalityRVal->type == TYPE_NULL && comparisonVal->type == TYPE_NUMBER) {
	    value = comparisonVal->literal.literal_num;
	    value_free(comparisonVal); value_free(equalityRVal);
	    return value_newNumber(value);
	} else if (equalityRVal->type == TYPE_NUMBER && comparisonVal->type == TYPE_NUMBER) {
	    // Apply the operation based on equalityRVal's metadata
	    value = comparisonVal->literal.literal_num;
	    double rvalue = equalityRVal->literal.literal_num;
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

ExecValue *execExpr(ASTNode *expr)
{
    if (expr->type != SYM_EXPR)
	return criticalError("Given invalid value for execExpr.");

    if (expr->numChildren != 1)
	return criticalError("Number of children of expr not 1.");

    return execEquality(expr->children[0]);
}

ExecValue *execPrntStmt(ASTNode *prntStmt)
{
    if (prntStmt->numChildren == 3 &&
	prntStmt->children[0]->tok->type == TOKEN_PRINT &&
	prntStmt->children[1]->type == SYM_EXPR &&
	prntStmt->children[2]->tok->type == TOKEN_NL) {
	ExecValue *exprResult = execExpr(prntStmt->children[1]);

	switch (exprResult->type) {
	case TYPE_STRING:
	    printf("%s", exprResult->literal.literal_str);
	    break;
	case TYPE_NUMBER:
	    printf("%f", exprResult->literal.literal_num);
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

ExecValue *execExprStmt(ASTNode *exprStmt)
{
    if (exprStmt->numChildren == 2 &&
	exprStmt->children[0]->type == SYM_EXPR &&
	exprStmt->children[1]->tok->type == TOKEN_NL) {
	ExecValue *exprResult = execExpr(exprStmt->children[0]);
	value_free(exprResult);
	return value_newNull();
    }
    return criticalError("Invalid expr statement.");
}

ExecValue *execStmt(ASTNode *stmt)
{
    if (stmt->numChildren == 1) {
	if (stmt->children[0]->type == SYM_EXPR_STMT)
	    return execExprStmt(stmt->children[0]);
	if (stmt->children[0]->type == SYM_PRNT_STMT)
	    return execPrntStmt(stmt->children[0]);
    }
    return criticalError("Invalid statement.");
}

ExecValue *execAsmt(ASTNode *asmt)
{
    if (asmt->numChildren == 4 &&
	asmt->children[0]->tok->type == TOKEN_IDENTIFIER &&
	asmt->children[1]->tok->type == TOKEN_EQUAL &&
	asmt->children[2]->type == SYM_EXPR &&
	asmt->children[3]->tok->type == TOKEN_NL) {
	ExecValue *rvalue = execExpr(asmt->children[2]);
	// TODO: implement symbol table assignment
	value_free(rvalue);
	return value_newNull();
    }
    return criticalError("Invalid assignment.");
}

ExecValue *execLine(ASTNode *line)
{
    if (line->numChildren == 1) {
	if (line->children[0]->type == SYM_ASMT)
	    return execAsmt(line->children[0]);
	if (line->children[0]->type == SYM_STMT)
	    return execStmt(line->children[0]);
    }
    return criticalError("Invalid line.");
}

ExecValue *execStart(ASTNode *start)
{
    // Returns the execution exit code
    //TODO: all runtime errors here
    int exitCode = 0;
    for (size_t i = 0; i < start->numChildren; i++) {
	ASTNode *child = start->children[0];
	ExecValue *result;
	if (child->type == SYM_LINE)
	    result = execLine(child);
	else if (child->tok->type == TOKEN_EOF)
	    break;
	else
	    return criticalError("Unexpected symbol.");

	value_free(result);
    }

    return value_newNumber(exitCode);
}
