
parser: parser.c symbol.c ../lexer/token.c
	$(CC) $^ -o parser -ggdb

symbol.o: symbol.c
	$(CC) symbol.c -c symbol.o

parser.o: parser.c
	$(CC) parser.c -c parser.o
