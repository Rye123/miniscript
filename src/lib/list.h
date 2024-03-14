#ifndef _LIST_H_
#define _LIST_H_
#include <stdio.h>

typedef struct _node {
    void *data;
    struct _node *prev;
    struct _node *next;
} ListNode;

typedef struct {
    size_t elemSz;
    size_t size;
    ListNode *head;
    ListNode *tail;
} List;

List* list_create(size_t elemSz);

/*
Adds an item to the list.
- `data` should be a pointer to the item of size `List.elemSz`. This function will make a COPY of the data and create a new `ListNode` that points to the copied data.
*/
void list_add_item(List *ls, const void *data);

/*
Returns an array of pointers to the values of the list. This takes a PRE-ALLOCATED array and overwrites the values inside with the array of pointers.

Remember to typecast the returned array.. 
For instance, if we have a list of integers `List *ls`:
```c
int *arr = malloc(ls->size * sizeof(int));
arr = (int *) list_to_arr(ls, (void *) arr);
for (int i = 0; i < ls->size; i++) {
    printf("%d\n", arr[i]);
}
```
*/
void* list_to_arr(List *ls, void *arr);

void list_free(List *ls);

#endif
