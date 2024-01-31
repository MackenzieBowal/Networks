/* Mackenzie Bowal
Simulates Datalink layer decision-making.
This program executes two different protocols on customer basket data from an input file. First it reads the file and populates
an integer array representing the leaves of a binary tree. An element is 0 if the item is not in the basket and 1 if it is. The
tree is then passed to a function which probes every element for data. The number of probes, collisions, successes, idles, and the efficiency
is then printed to the console. This process is repeated on a recursive function which calls each half of the input tree upon a collision.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Global variables updated in functions
int k;
int leaves;
int items = 0;
int probes = 0;
int successes = 0;
int idles = 0;
int collisions = 0;

/* linearProbe()
Takes an input tree array and probes each element, checking for data.
*/

int linearProbe(int tree[leaves]) {

    for (int i = 0; i<leaves; i++) {
        probes++;
        if (tree[i] == 0) {
            idles++;
        } else if (tree[i] == 1) {
            successes++;
            items++;
        }
    }
    return 0;
}

/* treeProbe()
Takes an input tree array and start and ending indices and probes for data in that subarray. Will call itself
recursively if the probe results in a collision.
*/

int treeProbe(int tree[leaves], int index1, int index2) {

    // Update probes and add up entries in subarray
    probes++;
    int sum = 0;
    for (int i = 0; i <= index2 - index1; i++) {
        sum = sum + tree[index1 + i];
    }

    // Check for collision, idle, or success
    if (sum == 1) {
        successes++;
        items++;
    } else {
        if (sum == 0) {
            idles++;
        } else {
            collisions++;
            int i1;
            int i2;
            i2 = index1 + (index2-index1)/2;
            i1 = i2 + 1;
            treeProbe(tree, index1, i2);
            treeProbe(tree, i1, index2);
        }
    }
    return 0;
}

/* Main()
Fills out the input tree array using command line and file data. Then calls each of the other two functions
on the tree and prints the resulting data to the console.
*/

int main(int argc, char *argv[]) {

    // Get k from command line
    k = atoi(argv[1]);

    leaves = 1;
    for (int b = 0; b < k; b++) {
        leaves = leaves * 2;
    }
    int tree[leaves];
    char num[20];
    char prev[20];
    bzero(num, 20);
    bzero(prev, 20);
    int j = 0;

    // Initialize the tree array to be empty (full of 0s)
    for (int a=0; a < leaves; a++) {
        tree[a] = 0;
    }

    scanf("%s", num);
    // Populate tree array with file data
    while (strcmp(prev, num) != 0) {
        j = atoi(num);
        tree[j] = 1;
        strcpy(prev, num);
        scanf("%s", num);
    }

    // Probe each leaf and print results
    linearProbe(tree);
    printf("\n\nLEAF-FIRST:\nItems\tProbes\tSuccesses\tIdles\tCollisions\tEfficiency\n_______________________________________________________________\n%d\t%d\t%d\t\t%d\t%d\t\t%d/%d\n", items, probes, successes, idles, collisions, successes, probes);     

    items = 0;
    probes = 0;
    successes = 0;
    idles = 0;
    collisions = 0;

    // Probe with the recursive approach and print results
    treeProbe(tree, 0, leaves-1);
    printf("\n\nROOT-FIRST:\nItems\tProbes\tSuccesses\tIdles\tCollisions\tEfficiency\n_______________________________________________________________\n%d\t%d\t%d\t\t%d\t%d\t\t%d/%d\n\n", items, probes, successes, idles, collisions, successes, probes);     
    
    return 0;
}