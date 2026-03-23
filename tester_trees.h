/*
 * Tree Tester - Comparison of 2-3 Tree vs Red-Black Tree
 * Benchmarks insert, search, delete across multiple scenarios and sizes.
 * Outputs results to console and tree_results.csv.
 *
 * Compile: gcc -O2 -o tester_trees -x c tester_trees.h -lm
 * Run:     ./tester_trees
 */
#ifndef TESTER_TREES_H
#define TESTER_TREES_H

#include "tt_tree.c"
#include "rb_tree.c"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* ========== Timing ========== */

static double elapsed_ns(struct timespec *t_start, struct timespec *t_end) {
    return (t_end->tv_sec - t_start->tv_sec) * 1e9
         + (t_end->tv_nsec - t_start->tv_nsec);
}

/* ========== Generic Interface ========== */

typedef struct {
    const char *name;
    void *(*create)(void);
    void (*insert)(void *, int);
    bool (*search)(void *, int);
    void (*delete_key)(void *, int);
    void (*destroy)(void *);
    int  (*get_size)(void *);
} TreeInterface;

/* --- 2-3 Tree wrappers --- */
static void *wrap_tt_create(void)        { return tt_create(); }
static void  wrap_tt_insert(void *t, int k) { tt_insert((TT_Tree *)t, k); }
static bool  wrap_tt_search(void *t, int k) { return tt_search((TT_Tree *)t, k); }
static void  wrap_tt_delete(void *t, int k) { tt_delete((TT_Tree *)t, k); }
static void  wrap_tt_destroy(void *t)       { tt_destroy((TT_Tree *)t); }
static int   wrap_tt_size(void *t)          { return ((TT_Tree *)t)->size; }

/* --- Red-Black Tree wrappers --- */
static void *wrap_rb_create(void)        { return rb_create(); }
static void  wrap_rb_insert(void *t, int k) { rb_insert((RB_Tree *)t, k); }
static bool  wrap_rb_search(void *t, int k) { return rb_search((RB_Tree *)t, k); }
static void  wrap_rb_delete(void *t, int k) { rb_delete((RB_Tree *)t, k); }
static void  wrap_rb_destroy(void *t)       { rb_destroy((RB_Tree *)t); }
static int   wrap_rb_size(void *t)          { return ((RB_Tree *)t)->size; }

static TreeInterface TREES[] = {
    {"2-3 Tree",       wrap_tt_create, wrap_tt_insert, wrap_tt_search, wrap_tt_delete, wrap_tt_destroy, wrap_tt_size},
    {"Red-Black Tree", wrap_rb_create, wrap_rb_insert, wrap_rb_search, wrap_rb_delete, wrap_rb_destroy, wrap_rb_size},
};
#define NUM_TREES 2

/* ========== Data Generation ========== */

static int *make_random_array(int n) {
    int *arr = (int *)malloc(n * sizeof(int));
    for (int i = 0; i < n; i++) arr[i] = rand();
    return arr;
}

static int *make_sequential_array(int n) {
    int *arr = (int *)malloc(n * sizeof(int));
    for (int i = 0; i < n; i++) arr[i] = i;
    return arr;
}

static int *make_reverse_array(int n) {
    int *arr = (int *)malloc(n * sizeof(int));
    for (int i = 0; i < n; i++) arr[i] = n - 1 - i;
    return arr;
}

/* ========== Benchmark Result ========== */

typedef struct {
    double ins_ns_per_op;
    double srch_ns_per_op;
    double del_ns_per_op;
} TreeBenchResult;

/* ========== Scenarios ========== */

