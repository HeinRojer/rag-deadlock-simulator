/*
 * rag_simulator.c
 *
 * Graphical Simulator for Resource Allocation Graphs (RAG)
 * CLI-based, C (GCC) on Ubuntu
 *
 * Features:
 *  - Add Processes / Resources
 *  - Add Request edges (P -> R)
 *  - Add Allocation edges (R -> P)
 *  - Display current RAG (text)
 *  - Detect deadlocks: Build Wait-For Graph (P -> P) and find cycles
 *    If cycle found, prints the process-cycle and reconstructs the
 *    R->P edges that create the waits (P -> R -> P ...)
 *
 * Compile:
 *   gcc -std=c11 -O2 -Wall -Wextra rag_simulator.c -o rag
 *
 * Run:
 *   ./rag
 *
 * Author: Rojer Hein (and team)
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAX_PROC 20
#define MAX_RES  20
#define NAMELEN  32

/* ---- Data model ---- */
int n_proc = 0;
int n_res = 0;

/* req[p][r] == 1 means Process p requests Resource r (P -> R) */
int req[MAX_PROC][MAX_RES];
/* alloc[r][p] == 1 means Resource r is allocated to Process p (R -> P) */
int alloc_[MAX_RES][MAX_PROC];

/* Names */
char P[MAX_PROC][NAMELEN];
char R[MAX_RES][NAMELEN];

/* Wait-For Graph: wfg[p1][p2] == 1 means p1 waits for p2 */
int wfg[MAX_PROC][MAX_PROC];

/* ---- Utility ---- */
static void flush_stdin(void) {
    int c;
    while ((c = getchar()) != '\n' && c != EOF) { /* discard */ }
}

static int read_int(const char *prompt, int minv, int maxv) {
    int x;
    while (1) {
        printf("%s", prompt);
        if (scanf("%d", &x) != 1) {
            printf("Invalid input. Try again.\n");
            flush_stdin();
            continue;
        }
        flush_stdin();
        if (x < minv || x > maxv) {
            printf("Value out of range (%d - %d). Try again.\n", minv, maxv);
            continue;
        }
        return x;
    }
}

static void read_string(const char *prompt, char *buf, size_t bufsz) {
    printf("%s", prompt);
    if (fgets(buf, (int)bufsz, stdin) == NULL) {
        buf[0] = '\0';
        return;
    }
    /* strip newline */
    char *nl = strchr(buf, '\n');
    if (nl) *nl = '\0';
}

/* ---- Core operations ---- */
void add_process(void) {
    if (n_proc >= MAX_PROC) {
        printf("Max processes reached (%d).\n", MAX_PROC);
        return;
    }
    char name[NAMELEN];
    read_string("Enter process name: ", name, sizeof(name));
    if (strlen(name) == 0) {
        printf("Empty name. Aborted.\n");
        return;
    }
    /* check duplicates */
    for (int i = 0; i < n_proc; ++i) {
        if (strcmp(P[i], name) == 0) {
            printf("Process with this name already exists.\n");
            return;
        }
    }
    strncpy(P[n_proc], name, NAMELEN-1);
    P[n_proc][NAMELEN-1] = '\0';
    /* clear req column for safety */
    for (int r = 0; r < MAX_RES; ++r) req[n_proc][r] = 0;
    printf("Added Process P%d: %s\n", n_proc, P[n_proc]);
    n_proc++;
}

void add_resource(void) {
    if (n_res >= MAX_RES) {
        printf("Max resources reached (%d).\n", MAX_RES);
        return;
    }
    char name[NAMELEN];
    read_string("Enter resource name: ", name, sizeof(name));
    if (strlen(name) == 0) {
        printf("Empty name. Aborted.\n");
        return;
    }
    /* check duplicates */
    for (int i = 0; i < n_res; ++i) {
        if (strcmp(R[i], name) == 0) {
            printf("Resource with this name already exists.\n");
            return;
        }
    }
    strncpy(R[n_res], name, NAMELEN-1);
    R[n_res][NAMELEN-1] = '\0';
    /* clear alloc row for safety */
    for (int p = 0; p < MAX_PROC; ++p) alloc_[n_res][p] = 0;
    printf("Added Resource R%d: %s\n", n_res, R[n_res]);
    n_res++;
}

void add_request_edge(void) {
    if (n_proc == 0 || n_res == 0) {
        printf("Need at least one process and one resource.\n");
        return;
    }
    printf("Processes:\n");
    for (int i = 0; i < n_proc; ++i) printf("  %d: %s\n", i, P[i]);
    printf("Resources:\n");
    for (int j = 0; j < n_res; ++j) printf("  %d: %s\n", j, R[j]);
    int p = read_int("Enter process index -> ", 0, n_proc-1);
    int r = read_int("Enter resource index -> ", 0, n_res-1);
    if (req[p][r]) {
        printf("Request edge already exists (P%d -> R%d).\n", p, r);
        return;
    }
    req[p][r] = 1;
    printf("Added request edge: %s -> %s\n", P[p], R[r]);
}

