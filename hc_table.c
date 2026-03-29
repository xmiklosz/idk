/* Hasovacia tabulka s retazenim (chaining) */
#ifndef HC_TABLE_C       /* ochrana pred viacnasobnym vlozenim */
#define HC_TABLE_C       /* definicia ochrany */

#include <stdio.h>       /* standardny vstup/vystup */
#include <stdlib.h>      /* standardna kniznica (malloc, calloc, free) */
#include <stdbool.h>     /* typ bool (true/false) */

#define HC_INITIAL_BUCKETS 16   /* pociatocny pocet kokov (buckets) */
#define HC_LOAD_THRESHOLD 0.75  /* prah zatazenia pre zvacsenie tabulky */

/* ========== Datove struktury ========== */

/* Polozka v retazci (spajany zoznam) */
typedef struct HC_Entry {
    int key;                  /* kluc ulozeny v polozke */
    struct HC_Entry *next;    /* ukazatel na dalsiu polozku v retazci */
} HC_Entry;

/* Hasovacia tabulka s retazenim */
typedef struct {
    HC_Entry **buckets;  /* pole ukazatelov na retazce (koky) */
    int capacity;        /* aktualna kapacita (pocet kokov) */
    int size;            /* pocet ulozenych prvkov */
} HC_Table;

/* ========== Hasovacia funkcia ========== */

/* Vypocita index koka pre dany kluc */
static int hc_compute_hash(int key, int capacity) {
    unsigned int k = (unsigned int)key;          /* pretypovanie na unsigned pre spravny modulo */
    return (int)(k % (unsigned int)capacity);    /* vrati index v rozsahu [0, capacity-1] */
}

/* ========== Vytvorenie / Zrusenie ========== */

/* Vytvori novu prazdnu hasovaciu tabulku */
HC_Table *hc_create(void) {
    HC_Table *table = (HC_Table *)malloc(sizeof(HC_Table)); /* alokacia struktury tabulky */
    if (!table) {                                           /* kontrola uspesnosti alokacie */
        fprintf(stderr, "hc_create: memory allocation error\n"); /* chybova hlaska */
        exit(EXIT_FAILURE);                                      /* ukoncenie programu */
    }
    table->capacity = HC_INITIAL_BUCKETS;                                       /* nastavenie pociatocnej kapacity */
    table->size = 0;                                                            /* pociatocny pocet prvkov je 0 */
    table->buckets = (HC_Entry **)calloc(table->capacity, sizeof(HC_Entry *));  /* alokacia pola kokov (vynulovane) */
    if (!table->buckets) {                                                      /* kontrola uspesnosti alokacie */
        fprintf(stderr, "hc_create: memory allocation error\n");                /* chybova hlaska */
        exit(EXIT_FAILURE);                                                     /* ukoncenie programu */
    }
    return table; /* vratenie vytvorenej tabulky */
}

/* Zrusi hasovaciu tabulku a uvolni vsetku pamat */
void hc_destroy(HC_Table *table) {
    if (!table) return;                                /* ak je tabulka NULL, nic nerobime */
    for (int i = 0; i < table->capacity; i++) {        /* prechod cez vsetky koky */
        HC_Entry *entry = table->buckets[i];           /* prvy prvok v retazci */
        while (entry) {                                /* prechod cez cely retazec */
            HC_Entry *next = entry->next;              /* ulozenie ukazatela na dalsi prvok */
            free(entry);                               /* uvolnenie aktualneho prvku */
            entry = next;                              /* posun na dalsi prvok */
        }
    }
    free(table->buckets);  /* uvolnenie pola kokov */
    free(table);           /* uvolnenie struktury tabulky */
}

/* ========== Zmena velkosti ========== */

/* Zdvojnasobi kapacitu tabulky a prehasuje vsetky prvky */
static void hc_grow(HC_Table *table) {
    int new_cap = table->capacity * 2;                                         /* nova kapacita je dvojnasobna */
    HC_Entry **new_buckets = (HC_Entry **)calloc(new_cap, sizeof(HC_Entry *)); /* alokacia noveho pola kokov */
    if (!new_buckets) {                                                        /* kontrola uspesnosti alokacie */
        fprintf(stderr, "hc_grow: memory allocation error\n");                 /* chybova hlaska */
        exit(EXIT_FAILURE);                                                    /* ukoncenie programu */
    }

    /* prehasovanie vsetkych prvkov do noveho pola kokov */
    for (int i = 0; i < table->capacity; i++) {        /* prechod cez stare koky */
        HC_Entry *entry = table->buckets[i];           /* prvy prvok v starom retazci */
        while (entry) {                                /* prechod cez cely retazec */
            HC_Entry *next = entry->next;              /* ulozenie ukazatela na dalsi prvok */
            int new_idx = hc_compute_hash(entry->key, new_cap); /* vypocet noveho indexu */
            entry->next = new_buckets[new_idx];        /* vlozenie na zaciatok noveho retazca */
            new_buckets[new_idx] = entry;              /* nastavenie ako prvy prvok v novom koku */
            entry = next;                              /* posun na dalsi prvok */
        }
    }
    free(table->buckets);          /* uvolnenie stareho pola kokov */
    table->buckets = new_buckets;  /* nastavenie noveho pola kokov */
    table->capacity = new_cap;     /* aktualizacia kapacity */
}

