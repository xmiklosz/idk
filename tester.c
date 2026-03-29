/*
 * Tester - Porovnanie implementacii dynamickych mnozin
 * Porovnava: 2-3 strom, Cerveno-cierny strom, Hasovanie retazenim, Hasovanie dvojitym hasovanim
 *
 * Kompilovanie: gcc -O2 -o tester tester.c -lm
 * Spustenie:    ./tester
 *
 * Vysledky sa vypisuju na konzolu a do CSV suboru (results.csv) pre tvorbu grafov.
 */
#include "tt_tree.h"   /* hlavickovy subor pre 2-3 strom */
#include "rb_tree.h"   /* hlavickovy subor pre cerveno-cierny strom */
#include "hc_table.h"  /* hlavickovy subor pre hasovaciu tabulku s retazenim */
#include "hd_table.h"  /* hlavickovy subor pre hasovaciu tabulku s dvojitym hasovanim */

#include <stdio.h>     /* standardny vstup/vystup */
#include <stdlib.h>    /* standardna kniznica (malloc, rand, srand) */
#include <string.h>    /* praca s retazcami */
#include <time.h>      /* meranie casu */
#include <math.h>      /* matematicke funkcie */

/* ========== Meranie casu ========== */

/* Funkcia na vypocet rozdielu casu v nanosekundach */
static double time_ns(struct timespec *start, struct timespec *end) {
    /* vrati rozdiel v nanosekundach medzi koncom a zaciatkom merania */
    return (end->tv_sec - start->tv_sec) * 1e9 + (end->tv_nsec - start->tv_nsec);
}

/* ========== Genericke rozhranie ========== */

/* Struktura pre jednotne rozhranie vsetkych datovych struktur */
typedef struct {
    const char *name;                    /* nazov datovej struktury */
    void *(*create)(void);              /* funkcia na vytvorenie */
    void (*insert)(void *, int);        /* funkcia na vlozenie kluca */
    bool (*search)(void *, int);        /* funkcia na vyhladanie kluca */
    void (*delete_fn)(void *, int);     /* funkcia na vymazanie kluca */
    void (*destroy)(void *);            /* funkcia na zrusenie struktury */
    int  (*get_count)(void *);          /* funkcia na ziskanie poctu prvkov */
} DS_Interface;

/* --- Obalove funkcie pre 2-3 strom --- */
static void *w_tt_create(void) { return tt_create(); }                          /* vytvorenie 2-3 stromu */
static void  w_tt_insert(void *t, int k) { tt_insert((TT_Tree *)t, k); }       /* vlozenie do 2-3 stromu */
static bool  w_tt_search(void *t, int k) { return tt_search((TT_Tree *)t, k); } /* vyhladanie v 2-3 strome */
static void  w_tt_delete(void *t, int k) { tt_delete((TT_Tree *)t, k); }       /* vymazanie z 2-3 stromu */
static void  w_tt_destroy(void *t) { tt_destroy((TT_Tree *)t); }               /* zrusenie 2-3 stromu */
static int   w_tt_count(void *t) { return ((TT_Tree *)t)->count; }             /* pocet prvkov v 2-3 strome */

/* --- Obalove funkcie pre cerveno-cierny strom --- */
static void *w_rb_create(void) { return rb_create(); }                          /* vytvorenie RB stromu */
static void  w_rb_insert(void *t, int k) { rb_insert((RB_Tree *)t, k); }       /* vlozenie do RB stromu */
static bool  w_rb_search(void *t, int k) { return rb_search((RB_Tree *)t, k); } /* vyhladanie v RB strome */
static void  w_rb_delete(void *t, int k) { rb_delete((RB_Tree *)t, k); }       /* vymazanie z RB stromu */
static void  w_rb_destroy(void *t) { rb_destroy((RB_Tree *)t); }               /* zrusenie RB stromu */
static int   w_rb_count(void *t) { return ((RB_Tree *)t)->count; }             /* pocet prvkov v RB strome */

