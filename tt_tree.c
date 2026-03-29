/* 2-3 Strom */
#ifndef TT_TREE_C       /* ochrana pred viacnasobnym vlozenim */
#define TT_TREE_C       /* definicia ochrany */

#include <stdio.h>       /* standardny vstup/vystup */
#include <stdlib.h>      /* standardna kniznica (malloc, calloc, free) */
#include <stdbool.h>     /* typ bool (true/false) */

/* ========== Datove struktury ========== */

/* Uzol 2-3 stromu */
typedef struct TT_Node {
    int keys[2];                  /* pole klucov (maximalne 2 kluce na uzol) */
    struct TT_Node *children[3]; /* pole potomkov (maximalne 3 potomkovia) */
    int num_keys;                /* pocet klucov v uzle (1 pre 2-uzol, 2 pre 3-uzol) */
} TT_Node;

/* 2-3 Strom */
typedef struct {
    TT_Node *root;  /* ukazatel na koren stromu */
    int size;       /* celkovy pocet ulozenych prvkov */
} TT_Tree;

/* Vysledok rekurzivneho vlozenia - nesie informaciu o rozdeleni nahor */
typedef struct {
    TT_Node *split_right; /* NULL znamena ze k rozdeleniu nedoslo */
    int median;           /* kluc povyseny do rodica pri rozdeleni */
} TT_SplitResult;

/* ========== Pomocne funkcie ========== */

/* Alokuje novy uzol s jednym klucom */
static TT_Node *tt_alloc_node(int key) {
    TT_Node *node = (TT_Node *)calloc(1, sizeof(TT_Node)); /* alokacia a vynulovanie uzla */
    if (!node) {                                             /* kontrola uspesnosti alokacie */
        fprintf(stderr, "tt_alloc_node: memory allocation error\n"); /* chybova hlaska */
        exit(EXIT_FAILURE);                                          /* ukoncenie programu */
    }
    node->keys[0] = key;   /* nastavenie prveho kluca */
    node->num_keys = 1;    /* uzol ma jeden kluc (2-uzol) */
    return node;            /* vratenie noveho uzla */
}

/* Zisti ci je uzol listom (nema potomkov) */
static bool tt_is_leaf(TT_Node *node) {
    return node->children[0] == NULL; /* list nema prveho potomka */
}

/* Rekurzivne uvolni vsetky uzly podstromu */
static void tt_free_subtree(TT_Node *node) {
    if (!node) return;                              /* zakladny pripad: prazdny uzol */
    for (int i = 0; i <= node->num_keys; i++)       /* prechod cez vsetkych potomkov */
        tt_free_subtree(node->children[i]);         /* rekurzivne uvolnenie potomka */
    free(node);                                     /* uvolnenie samotneho uzla */
}

/* ========== Vytvorenie / Zrusenie ========== */

/* Vytvori novy prazdny 2-3 strom */
TT_Tree *tt_create(void) {
    TT_Tree *tree = (TT_Tree *)calloc(1, sizeof(TT_Tree)); /* alokacia a vynulovanie struktury */
    if (!tree) {                                             /* kontrola uspesnosti alokacie */
        fprintf(stderr, "tt_create: memory allocation error\n"); /* chybova hlaska */
        exit(EXIT_FAILURE);                                      /* ukoncenie programu */
    }
    return tree; /* vratenie vytvoreneho stromu */
}

/* Zrusi 2-3 strom a uvolni vsetku pamat */
void tt_destroy(TT_Tree *tree) {
    if (!tree) return;              /* ak je strom NULL, nic nerobime */
    tt_free_subtree(tree->root);    /* uvolnenie vsetkych uzlov */
    free(tree);                     /* uvolnenie struktury stromu */
}

/* ========== Vyhladanie ========== */

