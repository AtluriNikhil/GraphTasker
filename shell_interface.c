// shell_interface.c
#define _POSIX_C_SOURCE 200809L
#include "shell_interface.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>    // strdup, strcmp, strcspn
#include <ctype.h>     // isspace

#define MAX_TOKENS 16

static const char *status_str(task_status_t s) {
    switch (s) {
      case PENDING:   return "PENDING";
      case RUNNING:   return "RUNNING";
      case COMPLETED: return "COMPLETED";
      case FAILED:    return "FAILED";
      default:        return "UNKNOWN";
    }
}


static void print_error(const char *msg) {
    fprintf(stderr, "[error] %s\n", msg);
}

static void print_help(void) {
    printf(
        "Available commands:\n"
        "  add_task <id> \"<cmd>\" <time> <freq>  - Add a new task\n"
        "  add_dep <from> <to>                   - Add a dependency\n"
        "  show tasks                            - List tasks\n"
        "  show deps                             - List dependencies\n"
        "  run [n_workers]                       - Start scheduler\n"
        "  help                                  - Show this help\n"
        "  exit                                  - Quit\n"
    );
}

// Split a line of input into words/tokens
//Handles quoted strings correctly
static int tokenize_line(char *line, char **out) {
    int n = 0;
    char *p = line;
    while (*p && n < MAX_TOKENS) {
        while (isspace((unsigned char)*p)) p++;
        if (!*p) break;
        if (*p == '"') {
            p++;
            out[n++] = p;
            while (*p && *p != '"') p++;
            if (*p == '"') *p++ = '\0';
        } else {
            out[n++] = p;
            while (*p && !isspace((unsigned char)*p)) p++;
            if (*p) *p++ = '\0';
        }
    }
    return n;
}

// add_task <id> "<cmd>" <time> <freq>
static void handle_add_task(char **argv, int argc, dag_t *d) {
    if (argc != 5) {
        print_error("Usage: add_task <id> \"<cmd>\" <time> <freq>");
        return;
    }
    char *id = argv[1], *cmd = argv[2], *t_s = argv[3], *f_s = argv[4];
    char *endp;

    time_t tval = (time_t)strtoul(t_s, &endp, 10);
    if (*endp) { print_error("Invalid time"); return; }

    long fl = strtol(f_s, &endp, 10);
    if (*endp || fl < 0) { print_error("Invalid freq"); return; }
    int freq = (int)fl;

    task_t *t = malloc(sizeof(*t));
    if (!t) { print_error("Out of memory"); return; }
    t->id = strdup(id);
    t->cmd = strdup(cmd);
    t->time = tval;
    t->freq = freq;
    t->status = PENDING;

    int r = dag_add_task(d, t);
    if (r == 0) {
        printf("Task '%s' added.\n", id);
    } else {
        free(t->id); free(t->cmd); free(t);
        if (r == -1) print_error("Task ID already exists");
        else          print_error("Failed to add task");
    }
}

// add_dep <from> <to>
static void handle_add_dep(char **argv, int argc, dag_t *d) {
    if (argc != 3) {
        print_error("Usage: add_dep <from> <to>");
        return;
    }
    int r = dag_add_dep(d, argv[1], argv[2]);
    if (r == 0) {
        printf("Dependency '%s'->'%s' added.\n", argv[1], argv[2]);
    } else if (r == -1) {
        print_error("Unknown task ID");
    } else if (r == -2) {
        print_error("Dependency already exists");
    } else if (r == -3) {
        print_error("Adding this would create a cycle");
    } else {
        print_error("Failed to add dependency");
    }
}

// show tasks | show deps
static void handle_show(char **argv, int argc, dag_t *d) {
    if (argc != 2) {
        print_error("Usage: show tasks|deps");
        return;
    }
    if (strcmp(argv[1], "tasks") == 0) {
        if (d->n_tasks == 0) {
            printf("No tasks.\n");
            return;
        }
        for (size_t i = 0; i < d->n_tasks; ++i) {
            task_t *t = d->tasks[i];
            printf("[%zu] %s: time=%ld freq=%d status=%s\n", i, t->id, (long)t->time, t->freq, status_str(t->status));
        }
    } else if (strcmp(argv[1], "deps") == 0) {
        if (d->n_tasks == 0) {
            printf("No dependencies.\n");
            return;
        }
        // count total dependencies
        size_t total = 0;
        for (size_t i = 0; i < d->n_tasks; ++i) total += d->n_deps[i];
        if (total == 0) {
            printf("No dependencies.\n");
            return;
        }
        for (size_t i = 0; i < d->n_tasks; ++i) {
            printf("%s -> ", d->tasks[i]->id);
            for (size_t k = 0; k < d->n_deps[i]; ++k) {
                printf("%s ", d->tasks[d->deps[i][k]]->id);
            }
            printf("\n");
        }
    } else {
        print_error("Unknown show option");
    }
}

// run [n_workers]
static void handle_run(char **argv, int argc, scheduler_t **ps, dag_t *d) {
    if (d->n_tasks == 0) {
        print_error("No tasks to run.");
        return;
    }
    size_t n_workers = 4;
    if (argc == 2) {
        char *endp;
        long nw = strtol(argv[1], &endp, 10);
        if (*endp || nw <= 0) { print_error("Invalid worker count"); return; }
        n_workers = (size_t)nw;
    } else if (argc > 2) {
        print_error("Usage: run [n_workers]");
        return;
    }

    if (*ps) {
        sched_stop(*ps);
        free(*ps);
        *ps = NULL;
    }
    scheduler_t *s = sched_init(d, n_workers);
    if (!s || sched_start(s) != 0) {
        print_error("Failed to start scheduler");
        free(s);
    } else {
        *ps = s;
        printf("Scheduler started with %zu workers.\n", n_workers);
    }
}

void shell_loop(dag_t *d, scheduler_t **ps) {
    char *line = NULL;
    size_t cap = 0;

    while (1) {
        if (getline(&line, &cap, stdin) == -1) break;
        line[strcspn(line, "\n")] = '\0';

        char *argv[MAX_TOKENS];
        int argc = tokenize_line(line, argv);
        if (argc == 0) continue;

        if (strcmp(argv[0], "add_task") == 0) {
            handle_add_task(argv, argc, d);
        } else if (strcmp(argv[0], "add_dep") == 0) {
            handle_add_dep(argv, argc, d);
        } else if (strcmp(argv[0], "show") == 0) {
            handle_show(argv, argc, d);
        } else if (strcmp(argv[0], "run") == 0) {
            handle_run(argv, argc, ps, d);
        } else if (strcmp(argv[0], "help") == 0) {
            print_help();
        } else if (strcmp(argv[0], "exit") == 0) {
            break;
        } else {
            print_error("Unknown command (type 'help')");
        }
    }
    free(line);
}