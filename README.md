# README.txt

## Project Title: GUSH (Georgetown University Shell)

**Author**: Alivia Castor  
**Net ID**: soc11  
**Date**: 02/24/2025

---

## 1. Overview

GUSH is a simple Unix-like shell implemented in C. It supports both interactive and batch modes and meets the A-level project requirements. The shell manages processes using fork/execve/wait/waitpid, supports built-in commands, handles input/output redirection, piping (including multiple pipes), background processes, and maintains a circular command history. All errors are reported with the message:  
```
An error has occurred
```
as specified.

---

## 2. Meeting the A-Level Requirements

### A. General Implementation

1. **Interactive Mode**  
   - Displays the prompt `gush> ` and waits for user commands.  
   - Each command is parsed, executed, and waited upon (unless backgrounded).  
   - Built-in commands are handled internally; external commands are executed via fork/execve.

2. **Batch Mode**  
   - When run with a file argument (e.g., `./gush twoDir.txt` or, if in the tests directory, `../gush twoDir.txt`), the shell reads commands from that file without printing a prompt and exits upon reaching EOF.  
   - Errors in batch mode print the standard error message without terminating the shell (unless fatal).

3. **Built-in Commands**  
   - **exit**: Exits the shell (must have no arguments).  
   - **cd**: Changes directory if exactly one argument is provided; otherwise, an error is printed.  
   - **path**: Sets or clears the shell’s search path.  
   - **pwd**: Prints the current working directory.  
   - **history**: Lists the last 10 commands (excluding the `history` command itself).  
   - **kill**: Sends SIGTERM to the specified process ID.  
   - **!n**: Recalls and re-executes the n-th command from history.

4. **Error Handling**  
   - For any shell-related error (invalid command, too many arguments, file issues, etc.), the shell prints the standardized error message:
     ```
     An error has occurred
     ```
   - The shell then continues processing subsequent commands.

---

### B. Processing Commands with Two or More Pipes

- **Pipes**:  
  - The shell supports both single and multiple pipes. For example:
    ```
    ls -l | wc -l
    ps aux | grep sbin | wc -l
    ```
  - Each stage of a pipeline is forked, with the appropriate pipe ends dup’d to STDIN or STDOUT.  
  - The shell waits for all processes in the pipeline (unless backgrounded).

---

### C. Redirecting Standard In

- **Input Redirection (`<`)**:  
  - For example:
    ```
    nl < message.txt
    ```
  - The shell opens the specified file, dup2’s it to STDIN, and then executes the command.  
  - If the file does not exist or cannot be opened, the shell prints the error message.

---

### D. Processing Multiple Background Processes

- **Background Execution (`&`)**:  
  - Commands appended with `&` run in the background (e.g., `./wasteTime &`).  
  - The shell immediately returns to the prompt after launching background processes.  
  - Multiple background commands on one line are supported (e.g., `./wasteTime & ./wasteTime &`), with the shell printing a simple job notification (e.g., `[1] PID`) for the last process.  
  - The built-in `kill` command can be used to terminate these processes by PID.

---

## 3. Additional Features

1. **History Functionality**  
   - A circular buffer stores the last 10 commands.  
   - The `history` command prints these commands, and `!n` re-executes one.  
   - The command `history` itself is not stored in the history list, matching the specification.

2. **Redirecting Standard Out (`>`)**  
   - For example: `ls > out.txt` writes the output to a file, creating or truncating it as needed.

3. **Combined Redirection and Pipes**  
   - Supports commands such as:
     ```
     cat < file.txt | grep pattern > results.txt
     ```
   - This is handled by the pipeline code, which sets up both pipe and redirection.

4. **Search Path Handling**  
   - The shell uses a search path (initially set to `/bin`, `/usr/bin`, etc.) that can be updated via the `path` command.  
   - If the path is empty, only absolute or relative commands (or built-ins) will run.

5. **Robust Error Handling**  
   - All errors print the same standardized message, ensuring consistency.

---

## 4. Building and Running

### 4.1 Compile

From the project root, run:
```bash
make
```
This compiles the source files (located in `src/`) into object files in the `obj/` directory and produces the executable **`gush`**.

---

### 4.2 Interactive Mode

Run:
```bash
./gush
```
You will see the prompt `gush>`. Enter commands one by one. Type `exit` (with no arguments) to quit.

---

### 4.3 Batch Mode

To run the shell in batch mode, provide a file argument containing commands:
- If the batch file (e.g., `twoDir.txt`) and the executable are in the same directory, run:
  ```bash
  ./gush twoDir.txt
  ```
- If your batch file is in the `tests/` directory and the executable is in the project root, navigate to the `tests/` directory and run:
  ```bash
  cd tests
  ../gush twoDir.txt
  ```
This instructs the shell to read commands from **twoDir.txt** without printing a prompt and to exit when it reaches EOF.

---

### 4.4 Debug Mode

