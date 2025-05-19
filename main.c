// main.c
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include "dag_manager.h"
#include "scheduler.h"
#include "shell_interface.h"

volatile sig_atomic_t stop_flag = 0;

void sigint_handler(int sig) {
    (void)sig;
    stop_flag = 1;
    const char msg[] = "\n^C received, exiting...\n";
    write(STDERR_FILENO, msg, sizeof(msg) - 1);
}

void install_signal_handlers(void) {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = sigint_handler;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT,  &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
}

int main(void) {
    dag_t *d = dag_init();
    if (!d) {
        fprintf(stderr, "Error: Failed to initialize DAG\n");
        return EXIT_FAILURE;
    }

    scheduler_t *sched = NULL;
    install_signal_handlers();
    shell_loop(d, &sched);

    if (sched) {
        sched_stop(sched);
        free(sched);
    }
    dag_free(d);
    return EXIT_SUCCESS;
}
