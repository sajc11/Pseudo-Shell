#include "shell.h"

char *search_executable(char *command) {
    if (!command) {
        DEBUG_PRINT("search_executable: null command\n");
        return NULL;
    }
    
    DEBUG_PRINTF("Searching for executable: %s\n", command);

    // Special debug for grep
    if (strcmp(command, "grep") == 0) {
        DEBUG_PRINT("Searching for grep specifically\n");
        // Check common grep locations
        const char *grep_paths[] = {
            "/usr/bin/grep",
            "/bin/grep"
        };
        int num_paths = sizeof(grep_paths) / sizeof(grep_paths[0]);
        
        for (int i = 0; i < num_paths; i++) {
            DEBUG_PRINTF("Trying grep at: %s\n", grep_paths[i]);
            if (access(grep_paths[i], X_OK) == 0) {
                DEBUG_PRINTF("Found grep at: %s\n", grep_paths[i]);
                return strdup(grep_paths[i]);
            }
        }
    }

    // Try /usr/bin specifically for nl
    if (strcmp(command, "nl") == 0) {
        DEBUG_PRINT("Checking /usr/bin for nl command\n");
        if (access("/usr/bin/nl", X_OK) == 0) {
            DEBUG_PRINT("Found nl in /usr/bin\n");
            return strdup("/usr/bin/nl");
        }
    }

    // If it starts with ./ or / or ../, treat as direct path
    if (command[0] == '/' || 
        (command[0] == '.' && command[1] == '/') ||
        (command[0] == '.' && command[1] == '.' && command[2] == '/')) {
        DEBUG_PRINT("Treating as direct path\n");
        if (access(command, X_OK) == 0) {
            char *result = strdup(command);
            if (result) {
                DEBUG_PRINT("Found local executable\n");
            }
            return result;
        }
        DEBUG_PRINT("Local executable not found or not executable\n");
        return NULL;
    }

    // Search in current directory first
    char current_path[1024];
    if (getcwd(current_path, sizeof(current_path)) != NULL) {
        int len = strlen(current_path) + strlen(command) + 3;
        char *full_path = malloc(len);
        if (full_path) {
            snprintf(full_path, len, "./%s", command);
            DEBUG_PRINTF("Trying current dir: %s\n", full_path);
            if (access(full_path, X_OK) == 0) {
                DEBUG_PRINT("Found in current directory\n");
                return full_path;
            }
            free(full_path);
        }
    }

    // Then search in PATH directories
    for (int i = 0; i < g_path_count; i++) {
        DEBUG_PRINTF("Checking path: %s\n", g_path[i]);
        
        int len = strlen(g_path[i]) + strlen(command) + 2;
        char *full_path = malloc(len);
        if (!full_path) {
            DEBUG_PRINT("Memory allocation failed\n");
            print_error();
            exit(1);
        }
        snprintf(full_path, len, "%s/%s", g_path[i], command);
        DEBUG_PRINTF("Trying path: %s\n", full_path);

        if (access(full_path, X_OK) == 0) {
            DEBUG_PRINT("Found executable in path\n");
            return full_path;
        }
        free(full_path);
    }
    
    DEBUG_PRINT("Executable not found in any path\n");
    return NULL;
}

