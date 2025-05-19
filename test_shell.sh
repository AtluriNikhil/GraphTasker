#!/usr/bin/env bash
set -euo pipefail

# Ensure the scheduler binary exists
if [[ ! -x ./task_scheduler ]]; then
  echo "ERROR: task_scheduler binary not found. Build it first."
  exit 1
fi

# Run a sequence of commands through the shell and capture output (including stderr)
output=$(printf "%s\n" \
  'add_task A "echo A" 0 0' \
  'add_task B "echo B" 0 0' \
  'add_task A "should fail" 0 0' \
  'add_dep A B' \
  'add_dep B A' \
  'show tasks' \
  'show deps' \
  'run 1' \
  'exit' \
| ./task_scheduler 2>&1)

# Optional: print the output for inspection
echo "===== PROGRAM OUTPUT ====="
printf "%s\n" "$output"
echo "=========================="

# Verify expected behavior
grep -q "Task 'A' added\."                <<<"$output" || { echo "❌ failed to add task A"; exit 1; }
grep -q "Task 'B' added\."                <<<"$output" || { echo "❌ failed to add task B"; exit 1; }
grep -q "Task ID already exists"         <<<"$output" || { echo "❌ duplicate-task not detected"; exit 1; }
grep -q "Adding this would create a cycle" <<<"$output" || { echo "❌ cycle detection failed"; exit 1; }
grep -q "^\[0\] A: time=0 freq=0 status="  <<<"$output" || { echo "❌ show tasks missing A"; exit 1; }
grep -q "^\[1\] B: time=0 freq=0 status="  <<<"$output" || { echo "❌ show tasks missing B"; exit 1; }
grep -q "^A -> B *$"                       <<<"$output" || { echo "❌ show deps missing A -> B"; exit 1; }
grep -q "Scheduler started with 1 workers\." <<<"$output" || { echo "❌ scheduler did not start"; exit 1; }

echo "✅ All shell‐interface tests passed!"