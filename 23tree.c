// Include the custom test.h header file for timing and logging macros
#include "test.h"   // Vlastné makrá pre meranie casu a logovanie
#include <stdio.h>   // Štandardné vstupno/výstupné funkcie (printf, fopen, fclose)
#include <stdlib.h>  // Funkcie pre prácu s pamätou (malloc, free, exit)
#include <stdbool.h> // Definícia logických hodnôt true/false
// Define a structure for a node in the 2-3 tree
typedef struct TreeNode {
    int nodeValues[2];          // Array to store 1 or 2 values (2-3 tree nodes can hold 1 or 2 values)
    struct TreeNode *childPointers[3]; // Pointers to 0-3 children (a 2-3 tree node can have 0, 2, or 3 children)
    int valueCount;          // Number of values in the node (1 for a 2-node, 2 for a 3-node)
} TreeNode;
// Define a structure for the 2-3 tree with metadata
typedef struct TwoThreeTree {
    TreeNode* rootNode;     // Pointer to the root node of the tree
    int nodeCount;  // Total number of nodes in the tree (used by test.h for logging)
} TwoThreeTree;
// Function to create a new node with a given value
TreeNode* createNode(TwoThreeTree* treeStruct, int targetValue) {
    TreeNode* newNode = calloc(1, sizeof(TreeNode));
    if (!newNode) {
        fprintf(stderr, "Chyba: alokacie pamate");
        exit(EXIT_FAILURE);
    }
    newNode->nodeValues[0] = targetValue;
    newNode->valueCount = 1;
    treeStruct->nodeCount++;
    return newNode;
}
// Free only this node (no children) – used after children have been moved elsewhere
static void freeNodeOnly(TwoThreeTree* treeStruct, TreeNode* node) {
    if (!node) return;
    free(node);
    treeStruct->nodeCount--;
}
// Function to recursively free a node and its children
void freeNode(TwoThreeTree* treeStruct, TreeNode* currentNode) {
    if (!currentNode) return;
    for (int index = 0; index <= currentNode->valueCount; index++) {
        freeNode(treeStruct, currentNode->childPointers[index]);
    }
    free(currentNode);
    treeStruct->nodeCount--;
}
// Function to print the tree in preorder traversal
void printPreorder(TreeNode* rootNode) {
    if (!rootNode) return;
    printf("[");
    for (int index = 0; index < rootNode->valueCount; index++) {
        printf("%d", rootNode->nodeValues[index]);
        if (index < rootNode->valueCount-1) printf(" ");
    }
    printf("] ");
    for (int index = 0; index <= rootNode->valueCount; index++) {
        printPreorder(rootNode->childPointers[index]);
    }
}
// Function to search for a value in the tree
bool lookup(TreeNode* rootNode, int targetValue) {
    if (!rootNode) return false;
    for (int index = 0; index < rootNode->valueCount; index++) {
        if (targetValue == rootNode->nodeValues[index]) return true;
        if (targetValue < rootNode->nodeValues[index])
            return lookup(rootNode->childPointers[index], targetValue);
    }
    return lookup(rootNode->childPointers[rootNode->valueCount], targetValue);
}
// Function to insert a value into the 2-3 tree with balancing
TreeNode* insert(TwoThreeTree* treeStruct, TreeNode* rootNode, int targetValue) {
    if (!rootNode) return createNode(treeStruct, targetValue);

    TreeNode* nodePath[64] = {0};
    int childIndexes[64] = {0};
    int currentDepth = 0;
    TreeNode* currentNode = rootNode;
    int insertPosition;

    while (currentNode) {
        insertPosition = 0;
        while (insertPosition < currentNode->valueCount && targetValue > currentNode->nodeValues[insertPosition])
            insertPosition++;
        if (insertPosition < currentNode->valueCount && currentNode->nodeValues[insertPosition] == targetValue)
            return rootNode; // duplicate

        nodePath[currentDepth] = currentNode;
        childIndexes[currentDepth] = insertPosition;
        currentDepth++;
        if (!currentNode->childPointers[0]) break;
        currentNode = currentNode->childPointers[insertPosition];
    }

    TreeNode* leafNode = nodePath[currentDepth-1];
    if (leafNode->valueCount < 2) {
        insertPosition = leafNode->valueCount;
        while (insertPosition > 0 && targetValue < leafNode->nodeValues[insertPosition-1]) {
            leafNode->nodeValues[insertPosition] = leafNode->nodeValues[insertPosition-1];
            insertPosition--;
        }
        leafNode->nodeValues[insertPosition] = targetValue;
        leafNode->valueCount++;
        return rootNode;
    }

    int tempValues[3] = {leafNode->nodeValues[0], leafNode->nodeValues[1], targetValue};
    for (int i = 0; i < 2; i++)
        for (int j = i+1; j < 3; j++)
            if (tempValues[i] > tempValues[j]) { int t = tempValues[i]; tempValues[i] = tempValues[j]; tempValues[j] = t; }

    TreeNode* newLeafNode = createNode(treeStruct, tempValues[2]);
    leafNode->nodeValues[0] = tempValues[0];
    leafNode->valueCount = 1;
    int promotedValue = tempValues[1];
    TreeNode* newSplitNode = newLeafNode;

    currentDepth -= 2;
    while (currentDepth >= 0) {
        TreeNode* parentNode = nodePath[currentDepth];
        insertPosition = childIndexes[currentDepth];

        if (parentNode->valueCount < 2) {
            for (int i = parentNode->valueCount; i > insertPosition; i--) {
                parentNode->nodeValues[i] = parentNode->nodeValues[i-1];
                parentNode->childPointers[i+1] = parentNode->childPointers[i];
            }
            parentNode->nodeValues[insertPosition] = promotedValue;
            parentNode->childPointers[insertPosition+1] = newSplitNode;
            parentNode->valueCount++;
            return rootNode;
        }

        TreeNode* newParentNode = createNode(treeStruct, 0);
        int pv[3];
        TreeNode* pc[4];
        for (int i = 0; i < 2; i++) pv[i] = parentNode->nodeValues[i];
        for (int i = 0; i < 3; i++) pc[i] = parentNode->childPointers[i];

        insertPosition = 2;
        while (insertPosition > 0 && promotedValue < pv[insertPosition-1]) {
            pv[insertPosition] = pv[insertPosition-1];
            pc[insertPosition+1] = pc[insertPosition];
            insertPosition--;
        }
        pv[insertPosition] = promotedValue;
        pc[insertPosition+1] = newSplitNode;

        parentNode->valueCount = 1;
        newParentNode->valueCount = 1;
        parentNode->nodeValues[0] = pv[0];
        newParentNode->nodeValues[0] = pv[2];
        promotedValue = pv[1];

        parentNode->childPointers[0] = pc[0];
        parentNode->childPointers[1] = pc[1];
        newParentNode->childPointers[0] = pc[2];
        newParentNode->childPointers[1] = pc[3];

        newSplitNode = newParentNode;
        currentDepth--;
    }

    TreeNode* newRootNode = createNode(treeStruct, promotedValue);
    newRootNode->childPointers[0] = rootNode;
    newRootNode->childPointers[1] = newSplitNode;
    return newRootNode;
}

