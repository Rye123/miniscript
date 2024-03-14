all: list.o
test: test_list

list.o: list.c
	$(CC) list.c -c list.o

test_list: list.c list_test.c
	$(CC) list_test.c list.c -o list_test
	./list_test
	$(RM) list_test

clean:
	$(RM) -f list.o list_test
