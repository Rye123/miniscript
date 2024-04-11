#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../logger/logger.h"
#include "../error/error.h"
#include "token.h"
#include "lexer.h"

void lexError(const char *errStr, int lineNum, int colNum, const Error ***errorsPtr, size_t *errorCount)
{
    Error *err = error_new(ERR_TOKEN, lineNum, colNum);
    sprintf(err->message, "%s", errStr);

    // Add Error to list
    *errorCount = *errorCount + 1;
    *errorsPtr = realloc(*errorsPtr, sizeof(Error *) * (*errorCount));
    (*errorsPtr)[*(errorCount) - 1] = err;
}

// Returns true if candidate is an exact match for expected
int strnncmp(const char *candidate, size_t candidateLen, const char *expected, const size_t expectedLen)
{
    if (candidateLen != expectedLen)
        return 0;
    return strncmp(candidate, expected, expectedLen) == 0;
}

TokenType matchKeywordOrIdentifier(const char* candidate, const size_t candidateLen)
{
    if (strnncmp(candidate, candidateLen, "if", 2))
        return TOKEN_IF;
    else if (strnncmp(candidate, candidateLen, "else", 4))
        return TOKEN_ELSE;
    else if (strnncmp(candidate, candidateLen, "while", 5))
        return TOKEN_WHILE;
    else if (strnncmp(candidate, candidateLen, "for", 3))
        return TOKEN_FOR;
    else if (strnncmp(candidate, candidateLen, "in", 2))
        return TOKEN_IN;
    else if (strnncmp(candidate, candidateLen, "end", 3))
        return TOKEN_END;
    else if (strnncmp(candidate, candidateLen, "break", 5))
        return TOKEN_BREAK;
    else if (strnncmp(candidate, candidateLen, "then", 4))
        return TOKEN_THEN;
    else if (strnncmp(candidate, candidateLen, "continue", 8))
        return TOKEN_CONTINUE;
    else if (strnncmp(candidate, candidateLen, "function", 8))
        return TOKEN_FUNCTION;
    else if (strnncmp(candidate, candidateLen, "return", 6))
        return TOKEN_RETURN;
    else if (strnncmp(candidate, candidateLen, "print", 5))
        return TOKEN_PRINT;
    else if (strnncmp(candidate, candidateLen, "new", 3))
        return TOKEN_NEW;
    else if (strnncmp(candidate, candidateLen, "self", 4))
        return TOKEN_SELF;
    else if (strnncmp(candidate, candidateLen, "and", 3))
        return TOKEN_AND;
    else if (strnncmp(candidate, candidateLen, "or", 2))
        return TOKEN_OR;
    else if (strnncmp(candidate, candidateLen, "not", 3))
        return TOKEN_NOT;
    else if (strnncmp(candidate, candidateLen, "isa", 3))
        return TOKEN_ISA;
    else if (strnncmp(candidate, candidateLen, "true", 4))
        return TOKEN_TRUE;
    else if (strnncmp(candidate, candidateLen, "false", 5))
        return TOKEN_FALSE;
    else if (strnncmp(candidate, candidateLen, "null", 4))
        return TOKEN_NULL;
    return TOKEN_IDENTIFIER;
}

int isDigit(char c) { return (c >= '0') && (c <= '9'); }
int isAlpha(char c) { return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '_'; }
int isAlphaDigit(char c) { return isDigit(c) || isAlpha(c); }

