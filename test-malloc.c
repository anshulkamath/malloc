#include "malloc.h"
#include <unistd.h>
#include <stdio.h>

#define RUN_TEST(func) _run_test(func, #func)
#define ARR_SIZE 100

int _run_test(int (*func)(), char *fname) {
    printf("Running %s:", fname);
    int passes = func();
    
    if (passes) printf("\t[PASS]\n");

    return passes;
}

int test_is_heap_clear() {
    if (!is_heap_clear()) {
        printf("\tERROR: the heap is not free!\n");
        return 0;
    }

    return 1;
}

int test_malloc_simple() {
    char *arr, *cpy_arr;
    int i;

    if (malloc(0) != NULL) {
        printf("\tERROR: malloc does not return NULL with 0 argument.\n");
    }

    // define array in its own block
    {
        arr = malloc(ARR_SIZE);

        if (arr == NULL) {
            printf("\tERROR: failed to allocate pointer.\n");
            return 0;
        }

        for (i = 0; i < ARR_SIZE; i++)
            arr[i] = 'a' + i % 26;
    }

    for (i = 0; i < ARR_SIZE; i++) {
        if (arr[i] == 'a' + i % 26) continue;

        printf(
            "\tERROR: Expected arr[%d] to be %c, but got %c.\n",
            i, 'a' + i % 26, arr[i]
        );

        free(arr);
        return 0;
    }

    free(arr);

    {
        cpy_arr = malloc(ARR_SIZE);

        if (cpy_arr == NULL) {
            printf("\tERROR: failed to allocate pointer.\n");
            return 0;
        }
    }

    if (cpy_arr != arr) {
        printf("\tERROR: did not allocate copy array properly.\n");
        return 0;
    }

    free(cpy_arr);

    if (!is_heap_clear()) {
        printf("\tERROR: mem leak - heap is not clear.\n");
        return 0;
    }

    return 1;
}

int test_malloc_complex() {
    char *arr1, *arr2, *arr3, *arr4;

    {
        arr1 = malloc(2);
        arr2 = malloc(1);
        free(arr1);
        
        arr3 = malloc(2);
        arr4 = malloc(3);
    }

    // arr3 should be allocated where arr1 was
    if (!(arr3 < arr2 && arr3 == arr1)) {
        printf("\tError on re-using memory.\n");
        return 0;
    }

    // arr3 should be allocated where arr1 was
    if (!(arr4 > arr3)) {
        printf("\tError on allocating in succession.\n");
        return 0;
    }

    free(arr2);
    free(arr3);
    free(arr4);

    if (!is_heap_clear()) {
        printf("\tERROR: mem leak - heap is not clear.\n");
        return 0;
    }

    return 1;
}

int test_malloc_complex2() {
    char *arr1, *arr2, *arr3, *arr4, *arr5;

    {
        arr1 = malloc(2);
        arr2 = malloc(4 + 4 + 24); // + 4 for 8-byte alignment, + 24 to account for header
        arr3 = malloc(2);
        free(arr2);
        arr4 = malloc(2);
        arr5 = malloc(2);
    }

    // arr4 should be where the first half of arr2 was
    // arr5 should be where the second half of arr2 was
    if (!(arr4 == arr2 && arr5 < arr3)) {
        printf("\tError on re-using memory.\n");
        return 0;
    }

    free(arr1);
    free(arr3);
    free(arr4);
    free(arr5);

    if (!is_heap_clear()) {
        printf("\tERROR: mem leak - heap is not clear.\n");
        return 0;
    }

    return 1;
}

int test_calloc() {
    char *arr;
    int i;

    {
        arr = calloc(ARR_SIZE, sizeof(char));
    }

    for (i = 0; i < ARR_SIZE; i++) {
        if (arr[i] != 0) {
            printf("\tERROR: calloc did not initialize to 0.\n");
            return 0;
        }
    }

    return 1;
}

int test_realloc() {
    char *arr;
    int i;

    {
        arr = malloc(ARR_SIZE);
    }

    for (i = 0; i < ARR_SIZE; i++)
        arr[i] = 'a' + i % 26;

    {
        arr = realloc(arr, ARR_SIZE * 2);
    }

    for (i = 0; i < ARR_SIZE; i++) {
        if (arr[i] != 'a' + i % 26) {
            printf(
                "\tERROR: realloc did not copy arr properly;"
                "arr[%d] = %c, but should be %c.\n",
                i, 'a' + i % 26, arr[i]
            );
            return 0;
        }
    }

    return 1;
}

int main() {
    int pass = 1;

    pass &= RUN_TEST(test_is_heap_clear);
    pass &= RUN_TEST(test_malloc_simple);
    pass &= RUN_TEST(test_malloc_complex);
    pass &= RUN_TEST(test_malloc_complex2);
    pass &= RUN_TEST(test_calloc);
    pass &= RUN_TEST(test_realloc);

    if (pass) printf("Passed all tests!\n");
    else printf("Did not pass all tests :(\n");

    return 0;
}
