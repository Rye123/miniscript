#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../logger/logger.h"
#include "token.h"

Token *token_new(TokenType type, const char *lexeme, const int lexemeLength, int lineNum, int colNum) {
    Token *ret = malloc(sizeof(Token));
    char *literal_str;

    /* Create a null-terminated copy of the lexeme to be saved */
    char *lexeme_cpy = NULL;
    if (lexeme != NULL) {
        lexeme_cpy = malloc((lexemeLength + 1) * sizeof(char));
        memcpy(lexeme_cpy, lexeme, lexemeLength);
        lexeme_cpy[lexemeLength] = '\0';
    }

    ret->type = type;
    ret->lexeme = lexeme_cpy;
    ret->lineNum = lineNum;
    ret->colNum = colNum;

    if (type == TOKEN_NUMBER) {
        /* Convert from string to double */
        ret->literal.literal_num = strtod(lexeme_cpy, NULL);
    } else if (type == TOKEN_STRING) {
        literal_str = malloc((lexemeLength - 1) * sizeof(char));
        if (!literal_str) {
            free(lexeme_cpy);
            free(ret);
            return NULL;
        }
        strncpy(literal_str, lexeme_cpy + 1, lexemeLength - 2);
        literal_str[lexemeLength - 2] = '\0';
        ret->literal.literal_str = literal_str;
    }
    return ret;
}

Token *token_clone(Token *tok) {
    size_t lexLen = 0;
    if (tok->lexeme != NULL)
        lexLen = strlen(tok->lexeme);
    return token_new(tok->type, tok->lexeme, lexLen, tok->lineNum, tok->colNum);
}

void token_print(Token *token) {
    if (token == NULL) {
        log_message(&executionLogger, "NULL Token.\n");
        return;
    }
    if (token->type == TOKEN_NL)
        log_message(&executionLogger, "Token: { type: %s (%d), lexeme: \"\\n\", line: %d, col: %d }\n",
                    TokenTypeString[token->type], token->type, token->lineNum, token->colNum);
    else
        log_message(&executionLogger, "Token: { type: %s (%d), lexeme: \"%s\", line: %d, col: %d }\n",
                    TokenTypeString[token->type], token->type, token->lexeme, token->lineNum, token->colNum);
}

void token_string(char *str, const Token *token) {
    if (token->type == TOKEN_NL)
        snprintf(str, MAX_LEXEME_SIZE, "Token: { type: %s (%d), lexeme: \"\\n\", line: %d, col: %d }",
                 TokenTypeString[token->type], token->type, token->lineNum, token->colNum);
    else
        snprintf(str, MAX_LEXEME_SIZE, "Token: { type: %s (%d), lexeme: \"%s\", line: %d, col: %d }",
                 TokenTypeString[token->type], token->type, token->lexeme, token->lineNum, token->colNum);
}

void token_free(Token *token) {
    if (token->lexeme != NULL)
        free(token->lexeme);
    free(token);
}