/* --- Obalove funkcie pre hasovaciu tabulku s retazenim --- */
static void *w_hc_create(void) { return hc_create(); }                          /* vytvorenie HC tabulky */
static void  w_hc_insert(void *t, int k) { hc_insert((HC_Table *)t, k); }       /* vlozenie do HC tabulky */
static bool  w_hc_search(void *t, int k) { return hc_search((HC_Table *)t, k); } /* vyhladanie v HC tabulke */
static void  w_hc_delete(void *t, int k) { hc_delete((HC_Table *)t, k); }       /* vymazanie z HC tabulky */
static void  w_hc_destroy(void *t) { hc_destroy((HC_Table *)t); }               /* zrusenie HC tabulky */
static int   w_hc_count(void *t) { return ((HC_Table *)t)->count; }             /* pocet prvkov v HC tabulke */

/* --- Obalove funkcie pre hasovaciu tabulku s dvojitym hasovanim --- */
static void *w_hd_create(void) { return hd_create(); }                          /* vytvorenie HD tabulky */
static void  w_hd_insert(void *t, int k) { hd_insert((HD_Table *)t, k); }       /* vlozenie do HD tabulky */
static bool  w_hd_search(void *t, int k) { return hd_search((HD_Table *)t, k); } /* vyhladanie v HD tabulke */
static void  w_hd_delete(void *t, int k) { hd_delete((HD_Table *)t, k); }       /* vymazanie z HD tabulky */
static void  w_hd_destroy(void *t) { hd_destroy((HD_Table *)t); }               /* zrusenie HD tabulky */
static int   w_hd_count(void *t) { return ((HD_Table *)t)->count; }             /* pocet prvkov v HD tabulke */

/* Pole vsetkych datovych struktur s ich rozhraniami */
static DS_Interface ALL_DS[] = {
    {"2-3 Tree",       w_tt_create, w_tt_insert, w_tt_search, w_tt_delete, w_tt_destroy, w_tt_count},       /* 2-3 strom */
    {"Red-Black Tree", w_rb_create, w_rb_insert, w_rb_search, w_rb_delete, w_rb_destroy, w_rb_count},       /* cerveno-cierny strom */
    {"Hash Chain",     w_hc_create, w_hc_insert, w_hc_search, w_hc_delete, w_hc_destroy, w_hc_count},       /* hasovacia tabulka s retazenim */
    {"Hash Double",    w_hd_create, w_hd_insert, w_hd_search, w_hd_delete, w_hd_destroy, w_hd_count},       /* hasovacia tabulka s dvojitym hasovanim */
};
#define NUM_DS 4 /* pocet datovych struktur */

/* ========== Generovanie dat ========== */

/* Vygeneruje pole n nahodnych cisel */
static int *generate_random(int n) {
    int *arr = (int *)malloc(n * sizeof(int)); /* alokacia pola */
    for (int i = 0; i < n; i++) arr[i] = rand(); /* naplnenie nahodnymi hodnotami */
    return arr; /* vratenie ukazatela na pole */
}

/* Vygeneruje pole s postupnostou 0, 1, 2, ..., n-1 */
static int *generate_sequential(int n) {
    int *arr = (int *)malloc(n * sizeof(int)); /* alokacia pola */
    for (int i = 0; i < n; i++) arr[i] = i; /* naplnenie postupnymi hodnotami */
    return arr; /* vratenie ukazatela na pole */
}

/* Vygeneruje pole s opacnou postupnostou n-1, n-2, ..., 0 */
static int *generate_reverse(int n) {
    int *arr = (int *)malloc(n * sizeof(int)); /* alokacia pola */
    for (int i = 0; i < n; i++) arr[i] = n - 1 - i; /* naplnenie opacnymi hodnotami */
    return arr; /* vratenie ukazatela na pole */
}

