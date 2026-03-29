/* Cerveno-cierny strom (Red-Black Tree) */
#ifndef RB_TREE_C       /* ochrana pred viacnasobnym vlozenim */
#define RB_TREE_C       /* definicia ochrany */

#include <stdio.h>       /* standardny vstup/vystup */
#include <stdlib.h>      /* standardna kniznica (malloc, free) */
#include <stdbool.h>     /* typ bool (true/false) */

/* ========== Datove struktury ========== */

/* Farba uzla cerveno-cierneho stromu */
typedef enum { RED, BLACK } RB_Color;

/* Uzol cerveno-cierneho stromu */
typedef struct RB_Node {
    int key;                          /* kluc ulozeny v uzle */
    RB_Color color;                   /* farba uzla (cervena alebo cierna) */
    struct RB_Node *left, *right, *parent; /* ukazatele na laveho, praveho potomka a rodica */
} RB_Node;

/* Cerveno-cierny strom */
typedef struct {
    RB_Node *root;  /* ukazatel na koren stromu */
    int size;       /* pocet prvkov v strome */
} RB_Tree;

/* ========== Pomocna funkcia pre farbu (NULL je CIERNA) ========== */

/* Vrati farbu uzla, NULL uzly su povazovane za cierne */
static RB_Color rb_node_color(RB_Node *node) {
    if (!node) return BLACK;  /* NULL uzly su cierne */
    return node->color;       /* vratenie farby uzla */
}

/* ========== Vytvorenie / Zrusenie ========== */

/* Vytvori novy prazdny cerveno-cierny strom */
RB_Tree *rb_create(void) {
    RB_Tree *tree = (RB_Tree *)malloc(sizeof(RB_Tree)); /* alokacia struktury stromu */
    if (!tree) {                                         /* kontrola uspesnosti alokacie */
        fprintf(stderr, "rb_create: memory allocation error\n"); /* chybova hlaska */
        exit(EXIT_FAILURE);                                      /* ukoncenie programu */
    }
    tree->root = NULL;  /* prazdny strom nema koren */
    tree->size = 0;     /* pociatocny pocet prvkov je 0 */
    return tree;        /* vratenie vytvoreneho stromu */
}

/* Rekurzivne uvolni vsetky uzly podstromu */
static void rb_free_subtree(RB_Node *node) {
    if (!node) return;                  /* zakladny pripad: prazdny uzol */
    rb_free_subtree(node->left);        /* rekurzivne uvolnenie laveho podstromu */
    rb_free_subtree(node->right);       /* rekurzivne uvolnenie praveho podstromu */
    free(node);                         /* uvolnenie samotneho uzla */
}

/* Zrusi cerveno-cierny strom a uvolni vsetku pamat */
void rb_destroy(RB_Tree *tree) {
    if (!tree) return;              /* ak je strom NULL, nic nerobime */
    rb_free_subtree(tree->root);    /* uvolnenie vsetkych uzlov */
    free(tree);                     /* uvolnenie struktury stromu */
}

/* ========== Rotacie ========== */

/* Lava rotacia okolo uzla x */
static void rb_left_rotate(RB_Tree *tree, RB_Node *x) {
    RB_Node *y = x->right;       /* y je pravy potomok x */
    x->right = y->left;          /* lavy podstrom y sa stane pravym podstromom x */
    if (y->left)                  /* ak y ma laveho potomka */
        y->left->parent = x;     /* nastavime jeho rodica na x */
    y->parent = x->parent;       /* rodic y sa stane rodicom x */
    if (!x->parent)               /* ak x bol koren */
        tree->root = y;          /* y sa stane novym korenom */
    else if (x == x->parent->left) /* ak x bol lavy potomok */
        x->parent->left = y;     /* y sa stane lavym potomkom rodica x */
    else                          /* ak x bol pravy potomok */
        x->parent->right = y;    /* y sa stane pravym potomkom rodica x */
    y->left = x;                  /* x sa stane lavym potomkom y */
    x->parent = y;                /* rodic x sa stane y */
}

/* Prava rotacia okolo uzla y */
static void rb_right_rotate(RB_Tree *tree, RB_Node *y) {
    RB_Node *x = y->left;        /* x je lavy potomok y */
    y->left = x->right;          /* pravy podstrom x sa stane lavym podstromom y */
    if (x->right)                 /* ak x ma praveho potomka */
        x->right->parent = y;    /* nastavime jeho rodica na y */
    x->parent = y->parent;       /* rodic x sa stane rodicom y */
    if (!y->parent)               /* ak y bol koren */
        tree->root = x;          /* x sa stane novym korenom */
    else if (y == y->parent->right) /* ak y bol pravy potomok */
        y->parent->right = x;    /* x sa stane pravym potomkom rodica y */
    else                          /* ak y bol lavy potomok */
        y->parent->left = x;     /* x sa stane lavym potomkom rodica y */
    x->right = y;                 /* y sa stane pravym potomkom x */
    y->parent = x;                /* rodic y sa stane x */
}

