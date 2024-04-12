#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include "../logger/logger.h"
#include "execvalue.h"

ExecValue *value_newNull()
{
    ExecValue *val = malloc(sizeof(ExecValue));
    val->type = TYPE_NULL;
    val->value.literal_null = NULL;
    val->tok = token_new(TOKEN_NULL, "null", 4, -1, -1);
    return val;
}

ExecValue *value_newString(char *strValue, Token *tokPtr)
{
    ExecValue *val = malloc(sizeof(ExecValue));
    val->type = TYPE_STRING;

    // Create null-terminated copy of the string
    char *str_cpy = strdup(strValue);
    val->value.literal_str = str_cpy;
    val->tok = tokPtr;
    return val;
}

ExecValue *value_newNumber(double numValue, Token *tokPtr)
{
    ExecValue *val = malloc(sizeof(ExecValue));
    val->type = TYPE_NUMBER;
    val->value.literal_num = numValue;
    val->tok = tokPtr;
    return val;
}

ExecValue *value_newIdentifier(char *identifierName, Token *tokPtr)
{
    ExecValue *val = malloc(sizeof(ExecValue));
    val->type = TYPE_IDENTIFIER;

    // Create null-terminated copy of the identifier name
    char *name_cpy = strdup(identifierName);
    val->value.identifier_name = name_cpy;
    val->tok = tokPtr;
    return val;
}

ExecValue *value_newError(Error *err, Token *tokPtr)
{
    ExecValue *val = malloc(sizeof(ExecValue));
    val->type = TYPE_ERROR;
    val->value.error_ptr = err;
    val->tok = tokPtr;
    if (val->tok != NULL) {
        err->lineNum = val->tok->lineNum;
        err->colNum = val->tok->colNum;
    } else {
        err->lineNum = -1;
        err->colNum = -1;
    }
    return val;
}

ExecValue *value_newFunction(ASTNode *argList, ASTNode *block, Token *tokPtr)
{
    ExecValue *val = malloc(sizeof(ExecValue));
    FunctionRef* fnRef = malloc(sizeof(FunctionRef));
    fnRef->argList = astnode_clone(argList);
    fnRef->fnBlk = astnode_clone(block);
    val->type = TYPE_FUNCTION;
    val->value.function_ref = fnRef;
    val->tok = tokPtr;
    return val;
}

ExecValue *value_clone(ExecValue *value)
{
    switch (value->type) {
    case TYPE_STRING: return value_newString(value->value.literal_str, value->tok);
    case TYPE_NUMBER: return value_newNumber(value->value.literal_num, value->tok);
    case TYPE_NULL: return value_newNull();
    case TYPE_IDENTIFIER: return value_newIdentifier(value->value.identifier_name, value->tok);
    case TYPE_FUNCTION: {
        ASTNode *argList = value->value.function_ref->argList;
        ASTNode *block   = value->value.function_ref->fnBlk;
        return value_newFunction(argList, block, value->tok);
    }
    case TYPE_ERROR: return value_newError(value->value.error_ptr, value->tok);
    default:
        log_message(&executionLogger, "Critical Error: value_clone: Unknown ValueType %d.\n", value->type);
        exit(1);
    }
    return NULL;
}

void value_free(ExecValue *value)
{
    switch (value->type) {
    case TYPE_STRING: free(value->value.literal_str); break;
    case TYPE_IDENTIFIER: free(value->value.identifier_name); break;
    case TYPE_ERROR: error_free(value->value.error_ptr); break;
    case TYPE_FUNCTION: {
        FunctionRef *ref = value->value.function_ref;
        astnode_free(ref->argList);
        astnode_free(ref->fnBlk);
        free(value->value.function_ref);
        break;
    }
    default: break;
    }
    free(value);
}

