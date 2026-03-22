/*
 * Hash Table with Separate Chaining
 * Collision resolution via linked lists at each bucket.
 * Dynamic resizing when load factor exceeds threshold.
 *
 * Operations: create, destroy, insert, search, delete, print
 */
#ifndef HC_TABLE_C
#define HC_TABLE_C

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#define HC_INITIAL_BUCKETS 16
#define HC_LOAD_THRESHOLD 0.75

/* ========== Data Structures ========== */

typedef struct HC_Entry {
    int key;
    struct HC_Entry *next;
} HC_Entry;

typedef struct {
    HC_Entry **buckets;
    int capacity;
    int size;
} HC_Table;

/* ========== Hash Function ========== */

static int hc_compute_hash(int key, int capacity) {
    unsigned int k = (unsigned int)key;
    return (int)(k % (unsigned int)capacity);
}

/* ========== Create / Destroy ========== */

HC_Table *hc_create(void) {
    HC_Table *table = (HC_Table *)malloc(sizeof(HC_Table));
    if (!table) {
        fprintf(stderr, "hc_create: memory allocation error\n");
        exit(EXIT_FAILURE);
    }
    table->capacity = HC_INITIAL_BUCKETS;
    table->size = 0;
    table->buckets = (HC_Entry **)calloc(table->capacity, sizeof(HC_Entry *));
    if (!table->buckets) {
        fprintf(stderr, "hc_create: memory allocation error\n");
        exit(EXIT_FAILURE);
    }
    return table;
}

void hc_destroy(HC_Table *table) {
    if (!table) return;
    for (int i = 0; i < table->capacity; i++) {
        HC_Entry *entry = table->buckets[i];
        while (entry) {
            HC_Entry *next = entry->next;
            free(entry);
            entry = next;
        }
    }
    free(table->buckets);
    free(table);
}

/* ========== Resize ========== */

static void hc_grow(HC_Table *table) {
    int new_cap = table->capacity * 2;
    HC_Entry **new_buckets = (HC_Entry **)calloc(new_cap, sizeof(HC_Entry *));
    if (!new_buckets) {
        fprintf(stderr, "hc_grow: memory allocation error\n");
        exit(EXIT_FAILURE);
    }

    /* rehash all entries into new bucket array */
    for (int i = 0; i < table->capacity; i++) {
        HC_Entry *entry = table->buckets[i];
        while (entry) {
            HC_Entry *next = entry->next;
            int new_idx = hc_compute_hash(entry->key, new_cap);
            entry->next = new_buckets[new_idx];
            new_buckets[new_idx] = entry;
            entry = next;
        }
    }
    free(table->buckets);
    table->buckets = new_buckets;
    table->capacity = new_cap;
}

/* ========== Search ========== */

bool hc_search(HC_Table *table, int key) {
    int idx = hc_compute_hash(key, table->capacity);
    HC_Entry *entry = table->buckets[idx];
    while (entry) {
        if (entry->key == key) return true;
        entry = entry->next;
    }
    return false;
}

/* ========== Insert ========== */

void hc_insert(HC_Table *table, int key) {
    /* check load factor and resize if needed */
    if ((double)table->size / table->capacity >= HC_LOAD_THRESHOLD)
        hc_grow(table);

    int idx = hc_compute_hash(key, table->capacity);

    /* check for duplicate key */
    HC_Entry *entry = table->buckets[idx];
    while (entry) {
        if (entry->key == key) return;  /* already exists */
        entry = entry->next;
    }

    /* prepend new entry to the chain */
    HC_Entry *new_entry = (HC_Entry *)malloc(sizeof(HC_Entry));
    if (!new_entry) {
        fprintf(stderr, "hc_insert: memory allocation error\n");
        exit(EXIT_FAILURE);
    }
    new_entry->key = key;
    new_entry->next = table->buckets[idx];
    table->buckets[idx] = new_entry;
    table->size++;
}

/* ========== Delete ========== */

void hc_delete(HC_Table *table, int key) {
    int idx = hc_compute_hash(key, table->capacity);
    HC_Entry *entry = table->buckets[idx];
    HC_Entry *prev = NULL;

    while (entry) {
        if (entry->key == key) {
            if (prev)
                prev->next = entry->next;
            else
                table->buckets[idx] = entry->next;
            free(entry);
            table->size--;
            return;
        }
        prev = entry;
        entry = entry->next;
    }
}

/* ========== Print (for debugging) ========== */

void hc_print(HC_Table *table) {
    printf("Hash Chain Table (size=%d, capacity=%d, load=%.2f)\n",
           table->size, table->capacity, (double)table->size / table->capacity);
    for (int i = 0; i < table->capacity; i++) {
        if (!table->buckets[i]) continue;
        printf("  [%d]:", i);
        HC_Entry *entry = table->buckets[i];
        while (entry) {
            printf(" %d", entry->key);
            entry = entry->next;
        }
        printf("\n");
    }
}

#endif /* HC_TABLE_C */