/* Nahodne premiesanie prvkov pola (Fisher-Yates algoritmus) */
static void shuffle(int *arr, int n) {
    for (int i = n - 1; i > 0; i--) { /* prechod od konca pola */
        int j = rand() % (i + 1); /* nahodny index od 0 po i */
        int tmp = arr[i]; arr[i] = arr[j]; arr[j] = tmp; /* vymena prvkov */
    }
}

/* ========== Vysledok merania ========== */

/* Struktura na uchovavanie vysledkov jedneho merania */
typedef struct {
    double insert_ns;          /* priemerny cas vlozenia v nanosekundach */
    double search_ns;          /* priemerny cas vyhladania v nanosekundach */
    double delete_ns;          /* priemerny cas vymazania v nanosekundach */
    int count_after_insert;    /* pocet prvkov po vlozeni */
    int count_after_delete;    /* pocet prvkov po vymazani */
} BenchResult;

/* ========== Scenare ========== */

/*
 * Scenar 1: Nahodne data
 * Vlozi N nahodnych hodnot, vyhlada N (mix existujucich a neexistujucich), vymaze N/2
 */
static BenchResult run_random(DS_Interface *ds, int N) {
    BenchResult res = {0};         /* inicializacia vysledku na nuly */
    struct timespec t0, t1;        /* casove znamky pre meranie */
    void *obj = ds->create();      /* vytvorenie datovej struktury */

    int *data = generate_random(N); /* vygenerovanie nahodnych dat */

    /* Vlozenie */
    clock_gettime(CLOCK_MONOTONIC, &t0); /* zaciatok merania vlozenia */
    for (int i = 0; i < N; i++) ds->insert(obj, data[i]); /* vlozenie vsetkych prvkov */
    clock_gettime(CLOCK_MONOTONIC, &t1); /* koniec merania vlozenia */
    res.insert_ns = time_ns(&t0, &t1) / N; /* vypocet priemerneho casu vlozenia */
    res.count_after_insert = ds->get_count(obj); /* ulozenie poctu prvkov po vlozeni */

    /* Vyhladavanie (polovica existujucich, polovica nahodnych) */
    clock_gettime(CLOCK_MONOTONIC, &t0); /* zaciatok merania vyhladavania */
    for (int i = 0; i < N / 2; i++) ds->search(obj, data[i]); /* vyhladanie existujucich */
    for (int i = 0; i < N / 2; i++) ds->search(obj, rand() + N); /* vyhladanie neexistujucich */
    clock_gettime(CLOCK_MONOTONIC, &t1); /* koniec merania vyhladavania */
    res.search_ns = time_ns(&t0, &t1) / N; /* vypocet priemerneho casu vyhladania */

    /* Vymazanie polovice */
    clock_gettime(CLOCK_MONOTONIC, &t0); /* zaciatok merania vymazania */
    for (int i = 0; i < N / 2; i++) ds->delete_fn(obj, data[i]); /* vymazanie prvej polovice */
    clock_gettime(CLOCK_MONOTONIC, &t1); /* koniec merania vymazania */
    res.delete_ns = time_ns(&t0, &t1) / (N / 2); /* vypocet priemerneho casu vymazania */
    res.count_after_delete = ds->get_count(obj); /* ulozenie poctu prvkov po vymazani */

    free(data);          /* uvolnenie pola dat */
    ds->destroy(obj);    /* zrusenie datovej struktury */
    return res;          /* vratenie vysledku */
}

/*
 * Scenar 2: Sekvencne (usporiadane) data
 * Vlozi 0..N-1, vyhlada vsetky, vymaze vsetky
 */
