/*
 * Hash Table with Chaining (Separate Chaining)
 * Collision resolution via linked lists at each bucket.
 * Dynamic resizing when load factor exceeds threshold.
 */
#ifndef HC_TABLE_H
#define HC_TABLE_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#define HC_INIT_CAPACITY 16
#define HC_MAX_LOAD 0.75

typedef struct HC_Node {
    int key;
    struct HC_Node *next;
} HC_Node;

typedef struct {
    HC_Node **buckets;
    int capacity;
    int count;
} HC_Table;

/* --- Create / Destroy --- */

HC_Table *hc_create(void) {
    HC_Table *t = (HC_Table *)malloc(sizeof(HC_Table));
    if (!t) { fprintf(stderr, "hc_create: alloc failed\n"); exit(1); }
    t->capacity = HC_INIT_CAPACITY;
    t->count = 0;
    t->buckets = (HC_Node **)calloc(t->capacity, sizeof(HC_Node *));
    if (!t->buckets) { fprintf(stderr, "hc_create: alloc failed\n"); exit(1); }
    return t;
}

void hc_destroy(HC_Table *t) {
    if (!t) return;
    for (int i = 0; i < t->capacity; i++) {
        HC_Node *cur = t->buckets[i];
        while (cur) {
            HC_Node *next = cur->next;
            free(cur);
            cur = next;
        }
    }
    free(t->buckets);
    free(t);
}

/* --- Hash function --- */

static int hc_hash(int key, int capacity) {
    unsigned int k = (unsigned int)key;
    return (int)(k % (unsigned int)capacity);
}

/* --- Resize --- */

static void hc_resize(HC_Table *t) {
    int new_cap = t->capacity * 2;
    HC_Node **new_buckets = (HC_Node **)calloc(new_cap, sizeof(HC_Node *));
    if (!new_buckets) { fprintf(stderr, "hc_resize: alloc failed\n"); exit(1); }

    for (int i = 0; i < t->capacity; i++) {
        HC_Node *cur = t->buckets[i];
        while (cur) {
            HC_Node *next = cur->next;
            int idx = hc_hash(cur->key, new_cap);
            cur->next = new_buckets[idx];
            new_buckets[idx] = cur;
            cur = next;
        }
    }
    free(t->buckets);
    t->buckets = new_buckets;
    t->capacity = new_cap;
}

/* --- Search --- */

bool hc_search(HC_Table *t, int key) {
    int idx = hc_hash(key, t->capacity);
    HC_Node *cur = t->buckets[idx];
    while (cur) {
        if (cur->key == key) return true;
        cur = cur->next;
    }
    return false;
}

/* --- Insert --- */

void hc_insert(HC_Table *t, int key) {
    if ((double)t->count / t->capacity >= HC_MAX_LOAD)
        hc_resize(t);

    int idx = hc_hash(key, t->capacity);

    /* Check for duplicate */
    HC_Node *cur = t->buckets[idx];
    while (cur) {
        if (cur->key == key) return;
        cur = cur->next;
    }

    /* Prepend new node */
    HC_Node *nd = (HC_Node *)malloc(sizeof(HC_Node));
    if (!nd) { fprintf(stderr, "hc_insert: alloc failed\n"); exit(1); }
    nd->key = key;
    nd->next = t->buckets[idx];
    t->buckets[idx] = nd;
    t->count++;
}

/* --- Delete --- */

void hc_delete(HC_Table *t, int key) {
    int idx = hc_hash(key, t->capacity);
    HC_Node *cur = t->buckets[idx];
    HC_Node *prev = NULL;

    while (cur) {
        if (cur->key == key) {
            if (prev) prev->next = cur->next;
            else t->buckets[idx] = cur->next;
            free(cur);
            t->count--;
            return;
        }
        prev = cur;
        cur = cur->next;
    }
}

/* --- Print (for debugging) --- */

void hc_print(HC_Table *t) {
    printf("Hash Chain Table (count=%d, capacity=%d, load=%.2f)\n",
           t->count, t->capacity, (double)t->count / t->capacity);
    for (int i = 0; i < t->capacity; i++) {
        if (!t->buckets[i]) continue;
        printf("  [%d]:", i);
        HC_Node *cur = t->buckets[i];
        while (cur) {
            printf(" %d", cur->key);
            cur = cur->next;
        }
        printf("\n");
    }
}

#endif /* HC_TABLE_H */
