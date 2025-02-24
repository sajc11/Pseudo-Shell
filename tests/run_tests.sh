#!/bin/bash
# run_tests.sh - Test suite for the gush shell

echo "========== Testing Built-In Commands =========="
echo "pwd" | ../gush > output_pwd.txt
echo "cd /" | ../gush > output_cd.txt
echo -e "ls\npwd\nhistory\n!2" | ../gush > output_history.txt

echo "========== Testing Redirection & Pipes =========="
echo "ls -l | wc -l" | ../gush > output_pipe.txt

echo "========== Testing Background Process =========="
./wasteTime &
bg_pid=$!
echo "Background wasteTime PID: $bg_pid"

echo "========== Testing Batch Mode =========="
../gush twoDir.txt > output_batch.txt

echo "Tests completed. Please review the output_*.txt files for results."
