SUBDIRS = lib lexer parser
INTERPRETER_OUT=interpreter
INTERPRETER_DEP=interpreter.o parser/parser.o lexer/lexer.o lib/list.o lexer/token.o

.PHONY: subdirs $(SUBDIRS)

all: interpreter
subdirs: $(SUBDIRS)

$(SUBDIRS):
	$(MAKE) -C $@

parser: lib
lexer: lib

main: main.o interpreter.o parser/parser.o lexer/lexer.o lib/list.o lexer/token.o
	$(CC) -o main main.o $(INTERPRETER_DEP)

main.o: main.c
	$(CC) main.c -c -o main.o

interpreter: $(INTERPRETER_DEP)
	$(CC) $(INTERPRETER_DEP) -o $(INTERPRETER_OUT)

interpreter.o: interpreter.c
	$(CC) interpreter.c -c -o interpreter.o

clean:
	$(RM) -f *.o $(INTERPRETER_OUT)