/* Scenario 1: Random data */
static TreeBenchResult tree_bench_random(TreeInterface *ds, int N) {
    TreeBenchResult res = {0};
    struct timespec t0, t1;
    void *obj = ds->create();
    int *data = make_random_array(N);

    clock_gettime(CLOCK_MONOTONIC, &t0);
    for (int i = 0; i < N; i++) ds->insert(obj, data[i]);
    clock_gettime(CLOCK_MONOTONIC, &t1);
    res.ins_ns_per_op = elapsed_ns(&t0, &t1) / N;

    clock_gettime(CLOCK_MONOTONIC, &t0);
    for (int i = 0; i < N / 2; i++) ds->search(obj, data[i]);
    for (int i = 0; i < N / 2; i++) ds->search(obj, rand() + N);
    clock_gettime(CLOCK_MONOTONIC, &t1);
    res.srch_ns_per_op = elapsed_ns(&t0, &t1) / N;

    clock_gettime(CLOCK_MONOTONIC, &t0);
    for (int i = 0; i < N / 2; i++) ds->delete_key(obj, data[i]);
    clock_gettime(CLOCK_MONOTONIC, &t1);
    res.del_ns_per_op = elapsed_ns(&t0, &t1) / (N / 2);

    free(data);
    ds->destroy(obj);
    return res;
}

/* Scenario 2: Sequential (sorted) data */
static TreeBenchResult tree_bench_sequential(TreeInterface *ds, int N) {
    TreeBenchResult res = {0};
    struct timespec t0, t1;
    void *obj = ds->create();
    int *data = make_sequential_array(N);

    clock_gettime(CLOCK_MONOTONIC, &t0);
    for (int i = 0; i < N; i++) ds->insert(obj, data[i]);
    clock_gettime(CLOCK_MONOTONIC, &t1);
    res.ins_ns_per_op = elapsed_ns(&t0, &t1) / N;

    clock_gettime(CLOCK_MONOTONIC, &t0);
    for (int i = 0; i < N; i++) ds->search(obj, data[i]);
    clock_gettime(CLOCK_MONOTONIC, &t1);
    res.srch_ns_per_op = elapsed_ns(&t0, &t1) / N;

    clock_gettime(CLOCK_MONOTONIC, &t0);
    for (int i = 0; i < N; i++) ds->delete_key(obj, data[i]);
    clock_gettime(CLOCK_MONOTONIC, &t1);
    res.del_ns_per_op = elapsed_ns(&t0, &t1) / N;

    free(data);
    ds->destroy(obj);
    return res;
}

/* Scenario 3: Reverse sorted data */
static TreeBenchResult tree_bench_reverse(TreeInterface *ds, int N) {
    TreeBenchResult res = {0};
    struct timespec t0, t1;
    void *obj = ds->create();
    int *data = make_reverse_array(N);

    clock_gettime(CLOCK_MONOTONIC, &t0);
    for (int i = 0; i < N; i++) ds->insert(obj, data[i]);
    clock_gettime(CLOCK_MONOTONIC, &t1);
    res.ins_ns_per_op = elapsed_ns(&t0, &t1) / N;

    clock_gettime(CLOCK_MONOTONIC, &t0);
    for (int i = 0; i < N; i++) ds->search(obj, data[i]);
    clock_gettime(CLOCK_MONOTONIC, &t1);
    res.srch_ns_per_op = elapsed_ns(&t0, &t1) / N;

    clock_gettime(CLOCK_MONOTONIC, &t0);
    for (int i = 0; i < N; i++) ds->delete_key(obj, data[i]);
    clock_gettime(CLOCK_MONOTONIC, &t1);
    res.del_ns_per_op = elapsed_ns(&t0, &t1) / N;

    free(data);
    ds->destroy(obj);
    return res;
}

/* Scenario 4: Mixed operations (40% insert, 40% search, 20% delete) */
static TreeBenchResult tree_bench_mixed(TreeInterface *ds, int N) {
    TreeBenchResult res = {0};
    struct timespec t0, t1;
    void *obj = ds->create();
    int next_val = 0;
    double ins_total = 0, srch_total = 0, del_total = 0;
    int ins_cnt = 0, srch_cnt = 0, del_cnt = 0;

    for (int i = 0; i < N; i++) {
        int op = rand() % 100;
        if (op < 40) {
            int val = next_val++;
            clock_gettime(CLOCK_MONOTONIC, &t0);
            ds->insert(obj, val);
            clock_gettime(CLOCK_MONOTONIC, &t1);
            ins_total += elapsed_ns(&t0, &t1);
            ins_cnt++;
        } else if (op < 80) {
            int val = rand() % (next_val + 1);
            clock_gettime(CLOCK_MONOTONIC, &t0);
            ds->search(obj, val);
            clock_gettime(CLOCK_MONOTONIC, &t1);
            srch_total += elapsed_ns(&t0, &t1);
            srch_cnt++;
        } else {
            int val = rand() % (next_val + 1);
            clock_gettime(CLOCK_MONOTONIC, &t0);
            ds->delete_key(obj, val);
            clock_gettime(CLOCK_MONOTONIC, &t1);
            del_total += elapsed_ns(&t0, &t1);
            del_cnt++;
        }
    }

    res.ins_ns_per_op = ins_cnt > 0 ? ins_total / ins_cnt : 0;
    res.srch_ns_per_op = srch_cnt > 0 ? srch_total / srch_cnt : 0;
    res.del_ns_per_op = del_cnt > 0 ? del_total / del_cnt : 0;

    ds->destroy(obj);
    return res;
}

