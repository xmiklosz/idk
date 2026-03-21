/*
 * Hash Table with Double Hashing (Open Addressing)
 * Collision resolution via double hashing probe sequence.
 * Lazy deletion with tombstones. Dynamic resizing.
 * Table size is always prime for correct probing.
 */
#ifndef HD_TABLE_H
#define HD_TABLE_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#define HD_INIT_CAPACITY 17  /* prime */
#define HD_MAX_LOAD 0.7

typedef enum { HD_EMPTY = 0, HD_ACTIVE, HD_DELETED } HD_Status;

typedef struct {
    int *keys;
    HD_Status *status;
    int capacity;
    int count;     /* active entries */
    int occupied;  /* active + deleted (for load factor) */
} HD_Table;

/* --- Prime helper --- */

static bool hd_is_prime(int n) {
    if (n < 2) return false;
    if (n < 4) return true;
    if (n % 2 == 0 || n % 3 == 0) return false;
    for (int i = 5; i * i <= n; i += 6)
        if (n % i == 0 || n % (i + 2) == 0) return false;
    return true;
}

static int hd_next_prime(int n) {
    if (n <= 2) return 2;
    if (n % 2 == 0) n++;
    while (!hd_is_prime(n)) n += 2;
    return n;
}

/* --- Hash functions --- */

static int hd_hash1(int key, int capacity) {
    unsigned int k = (unsigned int)key;
    return (int)(k % (unsigned int)capacity);
}

static int hd_hash2(int key, int capacity) {
    unsigned int k = (unsigned int)key;
    return 1 + (int)(k % (unsigned int)(capacity - 1));
}

static int hd_probe(int h1, int h2, int attempt, int capacity) {
    return (h1 + attempt * h2) % capacity;
}

/* --- Create / Destroy --- */

HD_Table *hd_create(void) {
    HD_Table *t = (HD_Table *)malloc(sizeof(HD_Table));
    if (!t) { fprintf(stderr, "hd_create: alloc failed\n"); exit(1); }
    t->capacity = HD_INIT_CAPACITY;
    t->count = 0;
    t->occupied = 0;
    t->keys = (int *)calloc(t->capacity, sizeof(int));
    t->status = (HD_Status *)calloc(t->capacity, sizeof(HD_Status));
    if (!t->keys || !t->status) { fprintf(stderr, "hd_create: alloc failed\n"); exit(1); }
    return t;
}

void hd_destroy(HD_Table *t) {
    if (!t) return;
    free(t->keys);
    free(t->status);
    free(t);
}

/* --- Resize --- */

static void hd_resize(HD_Table *t) {
    int new_cap = hd_next_prime(t->capacity * 2);
    int *new_keys = (int *)calloc(new_cap, sizeof(int));
    HD_Status *new_status = (HD_Status *)calloc(new_cap, sizeof(HD_Status));
    if (!new_keys || !new_status) { fprintf(stderr, "hd_resize: alloc failed\n"); exit(1); }

    for (int i = 0; i < t->capacity; i++) {
        if (t->status[i] == HD_ACTIVE) {
            int h1 = hd_hash1(t->keys[i], new_cap);
            int h2 = hd_hash2(t->keys[i], new_cap);
            for (int a = 0; a < new_cap; a++) {
                int slot = hd_probe(h1, h2, a, new_cap);
                if (new_status[slot] == HD_EMPTY) {
                    new_keys[slot] = t->keys[i];
                    new_status[slot] = HD_ACTIVE;
                    break;
                }
            }
        }
    }
    free(t->keys);
    free(t->status);
    t->keys = new_keys;
    t->status = new_status;
    t->capacity = new_cap;
    t->occupied = t->count; /* tombstones cleared after resize */
}

/* --- Search --- */

bool hd_search(HD_Table *t, int key) {
    int h1 = hd_hash1(key, t->capacity);
    int h2 = hd_hash2(key, t->capacity);
    for (int a = 0; a < t->capacity; a++) {
        int slot = hd_probe(h1, h2, a, t->capacity);
        if (t->status[slot] == HD_EMPTY) return false;
        if (t->status[slot] == HD_ACTIVE && t->keys[slot] == key) return true;
    }
    return false;
}

/* --- Insert --- */

void hd_insert(HD_Table *t, int key) {
    if ((double)t->occupied / t->capacity >= HD_MAX_LOAD)
        hd_resize(t);

    int h1 = hd_hash1(key, t->capacity);
    int h2 = hd_hash2(key, t->capacity);
    int first_deleted = -1;

    for (int a = 0; a < t->capacity; a++) {
        int slot = hd_probe(h1, h2, a, t->capacity);
        if (t->status[slot] == HD_ACTIVE && t->keys[slot] == key)
            return; /* duplicate */
        if (t->status[slot] == HD_DELETED && first_deleted == -1)
            first_deleted = slot;
        if (t->status[slot] == HD_EMPTY) {
            int target = (first_deleted != -1) ? first_deleted : slot;
            t->keys[target] = key;
            if (t->status[target] != HD_DELETED) t->occupied++;
            t->status[target] = HD_ACTIVE;
            t->count++;
            return;
        }
    }
    /* Table is full of active/deleted entries - use first_deleted */
    if (first_deleted != -1) {
        t->keys[first_deleted] = key;
        t->status[first_deleted] = HD_ACTIVE;
        t->count++;
    }
}

/* --- Delete --- */

void hd_delete(HD_Table *t, int key) {
    int h1 = hd_hash1(key, t->capacity);
    int h2 = hd_hash2(key, t->capacity);
    for (int a = 0; a < t->capacity; a++) {
        int slot = hd_probe(h1, h2, a, t->capacity);
        if (t->status[slot] == HD_EMPTY) return;
        if (t->status[slot] == HD_ACTIVE && t->keys[slot] == key) {
            t->status[slot] = HD_DELETED;
            t->count--;
            return;
        }
    }
}

/* --- Print (for debugging) --- */

void hd_print(HD_Table *t) {
    printf("Hash Double Table (count=%d, capacity=%d, load=%.2f)\n",
           t->count, t->capacity, (double)t->occupied / t->capacity);
    for (int i = 0; i < t->capacity; i++) {
        if (t->status[i] == HD_ACTIVE)
            printf("  [%d]: %d\n", i, t->keys[i]);
        else if (t->status[i] == HD_DELETED)
            printf("  [%d]: (deleted)\n", i);
    }
}

#endif /* HD_TABLE_H */
