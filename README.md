# Graph Tasker

[![Build Status](https://img.shields.io/badge/build-passing-brightgreen)](#)  
[![License](https://img.shields.io/badge/license-MIT-blue)](LICENSE)

A lightweight, POSIX-compliant concurrent task scheduler with DAG-based dependency management and a simple shell interface.

---

## Features

- **DAG-Based Dependencies**: Prevents cycles, ensures correct execution order.  
- **Concurrent Execution**: Worker-thread pool with mutex/condition-variable synchronization.  
- **Periodic & One-Shot Tasks**: Built-in support for repeating tasks via `freq`.  
- **Scriptable Shell**: `add_task`, `add_dep`, `show`, `run`, `help`, `exit`.  
- **Robust Testing**: Unit tests, integration scripts, AddressSanitizer, ThreadSanitizer.  
- **CI-Ready**: Example Makefile and GitHub Actions workflow included.

---

## Quickstart

```bash
# Clone the repository
git clone https://github.com/yourname/GraphTasker.git
cd GraphTasker

# Build the CLI
make

# Start the interactive shell
./task_scheduler
````

### Shell Commands

```text
add_task <id> "<cmd>" <time> <freq>   # schedule a task
add_dep <from> <to>                   # declare dependency
show tasks                            # list all tasks
show deps                             # list all dependencies
run [n_workers]                       # start scheduler
help                                  # show usage
exit                                  # quit
```

---

## Building & Testing

This project includes a `Makefile` with the following targets:

| Target          | Description                                               |
| --------------- | --------------------------------------------------------- |
| `all` (default) | Build the `task_scheduler` binary                         |
| `test`          | Run all unit tests (`test_dag_manager`, `test_scheduler`) |
| `test_dag`      | Run only the DAG-manager unit tests                       |
| `test_sched`    | Run only the Scheduler unit tests                         |
| `integration`   | Run end-to-end shell integration script                   |
| `sanitize`      | Build with ASan/UBSan (`sanitize_bin`)                    |
| `race`          | Build with TSan (`race_bin`)                              |
| `clean`         | Remove all build artifacts                                |

---

## Testing & Validation

Graph Tasker ships with a full suite of unit tests, integration scripts, and sanitizer/race‐detector builds. Below are the exact commands to verify everything before you push.

### 1) Build the CLI

```bash
make
````

This produces:

* `task_scheduler` (the main shell binary)

---

### 2) Unit Tests

#### DAG Manager Tests

```bash
make test_dag
# or, equivalently:
./test_dag_manager
```

You should see:

```
✅ All dag_manager tests passed!
```

#### Scheduler Tests

```bash
make test_sched
# or, equivalently:
./test_scheduler
```

You should see:

```
✅ All scheduler tests passed!
```

#### Run All Unit Tests

```bash
make test
```

---

### 3) End-to-End Integration Test

```bash
make integration
```

This uses `test_shell.sh` to drive `task_scheduler` in non-interactive mode. You should see:

```
✅ All shell-interface tests passed!
```

---

### 4) Static Analysis

Catch common C pitfalls with **cppcheck**:

```bash
cppcheck --enable=all --inconclusive --std=c11 .
```

---

### 5) AddressSanitizer & UBSan

Build and run with memory- and undefined-behavior sanitizers:

```bash
make sanitize
./sanitize_bin < test_commands.txt
```

Any memory leaks or UB will be reported by ASan/UBSan on stdout/stderr.

---

### 6) ThreadSanitizer

Detect data races in the scheduler:

```bash
make race
./race_bin < test_commands.txt
```

---

### 7) Valgrind Leak Check (Optional)

```bash
valgrind --leak-check=full ./task_scheduler < test_commands.txt
```

---

### 8) Clean Up

Remove all build artifacts:

```bash
make clean
```


### Example Workflow

```bash
# Build everything
make

# Run unit tests
make test

# Run end-to-end integration
make integration

# Build and run with AddressSanitizer / UBSan
make sanitize
./sanitize_bin < test_commands.txt

# Build and run with ThreadSanitizer
make race
./race_bin < test_commands.txt

# Clean up
make clean
```

---

## Examples

```bash
# Add two tasks and a dependency, then run with 2 workers
printf "%s\n" \
  'add_task A "echo Hello" 0 0' \
  'add_task B "echo World" 0 0' \
  'add_dep A B' \
  'run 2' \
  'exit' \
| ./task_scheduler
```

---

## Contributing

1. Fork the repository.
2. Create a feature branch: `git checkout -b feat/awesome`.
3. Write your code & tests.
4. Commit & push: `git push origin feat/awesome`.
5. Open a Pull Request and ensure CI passes.

---

## License

This project is licensed under the [https://github.com/AtluriNikhil/GraphTasker/blob/main/LICENSE](LICENSE).
Feel free to use, modify, and distribute under its terms.