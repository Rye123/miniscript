#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define ERR_BUF_SZ 255
#include "../lexer/lexer.h"
#include "../lexer/token.h"

void compare_lists(Token** actual, size_t actualSz, Token** expected);

void test_assert(int cond_result, const char* resultPass, const char* resultFail)
{
    if (cond_result)
        printf("\033[92mPASS: %s\033[0m\n", resultPass);
    else {
        printf("\033[91mFAIL: %s\033[0m\n", resultFail);
        exit(1);
    }
}

void freeTokenArr(Token **tokens)
{
    size_t i = 0;
    Token *tok;
    while (1) {
	tok = *(tokens + i);
	if (tok->type == TOKEN_EOF) {
	    token_free(tok);
	    return;
	}
	token_free(tok);
	i++;
    }
}

void resetTokens(Token ***tokenPtr, size_t *tokenSz)
{
    freeTokenArr(*tokenPtr);
    *tokenPtr = realloc(*tokenPtr, sizeof(Token *) * 0);
    *tokenSz = 0;
}

int main()
{
    Token **tokens = malloc(sizeof(Token *) * 0);
    size_t tokenSz = 0;

    // CASE 1
    char test1[] = "var 2";
    Token *expected1[] = {
	token_new(TOKEN_IDENTIFIER, "var", 3, 0, 3),
	token_new(TOKEN_NUMBER, "2", 1, 0, 5),
	token_new(TOKEN_EOF, NULL, 0, 0, 6)
    };
    printf("Test: %s\n", test1);
    lex((const Token ***) &tokens, &tokenSz, test1);
    compare_lists(tokens, tokenSz, expected1);
    printf("\n---\n\n");

    // CASE 2
    char test2[] = "a= 2.344+\n5.77n\t\"hello\"";
    resetTokens(&tokens, &tokenSz);
    Token *expected2[] = {
	token_new(TOKEN_IDENTIFIER, "a",         1, 0, 1),
	token_new(TOKEN_EQUAL,      "=",         1, 0, 2), 
	token_new(TOKEN_NUMBER,     "2.344",     5, 0, 8), 
	token_new(TOKEN_PLUS,       "+",         1, 0, 9), 
	token_new(TOKEN_NUMBER,     "5.77",      4, 1, 14), 
	token_new(TOKEN_IDENTIFIER, "n",         1, 1, 15), 
	token_new(TOKEN_STRING,     "\"hello\"", 7, 1, 23), 
	token_new(TOKEN_EOF,        NULL,        0, 1, 24)
    };
    printf("Test: %s\n", test2);
    lex((const Token ***) &tokens, &tokenSz, test2);
    compare_lists(tokens, tokenSz, expected2);
    printf("\n---\n\n");

    // CASE 3:
    char test3[] = "22a\"b\"\n\n\t5.5.5.5\n";
    resetTokens(&tokens, &tokenSz);
    Token *expected3[] = {
	token_new(TOKEN_NUMBER,     "22",    2, 0, 2),
	token_new(TOKEN_IDENTIFIER, "a",     1, 0, 3), 
	token_new(TOKEN_STRING,     "\"b\"", 3, 0, 6), 
	token_new(TOKEN_NUMBER,     "5.5",   3, 2, 12), 
	token_new(TOKEN_NUMBER,     ".5",    2, 2, 14), 
	token_new(TOKEN_NUMBER,     ".5",    2, 2, 16), 
	token_new(TOKEN_EOF,        NULL,    0, 3, 24)
    };
    printf("Test: %s\n", test3);
    lex((const Token ***) &tokens, &tokenSz, test3);
    compare_lists(tokens, tokenSz, expected3);
    printf("\n---\n\n");

    // Cleanup
    resetTokens(&tokens, &tokenSz);
    free(tokens);
    freeTokenArr(expected1);
    freeTokenArr(expected2);
    freeTokenArr(expected3);

    return 0;
}

void compare_lists(Token **actual, size_t actualSz, Token **expected)
{
    char passMsg[ERR_BUF_SZ];
    char errMsg[ERR_BUF_SZ];

    char tokStr1[MAX_LEXEME_SIZE];
    char tokStr2[MAX_LEXEME_SIZE];

    size_t i = 0;
    Token *actTok;
    Token *expTok;
    do {
	if (i >= actualSz)
	    actTok = *(actual + actualSz - 1);
	else
	    actTok = *(actual + i);
	expTok = *(expected + i);

	int result = 0;
	if (expTok->type == TOKEN_EOF)
	    result = expTok->type == actTok->type;
	else
	    result = (expTok->type == actTok->type) &&
		(expTok->lineNum == actTok->lineNum) &&
		(expTok->colNum  == actTok->colNum)  &&
		(strcmp(expTok->lexeme, actTok->lexeme) == 0);
	
	sprintf(passMsg, "actual[%lu] == expected[%lu] == %s(%s, line %d, col %d)",
		i, i,
		TokenTypeString[expTok->type], expTok->lexeme, expTok->lineNum, expTok->colNum
	);
	sprintf(errMsg,  "actual[%lu] != expected[%lu], expected: %s(%s, line %d, col %d), got %s(%s, line %d, col %d)",
		i, i,
		TokenTypeString[expTok->type], expTok->lexeme, expTok->lineNum, expTok->colNum,
		TokenTypeString[actTok->type], actTok->lexeme, actTok->lineNum, actTok->colNum
	);
	test_assert(result, passMsg, errMsg);

	i++;
    } while (expTok->type != TOKEN_EOF);
}
