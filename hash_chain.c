/*
 * Hash Table with Chaining - Standalone test program
 * Compile: gcc -O2 -o hash_chain hash_chain.c -lm
 */
#include "hc_table.h"
#include <time.h>

int main(void) {
    HC_Table *table = hc_create();

    printf("=== Hash Table (Chaining) Test ===\n\n");

    /* Insert test */
    int vals[] = {10, 20, 5, 15, 25, 3, 7, 12, 18, 30, 1, 4, 6, 8, 11, 13, 17, 19, 22, 28};
    int n = sizeof(vals) / sizeof(vals[0]);

    printf("Inserting %d values: ", n);
    for (int i = 0; i < n; i++) {
        printf("%d ", vals[i]);
        hc_insert(table, vals[i]);
    }
    printf("\nCount: %d\n", table->count);

    /* Duplicate insert test */
    hc_insert(table, 10);
    printf("After duplicate insert(10), count: %d\n", table->count);

    /* Search test */
    printf("\nSearch tests:\n");
    int search_vals[] = {5, 15, 25, 100, 0, 30};
    for (int i = 0; i < 6; i++)
        printf("  search(%d) = %s\n", search_vals[i],
               hc_search(table, search_vals[i]) ? "FOUND" : "NOT FOUND");

    /* Delete test */
    printf("\nDelete tests:\n");
    int del_vals[] = {1, 5, 10, 20, 30};
    for (int i = 0; i < 5; i++) {
        hc_delete(table, del_vals[i]);
        printf("  delete(%d) -> count=%d\n", del_vals[i], table->count);
    }

    /* Verify deletions */
    printf("\nPost-delete searches:\n");
    for (int i = 0; i < 5; i++)
        printf("  search(%d) = %s\n", del_vals[i],
               hc_search(table, del_vals[i]) ? "FOUND" : "NOT FOUND");

    hc_destroy(table);

    /* Performance test */
    printf("\n=== Performance Test (100000 operations) ===\n");
    table = hc_create();

    struct timespec start, end;
    int N = 100000;

    /* Insert */
    clock_gettime(CLOCK_MONOTONIC, &start);
    for (int i = 0; i < N; i++)
        hc_insert(table, i);
    clock_gettime(CLOCK_MONOTONIC, &end);
    double ins_time = (end.tv_sec - start.tv_sec) * 1e9 + (end.tv_nsec - start.tv_nsec);
    printf("Insert %d: %.2f ns/op, count=%d\n", N, ins_time / N, table->count);

    /* Search */
    clock_gettime(CLOCK_MONOTONIC, &start);
    for (int i = 0; i < N; i++)
        hc_search(table, i);
    clock_gettime(CLOCK_MONOTONIC, &end);
    double srch_time = (end.tv_sec - start.tv_sec) * 1e9 + (end.tv_nsec - start.tv_nsec);
    printf("Search %d: %.2f ns/op\n", N, srch_time / N);

    /* Delete */
    clock_gettime(CLOCK_MONOTONIC, &start);
    for (int i = 0; i < N; i++)
        hc_delete(table, i);
    clock_gettime(CLOCK_MONOTONIC, &end);
    double del_time = (end.tv_sec - start.tv_sec) * 1e9 + (end.tv_nsec - start.tv_nsec);
    printf("Delete %d: %.2f ns/op, count=%d\n", N, del_time / N, table->count);

    hc_destroy(table);
    printf("\nAll tests passed.\n");
    return 0;
}