/* Vyhlada kluc v 2-3 strome, vrati true ak sa nasiel */
bool tt_search(TT_Tree *tree, int key) {
    TT_Node *cur = tree->root;                /* zaciname od korena */
    while (cur) {                              /* kym nie sme na konci */
        int i = 0;                             /* index pre prechod klucmi */
        while (i < cur->num_keys) {            /* prechod cez kluce uzla */
            if (key == cur->keys[i]) return true; /* kluc najdeny */
            if (key < cur->keys[i]) break;     /* kluc je mensi, ideme do potomka */
            i++;                               /* posun na dalsi kluc */
        }
        cur = cur->children[i];                /* zostup do prislusneho potomka */
    }
    return false; /* kluc sa nenasiel */
}

/* ========== Vlozenie ========== */

/* Rekurzivne vlozenie kluca do podstromu */
static TT_SplitResult tt_insert_recursive(TT_Node *node, int key, bool *was_inserted) {
    TT_SplitResult result = {NULL, 0}; /* inicializacia vysledku (bez rozdelenia) */

    /* najdenie pozicie pre kluc v tomto uzle */
    int pos = 0;                                           /* pozicia pre vlozenie */
    while (pos < node->num_keys && key > node->keys[pos]) pos++; /* hladanie spravnej pozicie */

    /* kontrola duplicity */
    if (pos < node->num_keys && key == node->keys[pos]) { /* ak kluc uz existuje */
        *was_inserted = false;                              /* oznacenie ze sa nevlozil */
        return result;                                      /* vratenie bez zmeny */
    }

    int ins_key = key;           /* kluc na vlozenie */
    TT_Node *ins_child = NULL;   /* potomok priradeny k vlozeniu */

    /* ak nie je list, rekurzivne zostupime do prislusneho potomka */
    if (!tt_is_leaf(node)) {                                                       /* ak uzol nie je list */
        TT_SplitResult child_result = tt_insert_recursive(node->children[pos], key, was_inserted); /* rekurzivne vlozenie */
        if (!child_result.split_right)                                              /* ak potomok nebol rozdeleny */
            return result;  /* potomok absorboval kluc, ziadne rozdelenie */
        ins_key = child_result.median;          /* kluc povyseny z potomka */
        ins_child = child_result.split_right;   /* novy pravy uzol z rozdelenia potomka */
        /* prepocitanie pozicie pre povyseny kluc */
        pos = 0;                                                                    /* reset pozicie */
        while (pos < node->num_keys && ins_key > node->keys[pos]) pos++;           /* najdenie novej pozicie */
    }

    /* uzol ma miesto (je to 2-uzol) */
    if (node->num_keys < 2) {                                  /* ak ma uzol menej ako 2 kluce */
        /* posun klucov a potomkov doprava pre uvolnenie miesta na pozicii pos */
        for (int i = node->num_keys; i > pos; i--) {           /* posun od konca */
            node->keys[i] = node->keys[i - 1];                /* posun kluca */
            node->children[i + 1] = node->children[i];        /* posun potomka */
        }
        node->keys[pos] = ins_key;             /* vlozenie kluca na spravnu poziciu */
        node->children[pos + 1] = ins_child;   /* vlozenie potomka */
        node->num_keys++;                      /* zvysenie poctu klucov */
        return result;  /* ziadne rozdelenie nepotrebne */
    }

    /* uzol je plny (3-uzol s 2 klucmi) -> musi sa rozdelit */
    int tmp_keys[3];       /* docasne pole pre 3 kluce */
    TT_Node *tmp_ch[4];   /* docasne pole pre 4 potomkov */

    /* kopirovanie existujucich dat */
    tmp_keys[0] = node->keys[0];      /* prvy kluc */
    tmp_keys[1] = node->keys[1];      /* druhy kluc */
    tmp_ch[0] = node->children[0];    /* prvy potomok */
    tmp_ch[1] = node->children[1];    /* druhy potomok */
    tmp_ch[2] = node->children[2];    /* treti potomok */

    /* vlozenie noveho kluca a potomka na spravnu poziciu v docasnom poli */
    for (int i = 2; i > pos; i--) tmp_keys[i] = tmp_keys[i - 1];       /* posun klucov */
    for (int i = 3; i > pos + 1; i--) tmp_ch[i] = tmp_ch[i - 1];      /* posun potomkov */
    tmp_keys[pos] = ins_key;     /* vlozenie kluca */
    tmp_ch[pos + 1] = ins_child; /* vlozenie potomka */

    /* lavy uzol si ponecha tmp_keys[0] */
    node->keys[0] = tmp_keys[0];  /* prvy kluc ostava v lavom uzle */
    node->num_keys = 1;           /* lavy uzol ma teraz 1 kluc */
    node->children[0] = tmp_ch[0]; /* prvy potomok */
    node->children[1] = tmp_ch[1]; /* druhy potomok */
    node->children[2] = NULL;      /* treti potomok uz neexistuje */

    /* pravy uzol dostane tmp_keys[2] */
    TT_Node *right = tt_alloc_node(tmp_keys[2]); /* vytvorenie praveho uzla s tretim klucom */
    right->children[0] = tmp_ch[2];               /* treti potomok sa stane prvym potomkom praveho uzla */
    right->children[1] = tmp_ch[3];               /* stvrty potomok sa stane druhym potomkom praveho uzla */

    /* stredny kluc sa povysi do rodica */
    result.split_right = right;     /* pravy uzol z rozdelenia */
    result.median = tmp_keys[1];    /* stredny kluc na povysenie */
    return result;                  /* vratenie vysledku rozdelenia */
}