/* Scenario 5: Delete-heavy */
static TreeBenchResult tree_bench_delete_heavy(TreeInterface *ds, int N) {
    TreeBenchResult res = {0};
    struct timespec t0, t1;
    void *obj = ds->create();
    int *data = make_random_array(N);

    /* fill the structure first */
    for (int i = 0; i < N; i++) ds->insert(obj, data[i]);

    /* delete 80% */
    int del_n = (int)(N * 0.8);
    clock_gettime(CLOCK_MONOTONIC, &t0);
    for (int i = 0; i < del_n; i++) ds->delete_key(obj, data[i]);
    clock_gettime(CLOCK_MONOTONIC, &t1);
    res.del_ns_per_op = elapsed_ns(&t0, &t1) / del_n;

    /* re-insert 50% new values */
    int reins_n = N / 2;
    clock_gettime(CLOCK_MONOTONIC, &t0);
    for (int i = 0; i < reins_n; i++) ds->insert(obj, rand());
    clock_gettime(CLOCK_MONOTONIC, &t1);
    res.ins_ns_per_op = elapsed_ns(&t0, &t1) / reins_n;

    /* search N values */
    clock_gettime(CLOCK_MONOTONIC, &t0);
    for (int i = 0; i < N; i++) ds->search(obj, data[i]);
    clock_gettime(CLOCK_MONOTONIC, &t1);
    res.srch_ns_per_op = elapsed_ns(&t0, &t1) / N;

    free(data);
    ds->destroy(obj);
    return res;
}

/* ========== Correctness Check ========== */

static int verify_tree(TreeInterface *ds) {
    void *obj = ds->create();
    int errors = 0;
    int N = 1000;

    /* insert 0..N-1 */
    for (int i = 0; i < N; i++) ds->insert(obj, i);
    if (ds->get_size(obj) != N) {
        printf("  ERROR: %s size after insert = %d (expected %d)\n",
               ds->name, ds->get_size(obj), N);
        errors++;
    }

    /* search all inserted */
    for (int i = 0; i < N; i++) {
        if (!ds->search(obj, i)) {
            printf("  ERROR: %s search(%d) failed\n", ds->name, i);
            errors++;
            break;
        }
    }

    /* search non-existing */
    for (int i = N; i < N + 100; i++) {
        if (ds->search(obj, i)) {
            printf("  ERROR: %s search(%d) found phantom\n", ds->name, i);
            errors++;
            break;
        }
    }

    /* duplicate insert must not change size */
    ds->insert(obj, 0);
    if (ds->get_size(obj) != N) {
        printf("  ERROR: %s size changed on duplicate = %d\n", ds->name, ds->get_size(obj));
        errors++;
    }

    /* delete even numbers */
    for (int i = 0; i < N; i += 2) ds->delete_key(obj, i);
    if (ds->get_size(obj) != N / 2) {
        printf("  ERROR: %s size after partial delete = %d (expected %d)\n",
               ds->name, ds->get_size(obj), N / 2);
        errors++;
    }

    /* verify: evens gone, odds remain */
    for (int i = 0; i < N; i++) {
        bool found = ds->search(obj, i);
        bool expected = (i % 2 == 1);
        if (found != expected) {
            printf("  ERROR: %s search(%d) = %d (expected %d)\n",
                   ds->name, i, found, expected);
            errors++;
            break;
        }
    }

    /* delete non-existing should not crash */
    ds->delete_key(obj, -1);
    ds->delete_key(obj, N + 1);

    /* delete remaining */
    for (int i = 1; i < N; i += 2) ds->delete_key(obj, i);
    if (ds->get_size(obj) != 0) {
        printf("  ERROR: %s size after full delete = %d\n", ds->name, ds->get_size(obj));
        errors++;
    }

    ds->destroy(obj);
    return errors;
}

