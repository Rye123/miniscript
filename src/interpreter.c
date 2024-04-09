#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include "lexer/token.h"
#include "lexer/lexer.h"
#include "parser/parser.h"
#include "executor/executor.h"
#include "executor/symboltable.h"
#define LINE_MAX 255

void runLine(const char* source, Context *executionContext)
{
    printf("ENTERED: %s\n", source);
    // Lexical Analysis
    // 1. Initialisation
    size_t tokenCount = 0;
    Token** tokens = malloc(sizeof(Token *) * 0);
    // 2. Lexing
    lex((const Token***) &tokens, &tokenCount, source);
    //tokenCount++;
    //tokens = realloc(tokens, tokenCount * sizeof(Token *));
    //tokens[tokenCount-1] = token_new(TOKEN_NL, "\n", 2, tokens[tokenCount-2]->lineNum+1, 0);

    printf("--- LEXING RESULT ---\n");
    printf("Token Count: %lu\n", tokenCount);
    for (size_t i = 0; i < tokenCount; i++)
	token_print(tokens[i]);
    
    // 3. Syntactic Analysis
    ASTNode *root = astnode_new(SYM_START, NULL);
    parse(root, tokens, tokenCount);
    printf("\n--- PARSING RESULT ---\n");
    astnode_print(root);
    printf("\n");

    //printf("\n--- AST RESULT ---\n");
    //astnode_gen(root);
    //astnode_print(root);
    //printf("\n");

    // Execution
    printf("\n--- EXECUTION RESULT ---\n");
    ExecValue *val = execStart(executionContext, root);
    printf("\nExit Code: %f\n", val->value.literal_num);
    value_free(val);

    // Cleanup
    /* astnode_free(root); */
    for (size_t i = 0; i < tokenCount; i++)
	token_free(tokens[i]);
    free(tokens);
}

void runFile(const char* fname)
{
    FILE *srcFile = fopen(fname, "r");
    long fileSz = 0;
    char *source;
    if (srcFile == NULL) {
        fprintf(stderr, "Error opening %s: %s\n", fname, strerror(errno));
        exit(errno);
    }

    // Get size of file
    fseek(srcFile, 0, SEEK_END);
    fileSz = ftell(srcFile);
    fseek(srcFile, 0, SEEK_SET);

    // Allocate space for file's contents
    source = malloc((fileSz * sizeof(char)) + 1);
    if (source == NULL) {
        fprintf(stderr, "Error allocating memory for file %s: %s\n", fname, strerror(errno));
        exit(errno);
    }

    // Fill buffer with contents of file
    size_t totalSz = fread(source, sizeof(char), fileSz, srcFile);
    source[totalSz] = '\0';
    fclose(srcFile);

    // Run the entire file.
    Context *globalCtx = context_new(NULL, NULL);
    runLine(source, globalCtx);
}

void runREPL()
{
    char buffer[LINE_MAX];
    Context *globalCtx = context_new(NULL, NULL);
    printf("Miniscript 0.1\n");
    while (1) {
        printf(">> ");
        if (fgets(buffer, LINE_MAX, stdin) == NULL)
            break;
        runLine(buffer, globalCtx);
    }
}