void execute_external(char **args, int background, char *input_file, char *output_file) {
    DEBUG_PRINT("\nStarting execute_external\n");
    
    if (!args || !args[0]) {
        DEBUG_PRINT("Invalid args\n");
        print_error();
        return;
    }

    // Print all arguments for debugging
    DEBUG_PRINT("Command and arguments:\n");
    for (int i = 0; args[i] != NULL; i++) {
        DEBUG_PRINTF("  arg[%d]: %s\n", i, args[i]);
    }

    if (input_file) {
        DEBUG_PRINTF("Input redirection from file: %s\n", input_file);
    }
    if (output_file) {
        DEBUG_PRINTF("Output redirection to file: %s\n", output_file);
    }

    DEBUG_PRINTF("Background mode: %s\n", background ? "yes" : "no");

    char *exec_path = search_executable(args[0]);
    if (!exec_path) {
        DEBUG_PRINT("Executable not found\n");
        print_error();
        return;
    }

    DEBUG_PRINTF("Found executable at: %s\n", exec_path);

    // Setup basic environment
    char path_env[1024];
    snprintf(path_env, sizeof(path_env), "PATH=%s", g_path[0]);
    char *envp[] = {path_env, NULL};

    pid_t pid = fork();
    if (pid < 0) {
        DEBUG_PRINT("Fork failed\n");
        print_error();
        free(exec_path);
        return;
    }

    if (pid == 0) {  // Child process
        DEBUG_PRINT("Child process started\n");

        // Setup standard IO redirections
        if (input_file) {
            int fd_in = open(input_file, O_RDONLY);
            if (fd_in < 0) {
                DEBUG_PRINT("Failed to open input file\n");
                print_error();
                exit(1);
            }
            DEBUG_PRINT("Input file opened successfully\n");

            if (dup2(fd_in, STDIN_FILENO) < 0) {
                DEBUG_PRINT("Failed to redirect input\n");
                print_error();
                close(fd_in);
                exit(1);
            }
            DEBUG_PRINT("Input redirection successful\n");
            
            close(fd_in);
        }

        if (output_file) {
            int fd_out = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0666);
            if (fd_out < 0) {
                DEBUG_PRINT("Failed to open output file\n");
                print_error();
                exit(1);
            }
            if (dup2(fd_out, STDOUT_FILENO) < 0) {
                DEBUG_PRINT("Failed to redirect output\n");
                print_error();
                close(fd_out);
                exit(1);
            }
            close(fd_out);
        }

        // Set up process group for background processes
        if (background) {
            setpgid(0, 0);
        }

        DEBUG_PRINT("Executing command with execve\n");
        execve(exec_path, args, envp);
        
        // If we get here, execve failed
        DEBUG_PRINTF("execve failed, errno: %d\n", errno);
        print_error();
        free(exec_path);
        exit(1);
    }

    // Parent process
    DEBUG_PRINT("Parent process continuing\n");
    free(exec_path);

    // Set up process group for background processes
    if (background) {
        setpgid(pid, pid);
        printf("[1] %d\n", pid);
        DEBUG_PRINTF("Background process started with PID: %d\n", pid);
    } else {
        // Wait for foreground processes
        int status;
        waitpid(pid, &status, 0);
        DEBUG_PRINT("Foreground process completed\n");
    }
}
void execute_pipeline(Command **commands, int num_cmds, int background) {
    DEBUG_PRINTF("Starting pipeline execution with %d commands\n", num_cmds);
    
    // Debug print pipeline setup
    for (int i = 0; i < num_cmds; i++) {
        DEBUG_PRINTF("Command %d: %s\n", i, commands[i]->tokens[0]);
        if (commands[i]->input_file) {
            DEBUG_PRINTF("  Input file: %s\n", commands[i]->input_file);
        }
        if (commands[i]->output_file) {
            DEBUG_PRINTF("  Output file: %s\n", commands[i]->output_file);
        }
    }
    
    int pipes[2][2];  // Two sets of pipes for read/write
    pid_t *pids = malloc(sizeof(pid_t) * num_cmds);
    
    if (!pids) {
        DEBUG_PRINT("Failed to allocate memory for PIDs\n");
        print_error();
        return;
    }

    // For each command in the pipeline
    for (int i = 0; i < num_cmds; i++) {
        if (i < num_cmds - 1) {
            // Create pipe for all but the last command
            if (pipe(pipes[i % 2]) < 0) {
                DEBUG_PRINT("Pipe creation failed\n");
                print_error();
                free(pids);
                return;
            }
        }

        pids[i] = fork();
        if (pids[i] < 0) {
            DEBUG_PRINT("Fork failed\n");
            print_error();
            free(pids);
            return;
        }

        if (pids[i] == 0) {  // Child process
            // Set up input from previous pipe or input redirection
            if (i > 0) {
                DEBUG_PRINTF("Setting up pipe input for command %d\n", i);
                if (dup2(pipes[(i - 1) % 2][0], STDIN_FILENO) < 0) {
                    DEBUG_PRINT("Failed to setup pipe input\n");
                    print_error();
                    exit(1);
                }
            } else if (commands[i]->input_file) {
                // First command input redirection
                DEBUG_PRINTF("Setting up input redirection from %s\n", commands[i]->input_file);
                int fd = open(commands[i]->input_file, O_RDONLY);
                if (fd < 0) {
                    DEBUG_PRINT("Failed to open input file\n");
                    print_error();
                    exit(1);
                }
                if (dup2(fd, STDIN_FILENO) < 0) {
                    DEBUG_PRINT("Failed to redirect input\n");
                    print_error();
                    close(fd);
                    exit(1);
                }
                close(fd);
                DEBUG_PRINT("Input redirection successful\n");
            }

            // Set up output to next pipe or output redirection
            if (i < num_cmds - 1) {
                DEBUG_PRINTF("Setting up pipe output for command %d\n", i);
                if (dup2(pipes[i % 2][1], STDOUT_FILENO) < 0) {
                    DEBUG_PRINT("Failed to setup pipe output\n");
                    print_error();
                    exit(1);
                }
            } else if (commands[i]->output_file) {
                // Last command output redirection
                DEBUG_PRINTF("Setting up output redirection to %s\n", commands[i]->output_file);
                int fd = open(commands[i]->output_file, O_WRONLY|O_CREAT|O_TRUNC, 0666);
                if (fd < 0 || dup2(fd, STDOUT_FILENO) < 0) {
                    DEBUG_PRINT("Failed to setup output redirection\n");
                    print_error();
                    exit(1);
                }
                close(fd);
            }

            // Close all pipe fds in child
            for (int j = 0; j < 2; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }

            // Execute the command
            char *exec_path = search_executable(commands[i]->tokens[0]);
            if (!exec_path) {
                DEBUG_PRINT("Command not found\n");
                print_error();
                exit(1);
            }

            DEBUG_PRINTF("Executing command: %s\n", exec_path);
            execve(exec_path, commands[i]->tokens, NULL);
            free(exec_path);
            print_error();
            exit(1);
        }

        // Parent process
        // Close unused pipe ends
        if (i > 0) {
            close(pipes[(i - 1) % 2][0]);
            close(pipes[(i - 1) % 2][1]);
        }
    }

    // Wait for all processes unless in background mode
    if (!background) {
        DEBUG_PRINT("Waiting for pipeline processes\n");
        for (int i = 0; i < num_cmds; i++) {
            waitpid(pids[i], NULL, 0);
        }
        DEBUG_PRINT("All pipeline processes completed\n");
    } else {
        printf("[1] %d\n", pids[num_cmds-1]);
    }

    free(pids);
    DEBUG_PRINT("Pipeline execution completed\n");
}