void add_allocation_edge(void) {
    if (n_proc == 0 || n_res == 0) {
        printf("Need at least one process and one resource.\n");
        return;
    }
    printf("Resources:\n");
    for (int j = 0; j < n_res; ++j) printf("  %d: %s\n", j, R[j]);
    printf("Processes:\n");
    for (int i = 0; i < n_proc; ++i) printf("  %d: %s\n", i, P[i]);
    int r = read_int("Enter resource index -> ", 0, n_res-1);
    int p = read_int("Enter process index -> ", 0, n_proc-1);
    if (alloc_[r][p]) {
        printf("Allocation edge already exists (R%d -> P%d).\n", r, p);
        return;
    }
    /* enforce single-owner per resource in this simple model:
       if resource already allocated to another process, warn and override */
    for (int pp = 0; pp < n_proc; ++pp) {
        if (alloc_[r][pp]) {
            printf("Warning: Resource %s was allocated to %s. Overriding allocation.\n", R[r], P[pp]);
            alloc_[r][pp] = 0;
        }
    }
    alloc_[r][p] = 1;
    printf("Added allocation edge: %s -> %s\n", R[r], P[p]);
}

void remove_edges_menu(void) {
    printf("1) Remove Request Edge (P -> R)\n2) Remove Allocation Edge (R -> P)\n3) Cancel\n");
    int ch = read_int("Choice: ", 1, 3);
    if (ch == 1) {
        if (n_proc == 0 || n_res == 0) { printf("Empty.\n"); return; }
        for (int i=0;i<n_proc;i++) printf("P%d: %s\n", i, P[i]);
        for (int j=0;j<n_res;j++) printf("R%d: %s\n", j, R[j]);
        int p = read_int("Process index: ", 0, n_proc-1);
        int r = read_int("Resource index: ", 0, n_res-1);
        if (req[p][r]) { req[p][r] = 0; printf("Removed request edge.\n"); }
        else printf("Request edge did not exist.\n");
    } else if (ch == 2) {
        if (n_proc == 0 || n_res == 0) { printf("Empty.\n"); return; }
        for (int j=0;j<n_res;j++) printf("R%d: %s\n", j, R[j]);
        for (int i=0;i<n_proc;i++) printf("P%d: %s\n", i, P[i]);
        int r = read_int("Resource index: ", 0, n_res-1);
        int p = read_int("Process index: ", 0, n_proc-1);
        if (alloc_[r][p]) { alloc_[r][p] = 0; printf("Removed allocation edge.\n"); }
        else printf("Allocation edge did not exist.\n");
    } else {
        printf("Cancelled.\n");
    }
}

void reset_graph(void) {
    memset(req, 0, sizeof(req));
    memset(alloc_, 0, sizeof(alloc_));
    memset(P, 0, sizeof(P));
    memset(R, 0, sizeof(R));
    n_proc = 0;
    n_res = 0;
    printf("Graph reset.\n");
}

void print_rag(void) {
    printf("\n=== Current RAG State ===\n");
    printf("Processes (%d):\n", n_proc);
    for (int i = 0; i < n_proc; ++i) printf("  P%d: %s\n", i, P[i]);
    printf("Resources (%d):\n", n_res);
    for (int j = 0; j < n_res; ++j) printf("  R%d: %s\n", j, R[j]);

    printf("\nRequest Edges (P -> R):\n");
    int any = 0;
    for (int p = 0; p < n_proc; ++p) {
        for (int r = 0; r < n_res; ++r) {
            if (req[p][r]) { printf("  %s -> %s\n", P[p], R[r]); any = 1; }
        }
    }
    if (!any) printf("  (none)\n");

    printf("\nAllocation Edges (R -> P):\n");
    any = 0;
    for (int r = 0; r < n_res; ++r) {
        for (int p = 0; p < n_proc; ++p) {
            if (alloc_[r][p]) { printf("  %s -> %s\n", R[r], P[p]); any = 1; }
        }
    }
    if (!any) printf("  (none)\n");
    printf("=========================\n");
}

/* ---- Build Wait-For Graph (P -> P) ----
   For every request edge P -> R, if R is allocated to P2, add edge P -> P2 in WFG.
*/
void build_wfg(void) {
    memset(wfg, 0, sizeof(wfg));
    for (int p = 0; p < n_proc; ++p) {
        for (int r = 0; r < n_res; ++r) {
            if (req[p][r]) {
                for (int p2 = 0; p2 < n_proc; ++p2) {
                    if (alloc_[r][p2]) {
                        wfg[p][p2] = 1;
                    }
                }
            }
        }
    }
}

/* For reconstructing full P->R->P link for a pair (p -> p2),
   find a resource r such that req[p][r] && alloc_[r][p2].
   Returns index r or -1 if none found.
*/
int find_blocking_resource(int p, int p2) {
    for (int r = 0; r < n_res; ++r) {
        if (req[p][r] && alloc_[r][p2]) return r;
    }
    return -1;
}

/* DFS cycle detection on wfg with stack to reconstruct cycle */
int visited[MAX_PROC];
int in_stack[MAX_PROC];
int stack_nodes[MAX_PROC];
int stack_top;

