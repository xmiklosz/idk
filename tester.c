/*
 * Tester - Comparison of Dynamic Set Search Implementations
 * Compares: 2-3 Tree, Red-Black Tree, Hash Chain, Hash Double Hashing
 *
 * Compile: gcc -O2 -o tester tester.c -lm
 * Run:     ./tester
 *
 * Outputs results to console and CSV file (results.csv) for graphing.
 */
#include "tt_tree.h"
#include "rb_tree.h"
#include "hc_table.h"
#include "hd_table.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

/* ========== Timing ========== */

static double time_ns(struct timespec *start, struct timespec *end) {
    return (end->tv_sec - start->tv_sec) * 1e9 + (end->tv_nsec - start->tv_nsec);
}

/* ========== Generic Interface ========== */

typedef struct {
    const char *name;
    void *(*create)(void);
    void (*insert)(void *, int);
    bool (*search)(void *, int);
    void (*delete_fn)(void *, int);
    void (*destroy)(void *);
    int  (*get_count)(void *);
} DS_Interface;

/* --- Wrappers for 2-3 Tree --- */
static void *w_tt_create(void) { return tt_create(); }
static void  w_tt_insert(void *t, int k) { tt_insert((TT_Tree *)t, k); }
static bool  w_tt_search(void *t, int k) { return tt_search((TT_Tree *)t, k); }
static void  w_tt_delete(void *t, int k) { tt_delete((TT_Tree *)t, k); }
static void  w_tt_destroy(void *t) { tt_destroy((TT_Tree *)t); }
static int   w_tt_count(void *t) { return ((TT_Tree *)t)->count; }

/* --- Wrappers for Red-Black Tree --- */
static void *w_rb_create(void) { return rb_create(); }
static void  w_rb_insert(void *t, int k) { rb_insert((RB_Tree *)t, k); }
static bool  w_rb_search(void *t, int k) { return rb_search((RB_Tree *)t, k); }
static void  w_rb_delete(void *t, int k) { rb_delete((RB_Tree *)t, k); }
static void  w_rb_destroy(void *t) { rb_destroy((RB_Tree *)t); }
static int   w_rb_count(void *t) { return ((RB_Tree *)t)->count; }

/* --- Wrappers for Hash Chain --- */
static void *w_hc_create(void) { return hc_create(); }
static void  w_hc_insert(void *t, int k) { hc_insert((HC_Table *)t, k); }
static bool  w_hc_search(void *t, int k) { return hc_search((HC_Table *)t, k); }
static void  w_hc_delete(void *t, int k) { hc_delete((HC_Table *)t, k); }
static void  w_hc_destroy(void *t) { hc_destroy((HC_Table *)t); }
static int   w_hc_count(void *t) { return ((HC_Table *)t)->count; }

/* --- Wrappers for Hash Double --- */
static void *w_hd_create(void) { return hd_create(); }
static void  w_hd_insert(void *t, int k) { hd_insert((HD_Table *)t, k); }
static bool  w_hd_search(void *t, int k) { return hd_search((HD_Table *)t, k); }
static void  w_hd_delete(void *t, int k) { hd_delete((HD_Table *)t, k); }
static void  w_hd_destroy(void *t) { hd_destroy((HD_Table *)t); }
static int   w_hd_count(void *t) { return ((HD_Table *)t)->count; }

static DS_Interface ALL_DS[] = {
    {"2-3 Tree",       w_tt_create, w_tt_insert, w_tt_search, w_tt_delete, w_tt_destroy, w_tt_count},
    {"Red-Black Tree", w_rb_create, w_rb_insert, w_rb_search, w_rb_delete, w_rb_destroy, w_rb_count},
    {"Hash Chain",     w_hc_create, w_hc_insert, w_hc_search, w_hc_delete, w_hc_destroy, w_hc_count},
    {"Hash Double",    w_hd_create, w_hd_insert, w_hd_search, w_hd_delete, w_hd_destroy, w_hd_count},
};
#define NUM_DS 4

/* ========== Data Generation ========== */

static int *generate_random(int n) {
    int *arr = (int *)malloc(n * sizeof(int));
    for (int i = 0; i < n; i++) arr[i] = rand();
    return arr;
}

static int *generate_sequential(int n) {
    int *arr = (int *)malloc(n * sizeof(int));
    for (int i = 0; i < n; i++) arr[i] = i;
    return arr;
}

static int *generate_reverse(int n) {
    int *arr = (int *)malloc(n * sizeof(int));
    for (int i = 0; i < n; i++) arr[i] = n - 1 - i;
    return arr;
}

static void shuffle(int *arr, int n) {
    for (int i = n - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        int tmp = arr[i]; arr[i] = arr[j]; arr[j] = tmp;
    }
}

