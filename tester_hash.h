/*
 * Hash Tester - Comparison of Hash Chain vs Hash Double Hashing
 * Benchmarks insert, search, delete across multiple scenarios and sizes.
 * Outputs results to console and hash_results.csv.
 *
 * Compile: gcc -O2 -o tester_hash -x c tester_hash.h -lm
 * Run:     ./tester_hash
 */
#ifndef TESTER_HASH_H
#define TESTER_HASH_H

#include "hc_table.c"
#include "hd_table.c"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* ========== Timing ========== */

static double elapsed_ns_h(struct timespec *t_start, struct timespec *t_end) {
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
} HashInterface;

/* --- Hash Chain wrappers --- */
static void *wrap_hc_create(void)        { return hc_create(); }
static void  wrap_hc_insert(void *t, int k) { hc_insert((HC_Table *)t, k); }
static bool  wrap_hc_search(void *t, int k) { return hc_search((HC_Table *)t, k); }
static void  wrap_hc_delete(void *t, int k) { hc_delete((HC_Table *)t, k); }
static void  wrap_hc_destroy(void *t)       { hc_destroy((HC_Table *)t); }
static int   wrap_hc_size(void *t)          { return ((HC_Table *)t)->size; }

/* --- Hash Double wrappers --- */
static void *wrap_hd_create(void)        { return hd_create(); }
static void  wrap_hd_insert(void *t, int k) { hd_insert((HD_Table *)t, k); }
static bool  wrap_hd_search(void *t, int k) { return hd_search((HD_Table *)t, k); }
static void  wrap_hd_delete(void *t, int k) { hd_delete((HD_Table *)t, k); }
static void  wrap_hd_destroy(void *t)       { hd_destroy((HD_Table *)t); }
static int   wrap_hd_size(void *t)          { return ((HD_Table *)t)->size; }

static HashInterface HASHES[] = {
    {"Hash Chain",  wrap_hc_create, wrap_hc_insert, wrap_hc_search, wrap_hc_delete, wrap_hc_destroy, wrap_hc_size},
    {"Hash Double", wrap_hd_create, wrap_hd_insert, wrap_hd_search, wrap_hd_delete, wrap_hd_destroy, wrap_hd_size},
};
#define NUM_HASHES 2

/* ========== Data Generation ========== */

static int *gen_random_data(int n) {
    int *arr = (int *)malloc(n * sizeof(int));
    for (int i = 0; i < n; i++) arr[i] = rand();
    return arr;
}

static int *gen_sequential_data(int n) {
    int *arr = (int *)malloc(n * sizeof(int));
    for (int i = 0; i < n; i++) arr[i] = i;
    return arr;
}

static int *gen_reverse_data(int n) {
    int *arr = (int *)malloc(n * sizeof(int));
    for (int i = 0; i < n; i++) arr[i] = n - 1 - i;
    return arr;
}

/* ========== Benchmark Result ========== */

typedef struct {
    double ins_ns_per_op;
    double srch_ns_per_op;
    double del_ns_per_op;
} HashBenchResult;

/* ========== Scenarios ========== */

/* Scenario 1: Random data */
static HashBenchResult hash_bench_random(HashInterface *ds, int N) {
    HashBenchResult res = {0};
    struct timespec t0, t1;
    void *obj = ds->create();
    int *data = gen_random_data(N);

    clock_gettime(CLOCK_MONOTONIC, &t0);
    for (int i = 0; i < N; i++) ds->insert(obj, data[i]);
    clock_gettime(CLOCK_MONOTONIC, &t1);
    res.ins_ns_per_op = elapsed_ns_h(&t0, &t1) / N;

    clock_gettime(CLOCK_MONOTONIC, &t0);
    for (int i = 0; i < N / 2; i++) ds->search(obj, data[i]);
    for (int i = 0; i < N / 2; i++) ds->search(obj, rand() + N);
    clock_gettime(CLOCK_MONOTONIC, &t1);
    res.srch_ns_per_op = elapsed_ns_h(&t0, &t1) / N;

    clock_gettime(CLOCK_MONOTONIC, &t0);
    for (int i = 0; i < N / 2; i++) ds->delete_key(obj, data[i]);
    clock_gettime(CLOCK_MONOTONIC, &t1);
    res.del_ns_per_op = elapsed_ns_h(&t0, &t1) / (N / 2);

    free(data);
    ds->destroy(obj);
    return res;
}

