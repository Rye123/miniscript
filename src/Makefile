CC = gcc
CFLAGS = -g
LFLAGS = -lm
OBJS = interpreter.o executor/executor.o executor/symboltable.o parser/parser.o parser/symbol.o lexer/lexer.o lexer/token.o

all: main

%.o: %.c
	$(CC) $(CFLAGS) $^ -c -o $@

lexer/%.o: lexer/%.c
	$(CC) $(CFLAGS) $^ -c -o $@

parser/%.o: parser/%.c
	$(CC) $(CFLAGS) $^ -c -o $@

executor/%.o: executor/%.c
	$(CC) $(CFLAGS) $^ -c -o $@

tests/%.o: tests/%.c
	$(CC) $(CFLAGS) $^ -c -o $@

# Linking
.PHONY: main
main: main.o $(OBJS)
	$(CC) -o miniscript $(CFLAGS) main.o $(OBJS) $(LFLAGS)

.PHONY: test
test: tests/lex_test.o $(OBJS)
	$(CC) $(CFLAGS) tests/lex_test.o $(OBJS) -o tests/lex_test
	./tests/lex_test
	$(RM) -f ./tests/lex_test

.PHONY: clean
clean:
	$(RM) -f *.o executor/*.o lexer/*.o parser/*.o main
