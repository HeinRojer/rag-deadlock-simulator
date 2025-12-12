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

// NEW HELPER FUNCTION
void getNodeName(int id, char *buffer) {
    if (nodeType[id] == PROCESS)
        sprintf(buffer, "P%d", id);
    else
        sprintf(buffer, "R%d", id);
}

void displayGraph() {
    char nameA[10], nameB[10];

    printf("\n===== RAG =====\n");

    for (int i = 0; i < nodeCount; i++) {
        getNodeName(i, nameA);
        printf("%s: ", nameA);

        for (int j = 0; j < nodeCount; j++) {
            if (graph[i][j] == 1) {
                getNodeName(j, nameB);
                printf("-> %s ", nameB);
            }
        }
        printf("\n");
    }

    printf("================\n");
}

int main() {
    addProcess();
    addResource();
    addEdge(0, 1);

    displayGraph();
    return 0;
}
