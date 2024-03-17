#include <stdbool.h>
#ifndef _TOKEN_H_
#define _TOKEN_H_
#define MAX_LEXEME_SIZE 255

typedef enum {
    // Keywords: Control Flow
    TOKEN_IF,
    TOKEN_ELSE,
    TOKEN_WHILE, 
    TOKEN_FOR, 
    TOKEN_IN,
    TOKEN_END,
    TOKEN_BREAK, 
    TOKEN_CONTINUE,
    // Keywords: Functions
    TOKEN_FUNCTION,
    TOKEN_RETURN,
    TOKEN_PRINT,
    TOKEN_NEW,    // Used to create a new class instance
    // Keywords: Logical Operators
    TOKEN_AND,
    TOKEN_OR,
    TOKEN_NOT,
    TOKEN_ISA,    // Used to check super-classes
    TOKEN_TRUE, TOKEN_FALSE,
    // Identifiers/Literals
    TOKEN_IDENTIFIER,
    TOKEN_STRING,
    TOKEN_NUMBER,
    // Brackets
    TOKEN_PAREN_L, TOKEN_PAREN_R, // ( )
    TOKEN_BRACK_L, TOKEN_BRACK_R, // [ ]
    TOKEN_BRACE_L, TOKEN_BRACE_R, // { }
    // Single-character tokens
    TOKEN_PLUS, 
    TOKEN_MINUS,
    TOKEN_STAR, 
    TOKEN_SLASH,
    TOKEN_PERCENT,
    TOKEN_CARET,
    TOKEN_COLON,  // Used for maps and slices
    TOKEN_PERIOD, // Used for methods
    TOKEN_EQUAL,
    TOKEN_BANG,
    TOKEN_AT,

    // Multi-character tokens
    TOKEN_EQUAL_EQUAL,   // ==
    TOKEN_NOT_EQUAL,     // !=
    TOKEN_GREATER,       // >
    TOKEN_GREATER_EQUAL, // >=
    TOKEN_LESS,          // <
    TOKEN_LESS_EQUAL,    // <=
    TOKEN_SLASH_SLASH,   // //
    // Others
    TOKEN_EOF, TOKEN_UNKNOWN,
} TokenType;

// You can generate the below list from the above enum with enum_to_map.py
static const char* TokenTypeString[] = {"TOKEN_IF", "TOKEN_ELSE", "TOKEN_WHILE", "TOKEN_FOR", "TOKEN_IN", "TOKEN_END", "TOKEN_BREAK", "TOKEN_CONTINUE", "TOKEN_FUNCTION", "TOKEN_RETURN", "TOKEN_PRINT", "TOKEN_NEW", "TOKEN_AND", "TOKEN_OR", "TOKEN_NOT", "TOKEN_ISA", "TOKEN_TRUE", "TOKEN_FALSE", "TOKEN_IDENTIFIER", "TOKEN_STRING", "TOKEN_NUMBER", "TOKEN_PAREN_L", "TOKEN_PAREN_R", "TOKEN_BRACK_L", "TOKEN_BRACK_R", "TOKEN_BRACE_L", "TOKEN_BRACE_R", "TOKEN_PLUS", "TOKEN_MINUS", "TOKEN_STAR", "TOKEN_SLASH", "TOKEN_PERCENT", "TOKEN_CARET", "TOKEN_COLON", "TOKEN_PERIOD", "TOKEN_EQUAL", "TOKEN_BANG", "TOKEN_AT", "TOKEN_EQUAL_EQUAL", "TOKEN_NOT_EQUAL", "TOKEN_GREATER", "TOKEN_GREATER_EQUAL", "TOKEN_LESS", "TOKEN_LESS_EQUAL", "TOKEN_SLASH_SLASH", "TOKEN_EOF", "TOKEN_UNKNOWN"};

//TODO: Fix colNum
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
    // const int colNum;
} Token;

// Generates a new token. If the token type is a literal, the literal will be automatically generated. If the token is a literal number and exceeds the range for a double, errno will be set to ERANGE.
Token* createToken(TokenType type, const char* lexeme, const int lexemeLength, int lineNum);
// Returns a boolean if word matches test over length testLen
bool exactMatch(const char* word, const char* test, const int testLen);
// Returns a TokenType to match the keyword, or TOKEN_IDENTIFIER if it cannot be identified as a keyword.
TokenType matchTokenKeywordIdentifier(const char* lexeme, const int lexemeLength);
// Frees the memory associated with the token
void freeToken(Token* token); 
// Prints formatted Token string
void printToken(Token *token);

#endif
