#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <stddef.h>
#include <stdbool.h>
#include <pthread.h>
#include "dag_manager.h"

// Scheduler is responsible for managing multiple worker threads to execute tasks from DAG concurrently and efficiently 

typedef struct {
    dag_t          *dag; // DAG holds all the tasks
    size_t         *order; // It stores the order of tasks
    size_t          n_order;

    pthread_t      *workers; // Array to store thread ID's of all workers
    size_t          n_workers;

    size_t         *queue; // Circular queue to hold task indices ready to run
    size_t          q_head; // index of next task to take from the queue
    size_t          q_tail; // index where the next task will be added
    size_t          q_capacity; // Total capacity of the queue

    pthread_mutex_t mu_queue; // Mutex to guard access to the queue and control flags
    pthread_cond_t  cv_queue; // Consitional variables to signal changes in the queue

    bool            stop; // Set to true when threads should stop running
} scheduler_t;

/*
With the given DAG and number of worker threads, it creates and set up scheduler
It only initializes internal structure - it doesn't start the threads yet
Returns a pointer to the scheduler on success, or NULL if memory allocation fails
 */
scheduler_t *sched_init(dag_t *dag, size_t n_workers);

/*
By launching all the worker threads, will start the scheduler
Each thread runs the worker_loop() to pick and execute tasks
Returns 0 if everything starts correctly
If any thread fails to start, it will stop all others, cleans up and return -1
 */
int sched_start(scheduler_t *s);

/*
Stops the scheduler by signaling all worker threads to finish their work and exit
Broadcast a stop signal, wait for all threads to complete and then cleans up all thread related resources like mutexex, condition variables, queues etc
 */
void sched_stop(scheduler_t *s);

/*
This is the main logic function that each worker thread runs:
waits until there is task available or until a stop signal is received
picks a task from the queue and executes it using execute_task()
Updates the task status depending on whether it ran successfully
If the task is recurring one, it re-adds to the queue.
This function is passed to pthread_create when starting threads
 */
void *worker_loop(void *arg);

/*
Run the command for specific task in the DAG
It:
forks a new process
execute the shell command using /bin/sh
waits for the task to complete
returns the exit status from the command (0 = success, non-zero = failure)
 */
int execute_task(scheduler_t *s, size_t idx);

#endif