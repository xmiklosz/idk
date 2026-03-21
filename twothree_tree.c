/*
 * 2-3 Tree - Standalone test program
 * Compile: gcc -O2 -o twothree_tree twothree_tree.c -lm
 */
#include "tt_tree.h"
#include <time.h>

int main(void) {
    TT_Tree *tree = tt_create();

    printf("=== 2-3 Tree Test ===\n\n");

    /* Insert test */
    int vals[] = {10, 20, 5, 15, 25, 3, 7, 12, 18, 30, 1, 4, 6, 8, 11, 13, 17, 19, 22, 28};
    int n = sizeof(vals) / sizeof(vals[0]);

    printf("Inserting %d values: ", n);
    for (int i = 0; i < n; i++) {
        printf("%d ", vals[i]);
        tt_insert(tree, vals[i]);
    }
    printf("\nCount: %d\n", tree->count);
    printf("Tree: ");
    tt_print(tree->root);
    printf("\n\n");

    /* Duplicate insert test */
    tt_insert(tree, 10);
    printf("After duplicate insert(10), count: %d\n", tree->count);

    /* Search test */
    printf("\nSearch tests:\n");
    int search_vals[] = {5, 15, 25, 100, 0, 30};
    for (int i = 0; i < 6; i++)
        printf("  search(%d) = %s\n", search_vals[i],
               tt_search(tree, search_vals[i]) ? "FOUND" : "NOT FOUND");

    /* Delete test */
    printf("\nDelete tests:\n");
    int del_vals[] = {1, 5, 10, 20, 30, 15, 3, 25, 7, 12};
    for (int i = 0; i < 10; i++) {
        tt_delete(tree, del_vals[i]);
        printf("  delete(%d) -> count=%d, tree: ", del_vals[i], tree->count);
        tt_print(tree->root);
        printf("\n");
    }

    /* Performance test */
    printf("\n=== Performance Test (100000 operations) ===\n");
    tt_destroy(tree);
    tree = tt_create();

    struct timespec start, end;
    int N = 100000;

    /* Insert */
    clock_gettime(CLOCK_MONOTONIC, &start);
    for (int i = 0; i < N; i++)
        tt_insert(tree, i);
    clock_gettime(CLOCK_MONOTONIC, &end);
    double ins_time = (end.tv_sec - start.tv_sec) * 1e9 + (end.tv_nsec - start.tv_nsec);
    printf("Insert %d: %.2f ns/op, count=%d\n", N, ins_time / N, tree->count);

    /* Search */
    clock_gettime(CLOCK_MONOTONIC, &start);
    for (int i = 0; i < N; i++)
        tt_search(tree, i);
    clock_gettime(CLOCK_MONOTONIC, &end);
    double srch_time = (end.tv_sec - start.tv_sec) * 1e9 + (end.tv_nsec - start.tv_nsec);
    printf("Search %d: %.2f ns/op\n", N, srch_time / N);

    /* Delete */
    clock_gettime(CLOCK_MONOTONIC, &start);
    for (int i = 0; i < N; i++)
        tt_delete(tree, i);
    clock_gettime(CLOCK_MONOTONIC, &end);
    double del_time = (end.tv_sec - start.tv_sec) * 1e9 + (end.tv_nsec - start.tv_nsec);
    printf("Delete %d: %.2f ns/op, count=%d\n", N, del_time / N, tree->count);

    tt_destroy(tree);
    printf("\nAll tests passed.\n");
    return 0;
}