static BenchResult run_sequential(DS_Interface *ds, int N) {
    BenchResult res = {0};         /* inicializacia vysledku na nuly */
    struct timespec t0, t1;        /* casove znamky pre meranie */
    void *obj = ds->create();      /* vytvorenie datovej struktury */

    int *data = generate_sequential(N); /* vygenerovanie sekvencnych dat */

    /* Vlozenie v poradi */
    clock_gettime(CLOCK_MONOTONIC, &t0); /* zaciatok merania vlozenia */
    for (int i = 0; i < N; i++) ds->insert(obj, data[i]); /* vlozenie vsetkych prvkov v poradi */
    clock_gettime(CLOCK_MONOTONIC, &t1); /* koniec merania vlozenia */
    res.insert_ns = time_ns(&t0, &t1) / N; /* vypocet priemerneho casu vlozenia */
    res.count_after_insert = ds->get_count(obj); /* ulozenie poctu prvkov po vlozeni */

    /* Vyhladanie vsetkych */
    clock_gettime(CLOCK_MONOTONIC, &t0); /* zaciatok merania vyhladavania */
    for (int i = 0; i < N; i++) ds->search(obj, data[i]); /* vyhladanie kazdeho prvku */
    clock_gettime(CLOCK_MONOTONIC, &t1); /* koniec merania vyhladavania */
    res.search_ns = time_ns(&t0, &t1) / N; /* vypocet priemerneho casu vyhladania */

    /* Vymazanie vsetkych */
    clock_gettime(CLOCK_MONOTONIC, &t0); /* zaciatok merania vymazania */
    for (int i = 0; i < N; i++) ds->delete_fn(obj, data[i]); /* vymazanie kazdeho prvku */
    clock_gettime(CLOCK_MONOTONIC, &t1); /* koniec merania vymazania */
    res.delete_ns = time_ns(&t0, &t1) / N; /* vypocet priemerneho casu vymazania */
    res.count_after_delete = ds->get_count(obj); /* ulozenie poctu prvkov po vymazani */

    free(data);          /* uvolnenie pola dat */
    ds->destroy(obj);    /* zrusenie datovej struktury */
    return res;          /* vratenie vysledku */
}

/*
 * Scenar 3: Opacne usporiadane data
 * Vlozi N-1..0, vyhlada vsetky, vymaze vsetky
 */
static BenchResult run_reverse(DS_Interface *ds, int N) {
    BenchResult res = {0};         /* inicializacia vysledku na nuly */
    struct timespec t0, t1;        /* casove znamky pre meranie */
    void *obj = ds->create();      /* vytvorenie datovej struktury */

    int *data = generate_reverse(N); /* vygenerovanie opacne usporiadanych dat */

    clock_gettime(CLOCK_MONOTONIC, &t0); /* zaciatok merania vlozenia */
    for (int i = 0; i < N; i++) ds->insert(obj, data[i]); /* vlozenie vsetkych prvkov */
    clock_gettime(CLOCK_MONOTONIC, &t1); /* koniec merania vlozenia */
    res.insert_ns = time_ns(&t0, &t1) / N; /* vypocet priemerneho casu vlozenia */
    res.count_after_insert = ds->get_count(obj); /* ulozenie poctu prvkov po vlozeni */

    clock_gettime(CLOCK_MONOTONIC, &t0); /* zaciatok merania vyhladavania */
    for (int i = 0; i < N; i++) ds->search(obj, data[i]); /* vyhladanie kazdeho prvku */
    clock_gettime(CLOCK_MONOTONIC, &t1); /* koniec merania vyhladavania */
    res.search_ns = time_ns(&t0, &t1) / N; /* vypocet priemerneho casu vyhladania */

    clock_gettime(CLOCK_MONOTONIC, &t0); /* zaciatok merania vymazania */
    for (int i = 0; i < N; i++) ds->delete_fn(obj, data[i]); /* vymazanie kazdeho prvku */
    clock_gettime(CLOCK_MONOTONIC, &t1); /* koniec merania vymazania */
    res.delete_ns = time_ns(&t0, &t1) / N; /* vypocet priemerneho casu vymazania */
    res.count_after_delete = ds->get_count(obj); /* ulozenie poctu prvkov po vymazani */

    free(data);          /* uvolnenie pola dat */
    ds->destroy(obj);    /* zrusenie datovej struktury */
    return res;          /* vratenie vysledku */
}

