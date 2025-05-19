#include "dag_manager.h"
#include <string.h>
#include <stdio.h>

static int ensure_capacity(dag_t *d) {
    if (d->n_tasks < d->capacity) return 0;
    size_t new_cap = d->capacity * 2;
    
    task_t **new_tasks = realloc(d->tasks, new_cap * sizeof(task_t*));
    if (!new_tasks) return -2;
    d->tasks = new_tasks;

    size_t **new_deps = realloc(d->deps, new_cap * sizeof(size_t*));
    if (!new_deps) return -2;
    d->deps = new_deps;

    size_t *new_n_deps = realloc(d->n_deps, new_cap * sizeof(size_t));
    if (!new_n_deps) return -2;
    d->n_deps = new_n_deps;

    for (size_t i = d->capacity; i < new_cap; ++i) {
        d->deps[i] = NULL;
        d->n_deps[i] = 0;
    }
    d->capacity = new_cap;
    return 0;
}

// An empty DAG has been created and initialized
dag_t *dag_init(void) {
    dag_t *d = malloc(sizeof(dag_t));
    if (!d) return NULL;

    d->tasks = malloc(DAG_INITIAL_CAPACITY * sizeof(task_t*));
    d->deps  = malloc(DAG_INITIAL_CAPACITY * sizeof(size_t*));
    d->n_deps = calloc(DAG_INITIAL_CAPACITY, sizeof(size_t));
    if (!d->tasks || !d->deps || !d->n_deps) {
        free(d->tasks);
        free(d->deps);
        free(d->n_deps);
        free(d);
        return NULL;
    }
    d->n_tasks = 0;
    d->capacity = DAG_INITIAL_CAPACITY;
    for (size_t i = 0; i < d->capacity; ++i) d->deps[i] = NULL;
    return d;
}

// Looking for the index of a task by its ID string
int dag_find_index(dag_t *d, const char *id) {
    if (!d || !id) return -1;
    for (size_t i = 0; i < d->n_tasks; ++i) {
        if (strcmp(d->tasks[i]->id, id) == 0) return (int)i;
    }
    return -1;
}

// Adding a new task to the DAG, only if it doesn't already exist because no duplicates were allowed
int dag_add_task(dag_t *d, task_t *t) {
    if (!d || !t) return -2;
    // Preventing duplicates by checking for existing ID
    if (dag_find_index(d, t->id) >= 0) return -1;
    // To add a new task ensure there is no space
    int cap_res = ensure_capacity(d);
    if (cap_res != 0) return cap_res;
    
    // Task will be appended
    d->tasks[d->n_tasks] = t;
    d->deps[d->n_tasks]  = NULL;
    d->n_deps[d->n_tasks] = 0;
    d->n_tasks++;
    return 0;
}

// Using depth-first search, helper function is used to check for cycles
static bool dfs_cycle(dag_t *d, size_t u, int *colors) {
    colors[u] = 1; // GRAY: Node has been marked as "visiting"
    for (size_t k = 0; k < d->n_deps[u]; ++k) {
        size_t v = d->deps[u][k];
        if (colors[v] == 1) return true;  // back edge => cycle
        if (colors[v] == 0 && dfs_cycle(d, v, colors)) return true;
    }
    colors[u] = 2; // BLACK: Node has been marked as "visited"
    return false;
}

// Checking if DAG has any cycles
bool dag_detect_cycle(dag_t *d) {
    if (!d) return true;
    int *colors = calloc(d->n_tasks, sizeof(int));
    if (!colors) return true; // assume worst-case if allocation fails
    bool has_cycle = false;
    for (size_t i = 0; i < d->n_tasks; ++i) {
        if (colors[i] == 0 && dfs_cycle(d, i, colors)) {
            has_cycle = true;
            break;
        }
    }
    free(colors);
    return has_cycle;
}

// Adding a dependency task "from" must happen before task "to"
int dag_add_dep(dag_t *d, const char *from, const char *to) {
    int i = dag_find_index(d, from);
    int j = dag_find_index(d, to);
    if (i < 0 || j < 0) return -1;

    // Checking for existing dependency, we can't add same dependency twice
    for (size_t k = 0; k < d->n_deps[i]; ++k) {
        if (d->deps[i][k] == (size_t)j) return -2;
    }
    // Adding new dependency
    size_t new_count = d->n_deps[i] + 1;
    size_t *new_arr = realloc(d->deps[i], new_count * sizeof(size_t));
    if (!new_arr) return -1;
    d->deps[i] = new_arr;
    d->deps[i][d->n_deps[i]] = (size_t)j;
    d->n_deps[i] = new_count;

    // Checking for cycles after adding
    if (dag_detect_cycle(d)) {
        // undoing the addition
        d->n_deps[i]--;
        return -3;
    }
    return 0;
}

// Performing Topological sort using Kahn's algorithm
int dag_toposort(dag_t *d, size_t **out_order, size_t *out_n) {
    if (!d || !out_order || !out_n) return -2;
    size_t n = d->n_tasks;
    *out_n = n;
    *out_order = malloc(n * sizeof(size_t));
    if (n > 0 && !*out_order) return -2;

    // Counting the number of incoming edges (indegree)
    size_t *indegree = calloc(n, sizeof(size_t));
    if (n > 0 && !indegree) { free(*out_order); return -2; }
    for (size_t u = 0; u < n; ++u) {
        for (size_t k = 0; k < d->n_deps[u]; ++k) {
            indegree[d->deps[u][k]]++;
        }
    }
    
    // Initializing the queue with nodes that have no dependencies
    size_t *queue = malloc(n * sizeof(size_t));
    if (n > 0 && !queue) { free(indegree); free(*out_order); return -2; }
    size_t qh = 0, qt = 0;
    for (size_t u = 0; u < n; ++u) if (indegree[u] == 0) queue[qt++] = u;

    size_t idx = 0;
    while (qh < qt) {
        size_t u = queue[qh++];
        (*out_order)[idx++] = u;
        for (size_t k = 0; k < d->n_deps[u]; ++k) {
            size_t v = d->deps[u][k];
            if (--indegree[v] == 0) queue[qt++] = v;
        }
    }
    free(queue);
    free(indegree);

    // A cycle must exist, if we didn't process all tasks
    if (idx < n) {
        free(*out_order);
        *out_order = NULL;
        return -1;
    }
    return 0;
}

// Freeing all memory associated with the DAG
void dag_free(dag_t *d) {
    if (!d) return;
    for (size_t i = 0; i < d->n_tasks; ++i) {
        free(d->tasks[i]->id);
        free(d->tasks[i]->cmd);
        free(d->tasks[i]);
        free(d->deps[i]);
    }
    free(d->tasks);
    free(d->deps);
    free(d->n_deps);
    free(d);
}