/* ========== Benchmark Result ========== */

typedef struct {
    double insert_ns;
    double search_ns;
    double delete_ns;
    int count_after_insert;
    int count_after_delete;
} BenchResult;

/* ========== Scenarios ========== */

/*
 * Scenario 1: Random data
 * Insert N random values, search N (mix of existing and non-existing), delete N/2
 */
static BenchResult run_random(DS_Interface *ds, int N) {
    BenchResult res = {0};
    struct timespec t0, t1;
    void *obj = ds->create();

    int *data = generate_random(N);

    /* Insert */
    clock_gettime(CLOCK_MONOTONIC, &t0);
    for (int i = 0; i < N; i++) ds->insert(obj, data[i]);
    clock_gettime(CLOCK_MONOTONIC, &t1);
    res.insert_ns = time_ns(&t0, &t1) / N;
    res.count_after_insert = ds->get_count(obj);

    /* Search (half existing, half random) */
    clock_gettime(CLOCK_MONOTONIC, &t0);
    for (int i = 0; i < N / 2; i++) ds->search(obj, data[i]);
    for (int i = 0; i < N / 2; i++) ds->search(obj, rand() + N);
    clock_gettime(CLOCK_MONOTONIC, &t1);
    res.search_ns = time_ns(&t0, &t1) / N;

    /* Delete half */
    clock_gettime(CLOCK_MONOTONIC, &t0);
    for (int i = 0; i < N / 2; i++) ds->delete_fn(obj, data[i]);
    clock_gettime(CLOCK_MONOTONIC, &t1);
    res.delete_ns = time_ns(&t0, &t1) / (N / 2);
    res.count_after_delete = ds->get_count(obj);

    free(data);
    ds->destroy(obj);
    return res;
}

/*
 * Scenario 2: Sequential (sorted) data
 * Insert 0..N-1, search all, delete all
 */
static BenchResult run_sequential(DS_Interface *ds, int N) {
    BenchResult res = {0};
    struct timespec t0, t1;
    void *obj = ds->create();

    int *data = generate_sequential(N);

    /* Insert in order */
    clock_gettime(CLOCK_MONOTONIC, &t0);
    for (int i = 0; i < N; i++) ds->insert(obj, data[i]);
    clock_gettime(CLOCK_MONOTONIC, &t1);
    res.insert_ns = time_ns(&t0, &t1) / N;
    res.count_after_insert = ds->get_count(obj);

    /* Search all */
    clock_gettime(CLOCK_MONOTONIC, &t0);
    for (int i = 0; i < N; i++) ds->search(obj, data[i]);
    clock_gettime(CLOCK_MONOTONIC, &t1);
    res.search_ns = time_ns(&t0, &t1) / N;

    /* Delete all */
    clock_gettime(CLOCK_MONOTONIC, &t0);
    for (int i = 0; i < N; i++) ds->delete_fn(obj, data[i]);
    clock_gettime(CLOCK_MONOTONIC, &t1);
    res.delete_ns = time_ns(&t0, &t1) / N;
    res.count_after_delete = ds->get_count(obj);

    free(data);
    ds->destroy(obj);
    return res;
}

/*
 * Scenario 3: Reverse sorted data
 * Insert N-1..0, search all, delete all
 */
static BenchResult run_reverse(DS_Interface *ds, int N) {
    BenchResult res = {0};
    struct timespec t0, t1;
    void *obj = ds->create();

    int *data = generate_reverse(N);

    clock_gettime(CLOCK_MONOTONIC, &t0);
    for (int i = 0; i < N; i++) ds->insert(obj, data[i]);
    clock_gettime(CLOCK_MONOTONIC, &t1);
    res.insert_ns = time_ns(&t0, &t1) / N;
    res.count_after_insert = ds->get_count(obj);

    clock_gettime(CLOCK_MONOTONIC, &t0);
    for (int i = 0; i < N; i++) ds->search(obj, data[i]);
    clock_gettime(CLOCK_MONOTONIC, &t1);
    res.search_ns = time_ns(&t0, &t1) / N;

    clock_gettime(CLOCK_MONOTONIC, &t0);
    for (int i = 0; i < N; i++) ds->delete_fn(obj, data[i]);
    clock_gettime(CLOCK_MONOTONIC, &t1);
    res.delete_ns = time_ns(&t0, &t1) / N;
    res.count_after_delete = ds->get_count(obj);

    free(data);
    ds->destroy(obj);
    return res;
}

/*
 * Scenario 4: Mixed operations
 * Interleaved insert/search/delete (40/40/20 split)
 */
