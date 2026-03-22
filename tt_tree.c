/*
 * 2-3 Tree Implementation
 * Balanced search tree where each internal node has 2 or 3 children.
 * All leaves are at the same depth. Keys are unique.
 *
 * Operations: create, destroy, insert, search, delete, print
 */
#ifndef TT_TREE_C
#define TT_TREE_C

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

/* ========== Data Structures ========== */

typedef struct TT_Node {
    int keys[2];                  /* max 2 keys per node */
    struct TT_Node *children[3]; /* max 3 children */
    int num_keys;                /* 1 for 2-node, 2 for 3-node */
} TT_Node;

typedef struct {
    TT_Node *root;
    int size;   /* total number of stored elements */
} TT_Tree;

/* result of recursive insert - carries split info upward */
typedef struct {
    TT_Node *split_right; /* NULL means no split happened */
    int median;           /* key promoted to parent on split */
} TT_SplitResult;

/* ========== Helper Functions ========== */

static TT_Node *tt_alloc_node(int key) {
    TT_Node *node = (TT_Node *)calloc(1, sizeof(TT_Node));
    if (!node) {
        fprintf(stderr, "tt_alloc_node: memory allocation error\n");
        exit(EXIT_FAILURE);
    }
    node->keys[0] = key;
    node->num_keys = 1;
    return node;
}

static bool tt_is_leaf(TT_Node *node) {
    return node->children[0] == NULL;
}

static void tt_free_subtree(TT_Node *node) {
    if (!node) return;
    for (int i = 0; i <= node->num_keys; i++)
        tt_free_subtree(node->children[i]);
    free(node);
}

/* ========== Create / Destroy ========== */

TT_Tree *tt_create(void) {
    TT_Tree *tree = (TT_Tree *)calloc(1, sizeof(TT_Tree));
    if (!tree) {
        fprintf(stderr, "tt_create: memory allocation error\n");
        exit(EXIT_FAILURE);
    }
    return tree;
}

void tt_destroy(TT_Tree *tree) {
    if (!tree) return;
    tt_free_subtree(tree->root);
    free(tree);
}

/* ========== Search ========== */

bool tt_search(TT_Tree *tree, int key) {
    TT_Node *cur = tree->root;
    while (cur) {
        int i = 0;
        while (i < cur->num_keys) {
            if (key == cur->keys[i]) return true;
            if (key < cur->keys[i]) break;
            i++;
        }
        cur = cur->children[i];
    }
    return false;
}

/* ========== Insert ========== */

static TT_SplitResult tt_insert_recursive(TT_Node *node, int key, bool *was_inserted) {
    TT_SplitResult result = {NULL, 0};

    /* locate position for key in this node */
    int pos = 0;
    while (pos < node->num_keys && key > node->keys[pos]) pos++;

    /* duplicate check */
    if (pos < node->num_keys && key == node->keys[pos]) {
        *was_inserted = false;
        return result;
    }

    int ins_key = key;
    TT_Node *ins_child = NULL;

    /* if not leaf, recurse into the appropriate child */
    if (!tt_is_leaf(node)) {
        TT_SplitResult child_result = tt_insert_recursive(node->children[pos], key, was_inserted);
        if (!child_result.split_right)
            return result;  /* child absorbed it, no split to handle */
        ins_key = child_result.median;
        ins_child = child_result.split_right;
        /* recalculate position for the promoted key */
        pos = 0;
        while (pos < node->num_keys && ins_key > node->keys[pos]) pos++;
    }

    /* room in this node (it is a 2-node) */
    if (node->num_keys < 2) {
        /* shift keys and children right to make room at pos */
        for (int i = node->num_keys; i > pos; i--) {
            node->keys[i] = node->keys[i - 1];
            node->children[i + 1] = node->children[i];
        }
        node->keys[pos] = ins_key;
        node->children[pos + 1] = ins_child;
        node->num_keys++;
        return result;  /* no split needed */
    }

    /* node is full (3-node with 2 keys) -> must split */
    int tmp_keys[3];
    TT_Node *tmp_ch[4];

    /* copy existing data */
    tmp_keys[0] = node->keys[0];
    tmp_keys[1] = node->keys[1];
    tmp_ch[0] = node->children[0];
    tmp_ch[1] = node->children[1];
    tmp_ch[2] = node->children[2];

    /* insert new key+child into temporary sorted position */
    for (int i = 2; i > pos; i--) tmp_keys[i] = tmp_keys[i - 1];
    for (int i = 3; i > pos + 1; i--) tmp_ch[i] = tmp_ch[i - 1];
    tmp_keys[pos] = ins_key;
    tmp_ch[pos + 1] = ins_child;

    /* left node keeps tmp_keys[0] */
    node->keys[0] = tmp_keys[0];
    node->num_keys = 1;
    node->children[0] = tmp_ch[0];
    node->children[1] = tmp_ch[1];
    node->children[2] = NULL;

    /* right node gets tmp_keys[2] */
    TT_Node *right = tt_alloc_node(tmp_keys[2]);
    right->children[0] = tmp_ch[2];
    right->children[1] = tmp_ch[3];

    /* middle key is promoted */
    result.split_right = right;
    result.median = tmp_keys[1];
    return result;
}

