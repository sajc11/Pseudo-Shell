#include "shell.h"
#include <signal.h>
#include <stdlib.h>

// Check if a command is built-in.
int is_builtin(char **args) {
    if (args[0] == NULL)
        return 1;  // treat empty command as built-in (no-op)
    if (strcmp(args[0], "exit") == 0 ||
        strcmp(args[0], "cd") == 0 ||
        strcmp(args[0], "path") == 0 ||
        strcmp(args[0], "pwd") == 0 ||
        strcmp(args[0], "history") == 0 ||
        strcmp(args[0], "kill") == 0)
        return 1;
    // Check for history re-execution command (e.g., !2)
    if (args[0][0] == '!' && isdigit(args[0][1]))
        return 1;
    return 0;
}

// Execute a built-in command.
void execute_builtin(char **args) {
    if (strcmp(args[0], "exit") == 0) {
        // exit must not have any arguments.
        if (args[1] != NULL) {
            print_error();
        } else {
            exit(0);
        }
    } else if (strcmp(args[0], "cd") == 0) {
        // cd takes exactly one argument (or none)
        if (args[1] == NULL || args[2] != NULL) {
            print_error();
        } else {
            if (chdir(args[1]) != 0) {
                print_error();
            }
        }
    } else if (strcmp(args[0], "path") == 0) {
        // Reset the search path.
        for (int i = 0; i < g_path_count; i++) {
            free(g_path[i]);
        }
        free(g_path);
        g_path_count = 0;
        int i = 1;
        while (args[i] != NULL) {
            g_path_count++;
            i++;
        }
        g_path = malloc(sizeof(char*) * g_path_count);
        if (!g_path && g_path_count > 0) {
            print_error();
            exit(1);
        }
        for (i = 1; i <= g_path_count; i++) {
            g_path[i-1] = strdup(args[i]);
        }
    } else if (strcmp(args[0], "pwd") == 0) {
        char cwd[1024];
        if (getcwd(cwd, sizeof(cwd)) == NULL) {
            print_error();
        } else {
            printf("%s\n", cwd);
        }
    } else if (strcmp(args[0], "history") == 0) {
        print_history();
    } else if (strcmp(args[0], "kill") == 0) {
        // kill expects exactly one argument: a PID.
        if (args[1] == NULL || args[2] != NULL) {
            print_error();
        } else {
            int pid = atoi(args[1]);
            if (pid <= 0) {
                print_error();
            } else {
                if (kill(pid, SIGTERM) != 0) {
                    print_error();
                }
            }
        }
    } else if (args[0][0] == '!' && isdigit(args[0][1])) {
        // History re-execution: !n
        int num = atoi(args[0] + 1);
        char *cmd = get_history_command(num);
        if (cmd == NULL) {
            print_error();
        } else {
            // Print the command being re-executed
            printf("%s\n", cmd);
            // Duplicate the command and process it
            char *cmd_dup = strdup(cmd);
            if (!cmd_dup) {
                print_error();
                return;
            }
            process_line(cmd_dup);
            free(cmd_dup);
        }
    }
}