/* Vlozi kluc do 2-3 stromu (duplicity sa ignoruju) */
void tt_insert(TT_Tree *tree, int key) {
    if (!tree->root) {                         /* ak je strom prazdny */
        tree->root = tt_alloc_node(key);       /* vytvorenie korena s danym klucom */
        tree->size = 1;                        /* nastavenie poctu prvkov na 1 */
        return;                                /* koniec */
    }

    bool was_inserted = true;                  /* priznak ci sa kluc vlozil */
    TT_SplitResult split = tt_insert_recursive(tree->root, key, &was_inserted); /* rekurzivne vlozenie */

    if (!was_inserted) return;  /* duplicita, nic sa nevlozilo */
    tree->size++;               /* zvysenie poctu prvkov */

    if (split.split_right) {                                /* ak doslo k rozdeleniu korena */
        TT_Node *new_root = tt_alloc_node(split.median);   /* vytvorenie noveho korena so strednym klucom */
        new_root->children[0] = tree->root;                 /* stary koren sa stane lavym potomkom */
        new_root->children[1] = split.split_right;          /* pravy uzol z rozdelenia sa stane pravym potomkom */
        tree->root = new_root;                               /* nastavenie noveho korena */
    }
}

/* ========== Vymazanie ========== */

/*
 * Opravi podtecenie (underflow) v parent->children[idx].
 * Vrati true ak rodic sam podtecie po oprave.
 */
