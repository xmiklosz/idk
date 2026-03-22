/*
 * Red-Black Tree Implementation (CLRS style with NIL sentinel)
 * Self-balancing BST with O(log n) insert, search, delete.
 *
 * Properties:
 * 1. Every node is red or black
 * 2. Root is black
 * 3. NIL leaves are black
 * 4. Red node has only black children
 * 5. All paths from a node to descendant NILs have equal black count
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
    RB_Node *nil;   /* sentinel node (always black) */
    int size;
} RB_Tree;

/* ========== Create / Destroy ========== */

RB_Tree *rb_create(void) {
    RB_Tree *tree = (RB_Tree *)malloc(sizeof(RB_Tree));
    if (!tree) {
        fprintf(stderr, "rb_create: memory allocation error\n");
        exit(EXIT_FAILURE);
    }
    /* initialize sentinel */
    tree->nil = (RB_Node *)malloc(sizeof(RB_Node));
    if (!tree->nil) {
        fprintf(stderr, "rb_create: memory allocation error\n");
        exit(EXIT_FAILURE);
    }
    tree->nil->color = BLACK;
    tree->nil->key = 0;
    tree->nil->left = tree->nil->right = tree->nil->parent = tree->nil;
    tree->root = tree->nil;
    tree->size = 0;
    return tree;
}

static void rb_free_subtree(RB_Tree *tree, RB_Node *node) {
    if (node == tree->nil) return;
    rb_free_subtree(tree, node->left);
    rb_free_subtree(tree, node->right);
    free(node);
}

void rb_destroy(RB_Tree *tree) {
    if (!tree) return;
    rb_free_subtree(tree, tree->root);
    free(tree->nil);
    free(tree);
}

/* ========== Rotations ========== */

static void rb_left_rotate(RB_Tree *tree, RB_Node *x) {
    RB_Node *y = x->right;
    x->right = y->left;
    if (y->left != tree->nil)
        y->left->parent = x;
    y->parent = x->parent;
    if (x->parent == tree->nil)
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
    if (x->right != tree->nil)
        x->right->parent = y;
    x->parent = y->parent;
    if (y->parent == tree->nil)
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
    while (cur != tree->nil) {
        if (key == cur->key) return cur;
        if (key < cur->key)
            cur = cur->left;
        else
            cur = cur->right;
    }
    return tree->nil;
}

bool rb_search(RB_Tree *tree, int key) {
    return rb_find_node(tree, key) != tree->nil;
}

/* ========== Insert ========== */

static void rb_insert_fixup(RB_Tree *tree, RB_Node *z) {
    while (z->parent->color == RED) {
        if (z->parent == z->parent->parent->left) {
            RB_Node *uncle = z->parent->parent->right;
            if (uncle->color == RED) {
                /* Case 1: uncle is red - recolor */
                z->parent->color = BLACK;
                uncle->color = BLACK;
                z->parent->parent->color = RED;
                z = z->parent->parent;
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
            RB_Node *uncle = z->parent->parent->left;
            if (uncle->color == RED) {
                z->parent->color = BLACK;
                uncle->color = BLACK;
                z->parent->parent->color = RED;
                z = z->parent->parent;
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
    if (rb_find_node(tree, key) != tree->nil) return;

    RB_Node *z = (RB_Node *)malloc(sizeof(RB_Node));
    if (!z) {
        fprintf(stderr, "rb_insert: memory allocation error\n");
        exit(EXIT_FAILURE);
    }
    z->key = key;
    z->color = RED;
    z->left = z->right = tree->nil;

    /* standard BST insert */
    RB_Node *parent = tree->nil;
    RB_Node *cur = tree->root;
    while (cur != tree->nil) {
        parent = cur;
        if (key < cur->key)
            cur = cur->left;
        else
            cur = cur->right;
    }
    z->parent = parent;
    if (parent == tree->nil)
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
    if (u->parent == tree->nil)
        tree->root = v;
    else if (u == u->parent->left)
        u->parent->left = v;
    else
        u->parent->right = v;
    v->parent = u->parent;
}

static RB_Node *rb_tree_minimum(RB_Tree *tree, RB_Node *node) {
    while (node->left != tree->nil)
        node = node->left;
    return node;
}

static void rb_delete_fixup(RB_Tree *tree, RB_Node *x) {
    while (x != tree->root && x->color == BLACK) {
        if (x == x->parent->left) {
            RB_Node *w = x->parent->right;
            if (w->color == RED) {
                /* Case 1 */
                w->color = BLACK;
                x->parent->color = RED;
                rb_left_rotate(tree, x->parent);
                w = x->parent->right;
            }
            if (w->left->color == BLACK && w->right->color == BLACK) {
                /* Case 2 */
                w->color = RED;
                x = x->parent;
            } else {
                if (w->right->color == BLACK) {
                    /* Case 3 */
                    w->left->color = BLACK;
                    w->color = RED;
                    rb_right_rotate(tree, w);
                    w = x->parent->right;
                }
                /* Case 4 */
                w->color = x->parent->color;
                x->parent->color = BLACK;
                w->right->color = BLACK;
                rb_left_rotate(tree, x->parent);
                x = tree->root;
            }
        } else {
            /* mirror */
            RB_Node *w = x->parent->left;
            if (w->color == RED) {
                w->color = BLACK;
                x->parent->color = RED;
                rb_right_rotate(tree, x->parent);
                w = x->parent->left;
            }
            if (w->right->color == BLACK && w->left->color == BLACK) {
                w->color = RED;
                x = x->parent;
            } else {
                if (w->left->color == BLACK) {
                    w->right->color = BLACK;
                    w->color = RED;
                    rb_left_rotate(tree, w);
                    w = x->parent->left;
                }
                w->color = x->parent->color;
                x->parent->color = BLACK;
                w->left->color = BLACK;
                rb_right_rotate(tree, x->parent);
                x = tree->root;
            }
        }
    }
    x->color = BLACK;
}

void rb_delete(RB_Tree *tree, int key) {
    RB_Node *z = rb_find_node(tree, key);
    if (z == tree->nil) return;

    RB_Node *y = z;
    RB_Color orig_color = y->color;
    RB_Node *x;

    if (z->left == tree->nil) {
        x = z->right;
        rb_transplant(tree, z, z->right);
    } else if (z->right == tree->nil) {
        x = z->left;
        rb_transplant(tree, z, z->left);
    } else {
        y = rb_tree_minimum(tree, z->right);
        orig_color = y->color;
        x = y->right;
        if (y->parent == z) {
            x->parent = y;  /* needed when x is the sentinel */
        } else {
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
        rb_delete_fixup(tree, x);
}

/* ========== Print (preorder, for debugging) ========== */

static void rb_print_recursive(RB_Tree *tree, RB_Node *node) {
    if (node == tree->nil) return;
    printf("%d(%c) ", node->key, node->color == RED ? 'R' : 'B');
    rb_print_recursive(tree, node->left);
    rb_print_recursive(tree, node->right);
}

void rb_print(RB_Tree *tree) {
    rb_print_recursive(tree, tree->root);
}

#endif /* RB_TREE_C */
