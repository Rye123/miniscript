INTERPRETER_OUT=interpreter
INTERPRETER_DEP=interpreter.o

interpreter: $(INTERPRETER_DEP)
	$(CC) interpreter.o -o $(INTERPRETER_OUT)

interpreter.o: interpreter.c
	$(CC) interpreter.c -c -o interpreter.o

clean:
	$(RM) -f *.o $(INTERPRETER_OUT)