int value_falsiness(ExecValue *e)
{
    switch (e->type) {
    case TYPE_NULL: return 0; // NULL is FALSE
    case TYPE_NUMBER: return (e->value.literal_num != 0);  // any number other than 0 is TRUE
    case TYPE_STRING: return (strlen(e->value.literal_str) != 0); // any string other than "" is TRUE
    case TYPE_ERROR:
        criticalError("value_falsiness: Tried to get the falsiness of an error.");
        exit(1);
    case TYPE_IDENTIFIER:
        criticalError("value_falsiness: pass the VALUE of the identifier into this function with context_getValue(), instead of the identifier itself.");
        exit(1);
    case TYPE_FUNCTION: {
        Error *typeErr = error_new(ERR_RUNTIME_TYPE, -1, -1);
        snprintf(typeErr->message, MAX_ERRMSG_LEN, "tried to get the falsiness of a function.");
        return -1;
    }
    case TYPE_UNASSIGNED:
        criticalError("value_falsiness: passed an unassigned value..");
        exit(1);
    }
    return -1;
}

ExecValue* value_opUnaryPos(ExecValue *e)
{
    if (e == NULL)
        criticalError("pos: e1 or e2 is null");
    if (e->type == TYPE_NULL || e->type == TYPE_STRING) {
        Error *typeErr = error_new(ERR_RUNTIME_TYPE, -1, -1);
        snprintf(typeErr->message, MAX_ERRMSG_LEN, "unary positive expects a number, instead got %s", ValueTypeString[e->type]);
        return value_newError(typeErr, e->tok);
    }
    if (e->type == TYPE_IDENTIFIER)
        criticalError("pos: pass the VALUE of the identifier into this function with context_getValue(), instead of the identifier itself.");

    double result = e->value.literal_num;
    return value_newNumber(result, e->tok);
}

ExecValue* value_opUnaryNeg(ExecValue *e)
{
    if (e == NULL)
        criticalError("neg: e1 or e2 is null");
    if (e->type == TYPE_NULL || e->type == TYPE_STRING) {
        Error *typeErr = error_new(ERR_RUNTIME_TYPE, -1, -1);
        snprintf(typeErr->message, MAX_ERRMSG_LEN, "unary negative expects a number, instead got %s", ValueTypeString[e->type]);
        return value_newError(typeErr, e->tok);
    }
    if (e->type == TYPE_IDENTIFIER)
        criticalError("neg: pass the VALUE of the identifier into this function with context_getValue(), instead of the identifier itself.");

    double result = -e->value.literal_num;
    return value_newNumber(result, e->tok);
}

ExecValue* value_opNot(ExecValue *e)
{
    if (e == NULL)
        criticalError("not: e is null");

    double result = !value_falsiness(e);
    return value_newNumber(result, e->tok);
}

ExecValue* value_opOr(ExecValue *e1, ExecValue *e2)
{
    if (e1 == NULL || e2 == NULL)
        criticalError("or: e1 or e2 is null");

    double result = value_falsiness(e1) || value_falsiness(e2);
    return value_newNumber(result, e1->tok);
}

ExecValue* value_opAnd(ExecValue *e1, ExecValue *e2)
{
    if (e1 == NULL || e2 == NULL)
        criticalError("and: e1 or e2 is null");

    double result = value_falsiness(e1) && value_falsiness(e2);
    return value_newNumber(result, e1->tok);
}

ExecValue* value_opAdd(ExecValue *e1, ExecValue *e2)
{
    if (e1 == NULL || e2 == NULL)
        criticalError("add: e1 or e2 is null");
    if (e1->type == TYPE_IDENTIFIER || e2->type == TYPE_IDENTIFIER)
        criticalError("add: pass the VALUE of the identifier into this function with context_getValue(), instead of the identifier itself.");

    if (e1->type == TYPE_NUMBER && e2->type == TYPE_NUMBER) {
        double result = e1->value.literal_num + e2->value.literal_num;
        return value_newNumber(result, e1->tok);
    } else if (e1->type == TYPE_STRING && e2->type == TYPE_STRING) {
        char *s1 = e1->value.literal_str;
        char *s2 = e2->value.literal_str;
        size_t resultLen = strlen(s1) + strlen(s2);
        char *result = malloc((resultLen + 1) * sizeof(char));
        strncpy(result, s1, strlen(s1));
        strncpy(result + strlen(s1), s2, strlen(s2));
        result[resultLen] = '\0';
        ExecValue *resultVal = value_newString(result, e1->tok);
        free(result);
        return resultVal;
    }

    // Invalid types
    Error *typeErr = error_new(ERR_RUNTIME_TYPE, -1, -1);
    snprintf(typeErr->message, MAX_ERRMSG_LEN, "addition expects two strings or two numbers, instead got values of type %s and %s", ValueTypeString[e1->type], ValueTypeString[e2->type]);
    if (e1->type == TYPE_NUMBER || e1->type == TYPE_STRING)
        return value_newError(typeErr, e2->tok);
    else
        return value_newError(typeErr, e1->tok);
}