/* Scenario 2: Sequential data */
static HashBenchResult hash_bench_sequential(HashInterface *ds, int N) {
    HashBenchResult res = {0};
    struct timespec t0, t1;
    void *obj = ds->create();
    int *data = gen_sequential_data(N);

    clock_gettime(CLOCK_MONOTONIC, &t0);
    for (int i = 0; i < N; i++) ds->insert(obj, data[i]);
    clock_gettime(CLOCK_MONOTONIC, &t1);
    res.ins_ns_per_op = elapsed_ns_h(&t0, &t1) / N;

    clock_gettime(CLOCK_MONOTONIC, &t0);
    for (int i = 0; i < N; i++) ds->search(obj, data[i]);
    clock_gettime(CLOCK_MONOTONIC, &t1);
    res.srch_ns_per_op = elapsed_ns_h(&t0, &t1) / N;

    clock_gettime(CLOCK_MONOTONIC, &t0);
    for (int i = 0; i < N; i++) ds->delete_key(obj, data[i]);
    clock_gettime(CLOCK_MONOTONIC, &t1);
    res.del_ns_per_op = elapsed_ns_h(&t0, &t1) / N;

    free(data);
    ds->destroy(obj);
    return res;
}

/* Scenario 3: Reverse sorted data */
static HashBenchResult hash_bench_reverse(HashInterface *ds, int N) {
    HashBenchResult res = {0};
    struct timespec t0, t1;
    void *obj = ds->create();
    int *data = gen_reverse_data(N);

    clock_gettime(CLOCK_MONOTONIC, &t0);
    for (int i = 0; i < N; i++) ds->insert(obj, data[i]);
    clock_gettime(CLOCK_MONOTONIC, &t1);
    res.ins_ns_per_op = elapsed_ns_h(&t0, &t1) / N;

    clock_gettime(CLOCK_MONOTONIC, &t0);
    for (int i = 0; i < N; i++) ds->search(obj, data[i]);
    clock_gettime(CLOCK_MONOTONIC, &t1);
    res.srch_ns_per_op = elapsed_ns_h(&t0, &t1) / N;

    clock_gettime(CLOCK_MONOTONIC, &t0);
    for (int i = 0; i < N; i++) ds->delete_key(obj, data[i]);
    clock_gettime(CLOCK_MONOTONIC, &t1);
    res.del_ns_per_op = elapsed_ns_h(&t0, &t1) / N;

    free(data);
    ds->destroy(obj);
    return res;
}

