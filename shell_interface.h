#ifndef SHELL_INTERFACE_H
#define SHELL_INTERFACE_H

#include <stddef.h>
#include "dag_manager.h"
#include "scheduler.h"

void shell_loop(dag_t *d, scheduler_t **ps);

#endif