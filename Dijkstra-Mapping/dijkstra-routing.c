/* Mackenzie Bowal
This program initializes a graph and list of cities according to the "canadamap.txt" input file,
and executes Dijkstra's Algorithm on it, using YYC as the source node. 
The original graph, with edge weights represented by an adjacency matrix, the list of cities,
the paths found by the algorithm, and the minimum distance between YYC and every city are printed
to the console.
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <limits.h>
#include <stdbool.h>

#define INF 9999
#define V 21

/* nextNode()
Takes the current distances of nodes reachable by the algorithm and the list of nodes 
currently reachable and returns the node that has not yet been reached which has the 
smallest distance from the source node.
*/

int nextNode(int distances[V], bool inPath[V]) {
    int minimum = INT_MAX;
    int nextIndex;
    for (int n = 0; n < V; n++) {
        if (inPath[n] == false && distances[n] <= minimum) {
            minimum = distances[n];
            nextIndex = n;
        }
    }
    return nextIndex;
}

/* findPathToDest()
Creates a string of the form "XXX-->XXX-->XXX ..." given a destination city, the list of
cities, and the parent array.
*/

int findPathToDest(char path[100], int parent[V], int dest, char cities[V][5]) {
    int p = parent[dest];
    char pathStr[100];
    char helperStr[100];
    bzero(helperStr, 100);
    bzero(pathStr, 100);

    // Create the rightmost part of the string
    strcpy(pathStr, "-->");
    strcat(pathStr, cities[dest]);

    while (p != 0) {
        // Find next parent and concatenate onto the string
        strcpy(helperStr, "-->");
        strcat(helperStr, cities[p]);
        strcat(helperStr, pathStr);
        strcpy(pathStr, helperStr);
        p = parent[p];
        bzero(helperStr, 100);
    }

    // Add YYC as the first city
    strcpy(helperStr, "YYC");
    strcat(helperStr, pathStr);
    strcpy(pathStr, helperStr);

    // copy the string to path and return
    bzero(path, 100);
    strcpy(path, pathStr);
    return 0;
}


/* Main()
Initializes the graph adjacency matrix according to the input file, and then executes Dijkstra's Algorithm. Prints
the shortest paths for each destination city to the console, as well as the distance it takes to get to each 
of them from YYC.
*/

int main() {

    // Initialize cities and graph arrays
    char cities[V][5];
    bzero(cities, V*5);
    int graph[V][V];
    for (int a = 0; a<V; a++) {
        for (int b = 0; b<V; b++) {
            graph[a][b] = INF;
        }
    }     
    
    char city1[5];
    char city2[5];
    bzero(city1, 5);
    bzero(city2, 5);
    int weight = 0;
    char w[5];
    bzero(w, 5);
    int numCities = 0;
    int c1 = 0;
    int c2 = 0;
    int index1 = 0;
    int index2 = 0;

    // Read the first line from the file
    scanf("%s", city1);
    scanf("%s", city2);
    scanf("%s", w);
    weight = atoi(w);

    // Read the rest of the file and initialize the graph matrix and cities array
    for (int j = 0; j <= 39; j++) {

        // Check if each city is already in the cities array
        for (int i = 0; i <= numCities; i++) {
            if (strcmp(cities[i], city1) == 0) {
                index1 = i;
                c1 = 1;
            }
            if (strcmp(cities[i], city2) == 0) {
                index2 = i;
                c2 = 1;
            }
        }

        // If either of the cities are new, add them to the array
        if (c1 == 0) {
            strcpy(cities[numCities], city1);
            index1 = numCities;
            numCities++;
            if (c2 == 0) {
                strcpy(cities[numCities], city2);
                index2 = numCities;
                numCities++;
            }

        } else if (c2 == 0) {
            strcpy(cities[numCities], city2);
            index2 = numCities;
            numCities++;
        }

        // Add edge to graph adjacency matrix
        graph[index1][index2] = weight;
        graph[index2][index1] = weight;

        // Reset variables and read next line
        c1 = 0;
        c2 = 0;
        index1 = 0;
        index2 = 0;
        scanf("%s", city1);
        scanf("%s", city2);
        scanf("%s", w);
        weight = atoi(w);
    }

    // Print the adjacency matrix graph to the screen
    printf("\nGraph:\n");
    for (int k = 0; k < 21; k++) {
        for (int p = 0; p < 21; p++) {
            printf("%5d ", graph[k][p]);
        }
        printf("\n");
    }

    // Print the list of cities to the screen
    printf("\n\nCities:\n");
    for (int l = 0; l < 21; l++) {
        printf("%d: %s\n", l, cities[l]);
    }
    printf("\n\n");

    // Dijkstra's Algorithm
    int distances[V];
    bool inPath[V];
    int parent[V];
    char path[100];

    // Initialize distances, inPath, and parent
    for (int q = 0; q < V; q++) {
        distances[q] = INT_MAX;
        inPath[q] = false;
        parent[q] = -1;

        // Set the parents of nodes neighbouring the start to 0
        if (graph[0][q] < INF) {
            parent[q] = 0;
            distances[q] = graph[0][q];
        }
    }

    inPath[0] = true;
    distances[0] = 0;

    // Loop through all remaining cities and find shortest paths to other cities
    for (int n = 0; n < V-1; n++) {
        int u = nextNode(distances, inPath);
        inPath[u] = true;
        for (int v = 0; v < V; v++) {
            if (distances[v] == INT_MAX) {
                distances[v] = distances[u] + graph[u][v];
                parent[v] = u;
            }
            else if (distances[v] > distances[u] + graph[u][v]) {
                distances[v] = distances[u] + graph[u][v];
                parent[v] = u;
            }
        }
    }

    // Print the paths to the screen
    printf("\nPaths:\n");
    for (int t = 1; t < V; t++) {
        findPathToDest(path, parent, t, cities);
        printf("%s\n", path);
    }
    printf("\n");

    // Print the distances to the screen
    for (int t = 1; t < V; t++) {
        printf("Dist to %s: %d\n", cities[t], distances[t]);
    }
    printf("\n");

    return 0;
}
