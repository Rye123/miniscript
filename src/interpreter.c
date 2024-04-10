#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include "logger/logger.h"
#include "lexer/token.h"
#include "lexer/lexer.h"
#include "parser/parser.h"
#include "executor/executor.h"
#include "executor/symboltable.h"
#define LINE_MAX 255

void runLine(const char *source, Context *executionContext)
{
    log_message(&executionLogger, "Input:\n%s\n", source);

    // Lexical Analysis
    // 1. Initialisation
    size_t tokenCount = 0;
    Token **tokens = malloc(sizeof(Token *) * 0);
    // 2. Lexing
    lex((const Token ***) &tokens, &tokenCount, source);

    log_message(&executionLogger, "--- LEXING RESULT ---\n");
    log_message(&executionLogger, "Token Count: %lu\n", tokenCount);
    for (size_t i = 0; i < tokenCount; i++)
        token_print(tokens[i]);
    
    // 3. Syntactic Analysis
    ASTNode *root = astnode_new(SYM_START, NULL);
    parse(root, tokens, tokenCount);
    log_message(&executionLogger, "\n--- PARSE TREE ---\n");
    astnode_print(root);
    log_message(&executionLogger, "\n");

    log_message(&executionLogger, "\n--- AST ---\n");
    astnode_gen(root);
    astnode_print(root);
    log_message(&executionLogger, "\n");

    // Execution
    log_message(&executionLogger, "\n--- EXECUTION RESULT ---\n");
    ExecValue *val = execStart(executionContext, root);
    log_message(&executionLogger, "\nExit Code: %f\n", val->value.literal_num);
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
    log_message(&consoleLogger, "Miniscript 0.1\n");
    while (1) {
        log_message(&consoleLogger, ">> ");
        if (fgets(buffer, LINE_MAX, stdin) == NULL)
            break;

        if (strcmp(buffer, "exit\n") == 0) {
            break;
        }

        runLine(buffer, globalCtx);
        log_message(&executionLogger, "\n\n");
    }
}