/* ========== Vyhladanie ========== */

/* Vyhlada kluc v hasovacej tabulke, vrati true ak sa nasiel */
bool hc_search(HC_Table *table, int key) {
    int idx = hc_compute_hash(key, table->capacity); /* vypocet indexu koka */
    HC_Entry *entry = table->buckets[idx];           /* prvy prvok v retazci */
    while (entry) {                                  /* prechod cez retazec */
        if (entry->key == key) return true;          /* kluc najdeny */
        entry = entry->next;                         /* posun na dalsi prvok */
    }
    return false; /* kluc sa nenasiel */
}

/* ========== Vlozenie ========== */

/* Vlozi kluc do hasovacej tabulky (duplicity sa ignoruju) */
void hc_insert(HC_Table *table, int key) {
    /* kontrola faktora zatazenia a pripadne zvacsenie tabulky */
    if ((double)table->size / table->capacity >= HC_LOAD_THRESHOLD) /* ak je zatazenie prilis vysoke */
        hc_grow(table);                                              /* zvacsime tabulku */

    int idx = hc_compute_hash(key, table->capacity); /* vypocet indexu koka */

    /* kontrola ci kluc uz existuje (duplicita) */
    HC_Entry *entry = table->buckets[idx]; /* prvy prvok v retazci */
    while (entry) {                        /* prechod cez retazec */
        if (entry->key == key) return;     /* kluc uz existuje, nerobime nic */
        entry = entry->next;               /* posun na dalsi prvok */
    }

    /* vlozenie novej polozky na zaciatok retazca */
    HC_Entry *new_entry = (HC_Entry *)malloc(sizeof(HC_Entry)); /* alokacia novej polozky */
    if (!new_entry) {                                            /* kontrola uspesnosti alokacie */
        fprintf(stderr, "hc_insert: memory allocation error\n"); /* chybova hlaska */
        exit(EXIT_FAILURE);                                      /* ukoncenie programu */
    }
    new_entry->key = key;                  /* nastavenie kluca */
    new_entry->next = table->buckets[idx]; /* napojenie na existujuci retazec */
    table->buckets[idx] = new_entry;       /* nastavenie ako prvy prvok v koku */
    table->size++;                         /* zvysenie poctu prvkov */
}

/* ========== Vymazanie ========== */

/* Vymaze kluc z hasovacej tabulky */
void hc_delete(HC_Table *table, int key) {
    int idx = hc_compute_hash(key, table->capacity); /* vypocet indexu koka */
    HC_Entry *entry = table->buckets[idx];           /* prvy prvok v retazci */
    HC_Entry *prev = NULL;                           /* predchadzajuci prvok (pre prepojenie) */

    while (entry) {                      /* prechod cez retazec */
        if (entry->key == key) {         /* nasli sme kluc */
            if (prev)                    /* ak nie je prvy v retazci */
                prev->next = entry->next; /* prepojenie predchadzajuceho na nasledujuci */
            else                         /* ak je prvy v retazci */
                table->buckets[idx] = entry->next; /* nastavenie nasledujuceho ako prveho */
            free(entry);                 /* uvolnenie vymazaneho prvku */
            table->size--;               /* znizenie poctu prvkov */
            return;                      /* koniec */
        }
        prev = entry;         /* ulozenie aktualneho ako predchadzajuceho */
        entry = entry->next;  /* posun na dalsi prvok */
    }
}

/* ========== Vypis (pre ladenie) ========== */

/* Vypise obsah hasovacej tabulky na konzolu */
void hc_print(HC_Table *table) {
    printf("Hash Chain Table (size=%d, capacity=%d, load=%.2f)\n",
           table->size, table->capacity, (double)table->size / table->capacity); /* hlavicka s informaciami */
    for (int i = 0; i < table->capacity; i++) {  /* prechod cez vsetky koky */
        if (!table->buckets[i]) continue;        /* preskocenie prazdnych kokov */
        printf("  [%d]:", i);                    /* vypis indexu koka */
        HC_Entry *entry = table->buckets[i];     /* prvy prvok v retazci */
        while (entry) {                          /* prechod cez retazec */
            printf(" %d", entry->key);           /* vypis kluca */
            entry = entry->next;                 /* posun na dalsi prvok */
        }
        printf("\n");                            /* novy riadok po kazdom koku */
    }
}

#endif /* HC_TABLE_C */ /* koniec ochrany pred viacnasobnym vlozenim */
