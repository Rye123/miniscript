#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define ERR_BUF_SZ 255
#include "../lexer/lexer.h"
#include "../lexer/token.h"
#include "../lib/list.h"

void compare_lists(List* actual, List* expected);

void test_assert(int cond_result, const char* resultPass, const char* resultFail)
{
    if (cond_result)
        printf("\033[92mPASS: %s\033[0m\n", resultPass);
    else {
        printf("\033[91mFAIL: %s\033[0m\n", resultFail);
        exit(1);
    }
}

int main()
{
    // CASE 1:
    List* tokenLs = list_create(sizeof(Token));
    lex("var 2", tokenLs);

    List* expectedLs = list_create(sizeof(Token));
    list_add_item(expectedLs, token_new(TOKEN_IDENTIFIER, "var", 3, 0, 3));
    list_add_item(expectedLs, token_new(TOKEN_NUMBER, "2", 1, 0, 5)); 
    compare_lists(tokenLs, expectedLs);

    // CASE 2:
    tokenLs = list_create(sizeof(Token));
    lex("a= 2.344+\n5.77n\t\"hello\"", tokenLs);

    expectedLs = list_create(sizeof(Token));
    list_add_item(expectedLs, token_new(TOKEN_IDENTIFIER, "a", 1, 0, 1));
    list_add_item(expectedLs, token_new(TOKEN_EQUAL, "=", 1, 0, 2)); 
    list_add_item(expectedLs, token_new(TOKEN_NUMBER, "2.344", 5, 0, 8)); 
    list_add_item(expectedLs, token_new(TOKEN_PLUS, "+", 1, 0, 9)); 
    list_add_item(expectedLs, token_new(TOKEN_NUMBER, "5.77", 4, 1, 14)); 
    list_add_item(expectedLs, token_new(TOKEN_IDENTIFIER, "n", 1, 1, 15)); 
    list_add_item(expectedLs, token_new(TOKEN_STRING, "\"hello\"", 7, 1, 23)); 

    compare_lists(tokenLs, expectedLs);

    // CASE 3:
    tokenLs = list_create(sizeof(Token));
    lex("22a\"b\"\n\n\t5.5.5.5\n", tokenLs);

    expectedLs = list_create(sizeof(Token));
    list_add_item(expectedLs, token_new(TOKEN_NUMBER, "22", 2, 0, 2));
    list_add_item(expectedLs, token_new(TOKEN_IDENTIFIER, "a", 1, 0, 3)); 
    list_add_item(expectedLs, token_new(TOKEN_STRING, "\"b\"", 3, 0, 6)); 
    list_add_item(expectedLs, token_new(TOKEN_NUMBER, "5.5", 3, 0, 12)); 
    list_add_item(expectedLs, token_new(TOKEN_NUMBER, ".5", 2, 2, 14)); 
    list_add_item(expectedLs, token_new(TOKEN_NUMBER, ".5", 2, 2, 16)); 

    compare_lists(tokenLs, expectedLs);

    return 0;
}

void compare_lists(List* actual, List* expected){
    char passMsg[ERR_BUF_SZ];
    char errMsg[ERR_BUF_SZ];
    char* tokenString = (char*) malloc(sizeof(char)*50);
    char* tokenString2 = (char*) malloc(sizeof(char)*50);


    test_assert(actual->size == expected->size, "Size of token_ls is expected", "Size of token_ls is unexpected");

    size_t s = actual->size;
    ListNode* actual_node = actual->head;
    ListNode* expected_node = expected->head;
    for (int i = 0; (size_t) i < s; i++){
        printf("%d\n", i);
        token_string(tokenString, expected_node->data);
        token_string(tokenString2, actual_node->data);
        sprintf(passMsg, "Token in index %d is correct", i);
        sprintf(errMsg, "Expected:%p\n Actual:%p", tokenString, tokenString2);
        test_assert(token_compare(actual_node->data, expected_node->data), passMsg, errMsg);
        actual_node = actual_node->next;
        expected_node = expected_node->next;
    }
    free(tokenString);
    free(tokenString2);
}