static BenchResult run_mixed(DS_Interface *ds, int N) {
    BenchResult res = {0};
    struct timespec t0, t1;
    void *obj = ds->create();

    int ins_count = 0, srch_count = 0, del_count = 0;
    double ins_total = 0, srch_total = 0, del_total = 0;
    int next_val = 0;

    for (int i = 0; i < N; i++) {
        int op = rand() % 100;
        if (op < 40) {
            /* Insert */
            int val = next_val++;
            clock_gettime(CLOCK_MONOTONIC, &t0);
            ds->insert(obj, val);
            clock_gettime(CLOCK_MONOTONIC, &t1);
            ins_total += time_ns(&t0, &t1);
            ins_count++;
        } else if (op < 80) {
            /* Search */
            int val = rand() % (next_val + 1);
            clock_gettime(CLOCK_MONOTONIC, &t0);
            ds->search(obj, val);
            clock_gettime(CLOCK_MONOTONIC, &t1);
            srch_total += time_ns(&t0, &t1);
            srch_count++;
        } else {
            /* Delete */
            int val = rand() % (next_val + 1);
            clock_gettime(CLOCK_MONOTONIC, &t0);
            ds->delete_fn(obj, val);
            clock_gettime(CLOCK_MONOTONIC, &t1);
            del_total += time_ns(&t0, &t1);
            del_count++;
        }
    }

    res.insert_ns = ins_count > 0 ? ins_total / ins_count : 0;
    res.search_ns = srch_count > 0 ? srch_total / srch_count : 0;
    res.delete_ns = del_count > 0 ? del_total / del_count : 0;
    res.count_after_insert = ds->get_count(obj);

    ds->destroy(obj);
    return res;
}

/*
 * Scenario 5: Delete-heavy
 * Insert N, then delete 80%, then insert 50% more, then search all
 */
static BenchResult run_delete_heavy(DS_Interface *ds, int N) {
    BenchResult res = {0};
    struct timespec t0, t1;
    void *obj = ds->create();

    int *data = generate_random(N);

    /* Insert all */
    for (int i = 0; i < N; i++) ds->insert(obj, data[i]);
    res.count_after_insert = ds->get_count(obj);

    /* Delete 80% */
    int del_n = (int)(N * 0.8);
    clock_gettime(CLOCK_MONOTONIC, &t0);
    for (int i = 0; i < del_n; i++) ds->delete_fn(obj, data[i]);
    clock_gettime(CLOCK_MONOTONIC, &t1);
    res.delete_ns = time_ns(&t0, &t1) / del_n;

    /* Re-insert 50% */
    int reins_n = N / 2;
    clock_gettime(CLOCK_MONOTONIC, &t0);
    for (int i = 0; i < reins_n; i++) ds->insert(obj, rand());
    clock_gettime(CLOCK_MONOTONIC, &t1);
    res.insert_ns = time_ns(&t0, &t1) / reins_n;

    /* Search all remaining */
    int cur_count = ds->get_count(obj);
    clock_gettime(CLOCK_MONOTONIC, &t0);
    for (int i = 0; i < N; i++) ds->search(obj, data[i]);
    clock_gettime(CLOCK_MONOTONIC, &t1);
    res.search_ns = time_ns(&t0, &t1) / N;
    res.count_after_delete = cur_count;

    free(data);
    ds->destroy(obj);
    return res;
}

/* ========== Correctness Verification ========== */

static int verify_ds(DS_Interface *ds) {
    void *obj = ds->create();
    int errors = 0;
    int N = 1000;

    /* Insert 0..N-1 */
    for (int i = 0; i < N; i++) ds->insert(obj, i);
    if (ds->get_count(obj) != N) {
        printf("  ERROR: %s count after insert = %d (expected %d)\n",
               ds->name, ds->get_count(obj), N);
        errors++;
    }

    /* Search all */
    for (int i = 0; i < N; i++) {
        if (!ds->search(obj, i)) {
            printf("  ERROR: %s search(%d) failed after insert\n", ds->name, i);
            errors++;
            break;
        }
    }

    /* Search non-existing */
    for (int i = N; i < N + 100; i++) {
        if (ds->search(obj, i)) {
            printf("  ERROR: %s search(%d) found non-existing\n", ds->name, i);
            errors++;
            break;
        }
    }

    /* Duplicate insert */
    ds->insert(obj, 0);
    if (ds->get_count(obj) != N) {
        printf("  ERROR: %s count changed after duplicate insert = %d\n",
               ds->name, ds->get_count(obj));
        errors++;
    }

    /* Delete even numbers */
    for (int i = 0; i < N; i += 2) ds->delete_fn(obj, i);
    if (ds->get_count(obj) != N / 2) {
        printf("  ERROR: %s count after delete = %d (expected %d)\n",
               ds->name, ds->get_count(obj), N / 2);
        errors++;
    }

    /* Verify deleted are gone, others remain */
    for (int i = 0; i < N; i++) {
        bool found = ds->search(obj, i);
        bool expected = (i % 2 == 1);
        if (found != expected) {
            printf("  ERROR: %s search(%d) = %d (expected %d) after partial delete\n",
                   ds->name, i, found, expected);
            errors++;
            break;
        }
    }

    /* Delete non-existing (should not crash) */
    ds->delete_fn(obj, -1);
    ds->delete_fn(obj, N + 1);

    /* Delete remaining */
    for (int i = 1; i < N; i += 2) ds->delete_fn(obj, i);
    if (ds->get_count(obj) != 0) {
        printf("  ERROR: %s count after full delete = %d\n",
               ds->name, ds->get_count(obj));
        errors++;
    }

    ds->destroy(obj);
    return errors;
}

