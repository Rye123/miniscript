#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "list.h"

List *list_create(size_t elemSz)
{
    List *ls = malloc(sizeof(List));
    ls->elemSz = elemSz;
    ls->size = 0;
    ls->head = NULL;
    ls->tail = NULL;
    return ls;
}

void list_add_item(List *ls, const void *data)
{
    ListNode *node = malloc(sizeof(ListNode));
    node->data = malloc(ls->elemSz);
    memcpy(node->data, data, ls->elemSz);
    node->prev = NULL;
    node->next = NULL;
    
    if (ls->tail == NULL) {
        // Create single node that is both head and tail
        ls->head = node;
        ls->tail = node;
    } else {
        // Add new node at the end
        node->prev = ls->tail;
        ls->tail->next = node;
        ls->tail = node;
    }
    ls->size++;
}

void *list_to_arr(List *ls, void *arr)
{
    ListNode *node = ls->head;
    size_t cur_i = 0;
    while (node != NULL) {
        memcpy(arr + (cur_i * ls->elemSz), node->data, ls->elemSz);
        node = node->next;
        cur_i += 1;
    }
    return arr;
}

void list_free(List *ls)
{
    // Iterate through list
    ListNode* node = ls->head;
    ListNode* nextNode = NULL;
    while (node != NULL) {
        // Free each node's data, then the node itself.
        free(node->data);
        nextNode = node->next;
        free(node);
        node = nextNode;
    }
    free(ls);
}
