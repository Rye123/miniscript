#include <stdio.h>
#include <stdlib.h>
#define ERR_BUF_SZ 255
#include "list.h"

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
    char errMsg[ERR_BUF_SZ];
    // Test empty list
    List *test_ls_0 = list_create(sizeof(int));
    int *res_ls_0 = NULL;
    res_ls_0 = (int*) list_to_arr(test_ls_0, (void *) res_ls_0);
    test_assert(test_ls_0->size == 0, "test_ls_0 is empty", "test_ls_0 is not empty");
    test_assert(res_ls_0 == NULL, "res_ls_0 is empty", "res_ls_0 is not empty");
    list_free(test_ls_0);
    free(res_ls_0);

    // Test add_item
    int exp_arr_1[5] = {1, 2, 3, 4, 5};
    int *res_ls_1 = malloc(5 * sizeof(int));
    List *test_ls_1 = list_create(sizeof(int));
    for (int i = 0; i < 5; i++)
        list_add_item(test_ls_1,  exp_arr_1 + i);
    res_ls_1 = (int*) list_to_arr(test_ls_1, (void *) res_ls_1);
    int matched_1 = 1;
    for (int i = 0; i < 5; i++) {
        if (res_ls_1[i] != exp_arr_1[i]) {
            sprintf(errMsg, "res_ls_1[%d] != test_ls_1[%d], expected %d, got %d", i, i, res_ls_1[i], exp_arr_1[i]);
            matched_1 = 0;
            break;
        }
    }
    test_assert(matched_1, "test_ls_1 matches exp_arr_1", errMsg);
    list_free(test_ls_1);
    free(res_ls_1);

    return 0;
}