static bool tt_fix_underflow(TT_Node *parent, int idx) {
    TT_Node *child = parent->children[idx]; /* podteceny potomok */

    /* pokus o pozicanie od laveho surodenca */
    if (idx > 0) {                                         /* ak existuje lavy surodenec */
        TT_Node *left_sib = parent->children[idx - 1];    /* lavy surodenec */
        if (left_sib->num_keys == 2) {                     /* ak ma lavy surodenec 2 kluce (moze poziciat) */
            /* rotacia doprava cez rodica */
            child->keys[0] = parent->keys[idx - 1];                    /* kluc z rodica ide do potomka */
            child->num_keys = 1;                                        /* potomok ma teraz 1 kluc */
            child->children[1] = child->children[0];                    /* posun potomkov */
            child->children[0] = left_sib->children[left_sib->num_keys]; /* posledny potomok laveho surodenca */
            parent->keys[idx - 1] = left_sib->keys[left_sib->num_keys - 1]; /* posledny kluc laveho surodenca ide do rodica */
            left_sib->children[left_sib->num_keys] = NULL;             /* odstranenie posledneho potomka */
            left_sib->num_keys--;                                       /* znizenie poctu klucov laveho surodenca */
            return false;                                               /* rodic nepodtiekol */
        }
    }

    /* pokus o pozicanie od praveho surodenca */
    if (idx < parent->num_keys) {                          /* ak existuje pravy surodenec */
        TT_Node *right_sib = parent->children[idx + 1];   /* pravy surodenec */
        if (right_sib->num_keys == 2) {                    /* ak ma pravy surodenec 2 kluce (moze poziciat) */
            /* rotacia dolava cez rodica */
            child->keys[0] = parent->keys[idx];                        /* kluc z rodica ide do potomka */
            child->num_keys = 1;                                        /* potomok ma teraz 1 kluc */
            child->children[1] = right_sib->children[0];               /* prvy potomok praveho surodenca */
            parent->keys[idx] = right_sib->keys[0];                    /* prvy kluc praveho surodenca ide do rodica */
            /* posun praveho surodenca dolava */
            right_sib->keys[0] = right_sib->keys[1];                   /* posun kluca */
            right_sib->children[0] = right_sib->children[1];           /* posun potomka */
            right_sib->children[1] = right_sib->children[2];           /* posun potomka */
            right_sib->children[2] = NULL;                              /* odstranenie posledneho potomka */
            right_sib->num_keys--;                                      /* znizenie poctu klucov praveho surodenca */
            return false;                                               /* rodic nepodtiekol */
        }
    }

    /* nemozno poziciat, musime zlucit */
    if (idx > 0) {
        /* zlucenie potomka do laveho surodenca */
        TT_Node *left_sib = parent->children[idx - 1];    /* lavy surodenec */
        left_sib->keys[1] = parent->keys[idx - 1];        /* oddelovac z rodica sa stane druhym klucom */
        left_sib->children[2] = child->children[0];        /* potomok podteceneho uzla */
        left_sib->num_keys = 2;                            /* lavy surodenec ma teraz 2 kluce */

        /* odstranenie oddelovaca z rodica */
        for (int i = idx - 1; i < parent->num_keys - 1; i++) {  /* posun klucov v rodicovi */
            parent->keys[i] = parent->keys[i + 1];              /* posun kluca dolava */
            parent->children[i + 1] = parent->children[i + 2];  /* posun potomka dolava */
        }
        parent->children[parent->num_keys] = NULL;  /* odstranenie posledneho ukazatela */
        parent->num_keys--;                          /* znizenie poctu klucov rodica */
        free(child);                                 /* uvolnenie podteceneho uzla */
        return parent->num_keys == 0;                /* vrati true ak rodic podtiekol */
    } else {
        /* zlucenie praveho surodenca do potomka */
        TT_Node *right_sib = parent->children[1];         /* pravy surodenec */
        child->keys[0] = parent->keys[0];                 /* oddelovac z rodica sa stane prvym klucom */
        child->keys[1] = right_sib->keys[0];              /* kluc praveho surodenca sa stane druhym klucom */
        child->children[1] = right_sib->children[0];      /* prvy potomok praveho surodenca */
        child->children[2] = right_sib->children[1];      /* druhy potomok praveho surodenca */
        child->num_keys = 2;                               /* potomok ma teraz 2 kluce */

        /* odstranenie oddelovaca z rodica */
        for (int i = 0; i < parent->num_keys - 1; i++) {  /* posun klucov v rodicovi */
            parent->keys[i] = parent->keys[i + 1];        /* posun kluca dolava */
            parent->children[i + 1] = parent->children[i + 2]; /* posun potomka dolava */
        }
        parent->children[parent->num_keys] = NULL;  /* odstranenie posledneho ukazatela */
        parent->num_keys--;                          /* znizenie poctu klucov rodica */
        free(right_sib);                             /* uvolnenie praveho surodenca */
        return parent->num_keys == 0;                /* vrati true ak rodic podtiekol */
    }
}

/*
 * Rekurzivne vymazanie. Vrati true ak uzol podtecie.
 */
