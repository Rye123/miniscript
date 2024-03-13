#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#define LINE_MAX 255

void runLine(const char* source)
{
    printf("ENTERED: %s\n", source);
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

    // Run each line individually
    char *line = strtok(source, "\n");
    while (line != NULL) {
        runLine(line);
        line = strtok(NULL, "\n");
    }
}

void runREPL()
{
    char buffer[LINE_MAX];
    printf("Miniscript 0.1\n");
    while (1) {
        printf(">> ");
        if (fgets(buffer, LINE_MAX, stdin) == NULL)
            break;
        runLine(buffer);
    }
}

int main(int argc, char** argv)
{
    if (argc == 1)
        runREPL();
    else if (argc == 2)
        runFile(argv[1]);
    else {
        printf("Usage: ./interpreter [file]\n");
        return 1;
    }
    return 0;
}