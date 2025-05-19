#ifndef DAG_MANAGER_H
#define DAG_MANAGER_H

#include <stdlib.h>
#include <stdbool.h>
#include <time.h>

//Starting Size for the task list
#define DAG_INITIAL_CAPACITY 16

// Different Task status enumeration
typedef enum { PENDING, RUNNING, COMPLETED, FAILED } task_status_t;

// It Represents a single task that can be scheduled and executed
typedef struct {
    char          *id; // unique name for the task
    char          *cmd; // shell command this task will run
    time_t         time; // when this task should run (in seconds)
    int            freq; // how often it should repeat
    task_status_t  status; // what is happening with this task right now
} task_t;

// Directed Acyclic Graph structure to represents tasks and dependencies
typedef struct {
    task_t       **tasks; // dynamic list of pointers to tasks
    size_t         n_tasks; // how many tasks are currently in the DAG
    size_t         capacity; // Total space currently allocated for task 
    size_t       **deps;
    size_t        *n_deps;
} dag_t;

// Create an new empty DAG; 
//returns NULL if something went wrong with memory allocation
dag_t *dag_init(void);

// Add a new task to the DAG
// Returns 0 if added successfully, -1 if a task with same ID already exist, -2 on if memory allocation failure
int dag_add_task(dag_t *d, task_t *t);

// Look up the position of a task by its ID
// Returns >=0 index if found, or -1 if no such task exists
int dag_find_index(dag_t *d, const char *id);

// Create a dependency between two tasks:
//Task "from" ust run before task "to"
// Returns 
// 0 if the dependency was added successfully,
// -1 if one or both task ID's not found,
// -2 if the dependency already exists,
// -3 if the dependency would create a cycle
int dag_add_dep(dag_t *d, const char *from, const char *to);

// Check DAG for cycles; returns true if cycle exists, otherwise false
bool dag_detect_cycle(dag_t *d);

// Produce a topological ordering of tasks
// Outputs 
// - out_order : pointer to an array of task indices
// - out_n : number of tasks sorted
// Returns 
// 0 if sorting was successful, 
// -1 if the DAG contains cycle, 
// -2 if memory allocation failed
int dag_toposort(dag_t *d, size_t **out_order, size_t *out_n);

// Freeing all the memory associated with the DAG, includes tasks and dependencies
void dag_free(dag_t *d);

#endif
