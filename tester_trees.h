/*
 * Tester stromov - Porovnanie 2-3 stromu a cerveno-cierneho stromu
 * Meria vlozenie, vyhladanie, vymazanie napriec viacerymi scenarmi a velkostami.
 * Vysledky sa vypisuju na konzolu a do tree_results.csv.
 *
 * Kompilovanie: gcc -O2 -o tester_trees -x c tester_trees.h -lm
 * Spustenie:    ./tester_trees
 */
#ifndef TESTER_TREES_H  /* ochrana pred viacnasobnym vlozenim */
#define TESTER_TREES_H  /* definicia ochrany */

#include "tt_tree.c"  /* zdrojovy kod 2-3 stromu */
#include "rb_tree.c"  /* zdrojovy kod cerveno-cierneho stromu */

#include <stdio.h>    /* standardny vstup/vystup */
#include <stdlib.h>   /* standardna kniznica (malloc, rand, srand) */
#include <string.h>   /* praca s retazcami */
#include <time.h>     /* meranie casu */

/* ========== Meranie casu ========== */

/* Funkcia na vypocet rozdielu casu v nanosekundach */
static double elapsed_ns(struct timespec *t_start, struct timespec *t_end) {
    return (t_end->tv_sec - t_start->tv_sec) * 1e9    /* rozdiel sekund prevedeny na nanosekundy */
         + (t_end->tv_nsec - t_start->tv_nsec);        /* plus rozdiel nanosekund */
}

/* ========== Genericke rozhranie ========== */

/* Struktura pre jednotne rozhranie stromov */
typedef struct {
    const char *name;                    /* nazov stromu */
    void *(*create)(void);              /* funkcia na vytvorenie */
    void (*insert)(void *, int);        /* funkcia na vlozenie kluca */
    bool (*search)(void *, int);        /* funkcia na vyhladanie kluca */
    void (*delete_key)(void *, int);    /* funkcia na vymazanie kluca */
    void (*destroy)(void *);            /* funkcia na zrusenie stromu */
    int  (*get_size)(void *);           /* funkcia na ziskanie poctu prvkov */
} TreeInterface;

/* --- Obalove funkcie pre 2-3 strom --- */
static void *wrap_tt_create(void)        { return tt_create(); }                 /* vytvorenie 2-3 stromu */
static void  wrap_tt_insert(void *t, int k) { tt_insert((TT_Tree *)t, k); }     /* vlozenie do 2-3 stromu */
static bool  wrap_tt_search(void *t, int k) { return tt_search((TT_Tree *)t, k); } /* vyhladanie v 2-3 strome */
static void  wrap_tt_delete(void *t, int k) { tt_delete((TT_Tree *)t, k); }     /* vymazanie z 2-3 stromu */
static void  wrap_tt_destroy(void *t)       { tt_destroy((TT_Tree *)t); }       /* zrusenie 2-3 stromu */
static int   wrap_tt_size(void *t)          { return ((TT_Tree *)t)->size; }     /* pocet prvkov v 2-3 strome */

/* --- Obalove funkcie pre cerveno-cierny strom --- */
static void *wrap_rb_create(void)        { return rb_create(); }                 /* vytvorenie RB stromu */
static void  wrap_rb_insert(void *t, int k) { rb_insert((RB_Tree *)t, k); }     /* vlozenie do RB stromu */
static bool  wrap_rb_search(void *t, int k) { return rb_search((RB_Tree *)t, k); } /* vyhladanie v RB strome */
static void  wrap_rb_delete(void *t, int k) { rb_delete((RB_Tree *)t, k); }     /* vymazanie z RB stromu */
static void  wrap_rb_destroy(void *t)       { rb_destroy((RB_Tree *)t); }       /* zrusenie RB stromu */
static int   wrap_rb_size(void *t)          { return ((RB_Tree *)t)->size; }     /* pocet prvkov v RB strome */

