#include "test.h" // Include the test.h header file for timing and logging macros
#include <stdio.h> // Include standard input/output library
#include <stdlib.h> // Include standard library for memory allocation
#include <math.h> // Include math library for mathematical functions like log2
#include <string.h> // Include string library
#include <limits.h> // Include limits library

/* Linux / POSIX path resolution (replaces Windows _fullpath/_MAX_PATH) */
#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

typedef enum { RED, BLACK } Color; // Define an enumeration for node colors
struct Node { // Define a structure for a node in the Red-Black Tree
    int value; // Integer field to store the value of the node
    Color color; // Color field to store the color of the node (RED or BLACK)
    struct Node *left; // Pointer to the left child of the node
    struct Node *right; // Pointer to the right child of the node
    struct Node *parent; // Pointer to the parent of the node
}; // End of the Node structure definition
struct Tree { // Define a structure for the Red-Black Tree itself
    int count; // Integer field to store the number of nodes in the tree
    struct Node *root; // Pointer to the root node of the tree
}; // End of the Tree structure definition
void rotate_left(struct Tree *tree, struct Node *x) { // Function to perform a left rotation on the tree at node x
    struct Node *y = x->right; // Set y as the right child of node x
    x->right = y->left; // Move y's left subtree to become x's right subtree
    if (y->left != NULL) y->left->parent = x; // If y has a left child, update its parent to x
    y->parent = x->parent; // Set y's parent to x's parent (y takes x's place)

    if (x->parent == NULL) // Check if x is the root (has no parent)
        tree->root = y; // If x is the root, update the tree's root to y
    else if (x == x->parent->left) // Check if x is the left child of its parent
        x->parent->left = y; // If x is a left child, update the parent's left pointer to y
    else // If x is not the left child, it must be the right child
        x->parent->right = y; // Update the parent's right pointer to y

    y->left = x; // Make x the left child of y
    x->parent = y; // Update x's parent to y
} // End of the rotate_left function
void rotate_right(struct Tree *tree, struct Node *y) { // Function to perform a right rotation on the tree at node y
    struct Node *x = y->left; // Set x as the left child of node y
    y->left = x->right; // Move x's right subtree to become y's left subtree
    if (x->right != NULL) x->right->parent = y; // If x has a right child, update its parent to y
    x->parent = y->parent; // Set x's parent to y's parent (x takes y's place)

    if (y->parent == NULL) // Check if y is the root (has no parent)
        tree->root = x; // If y is the root, update the tree's root to x
    else if (y == y->parent->right) // Check if y is the right child of its parent
        y->parent->right = x; // If y is a right child, update the parent's right pointer to x
    else // If y is not the right child, it must be the left child
        y->parent->left = x; // Update the parent's left pointer to x

    x->right = y; // Make y the right child of x
    y->parent = x; // Update y's parent to x
} // End of the rotate_right function
int log2_tree_height(struct Node *root) { // Function to calculate the height of the tree using log2
    if (root == NULL) { // Check if the current node (subtree root) is NULL
        return 0; // If the node is NULL, return 0 (base case for empty tree)
    } // End of the if statement for the base case
    int left_height = log2_tree_height(root->left); // Recursively calculate the height of the left subtree
    int right_height = log2_tree_height(root->right); // Recursively calculate the height of the right subtree
    return (left_height > right_height ? left_height : right_height) + 1; // Return the maximum height between left and right subtrees, plus 1 for the current node
} // End of the log2_tree_height function
struct Node* tree_search(struct Node* root, int value) { // Function to search for a node with a given value in the tree
    while (root != NULL && root->value != value) { // Loop while the current node is not NULL and its value doesn't match the target value
        root = (value < root->value) ? root->left : root->right; // Move to the left child if value is less than the current node's value, otherwise move to the right child
    } // End of the while loop for searching
    return root; // Return the found node (or NULL if not found)
} // End of the tree_search function
struct Node* minimum(struct Node* node) { // Function to find the node with the minimum value in a subtree
    while (node->left != NULL) // Loop while the current node has a left child
        node = node->left; // Move to the left child
    return node; // Return the leftmost node (which has the minimum value)
} // End of the minimum function
void tree_delete_fixup(struct Tree *tree, struct Node *x) { // Function to fix Red-Black Tree properties after deletion
    while (x != NULL && x != tree->root && x->color == BLACK) { // Loop while x exists, is not the root, and is BLACK
        if (x->parent == NULL) break; // Safety: stop if parent is NULL
        if (x == x->parent->left) { // Check if x is the left child of its parent
            struct Node *w = x->parent->right; // Set w as x's sibling (right child of x's parent)
            if (w == NULL) { x = x->parent; continue; } // Safety: no sibling, move up

            if (w->color == RED) { // Check if the sibling is RED (Case 1)
                w->color = BLACK; // Change the sibling's color to BLACK
                x->parent->color = RED; // Change x's parent's color to RED
                rotate_left(tree, x->parent); // Perform a left rotation on x's parent
                w = x->parent->right; // Update w to the new right child of x's parent after rotation
                if (w == NULL) { x = x->parent; continue; }
            } // End of Case 1

            if ((w->left == NULL || w->left->color == BLACK) && // Check if the sibling's left child is NULL or BLACK
                (w->right == NULL || w->right->color == BLACK)) { // Check if the sibling's right child is NULL or BLACK
                w->color = RED; // Change the sibling's color to RED
                x = x->parent; // Move x up to its parent to continue the fix-up process
            } else { // Proceed to Case 3 or 4
                if (w->right == NULL || w->right->color == BLACK) { // Case 3
                    if (w->left) w->left->color = BLACK;
                    w->color = RED;
                    rotate_right(tree, w);
                    w = x->parent->right;
                    if (w == NULL) { x = x->parent; continue; }
                } // End of Case 3
                w->color = x->parent->color; // Case 4
                x->parent->color = BLACK;
                if (w->right) w->right->color = BLACK;
                rotate_left(tree, x->parent);
                x = tree->root; // Set x to the root to exit the loop
            } // End of the else block for Cases 3 and 4
        } else { // Symmetric case when x is the right child of its parent
            struct Node *w = x->parent->left; // Set w as x's sibling (left child of x's parent)
            if (w == NULL) { x = x->parent; continue; } // Safety: no sibling, move up

            if (w->color == RED) { // Symmetric Case 1
                w->color = BLACK;
                x->parent->color = RED;
                rotate_right(tree, x->parent);
                w = x->parent->left;
                if (w == NULL) { x = x->parent; continue; }
            }
            if ((w->right == NULL || w->right->color == BLACK) &&
                (w->left == NULL || w->left->color == BLACK)) { // Symmetric Case 2
                w->color = RED;
                x = x->parent;
            } else {
                if (w->left == NULL || w->left->color == BLACK) { // Symmetric Case 3
                    if (w->right) w->right->color = BLACK;
                    w->color = RED;
                    rotate_left(tree, w);
                    w = x->parent->left;
                    if (w == NULL) { x = x->parent; continue; }
                } // End of symmetric Case 3
                w->color = x->parent->color; // Symmetric Case 4
                x->parent->color = BLACK;
                if (w->left) w->left->color = BLACK;
                rotate_right(tree, x->parent);
                x = tree->root; // Set x to the root to exit the loop
            }
        } // End of the else block for the symmetric case
    } // End of the while loop for the fix-up process
    if (x) x->color = BLACK; // If x exists, ensure its color is BLACK (root must be BLACK)
} // End of the tree_delete_fixup function
void tree_delete(struct Tree *tree, int value) { // Function to delete a node with a given value from the tree
    struct Node *z = tree_search(tree->root, value); // Search for the node with the given value in the tree
    if (z == NULL) { // Check if the node was not found
        printf("Not in the tree: %d\n", value); // Print an error message indicating the value is not in the tree
        return; // Exit the function since the node doesn't exist
    } // End of the if statement for handling a non-existent node
    struct Node *y = z; // Set y to the node to be deleted (z)
    struct Node *x; // Declare a pointer for the child of the node that will replace z
    struct Node *x_parent = NULL; // Track x's parent explicitly (needed when x is NULL)
    Color y_original_color = y->color; // Store the original color of y for later use in fix-up
    if (z->left == NULL) { // Check if z has no left child
        x = z->right; // Set x to z's right child (which may be NULL)
        x_parent = z->parent;
        if (x) x->parent = z->parent; // If x exists, update its parent to z's parent
        if (z->parent == NULL) tree->root = x; // If z is the root, update the tree's root to x
        else if (z == z->parent->left) z->parent->left = x; // If z is a left child, update the parent's left pointer to x
        else z->parent->right = x; // If z is a right child, update the parent's right pointer to x
    } else if (z->right == NULL) { // Check if z has no right child (but has a left child)
        x = z->left; // Set x to z's left child
        x_parent = z->parent;
        if (x) x->parent = z->parent; // If x exists, update its parent to z's parent
        if (z->parent == NULL) tree->root = x; // If z is the root, update the tree's root to x
        else if (z == z->parent->left) z->parent->left = x; // If z is a left child, update the parent's left pointer to x
        else z->parent->right = x; // If z is a right child, update the parent's right pointer to x
    } else { // If z has two children
        y = minimum(z->right); // Find the successor (minimum node in the right subtree)
        y_original_color = y->color; // Update the original color to the successor's color
        x = y->right; // Set x to the right child of the successor (may be NULL)
        if (y->parent == z) { // Check if the successor's parent is z (successor is the direct right child)
            x_parent = y;
            if (x) x->parent = y; // If x exists, update its parent to y
        } else { // If the successor is not the direct right child of z
            x_parent = y->parent;
            if (x) x->parent = y->parent; // If x exists, update its parent to the successor's parent
            y->parent->left = x; // Update the successor's parent's left pointer to x
            y->right = z->right; // Set the successor's right child to z's right child
            z->right->parent = y; // Update the parent of z's right child to the successor
        } // End of the else block for handling the successor's position
        if (z->parent == NULL) tree->root = y; // If z is the root, update the tree's root to the successor
        else if (z == z->parent->left) z->parent->left = y; // If z is a left child, update the parent's left pointer to the successor
        else z->parent->right = y; // If z is a right child, update the parent's right pointer to the successor
        y->parent = z->parent; // Set the successor's parent to z's parent
        y->left = z->left; // Set the successor's left child to z's left child
        z->left->parent = y; // Update the parent of z's left child to the successor
        y->color = z->color; // Set the successor's color to z's color
    } // End of the else block for handling two children
    free(z); // Free the memory of the deleted node (z)
    tree->count--; // Decrement the node count in the tree
    if (y_original_color == BLACK) { // Check if the original color of the deleted node was BLACK
        if (x != NULL)
            tree_delete_fixup(tree, x); // Fix-up starting from x
        else if (x_parent != NULL) {
            /* x is NULL (replaced by a NULL sentinel); pass a temporary
               sentinel approach: we fix from x_parent using the sibling. */
            /* Create a placeholder to walk up the tree */
            struct Node placeholder;
            placeholder.color = BLACK;
            placeholder.left = placeholder.right = NULL;
            placeholder.parent = x_parent;
            /* Attach placeholder temporarily */
            if (x_parent->left == NULL && x_parent->right != NULL)
                x_parent->left = &placeholder;
            else
                x_parent->right = &placeholder;
            tree_delete_fixup(tree, &placeholder);
            /* Detach placeholder */
            if (x_parent->left == &placeholder) x_parent->left = NULL;
            else if (x_parent->right == &placeholder) x_parent->right = NULL;
        }
    }
} // End of the tree_delete function
void tree_fix_up(struct Tree *tree, struct Node *z) { // Function to fix Red-Black Tree properties after insertion
    while (z->parent && z->parent->color == RED) { // Loop while z has a parent and the parent is RED
        if (z->parent == z->parent->parent->left) { // Check if z's parent is the left child of its grandparent
            struct Node *y = z->parent->parent->right; // Set y as the uncle (right child of z's grandparent)

            if (y && y->color == RED) { // Check if the uncle exists and is RED (Case 1)
                z->parent->color = BLACK;
                y->color = BLACK;
                z->parent->parent->color = RED;
                z = z->parent->parent; // Move z up to its grandparent
            } else { // Case 2 or 3
                if (z == z->parent->right) { // Case 2: triangle configuration
                    z = z->parent;
                    rotate_left(tree, z);
                } // End of Case 2
                z->parent->color = BLACK; // Case 3
                z->parent->parent->color = RED;
                rotate_right(tree, z->parent->parent);
            }
        } else { // Symmetric case when z's parent is the right child of its grandparent
            struct Node *y = z->parent->parent->left; // Uncle
            if (y && y->color == RED) { // Symmetric Case 1
                z->parent->color = BLACK;
                y->color = BLACK;
                z->parent->parent->color = RED;
                z = z->parent->parent;
            } else {
                if (z == z->parent->left) { // Symmetric Case 2
                    z = z->parent;
                    rotate_right(tree, z);
                }
                z->parent->color = BLACK; // Symmetric Case 3
                z->parent->parent->color = RED;
                rotate_left(tree, z->parent->parent);
            }
        }
    }
    tree->root->color = BLACK; // Ensure the root of the tree is BLACK
} // End of the tree_fix_up function
struct Tree *tree_create() { // Function to create a new empty Red-Black Tree
    struct Tree *tree = (struct Tree *)malloc(sizeof(struct Tree));
    tree->count = 0;
    tree->root = NULL;
    return tree;
} // End of the tree_create function
void tree_insert(struct Tree *tree, int value) { // Function to insert a new value into the tree
    struct Node *new_node = (struct Node *)malloc(sizeof(struct Node));
    new_node->value = value;
    new_node->color = RED; // New nodes are always RED
    new_node->left = new_node->right = new_node->parent = NULL;
    struct Node *y = NULL;
    struct Node *x = tree->root;
    while (x != NULL) {
        y = x;
        if (new_node->value < x->value)
            x = x->left;
        else
            x = x->right;
    }
    new_node->parent = y;
    if (y == NULL)
        tree->root = new_node;
    else if (new_node->value < y->value)
        y->left = new_node;
    else
        y->right = new_node;
    tree->count++;
    tree_fix_up(tree, new_node); // Restore Red-Black properties after insertion
} // End of the tree_insert function
void preOrder_recursive(struct Node *node) { // Function to perform a pre-order traversal of the tree for printing
    if (node == NULL) return;
    printf("%d(%s) ", node->value, node->color == RED ? "R" : "B");
    preOrder_recursive(node->left);
    preOrder_recursive(node->right);
} // End of the preOrder_recursive function

