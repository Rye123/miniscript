
lex_test: lex_test.c ../parser/parser.o ../lexer/lexer.o ../lib/list.o ../lexer/token.o
	$(CC) lex_test.c ../parser/parser.o ../lexer/lexer.o ../lib/list.o ../lexer/token.o -o lex_test -g
	./lex_test

clean:
	$(RM) -f *.o lex_test