/* Pole vsetkych stromov s ich rozhraniami */
static TreeInterface TREES[] = {
    {"2-3 Tree",       wrap_tt_create, wrap_tt_insert, wrap_tt_search, wrap_tt_delete, wrap_tt_destroy, wrap_tt_size},       /* 2-3 strom */
    {"Red-Black Tree", wrap_rb_create, wrap_rb_insert, wrap_rb_search, wrap_rb_delete, wrap_rb_destroy, wrap_rb_size},       /* cerveno-cierny strom */
};
#define NUM_TREES 2 /* pocet stromov */

/* ========== Generovanie dat ========== */

/* Vygeneruje pole n nahodnych cisel */
static int *make_random_array(int n) {
    int *arr = (int *)malloc(n * sizeof(int)); /* alokacia pola */
    for (int i = 0; i < n; i++) arr[i] = rand(); /* naplnenie nahodnymi hodnotami */
    return arr; /* vratenie ukazatela na pole */
}

/* Vygeneruje pole s postupnostou 0, 1, 2, ..., n-1 */
static int *make_sequential_array(int n) {
    int *arr = (int *)malloc(n * sizeof(int)); /* alokacia pola */
    for (int i = 0; i < n; i++) arr[i] = i; /* naplnenie postupnymi hodnotami */
    return arr; /* vratenie ukazatela na pole */
}

/* Vygeneruje pole s opacnou postupnostou n-1, n-2, ..., 0 */
static int *make_reverse_array(int n) {
    int *arr = (int *)malloc(n * sizeof(int)); /* alokacia pola */
    for (int i = 0; i < n; i++) arr[i] = n - 1 - i; /* naplnenie opacnymi hodnotami */
    return arr; /* vratenie ukazatela na pole */
}

/* ========== Vysledok merania ========== */

/* Struktura na uchovavanie vysledkov jedneho merania stromu */
typedef struct {
    double ins_ns_per_op;  /* priemerny cas vlozenia v nanosekundach */
    double srch_ns_per_op; /* priemerny cas vyhladania v nanosekundach */
    double del_ns_per_op;  /* priemerny cas vymazania v nanosekundach */
} TreeBenchResult;

/* ========== Scenare ========== */

/* Scenar 1: Nahodne data */
static TreeBenchResult tree_bench_random(TreeInterface *ds, int N) {
    TreeBenchResult res = {0};     /* inicializacia vysledku na nuly */
    struct timespec t0, t1;        /* casove znamky pre meranie */
    void *obj = ds->create();      /* vytvorenie stromu */
    int *data = make_random_array(N); /* vygenerovanie nahodnych dat */

    clock_gettime(CLOCK_MONOTONIC, &t0); /* zaciatok merania vlozenia */
    for (int i = 0; i < N; i++) ds->insert(obj, data[i]); /* vlozenie vsetkych prvkov */
    clock_gettime(CLOCK_MONOTONIC, &t1); /* koniec merania vlozenia */
    res.ins_ns_per_op = elapsed_ns(&t0, &t1) / N; /* vypocet priemerneho casu vlozenia */

    clock_gettime(CLOCK_MONOTONIC, &t0); /* zaciatok merania vyhladavania */
    for (int i = 0; i < N / 2; i++) ds->search(obj, data[i]); /* vyhladanie existujucich */
    for (int i = 0; i < N / 2; i++) ds->search(obj, rand() + N); /* vyhladanie neexistujucich */
    clock_gettime(CLOCK_MONOTONIC, &t1); /* koniec merania vyhladavania */
    res.srch_ns_per_op = elapsed_ns(&t0, &t1) / N; /* priemerny cas vyhladania */

    clock_gettime(CLOCK_MONOTONIC, &t0); /* zaciatok merania vymazania */
    for (int i = 0; i < N / 2; i++) ds->delete_key(obj, data[i]); /* vymazanie prvej polovice */
    clock_gettime(CLOCK_MONOTONIC, &t1); /* koniec merania vymazania */
    res.del_ns_per_op = elapsed_ns(&t0, &t1) / (N / 2); /* priemerny cas vymazania */

    free(data);          /* uvolnenie pola dat */
    ds->destroy(obj);    /* zrusenie stromu */
    return res;          /* vratenie vysledku */
}