/* ========== Vyhladanie ========== */

/* Najde uzol s danym klucom v strome, vrati NULL ak sa nenasiel */
static RB_Node *rb_find_node(RB_Tree *tree, int key) {
    RB_Node *cur = tree->root;     /* zaciname od korena */
    while (cur) {                  /* kym nie sme na konci */
        if (key == cur->key) return cur; /* kluc najdeny */
        if (key < cur->key)        /* ak je kluc mensi */
            cur = cur->left;       /* ideme dolava */
        else                       /* ak je kluc vacsi */
            cur = cur->right;      /* ideme doprava */
    }
    return NULL; /* kluc sa nenasiel */
}

/* Vyhlada kluc v strome, vrati true ak existuje */
bool rb_search(RB_Tree *tree, int key) {
    return rb_find_node(tree, key) != NULL; /* true ak uzol existuje */
}

/* ========== Vlozenie ========== */

/* Opravi vlastnosti cerveno-cierneho stromu po vlozeni */
static void rb_insert_fixup(RB_Tree *tree, RB_Node *z) {
    while (z->parent && z->parent->color == RED) {     /* kym je rodic cerveny (porusenie vlastnosti) */
        RB_Node *grandparent = z->parent->parent;      /* stary rodic (dedo) */
        if (z->parent == grandparent->left) {           /* ak je rodic lavy potomok deda */
            RB_Node *uncle = grandparent->right;        /* strykko je pravy potomok deda */
            if (rb_node_color(uncle) == RED) {           /* Pripad 1: strykko je cerveny - prefarbenie */
                z->parent->color = BLACK;               /* rodic sa stane ciernym */
                uncle->color = BLACK;                   /* strykko sa stane ciernym */
                grandparent->color = RED;               /* dedo sa stane cervenym */
                z = grandparent;                        /* pokracujeme od deda */
            } else {
                if (z == z->parent->right) {            /* Pripad 2: strykko cierny, z je pravy potomok */
                    z = z->parent;                      /* posunieme sa na rodica */
                    rb_left_rotate(tree, z);            /* lava rotacia */
                }
                /* Pripad 3: strykko cierny, z je lavy potomok */
                z->parent->color = BLACK;               /* rodic sa stane ciernym */
                z->parent->parent->color = RED;         /* dedo sa stane cervenym */
                rb_right_rotate(tree, z->parent->parent); /* prava rotacia okolo deda */
            }
        } else {
            /* zrkadlovy pripad: rodic je pravy potomok deda */
            RB_Node *uncle = grandparent->left;         /* strykko je lavy potomok deda */
            if (rb_node_color(uncle) == RED) {           /* Pripad 1: strykko je cerveny */
                z->parent->color = BLACK;               /* rodic sa stane ciernym */
                uncle->color = BLACK;                   /* strykko sa stane ciernym */
                grandparent->color = RED;               /* dedo sa stane cervenym */
                z = grandparent;                        /* pokracujeme od deda */
            } else {
                if (z == z->parent->left) {             /* Pripad 2: strykko cierny, z je lavy potomok */
                    z = z->parent;                      /* posunieme sa na rodica */
                    rb_right_rotate(tree, z);           /* prava rotacia */
                }
                /* Pripad 3: strykko cierny, z je pravy potomok */
                z->parent->color = BLACK;               /* rodic sa stane ciernym */
                z->parent->parent->color = RED;         /* dedo sa stane cervenym */
                rb_left_rotate(tree, z->parent->parent); /* lava rotacia okolo deda */
            }
        }
    }
    tree->root->color = BLACK; /* koren musi byt vzdy cierny */
}