ExecValue* value_opSub(ExecValue *e1, ExecValue *e2)
{
    if (e1 == NULL || e2 == NULL)
        criticalError("sub: e1 or e2 is null");
    if (e1->type == TYPE_IDENTIFIER || e2->type == TYPE_IDENTIFIER)
        criticalError("sub: pass the VALUE of the identifier into this function with context_getValue(), instead of the identifier itself.");

    if (e1->type == TYPE_NUMBER && e2->type == TYPE_NUMBER) {
        double result = e1->value.literal_num - e2->value.literal_num;
        return value_newNumber(result, e1->tok);
    } else if (e1->type == TYPE_STRING && e2->type == TYPE_STRING) {
        // Delete s2 from the end of s1, assuming s2 is an exact match of the end of s1
        char *s1 = e1->value.literal_str;
        char *s2 = e2->value.literal_str;
        size_t s1Len = strlen(s1);
        size_t s2Len = strlen(s2);
        if (s1Len < s2Len) {
            // Impossible for s2 to be an exact match of s1
            return value_newString(s1, e1->tok);
        }
        
        int matching = 0; // true if we're in the matching state
        size_t s2Idx = s2Len - 1;
        size_t s1StartOfS2 = s1Len;

        short match = 0;

        for (size_t i = s1Len-1; i >= 0; i--) {
            if (s2Idx == -1) {
                // Exact match found
                match = 1;
                break;
            }
            if (s1[i] != s2[s2Idx]) {
                // Definitely not an exact match
                match = 0;
                break;
            }
            s2Idx--;
        }

        if (!match)
            return value_newString(s1, e1->tok);

        // Exact match: can safely just copy exactly resultLen characters starting from s1
        size_t resultLen = s1Len - s2Len;
        char *resultString = malloc(sizeof(char) * (resultLen + 1));
        strncpy(resultString, s1, resultLen);
        resultString[resultLen] = '\0';
        ExecValue *resultVal = value_newString(resultString, e1->tok);
        free(resultString);
        return resultVal;
    }

    // Invalid types
    Error *typeErr = error_new(ERR_RUNTIME_TYPE, -1, -1);
    snprintf(typeErr->message, MAX_ERRMSG_LEN, "subtraction expects two numbers or two strings, instead got values of type %s and %s", ValueTypeString[e1->type], ValueTypeString[e2->type]);
    if (e1->type == TYPE_NUMBER)
        return value_newError(typeErr, e2->tok);
    else
        return value_newError(typeErr, e1->tok);
}

ExecValue* value_opMul(ExecValue *e1, ExecValue *e2)
{
    if (e1 == NULL || e2 == NULL)
        criticalError("mul: e1 or e2 is null");
    if (e1->type == TYPE_IDENTIFIER || e2->type == TYPE_IDENTIFIER)
        criticalError("mul: pass the VALUE of the identifier into this function with context_getValue(), instead of the identifier itself.");

    if (e1->type == TYPE_NUMBER && e2->type == TYPE_NUMBER) {
        double result = e1->value.literal_num * e2->value.literal_num;
        return value_newNumber(result, e1->tok);
    }
    if (e1->type == TYPE_STRING && e2->type == TYPE_NUMBER) {
        double multiplier = e2->value.literal_num;
        if (multiplier < 0)
            multiplier = 0;
        char *str = e1->value.literal_str;
        size_t origLen = strlen(str);
        size_t resultLen = (size_t) ((double) origLen * multiplier);
        char *newStr = malloc((resultLen + 1) * sizeof(char));
        // Start copying the str into newStr until we reach the end of string
        for (size_t i = 0; i < resultLen; i++)
            newStr[i] = *(str + (i % origLen));
        newStr[resultLen] = '\0';
        ExecValue *resultVal = value_newString(newStr, e1->tok);
        free(newStr);
        return resultVal;
    }

    // Invalid types
    Error *typeErr = error_new(ERR_RUNTIME_TYPE, -1, -1);
    snprintf(typeErr->message, MAX_ERRMSG_LEN, "multiplication expects two numbers or a string and a number, instead got values of type %s and %s", ValueTypeString[e1->type], ValueTypeString[e2->type]);
    if (e1->type == TYPE_NUMBER)
        return value_newError(typeErr, e2->tok);
    else
        return value_newError(typeErr, e1->tok);
}