/* Scenar 2: Sekvencne (usporiadane) data */
static TreeBenchResult tree_bench_sequential(TreeInterface *ds, int N) {
    TreeBenchResult res = {0};     /* inicializacia vysledku na nuly */
    struct timespec t0, t1;        /* casove znamky pre meranie */
    void *obj = ds->create();      /* vytvorenie stromu */
    int *data = make_sequential_array(N); /* vygenerovanie sekvencnych dat */

    clock_gettime(CLOCK_MONOTONIC, &t0); /* zaciatok merania vlozenia */
    for (int i = 0; i < N; i++) ds->insert(obj, data[i]); /* vlozenie vsetkych prvkov v poradi */
    clock_gettime(CLOCK_MONOTONIC, &t1); /* koniec merania vlozenia */
    res.ins_ns_per_op = elapsed_ns(&t0, &t1) / N; /* priemerny cas vlozenia */

    clock_gettime(CLOCK_MONOTONIC, &t0); /* zaciatok merania vyhladavania */
    for (int i = 0; i < N; i++) ds->search(obj, data[i]); /* vyhladanie kazdeho prvku */
    clock_gettime(CLOCK_MONOTONIC, &t1); /* koniec merania vyhladavania */
    res.srch_ns_per_op = elapsed_ns(&t0, &t1) / N; /* priemerny cas vyhladania */

    clock_gettime(CLOCK_MONOTONIC, &t0); /* zaciatok merania vymazania */
    for (int i = 0; i < N; i++) ds->delete_key(obj, data[i]); /* vymazanie kazdeho prvku */
    clock_gettime(CLOCK_MONOTONIC, &t1); /* koniec merania vymazania */
    res.del_ns_per_op = elapsed_ns(&t0, &t1) / N; /* priemerny cas vymazania */

    free(data);          /* uvolnenie pola dat */
    ds->destroy(obj);    /* zrusenie stromu */
    return res;          /* vratenie vysledku */
}

/* Scenar 3: Opacne usporiadane data */
static TreeBenchResult tree_bench_reverse(TreeInterface *ds, int N) {
    TreeBenchResult res = {0};     /* inicializacia vysledku na nuly */
    struct timespec t0, t1;        /* casove znamky pre meranie */
    void *obj = ds->create();      /* vytvorenie stromu */
    int *data = make_reverse_array(N); /* vygenerovanie opacne usporiadanych dat */

    clock_gettime(CLOCK_MONOTONIC, &t0); /* zaciatok merania vlozenia */
    for (int i = 0; i < N; i++) ds->insert(obj, data[i]); /* vlozenie vsetkych prvkov */
    clock_gettime(CLOCK_MONOTONIC, &t1); /* koniec merania vlozenia */
    res.ins_ns_per_op = elapsed_ns(&t0, &t1) / N; /* priemerny cas vlozenia */

    clock_gettime(CLOCK_MONOTONIC, &t0); /* zaciatok merania vyhladavania */
    for (int i = 0; i < N; i++) ds->search(obj, data[i]); /* vyhladanie kazdeho prvku */
    clock_gettime(CLOCK_MONOTONIC, &t1); /* koniec merania vyhladavania */
    res.srch_ns_per_op = elapsed_ns(&t0, &t1) / N; /* priemerny cas vyhladania */

    clock_gettime(CLOCK_MONOTONIC, &t0); /* zaciatok merania vymazania */
    for (int i = 0; i < N; i++) ds->delete_key(obj, data[i]); /* vymazanie kazdeho prvku */
    clock_gettime(CLOCK_MONOTONIC, &t1); /* koniec merania vymazania */
    res.del_ns_per_op = elapsed_ns(&t0, &t1) / N; /* priemerny cas vymazania */

    free(data);          /* uvolnenie pola dat */
    ds->destroy(obj);    /* zrusenie stromu */
    return res;          /* vratenie vysledku */
}