/*
 * Scenar 4: Zmiesane operacie
 * Striedajuce sa vlozenie/vyhladanie/vymazanie (rozdelenie 40/40/20)
 */
static BenchResult run_mixed(DS_Interface *ds, int N) {
    BenchResult res = {0};         /* inicializacia vysledku na nuly */
    struct timespec t0, t1;        /* casove znamky pre meranie */
    void *obj = ds->create();      /* vytvorenie datovej struktury */

    int ins_count = 0, srch_count = 0, del_count = 0; /* pocitadla operacii */
    double ins_total = 0, srch_total = 0, del_total = 0; /* celkove casy operacii */
    int next_val = 0; /* dalsia hodnota na vlozenie */

    for (int i = 0; i < N; i++) { /* cyklus cez N operacii */
        int op = rand() % 100; /* nahodne cislo od 0 do 99 pre vyber operacie */
        if (op < 40) {
            /* Vlozenie (40% pravdepodobnost) */
            int val = next_val++; /* inkrementacia hodnoty na vlozenie */
            clock_gettime(CLOCK_MONOTONIC, &t0); /* zaciatok merania */
            ds->insert(obj, val); /* vlozenie hodnoty */
            clock_gettime(CLOCK_MONOTONIC, &t1); /* koniec merania */
            ins_total += time_ns(&t0, &t1); /* pripocitanie casu vlozenia */
            ins_count++; /* zvysenie pocitadla vlozeni */
        } else if (op < 80) {
            /* Vyhladavanie (40% pravdepodobnost) */
            int val = rand() % (next_val + 1); /* nahodna hodnota na vyhladanie */
            clock_gettime(CLOCK_MONOTONIC, &t0); /* zaciatok merania */
            ds->search(obj, val); /* vyhladanie hodnoty */
            clock_gettime(CLOCK_MONOTONIC, &t1); /* koniec merania */
            srch_total += time_ns(&t0, &t1); /* pripocitanie casu vyhladania */
            srch_count++; /* zvysenie pocitadla vyhladani */
        } else {
            /* Vymazanie (20% pravdepodobnost) */
            int val = rand() % (next_val + 1); /* nahodna hodnota na vymazanie */
            clock_gettime(CLOCK_MONOTONIC, &t0); /* zaciatok merania */
            ds->delete_fn(obj, val); /* vymazanie hodnoty */
            clock_gettime(CLOCK_MONOTONIC, &t1); /* koniec merania */
            del_total += time_ns(&t0, &t1); /* pripocitanie casu vymazania */
            del_count++; /* zvysenie pocitadla vymazani */
        }
    }

    res.insert_ns = ins_count > 0 ? ins_total / ins_count : 0; /* priemerny cas vlozenia */
    res.search_ns = srch_count > 0 ? srch_total / srch_count : 0; /* priemerny cas vyhladania */
    res.delete_ns = del_count > 0 ? del_total / del_count : 0; /* priemerny cas vymazania */
    res.count_after_insert = ds->get_count(obj); /* pocet prvkov na konci */

    ds->destroy(obj); /* zrusenie datovej struktury */
    return res;       /* vratenie vysledku */
}

/*
 * Scenar 5: Intenzivne mazanie
 * Vlozi N, vymaze 80%, vlozi 50% novych, vyhlada vsetky
 */
