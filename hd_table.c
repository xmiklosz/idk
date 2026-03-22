/*
 * Hash Table with Double Hashing (Open Addressing)
 * Collision resolution via double hashing probe sequence.
 * Lazy deletion using tombstones. Dynamic resizing.
 * Table capacity is always prime for correct probing.
 *
 * Operations: create, destroy, insert, search, delete, print
 */
#ifndef HD_TABLE_C
#define HD_TABLE_C

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#define HD_INITIAL_CAPACITY 17   /* starting prime */
#define HD_LOAD_THRESHOLD 0.7

/* ========== Data Structures ========== */

typedef enum { SLOT_EMPTY = 0, SLOT_ACTIVE, SLOT_DELETED } HD_SlotStatus;

typedef struct {
    int *keys;
    HD_SlotStatus *status;
    int capacity;
    int size;       /* number of active entries */
    int occupied;   /* active + deleted (used for load factor) */
} HD_Table;

/* ========== Prime Helpers ========== */

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

/* ========== Hash Functions ========== */

static int hd_primary_hash(int key, int capacity) {
    unsigned int k = (unsigned int)key;
    return (int)(k % (unsigned int)capacity);
}

static int hd_secondary_hash(int key, int capacity) {
    unsigned int k = (unsigned int)key;
    /* returns value in [1, capacity-1], never 0 */
    return 1 + (int)(k % (unsigned int)(capacity - 1));
}

static int hd_probe_slot(int h1, int h2, int attempt, int capacity) {
    return ((unsigned int)h1 + (unsigned int)attempt * (unsigned int)h2) % (unsigned int)capacity;
}

/* ========== Create / Destroy ========== */

HD_Table *hd_create(void) {
    HD_Table *table = (HD_Table *)malloc(sizeof(HD_Table));
    if (!table) {
        fprintf(stderr, "hd_create: memory allocation error\n");
        exit(EXIT_FAILURE);
    }
    table->capacity = HD_INITIAL_CAPACITY;
    table->size = 0;
    table->occupied = 0;
    table->keys = (int *)calloc(table->capacity, sizeof(int));
    table->status = (HD_SlotStatus *)calloc(table->capacity, sizeof(HD_SlotStatus));
    if (!table->keys || !table->status) {
        fprintf(stderr, "hd_create: memory allocation error\n");
        exit(EXIT_FAILURE);
    }
    return table;
}

void hd_destroy(HD_Table *table) {
    if (!table) return;
    free(table->keys);
    free(table->status);
    free(table);
}

/* ========== Resize ========== */

static void hd_grow(HD_Table *table) {
    int new_cap = hd_next_prime(table->capacity * 2);
    int *new_keys = (int *)calloc(new_cap, sizeof(int));
    HD_SlotStatus *new_status = (HD_SlotStatus *)calloc(new_cap, sizeof(HD_SlotStatus));
    if (!new_keys || !new_status) {
        fprintf(stderr, "hd_grow: memory allocation error\n");
        exit(EXIT_FAILURE);
    }

    /* rehash only active entries (tombstones are discarded) */
    for (int i = 0; i < table->capacity; i++) {
        if (table->status[i] != SLOT_ACTIVE) continue;
        int h1 = hd_primary_hash(table->keys[i], new_cap);
        int h2 = hd_secondary_hash(table->keys[i], new_cap);
        for (int a = 0; a < new_cap; a++) {
            int slot = hd_probe_slot(h1, h2, a, new_cap);
            if (new_status[slot] == SLOT_EMPTY) {
                new_keys[slot] = table->keys[i];
                new_status[slot] = SLOT_ACTIVE;
                break;
            }
        }
    }

    free(table->keys);
    free(table->status);
    table->keys = new_keys;
    table->status = new_status;
    table->capacity = new_cap;
    table->occupied = table->size;  /* tombstones are gone */
}

/* ========== Search ========== */

bool hd_search(HD_Table *table, int key) {
    int h1 = hd_primary_hash(key, table->capacity);
    int h2 = hd_secondary_hash(key, table->capacity);
    for (int a = 0; a < table->capacity; a++) {
        int slot = hd_probe_slot(h1, h2, a, table->capacity);
        if (table->status[slot] == SLOT_EMPTY)
            return false;
        if (table->status[slot] == SLOT_ACTIVE && table->keys[slot] == key)
            return true;
        /* SLOT_DELETED -> keep probing */
    }
    return false;
}

/* ========== Insert ========== */

void hd_insert(HD_Table *table, int key) {
    if ((double)table->occupied / table->capacity >= HD_LOAD_THRESHOLD)
        hd_grow(table);

    int h1 = hd_primary_hash(key, table->capacity);
    int h2 = hd_secondary_hash(key, table->capacity);
    int first_tombstone = -1;

    for (int a = 0; a < table->capacity; a++) {
        int slot = hd_probe_slot(h1, h2, a, table->capacity);

        if (table->status[slot] == SLOT_ACTIVE && table->keys[slot] == key)
            return;  /* duplicate, do nothing */

        if (table->status[slot] == SLOT_DELETED && first_tombstone == -1)
            first_tombstone = slot;

        if (table->status[slot] == SLOT_EMPTY) {
            int target = (first_tombstone != -1) ? first_tombstone : slot;
            table->keys[target] = key;
            if (table->status[target] != SLOT_DELETED)
                table->occupied++;
            table->status[target] = SLOT_ACTIVE;
            table->size++;
            return;
        }
    }

    /* all slots are active or deleted - use first tombstone */
    if (first_tombstone != -1) {
        table->keys[first_tombstone] = key;
        table->status[first_tombstone] = SLOT_ACTIVE;
        table->size++;
    }
}

/* ========== Delete ========== */

void hd_delete(HD_Table *table, int key) {
    int h1 = hd_primary_hash(key, table->capacity);
    int h2 = hd_secondary_hash(key, table->capacity);
    for (int a = 0; a < table->capacity; a++) {
        int slot = hd_probe_slot(h1, h2, a, table->capacity);
        if (table->status[slot] == SLOT_EMPTY)
            return;  /* not found */
        if (table->status[slot] == SLOT_ACTIVE && table->keys[slot] == key) {
            table->status[slot] = SLOT_DELETED;
            table->size--;
            return;
        }
    }
}

/* ========== Print (for debugging) ========== */

void hd_print(HD_Table *table) {
    printf("Hash Double Table (size=%d, capacity=%d, load=%.2f)\n",
           table->size, table->capacity, (double)table->occupied / table->capacity);
    for (int i = 0; i < table->capacity; i++) {
        if (table->status[i] == SLOT_ACTIVE)
            printf("  [%d]: %d\n", i, table->keys[i]);
        else if (table->status[i] == SLOT_DELETED)
            printf("  [%d]: (tombstone)\n", i);
    }
}

#endif /* HD_TABLE_C */