/*
 * fixUnderflow – restore 2-3 tree properties after a value was removed from
 * `currentNode` (valueCount may now be 0).
 *
 * nodePath[0..pathTop]   = ancestors, nodePath[pathTop] is the direct parent.
 * childIndexes[pathTop+1] = index of currentNode within its parent.
 */
void fixUnderflow(TwoThreeTree* treeStruct, TreeNode* currentNode,
                  TreeNode** nodePath, int* childIndexes, int pathTop) {
    while (currentNode && currentNode->valueCount < 1) {
        if (pathTop < 0) break; // at root, nothing to do

        TreeNode* parentNode = nodePath[pathTop];
        int idx = childIndexes[pathTop + 1];

        TreeNode* leftSibling  = (idx > 0)                    ? parentNode->childPointers[idx-1] : NULL;
        TreeNode* rightSibling = (idx < parentNode->valueCount) ? parentNode->childPointers[idx+1] : NULL;

        // ── Borrow from left sibling ───────────────────────────────────────
        if (leftSibling && leftSibling->valueCount > 1) {
            currentNode->nodeValues[0] = parentNode->nodeValues[idx-1];
            parentNode->nodeValues[idx-1] = leftSibling->nodeValues[leftSibling->valueCount-1];
            if (leftSibling->childPointers[0]) {
                currentNode->childPointers[1] = currentNode->childPointers[0];
                currentNode->childPointers[0] = leftSibling->childPointers[leftSibling->valueCount];
                leftSibling->childPointers[leftSibling->valueCount] = NULL;
            }
            leftSibling->valueCount--;
            currentNode->valueCount = 1;
            return; // fixed

        // ── Borrow from right sibling ──────────────────────────────────────
        } else if (rightSibling && rightSibling->valueCount > 1) {
            currentNode->nodeValues[0] = parentNode->nodeValues[idx];
            parentNode->nodeValues[idx] = rightSibling->nodeValues[0];
            if (rightSibling->childPointers[0]) {
                currentNode->childPointers[1] = rightSibling->childPointers[0];
                for (int i = 0; i < rightSibling->valueCount; i++)
                    rightSibling->childPointers[i] = rightSibling->childPointers[i+1];
                rightSibling->childPointers[rightSibling->valueCount] = NULL;
            }
            for (int i = 0; i < rightSibling->valueCount-1; i++)
                rightSibling->nodeValues[i] = rightSibling->nodeValues[i+1];
            rightSibling->valueCount--;
            currentNode->valueCount = 1;
            return; // fixed

        // ── Merge ──────────────────────────────────────────────────────────
        } else if (leftSibling) {
            // Merge currentNode into leftSibling
            leftSibling->nodeValues[leftSibling->valueCount] = parentNode->nodeValues[idx-1];
            if (currentNode->childPointers[0]) {
                leftSibling->childPointers[leftSibling->valueCount + 1] = currentNode->childPointers[0];
                currentNode->childPointers[0] = NULL; // prevent double-free
            }
            leftSibling->valueCount++;
            // Remove separator and currentNode pointer from parent
            for (int i = idx-1; i < parentNode->valueCount-1; i++) {
                parentNode->nodeValues[i] = parentNode->nodeValues[i+1];
                parentNode->childPointers[i+1] = parentNode->childPointers[i+2];
            }
            parentNode->childPointers[parentNode->valueCount] = NULL;
            parentNode->valueCount--;
            freeNodeOnly(treeStruct, currentNode);
            currentNode = parentNode;
            pathTop--;

        } else if (rightSibling) {
            // Merge rightSibling into currentNode
            currentNode->nodeValues[0] = parentNode->nodeValues[idx];
            currentNode->valueCount = 1;
            currentNode->nodeValues[1] = rightSibling->nodeValues[0];
            if (rightSibling->childPointers[0]) {
                currentNode->childPointers[1] = rightSibling->childPointers[0];
                currentNode->childPointers[2] = rightSibling->childPointers[1];
                rightSibling->childPointers[0] = NULL; // prevent double-free
                rightSibling->childPointers[1] = NULL;
            }
            if (rightSibling->valueCount == 2) {
                currentNode->nodeValues[2] = rightSibling->nodeValues[1]; // won't happen normally
            }
            currentNode->valueCount += rightSibling->valueCount;
            // Remove separator and rightSibling pointer from parent
            for (int i = idx; i < parentNode->valueCount-1; i++) {
                parentNode->nodeValues[i] = parentNode->nodeValues[i+1];
                parentNode->childPointers[i+1] = parentNode->childPointers[i+2];
            }
            parentNode->childPointers[parentNode->valueCount] = NULL;
            parentNode->valueCount--;
            freeNodeOnly(treeStruct, rightSibling);
            currentNode = parentNode;
            pathTop--;
        } else {
            break; // should not happen
        }
    }

    // If root is now empty, shrink the tree by one level
    if (treeStruct->rootNode && treeStruct->rootNode->valueCount == 0) {
        TreeNode* newRoot = treeStruct->rootNode->childPointers[0];
        treeStruct->rootNode->childPointers[0] = NULL;
        freeNodeOnly(treeStruct, treeStruct->rootNode);
        treeStruct->rootNode = newRoot;
    }
}

