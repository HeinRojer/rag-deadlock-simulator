#include <stdio.h>
#include <stdbool.h>

#define MAX_NODES 50
#define PROCESS 1
#define RESOURCE 2

int graph[MAX_NODES][MAX_NODES];
int nodeType[MAX_NODES];
int nodeCount = 0;

void addProcess() {
    nodeType[nodeCount] = PROCESS;
    nodeCount++;
}

void addResource() {
    nodeType[nodeCount] = RESOURCE;
    nodeCount++;
}

void addEdge(int from, int to) {
    if (graph[from][to] == 1) {
        printf("Edge already exists.\n");
        return;
    }
    graph[from][to] = 1;
    printf("Edge %d -> %d added.\n", from, to);
}

int main() {
    addProcess();
    addResource();
    addEdge(0, 1);
    return 0;
}