/* Scenario 4: Mixed operations (40/40/20) */
static HashBenchResult hash_bench_mixed(HashInterface *ds, int N) {
    HashBenchResult res = {0};
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
            ins_total += elapsed_ns_h(&t0, &t1);
            ins_cnt++;
        } else if (op < 80) {
            int val = rand() % (next_val + 1);
            clock_gettime(CLOCK_MONOTONIC, &t0);
            ds->search(obj, val);
            clock_gettime(CLOCK_MONOTONIC, &t1);
            srch_total += elapsed_ns_h(&t0, &t1);
            srch_cnt++;
        } else {
            int val = rand() % (next_val + 1);
            clock_gettime(CLOCK_MONOTONIC, &t0);
            ds->delete_key(obj, val);
            clock_gettime(CLOCK_MONOTONIC, &t1);
            del_total += elapsed_ns_h(&t0, &t1);
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
static HashBenchResult hash_bench_delete_heavy(HashInterface *ds, int N) {
    HashBenchResult res = {0};
    struct timespec t0, t1;
    void *obj = ds->create();
    int *data = gen_random_data(N);

    for (int i = 0; i < N; i++) ds->insert(obj, data[i]);

    int del_n = (int)(N * 0.8);
    clock_gettime(CLOCK_MONOTONIC, &t0);
    for (int i = 0; i < del_n; i++) ds->delete_key(obj, data[i]);
    clock_gettime(CLOCK_MONOTONIC, &t1);
    res.del_ns_per_op = elapsed_ns_h(&t0, &t1) / del_n;

    int reins_n = N / 2;
    clock_gettime(CLOCK_MONOTONIC, &t0);
    for (int i = 0; i < reins_n; i++) ds->insert(obj, rand());
    clock_gettime(CLOCK_MONOTONIC, &t1);
    res.ins_ns_per_op = elapsed_ns_h(&t0, &t1) / reins_n;

    clock_gettime(CLOCK_MONOTONIC, &t0);
    for (int i = 0; i < N; i++) ds->search(obj, data[i]);
    clock_gettime(CLOCK_MONOTONIC, &t1);
    res.srch_ns_per_op = elapsed_ns_h(&t0, &t1) / N;

    free(data);
    ds->destroy(obj);
    return res;
}

/* ========== Correctness Check ========== */

static int verify_hash(HashInterface *ds) {
    void *obj = ds->create();
    int errors = 0;
    int N = 1000;

    for (int i = 0; i < N; i++) ds->insert(obj, i);
    if (ds->get_size(obj) != N) {
        printf("  ERROR: %s size after insert = %d (expected %d)\n",
               ds->name, ds->get_size(obj), N);
        errors++;
    }

    for (int i = 0; i < N; i++) {
        if (!ds->search(obj, i)) {
            printf("  ERROR: %s search(%d) failed\n", ds->name, i);
            errors++;
            break;
        }
    }

    for (int i = N; i < N + 100; i++) {
        if (ds->search(obj, i)) {
            printf("  ERROR: %s search(%d) found phantom\n", ds->name, i);
            errors++;
            break;
        }
    }

    ds->insert(obj, 0);
    if (ds->get_size(obj) != N) {
        printf("  ERROR: %s size changed on duplicate = %d\n", ds->name, ds->get_size(obj));
        errors++;
    }

    for (int i = 0; i < N; i += 2) ds->delete_key(obj, i);
    if (ds->get_size(obj) != N / 2) {
        printf("  ERROR: %s size after partial delete = %d (expected %d)\n",
               ds->name, ds->get_size(obj), N / 2);
        errors++;
    }

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

    ds->delete_key(obj, -1);
    ds->delete_key(obj, N + 1);

    for (int i = 1; i < N; i += 2) ds->delete_key(obj, i);
    if (ds->get_size(obj) != 0) {
        printf("  ERROR: %s size after full delete = %d\n", ds->name, ds->get_size(obj));
        errors++;
    }

    ds->destroy(obj);
    return errors;
}

/* ========== Scenario Table ========== */

typedef HashBenchResult (*HashScenarioFn)(HashInterface *ds, int N);

typedef struct {
    const char *name;
    HashScenarioFn fn;
} HashScenario;

static HashScenario HASH_SCENARIOS[] = {
    {"Random",       hash_bench_random},
    {"Sequential",   hash_bench_sequential},
    {"Reverse",      hash_bench_reverse},
    {"Mixed",        hash_bench_mixed},
    {"Delete-heavy", hash_bench_delete_heavy},
};
#define NUM_HASH_SCENARIOS 5

static int HASH_SIZES[] = {1000, 10000, 100000, 1000000, 10000000};
#define NUM_HASH_SIZES 5

/* ========== Main ========== */

int main(void) {
    srand(42);

    printf("================================================================\n");
    printf("  Hash Table Comparison: Chaining vs Double Hashing\n");
    printf("================================================================\n\n");

    /* correctness */
    printf("--- Correctness Verification ---\n");
    int total_err = 0;
    for (int d = 0; d < NUM_HASHES; d++) {
        int err = verify_hash(&HASHES[d]);
        printf("  %-16s: %s\n", HASHES[d].name, err == 0 ? "OK" : "FAILED");
        total_err += err;
    }
    if (total_err > 0) {
        printf("\nVerification failed (%d errors). Stopping.\n", total_err);
        return 1;
    }
    printf("Both hash tables passed all correctness checks.\n\n");

    /* CSV output */
    FILE *csv = fopen("hash_results.csv", "w");
    if (!csv) { perror("Cannot create hash_results.csv"); return 1; }
    fprintf(csv, "Scenario;Size;DataStructure;AvgInsert_ns;AvgSearch_ns;AvgDelete_ns\n");

    /* benchmarks */
    for (int s = 0; s < NUM_HASH_SCENARIOS; s++) {
        printf("=== Scenario: %s ===\n", HASH_SCENARIOS[s].name);
        for (int z = 0; z < NUM_HASH_SIZES; z++) {
            int N = HASH_SIZES[z];
            printf("\n  N = %d\n", N);
            printf("  %-16s | %12s | %12s | %12s\n",
                   "Structure", "Insert ns/op", "Search ns/op", "Delete ns/op");
            printf("  %-16s-+-%12s-+-%12s-+-%12s\n",
                   "----------------", "------------", "------------", "------------");

            for (int d = 0; d < NUM_HASHES; d++) {
                srand(42);
                HashBenchResult r = HASH_SCENARIOS[s].fn(&HASHES[d], N);
                printf("  %-16s | %12.1f | %12.1f | %12.1f\n",
                       HASHES[d].name, r.ins_ns_per_op, r.srch_ns_per_op, r.del_ns_per_op);

                char c_ins[32], c_srch[32], c_del[32];
                snprintf(c_ins, sizeof(c_ins), "%.1f", r.ins_ns_per_op);
                snprintf(c_srch, sizeof(c_srch), "%.1f", r.srch_ns_per_op);
                snprintf(c_del, sizeof(c_del), "%.1f", r.del_ns_per_op);
                for (char *p = c_ins; *p; p++) if (*p == '.') *p = ',';
                for (char *p = c_srch; *p; p++) if (*p == '.') *p = ',';
                for (char *p = c_del; *p; p++) if (*p == '.') *p = ',';
                fprintf(csv, "%s;%d;%s;%s;%s;%s\n",
                        HASH_SCENARIOS[s].name, N, HASHES[d].name, c_ins, c_srch, c_del);
            }
        }
        printf("\n");
    }

    fclose(csv);
    printf("Results saved to hash_results.csv\n");
    printf("Done.\n");
    return 0;
}

#endif /* TESTER_HASH_H */