/* When a cycle back-edge to 'to' is found (to is in_stack), this prints the cycle
   by collecting nodes from the stack. It prints the process-only cycle and also
   reconstructs each P->R->P link using a blocking resource when possible.
*/
void print_cycle_from_stack(int to) {
    int idx = -1;
    for (int i = 0; i <= stack_top; ++i) {
        if (stack_nodes[i] == to) { idx = i; break; }
    }
    if (idx == -1) {
        printf("Cycle start not found in stack (internal error).\n");
        return;
    }
    int len = stack_top - idx + 1;
    printf("\nDetected cycle of %d process(es):\n", len);
    /* print cycle as P -> R -> P -> R -> ... */
    for (int i = idx; i <= stack_top; ++i) {
        int p = stack_nodes[i];
        int nextp;
        if (i < stack_top) nextp = stack_nodes[i+1];
        else nextp = stack_nodes[idx]; /* close the cycle */
        int r = find_blocking_resource(p, nextp);
        if (r >= 0) {
            printf("  %s (P%d)  ->  %s (R%d)  ->  %s (P%d)\n", P[p], p, R[r], r, P[nextp], nextp);
        } else {
            /* Fallback: print P -> P if no exact resource mapping found */
            printf("  %s (P%d)  ->  %s (P%d)   [resource unknown]\n", P[p], p, P[nextp], nextp);
        }
    }
    printf("\n");
}

/* return 1 if any cycle found, 0 otherwise */
int dfs_cycle(int u) {
    visited[u] = 1;
    in_stack[u] = 1;
    stack_nodes[++stack_top] = u;

    for (int v = 0; v < n_proc; ++v) {
        if (!wfg[u][v]) continue;
        if (!visited[v]) {
            if (dfs_cycle(v)) return 1; /* early exit on first cycle */
        } else if (in_stack[v]) {
            /* found back-edge u -> v; reconstruct cycle starting at v */
            print_cycle_from_stack(v);
            return 1;
        }
    }

    in_stack[u] = 0;
    --stack_top;
    return 0;
}

void detect_deadlock(void) {
    if (n_proc == 0) { printf("No processes present.\n"); return; }
    build_wfg();
    memset(visited, 0, sizeof(visited));
    memset(in_stack, 0, sizeof(in_stack));
    stack_top = -1;
    int found = 0;
    for (int i = 0; i < n_proc; ++i) {
        if (!visited[i]) {
            if (dfs_cycle(i)) { found = 1; break; } /* report first cycle only */
        }
    }
    if (!found) {
        printf("\n✔ No deadlock detected (no cycles in Wait-For Graph).\n\n");
    } else {
        printf("❌ Deadlock exists in the system (see above cycle).\n\n");
    }
}

/* ---- Sample prefill to quickly test (optional helper) ---- */
void sample_prefill(void) {
    reset_graph();
    /* processes */
    strncpy(P[0], "P0", NAMELEN); strncpy(P[1], "P1", NAMELEN);
    strncpy(P[2], "P2", NAMELEN);
    n_proc = 3;
    /* resources */
    strncpy(R[0], "R0", NAMELEN); strncpy(R[1], "R1", NAMELEN);
    n_res = 2;
    /* edges forming cycle: P0 -> R0 (req), R0 -> P1 (alloc)
                             P1 -> R1 (req), R1 -> P0 (alloc) */
    memset(req,0,sizeof(req)); memset(alloc_,0,sizeof(alloc_));
    req[0][0] = 1; alloc_[0][1] = 1;
    req[1][1] = 1; alloc_[1][0] = 1;
    printf("Sample graph loaded (P0<->P1 cycle). Use Detect Deadlock to test.\n");
}

/* ---- Main menu ---- */
void print_menu(void) {
    printf("\n===== RAG SIMULATOR (C) =====\n");
    printf("1) Add Process\n");
    printf("2) Add Resource\n");
    printf("3) Add Request Edge (P -> R)\n");
    printf("4) Add Allocation Edge (R -> P)\n");
    printf("5) Remove an Edge\n");
    printf("6) Display Graph\n");
    printf("7) Detect Deadlock\n");
    printf("8) Reset Graph\n");
    printf("9) Load Sample Example (quick test)\n");
    printf("0) Exit\n");
    printf("=============================\n");
}

int main(void) {
    /* initialize */
    memset(req, 0, sizeof(req));
    memset(alloc_, 0, sizeof(alloc_));
    memset(P, 0, sizeof(P));
    memset(R, 0, sizeof(R));

    while (1) {
        print_menu();
        int ch = read_int("Enter choice: ", 0, 9);
        switch (ch) {
            case 1: add_process(); break;
            case 2: add_resource(); break;
            case 3: add_request_edge(); break;
            case 4: add_allocation_edge(); break;
            case 5: remove_edges_menu(); break;
            case 6: print_rag(); break;
            case 7: detect_deadlock(); break;
            case 8: reset_graph(); break;
            case 9: sample_prefill(); break;
            case 0: printf("Exiting. Bye.\n"); return 0;
            default: printf("Invalid choice.\n"); break;
        }
    }

    return 0;
}