ExecValue* value_opDiv(ExecValue *e1, ExecValue *e2)
{
    if (e1 == NULL || e2 == NULL)
        criticalError("div: e1 or e2 is null");
    if (e1->type == TYPE_IDENTIFIER || e2->type == TYPE_IDENTIFIER)
        criticalError("div: pass the VALUE of the identifier into this function with context_getValue(), instead of the identifier itself.");

    if (e1->type == TYPE_NUMBER && e2->type == TYPE_NUMBER) {
        double result = e1->value.literal_num / e2->value.literal_num;
        return value_newNumber(result, e1->tok);
    }
    if (e1->type == TYPE_STRING && e2->type == TYPE_NUMBER) {
        double multiplier = e2->value.literal_num;
        if (multiplier < 0)
            multiplier = 0;
        char *str = e1->value.literal_str;
        size_t origLen = strlen(str);
        size_t resultLen = (size_t) ((double) origLen / multiplier);
        char *newStr = malloc((resultLen + 1) * sizeof(char));
        // Start copying the str into newStr until we reach the end of string
        for (size_t i = 0; i < resultLen; i++)
            newStr[i] = *(str + (i % origLen));
        newStr[resultLen] = '\0';
        ExecValue *resultVal = value_newString(newStr, e1->tok);
        free(newStr);
        return resultVal;
    }

    // Invalid types
    Error *typeErr = error_new(ERR_RUNTIME_TYPE, -1, -1);
    snprintf(typeErr->message, MAX_ERRMSG_LEN, "division expects two numbers or a string and a number, instead got values of type %s and %s", ValueTypeString[e1->type], ValueTypeString[e2->type]);
    if (e1->type == TYPE_NUMBER)
        return value_newError(typeErr, e2->tok);
    else
        return value_newError(typeErr, e1->tok);
}

ExecValue* value_opMod(ExecValue *e1, ExecValue *e2)
{
    if (e1 == NULL || e2 == NULL)
        criticalError("mod: e1 or e2 is null");
    if (e1->type == TYPE_IDENTIFIER || e2->type == TYPE_IDENTIFIER)
        criticalError("mod: pass the VALUE of the identifier into this function with context_getValue(), instead of the identifier itself.");

    if (e1->type == TYPE_NUMBER && e2->type == TYPE_NUMBER) {
        double result = fmod(e1->value.literal_num, e2->value.literal_num);
        return value_newNumber(result, e1->tok);
    }

    // Invalid types
    Error *typeErr = error_new(ERR_RUNTIME_TYPE, -1, -1);
    snprintf(typeErr->message, MAX_ERRMSG_LEN, "modulo expects two numbers, instead got values of type %s and %s", ValueTypeString[e1->type], ValueTypeString[e2->type]);
    if (e1->type == TYPE_NUMBER)
        return value_newError(typeErr, e2->tok);
    else
        return value_newError(typeErr, e1->tok);
}

ExecValue* value_opPow(ExecValue *e1, ExecValue *e2)
{
    if (e1 == NULL || e2 == NULL)
        criticalError("pow: e1 or e2 is null");
    if (e1->type == TYPE_IDENTIFIER || e2->type == TYPE_IDENTIFIER)
        criticalError("pow: pass the VALUE of the identifier into this function with context_getValue(), instead of the identifier itself.");

    if (e1->type == TYPE_NUMBER && e2->type == TYPE_NUMBER) {
        double result = pow(e1->value.literal_num, e2->value.literal_num);
        return value_newNumber(result, e1->tok);
    }

    // Invalid types
    Error *typeErr = error_new(ERR_RUNTIME_TYPE, -1, -1);
    snprintf(typeErr->message, MAX_ERRMSG_LEN, "power expects two numbers, instead got values of type %s and %s", ValueTypeString[e1->type], ValueTypeString[e2->type]);
    if (e1->type == TYPE_NUMBER)
        return value_newError(typeErr, e2->tok);
    else
        return value_newError(typeErr, e1->tok);
}