/* Scenar 4: Zmiesane operacie (40% vlozenie, 40% vyhladanie, 20% vymazanie) */
static TreeBenchResult tree_bench_mixed(TreeInterface *ds, int N) {
    TreeBenchResult res = {0};     /* inicializacia vysledku na nuly */
    struct timespec t0, t1;        /* casove znamky pre meranie */
    void *obj = ds->create();      /* vytvorenie stromu */
    int next_val = 0;              /* dalsia hodnota na vlozenie */
    double ins_total = 0, srch_total = 0, del_total = 0; /* celkove casy operacii */
    int ins_cnt = 0, srch_cnt = 0, del_cnt = 0; /* pocitadla operacii */

    for (int i = 0; i < N; i++) { /* cyklus cez N operacii */
        int op = rand() % 100; /* nahodne cislo od 0 do 99 pre vyber operacie */
        if (op < 40) { /* 40% pravdepodobnost vlozenia */
            int val = next_val++; /* inkrementacia hodnoty na vlozenie */
            clock_gettime(CLOCK_MONOTONIC, &t0); /* zaciatok merania */
            ds->insert(obj, val); /* vlozenie hodnoty */
            clock_gettime(CLOCK_MONOTONIC, &t1); /* koniec merania */
            ins_total += elapsed_ns(&t0, &t1); /* pripocitanie casu vlozenia */
            ins_cnt++; /* zvysenie pocitadla vlozeni */
        } else if (op < 80) { /* 40% pravdepodobnost vyhladania */
            int val = rand() % (next_val + 1); /* nahodna hodnota na vyhladanie */
            clock_gettime(CLOCK_MONOTONIC, &t0); /* zaciatok merania */
            ds->search(obj, val); /* vyhladanie hodnoty */
            clock_gettime(CLOCK_MONOTONIC, &t1); /* koniec merania */
            srch_total += elapsed_ns(&t0, &t1); /* pripocitanie casu vyhladania */
            srch_cnt++; /* zvysenie pocitadla vyhladani */
        } else { /* 20% pravdepodobnost vymazania */
            int val = rand() % (next_val + 1); /* nahodna hodnota na vymazanie */
            clock_gettime(CLOCK_MONOTONIC, &t0); /* zaciatok merania */
            ds->delete_key(obj, val); /* vymazanie hodnoty */
            clock_gettime(CLOCK_MONOTONIC, &t1); /* koniec merania */
            del_total += elapsed_ns(&t0, &t1); /* pripocitanie casu vymazania */
            del_cnt++; /* zvysenie pocitadla vymazani */
        }
    }

    res.ins_ns_per_op = ins_cnt > 0 ? ins_total / ins_cnt : 0; /* priemerny cas vlozenia */
    res.srch_ns_per_op = srch_cnt > 0 ? srch_total / srch_cnt : 0; /* priemerny cas vyhladania */
    res.del_ns_per_op = del_cnt > 0 ? del_total / del_cnt : 0; /* priemerny cas vymazania */

    ds->destroy(obj); /* zrusenie stromu */
    return res;       /* vratenie vysledku */
}