// Lexes a single number, starting from lexStart. Returns the position of the end of the lexeme.
size_t lexNumber(const char *source, size_t srcLen, size_t lexStart, size_t *errLen, char *errMsg)
{
    size_t lexEnd = lexStart;
    // Scan until not digit
    for (size_t i = lexStart; isDigit(*(source + i)); i++)
        lexEnd++;

    // Check for floating point
    if (*(source + lexEnd) == '.') {
        size_t dotPosition = lexEnd;
        lexEnd++;
        // Scan until not digit
        for (size_t i = lexEnd; isDigit(*(source + i)); i++)
            lexEnd++;

        // Here, if characters AFTER the dot are NOT digits, then this is a method call (i.e. 1.fn should be parsed as number, dot, identifier)
        if (lexEnd == dotPosition)
            return lexEnd;
    }

    // Check for scientific notation
    if (*(source + lexEnd) == 'e' || *(source + lexEnd) == 'E') {
        size_t ePosition = lexEnd;
        char lookahead1 = (ePosition + 1 >= srcLen) ? '\0' : *(source + ePosition + 1);
        char lookahead2 = (ePosition + 2 >= srcLen) ? '\0' : *(source + ePosition + 2);

        if (lookahead1 == '-') {
            // Lex a negative exponent
            if (!isDigit(lookahead2)) {
                // Here, we have something like "1e-a" -- we expect a digit for
                // the negative exponent, but we got some other character instead
                sprintf(errMsg, "Invalid scientific notation, expected digit after '-', instead got '%c'", lookahead2);
                *errLen = strlen(errMsg);
                lexEnd++;
                return lexEnd;
            }

            // Shift lexEnd to be after the "e-"
            lexEnd += 2;
        } else if (lookahead1 == '+') {
            // Lex a positive exponent
            if (!isDigit(lookahead2)) {
                // Here, we have something like "1e+a" -- we expect a digit for
                // the negative exponent, but we got some other character instead
                sprintf(errMsg, "Invalid scientific notation, expected digit after '+', instead got '%c'", lookahead2);
                *errLen = strlen(errMsg);
                lexEnd++;
                return lexEnd;
            }
            // Shift lexEnd to be after the "e+"
            lexEnd += 2;
        } else {
            // Shift lexEnd to be after the "e"
            lexEnd++;
        }
        // Lex rest of exponent
        for (size_t i = lexEnd; isDigit(*(source + i)); i++)
            lexEnd++;
    }
    return lexEnd;
}
int lex(const Token ***tokensPtr, size_t *tokenCount, const Error ***errorsPtr, size_t *errorCount, const char *source)
{
    size_t srcLen = strlen(source);
    int lineNum = 0;
    int colNum = 0;
    char *errMsg;
    size_t errLen = 0;

    size_t lexStart = 0;  // Start of the lexeme
    size_t lexEnd = -1;   // End of the lexeme
    while (lexStart < srcLen) {
        TokenType tokType = TOKEN_UNKNOWN;
        char lookahead = *(source + lexStart);
        char lookahead2 = (lexStart >= srcLen) ? '\0' : *(source + lexStart + 1);
        errMsg = calloc(MAX_ERRMSG_LEN, sizeof(char));
        errLen = 0;
        lexEnd = lexStart;

        // Scan a single lexeme
        switch (lookahead) {
        case '"': {
            // Possible: Literal String (Note: "'" is not recognised)
            tokType = TOKEN_STRING;
            
            // Positional indicators for error logging
            size_t startLine = lineNum;
            size_t startCol = colNum;

            // Iterate until end of string
            // We want lexStart to be at the first ", and lexEnd to be AFTER the next ".
            lexEnd++; colNum++;
            for (size_t i = lexStart + 1; *(source + i) != '"'; i++) {
                if (*(source + i) == '\n' || *(source + i) == '\0')
                    break;
                lexEnd++; colNum++;
            }

            // INVARIANT: lexEnd either points to the next ", or
            //            lexEnd points to \n or EOF.
            if (*(source + lexEnd) == '\n' || *(source + lexEnd) == '\0') {
                tokType = TOKEN_UNKNOWN;
                lexError("Unterminated string", lineNum, colNum, errorsPtr, errorCount);
                lexEnd = srcLen;
            } else {
                // make lexEnd point to the character AFTER the end quotes.
                lexEnd += 1; colNum += 1; 
            }
            break;
        }
        case '+':
            // Possible: +, +=
            if (lookahead2 == '=') {
                tokType = TOKEN_PLUS_EQUAL;
                lexEnd += 2; colNum += 2;
            } else {
                tokType = TOKEN_PLUS;
                lexEnd++; colNum++;
            }
            break;
        case '-':
            // Possible: -, -=
            if (lookahead2 == '=') {
                tokType = TOKEN_MINUS_EQUAL;
                lexEnd += 2; colNum += 2;
            } else {
                tokType = TOKEN_MINUS;
                lexEnd++; colNum++;
            }
            break;
        case '*':
            // Possible: *, *=
            if (lookahead2 == '=') {
                tokType = TOKEN_STAR_EQUAL;
                lexEnd += 2; colNum += 2;
            } else {
                tokType = TOKEN_STAR;
                lexEnd++; colNum++;
            }
            break;
        case '/':
            // Possible: /, /=. //
            if (lookahead2 == '=') {
                tokType = TOKEN_SLASH_EQUAL;
                lexEnd += 2;
                colNum += 2;
            } else if (lookahead2 == '/') {
                // Comment
                for (int i = lexStart + 1; *(source + i) != '\n' && *(source + i) != '\0'; i++)
                    lexEnd++; colNum++;
                // INVARIANT: Now lexEnd points at the newline/EOF character.
                lexEnd++; colNum++;
            } else {
                tokType = TOKEN_SLASH;
                lexEnd++;
                colNum++;
            }
            break;
        case '%':
            // Possible: %, %=
            if (lookahead2 == '=') {
                tokType = TOKEN_PERCENT_EQUAL;
                lexEnd += 2; colNum += 2;
            } else {
                tokType = TOKEN_PERCENT;
                lexEnd++; colNum++;
            }
            break;
        case '^':
            // Possible: ^, ^=
            if (lookahead2 == '=') {
                tokType = TOKEN_CARET_EQUAL;
                lexEnd += 2; colNum += 2;
            } else {
                tokType = TOKEN_CARET;
                lexEnd++; colNum++;
            }
            break;
        case ':':
            tokType = TOKEN_COLON;
            lexEnd++; colNum++;
            break;
        case '.':
            // Possible: ., Number starting with .
            if (isDigit(lookahead2)) {
                tokType = TOKEN_NUMBER;
                lexEnd = lexNumber(source, srcLen, lexStart, &errLen, errMsg);
                colNum += lexEnd - lexStart;
                if (errLen > 0) {
                    tokType = TOKEN_UNKNOWN;
                    lexError(errMsg, lineNum, colNum, errorsPtr, errorCount);
                }
            } else {
                tokType = TOKEN_PERIOD;
                lexEnd++; colNum++;
            }
            break;
        case '=':
            // Possible: =, ==
            if (lookahead2 == '=') {
                tokType = TOKEN_EQUAL_EQUAL;
                lexEnd += 2; colNum += 2;
            } else {
                tokType = TOKEN_EQUAL;
                lexEnd++; colNum++;
            }
            break;
        case '@':
            tokType = TOKEN_AT;
            lexEnd++; colNum++;
            break;
        case '!':
            // Possible: !=
            if (lookahead2 == '=') {
                tokType = TOKEN_BANG_EQUAL;
                lexEnd += 2; colNum += 2;
            } else {
                // Unknown
                tokType = TOKEN_UNKNOWN;
                sprintf(errMsg, "Unexpected character %c", lookahead);
                lexError(errMsg, lineNum, colNum, errorsPtr, errorCount);
                lexEnd++; colNum++;
            }
            break;
        case '>':
            // Possible: >, >=
            if (lookahead2 == '=') {
                tokType = TOKEN_GREATER_EQUAL;
                lexEnd += 2; colNum += 2;
            } else {
                tokType = TOKEN_GREATER;
                lexEnd++; colNum++;
            }
            break;
        case '<':
            // Possible: <, <=
            if (lookahead2 == '=') {
                tokType = TOKEN_LESS_EQUAL;
                lexEnd += 2; colNum += 2;
            } else {
                tokType = TOKEN_LESS;
                lexEnd++; colNum++;
            }
            break;
        case '(':
            tokType = TOKEN_PAREN_L;
            lexEnd++; colNum++;
            break;
        case ')':
            tokType = TOKEN_PAREN_R;
            lexEnd++; colNum++;
            break;
        case '[':
            tokType = TOKEN_BRACK_L;
            lexEnd++; colNum++;
            break;
        case ']':
            tokType = TOKEN_BRACK_R;
            lexEnd++; colNum++;
            break;
        case '{':
            tokType = TOKEN_BRACE_L;
            lexEnd++; colNum++;
            break;
        case '}':
            tokType = TOKEN_BRACE_R;
            lexEnd++; colNum++;
            break;
        case '\n':
            tokType = TOKEN_NL;
            lineNum++; colNum = 0;
            lexEnd++;
            break;
        default:
            if (isAlpha(lookahead)) {
                // Possible: Keyword, Identifier
                for (size_t i = lexStart; isAlphaDigit(*(source + i)); i++) {
                    lexEnd++; colNum++;
                }
                size_t kwLen = lexEnd - lexStart;
                tokType = matchKeywordOrIdentifier(source + lexStart, kwLen);
            } else if (isDigit(lookahead)) {
                // Possible: Literal Number
                tokType = TOKEN_NUMBER;
                lexEnd = lexNumber(source, srcLen, lexStart, &errLen, errMsg);
                colNum += lexEnd - lexStart;  // lexEnd and lexStart on same line, so colNum would be still correct
                if (errLen > 0) {
                    tokType = TOKEN_UNKNOWN;
                    lexError(errMsg, lineNum, colNum, errorsPtr, errorCount);
                }
            } else if (lookahead == ' ' || lookahead == '\t' || lookahead == '\r') {
                // Whitespace, ignore 
                lexEnd++; colNum++;
            } else {
                // Unknown
                tokType = TOKEN_UNKNOWN;
                sprintf(errMsg, "Unexpected character %c", lookahead);
                lexError(errMsg, lineNum, colNum, errorsPtr, errorCount);
                lexEnd++; colNum++;
            }
        }

        if (tokType != TOKEN_UNKNOWN) {
            // INVARIANT: lexeme string is source[lexStart:lexEnd], where lexEnd is the start of the next lexeme.
            size_t lexemeLen = lexEnd - lexStart;

            // Add Token to list
            *tokenCount = *tokenCount + 1;
            *tokensPtr = realloc(*tokensPtr, sizeof(Token *) * (*tokenCount));
            (*tokensPtr)[*(tokenCount) - 1] = token_new(tokType, source + lexStart, lexemeLen, lineNum, colNum);
        }

        lexStart = lexEnd;
        free(errMsg);
    }

    // Add NL token, if it doesn't already end with one
    
    if (*tokenCount > 0 && (*tokensPtr)[*(tokenCount) - 1]->type != TOKEN_NL) {
        *tokenCount += 2;
        *tokensPtr = realloc(*tokensPtr, sizeof(Token *) * (*tokenCount));
        lineNum += 1; colNum = 0;
        (*tokensPtr)[*(tokenCount) - 2] = token_new(TOKEN_NL, "\n", 1, lineNum, colNum);
        lineNum += 1; colNum = 0;
    } else {
        *tokenCount += 1;
        *tokensPtr = realloc(*tokensPtr, sizeof(Token *) * (*tokenCount));
    }

    // Add EOF token
    (*tokensPtr)[*(tokenCount) - 1] = token_new(TOKEN_EOF, "", 0, lineNum+1, 0);

    return 1;
}