static BenchResult run_delete_heavy(DS_Interface *ds, int N) {
    BenchResult res = {0};         /* inicializacia vysledku na nuly */
    struct timespec t0, t1;        /* casove znamky pre meranie */
    void *obj = ds->create();      /* vytvorenie datovej struktury */

    int *data = generate_random(N); /* vygenerovanie nahodnych dat */

    /* Vlozenie vsetkych prvkov */
    for (int i = 0; i < N; i++) ds->insert(obj, data[i]); /* vlozenie kazdeho prvku */
    res.count_after_insert = ds->get_count(obj); /* ulozenie poctu prvkov po vlozeni */

    /* Vymazanie 80% prvkov */
    int del_n = (int)(N * 0.8); /* vypocet poctu prvkov na vymazanie */
    clock_gettime(CLOCK_MONOTONIC, &t0); /* zaciatok merania vymazania */
    for (int i = 0; i < del_n; i++) ds->delete_fn(obj, data[i]); /* vymazanie 80% prvkov */
    clock_gettime(CLOCK_MONOTONIC, &t1); /* koniec merania vymazania */
    res.delete_ns = time_ns(&t0, &t1) / del_n; /* priemerny cas vymazania */

    /* Opatovne vlozenie 50% novych hodnot */
    int reins_n = N / 2; /* pocet novych prvkov na vlozenie */
    clock_gettime(CLOCK_MONOTONIC, &t0); /* zaciatok merania opatovneho vlozenia */
    for (int i = 0; i < reins_n; i++) ds->insert(obj, rand()); /* vlozenie novych nahodnych hodnot */
    clock_gettime(CLOCK_MONOTONIC, &t1); /* koniec merania opatovneho vlozenia */
    res.insert_ns = time_ns(&t0, &t1) / reins_n; /* priemerny cas vlozenia */

    /* Vyhladanie vsetkych povodnych hodnot */
    int cur_count = ds->get_count(obj); /* aktualny pocet prvkov */
    clock_gettime(CLOCK_MONOTONIC, &t0); /* zaciatok merania vyhladavania */
    for (int i = 0; i < N; i++) ds->search(obj, data[i]); /* vyhladanie povodnych hodnot */
    clock_gettime(CLOCK_MONOTONIC, &t1); /* koniec merania vyhladavania */
    res.search_ns = time_ns(&t0, &t1) / N; /* priemerny cas vyhladania */
    res.count_after_delete = cur_count; /* ulozenie aktualneho poctu prvkov */

    free(data);          /* uvolnenie pola dat */
    ds->destroy(obj);    /* zrusenie datovej struktury */
    return res;          /* vratenie vysledku */
}

/* ========== Overenie spravnosti ========== */