/* Scenar 5: Intenzivne mazanie */
static TreeBenchResult tree_bench_delete_heavy(TreeInterface *ds, int N) {
    TreeBenchResult res = {0};     /* inicializacia vysledku na nuly */
    struct timespec t0, t1;        /* casove znamky pre meranie */
    void *obj = ds->create();      /* vytvorenie stromu */
    int *data = make_random_array(N); /* vygenerovanie nahodnych dat */

    /* najprv naplnime strukturu */
    for (int i = 0; i < N; i++) ds->insert(obj, data[i]); /* vlozenie vsetkych prvkov */

    /* vymazanie 80% */
    int del_n = (int)(N * 0.8); /* vypocet poctu prvkov na vymazanie */
    clock_gettime(CLOCK_MONOTONIC, &t0); /* zaciatok merania vymazania */
    for (int i = 0; i < del_n; i++) ds->delete_key(obj, data[i]); /* vymazanie 80% prvkov */
    clock_gettime(CLOCK_MONOTONIC, &t1); /* koniec merania vymazania */
    res.del_ns_per_op = elapsed_ns(&t0, &t1) / del_n; /* priemerny cas vymazania */

    /* opatovne vlozenie 50% novych hodnot */
    int reins_n = N / 2; /* pocet novych prvkov na vlozenie */
    clock_gettime(CLOCK_MONOTONIC, &t0); /* zaciatok merania opatovneho vlozenia */
    for (int i = 0; i < reins_n; i++) ds->insert(obj, rand()); /* vlozenie novych nahodnych hodnot */
    clock_gettime(CLOCK_MONOTONIC, &t1); /* koniec merania opatovneho vlozenia */
    res.ins_ns_per_op = elapsed_ns(&t0, &t1) / reins_n; /* priemerny cas vlozenia */

    /* vyhladanie N hodnot */
    clock_gettime(CLOCK_MONOTONIC, &t0); /* zaciatok merania vyhladavania */
    for (int i = 0; i < N; i++) ds->search(obj, data[i]); /* vyhladanie povodnych hodnot */
    clock_gettime(CLOCK_MONOTONIC, &t1); /* koniec merania vyhladavania */
    res.srch_ns_per_op = elapsed_ns(&t0, &t1) / N; /* priemerny cas vyhladania */

    free(data);          /* uvolnenie pola dat */
    ds->destroy(obj);    /* zrusenie stromu */
    return res;          /* vratenie vysledku */
}

/* ========== Overenie spravnosti ========== */

/* Funkcia na overenie spravnosti stromovej struktury */
static int verify_tree(TreeInterface *ds) {
    void *obj = ds->create(); /* vytvorenie stromu */
    int errors = 0;           /* pocitadlo chyb */
    int N = 1000;             /* pocet testovacich prvkov */

    /* vlozenie hodnot 0 az N-1 */
    for (int i = 0; i < N; i++) ds->insert(obj, i); /* vlozenie kazdeho prvku */
    if (ds->get_size(obj) != N) { /* kontrola poctu prvkov */
        printf("  ERROR: %s size after insert = %d (expected %d)\n",
               ds->name, ds->get_size(obj), N); /* vypis chyby */
        errors++; /* zvysenie pocitadla chyb */
    }

    /* vyhladanie vsetkych vlozenych */
    for (int i = 0; i < N; i++) { /* prechod cez vsetky prvky */
        if (!ds->search(obj, i)) { /* ak sa prvok nenasiel */
            printf("  ERROR: %s search(%d) failed\n", ds->name, i); /* vypis chyby */
            errors++; /* zvysenie pocitadla chyb */
            break;    /* ukoncenie cyklu */
        }
    }

    /* vyhladanie neexistujucich */
    for (int i = N; i < N + 100; i++) { /* hodnoty ktore neboli vlozene */
        if (ds->search(obj, i)) { /* ak sa prvok nasiel (chyba) */
            printf("  ERROR: %s search(%d) found phantom\n", ds->name, i); /* vypis chyby */
            errors++; /* zvysenie pocitadla chyb */
            break;    /* ukoncenie cyklu */
        }
    }

    /* duplicitne vlozenie nesmie zmenit velkost */
    ds->insert(obj, 0); /* pokus o vlozenie existujuceho prvku */
    if (ds->get_size(obj) != N) { /* kontrola ze velkost sa nezmenila */
        printf("  ERROR: %s size changed on duplicate = %d\n", ds->name, ds->get_size(obj)); /* vypis chyby */
        errors++; /* zvysenie pocitadla chyb */
    }

    /* vymazanie parnych cisel */
    for (int i = 0; i < N; i += 2) ds->delete_key(obj, i); /* vymazanie 0, 2, 4, ... */
    if (ds->get_size(obj) != N / 2) { /* kontrola ze ostala polovica */
        printf("  ERROR: %s size after partial delete = %d (expected %d)\n",
               ds->name, ds->get_size(obj), N / 2); /* vypis chyby */
        errors++; /* zvysenie pocitadla chyb */
    }

    /* overenie: parne su prec, neparne ostali */
    for (int i = 0; i < N; i++) { /* prechod cez vsetky hodnoty */
        bool found = ds->search(obj, i); /* vyhladanie prvku */
        bool expected = (i % 2 == 1); /* ocakavany vysledok (len neparne) */
        if (found != expected) { /* ak vysledok nesedi */
            printf("  ERROR: %s search(%d) = %d (expected %d)\n",
                   ds->name, i, found, expected); /* vypis chyby */
            errors++; /* zvysenie pocitadla chyb */
            break;    /* ukoncenie cyklu */
        }
    }

    /* vymazanie neexistujucich nesmie spadnut */
    ds->delete_key(obj, -1);    /* vymazanie zapornej hodnoty */
    ds->delete_key(obj, N + 1); /* vymazanie hodnoty vacsej nez N */

    /* vymazanie zvysnich */
    for (int i = 1; i < N; i += 2) ds->delete_key(obj, i); /* vymazanie 1, 3, 5, ... */
    if (ds->get_size(obj) != 0) { /* kontrola ze strom je prazdny */
        printf("  ERROR: %s size after full delete = %d\n", ds->name, ds->get_size(obj)); /* vypis chyby */
        errors++; /* zvysenie pocitadla chyb */
    }

    ds->destroy(obj); /* zrusenie stromu */
    return errors;    /* vratenie poctu chyb */
}

