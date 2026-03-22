/*
 * Red-Black Tree Implementation (NULL-based, no sentinel)
 * Self-balancing BST with O(log n) insert, search, delete.
 *
 * NULL pointers represent leaf nodes and are treated as BLACK.
 *
 * Properties:
 * 1. Every node is red or black
 * 2. Root is black
 * 3. NULL leaves are black
 * 4. Red node has only black children
 * 5. All paths from a node to descendant NULLs have equal black count
 */
#ifndef RB_TREE_C
#define RB_TREE_C

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

/* ========== Data Structures ========== */

typedef enum { RED, BLACK } RB_Color;

typedef struct RB_Node {
    int key;
    RB_Color color;
    struct RB_Node *left, *right, *parent;
} RB_Node;

typedef struct {
    RB_Node *root;
    int size;
} RB_Tree;

/* ========== Color helper (NULL is BLACK) ========== */

static RB_Color rb_node_color(RB_Node *node) {
    if (!node) return BLACK;
    return node->color;
}

/* ========== Create / Destroy ========== */

RB_Tree *rb_create(void) {
    RB_Tree *tree = (RB_Tree *)malloc(sizeof(RB_Tree));
    if (!tree) {
        fprintf(stderr, "rb_create: memory allocation error\n");
        exit(EXIT_FAILURE);
    }
    tree->root = NULL;
    tree->size = 0;
    return tree;
}

static void rb_free_subtree(RB_Node *node) {
    if (!node) return;
    rb_free_subtree(node->left);
    rb_free_subtree(node->right);
    free(node);
}

void rb_destroy(RB_Tree *tree) {
    if (!tree) return;
    rb_free_subtree(tree->root);
    free(tree);
}

/* ========== Rotations ========== */

static void rb_left_rotate(RB_Tree *tree, RB_Node *x) {
    RB_Node *y = x->right;
    x->right = y->left;
    if (y->left)
        y->left->parent = x;
    y->parent = x->parent;
    if (!x->parent)
        tree->root = y;
    else if (x == x->parent->left)
        x->parent->left = y;
    else
        x->parent->right = y;
    y->left = x;
    x->parent = y;
}

static void rb_right_rotate(RB_Tree *tree, RB_Node *y) {
    RB_Node *x = y->left;
    y->left = x->right;
    if (x->right)
        x->right->parent = y;
    x->parent = y->parent;
    if (!y->parent)
        tree->root = x;
    else if (y == y->parent->right)
        y->parent->right = x;
    else
        y->parent->left = x;
    x->right = y;
    y->parent = x;
}

/* ========== Search ========== */

static RB_Node *rb_find_node(RB_Tree *tree, int key) {
    RB_Node *cur = tree->root;
    while (cur) {
        if (key == cur->key) return cur;
        if (key < cur->key)
            cur = cur->left;
        else
            cur = cur->right;
    }
    return NULL;
}

bool rb_search(RB_Tree *tree, int key) {
    return rb_find_node(tree, key) != NULL;
}

/* ========== Insert ========== */

static void rb_insert_fixup(RB_Tree *tree, RB_Node *z) {
    while (z->parent && z->parent->color == RED) {
        RB_Node *grandparent = z->parent->parent;
        if (z->parent == grandparent->left) {
            RB_Node *uncle = grandparent->right;
            if (rb_node_color(uncle) == RED) {
                /* Case 1: uncle is red - recolor */
                z->parent->color = BLACK;
                uncle->color = BLACK;
                grandparent->color = RED;
                z = grandparent;
            } else {
                if (z == z->parent->right) {
                    /* Case 2: uncle black, z is right child */
                    z = z->parent;
                    rb_left_rotate(tree, z);
                }
                /* Case 3: uncle black, z is left child */
                z->parent->color = BLACK;
                z->parent->parent->color = RED;
                rb_right_rotate(tree, z->parent->parent);
            }
        } else {
            /* mirror: parent is right child of grandparent */
            RB_Node *uncle = grandparent->left;
            if (rb_node_color(uncle) == RED) {
                z->parent->color = BLACK;
                uncle->color = BLACK;
                grandparent->color = RED;
                z = grandparent;
            } else {
                if (z == z->parent->left) {
                    z = z->parent;
                    rb_right_rotate(tree, z);
                }
                z->parent->color = BLACK;
                z->parent->parent->color = RED;
                rb_left_rotate(tree, z->parent->parent);
            }
        }
    }
    tree->root->color = BLACK;
}

