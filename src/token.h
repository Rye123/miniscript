#ifndef _TOKEN_H_
#define _TOKEN_H_

typedef enum {
  // Keywords
  TOKEN_IF, TOKEN_ELSE, 
  TOKEN_END,
  TOKEN_WHILE,
  TOKEN_FOR, TOKEN_IN,
  TOKEN_BREAK, TOKEN_CONTINUE,
  TOKEN_RETURN, TOKEN_FUNCTION, TOKEN_AT,
  // Single-character tokens
  TOKEN_PLUS, TOKEN_MINUS, TOKEN_STAR, TOKEN_BACKSLASH,
  TOKEN_PERCENT,
  TOKEN_CARET,
  // Others
  TOKEN_PERIOD, TOKEN_COLON, 
  TOKEN_PAREN_L, TOKEN_PAREN_R,
  TOKEN_BRACE_L, TOKEN_BRACE_R,
  TOKEN_CUR_BRACE_L, TOKEN_CUR_BRACE_R,
  TOKEN_INDENT,
  // Logical operators
  TOKEN_AND, TOKEN_OR, TOKEN_NOT,
  // Comparison
  TOKEN_EQUAL, TOKEN_EQUAL_EQUAL, TOKEN_NOT_EQUAL,
  TOKEN_GREATER, TOKEN_GREATER_EQUAL,
  TOKEN_LESS, TOKEN_LESS_EQUAL,
  // Literals
  TOKEN_IDENTIFIER, TOKEN_STRING, TOKEN_NUMBER,
  TOKEN_EOF,

  //TODO: Add intrinsic functions
  TOKEN_PRINT,


} TokenType;

static const char* TokenTypeString[] = {
  "TOKEN_IF", "TOKEN_ELSE", "TOKEN_END", "TOKEN_WHILE", "TOKEN_FOR", "TOKEN_IN", "TOKEN_BREAK", "TOKEN_CONTINUE", "TOKEN_RETURN", "TOKEN_FUNCTION", "TOKEN_AT", "TOKEN_PLUS", "TOKEN_MINUS", "TOKEN_STAR", "TOKEN_BACKSLASH", "TOKEN_PERCENT", "TOKEN_CARET", "TOKEN_PERIOD", "TOKEN_COLON", "TOKEN_PAREN_L", "TOKEN_PAREN_R", "TOKEN_BRACE_L", "TOKEN_BRACE_R", "TOKEN_CUR_BRACE_L", "TOKEN_CUR_BRACE_R", "TOKEN_INDENT", "TOKEN_AND", "TOKEN_OR", "TOKEN_NOT", "TOKEN_EQUAL", "TOKEN_EQUAL_EQUAL", "TOKEN_NOT_EQUAL", "TOKEN_GREATER", "TOKEN_GREATER_EQUAL", "TOKEN_LESS", "TOKEN_LESS_EQUAL", "TOKEN_EOF"
};

typedef struct {
  const TokenType type;
  char* lexeme;
  const union
  {
    void* literal_null;
    double literal_num;
    char* literal_str;
  } literal;
  const int lineNum;
} Token;

// Generates a new token. If the token type is a literal, the literal will be automatically generated. If the token is a literal number and exceeds the range for a double, errno will be set to ERANGE.
Token* CreateToken(TokenType type, const char* lexeme, const int lexemeLength, int lineNum);

// Returns a TokenType to match the keyword, or TOKEN_IDENTIFIER if it cannot be identified as a keyword.
TokenType MatchTokenKeyword(const char* lexeme, const int lexemeLength);

// Frees the memory associated with the token
void FreeToken(Token* token);

#endif