/* ========== Tabulka scenarov ========== */

/* Typ ukazatela na funkciu scenara stromu */
typedef TreeBenchResult (*TreeScenarioFn)(TreeInterface *ds, int N);

/* Struktura pre scenar stromu */
typedef struct {
    const char *name;    /* nazov scenara */
    TreeScenarioFn fn;   /* funkcia scenara */
} TreeScenario;

/* Pole vsetkych scenarov */
static TreeScenario TREE_SCENARIOS[] = {
    {"Random",       tree_bench_random},       /* scenar s nahodnymi datami */
    {"Sequential",   tree_bench_sequential},   /* scenar so sekvencnymi datami */
    {"Reverse",      tree_bench_reverse},      /* scenar s opacne usporiadanymi datami */
    {"Mixed",        tree_bench_mixed},        /* scenar so zmiesanymi operaciami */
    {"Delete-heavy", tree_bench_delete_heavy}, /* scenar s intenzivnym mazanim */
};
#define NUM_TREE_SCENARIOS 5 /* pocet scenarov */

/* Velkosti testovacich dat */
static int TREE_SIZES[] = {1000, 10000, 100000, 1000000, 10000000};
#define NUM_TREE_SIZES 5 /* pocet velkosti */

/* ========== Hlavny program ========== */