/* ========== Scenario Table ========== */

typedef TreeBenchResult (*TreeScenarioFn)(TreeInterface *ds, int N);

typedef struct {
    const char *name;
    TreeScenarioFn fn;
} TreeScenario;

static TreeScenario TREE_SCENARIOS[] = {
    {"Random",       tree_bench_random},
    {"Sequential",   tree_bench_sequential},
    {"Reverse",      tree_bench_reverse},
    {"Mixed",        tree_bench_mixed},
    {"Delete-heavy", tree_bench_delete_heavy},
};
#define NUM_TREE_SCENARIOS 5

static int TREE_SIZES[] = {1000, 10000, 100000, 1000000, 10000000};
#define NUM_TREE_SIZES 5

/* ========== Main ========== */

int main(void) {
    srand(42);

    printf("================================================================\n");
    printf("  Tree Comparison: 2-3 Tree vs Red-Black Tree\n");
    printf("================================================================\n\n");

    /* correctness */
    printf("--- Correctness Verification ---\n");
    int total_err = 0;
    for (int d = 0; d < NUM_TREES; d++) {
        int err = verify_tree(&TREES[d]);
        printf("  %-16s: %s\n", TREES[d].name, err == 0 ? "OK" : "FAILED");
        total_err += err;
    }
    if (total_err > 0) {
        printf("\nVerification failed (%d errors). Stopping.\n", total_err);
        return 1;
    }
    printf("Both trees passed all correctness checks.\n\n");

    /* CSV output */
    FILE *csv = fopen("tree_results.csv", "w");
    if (!csv) { perror("Cannot create tree_results.csv"); return 1; }
    fprintf(csv, "Scenario;Size;DataStructure;AvgInsert_ns;AvgSearch_ns;AvgDelete_ns\n");

    /* benchmarks */
    for (int s = 0; s < NUM_TREE_SCENARIOS; s++) {
        printf("=== Scenario: %s ===\n", TREE_SCENARIOS[s].name);
        for (int z = 0; z < NUM_TREE_SIZES; z++) {
            int N = TREE_SIZES[z];
            printf("\n  N = %d\n", N);
            printf("  %-16s | %12s | %12s | %12s\n",
                   "Structure", "Insert ns/op", "Search ns/op", "Delete ns/op");
            printf("  %-16s-+-%12s-+-%12s-+-%12s\n",
                   "----------------", "------------", "------------", "------------");

            for (int d = 0; d < NUM_TREES; d++) {
                srand(42);
                TreeBenchResult r = TREE_SCENARIOS[s].fn(&TREES[d], N);
                printf("  %-16s | %12.1f | %12.1f | %12.1f\n",
                       TREES[d].name, r.ins_ns_per_op, r.srch_ns_per_op, r.del_ns_per_op);

                /* CSV with comma decimal separator for Slovak locale */
                char c_ins[32], c_srch[32], c_del[32];
                snprintf(c_ins, sizeof(c_ins), "%.1f", r.ins_ns_per_op);
                snprintf(c_srch, sizeof(c_srch), "%.1f", r.srch_ns_per_op);
                snprintf(c_del, sizeof(c_del), "%.1f", r.del_ns_per_op);
                for (char *p = c_ins; *p; p++) if (*p == '.') *p = ',';
                for (char *p = c_srch; *p; p++) if (*p == '.') *p = ',';
                for (char *p = c_del; *p; p++) if (*p == '.') *p = ',';
                fprintf(csv, "%s;%d;%s;%s;%s;%s\n",
                        TREE_SCENARIOS[s].name, N, TREES[d].name, c_ins, c_srch, c_del);
            }
        }
        printf("\n");
    }

    fclose(csv);
    printf("Results saved to tree_results.csv\n");
    printf("Done.\n");
    return 0;
}

#endif /* TESTER_TREES_H */