static bool tt_delete_recursive(TT_Node *node, int key, bool *was_deleted) {
    int pos;              /* pozicia kluca alebo potomka */
    bool found = false;   /* ci bol kluc najdeny v tomto uzle */

    for (pos = 0; pos < node->num_keys; pos++) {       /* prechod cez kluce uzla */
        if (key == node->keys[pos]) { found = true; break; } /* kluc najdeny */
        if (key < node->keys[pos]) break;               /* kluc je mensi, ideme do potomka */
    }

    if (tt_is_leaf(node)) {                             /* ak sme v liste */
        if (!found) return false;                       /* kluc sa nenasiel */
        *was_deleted = true;                            /* oznacenie ze sa vymazal */
        /* odstranenie kluca posunom dolava */
        for (int i = pos; i < node->num_keys - 1; i++) /* posun klucov */
            node->keys[i] = node->keys[i + 1];         /* posun kluca dolava */
        node->num_keys--;                               /* znizenie poctu klucov */
        return node->num_keys == 0;  /* podtecenie ak list ostane prazdny */
    }

    /* vnutorny uzol */
    int target_key = key;    /* kluc na vymazanie */
    int child_idx = pos;     /* index potomka pre zostup */

    if (found) {
        /* vymenime s in-order predchodcom (najvacsi kluc v lavom podstrome) */
        TT_Node *pred = node->children[pos];            /* zaciatok hladania predchodcu */
        while (!tt_is_leaf(pred))                        /* kym nie sme v liste */
            pred = pred->children[pred->num_keys];       /* ideme do najpraveho potomka */
        int pred_key = pred->keys[pred->num_keys - 1];  /* najvacsi kluc v liste */
        node->keys[pos] = pred_key;                      /* nahradenie kluca predchodcom */
        target_key = pred_key;                           /* teraz mazeme predchodcu */
        *was_deleted = true;                              /* oznacenie ze sa vymazal */
    }

    bool child_underflow = tt_delete_recursive(node->children[child_idx], target_key, was_deleted); /* rekurzivne vymazanie v potomkovi */
    if (!child_underflow) return false;                  /* ak potomok nepodtiekol, koniec */

    return tt_fix_underflow(node, child_idx);            /* oprava podtecenia */
}

/* Vymaze kluc z 2-3 stromu */
void tt_delete(TT_Tree *tree, int key) {
    if (!tree->root) return;                            /* ak je strom prazdny, nic nerobime */

    bool was_deleted = false;                           /* priznak ci sa kluc vymazal */
    bool underflow = tt_delete_recursive(tree->root, key, &was_deleted); /* rekurzivne vymazanie */

    if (!was_deleted) return;                           /* ak sa nic nevymazalo, koniec */
    tree->size--;                                       /* znizenie poctu prvkov */

    if (underflow) {                                    /* ak koren podtiekol */
        TT_Node *old_root = tree->root;                /* ulozenie stareho korena */
        tree->root = old_root->children[0];             /* jediny potomok sa stane novym korenom */
        free(old_root);                                 /* uvolnenie stareho korena */
    }
}

/* ========== Vypis (preorder, pre ladenie) ========== */

/* Rekurzivny vypis uzlov v preorder poradi */
void tt_print_node(TT_Node *node) {
    if (!node) return;                                  /* zakladny pripad: prazdny uzol */
    printf("[");                                        /* zaciatok vypisu uzla */
    for (int i = 0; i < node->num_keys; i++) {          /* prechod cez kluce uzla */
        if (i > 0) printf(",");                         /* oddelovac medzi klucmi */
        printf("%d", node->keys[i]);                    /* vypis kluca */
    }
    printf("] ");                                       /* koniec vypisu uzla */
    for (int i = 0; i <= node->num_keys; i++)           /* prechod cez potomkov */
        tt_print_node(node->children[i]);               /* rekurzivny vypis potomka */
}

#endif /* TT_TREE_C */ /* koniec ochrany pred viacnasobnym vlozenim */
