#include "token.h"
#include "lexer.h"
#include "../lib/list.h"
#include <string.h>

void lex(const char *source, List* tokenLs)
{
    size_t source_code_length = strlen(source);
    size_t start_position;
    size_t current_position = 0;
    int current_line_number = 1;

    while (current_position < source_code_length){
        switch (source[current_position]){
            case '(': list_add_item(tokenLs, createToken(TOKEN_PAREN_L, &source[current_position], 1, current_line_number)); break;
            case ')': list_add_item(tokenLs, createToken(TOKEN_PAREN_R, &source[current_position], 1, current_line_number)); break;
            case '[': list_add_item(tokenLs, createToken(TOKEN_BRACK_L, &source[current_position], 1, current_line_number)); break;
            case ']': list_add_item(tokenLs, createToken(TOKEN_BRACK_R, &source[current_position], 1, current_line_number)); break;
            case '{': list_add_item(tokenLs, createToken(TOKEN_BRACE_L, &source[current_position], 1, current_line_number)); break;
            case '}': list_add_item(tokenLs, createToken(TOKEN_BRACE_R, &source[current_position], 1, current_line_number)); break;
            case '+': list_add_item(tokenLs, createToken(TOKEN_PLUS, &source[current_position], 1, current_line_number)); break;
            case '-': list_add_item(tokenLs, createToken(TOKEN_MINUS, &source[current_position], 1, current_line_number)); break;
            case '*': list_add_item(tokenLs, createToken(TOKEN_STAR, &source[current_position], 1, current_line_number)); break;
            case '/': 
                switch (source[current_position+1]){
                    case '/': list_add_item(tokenLs, createToken(TOKEN_SLASH_SLASH, &source[current_position], 2, current_line_number)); current_position++; break;
                    default: list_add_item(tokenLs, createToken(TOKEN_SLASH, &source[current_position], 1, current_line_number)); break;
                }
                break;
            case '%': list_add_item(tokenLs, createToken(TOKEN_PERCENT, &source[current_position], 1, current_line_number)); break;
            case '^': list_add_item(tokenLs, createToken(TOKEN_CARET, &source[current_position], 1, current_line_number)); break;
            case ':': list_add_item(tokenLs, createToken(TOKEN_COLON, &source[current_position], 1, current_line_number)); break;
            case '.': list_add_item(tokenLs, createToken(TOKEN_PERIOD, &source[current_position], 1, current_line_number)); break;
            case '=': 
                switch (source[current_position+1]){
                    case '=': list_add_item(tokenLs, createToken(TOKEN_EQUAL_EQUAL, &source[current_position], 2, current_line_number)); current_position++; break;
                    default: list_add_item(tokenLs, createToken(TOKEN_EQUAL, &source[current_position], 1, current_line_number)); break;
                }
                break;
            case '!': 
                switch (source[current_position+1]){
                    case '=': list_add_item(tokenLs, createToken(TOKEN_NOT_EQUAL, &source[current_position], 2, current_line_number)); current_position++; break;
                    default: list_add_item(tokenLs, createToken(TOKEN_BANG, &source[current_position], 1, current_line_number)); break;
                }
                break;
            case '>': 
                switch (source[current_position+1]){
                    case '=': list_add_item(tokenLs, createToken(TOKEN_GREATER_EQUAL, &source[current_position], 2, current_line_number)); current_position++; break;
                    default: list_add_item(tokenLs, createToken(TOKEN_GREATER, &source[current_position], 1, current_line_number)); break;
                }
                break;
            case '<': 
                switch (source[current_position+1]){
                    case '=': list_add_item(tokenLs, createToken(TOKEN_LESS_EQUAL, &source[current_position], 2, current_line_number)); current_position++; break;
                    default: list_add_item(tokenLs, createToken(TOKEN_LESS, &source[current_position], 1, current_line_number)); break;
                }
                break;
            case ' ':
            case '\r':
            case '\t':
                break;
            case '\n': current_line_number++; break;
            case '"':
                // Handle strings
                start_position = current_position + 1; // Ignore the "
                current_position++;
                while (source[current_position] != '"' & current_position < source_code_length){
                    current_position++;
                }
                //TODO: Error checking if end of code
                list_add_item(tokenLs, 
                    createToken(TOKEN_STRING, &source[start_position], current_position-start_position, current_line_number)
                );
                break;
            default:
                // Handle identifiers/keywords
                if (isDigit(source[current_position])){
                    start_position = current_position;
                    bool hasDecimal = false;
                    while (current_position < source_code_length){
                        if (source[current_position] == '.'){
                            if (hasDecimal){
                                current_position--;
                                break;
                            } else {
                                hasDecimal = true;
                                current_position++;
                            }
                        } else if (isDigit(source[current_position])) {
                            current_position++;
                        } else {
                            current_position--;
                            break;
                        }
                    }
                    list_add_item(tokenLs,
                        createToken(TOKEN_NUMBER, &source[start_position], current_position-start_position+1, current_line_number)
                    );
                } else if (isAlpha(source[current_position])) {
                    start_position = current_position;
                    while (isAlphaDigit(source[current_position])){
                        current_position++;
                    }
                    current_position--;

                    TokenType tokenType = matchTokenKeywordIdentifier(&source[start_position], current_position-start_position+1);
                    list_add_item(tokenLs,
                        createToken(tokenType, &source[start_position], current_position-start_position+1, current_position)
                    );
                } else {
                    //TODO: Handle errors
                    printf("Unsupported character\n");
                }
        }
        current_position++;
    }
}

bool isDigit(char c)
{
  return (c >= '0' && c <= '9');
}

bool isAlpha(char c)
{
  return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '_';
}

bool isAlphaDigit(char c)
{
  return isAlpha(c) || isDigit(c);
}