void rb_insert(RB_Tree *tree, int key) {
    /* reject duplicates */
    if (rb_find_node(tree, key)) return;

    RB_Node *z = (RB_Node *)malloc(sizeof(RB_Node));
    if (!z) {
        fprintf(stderr, "rb_insert: memory allocation error\n");
        exit(EXIT_FAILURE);
    }
    z->key = key;
    z->color = RED;
    z->left = z->right = NULL;

    /* standard BST insert */
    RB_Node *parent = NULL;
    RB_Node *cur = tree->root;
    while (cur) {
        parent = cur;
        if (key < cur->key)
            cur = cur->left;
        else
            cur = cur->right;
    }
    z->parent = parent;
    if (!parent)
        tree->root = z;
    else if (key < parent->key)
        parent->left = z;
    else
        parent->right = z;

    tree->size++;
    rb_insert_fixup(tree, z);
}

/* ========== Delete ========== */

static void rb_transplant(RB_Tree *tree, RB_Node *u, RB_Node *v) {
    if (!u->parent)
        tree->root = v;
    else if (u == u->parent->left)
        u->parent->left = v;
    else
        u->parent->right = v;
    if (v)
        v->parent = u->parent;
}

static RB_Node *rb_tree_minimum(RB_Node *node) {
    while (node->left)
        node = node->left;
    return node;
}

/*
 * Delete fixup with NULL-safe approach.
 * x_parent is tracked separately because x itself may be NULL.
 */
static void rb_delete_fixup(RB_Tree *tree, RB_Node *x, RB_Node *x_parent) {
    while (x != tree->root && rb_node_color(x) == BLACK) {
        if (x == x_parent->left) {
            RB_Node *w = x_parent->right;
            if (rb_node_color(w) == RED) {
                /* Case 1 */
                w->color = BLACK;
                x_parent->color = RED;
                rb_left_rotate(tree, x_parent);
                w = x_parent->right;
            }
            if (rb_node_color(w->left) == BLACK && rb_node_color(w->right) == BLACK) {
                /* Case 2 */
                w->color = RED;
                x = x_parent;
                x_parent = x->parent;
            } else {
                if (rb_node_color(w->right) == BLACK) {
                    /* Case 3 */
                    if (w->left) w->left->color = BLACK;
                    w->color = RED;
                    rb_right_rotate(tree, w);
                    w = x_parent->right;
                }
                /* Case 4 */
                w->color = x_parent->color;
                x_parent->color = BLACK;
                if (w->right) w->right->color = BLACK;
                rb_left_rotate(tree, x_parent);
                x = tree->root;
            }
        } else {
            /* mirror */
            RB_Node *w = x_parent->left;
            if (rb_node_color(w) == RED) {
                w->color = BLACK;
                x_parent->color = RED;
                rb_right_rotate(tree, x_parent);
                w = x_parent->left;
            }
            if (rb_node_color(w->right) == BLACK && rb_node_color(w->left) == BLACK) {
                w->color = RED;
                x = x_parent;
                x_parent = x->parent;
            } else {
                if (rb_node_color(w->left) == BLACK) {
                    if (w->right) w->right->color = BLACK;
                    w->color = RED;
                    rb_left_rotate(tree, w);
                    w = x_parent->left;
                }
                w->color = x_parent->color;
                x_parent->color = BLACK;
                if (w->left) w->left->color = BLACK;
                rb_right_rotate(tree, x_parent);
                x = tree->root;
            }
        }
    }
    if (x) x->color = BLACK;
}

void rb_delete(RB_Tree *tree, int key) {
    RB_Node *z = rb_find_node(tree, key);
    if (!z) return;

    RB_Node *y = z;
    RB_Color orig_color = y->color;
    RB_Node *x;
    RB_Node *x_parent;

    if (!z->left) {
        x = z->right;
        x_parent = z->parent;
        rb_transplant(tree, z, z->right);
    } else if (!z->right) {
        x = z->left;
        x_parent = z->parent;
        rb_transplant(tree, z, z->left);
    } else {
        y = rb_tree_minimum(z->right);
        orig_color = y->color;
        x = y->right;
        if (y->parent == z) {
            x_parent = y;
        } else {
            x_parent = y->parent;
            rb_transplant(tree, y, y->right);
            y->right = z->right;
            y->right->parent = y;
        }
        rb_transplant(tree, z, y);
        y->left = z->left;
        y->left->parent = y;
        y->color = z->color;
    }
    free(z);
    tree->size--;

    if (orig_color == BLACK)
        rb_delete_fixup(tree, x, x_parent);
}

/* ========== Print (preorder, for debugging) ========== */

static void rb_print_recursive(RB_Node *node) {
    if (!node) return;
    printf("%d(%c) ", node->key, node->color == RED ? 'R' : 'B');
    rb_print_recursive(node->left);
    rb_print_recursive(node->right);
}

void rb_print(RB_Tree *tree) {
    rb_print_recursive(tree->root);
}

#endif /* RB_TREE_C */
