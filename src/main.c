#include <stdio.h>
#include "interpreter.h"

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