/* Vlozi kluc do cerveno-cierneho stromu (duplicity sa ignoruju) */
void rb_insert(RB_Tree *tree, int key) {
    /* odmietnutie duplicit */
    if (rb_find_node(tree, key)) return; /* ak kluc uz existuje, nerobime nic */

    RB_Node *z = (RB_Node *)malloc(sizeof(RB_Node)); /* alokacia noveho uzla */
    if (!z) {                                          /* kontrola uspesnosti alokacie */
        fprintf(stderr, "rb_insert: memory allocation error\n"); /* chybova hlaska */
        exit(EXIT_FAILURE);                                      /* ukoncenie programu */
    }
    z->key = key;                  /* nastavenie kluca */
    z->color = RED;                /* novy uzol je vzdy cerveny */
    z->left = z->right = NULL;     /* novy uzol nema potomkov */

    /* standardne vlozenie do binarneho vyhladavacieho stromu */
    RB_Node *parent = NULL;        /* rodic noveho uzla */
    RB_Node *cur = tree->root;     /* zaciname od korena */
    while (cur) {                  /* hladame spravne miesto */
        parent = cur;              /* ulozenie aktualneho uzla ako rodica */
        if (key < cur->key)        /* ak je kluc mensi */
            cur = cur->left;       /* ideme dolava */
        else                       /* ak je kluc vacsi */
            cur = cur->right;      /* ideme doprava */
    }
    z->parent = parent;            /* nastavenie rodica noveho uzla */
    if (!parent)                   /* ak strom bol prazdny */
        tree->root = z;           /* novy uzol sa stane korenom */
    else if (key < parent->key)    /* ak je kluc mensi nez kluc rodica */
        parent->left = z;         /* vlozenie ako lavy potomok */
    else                           /* ak je kluc vacsi nez kluc rodica */
        parent->right = z;        /* vlozenie ako pravy potomok */

    tree->size++;                  /* zvysenie poctu prvkov */
    rb_insert_fixup(tree, z);      /* oprava vlastnosti cerveno-cierneho stromu */
}

/* ========== Vymazanie ========== */

/* Nahradi podstrom zakoreneny v u podstromom zakorenenym v v */
static void rb_transplant(RB_Tree *tree, RB_Node *u, RB_Node *v) {
    if (!u->parent)                /* ak u je koren */
        tree->root = v;           /* v sa stane novym korenom */
    else if (u == u->parent->left) /* ak u je lavy potomok */
        u->parent->left = v;      /* v nahradi u ako lavy potomok */
    else                           /* ak u je pravy potomok */
        u->parent->right = v;     /* v nahradi u ako pravy potomok */
    if (v)                         /* ak v nie je NULL */
        v->parent = u->parent;    /* nastavenie rodica v na rodica u */
}

/* Najde uzol s minimalnym klucom v podstrome */
static RB_Node *rb_tree_minimum(RB_Node *node) {
    while (node->left)             /* kym existuje lavy potomok */
        node = node->left;        /* ideme dolava */
    return node;                   /* vratenie uzla s minimalnym klucom */
}

/*
 * Opravi vlastnosti cerveno-cierneho stromu po vymazani.
 * x_parent sa sleduje osobitne pretoze x moze byt NULL.
 */
static void rb_delete_fixup(RB_Tree *tree, RB_Node *x, RB_Node *x_parent) {
    while (x != tree->root && rb_node_color(x) == BLACK) { /* kym x nie je koren a je cierny */
        if (x == x_parent->left) {                         /* ak x je lavy potomok */
            RB_Node *w = x_parent->right;                  /* w je surodene (brat) x */
            if (rb_node_color(w) == RED) {                  /* Pripad 1: brat je cerveny */
                w->color = BLACK;                          /* brat sa stane ciernym */
                x_parent->color = RED;                     /* rodic sa stane cervenym */
                rb_left_rotate(tree, x_parent);            /* lava rotacia okolo rodica */
                w = x_parent->right;                       /* aktualizacia brata */
            }
            if (rb_node_color(w->left) == BLACK && rb_node_color(w->right) == BLACK) { /* Pripad 2: obaja potomkovia brata su cierny */
                w->color = RED;                            /* brat sa stane cervenym */
                x = x_parent;                              /* posun nahor */
                x_parent = x->parent;                      /* aktualizacia rodica */
            } else {
                if (rb_node_color(w->right) == BLACK) {     /* Pripad 3: pravy potomok brata je cierny */
                    if (w->left) w->left->color = BLACK;   /* lavy potomok brata sa stane ciernym */
                    w->color = RED;                        /* brat sa stane cervenym */
                    rb_right_rotate(tree, w);              /* prava rotacia okolo brata */
                    w = x_parent->right;                   /* aktualizacia brata */
                }
                /* Pripad 4: pravy potomok brata je cerveny */
                w->color = x_parent->color;                /* brat prevezme farbu rodica */
                x_parent->color = BLACK;                   /* rodic sa stane ciernym */
                if (w->right) w->right->color = BLACK;    /* pravy potomok brata sa stane ciernym */
                rb_left_rotate(tree, x_parent);            /* lava rotacia okolo rodica */
                x = tree->root;                            /* koniec opravy */
            }
        } else {
            /* zrkadlovy pripad: x je pravy potomok */
            RB_Node *w = x_parent->left;                   /* w je surodene (brat) x */
            if (rb_node_color(w) == RED) {                  /* Pripad 1: brat je cerveny */
                w->color = BLACK;                          /* brat sa stane ciernym */
                x_parent->color = RED;                     /* rodic sa stane cervenym */
                rb_right_rotate(tree, x_parent);           /* prava rotacia okolo rodica */
                w = x_parent->left;                        /* aktualizacia brata */
            }
            if (rb_node_color(w->right) == BLACK && rb_node_color(w->left) == BLACK) { /* Pripad 2: obaja potomkovia brata su cierny */
                w->color = RED;                            /* brat sa stane cervenym */
                x = x_parent;                              /* posun nahor */
                x_parent = x->parent;                      /* aktualizacia rodica */
            } else {
                if (rb_node_color(w->left) == BLACK) {      /* Pripad 3: lavy potomok brata je cierny */
                    if (w->right) w->right->color = BLACK; /* pravy potomok brata sa stane ciernym */
                    w->color = RED;                        /* brat sa stane cervenym */
                    rb_left_rotate(tree, w);               /* lava rotacia okolo brata */
                    w = x_parent->left;                    /* aktualizacia brata */
                }
                /* Pripad 4: lavy potomok brata je cerveny */
                w->color = x_parent->color;                /* brat prevezme farbu rodica */
                x_parent->color = BLACK;                   /* rodic sa stane ciernym */
                if (w->left) w->left->color = BLACK;      /* lavy potomok brata sa stane ciernym */
                rb_right_rotate(tree, x_parent);           /* prava rotacia okolo rodica */
                x = tree->root;                            /* koniec opravy */
            }
        }
    }
    if (x) x->color = BLACK; /* ak x existuje, nastavime ho na cierny */
}