/* Funkcia na overenie spravnosti datovej struktury */
static int verify_ds(DS_Interface *ds) {
    void *obj = ds->create(); /* vytvorenie datovej struktury */
    int errors = 0;           /* pocitadlo chyb */
    int N = 1000;             /* pocet testovacich prvkov */

    /* Vlozenie hodnot 0 az N-1 */
    for (int i = 0; i < N; i++) ds->insert(obj, i); /* vlozenie kazdeho prvku */
    if (ds->get_count(obj) != N) { /* kontrola poctu prvkov */
        printf("  ERROR: %s count after insert = %d (expected %d)\n",
               ds->name, ds->get_count(obj), N); /* vypis chyby */
        errors++; /* zvysenie pocitadla chyb */
    }

    /* Vyhladanie vsetkych vlozenych prvkov */
    for (int i = 0; i < N; i++) { /* prechod cez vsetky prvky */
        if (!ds->search(obj, i)) { /* ak sa prvok nenasiel */
            printf("  ERROR: %s search(%d) failed after insert\n", ds->name, i); /* vypis chyby */
            errors++; /* zvysenie pocitadla chyb */
            break;    /* ukoncenie cyklu */
        }
    }

    /* Vyhladanie neexistujucich prvkov */
    for (int i = N; i < N + 100; i++) { /* hodnoty ktore neboli vlozene */
        if (ds->search(obj, i)) { /* ak sa prvok nasiel (chyba) */
            printf("  ERROR: %s search(%d) found non-existing\n", ds->name, i); /* vypis chyby */
            errors++; /* zvysenie pocitadla chyb */
            break;    /* ukoncenie cyklu */
        }
    }

    /* Duplicitne vlozenie - pocet sa nesmie zmenit */
    ds->insert(obj, 0); /* pokus o vlozenie existujuceho prvku */
    if (ds->get_count(obj) != N) { /* kontrola ze pocet sa nezmenil */
        printf("  ERROR: %s count changed after duplicate insert = %d\n",
               ds->name, ds->get_count(obj)); /* vypis chyby */
        errors++; /* zvysenie pocitadla chyb */
    }

    /* Vymazanie parnych cisel */
    for (int i = 0; i < N; i += 2) ds->delete_fn(obj, i); /* vymazanie 0, 2, 4, ... */
    if (ds->get_count(obj) != N / 2) { /* kontrola ze ostala polovica */
        printf("  ERROR: %s count after delete = %d (expected %d)\n",
               ds->name, ds->get_count(obj), N / 2); /* vypis chyby */
        errors++; /* zvysenie pocitadla chyb */
    }

    /* Overenie ze vymazane su prec a ostatne ostali */
    for (int i = 0; i < N; i++) { /* prechod cez vsetky hodnoty */
        bool found = ds->search(obj, i); /* vyhladanie prvku */
        bool expected = (i % 2 == 1); /* ocakavany vysledok (len neparne) */
        if (found != expected) { /* ak vysledok nesedi */
            printf("  ERROR: %s search(%d) = %d (expected %d) after partial delete\n",
                   ds->name, i, found, expected); /* vypis chyby */
            errors++; /* zvysenie pocitadla chyb */
            break;    /* ukoncenie cyklu */
        }
    }

    /* Vymazanie neexistujucich prvkov (nesmie spadnut) */
    ds->delete_fn(obj, -1);    /* vymazanie zapornej hodnoty */
    ds->delete_fn(obj, N + 1); /* vymazanie hodnoty vacsej nez N */

    /* Vymazanie zvysnich prvkov */
    for (int i = 1; i < N; i += 2) ds->delete_fn(obj, i); /* vymazanie 1, 3, 5, ... */
    if (ds->get_count(obj) != 0) { /* kontrola ze struktura je prazdna */
        printf("  ERROR: %s count after full delete = %d\n",
               ds->name, ds->get_count(obj)); /* vypis chyby */
        errors++; /* zvysenie pocitadla chyb */
    }

    ds->destroy(obj); /* zrusenie datovej struktury */
    return errors;    /* vratenie poctu chyb */
}

/* ========== Hlavny program ========== */

/* Typ ukazatela na funkciu scenara */
typedef BenchResult (*ScenarioFn)(DS_Interface *ds, int N);

/* Struktura pre scenar */
typedef struct {
    const char *name; /* nazov scenara */
    ScenarioFn fn;    /* funkcia scenara */
} Scenario;

/* Pole vsetkych scenarov */
static Scenario SCENARIOS[] = {
    {"Random",       run_random},       /* scenar s nahodnymi datami */
    {"Sequential",   run_sequential},   /* scenar so sekvencnymi datami */
    {"Reverse",      run_reverse},      /* scenar s opacne usporiadanymi datami */
    {"Mixed",        run_mixed},        /* scenar so zmiesanymi operaciami */
    {"Delete-heavy", run_delete_heavy}, /* scenar s intenzivnym mazanim */
};
#define NUM_SCENARIOS 5 /* pocet scenarov */

/* Velkosti testovacich dat */
static int SIZES[] = {1000, 10000, 100000, 1000000, 10000000};
#define NUM_SIZES 5 /* pocet velkosti */

