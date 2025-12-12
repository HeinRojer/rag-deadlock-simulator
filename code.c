#include <stdio.h>
#include <stdbool.h>

#define MAX_NODES 50
#define PROCESS 1
#define RESOURCE 2

int graph[MAX_NODES][MAX_NODES];
int nodeType[MAX_NODES];
int nodeCount = 0;

bool visited[MAX_NODES];
bool recStack[MAX_NODES];

void addProcess() { nodeType[nodeCount++] = PROCESS; }
void addResource() { nodeType[nodeCount++] = RESOURCE; }
void addEdge(int a, int b) { graph[a][b] = 1; }

bool dfs(int node) {
    visited[node] = true;
    recStack[node] = true;

    for (int next = 0; next < nodeCount; next++) {
        if (graph[node][next] == 1) {
            if (!visited[next] && dfs(next)) return true;
            if (recStack[next]) return true;
        }
    }

    recStack[node] = false;
    return false;
}

void detectDeadlock() {
    for (int i = 0; i < nodeCount; i++)
        visited[i] = recStack[i] = false;

    for (int i = 0; i < nodeCount; i++)
        if (!visited[i] && dfs(i)) {
            printf("Deadlock detected!\n");
            return;
        }

    printf("No deadlock.\n");
}

int main() {
    addProcess();
    addResource();
    addEdge(0, 1);
    addEdge(1, 0);
    detectDeadlock();
    return 0;
}
