#ifndef TEST_H
#define TEST_H

#include <time.h>
#include <math.h>
#include <stdio.h>

/* Hash table constants */
#define NULL_ENTRY    (-2147483648)  /* INT_MIN – marks an empty slot */
#define REMOVED_ENTRY (-2147483647) /* marks a deleted slot */
#define MAX_FILL_RATE 0.7           /* resize when load factor >= 70 % */
#define STARTING_SIZE 11            /* initial hash-table capacity */

/*
 * TIMED_LOG_N – used by the 2-3 tree.
 * tree_ptr must have a field  int nodeCount.
 * CSV row:  Nodes;LogN;Actual
 */
#define TIMED_LOG_N(op, val, expr, tree_ptr, csv) \
    do { \
        struct timespec _ts0, _ts1; \
        clock_gettime(CLOCK_MONOTONIC, &_ts0); \
        expr; \
        clock_gettime(CLOCK_MONOTONIC, &_ts1); \
        double _ns = (_ts1.tv_sec - _ts0.tv_sec) * 1e9 \
                   + (_ts1.tv_nsec - _ts0.tv_nsec); \
        int    _n  = (tree_ptr)->nodeCount; \
        double _lg = (_n > 1) ? log2((double)_n) : 0.0; \
        fprintf((csv), "%d;%.4f;%.2f\n", _n, _lg, _ns); \
        printf("%c%d", (op), (val)); \
    } while (0)

/*
 * TIMED_LOG_OP – used by the hash table.
 * table must have fields  int numEntries, int maxSize.
 * CSV row:  CurrentNodes;Time;Op;LoadFactor;LoadFactorThreshold
 */
#define TIMED_LOG_OP(op, expr, table, csv) \
    do { \
        struct timespec _ts0, _ts1; \
        clock_gettime(CLOCK_MONOTONIC, &_ts0); \
        expr; \
        clock_gettime(CLOCK_MONOTONIC, &_ts1); \
        double _ns = (_ts1.tv_sec - _ts0.tv_sec) * 1e9 \
                   + (_ts1.tv_nsec - _ts0.tv_nsec); \
        double _lf = (double)(table)->numEntries / (double)(table)->maxSize; \
        fprintf((csv), "%d;%.2f;%c;%.4f;%.2f\n", \
                (table)->numEntries, _ns, (op), _lf, (double)MAX_FILL_RATE); \
    } while (0)

/*
 * TIMED_LOG_N_CSV – used by the Red-Black tree.
 * tree_ptr must have a field  int count.
 * CSV row:  Nodes;LogN;Actual
 */
#define TIMED_LOG_N_CSV(op, val, block, tree_ptr, csv) \
    do { \
        struct timespec _ts0, _ts1; \
        clock_gettime(CLOCK_MONOTONIC, &_ts0); \
        block \
        clock_gettime(CLOCK_MONOTONIC, &_ts1); \
        double _ns = (_ts1.tv_sec - _ts0.tv_sec) * 1e9 \
                   + (_ts1.tv_nsec - _ts0.tv_nsec); \
        int    _n  = (tree_ptr)->count; \
        double _lg = (_n > 1) ? log2((double)_n) : 0.0; \
        fprintf((csv), "%d;%.4f;%.2f\n", _n, _lg, _ns); \
    } while (0)

#endif /* TEST_H */
