#include "test.h"   // Include the custom header file for timing and logging macros
#include <stdio.h>  // Include standard I/O library for functions like printf and fprintf
#include <stdlib.h> // Include standard library for functions like malloc, free, and calloc
#include <string.h> // Include string library for string operations (not used in this code)
// Define the structure for an entry in the hash table
typedef struct {
    int data;      // The value stored in the hash table entry
    char status;   // The status of the entry: 0=empty, 1=active, 2=deleted
} HashEntry;
// Define the structure for the hash table
typedef struct {
    HashEntry *elements;    // Pointer to the array of entries in the hash table
    int numEntries;         // Current number of active entries in the hash table
    int maxSize;            // Total capacity of the hash table (size of the array)
    int highestIndex;       // Highest index ever used in the array (for printing optimization)
} HashTable;
// First hash function for double hashing
int hash1(int value, int maxSize) {
    return (value & 0x7FFFFFFF) % maxSize;  // Mask the highest bit of the value and compute modulo maxSize
}
// Second hash function for double hashing
int hash2(int value, int maxSize) {
    return 1 + ((value & 0x7FFFFFFF) % (maxSize - 1));  // Compute a value between 1 and maxSize-2 using modulo
}
// Function to create a new hash table
HashTable* createHashTable(int maxSize) {
    HashTable* table = (HashTable*)malloc(sizeof(HashTable));  // Allocate memory for the hash table structure
    table->maxSize = maxSize;    // Set the maximum size (capacity) of the hash table
    table->numEntries = 0;       // Initialize the number of entries to 0
    table->highestIndex = -1;    // Initialize the highest used index to -1 (no slots used yet)
    table->elements = (HashEntry*)calloc(maxSize, sizeof(HashEntry));  // Allocate memory for the array of entries, initialized to 0
    for (int idx = 0; idx < maxSize; idx++) {  // Loop through each slot in the array
        table->elements[idx].data = NULL_ENTRY;  // Set the data of the entry to the empty value (NULL_ENTRY)
        table->elements[idx].status = 0;         // Set the status of the entry to empty (0)
    }
    return table;  // Return the pointer to the newly created hash table
}
// Function to compute the probe index for double hashing
int get_probe_index(int hashA, int hashB, int attempt, int maxSize) {
    return (hashA + attempt * hashB) % maxSize;  // Compute the probe index using the formula: (hashA + attempt * hashB) % maxSize
}
// Function to print the contents of the hash table
void printHashTable(HashTable* table) {
    printf("\nHash tabulka (Velkost: %d/%d):\n", table->numEntries, table->maxSize);  // Print the header with the number of entries and max size
    printf("--------------------------------\n");  // Print a separator line
    printf("Index\tKluc\tStav\n");  // Print the column headers: Index, Key (Kluc), Status (Stav)
    printf("--------------------------------\n");  // Print another separator line
    if (table->highestIndex == -1) {  // Check if the hash table is empty (no slots used)
        printf("Prazdna tabulka\n");  // Print a message indicating the table is empty
    } else {  // If the table is not empty
        for (int idx = 0; idx <= table->highestIndex; idx++) {  // Loop through all slots up to the highest used index
            printf("%d\t", idx);  // Print the current slot index
            if (table->elements[idx].status == 0) {  // Check if the slot is empty
                printf("-\tPrazdny\n");  // Print a dash for the value and "Prazdny" (empty) for the status
            } else if (table->elements[idx].status == 1) {  // Check if the slot is active
                printf("%d\tAktivny\n", table->elements[idx].data);  // Print the value and "Aktivny" (active) for the status
            } else {  // If the slot is deleted (status == 2)
                printf("-\tVymazany\n");  // Print a dash for the value and "Vymazany" (deleted) for the status
            }
        }
    }
    printf("--------------------------------\n");  // Print a final separator line
}
// Function to resize the hash table when the load factor is exceeded
void resize(HashTable* table) {
    int newMaxSize = table->maxSize * 2;  // Calculate the new size by doubling the current max size
    HashEntry* newElements = (HashEntry*)calloc(newMaxSize, sizeof(HashEntry));  // Allocate memory for the new array of entries
    int newHighestIndex = -1;  // Initialize the new highest used index to -1
    for (int idx = 0; idx < newMaxSize; idx++) {  // Loop through each slot in the new array
        newElements[idx].data = NULL_ENTRY;  // Set the data of the new entry to the empty value (NULL_ENTRY)
        newElements[idx].status = 0;         // Set the status of the new entry to empty (0)
    }
    for (int idx = 0; idx < table->maxSize; idx++) {  // Loop through each slot in the old array
        if (table->elements[idx].status == 1) {  // Check if the slot contains an active entry
            int value = table->elements[idx].data;  // Get the value from the active entry
            int hashA = hash1(value, newMaxSize);  // Compute the first hash for the new size
            int hashB = hash2(value, newMaxSize);  // Compute the second hash for the new size
            for (int attempt = 0; attempt < newMaxSize; attempt++) {  // Loop to find a new slot in the resized array
                int slot = get_probe_index(hashA, hashB, attempt, newMaxSize);  // Compute the probe index for the current attempt
                if (newElements[slot].status == 0) {  // Check if the computed slot is empty
                    newElements[slot].data = value;  // Store the value in the new slot
                    newElements[slot].status = 1;    // Set the status of the new slot to active (1)
                    if (slot > newHighestIndex) newHighestIndex = slot;  // Update the new highest index if necessary
                    break;  // Exit the loop since the value has been placed
                }
            }
        }
    }
    free(table->elements);  // Free the memory of the old array of entries
    table->elements = newElements;  // Assign the new array to the hash table
    table->maxSize = newMaxSize;  // Update the max size of the hash table
    table->highestIndex = newHighestIndex;  // Update the highest used index
}
// Function to search for a value in the hash table
int search(HashTable* table, int value) {
    int hashA = hash1(value, table->maxSize);  // Compute the first hash for the value
    int hashB = hash2(value, table->maxSize);  // Compute the second hash for the value
    for (int attempt = 0; attempt < table->maxSize; attempt++) {  // Loop through possible slots using double hashing
        int slot = get_probe_index(hashA, hashB, attempt, table->maxSize);  // Compute the probe index for the current attempt
        if (table->elements[slot].status == 0) return 0;  // If an empty slot is found, the value is not in the table; return 0
        if (table->elements[slot].status == 1 && table->elements[slot].data == value) return 1;  // If the value is found in an active slot, return 1
    }
    return 0;  // If the entire table is searched without finding the value, return 0

}
void insert(HashTable* table, int value) {
    if ((double)table->numEntries / table->maxSize >= MAX_FILL_RATE) resize(table);  // Check if the load factor exceeds the threshold and resize if needed
    int hashA = hash1(value, table->maxSize);  // Compute the first hash for the value
    int hashB = hash2(value, table->maxSize);  // Compute the second hash for the value
    int firstRemoved = -1;  // Initialize the index of the first deleted slot to -1
    for (int attempt = 0; attempt < table->maxSize; attempt++) {  // Loop through possible slots using double hashing
        int slot = get_probe_index(hashA, hashB, attempt, table->maxSize);  // Compute the probe index for the current attempt
        if (table->elements[slot].status == 1 && table->elements[slot].data == value) return;  // Check for duplicates; if found, return without inserting
        if (table->elements[slot].status == 2 && firstRemoved == -1) firstRemoved = slot;  // Record the first deleted slot encountered
        if (table->elements[slot].status == 0) {  // Check if the current slot is empty
            int targetSlot = (firstRemoved != -1) ? firstRemoved : slot;  // Choose the target slot (prefer a deleted slot if available)
            table->elements[targetSlot].data = value;  // Store the value in the target slot
            table->elements[targetSlot].status = 1;    // Set the status of the target slot to active (1)
            table->numEntries++;  // Increment the number of entries in the hash table
            if (targetSlot > table->highestIndex) table->highestIndex = targetSlot;  // Update the highest used index if necessary
            return;  // Exit the function since the value has been inserted
        }
    }
    if (firstRemoved != -1) {  // Check if a deleted slot was found during the loop
        table->elements[firstRemoved].data = value;  // Store the value in the first deleted slot
        table->elements[firstRemoved].status = 1;    // Set the status of the slot to active (1)
        table->numEntries++;  // Increment the number of entries in the hash table
        if (firstRemoved > table->highestIndex) table->highestIndex = firstRemoved;  // Update the highest used index if necessary
    }
}
// Function to delete a value from the hash table
void delete(HashTable* table, int value) {
    int hashA = hash1(value, table->maxSize);  // Compute the first hash for the value
    int hashB = hash2(value, table->maxSize);  // Compute the second hash for the value
    for (int attempt = 0; attempt < table->maxSize; attempt++) {  // Loop through possible slots using double hashing
        int slot = get_probe_index(hashA, hashB, attempt, table->maxSize);  // Compute the probe index for the current attempt
        if (table->elements[slot].status == 0) return;  // If an empty slot is found, the value is not in the table; return
        if (table->elements[slot].status == 1 && table->elements[slot].data == value) {  // Check if the value is found in an active slot
            table->elements[slot].status = 2;  // Mark the slot as deleted (status = 2)
            table->elements[slot].data = REMOVED_ENTRY;  // Set the data to the special deleted value (REMOVED_ENTRY)
            table->numEntries--;  // Decrement the number of entries in the hash table
            return;  // Exit the function since the value has been deleted
        }
    }
}
// Function to process the input file and perform operations on the hash table
void processFileWithCSV(HashTable* table, const char* filename, FILE* csv) {
    FILE* inputFile = fopen(filename, "r");  // Open the input file in read mode
    if (!inputFile) {                        // Check if the file opening failed
        perror("Chyba pri citani suboru");   // Print an error message if the file cannot be opened
        exit(1);                             // Exit the program with an error code
    }
    int operation, value;  // Declare variables to store the operation type and value from the file
    while (fscanf(inputFile, "%d %d", &operation, &value) == 2) {  // Read an operation and value from the file
        switch (operation) {  // Switch based on the operation type
            case 0:  // Case 0: Insert operation
                TIMED_LOG_OP('+', insert(table, value), table, csv);  // Time and log the insert operation
                printf("Vlozene: %d\n", value);  // Print a message indicating the value was inserted
                break;  // Exit the switch case
            case 1: {  // Case 1: Search operation
                int found;  // Declare a variable to store the result of the search
                TIMED_LOG_OP('?', found = search(table, value), table, csv);  // Time and log the search operation
                printf("Hladanie %d: %s\n", value, found ? "Nasiel" : "Nenasiel");  // Print whether the value was found or not
                break;  // Exit the switch case
            }
            case 2:  // Case 2: Delete operation
                TIMED_LOG_OP('-', delete(table, value), table, csv);  // Time and log the delete operation
                printf("Vymazane: %d\n", value);  // Print a message indicating the value was deleted
                break;  // Exit the switch case
        }
    }
    fclose(inputFile);  // Close the input file
}
// Main function to run the program
int main() {
    FILE* csvOutput = fopen("graph_data2.csv", "w");  // Open the CSV output file in write mode
    if (!csvOutput) {  // Check if the file opening failed
        perror("Chyba pri CSV");  // Print an error message if the CSV file cannot be opened
        return 1;  // Return an error code to indicate failure
    }
    fprintf(csvOutput, "CurrentNodes;Time;Op;LoadFactor;LoadFactorThreshold\n");  // Write the CSV header
    HashTable* table = createHashTable(STARTING_SIZE);  // Create a new hash table with the initial size
    processFileWithCSV(table, "mixed_operations1000_3.txt", csvOutput);  // Process the input file and perform operations
    printHashTable(table);  // Print the final contents of the hash table
    fclose(csvOutput);     // Close the CSV output file
    free(table->elements); // Free the memory allocated for the hash table's array of entries
    free(table);           // Free the memory allocated for the hash table structure
    return 0;  // Return 0 to indicate successful program completion
}