/* Vymaze kluc z cerveno-cierneho stromu */
void rb_delete(RB_Tree *tree, int key) {
    RB_Node *z = rb_find_node(tree, key); /* najdenie uzla s danym klucom */
    if (!z) return;                        /* ak sa nenasiel, nic nerobime */

    RB_Node *y = z;                        /* y je uzol ktory bude skutocne vymazany */
    RB_Color orig_color = y->color;        /* povodna farba y */
    RB_Node *x;                            /* nahradny uzol */
    RB_Node *x_parent;                     /* rodic nahradneho uzla */

    if (!z->left) {                        /* ak z nema laveho potomka */
        x = z->right;                     /* nahradny uzol je pravy potomok */
        x_parent = z->parent;             /* rodic nahradneho uzla */
        rb_transplant(tree, z, z->right); /* nahradenie z jeho pravym potomkom */
    } else if (!z->right) {               /* ak z nema praveho potomka */
        x = z->left;                      /* nahradny uzol je lavy potomok */
        x_parent = z->parent;             /* rodic nahradneho uzla */
        rb_transplant(tree, z, z->left);  /* nahradenie z jeho lavym potomkom */
    } else {                               /* ak z ma oboch potomkov */
        y = rb_tree_minimum(z->right);    /* y je naslednik (minimum v pravom podstrome) */
        orig_color = y->color;            /* ulozenie povodnej farby naslennika */
        x = y->right;                     /* nahradny uzol je pravy potomok naslennika */
        if (y->parent == z) {             /* ak je naslednik priamym potomkom z */
            x_parent = y;                 /* rodic nahradneho uzla je naslednik */
        } else {                           /* ak naslednik nie je priamym potomkom z */
            x_parent = y->parent;         /* rodic nahradneho uzla */
            rb_transplant(tree, y, y->right); /* nahradenie naslennika jeho pravym potomkom */
            y->right = z->right;          /* pravy potomok z sa stane pravym potomkom naslennika */
            y->right->parent = y;         /* nastavenie rodica */
        }
        rb_transplant(tree, z, y);        /* nahradenie z naslenikom */
        y->left = z->left;               /* lavy potomok z sa stane lavym potomkom naslennika */
        y->left->parent = y;             /* nastavenie rodica */
        y->color = z->color;             /* naslednik prevezme farbu z */
    }
    free(z);                               /* uvolnenie vymazaneho uzla */
    tree->size--;                          /* znizenie poctu prvkov */

    if (orig_color == BLACK)               /* ak bol vymazany uzol cierny */
        rb_delete_fixup(tree, x, x_parent); /* oprava vlastnosti stromu */
}

/* ========== Vypis (preorder, pre ladenie) ========== */

/* Rekurzivny vypis uzlov v preorder poradi */
static void rb_print_recursive(RB_Node *node) {
    if (!node) return;                                                 /* zakladny pripad: prazdny uzol */
    printf("%d(%c) ", node->key, node->color == RED ? 'R' : 'B');     /* vypis kluca a farby */
    rb_print_recursive(node->left);                                    /* rekurzivny vypis laveho podstromu */
    rb_print_recursive(node->right);                                   /* rekurzivny vypis praveho podstromu */
}

/* Vypise cerveno-cierny strom v preorder poradi */
void rb_print(RB_Tree *tree) {
    rb_print_recursive(tree->root); /* spustenie rekurzivneho vypisu od korena */
}

#endif /* RB_TREE_C */ /* koniec ochrany pred viacnasobnym vlozenim */
