#include "interpreter.h"

void initFSM(FSM *fsm) {
    fsm->current_state = INIT;
}

void transition(FSM *fsm, int success) {
    if (success) {
        switch (fsm->current_state) {
            case INIT:
                fsm->current_state = LEXING;
                break;
            case LEXING:
                fsm->current_state = PARSING;
                break;
            case PARSING:
                fsm->current_state = EXECUTING;
                break;
            case EXECUTING:
                fsm->current_state = CLEANING;
                break;
            default:
                fsm->current_state = CLEANING;
                break;
        }
    } else {
        switch (fsm->current_state) {
            case LEXING:
                fsm->current_state = LEXING_ERROR;
                break;
            case LEXING_ERROR:
                fsm->current_state = CLEANING;
                break;
            case PARSING:
                fsm->current_state = PARSING_ERROR;
                break;
            case PARSING_ERROR:
                fsm->current_state = CLEANING;
                break;
            case EXECUTING:
                fsm->current_state = EXECUTING_ERROR;
                break;
            case EXECUTING_ERROR:
                fsm->current_state = CLEANING;
                break;
            default:
                fsm->current_state = CLEANING;
                break;
        }
    }
}

void reportError(const char *msg) {
    log_message(&consoleLogger, "\033[91m%s\033[0m\n", msg);
    log_message(&executionLogger, "%s\n", msg);
}

/* Returns true if expecting more input */
int runLine(const char *source, Context *executionContext, int asREPL) {
    int success;
    FSM fsm;
    size_t tokenCount;
    size_t errorCount;
    size_t i;
    Token **tokens;
    Error **errors;
    char errStr[MAX_ERRSTR_LEN];
    ASTNode *root;
    LexResult lexResult;
    ExecValue *val;
    Error *parseError;

    initFSM(&fsm);
    while (fsm.current_state != CLEANING) {
        switch (fsm.current_state) {
            case INIT:
                /* 0. Initialisation */
                success = 1;
                tokenCount = 0;
                errorCount = 0;
                tokens = malloc(sizeof(Token *) * 0);
                errors = malloc(sizeof(Error *) * 0);
                root = astnode_new(SYM_START, NULL);
                initLexResult(&lexResult);

                initErrorContext(source);
                log_message(&executionLogger, "Input:\n%s\n", source);

                transition(&fsm, success);
                break;
            case LEXING:
                lex((const Token ***) &tokens, &tokenCount, source, &lexResult);

                log_message(&executionLogger, "--- LEXING RESULT ---\n");
                log_message(&executionLogger, "Token Count: %lu\n", tokenCount);
                for (i = 0; i < tokenCount; i++)
                    token_print(tokens[i]);

                transition(&fsm, !lexResult.hasError);
                break;
            case LEXING_ERROR:
                if (lexResult.hasError) {
                    lexError(lexResult.errorMessage, lexResult.lineNum, lexResult.colNum, (const Error ***) &errors,
                             &errorCount);
                }

                if (errorCount != 0) {
                    char errStr[MAX_ERRSTR_LEN];
                    for (i = 0; i < errorCount; i++) {
                        error_string(errors[i], errStr, MAX_ERRSTR_LEN);
                        reportError(errStr);
                        error_free(errors[i]);
                    }
                    free(errors);
                }

                transition(&fsm, success);
                break;
            case PARSING:
                parseError = parse(root, tokens, tokenCount);
                log_message(&executionLogger, "\n--- PARSE TREE ---\n");
                astnode_print(root);
                log_message(&executionLogger, "\n");

                if (parseError != NULL) {
                    if (parseError->type == ERR_SYNTAX_EOF && asREPL) {
                        /* Only ask for more input if this is in REPL mode. */
                        error_free(parseError);
                        astnode_free(root);
                        return 1;
                    }

                    transition(&fsm, !success);
                    break;
                }

                log_message(&executionLogger, "\n--- AST ---\n");
                astnode_gen(root);
                astnode_print(root);
                log_message(&executionLogger, "\n");
                transition(&fsm, success);
                break;
            case PARSING_ERROR:
                error_string(parseError, errStr, MAX_ERRSTR_LEN);
                reportError(errStr);

                transition(&fsm, success);
                break;
            case EXECUTING:
                log_message(&executionLogger, "\n--- EXECUTION RESULT ---\n");
                val = execStart(executionContext, root);

                if (val->type == TYPE_ERROR) {
                    transition(&fsm, !success);
                    break;
                }
                value_free(val);

                transition(&fsm, success);
                break;
            case EXECUTING_ERROR:
                error_string(val->value.error_ptr, errStr, MAX_ERRSTR_LEN);
                reportError(errStr);
                value_free(val);

                transition(&fsm, success);
                break;
            default:
                break;
        }
    }

    if (fsm.current_state == CLEANING) {
        /* 4. Clean up */
        astnode_free(root);
        for (i = 0; i < tokenCount; i++)
            token_free(tokens[i]);
        free(tokens);
        free(errorContext);
        errorContext = NULL;
    }
    return 0;
}

void runFile(const char *fname) {
    Context *globalCtx;
    FILE *srcFile = fopen(fname, "r");
    long fileSz;
    char *source;
    size_t totalSz;
    if (srcFile == NULL) {
        fprintf(stderr, "Error opening %s: %s\n", fname, strerror(errno));
        exit(errno);
    }

    /* Get size of file */
    fseek(srcFile, 0, SEEK_END);
    fileSz = ftell(srcFile);
    fseek(srcFile, 0, SEEK_SET);

    /* Allocate space for file's contents */
    source = malloc((fileSz * sizeof(char)) + 1);
    if (source == NULL) {
        fprintf(stderr, "Error allocating memory for file %s: %s\n", fname, strerror(errno));
        exit(errno);
    }

    /* Fill buffer with contents of file */
    totalSz = fread(source, sizeof(char), fileSz, srcFile);
    source[totalSz] = '\0';
    fclose(srcFile);

    /* Run the entire file. */
    globalCtx = context_new(NULL, NULL);
    runLine(source, globalCtx, 0);
}

void runREPL(void) {
    char source[REPL_BUF_MAX];
    Context *globalCtx = context_new(NULL, NULL);
    size_t sourceSz = 0;
    size_t bufSz;
    char buffer[LINE_MAX];
    memset(source, 0, REPL_BUF_MAX);

    log_message(&consoleLogger, "Miniscript 0.1\n");
    while (1) {
        if (strcmp(buffer, "exit\n") == 0) {
            break;
        }

        if (sourceSz > 0)
            log_message(&consoleLogger, ".. "); /* Prompt for more input */
        else
            log_message(&consoleLogger, ">> "); /* Standard prompt */

        if (fgets(buffer, LINE_MAX, stdin) == NULL)
            break;

        /* Store existing buffer data into source */
        bufSz = strnlen(buffer, LINE_MAX);
        if (bufSz > (REPL_BUF_MAX - sourceSz))
            criticalError("Too much input in REPL buffer, increase REPL_BUF_MAX.");

        strncpy(source + sourceSz, buffer, bufSz);
        sourceSz += bufSz;

        if (!runLine(source, globalCtx, 1)) {
            /* Not expecting any more input */
            sourceSz = 0;
            memset(source, 0, REPL_BUF_MAX);
            log_message(&executionLogger, "\n\n");
        }
    }
}