/*
 * delet – remove targetValue from the 2-3 tree.
 * Returns the (possibly new) root.
 *
 * Strategy
 *   1. Walk down, recording the path in nodePath / childIndexes.
 *   2a. Leaf hit  → remove value, call fixUnderflow.
 *   2b. Internal  → find in-order predecessor (rightmost of left subtree),
 *                   copy its value up, then delete it from its leaf.
 */
TreeNode* delet(TwoThreeTree* treeStruct, TreeNode* rootNode, int targetValue) {
    if (!rootNode) return NULL;

    TreeNode* nodePath[64] = {0};
    int childIndexes[64] = {0};
    int pathTop = -1;

    TreeNode* currentNode = rootNode;
    int searchValue = targetValue;

    while (currentNode) {
        int found = -1;
        int childIdx = currentNode->valueCount; // default: go right-most

        for (int i = 0; i < currentNode->valueCount; i++) {
            if (searchValue == currentNode->nodeValues[i]) { found = i; break; }
            if (searchValue < currentNode->nodeValues[i]) { childIdx = i; break; }
        }

        if (found >= 0) {
            if (!currentNode->childPointers[0]) {
                // ── Leaf: remove value directly ───────────────────────────
                for (int i = found; i < currentNode->valueCount - 1; i++)
                    currentNode->nodeValues[i] = currentNode->nodeValues[i+1];
                currentNode->valueCount--;
                fixUnderflow(treeStruct, currentNode, nodePath, childIndexes, pathTop);
                return treeStruct->rootNode;
            } else {
                // ── Internal: replace with in-order predecessor ───────────
                // Push current node; the child we descend into is childPointers[found]
                nodePath[++pathTop] = currentNode;
                childIndexes[pathTop + 1] = found;

                // Walk down to the predecessor leaf (rightmost of left subtree)
                TreeNode* pred = currentNode->childPointers[found];
                while (pred->childPointers[pred->valueCount]) {
                    nodePath[++pathTop] = pred;
                    childIndexes[pathTop + 1] = pred->valueCount;
                    pred = pred->childPointers[pred->valueCount];
                }

                // Replace value and delete predecessor from its leaf
                currentNode->nodeValues[found] = pred->nodeValues[pred->valueCount - 1];
                pred->valueCount--;
                fixUnderflow(treeStruct, pred, nodePath, childIndexes, pathTop);
                return treeStruct->rootNode;
            }
        }

        // Value not in this node; descend
        nodePath[++pathTop] = currentNode;
        childIndexes[pathTop + 1] = childIdx;
        currentNode = currentNode->childPointers[childIdx];
    }

    return rootNode; // value not found
}

