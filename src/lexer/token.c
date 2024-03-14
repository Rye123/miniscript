#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <float.h>
#include "token.h"

Token* createToken(TokenType type, const char* lexeme, const int lexemeLength, int lineNum)
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
    memcpy(ret, &tok, sizeof(Token));
  } else {
    Token tok = {type, lexeme_cpy, .literal.literal_null=NULL, lineNum};
    memcpy(ret, &tok, sizeof(Token));
  }
  return ret;
}

// Returns true if `test` is an exact match for `word`. `word` should be a null-terminated string.
bool exactMatch(const char* word, const char* test, const int testLen)
{
  return testLen == strlen(word) && (strncmp(word, test, testLen) == 0);
}

TokenType matchTokenKeywordIdentifier(const char* lexeme, const int lexemeLength)
{
  //TODO: Update all possible keywords
  if (exactMatch("and", lexeme, lexemeLength)) return TOKEN_AND;
  else if (exactMatch("or", lexeme, lexemeLength)) return TOKEN_OR;
  else if (exactMatch("if", lexeme, lexemeLength)) return TOKEN_IF;
  else if (exactMatch("else", lexeme, lexemeLength)) return TOKEN_ELSE;
  else if (exactMatch("while", lexeme, lexemeLength)) return TOKEN_WHILE;
  else if (exactMatch("for", lexeme, lexemeLength)) return TOKEN_FOR;
  return TOKEN_IDENTIFIER;
}

void printToken(Token *token)
{
  // printf("Token: %s\n", TokenTypeString[token->type]);
  printf("Token: %i, %s\n", token->type, token->lexeme);
}

void freeToken(Token* token)
{
  free(token->lexeme);
  free(token);
}