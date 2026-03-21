/*
 * Red-Black Tree Implementation (CLRS style with NIL sentinel)
 * Self-balancing BST with O(log n) insert, search, delete.
 */
#ifndef RB_TREE_H
#define RB_TREE_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

typedef enum { RB_RED, RB_BLACK } RB_Color;

typedef struct RB_Node {
    int key;
    RB_Color color;
    struct RB_Node *left, *right, *parent;
} RB_Node;

typedef struct {
    RB_Node *root;
    RB_Node *nil; /* sentinel node */
    int count;
} RB_Tree;

/* --- Create / Destroy --- */

RB_Tree *rb_create(void) {
    RB_Tree *t = (RB_Tree *)malloc(sizeof(RB_Tree));
    if (!t) { fprintf(stderr, "rb_create: alloc failed\n"); exit(1); }
    t->nil = (RB_Node *)malloc(sizeof(RB_Node));
    t->nil->color = RB_BLACK;
    t->nil->left = t->nil->right = t->nil->parent = t->nil;
    t->nil->key = 0;
    t->root = t->nil;
    t->count = 0;
    return t;
}

static void rb_destroy_nodes(RB_Tree *t, RB_Node *nd) {
    if (nd == t->nil) return;
    rb_destroy_nodes(t, nd->left);
    rb_destroy_nodes(t, nd->right);
    free(nd);
}

void rb_destroy(RB_Tree *t) {
    if (!t) return;
    rb_destroy_nodes(t, t->root);
    free(t->nil);
    free(t);
}

/* --- Rotations --- */

static void rb_rotate_left(RB_Tree *t, RB_Node *x) {
    RB_Node *y = x->right;
    x->right = y->left;
    if (y->left != t->nil) y->left->parent = x;
    y->parent = x->parent;
    if (x->parent == t->nil) t->root = y;
    else if (x == x->parent->left) x->parent->left = y;
    else x->parent->right = y;
    y->left = x;
    x->parent = y;
}

static void rb_rotate_right(RB_Tree *t, RB_Node *y) {
    RB_Node *x = y->left;
    y->left = x->right;
    if (x->right != t->nil) x->right->parent = y;
    x->parent = y->parent;
    if (y->parent == t->nil) t->root = x;
    else if (y == y->parent->right) y->parent->right = x;
    else y->parent->left = x;
    x->right = y;
    y->parent = x;
}

/* --- Search --- */

static RB_Node *rb_search_node(RB_Tree *t, int key) {
    RB_Node *cur = t->root;
    while (cur != t->nil) {
        if (key == cur->key) return cur;
        cur = (key < cur->key) ? cur->left : cur->right;
    }
    return t->nil;
}

bool rb_search(RB_Tree *t, int key) {
    return rb_search_node(t, key) != t->nil;
}

/* --- Insert --- */

static void rb_insert_fixup(RB_Tree *t, RB_Node *z) {
    while (z->parent->color == RB_RED) {
        if (z->parent == z->parent->parent->left) {
            RB_Node *y = z->parent->parent->right;
            if (y->color == RB_RED) {
                z->parent->color = RB_BLACK;
                y->color = RB_BLACK;
                z->parent->parent->color = RB_RED;
                z = z->parent->parent;
            } else {
                if (z == z->parent->right) {
                    z = z->parent;
                    rb_rotate_left(t, z);
                }
                z->parent->color = RB_BLACK;
                z->parent->parent->color = RB_RED;
                rb_rotate_right(t, z->parent->parent);
            }
        } else {
            RB_Node *y = z->parent->parent->left;
            if (y->color == RB_RED) {
                z->parent->color = RB_BLACK;
                y->color = RB_BLACK;
                z->parent->parent->color = RB_RED;
                z = z->parent->parent;
            } else {
                if (z == z->parent->left) {
                    z = z->parent;
                    rb_rotate_right(t, z);
                }
                z->parent->color = RB_BLACK;
                z->parent->parent->color = RB_RED;
                rb_rotate_left(t, z->parent->parent);
            }
        }
    }
    t->root->color = RB_BLACK;
}

