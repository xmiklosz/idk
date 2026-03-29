/* Hasovacia tabulka s dvojitym hasovanim (double hashing) */
#ifndef HD_TABLE_C       /* ochrana pred viacnasobnym vlozenim */
#define HD_TABLE_C       /* definicia ochrany */

#include <stdio.h>       /* standardny vstup/vystup */
#include <stdlib.h>      /* standardna kniznica (malloc, calloc, free) */
#include <stdbool.h>     /* typ bool (true/false) */

#define HD_INITIAL_CAPACITY 17   /* pociatocna kapacita (prvocislo) */
#define HD_LOAD_THRESHOLD 0.7    /* prah zatazenia pre zvacsenie tabulky */

/* ========== Datove struktury ========== */

/* Stavy slotov v hasovacej tabulke */
typedef enum { SLOT_EMPTY = 0, SLOT_ACTIVE, SLOT_DELETED } HD_SlotStatus;
/* SLOT_EMPTY = prazdny slot, SLOT_ACTIVE = obsadeny slot, SLOT_DELETED = vymazany slot (nahrobny kamen) */

/* Hasovacia tabulka s dvojitym hasovanim */
typedef struct {
    int *keys;              /* pole klucov */
    HD_SlotStatus *status;  /* pole stavov slotov */
    int capacity;           /* kapacita tabulky */
    int size;               /* pocet aktivnych poloziek */
    int occupied;           /* pocet obsadenych slotov (aktivne + vymazane, pre faktor zatazenia) */
} HD_Table;

/* ========== Pomocne funkcie pre prvocisla ========== */

/* Zisti ci je cislo n prvocislo */
static bool hd_is_prime(int n) {
    if (n < 2) return false;                              /* cisla mensie ako 2 nie su prvocisla */
    if (n < 4) return true;                               /* 2 a 3 su prvocisla */
    if (n % 2 == 0 || n % 3 == 0) return false;          /* delitelne 2 alebo 3 nie su prvocisla */
    for (int i = 5; i * i <= n; i += 6)                   /* testovanie delitelov tvaru 6k+-1 */
        if (n % i == 0 || n % (i + 2) == 0) return false; /* ak je delitelne, nie je prvocislo */
    return true;                                           /* je prvocislo */
}

/* Najde najblizsie prvocislo vacsie alebo rovne n */
static int hd_next_prime(int n) {
    if (n <= 2) return 2;              /* najmensi prvocislo je 2 */
    if (n % 2 == 0) n++;              /* zaciname od neparneho cisla */
    while (!hd_is_prime(n)) n += 2;   /* skusame neparne cisla kym nenajdeme prvocislo */
    return n;                          /* vratenie najdeneho prvocisla */
}

/* ========== Hasovacie funkcie ========== */

/* Primarna hasovacia funkcia */
static int hd_primary_hash(int key, int capacity) {
    unsigned int k = (unsigned int)key;          /* pretypovanie na unsigned */
    return (int)(k % (unsigned int)capacity);    /* vrati index v rozsahu [0, capacity-1] */
}

/* Sekundarna hasovacia funkcia (pre dvojite hasovanie) */
static int hd_secondary_hash(int key, int capacity) {
    unsigned int k = (unsigned int)key;                   /* pretypovanie na unsigned */
    /* vrati hodnotu v rozsahu [1, capacity-1], nikdy 0 */
    return 1 + (int)(k % (unsigned int)(capacity - 1));   /* zabezpecuje nenulovy krok */
}

/* Vypocita slot pre dany pokus (attempt) pomocou dvojiteho hasovania */
static int hd_probe_slot(int h1, int h2, int attempt, int capacity) {
    return ((unsigned int)h1 + (unsigned int)attempt * (unsigned int)h2) % (unsigned int)capacity; /* (h1 + pokus * h2) mod kapacita */
}

/* ========== Vytvorenie / Zrusenie ========== */