/* Hlavna funkcia programu */
int main(void) {
    srand(42); /* inicializacia generatora nahodnych cisel s pevnym seedom pre reprodukovatelnost */

    /* Vypis hlavicky */
    printf("================================================================\n");
    printf("  Dynamic Set Search - Implementation Comparison\n");              /* nazov programu */
    printf("  Structures: 2-3 Tree, Red-Black Tree, Hash Chain, Hash Double\n"); /* zoznam struktur */
    printf("================================================================\n\n");

    /* Overenie spravnosti vsetkych datovych struktur */
    printf("--- Correctness Verification ---\n"); /* hlavicka overenia */
    int total_errors = 0; /* celkovy pocet chyb */
    for (int d = 0; d < NUM_DS; d++) { /* prechod cez vsetky struktury */
        int err = verify_ds(&ALL_DS[d]); /* overenie jednej struktury */
        printf("  %-16s: %s\n", ALL_DS[d].name, err == 0 ? "OK" : "FAILED"); /* vypis vysledku */
        total_errors += err; /* pripocitanie chyb */
    }
    if (total_errors > 0) { /* ak boli nejake chyby */
        printf("\nCorrectness verification failed with %d errors. Aborting.\n", total_errors); /* vypis chyby */
        return 1; /* ukoncenie programu s chybou */
    }
    printf("All structures passed correctness checks.\n\n"); /* vsetky testy presli */

    /* Otvorenie CSV suboru */
    FILE *csv = fopen("results.csv", "w"); /* otvorenie suboru na zapis */
    if (!csv) { perror("Cannot open results.csv"); return 1; } /* kontrola chyby otvorenia */
    fprintf(csv, "Scenario;Size;DataStructure;AvgInsert_ns;AvgSearch_ns;AvgDelete_ns\n"); /* hlavicka CSV */

    /* Spustenie vsetkych merani */
    for (int s = 0; s < NUM_SCENARIOS; s++) { /* prechod cez vsetky scenare */
        printf("=== Scenario: %s ===\n", SCENARIOS[s].name); /* vypis nazvu scenara */
        for (int z = 0; z < NUM_SIZES; z++) { /* prechod cez vsetky velkosti */
            int N = SIZES[z]; /* aktualna velkost */
            printf("\n  N = %d\n", N); /* vypis velkosti */
            printf("  %-16s | %12s | %12s | %12s\n",
                   "Data Structure", "Insert ns/op", "Search ns/op", "Delete ns/op"); /* hlavicka tabulky */
            printf("  %-16s-+-%12s-+-%12s-+-%12s\n",
                   "----------------", "------------", "------------", "------------"); /* oddelovac tabulky */

            for (int d = 0; d < NUM_DS; d++) { /* prechod cez vsetky struktury */
                srand(42); /* reset seedu pre kazde meranie */
                BenchResult r = SCENARIOS[s].fn(&ALL_DS[d], N); /* spustenie merania */
                printf("  %-16s | %12.1f | %12.1f | %12.1f\n",
                       ALL_DS[d].name, r.insert_ns, r.search_ns, r.delete_ns); /* vypis vysledkov */

                /* Zapis do CSV s desatinnou ciarkou */
                char ins[32], srch[32], del[32]; /* buffery pre formatovanie cisel */
                snprintf(ins, sizeof(ins), "%.1f", r.insert_ns); /* formatovanie casu vlozenia */
                snprintf(srch, sizeof(srch), "%.1f", r.search_ns); /* formatovanie casu vyhladania */
                snprintf(del, sizeof(del), "%.1f", r.delete_ns); /* formatovanie casu vymazania */
                /* Nahradenie bodky ciarkou pre slovensky format */
                for (char *p = ins; *p; p++) if (*p == '.') *p = ','; /* nahradenie v case vlozenia */
                for (char *p = srch; *p; p++) if (*p == '.') *p = ','; /* nahradenie v case vyhladania */
                for (char *p = del; *p; p++) if (*p == '.') *p = ','; /* nahradenie v case vymazania */
                fprintf(csv, "%s;%d;%s;%s;%s;%s\n",
                        SCENARIOS[s].name, N, ALL_DS[d].name, ins, srch, del); /* zapis riadku do CSV */
            }
        }
        printf("\n"); /* prazdny riadok medzi scenarmi */
    }

    fclose(csv); /* zatvorenie CSV suboru */
    printf("Results saved to results.csv\n"); /* informacia o ulozeni */
    printf("Done.\n"); /* koniec programu */
    return 0; /* uspesne ukoncenie */
}