/* Free all nodes recursively */
void tree_free_nodes(struct Node *node) {
    if (!node) return;
    tree_free_nodes(node->left);
    tree_free_nodes(node->right);
    free(node);
}

int main() { // Main function, entry point of the program
    struct Tree *tree = tree_create(); // Create a new Red-Black Tree
    if (!tree) return 1;

    FILE* input = fopen("mixed_operations1000_3.txt", "r"); // Open the input file
    if (!input) {
        perror("Failed to open input file");
        free(tree);
        return 1;
    }
    printf("Input file opened successfully.\n");

    FILE* csv_file = fopen("output3.csv", "w"); // Open the CSV file
    if (!csv_file) {
        perror("Failed to open CSV file");
        fclose(input);
        free(tree);
        return 1;
    }
    printf("CSV file opened successfully.\n");

    fprintf(csv_file, "Nodes;LogN;Actual\n"); // Write the CSV header

    int action, value;
    while (fscanf(input, "%d %d", &action, &value) == 2) {
        if (action == 0) { // Insert operation
            TIMED_LOG_N_CSV('+', value, {
                if (!tree_search(tree->root, value)) {
                    tree_insert(tree, value);
                }
            }, tree, csv_file);
        } else if (action == 1) { // Search operation
            printf("?%-4d:", value);
            TIMED_LOG_N_CSV('?', value, {
                char result = tree_search(tree->root, value) ? 'Y' : 'N';
                printf("%c ", result);
            }, tree, csv_file);
        } else if (action == 2) { // Delete operation
            TIMED_LOG_N_CSV('-', value, {
                if (tree_search(tree->root, value)) {
                    tree_delete(tree, value);
                }
            }, tree, csv_file);
        } else {
            fprintf(stderr, "Invalid action: %d\n", action);
        }
    }

    fclose(input);
    fclose(csv_file);

    printf("\nFinal tree (%d nodes): ", tree->count);
    if (tree->root) {
        preOrder_recursive(tree->root);
    }
    printf("\n");

    tree_free_nodes(tree->root);
    free(tree);
    return 0;
} // End of the main function