/* Vytvori novu prazdnu hasovaciu tabulku s dvojitym hasovanim */
HD_Table *hd_create(void) {
    HD_Table *table = (HD_Table *)malloc(sizeof(HD_Table)); /* alokacia struktury tabulky */
    if (!table) {                                           /* kontrola uspesnosti alokacie */
        fprintf(stderr, "hd_create: memory allocation error\n"); /* chybova hlaska */
        exit(EXIT_FAILURE);                                      /* ukoncenie programu */
    }
    table->capacity = HD_INITIAL_CAPACITY;                                      /* nastavenie pociatocnej kapacity */
    table->size = 0;                                                            /* pociatocny pocet prvkov je 0 */
    table->occupied = 0;                                                        /* pociatocny pocet obsadenych slotov je 0 */
    table->keys = (int *)calloc(table->capacity, sizeof(int));                  /* alokacia pola klucov */
    table->status = (HD_SlotStatus *)calloc(table->capacity, sizeof(HD_SlotStatus)); /* alokacia pola stavov */
    if (!table->keys || !table->status) {                                       /* kontrola uspesnosti alokacie */
        fprintf(stderr, "hd_create: memory allocation error\n");                /* chybova hlaska */
        exit(EXIT_FAILURE);                                                     /* ukoncenie programu */
    }
    return table; /* vratenie vytvorenej tabulky */
}

/* Zrusi hasovaciu tabulku a uvolni vsetku pamat */
void hd_destroy(HD_Table *table) {
    if (!table) return;      /* ak je tabulka NULL, nic nerobime */
    free(table->keys);       /* uvolnenie pola klucov */
    free(table->status);     /* uvolnenie pola stavov */
    free(table);             /* uvolnenie struktury tabulky */
}

/* ========== Zmena velkosti ========== */

/* Zdvojnasobi kapacitu tabulky (na najblizsie prvocislo) a prehasuje vsetky prvky */
static void hd_grow(HD_Table *table) {
    int new_cap = hd_next_prime(table->capacity * 2);                                /* nova kapacita je dvojnasobok zaokruhleny na prvocislo */
    int *new_keys = (int *)calloc(new_cap, sizeof(int));                             /* alokacia noveho pola klucov */
    HD_SlotStatus *new_status = (HD_SlotStatus *)calloc(new_cap, sizeof(HD_SlotStatus)); /* alokacia noveho pola stavov */
    if (!new_keys || !new_status) {                                                  /* kontrola uspesnosti alokacie */
        fprintf(stderr, "hd_grow: memory allocation error\n");                       /* chybova hlaska */
        exit(EXIT_FAILURE);                                                          /* ukoncenie programu */
    }

    /* prehasovanie iba aktivnych poloziek (nahrobne kamene sa zahodia) */
    for (int i = 0; i < table->capacity; i++) {            /* prechod cez stare sloty */
        if (table->status[i] != SLOT_ACTIVE) continue;    /* preskocenie neaktivnych slotov */
        int h1 = hd_primary_hash(table->keys[i], new_cap);   /* vypocet primarneho hasu */
        int h2 = hd_secondary_hash(table->keys[i], new_cap); /* vypocet sekundarneho hasu */
        for (int a = 0; a < new_cap; a++) {                   /* linearny pokus najst volny slot */
            int slot = hd_probe_slot(h1, h2, a, new_cap);    /* vypocet slotu pre dany pokus */
            if (new_status[slot] == SLOT_EMPTY) {             /* ak je slot prazdny */
                new_keys[slot] = table->keys[i];              /* vlozenie kluca */
                new_status[slot] = SLOT_ACTIVE;               /* nastavenie slotu ako aktivneho */
                break;                                        /* slot najdeny, koniec */
            }
        }
    }

    free(table->keys);               /* uvolnenie stareho pola klucov */
    free(table->status);             /* uvolnenie stareho pola stavov */
    table->keys = new_keys;          /* nastavenie noveho pola klucov */
    table->status = new_status;      /* nastavenie noveho pola stavov */
    table->capacity = new_cap;       /* aktualizacia kapacity */
    table->occupied = table->size;   /* nahrobne kamene su prec, occupied = size */
}

/* ========== Vyhladanie ========== */

/* Vyhlada kluc v hasovacej tabulke, vrati true ak sa nasiel */
bool hd_search(HD_Table *table, int key) {
    int h1 = hd_primary_hash(key, table->capacity);   /* vypocet primarneho hasu */
    int h2 = hd_secondary_hash(key, table->capacity);  /* vypocet sekundarneho hasu */
    for (int a = 0; a < table->capacity; a++) {        /* skusanie slotov */
        int slot = hd_probe_slot(h1, h2, a, table->capacity); /* vypocet slotu pre dany pokus */
        if (table->status[slot] == SLOT_EMPTY)         /* ak je slot prazdny */
            return false;                              /* kluc sa v tabulke nenachadza */
        if (table->status[slot] == SLOT_ACTIVE && table->keys[slot] == key) /* ak je aktivny a kluc sedi */
            return true;                               /* kluc najdeny */
        /* SLOT_DELETED -> pokracujeme v hladani */
    }
    return false; /* presli sme celu tabulku, kluc sa nenasiel */
}

