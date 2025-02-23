#include "shell.h"

// search_executable() looks for the command in the directories specified by g_path.
char *search_executable(char *command) {
    // If command contains a '/', treat it as a path.
    if (strchr(command, '/')) {
        if (access(command, X_OK) == 0) {
            return strdup(command);
        } else {
            return NULL;
        }
    }
    // Otherwise, search each directory in g_path.
    for (int i = 0; i < g_path_count; i++) {
        int len = strlen(g_path[i]) + strlen(command) + 2;
        char *full_path = malloc(len);
        if (!full_path) {
            print_error();
            exit(1);
        }
        snprintf(full_path, len, "%s/%s", g_path[i], command);
        if (access(full_path, X_OK) == 0) {
            return full_path;
        }
        free(full_path);
    }
    return NULL;
}

// execute_external() forks a child to run an external command,
// setting up input/output redirection if needed.
void execute_external(char **args, int background, char *input_file, char *output_file) {
    char *exec_path = search_executable(args[0]);
    if (exec_path == NULL) {
        print_error();
        return;
    }
    pid_t pid = fork();
    if (pid < 0) {
        print_error();
        free(exec_path);
        return;
    } else if (pid == 0) {  // Child process
        // Set up input redirection, if specified.
        if (input_file) {
            int fd_in = open(input_file, O_RDONLY);
            if (fd_in < 0) {
                print_error();
                exit(1);
            }
            if (dup2(fd_in, STDIN_FILENO) < 0) {
                print_error();
                exit(1);
            }
            close(fd_in);
        }
        // Set up output redirection, if specified.
        if (output_file) {
            int fd_out = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0666);
            if (fd_out < 0) {
                print_error();
                exit(1);
            }
            if (dup2(fd_out, STDOUT_FILENO) < 0) {
                print_error();
                exit(1);
            }
            close(fd_out);
        }
        // Execute the command.
        if (execve(exec_path, args, NULL) == -1) {
            print_error();
        }
        exit(1);
    } else {  // Parent process
        if (!background) {
            waitpid(pid, NULL, 0);
        }
    }
    free(exec_path);
}

// execute_pipeline() handles commands connected with the pipe operator.
void execute_pipeline(char ***commands, int num_cmds, int background) {
    int in_fd = 0;
    int pipe_fd[2];
    pid_t *pids = malloc(sizeof(pid_t) * num_cmds);
    if (!pids) {
        print_error();
        return;
    }
    for (int i = 0; i < num_cmds; i++) {
        if (i < num_cmds - 1) {
            if (pipe(pipe_fd) < 0) {
                print_error();
                free(pids);
                return;
            }
        }
        pid_t pid = fork();
        if (pid < 0) {
            print_error();
            free(pids);
            return;
        } else if (pid == 0) { // Child process
            if (in_fd != 0) {
                if (dup2(in_fd, STDIN_FILENO) < 0) {
                    print_error();
                    exit(1);
                }
                close(in_fd);
            }
            if (i < num_cmds - 1) {
                close(pipe_fd[0]);
                if (dup2(pipe_fd[1], STDOUT_FILENO) < 0) {
                    print_error();
                    exit(1);
                }
                close(pipe_fd[1]);
            }
            char *exec_path = search_executable(commands[i][0]);
            if (exec_path == NULL) {
                print_error();
                exit(1);
            }
            if (execve(exec_path, commands[i], NULL) < 0) {
                print_error();
                exit(1);
            }
        } else {
            pids[i] = pid;
            if (in_fd != 0) {
                close(in_fd);
            }
            if (i < num_cmds - 1) {
                close(pipe_fd[1]);
                in_fd = pipe_fd[0];
            }
        }
    }
    if (!background) {
        for (int i = 0; i < num_cmds; i++) {
            waitpid(pids[i], NULL, 0);
        }
    }
    free(pids);
}
