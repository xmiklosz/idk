/*
 * 2-3 Tree Implementation
 * Balanced search tree where each internal node has 2 or 3 children.
 * All leaves are at the same depth. Keys are unique.
 */
#ifndef TT_TREE_H
#define TT_TREE_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

typedef struct TT_Node {
    int keys[2];
    struct TT_Node *children[3];
    int n; /* number of keys: 1 (2-node) or 2 (3-node) */
} TT_Node;

typedef struct {
    TT_Node *root;
    int count; /* number of elements stored */
} TT_Tree;

/* --- Forward declarations --- */
static bool tt_fix_underflow(TT_Node *parent, int child_idx);

/* --- Create / Destroy --- */

static TT_Node *tt_new_node(int key) {
    TT_Node *nd = (TT_Node *)calloc(1, sizeof(TT_Node));
    if (!nd) { fprintf(stderr, "tt_new_node: allocation failed\n"); exit(1); }
    nd->keys[0] = key;
    nd->n = 1;
    return nd;
}

static void tt_destroy_nodes(TT_Node *nd) {
    if (!nd) return;
    for (int i = 0; i <= nd->n; i++)
        tt_destroy_nodes(nd->children[i]);
    free(nd);
}

TT_Tree *tt_create(void) {
    TT_Tree *t = (TT_Tree *)calloc(1, sizeof(TT_Tree));
    return t;
}

void tt_destroy(TT_Tree *t) {
    if (!t) return;
    tt_destroy_nodes(t->root);
    free(t);
}

/* --- Search --- */

bool tt_search(TT_Tree *t, int key) {
    TT_Node *cur = t->root;
    while (cur) {
        int i;
        for (i = 0; i < cur->n; i++) {
            if (key == cur->keys[i]) return true;
            if (key < cur->keys[i]) break;
        }
        cur = cur->children[i];
    }
    return false;
}

/* --- Insert --- */

static bool tt_is_leaf(TT_Node *nd) {
    return nd->children[0] == NULL;
}

typedef struct {
    TT_Node *new_right; /* NULL if no split occurred */
    int promoted;
} TT_Split;

static TT_Split tt_insert_rec(TT_Node *nd, int key, bool *inserted) {
    TT_Split result = {NULL, 0};

    /* Find position */
    int pos = 0;
    while (pos < nd->n && key > nd->keys[pos]) pos++;
    if (pos < nd->n && key == nd->keys[pos]) {
        *inserted = false;
        return result;
    }

    int new_key = key;
    TT_Node *new_child = NULL;

    if (!tt_is_leaf(nd)) {
        TT_Split child_split = tt_insert_rec(nd->children[pos], key, inserted);
        if (!child_split.new_right) return result;
        new_key = child_split.promoted;
        new_child = child_split.new_right;
        /* Recalculate position for promoted key */
        pos = 0;
        while (pos < nd->n && new_key > nd->keys[pos]) pos++;
    }

    /* Insert new_key into this node */
    if (nd->n < 2) {
        for (int i = nd->n; i > pos; i--) {
            nd->keys[i] = nd->keys[i - 1];
            nd->children[i + 1] = nd->children[i];
        }
        nd->keys[pos] = new_key;
        nd->children[pos + 1] = new_child;
        nd->n++;
        return result; /* no split */
    }

    /* Node is full (2 keys), need to split */
    int all_keys[3];
    TT_Node *all_ch[4];
    for (int i = 0; i < 2; i++) all_keys[i] = nd->keys[i];
    for (int i = 0; i < 3; i++) all_ch[i] = nd->children[i];

    /* Insert into temporary arrays */
    for (int i = 2; i > pos; i--) all_keys[i] = all_keys[i - 1];
    for (int i = 3; i > pos + 1; i--) all_ch[i] = all_ch[i - 1];
    all_keys[pos] = new_key;
    all_ch[pos + 1] = new_child;

    /* Left node keeps all_keys[0], right gets all_keys[2], middle promoted */
    nd->keys[0] = all_keys[0];
    nd->n = 1;
    nd->children[0] = all_ch[0];
    nd->children[1] = all_ch[1];
    nd->children[2] = NULL;

    TT_Node *right = tt_new_node(all_keys[2]);
    right->children[0] = all_ch[2];
    right->children[1] = all_ch[3];

    result.new_right = right;
    result.promoted = all_keys[1];
    return result;
}

void tt_insert(TT_Tree *t, int key) {
    if (!t->root) {
        t->root = tt_new_node(key);
        t->count = 1;
        return;
    }
    bool inserted = true;
    TT_Split split = tt_insert_rec(t->root, key, &inserted);
    if (!inserted) return;
    t->count++;
    if (split.new_right) {
        TT_Node *new_root = tt_new_node(split.promoted);
        new_root->children[0] = t->root;
        new_root->children[1] = split.new_right;
        t->root = new_root;
    }
}