/* Hlavna funkcia programu */
int main(void) {
    srand(42); /* inicializacia generatora nahodnych cisel s pevnym seedom pre reprodukovatelnost */

    /* Vypis hlavicky */
    printf("================================================================\n");
    printf("  Tree Comparison: 2-3 Tree vs Red-Black Tree\n"); /* nazov porovnania */
    printf("================================================================\n\n");

    /* overenie spravnosti */
    printf("--- Correctness Verification ---\n"); /* hlavicka overenia */
    int total_err = 0; /* celkovy pocet chyb */
    for (int d = 0; d < NUM_TREES; d++) { /* prechod cez vsetky stromy */
        int err = verify_tree(&TREES[d]); /* overenie jedneho stromu */
        printf("  %-16s: %s\n", TREES[d].name, err == 0 ? "OK" : "FAILED"); /* vypis vysledku */
        total_err += err; /* pripocitanie chyb */
    }
    if (total_err > 0) { /* ak boli nejake chyby */
        printf("\nVerification failed (%d errors). Stopping.\n", total_err); /* vypis chyby */
        return 1; /* ukoncenie programu s chybou */
    }
    printf("Both trees passed all correctness checks.\n\n"); /* oba stromy presli */

    /* CSV vystup */
    FILE *csv = fopen("tree_results.csv", "w"); /* otvorenie CSV suboru na zapis */
    if (!csv) { perror("Cannot create tree_results.csv"); return 1; } /* kontrola chyby otvorenia */
    fprintf(csv, "Scenario;Size;DataStructure;AvgInsert_ns;AvgSearch_ns;AvgDelete_ns\n"); /* hlavicka CSV */

    /* spustenie merani */
    for (int s = 0; s < NUM_TREE_SCENARIOS; s++) { /* prechod cez vsetky scenare */
        printf("=== Scenario: %s ===\n", TREE_SCENARIOS[s].name); /* vypis nazvu scenara */
        for (int z = 0; z < NUM_TREE_SIZES; z++) { /* prechod cez vsetky velkosti */
            int N = TREE_SIZES[z]; /* aktualna velkost */
            printf("\n  N = %d\n", N); /* vypis velkosti */
            printf("  %-16s | %12s | %12s | %12s\n",
                   "Structure", "Insert ns/op", "Search ns/op", "Delete ns/op"); /* hlavicka tabulky */
            printf("  %-16s-+-%12s-+-%12s-+-%12s\n",
                   "----------------", "------------", "------------", "------------"); /* oddelovac tabulky */

            for (int d = 0; d < NUM_TREES; d++) { /* prechod cez vsetky stromy */
                srand(42); /* reset seedu pre kazde meranie */
                TreeBenchResult r = TREE_SCENARIOS[s].fn(&TREES[d], N); /* spustenie merania */
                printf("  %-16s | %12.1f | %12.1f | %12.1f\n",
                       TREES[d].name, r.ins_ns_per_op, r.srch_ns_per_op, r.del_ns_per_op); /* vypis vysledkov */

                /* CSV s desatinnou ciarkou pre slovensky format */
                char c_ins[32], c_srch[32], c_del[32]; /* buffery pre formatovanie cisel */
                snprintf(c_ins, sizeof(c_ins), "%.1f", r.ins_ns_per_op); /* formatovanie casu vlozenia */
                snprintf(c_srch, sizeof(c_srch), "%.1f", r.srch_ns_per_op); /* formatovanie casu vyhladania */
                snprintf(c_del, sizeof(c_del), "%.1f", r.del_ns_per_op); /* formatovanie casu vymazania */
                for (char *p = c_ins; *p; p++) if (*p == '.') *p = ','; /* nahradenie bodky ciarkou v case vlozenia */
                for (char *p = c_srch; *p; p++) if (*p == '.') *p = ','; /* nahradenie bodky ciarkou v case vyhladania */
                for (char *p = c_del; *p; p++) if (*p == '.') *p = ','; /* nahradenie bodky ciarkou v case vymazania */
                fprintf(csv, "%s;%d;%s;%s;%s;%s\n",
                        TREE_SCENARIOS[s].name, N, TREES[d].name, c_ins, c_srch, c_del); /* zapis riadku do CSV */
            }
        }
        printf("\n"); /* prazdny riadok medzi scenarmi */
    }

    fclose(csv); /* zatvorenie CSV suboru */
    printf("Results saved to tree_results.csv\n"); /* informacia o ulozeni */
    printf("Done.\n"); /* koniec programu */
    return 0; /* uspesne ukoncenie */
}

#endif /* TESTER_TREES_H */ /* koniec ochrany pred viacnasobnym vlozenim */