ExecValue* value_opEqEq(ExecValue *e1, ExecValue *e2)
{
    if (e1 == NULL || e2 == NULL)
        criticalError("eq: e1 or e2 is null");
    if (e1->type == TYPE_IDENTIFIER || e2->type == TYPE_IDENTIFIER)
        criticalError("eq: pass the VALUE of the identifier into this function with context_getValue(), instead of the identifier itself.");

    if (e1->type == TYPE_NUMBER && e2->type == TYPE_NUMBER) {
        double result = e1->value.literal_num == e2->value.literal_num;
        return value_newNumber(result, e1->tok);
    } else if (e1->type == TYPE_STRING && e2->type == TYPE_STRING) {
        char *s1 = e1->value.literal_str;
        char *s2 = e2->value.literal_str;
        double result = 1;

        if (strlen(s1) != strlen(s2))
            result = 0;
        else
            result = (strncmp(s1, s2, strlen(s1)) == 0) ? 1 : 0;
        return value_newNumber(result, e1->tok);
    } else if (e1->type == TYPE_NULL && e2->type == TYPE_NULL) {
        return value_newNumber(1, e1->tok);
    }
    
    // different types, so not equal
    return value_newNumber(0, e1->tok);
}

ExecValue* value_opNEq(ExecValue *e1, ExecValue *e2)
{
    if (e1 == NULL || e2 == NULL)
        criticalError("neq: e1 or e2 is null");
    if (e1->type == TYPE_IDENTIFIER || e2->type == TYPE_IDENTIFIER)
        criticalError("neq: pass the VALUE of the identifier into this function with context_getValue(), instead of the identifier itself.");
    
    ExecValue *eqeqRes = value_opEqEq(e1, e2);
    eqeqRes->value.literal_num = !eqeqRes->value.literal_num;
    return eqeqRes;
}

ExecValue* value_opGt(ExecValue *e1, ExecValue *e2)
{
    if (e1 == NULL || e2 == NULL)
        criticalError("gt: e1 or e2 is null");
    if (e1->type == TYPE_IDENTIFIER || e2->type == TYPE_IDENTIFIER)
        criticalError("gt: pass the VALUE of the identifier into this function with context_getValue(), instead of the identifier itself.");

    if (e1->type == TYPE_NUMBER && e2->type == TYPE_NUMBER) {
        double result = e1->value.literal_num > e2->value.literal_num;
        return value_newNumber(result, e1->tok);
    } else if (e1->type == TYPE_STRING && e2->type == TYPE_STRING) {
        // e1 is gt if it "collates after" e2 i.e. if the first non-matching char in e1 is greater than e2 in ASCII
        // We compare them by strcmp with the smaller size, and if it's still a match, the string with the larger size is greater
        char *s1 = e1->value.literal_str; 
        char *s2 = e2->value.literal_str;
        size_t s1Len = strlen(s1);
        size_t s2Len = strlen(s2);
        size_t smallerLen = (s1Len < s2Len) ? s1Len : s2Len;

        int comparison = strncmp(s1, s2, smallerLen);
        if (comparison != 0) {
            double value = (comparison > 0) ? 1 : 0;
            return value_newNumber(value, e1->tok);
        }
        
        if (s1Len > s2Len)
            return value_newNumber(1, e1->tok);
        return value_newNumber(0, e1->tok);
    }

    // Invalid types
    Error *typeErr = error_new(ERR_RUNTIME_TYPE, -1, -1);
    snprintf(typeErr->message, MAX_ERRMSG_LEN, "greaterThan expects two numbers or two strings, instead got values of type %s and %s", ValueTypeString[e1->type], ValueTypeString[e2->type]);
    if (e1->type == TYPE_NUMBER || e1->type == TYPE_STRING)
        return value_newError(typeErr, e2->tok);
    else
        return value_newError(typeErr, e1->tok);
}

