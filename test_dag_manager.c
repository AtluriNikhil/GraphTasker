#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dag_manager.h"

static char *my_strdup(const char *s) {
    size_t n = strlen(s) + 1;
    char *p = malloc(n);
    if (p) memcpy(p, s, n);
    return p;
}

static task_t* make_task(const char *id) {
    task_t *t = malloc(sizeof(task_t));
    if (!t) return NULL;
    t->id     = my_strdup(id);
    t->cmd    = my_strdup("");
    t->time   = 0;
    t->freq   = 0;
    t->status = PENDING;
    if (!t->id || !t->cmd) {
        free(t->id);
        free(t->cmd);
        free(t);
        return NULL;
    }
    return t;
}

static void free_task(task_t *t) {
    if (!t) return;
    free(t->id);
    free(t->cmd);
    free(t);
}

static void die(const char *msg) {
    fprintf(stderr, "❌ %s\n", msg);
    exit(1);
}

int main(void) {
    dag_t *d = dag_init();
    if (!d) die("dag_init() returned NULL");

    // 1) dag_find_index on empty DAG
    if (dag_find_index(d, "X") != -1) die("Empty DAG: dag_find_index should be -1");

    // 2) Add two tasks A and B
    task_t *tA = make_task("A"), *tB = make_task("B");
    if (!tA || !tB) die("make_task failed");
    if (dag_add_task(d, tA) != 0) die("Failed to add task A");
    if (dag_add_task(d, tB) != 0) die("Failed to add task B");

    // 3) Duplicate-task detection
    task_t *tDup = make_task("A");
    int r = dag_add_task(d, tDup);
    if (r != -1) die("Duplicate ID not detected");
    free_task(tDup);

    // 4) dag_find_index correctness
    int idxA = dag_find_index(d, "A");
    int idxB = dag_find_index(d, "B");
    if (idxA < 0 || idxB < 0 || idxA == idxB) die("dag_find_index returned wrong indices");

    // 5) dag_add_dep: missing IDs
    if (dag_add_dep(d, "X", "Y") != -1) die("Missing-ID case failed");

    // 6) Valid dependency A -> B
    if (dag_add_dep(d, "A", "B") != 0) die("Failed to add valid dependency A->B");

    // 7) Duplicate-dependency detection
    if (dag_add_dep(d, "A", "B") != -2) die("Duplicate-dependency not detected");

    // 8) Cycle-prevention: B -> A would create a cycle
    if (dag_add_dep(d, "B", "A") != -3) die("Cycle not detected");

    // 9) dag_detect_cycle should still be false (we rolled back)
    if (dag_detect_cycle(d)) die("dag_detect_cycle() should be false for acyclic graph");

    // 10) Topological sort on simple graph
    size_t *order = NULL, n_order = 0;
    r = dag_toposort(d, &order, &n_order);
    if (r != 0) die("Toposort failed on acyclic graph");
    if (n_order != 2)      die("Toposort returned wrong length");
    if (order[0] != (size_t)idxA || order[1] != (size_t)idxB) {
        die("Toposort order incorrect (expected A before B)");
    }
    free(order);

    // 11) Force a resize by adding > DAG_INITIAL_CAPACITY tasks
    size_t initial_cap = DAG_INITIAL_CAPACITY;
    char name[32];
    for (size_t i = 0; i < initial_cap; ++i) {
        snprintf(name, sizeof(name), "T%zu", i);
        task_t *t = make_task(name);
        if (!t) die("make_task failed during resizing test");
        if (dag_add_task(d, t) != 0) die("Failed to add task during resizing loop");
    }
    if (d->capacity <= initial_cap) die("ensure_capacity did not grow capacity");

    // 12) Finally, confirm no accidental cycles introduced
    if (dag_detect_cycle(d)) die("Unexpected cycle after bulk-add");

    // Clean up
    dag_free(d);

    printf("✅ All dag_manager tests passed!\n");
    return 0;
}
