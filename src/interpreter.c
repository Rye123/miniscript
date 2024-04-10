#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include "logger/logger.h"
#include "error/error.h"
#include "lexer/token.h"
#include "lexer/lexer.h"
#include "parser/parser.h"
#include "executor/executor.h"
#include "executor/symboltable.h"
#define REPL_BUF_MAX 2000
#define LINE_MAX 1000

void reportError(const char *msg)
{
    log_message(&consoleLogger, "\033[91m%s\033[0m\n", msg);
    log_message(&executionLogger, "%s\n", msg);
}

// Returns true if expecting more input
int runLine(const char *source, Context *executionContext)
{
    //executionLogger = consoleLogger;
    initErrorContext(source);
    log_message(&executionLogger, "Input:\n%s\n", source);

    // Lexical Analysis
    // 1. Initialisation
    size_t tokenCount = 0;
    size_t errorCount = 0;
    Token **tokens = malloc(sizeof(Token *) * 0);
    Error **errors = malloc(sizeof(Error *) * 0);
    // 2. Lexing
    lex((const Token ***) &tokens, &tokenCount,
        (const Error ***) &errors, &errorCount, source);

    log_message(&executionLogger, "--- LEXING RESULT ---\n");
    log_message(&executionLogger, "Token Count: %lu\n", tokenCount);
    for (size_t i = 0; i < tokenCount; i++)
        token_print(tokens[i]);

    if (errorCount != 0) {
        char errStr[MAX_ERRSTR_LEN];
        for (size_t i = 0; i < errorCount; i++) {
            error_string(errors[i], errStr, MAX_ERRSTR_LEN);
            reportError(errStr);
            error_free(errors[i]);
        }
        free(errors);
        for (size_t i = 0; i < tokenCount; i++)
            token_free(tokens[i]);
        free(tokens);
        return 0;
    }
    
    // 3. Syntactic Analysis
    ASTNode *root = astnode_new(SYM_START, NULL);
    Error *parseError = parse(root, tokens, tokenCount);
    log_message(&executionLogger, "\n--- PARSE TREE ---\n");
    astnode_print(root);
    log_message(&executionLogger, "\n");

    if (parseError != NULL) {
        if (parseError->type == ERR_SYNTAX_EOF) {
            error_free(parseError);
            astnode_free(root);
            return 1;
        }
        char errStr[MAX_ERRSTR_LEN];
        error_string(parseError, errStr, MAX_ERRSTR_LEN);
        reportError(errStr);
        error_free(parseError);
        astnode_free(root);
        return 0;
    }

    log_message(&executionLogger, "\n--- AST ---\n");
    astnode_gen(root);
    astnode_print(root);
    log_message(&executionLogger, "\n");

    // Execution
    log_message(&executionLogger, "\n--- EXECUTION RESULT ---\n");
    ExecValue *val = execStart(executionContext, root);
    if (val->type == TYPE_ERROR) {
        char errStr[MAX_ERRSTR_LEN];
        error_string(val->value.error_ptr, errStr, MAX_ERRSTR_LEN);
        reportError(errStr);
    }
    value_free(val);

    // Cleanup
    astnode_free(root);
    for (size_t i = 0; i < tokenCount; i++)
        token_free(tokens[i]);
    free(tokens);
    free(errorContext);
    errorContext = NULL;
    return 0;
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
    Context *globalCtx = context_new(NULL, NULL);
    log_message(&consoleLogger, "Miniscript 0.1\n");

    char source[REPL_BUF_MAX];
    size_t sourceSz = 0;
    char buffer[LINE_MAX];
    memset(source, 0, REPL_BUF_MAX);
    while (1) {
        if (sourceSz > 0)
            log_message(&consoleLogger, "..     "); // Prompt for more input
        else
            log_message(&consoleLogger, ">> "); // Standard prompt

        if (fgets(buffer, LINE_MAX, stdin) == NULL)
            break;

        // Store existing buffer data into source
        size_t bufSz = strnlen(buffer, LINE_MAX);
        if (bufSz > (REPL_BUF_MAX - sourceSz))
            criticalError("Too much input in REPL buffer, increasing REPL_BUF_MAX.");
        
        strncpy(source + sourceSz, buffer, bufSz);
        sourceSz += bufSz;

        if (!runLine(source, globalCtx)) {
            // Not expecting any more input
            sourceSz = 0;
            memset(source, 0, REPL_BUF_MAX);
            log_message(&executionLogger, "\n\n");
        }
    }
}