To see detailed debug messages during compilation and execution, use the debug option:
```bash
make clean && make debug
```
This rebuilds the project with the `DEBUG` flag enabled, causing the shell to print detailed debug messages that help trace the flow of execution and diagnose issues.

---

### 4.5 Test Script (`run_tests.sh`)

1. **Location**: The script is located in the `tests/` directory.  
2. **Usage**:
   ```bash
   cd tests
   chmod +x run_tests.sh   # Make the script executable if not already
   ./run_tests.sh
   ```
   The script:
   - Pipes various commands into `gush` and redirects outputs to files (e.g., `output_pwd.txt`, `output_history.txt`).
   - Runs background commands (using `wasteTime`) to test parallel process handling.
   - Runs batch mode using `twoDir.txt` and saves output to `output_batch.txt`.
3. **Review**:  
   After execution, inspect the output files to confirm that all features function as expected.

---

### 4.6 Parser Test (`test_parser`)

1. **Compile the Parser Test**:
   ```bash
   make test_parser
   ```
   This creates the executable **`test_parser`**.
2. **Run the Parser Test**:
   ```bash
   ./test_parser
   ```
   The test program calls `parse_line_advanced()` on sample inputs (such as `ls -l`, `cat < message.txt | grep Joy`, etc.) and prints the tokenized output, including background flags and redirection information.
3. **Validation**:  
   Confirm that the output correctly reflects the intended parsing behavior.
4. **Cleaning**:
    Use the `make clean` command to remove both the main executable (`gush`) and the `test_parser` executable, along with all object files in the `obj/` directory.

---

## 5. Testing

1. **Manual Testing**  
   - Execute built-in commands (exit, cd, path, pwd, history, kill, !n) and external commands interactively.  
   - Test redirection, pipes, and background processing, ensuring each behaves as expected.

2. **Scripted Testing**  
   - Run `run_tests.sh` to automatically verify features.  
   - Examine the output files (`output_pipe.txt`, `output_history.txt`, etc.) for correctness.

3. **Parser Testing**  
   - Run the parser test (`test_parser`) to confirm that command tokenization and detection of redirection, pipes, and background operators are correct.

4. **wasteTime Program**  
   - The `wasteTime` utility simulates a CPU-bound process (running for about two minutes) and outputs "wasteTime completed" upon finishing.  
   - Running it in the background (with `&`) demonstrates that the shell correctly handles multiple parallel processes.

---

## 6. Directory Structure

```
Pseudo-Shell/
├── documents/
│   ├── guShell_Example_Tests.pdf
│   ├── pseudo_shell.pdf
│   └── screenshots/                # organized validation screenshots
│       ├── Background-Processing/  # includes prallel background processes
│       ├── Batch-Mode/             # batch mode output - `testDir/` files
│       ├── Commands/               # gush commands - basic and external commands
│       ├── Error-Handling/         # error handling output for multiple error cases
│       ├── Make-Commands/          # make commands - clean, debug, test_parser, general
│       ├── Pipes/                  # single, multiple, and redirected pipes
│       ├── Redirection/            # input, output, and combined redirection
│       └── Run-Tests/              # run_tests.sh output - `output_*.txt` files
├── src/
│   ├── builtins.c
│   ├── exec.c
│   ├── history.c
│   ├── main.c
│   ├── parser.c
│   ├── shell.h
│   └── utils.c
├── tests/
│   ├── run_tests.sh
│   ├── test_parser.c
│   ├── wasteTime.c
│   ├── twoDir.txt
│   ├── message.txt
│   ├── output_batch.txt
│   ├── output_cd.txt
│   ├── output_history.txt
│   ├── output_pipe.txt
│   ├── output_pwd.txt
│   └── testDir/
│       ├── A/
│       │   ├── file_A.txt
│       │   ├── file_B.txt
│       │   └── file_C.txt
│       ├── B/
│       │   └── file_B.txt
│       └── C/
│           └── file_C.txt
├── obj/                       # Compiled object files
│   ├── builtins.o
│   ├── exec.o
│   ├── history.o
│   ├── main.o
│   ├── parser.o
│   └── utils.o
├── Makefile
├── gush                     # Final executable (after make)
├── test_parser              # Parser test executable (after make test_parser)
└── README.txt               # This file
```

---

## 7. Conclusion

My implementation of the Pseudo-Shell, GUSH, meets all the A-level project requirements by:

- Supporting interactive and batch modes,
- Managing processes using fork/execve/wait/waitpid,
- Implementing all required built-in commands,
- Handling input and output redirection,
- Processing commands with multiple pipes (including two or more),
- Managing multiple background processes,
- And providing robust error handling with a single standardized error message.

The project has been thoroughly tested using both manual tests and automated scripts (`run_tests.sh` and `test_parser`), with validation screenshots provided in the documents/screenshots directory.

**Thank you for reviewing this project!**  
If you have any questions or suggestions, please feel free to reach out.

--- 