void tt_insert(TT_Tree *tree, int key) {
    if (!tree->root) {
        tree->root = tt_alloc_node(key);
        tree->size = 1;
        return;
    }

    bool was_inserted = true;
    TT_SplitResult split = tt_insert_recursive(tree->root, key, &was_inserted);

    if (!was_inserted) return;  /* duplicate */
    tree->size++;

    if (split.split_right) {
        /* root was split, create new root */
        TT_Node *new_root = tt_alloc_node(split.median);
        new_root->children[0] = tree->root;
        new_root->children[1] = split.split_right;
        tree->root = new_root;
    }
}

/* ========== Delete ========== */

/*
 * Fix underflow in parent->children[idx].
 * Returns true if parent itself underflows after the fix.
 */
static bool tt_fix_underflow(TT_Node *parent, int idx) {
    TT_Node *child = parent->children[idx];

    /* try to borrow from left sibling */
    if (idx > 0) {
        TT_Node *left_sib = parent->children[idx - 1];
        if (left_sib->num_keys == 2) {
            /* rotate right through parent */
            child->keys[0] = parent->keys[idx - 1];
            child->num_keys = 1;
            child->children[1] = child->children[0];
            child->children[0] = left_sib->children[left_sib->num_keys];
            parent->keys[idx - 1] = left_sib->keys[left_sib->num_keys - 1];
            left_sib->children[left_sib->num_keys] = NULL;
            left_sib->num_keys--;
            return false;
        }
    }

    /* try to borrow from right sibling */
    if (idx < parent->num_keys) {
        TT_Node *right_sib = parent->children[idx + 1];
        if (right_sib->num_keys == 2) {
            /* rotate left through parent */
            child->keys[0] = parent->keys[idx];
            child->num_keys = 1;
            child->children[1] = right_sib->children[0];
            parent->keys[idx] = right_sib->keys[0];
            /* shift right sibling left */
            right_sib->keys[0] = right_sib->keys[1];
            right_sib->children[0] = right_sib->children[1];
            right_sib->children[1] = right_sib->children[2];
            right_sib->children[2] = NULL;
            right_sib->num_keys--;
            return false;
        }
    }

    /* cannot borrow, must merge */
    if (idx > 0) {
        /* merge child into left sibling */
        TT_Node *left_sib = parent->children[idx - 1];
        left_sib->keys[1] = parent->keys[idx - 1];
        left_sib->children[2] = child->children[0];
        left_sib->num_keys = 2;

        /* remove separator from parent */
        for (int i = idx - 1; i < parent->num_keys - 1; i++) {
            parent->keys[i] = parent->keys[i + 1];
            parent->children[i + 1] = parent->children[i + 2];
        }
        parent->children[parent->num_keys] = NULL;
        parent->num_keys--;
        free(child);
        return parent->num_keys == 0;
    } else {
        /* merge right sibling into child */
        TT_Node *right_sib = parent->children[1];
        child->keys[0] = parent->keys[0];
        child->keys[1] = right_sib->keys[0];
        child->children[1] = right_sib->children[0];
        child->children[2] = right_sib->children[1];
        child->num_keys = 2;

        /* remove separator from parent */
        for (int i = 0; i < parent->num_keys - 1; i++) {
            parent->keys[i] = parent->keys[i + 1];
            parent->children[i + 1] = parent->children[i + 2];
        }
        parent->children[parent->num_keys] = NULL;
        parent->num_keys--;
        free(right_sib);
        return parent->num_keys == 0;
    }
}

/*
 * Recursive delete. Returns true if node underflows.
 */
static bool tt_delete_recursive(TT_Node *node, int key, bool *was_deleted) {
    int pos;
    bool found = false;

    for (pos = 0; pos < node->num_keys; pos++) {
        if (key == node->keys[pos]) { found = true; break; }
        if (key < node->keys[pos]) break;
    }

    if (tt_is_leaf(node)) {
        if (!found) return false;
        *was_deleted = true;
        /* remove key by shifting left */
        for (int i = pos; i < node->num_keys - 1; i++)
            node->keys[i] = node->keys[i + 1];
        node->num_keys--;
        return node->num_keys == 0;  /* underflow if leaf becomes empty */
    }

    /* internal node */
    int target_key = key;
    int child_idx = pos;

    if (found) {
        /* swap with in-order predecessor (rightmost key in left subtree) */
        TT_Node *pred = node->children[pos];
        while (!tt_is_leaf(pred))
            pred = pred->children[pred->num_keys];
        int pred_key = pred->keys[pred->num_keys - 1];
        node->keys[pos] = pred_key;
        target_key = pred_key;
        *was_deleted = true;
    }

    bool child_underflow = tt_delete_recursive(node->children[child_idx], target_key, was_deleted);
    if (!child_underflow) return false;

    return tt_fix_underflow(node, child_idx);
}

void tt_delete(TT_Tree *tree, int key) {
    if (!tree->root) return;

    bool was_deleted = false;
    bool underflow = tt_delete_recursive(tree->root, key, &was_deleted);

    if (!was_deleted) return;
    tree->size--;

    if (underflow) {
        /* root became empty, its only child becomes new root */
        TT_Node *old_root = tree->root;
        tree->root = old_root->children[0];
        free(old_root);
    }
}

/* ========== Print (preorder, for debugging) ========== */

void tt_print_node(TT_Node *node) {
    if (!node) return;
    printf("[");
    for (int i = 0; i < node->num_keys; i++) {
        if (i > 0) printf(",");
        printf("%d", node->keys[i]);
    }
    printf("] ");
    for (int i = 0; i <= node->num_keys; i++)
        tt_print_node(node->children[i]);
}

#endif /* TT_TREE_C */