/* --- Delete --- */

static bool tt_fix_underflow(TT_Node *parent, int child_idx) {
    TT_Node *child = parent->children[child_idx];
    /* child has 0 keys (underflow) */

    /* Try borrowing from left sibling */
    if (child_idx > 0) {
        TT_Node *left = parent->children[child_idx - 1];
        if (left->n == 2) {
            /* Rotate right: parent key goes to child, left's max goes to parent */
            child->keys[0] = parent->keys[child_idx - 1];
            child->n = 1;
            child->children[1] = child->children[0];
            child->children[0] = left->children[left->n];
            parent->keys[child_idx - 1] = left->keys[left->n - 1];
            left->children[left->n] = NULL;
            left->n--;
            return false;
        }
    }

    /* Try borrowing from right sibling */
    if (child_idx < parent->n) {
        TT_Node *right = parent->children[child_idx + 1];
        if (right->n == 2) {
            /* Rotate left: parent key goes to child, right's min goes to parent */
            child->keys[0] = parent->keys[child_idx];
            child->n = 1;
            child->children[1] = right->children[0];
            parent->keys[child_idx] = right->keys[0];
            right->keys[0] = right->keys[1];
            right->children[0] = right->children[1];
            right->children[1] = right->children[2];
            right->children[2] = NULL;
            right->n--;
            return false;
        }
    }

    /* Must merge */
    if (child_idx > 0) {
        /* Merge child into left sibling */
        TT_Node *left = parent->children[child_idx - 1];
        left->keys[1] = parent->keys[child_idx - 1];
        left->children[2] = child->children[0];
        left->n = 2;

        /* Remove from parent */
        for (int i = child_idx - 1; i < parent->n - 1; i++) {
            parent->keys[i] = parent->keys[i + 1];
            parent->children[i + 1] = parent->children[i + 2];
        }
        parent->children[parent->n] = NULL;
        parent->n--;

        free(child);
        return parent->n == 0; /* underflow propagates if parent is empty */
    } else {
        /* Merge right sibling into child */
        TT_Node *right = parent->children[1];
        child->keys[0] = parent->keys[0];
        child->keys[1] = right->keys[0];
        child->children[1] = right->children[0];
        child->children[2] = right->children[1];
        child->n = 2;

        /* Remove from parent */
        for (int i = 0; i < parent->n - 1; i++) {
            parent->keys[i] = parent->keys[i + 1];
            parent->children[i + 1] = parent->children[i + 2];
        }
        parent->children[parent->n] = NULL;
        parent->n--;

        free(right);
        return parent->n == 0;
    }
}

static bool tt_delete_rec(TT_Node *node, int key, bool *deleted) {
    int pos;
    bool found_here = false;

    for (pos = 0; pos < node->n; pos++) {
        if (key == node->keys[pos]) { found_here = true; break; }
        if (key < node->keys[pos]) break;
    }

    if (tt_is_leaf(node)) {
        if (!found_here) return false;
        *deleted = true;
        for (int i = pos; i < node->n - 1; i++)
            node->keys[i] = node->keys[i + 1];
        node->n--;
        return node->n == 0;
    }

    /* Internal node */
    int del_key = key;
    int child_pos = pos;

    if (found_here) {
        /* Replace with in-order predecessor (rightmost in left subtree) */
        TT_Node *pred = node->children[pos];
        while (!tt_is_leaf(pred)) pred = pred->children[pred->n];
        int pred_key = pred->keys[pred->n - 1];
        node->keys[pos] = pred_key;
        del_key = pred_key;
        *deleted = true;
        /* child_pos stays as pos - delete predecessor from left subtree */
    }

    bool child_underflow = tt_delete_rec(node->children[child_pos], del_key, deleted);
    if (!child_underflow) return false;

    return tt_fix_underflow(node, child_pos);
}

void tt_delete(TT_Tree *t, int key) {
    if (!t->root) return;

    bool deleted = false;
    bool underflow = tt_delete_rec(t->root, key, &deleted);
    if (!deleted) return;
    t->count--;

    if (underflow) {
        TT_Node *old = t->root;
        t->root = old->children[0]; /* may be NULL if tree is now empty */
        free(old);
    }
}

/* --- Print (preorder for debugging) --- */

void tt_print(TT_Node *nd) {
    if (!nd) return;
    printf("[");
    for (int i = 0; i < nd->n; i++) {
        if (i > 0) printf(",");
        printf("%d", nd->keys[i]);
    }
    printf("] ");
    for (int i = 0; i <= nd->n; i++)
        tt_print(nd->children[i]);
}

#endif /* TT_TREE_H */