ExecValue* value_opGEq(ExecValue *e1, ExecValue *e2)
{
    if (e1 == NULL || e2 == NULL)
        criticalError("geq: e1 or e2 is null");
    if (e1->type == TYPE_IDENTIFIER || e2->type == TYPE_IDENTIFIER)
        criticalError("geq: pass the VALUE of the identifier into this function with context_getValue(), instead of the identifier itself.");

    if (e1->type == TYPE_NUMBER && e2->type == TYPE_NUMBER) {
        double result = e1->value.literal_num >= e2->value.literal_num;
        return value_newNumber(result, e1->tok);
    } else if (e1->type == TYPE_STRING && e2->type == TYPE_STRING) {
        ExecValue *gt = value_opGt(e1, e2);
        if (gt->value.literal_num == 0) {
            value_free(gt);
            return value_opEqEq(e1, e2);
        }
        return gt;
    }

    // Invalid types
    Error *typeErr = error_new(ERR_RUNTIME_TYPE, -1, -1);
    snprintf(typeErr->message, MAX_ERRMSG_LEN, "greaterThanOrEqualTo expects two numbers or two strings, instead got values of type %s and %s", ValueTypeString[e1->type], ValueTypeString[e2->type]);
    if (e1->type == TYPE_NUMBER || e1->type == TYPE_STRING)
        return value_newError(typeErr, e2->tok);
    else
        return value_newError(typeErr, e1->tok);
}

ExecValue* value_opLt(ExecValue *e1, ExecValue *e2)
{
    if (e1 == NULL || e2 == NULL)
        criticalError("lt: e1 or e2 is null");
    if (e1->type == TYPE_IDENTIFIER || e2->type == TYPE_IDENTIFIER)
        criticalError("lt: pass the VALUE of the identifier into this function with context_getValue(), instead of the identifier itself.");

    if (e1->type == TYPE_NUMBER && e2->type == TYPE_NUMBER) {
        double result = e1->value.literal_num < e2->value.literal_num;
        return value_newNumber(result, e1->tok);
    } else if (e1->type == TYPE_STRING && e2->type == TYPE_STRING) {
        // < is the complement of >=
        ExecValue *geq = value_opGEq(e1, e2);
        geq->value.literal_num = (geq->value.literal_num == 0) ? 1 : 0;
        return geq;
    }

    // Invalid types
    Error *typeErr = error_new(ERR_RUNTIME_TYPE, -1, -1);
    snprintf(typeErr->message, MAX_ERRMSG_LEN, "lessThan expects two numbers or two strings, instead got values of type %s and %s", ValueTypeString[e1->type], ValueTypeString[e2->type]);
    if (e1->type == TYPE_NUMBER || e1->type == TYPE_STRING)
        return value_newError(typeErr, e2->tok);
    else
        return value_newError(typeErr, e1->tok);
}

ExecValue* value_opLEq(ExecValue *e1, ExecValue *e2)
{
    if (e1 == NULL || e2 == NULL)
        criticalError("leq: e1 or e2 is null");
    if (e1->type == TYPE_IDENTIFIER || e2->type == TYPE_IDENTIFIER)
        criticalError("leq: pass the VALUE of the identifier into this function with context_getValue(), instead of the identifier itself.");

    if (e1->type == TYPE_NUMBER && e2->type == TYPE_NUMBER) {
        double result = e1->value.literal_num <= e2->value.literal_num;
        return value_newNumber(result, e1->tok);
    } else if (e1->type == TYPE_STRING && e2->type == TYPE_STRING) {
        // <= is the complement of >
        ExecValue *gt = value_opGt(e1, e2);
        gt->value.literal_num = (gt->value.literal_num == 0) ? 1 : 0;
        return gt;
    }

    // Invalid types
    Error *typeErr = error_new(ERR_RUNTIME_TYPE, -1, -1);
    snprintf(typeErr->message, MAX_ERRMSG_LEN, "lessThanOrEqualTo expects two numbers or two strings, instead got values of type %s and %s", ValueTypeString[e1->type], ValueTypeString[e2->type]);
    if (e1->type == TYPE_NUMBER || e1->type == TYPE_STRING)
        return value_newError(typeErr, e2->tok);
    else
        return value_newError(typeErr, e1->tok);
}