/* ========== Main ========== */

typedef BenchResult (*ScenarioFn)(DS_Interface *ds, int N);

typedef struct {
    const char *name;
    ScenarioFn fn;
} Scenario;

static Scenario SCENARIOS[] = {
    {"Random",       run_random},
    {"Sequential",   run_sequential},
    {"Reverse",      run_reverse},
    {"Mixed",        run_mixed},
    {"Delete-heavy", run_delete_heavy},
};
#define NUM_SCENARIOS 5

static int SIZES[] = {1000, 10000, 100000, 1000000, 10000000};
#define NUM_SIZES 5

int main(void) {
    srand(time(NULL));

    printf("================================================================\n");
    printf("  Dynamic Set Search - Implementation Comparison\n");
    printf("  Structures: 2-3 Tree, Red-Black Tree, Hash Chain, Hash Double\n");
    printf("================================================================\n\n");

    /* Correctness verification */
    printf("--- Correctness Verification ---\n");
    int total_errors = 0;
    for (int d = 0; d < NUM_DS; d++) {
        int err = verify_ds(&ALL_DS[d]);
        printf("  %-16s: %s\n", ALL_DS[d].name, err == 0 ? "OK" : "FAILED");
        total_errors += err;
    }
    if (total_errors > 0) {
        printf("\nCorrectness verification failed with %d errors. Aborting.\n", total_errors);
        return 1;
    }
    printf("All structures passed correctness checks.\n\n");

    /* Open CSV */
    FILE *csv = fopen("results.csv", "w");
    if (!csv) { perror("Cannot open results.csv"); return 1; }
    fprintf(csv, "Scenario;Size;DataStructure;AvgInsert_ns;AvgSearch_ns;AvgDelete_ns\n");

    /* Run benchmarks */
    for (int s = 0; s < NUM_SCENARIOS; s++) {
        printf("=== Scenario: %s ===\n", SCENARIOS[s].name);
        for (int z = 0; z < NUM_SIZES; z++) {
            int N = SIZES[z];
            printf("\n  N = %d\n", N);
            printf("  %-16s | %12s | %12s | %12s\n",
                   "Data Structure", "Insert ns/op", "Search ns/op", "Delete ns/op");
            printf("  %-16s-+-%12s-+-%12s-+-%12s\n",
                   "----------------", "------------", "------------", "------------");

            for (int d = 0; d < NUM_DS; d++) {
                srand(time(NULL) ^ (d * 31 + z * 997 + s * 7919));
                BenchResult r = SCENARIOS[s].fn(&ALL_DS[d], N);
                printf("  %-16s | %12.1f | %12.1f | %12.1f\n",
                       ALL_DS[d].name, r.insert_ns, r.search_ns, r.delete_ns);

                /* CSV output with comma decimal separator */
                char ins[32], srch[32], del[32];
                snprintf(ins, sizeof(ins), "%.1f", r.insert_ns);
                snprintf(srch, sizeof(srch), "%.1f", r.search_ns);
                snprintf(del, sizeof(del), "%.1f", r.delete_ns);
                /* Replace . with , for Slovak locale */
                for (char *p = ins; *p; p++) if (*p == '.') *p = ',';
                for (char *p = srch; *p; p++) if (*p == '.') *p = ',';
                for (char *p = del; *p; p++) if (*p == '.') *p = ',';
                fprintf(csv, "%s;%d;%s;%s;%s;%s\n",
                        SCENARIOS[s].name, N, ALL_DS[d].name, ins, srch, del);
            }
        }
        printf("\n");
    }

    fclose(csv);
    printf("Results saved to results.csv\n");
    printf("Done.\n");
    return 0;
}