void rb_insert(RB_Tree *t, int key) {
    /* Check for duplicate */
    if (rb_search_node(t, key) != t->nil) return;

    RB_Node *z = (RB_Node *)malloc(sizeof(RB_Node));
    if (!z) { fprintf(stderr, "rb_insert: alloc failed\n"); exit(1); }
    z->key = key;
    z->color = RB_RED;
    z->left = z->right = t->nil;

    RB_Node *y = t->nil;
    RB_Node *x = t->root;
    while (x != t->nil) {
        y = x;
        x = (key < x->key) ? x->left : x->right;
    }
    z->parent = y;
    if (y == t->nil) t->root = z;
    else if (key < y->key) y->left = z;
    else y->right = z;

    t->count++;
    rb_insert_fixup(t, z);
}

/* --- Delete --- */

static void rb_transplant(RB_Tree *t, RB_Node *u, RB_Node *v) {
    if (u->parent == t->nil) t->root = v;
    else if (u == u->parent->left) u->parent->left = v;
    else u->parent->right = v;
    v->parent = u->parent;
}

static RB_Node *rb_minimum(RB_Tree *t, RB_Node *nd) {
    while (nd->left != t->nil) nd = nd->left;
    return nd;
}

static void rb_delete_fixup(RB_Tree *t, RB_Node *x) {
    while (x != t->root && x->color == RB_BLACK) {
        if (x == x->parent->left) {
            RB_Node *w = x->parent->right;
            if (w->color == RB_RED) {
                w->color = RB_BLACK;
                x->parent->color = RB_RED;
                rb_rotate_left(t, x->parent);
                w = x->parent->right;
            }
            if (w->left->color == RB_BLACK && w->right->color == RB_BLACK) {
                w->color = RB_RED;
                x = x->parent;
            } else {
                if (w->right->color == RB_BLACK) {
                    w->left->color = RB_BLACK;
                    w->color = RB_RED;
                    rb_rotate_right(t, w);
                    w = x->parent->right;
                }
                w->color = x->parent->color;
                x->parent->color = RB_BLACK;
                w->right->color = RB_BLACK;
                rb_rotate_left(t, x->parent);
                x = t->root;
            }
        } else {
            RB_Node *w = x->parent->left;
            if (w->color == RB_RED) {
                w->color = RB_BLACK;
                x->parent->color = RB_RED;
                rb_rotate_right(t, x->parent);
                w = x->parent->left;
            }
            if (w->right->color == RB_BLACK && w->left->color == RB_BLACK) {
                w->color = RB_RED;
                x = x->parent;
            } else {
                if (w->left->color == RB_BLACK) {
                    w->right->color = RB_BLACK;
                    w->color = RB_RED;
                    rb_rotate_left(t, w);
                    w = x->parent->left;
                }
                w->color = x->parent->color;
                x->parent->color = RB_BLACK;
                w->left->color = RB_BLACK;
                rb_rotate_right(t, x->parent);
                x = t->root;
            }
        }
    }
    x->color = RB_BLACK;
}

void rb_delete(RB_Tree *t, int key) {
    RB_Node *z = rb_search_node(t, key);
    if (z == t->nil) return;

    RB_Node *y = z;
    RB_Color y_orig = y->color;
    RB_Node *x;

    if (z->left == t->nil) {
        x = z->right;
        rb_transplant(t, z, z->right);
    } else if (z->right == t->nil) {
        x = z->left;
        rb_transplant(t, z, z->left);
    } else {
        y = rb_minimum(t, z->right);
        y_orig = y->color;
        x = y->right;
        if (y->parent == z) {
            x->parent = y; /* important when x is nil */
        } else {
            rb_transplant(t, y, y->right);
            y->right = z->right;
            y->right->parent = y;
        }
        rb_transplant(t, z, y);
        y->left = z->left;
        y->left->parent = y;
        y->color = z->color;
    }
    free(z);
    t->count--;
    if (y_orig == RB_BLACK) rb_delete_fixup(t, x);
}

/* --- Print (preorder for debugging) --- */

static void rb_print_rec(RB_Tree *t, RB_Node *nd) {
    if (nd == t->nil) return;
    printf("%d(%c) ", nd->key, nd->color == RB_RED ? 'R' : 'B');
    rb_print_rec(t, nd->left);
    rb_print_rec(t, nd->right);
}

void rb_print(RB_Tree *t) {
    rb_print_rec(t, t->root);
}

#endif /* RB_TREE_H */
