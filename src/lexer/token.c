#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <float.h>
#include "token.h"

Token* token_new(TokenType type, const char* lexeme, const int lexemeLength, int lineNum, int colNum)
{
    Token* ret = malloc(sizeof(Token));

    // Create a null-terminated copy of the lexeme to be saved
    char* lexeme_cpy = NULL;
    if (lexeme != NULL) {
        lexeme_cpy = malloc((lexemeLength + 1) * sizeof(char));
        memcpy(lexeme_cpy, lexeme, lexemeLength);
        lexeme_cpy[lexemeLength] = '\0';
    }

    if (type == TOKEN_NUMBER) {
        // Convert from string to double
        double literal_num = strtod(lexeme_cpy, NULL);
        Token tok = {type, lexeme_cpy, .literal.literal_num=literal_num, lineNum};
        memcpy(ret, &tok, sizeof(Token));
    } else if (type == TOKEN_STRING) {
        Token tok = {type, lexeme_cpy, .literal.literal_str = lexeme_cpy, lineNum};
        // TODO: remove first and last quotes
        memcpy(ret, &tok, sizeof(Token));
    } else {
        Token tok = {type, lexeme_cpy, .literal.literal_null=NULL, lineNum, colNum};
        memcpy(ret, &tok, sizeof(Token));
    }
    return ret;
}

// Returns true if `test` is an exact match for `word`. `word` should be a null-terminated string.
bool exactMatch(const char* word, const char* test, const int testLen)
{
    return testLen == strlen(word) && (strncmp(word, test, testLen) == 0);
}

void token_print(Token *token)
{
    printf("Token: { type: %s (%d), lexeme: %s, line: %d, col: %d}\n", TokenTypeString[token->type], token->type, token->lexeme, token->lineNum, token->colNum);
}

void token_free(Token* token)
{
    free(token->lexeme);
    free(token);
}
