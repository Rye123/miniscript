CFLAGS = -ggdb
OBJS = main.o interpreter.o parser/parser.o parser/symbol.o lexer/lexer.o lexer/token.o

all: main

%.o: %.c
	$(CC) $(CFLAGS) $^ -c -o $@

lexer/%.o: lexer/%.c
	$(CC) $(CFLAGS) $^ -c -o $@

parser/%.o: parser/%.c
	$(CC) $(CFLAGS) $^ -c -o $@

# Linking
main: $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $@

clean:
	$(RM) -f *.o lexer/*.o parser/*.o main
