# Makefile for Graph Tasker

# Compiler and flags
CC        := gcc
CFLAGS    := -std=c11 -Wall -Wextra -pthread
ASANFLAGS := -fsanitize=address,undefined
TSANFLAGS := -fsanitize=thread

# Source files
SRC       := dag_manager.c scheduler.c shell_interface.c main.c
TEST_DAG  := test_dag_manager.c
TEST_SCH  := test_scheduler.c

# Targets
.PHONY: all clean test test_dag test_sched sanitize race integration

all: task_scheduler

# Build the main executable
task_scheduler: $(SRC)
	$(CC) $(CFLAGS) $^ -o $@

# Unit tests
test_dag_manager: dag_manager.c $(TEST_DAG)
	$(CC) $(CFLAGS) $(ASANFLAGS) $^ -o $@

test_scheduler: dag_manager.c scheduler.c $(TEST_SCH)
	$(CC) $(CFLAGS) $(ASANFLAGS) $^ -o $@

# Run all unit tests
test: test_dag_manager test_scheduler
	@echo "Running DAG Manager tests..."
	./test_dag_manager
	@echo "Running Scheduler tests..."
	./test_scheduler

test_dag: test_dag_manager
	@echo "Running DAG Manager tests..."
	./test_dag_manager

test_sched: test_scheduler
	@echo "Running Scheduler tests..."
	./test_scheduler

# Sanitizer builds
sanitize: dag_manager.c scheduler.c shell_interface.c main.c
	$(CC) $(CFLAGS) $(ASANFLAGS) $^ -o sanitize_bin
	@echo "Built sanitize_bin with ASan/UBSan."

race: dag_manager.c scheduler.c shell_interface.c main.c
	$(CC) $(CFLAGS) $(TSANFLAGS) $^ -o race_bin
	@echo "Built race_bin with TSan."

# Integration test (requires test_shell.sh)
integration: task_scheduler test_shell.sh
	@echo "Running integration tests..."
	./test_shell.sh

# Clean up build artifacts
clean:
	rm -f task_scheduler sanitize_bin race_bin
	rm -f test_dag_manager test_scheduler
	rm -f *.o