/* ========== Vlozenie ========== */

/* Vlozi kluc do hasovacej tabulky (duplicity sa ignoruju) */
void hd_insert(HD_Table *table, int key) {
    if ((double)table->occupied / table->capacity >= HD_LOAD_THRESHOLD) /* ak je zatazenie prilis vysoke */
        hd_grow(table);                                                 /* zvacsime tabulku */

    int h1 = hd_primary_hash(key, table->capacity);   /* vypocet primarneho hasu */
    int h2 = hd_secondary_hash(key, table->capacity);  /* vypocet sekundarneho hasu */
    int first_tombstone = -1;                          /* index prveho nahrobneho kamena (-1 = ziadny) */

    for (int a = 0; a < table->capacity; a++) {        /* skusanie slotov */
        int slot = hd_probe_slot(h1, h2, a, table->capacity); /* vypocet slotu pre dany pokus */

        if (table->status[slot] == SLOT_ACTIVE && table->keys[slot] == key) /* ak kluc uz existuje */
            return;  /* duplicita, nerobime nic */

        if (table->status[slot] == SLOT_DELETED && first_tombstone == -1) /* ak sme nasli prvy nahrobny kamen */
            first_tombstone = slot;                                       /* zapamatame si jeho index */

        if (table->status[slot] == SLOT_EMPTY) {                          /* ak sme nasli prazdny slot */
            int target = (first_tombstone != -1) ? first_tombstone : slot; /* pouzijeme nahrobny kamen ak existuje */
            table->keys[target] = key;                                     /* vlozenie kluca */
            if (table->status[target] != SLOT_DELETED)                     /* ak to nie je nahrobny kamen */
                table->occupied++;                                         /* zvysenie poctu obsadenych slotov */
            table->status[target] = SLOT_ACTIVE;                           /* nastavenie slotu ako aktivneho */
            table->size++;                                                 /* zvysenie poctu prvkov */
            return;                                                        /* koniec */
        }
    }

    /* vsetky sloty su aktivne alebo vymazane - pouzijeme prvy nahrobny kamen */
    if (first_tombstone != -1) {                            /* ak existuje nahrobny kamen */
        table->keys[first_tombstone] = key;                 /* vlozenie kluca na miesto nahrobneho kamena */
        table->status[first_tombstone] = SLOT_ACTIVE;       /* nastavenie slotu ako aktivneho */
        table->size++;                                      /* zvysenie poctu prvkov */
    }
}

/* ========== Vymazanie ========== */

/* Vymaze kluc z hasovacej tabulky (oznaci slot ako vymazany - nahrobny kamen) */
void hd_delete(HD_Table *table, int key) {
    int h1 = hd_primary_hash(key, table->capacity);   /* vypocet primarneho hasu */
    int h2 = hd_secondary_hash(key, table->capacity);  /* vypocet sekundarneho hasu */
    for (int a = 0; a < table->capacity; a++) {        /* skusanie slotov */
        int slot = hd_probe_slot(h1, h2, a, table->capacity); /* vypocet slotu pre dany pokus */
        if (table->status[slot] == SLOT_EMPTY)         /* ak je slot prazdny */
            return;  /* kluc sa nenasiel */
        if (table->status[slot] == SLOT_ACTIVE && table->keys[slot] == key) { /* ak sme nasli kluc */
            table->status[slot] = SLOT_DELETED;        /* oznacenie slotu ako vymazaneho (nahrobny kamen) */
            table->size--;                             /* znizenie poctu prvkov */
            return;                                    /* koniec */
        }
    }
}

/* ========== Vypis (pre ladenie) ========== */

/* Vypise obsah hasovacej tabulky na konzolu */
void hd_print(HD_Table *table) {
    printf("Hash Double Table (size=%d, capacity=%d, load=%.2f)\n",
           table->size, table->capacity, (double)table->occupied / table->capacity); /* hlavicka s informaciami */
    for (int i = 0; i < table->capacity; i++) {       /* prechod cez vsetky sloty */
        if (table->status[i] == SLOT_ACTIVE)          /* ak je slot aktivny */
            printf("  [%d]: %d\n", i, table->keys[i]); /* vypis indexu a kluca */
        else if (table->status[i] == SLOT_DELETED)    /* ak je slot vymazany */
            printf("  [%d]: (tombstone)\n", i);        /* vypis nahrobneho kamena */
    }
}

#endif /* HD_TABLE_C */ /* koniec ochrany pred viacnasobnym vlozenim */
