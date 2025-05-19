#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include "dag_manager.h"
#include "scheduler.h"

static char *my_strdup(const char *s) {
    size_t n = strlen(s) + 1;
    char *p = malloc(n);
    if (p) memcpy(p, s, n);
    return p;
}

static task_t* make_task(const char *id, const char *cmd, int freq) {
    task_t *t = malloc(sizeof(task_t));
    assert(t != NULL);
    t->id     = my_strdup(id);
    t->cmd    = my_strdup(cmd);
    t->time   = 0;
    t->freq   = freq;
    t->status = PENDING;
    assert(t->id && t->cmd);
    return t;
}

// Test invalid initialization inputs
static void test_init_invalid(void) {
    assert(sched_init(NULL, 1) == NULL);
    dag_t *d = dag_init();
    assert(d);
    assert(sched_init(d, 0) == NULL);
    dag_free(d);
}

// Test scheduler on an empty DAG
static void test_empty_dag(void) {
    dag_t *d = dag_init();
    assert(d);
    scheduler_t *s = sched_init(d, 1);
    assert(s);
    assert(sched_start(s) == 0);
    sched_stop(s);
    free(s);
    dag_free(d);
}

// Test single-task execution with 1 worker
static void test_single_worker(void) {
    dag_t *d = dag_init();
    task_t *t = make_task("ONLY", "true", 0);
    assert(dag_add_task(d, t) == 0);

    scheduler_t *s = sched_init(d, 1);
    assert(s);
    assert(sched_start(s) == 0);

    // Wait (up to approx 2s) for the task to complete
    int waited = 0;
    while (t->status == PENDING && waited < 200) {
        usleep(10000);
        waited++;
    }
    sched_stop(s);
    free(s);

    assert(t->status == COMPLETED);
    dag_free(d);
}

// Test multi-task execution with 2 workers
static void test_multi_worker_status(void) {
    dag_t *d = dag_init();
    task_t *t0 = make_task("T0", "true", 0);
    task_t *t1 = make_task("T1", "false", 0);
    assert(dag_add_task(d, t0) == 0);
    assert(dag_add_task(d, t1) == 0);

    scheduler_t *s = sched_init(d, 2);
    assert(s);
    assert(sched_start(s) == 0);

    // Wait (up to approx 2s) for both tasks
    int waited = 0;
    while ((t0->status == PENDING || t1->status == PENDING) && waited < 200) {
        usleep(10000);
        waited++;
    }
    sched_stop(s);
    free(s);

    assert(t0->status == COMPLETED);
    assert(t1->status == FAILED);
    dag_free(d);
}

int main(void) {
    test_init_invalid();
    test_empty_dag();
    test_single_worker();
    test_multi_worker_status();

    printf("✅ All scheduler tests passed!\n");
    return 0;
}