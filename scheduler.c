// scheduler.c
#include "scheduler.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>

scheduler_t *sched_init(dag_t *dag, size_t n_workers) {
    if (!dag || n_workers == 0) return NULL;
    scheduler_t *s = malloc(sizeof(scheduler_t));
    if (!s) return NULL;

    s->dag = dag;
    s->order = NULL;
    s->n_order = 0;
    s->n_workers = n_workers;

    // Create an array to hold all worker thread ID's
    s->workers = malloc(n_workers * sizeof(pthread_t));
    if (!s->workers) goto fail_s;

    // Task queue has been setted up
    //Size is one more than the umber of tasks to distinguish full from empty
    s->q_capacity = (dag->n_tasks > 0 ? dag->n_tasks + 1 : 1);
    s->queue = malloc(s->q_capacity * sizeof(size_t));
    if (!s->queue) goto fail_workers;
    s->q_head = 0;
    s->q_tail = 0;

    s->stop = false;
    if (pthread_mutex_init(&s->mu_queue, NULL) != 0) goto fail_queue;
    if (pthread_cond_init(&s->cv_queue, NULL) != 0) goto fail_mutex;

    return s;

fail_mutex:
    pthread_mutex_destroy(&s->mu_queue);
fail_queue:
    free(s->queue);
fail_workers:
    free(s->workers);
fail_s:
    free(s);
    return NULL;
}

int sched_start(scheduler_t *s) {
    if (!s) return -1;

    // Generate a topological order for the tasks
    if (s->order) free(s->order);
    if (dag_toposort(s->dag, &s->order, &s->n_order) != 0) {
        s->order = NULL;
        return -1;
    }

    // Load the queue the tasks in the sorted order
    for (size_t i = 0; i < s->n_order; ++i) {
        s->queue[s->q_tail] = s->order[i];
        s->q_tail = (s->q_tail + 1) % s->q_capacity;
    }

    // Start each worker thread
    for (size_t i = 0; i < s->n_workers; ++i) {
        if (pthread_create(&s->workers[i], NULL, worker_loop, s) != 0) {
            // If a thread fails to start, will stop all previously created threads 
            pthread_mutex_lock(&s->mu_queue);
            s->stop = true;
            pthread_cond_broadcast(&s->cv_queue);
            pthread_mutex_unlock(&s->mu_queue);
            for (size_t j = 0; j < i; ++j) pthread_join(s->workers[j], NULL);
            return -1;
        }
    }
    return 0;
}

void sched_stop(scheduler_t *s) {
    if (!s) return;

    // Tell all threads to stop
    pthread_mutex_lock(&s->mu_queue);
    s->stop = true;
    pthread_cond_broadcast(&s->cv_queue);
    pthread_mutex_unlock(&s->mu_queue);

    // Waiting for each thread to finish
    for (size_t i = 0; i < s->n_workers; ++i) {
        pthread_join(s->workers[i], NULL);
    }

    pthread_mutex_destroy(&s->mu_queue);
    pthread_cond_destroy(&s->cv_queue);
    free(s->workers);
    free(s->queue);
    free(s->order);
}

void *worker_loop(void *arg) {
    scheduler_t *s = (scheduler_t *)arg;
    while (1) {
        pthread_mutex_lock(&s->mu_queue);
        // Wait until there is a task in the queue or stop signal
        while (s->q_head == s->q_tail && !s->stop) {
            pthread_cond_wait(&s->cv_queue, &s->mu_queue);
        }
        // Exist if there is nothing to do and we are stopping
        if (s->stop && s->q_head == s->q_tail) {
            pthread_mutex_unlock(&s->mu_queue);
            break;
        }
        // A task will be removed from the queue
        size_t idx = s->queue[s->q_head];
        s->q_head = (s->q_head + 1) % s->q_capacity;
        pthread_mutex_unlock(&s->mu_queue);

        int code = execute_task(s, idx);
        if (code == 0) {
            s->dag->tasks[idx]->status = COMPLETED;
        } else {
            s->dag->tasks[idx]->status = FAILED;
        }

        // If task should repeat periodically, schedule it again
        if (s->dag->tasks[idx]->freq > 0) {
            s->dag->tasks[idx]->time += s->dag->tasks[idx]->freq;
            pthread_mutex_lock(&s->mu_queue);
            s->queue[s->q_tail] = idx;
            s->q_tail = (s->q_tail + 1) % s->q_capacity;
            pthread_cond_signal(&s->cv_queue);
            pthread_mutex_unlock(&s->mu_queue);
        }
    }
    return NULL;
}

int execute_task(scheduler_t *s, size_t idx) {
    if (!s || idx >= s->dag->n_tasks) return -1;
    task_t *t = s->dag->tasks[idx];
    pid_t pid = fork();
    if (pid < 0) return -1;
    if (pid == 0) {
        execl("/bin/sh", "sh", "-c", t->cmd, (char *)NULL);
        _exit(127);
    }
    int status;
    if (waitpid(pid, &status, 0) < 0) return -1;
    if (WIFEXITED(status)) return WEXITSTATUS(status);
    return -1;
}