// Main function to run the program
int main() {
    FILE *file = fopen("mixed_operations1000_3.txt", "r");
    FILE *csv_file = fopen("output3.csv", "w");
    if (!file || !csv_file) {
        fprintf(stderr, "File opening error\n");
        return 1;
    }
    fprintf(csv_file, "Nodes;LogN;Actual\n");
    TwoThreeTree treeStruct = {NULL, 0};
    int operationType, targetValue;
    char inputLine[100];
    while (fgets(inputLine, sizeof(inputLine), file)) {
        if (sscanf(inputLine, "%d %d", &operationType, &targetValue) != 2) continue;
        switch (operationType) {
            case 0:
                TIMED_LOG_N('+', targetValue, treeStruct.rootNode = insert(&treeStruct, treeStruct.rootNode, targetValue), &treeStruct, csv_file);
                break;
            case 1: {
                bool isValueFound;
                TIMED_LOG_N('?', targetValue, isValueFound = lookup(treeStruct.rootNode, targetValue), &treeStruct, csv_file);
                printf(": %s\n", isValueFound ? "Najdene" : "Nenajdene");
                break;
            }
            case 2:
                TIMED_LOG_N('-', targetValue, treeStruct.rootNode = delet(&treeStruct, treeStruct.rootNode, targetValue), &treeStruct, csv_file);
                break;
            default:
                printf("Chybna operacia: %d\n", operationType);
        }
    }
    fclose(file);
    fclose(csv_file);
    printf("\nFinal Tree: ");
    printPreorder(treeStruct.rootNode);
    printf("\nNodes: %d\n", treeStruct.nodeCount);
    freeNode(&treeStruct, treeStruct.rootNode);
    return 0;
}
