#include "shell.h"
#include <signal.h>
#include <stdlib.h>

int is_builtin(char **args) {
    if (!args || !args[0]) return 1;  // treat empty command as built-in
    
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

void execute_builtin(char **args) {
    if (!args || !args[0]) return;

    if (strcmp(args[0], "exit") == 0) {
        if (args[1] != NULL) {
            print_error();
        } else {
            exit(0);
        }
    } else if (strcmp(args[0], "cd") == 0) {
        if (args[1] == NULL || args[2] != NULL) {
            print_error();
        } else {
            if (chdir(args[1]) != 0) {
                print_error();
            }
        }
    } else if (strcmp(args[0], "path") == 0) {
        DEBUG_PRINT("Executing path command\n");
        
        // Count new paths
        int new_count = 0;
        while (args[new_count + 1] != NULL) {
            new_count++;
        }
        
        // Free old paths
        for (int i = 0; i < g_path_count; i++) {
            free(g_path[i]);
        }
        free(g_path);
        
        // Allocate and copy new paths
        g_path_count = new_count;
        if (new_count > 0) {
            g_path = malloc(sizeof(char*) * new_count);
            if (!g_path) {
                print_error();
                exit(1);
            }
            
            for (int i = 0; i < new_count; i++) {
                g_path[i] = strdup(args[i + 1]);
                if (!g_path[i]) {
                    // Cleanup on error
                    for (int j = 0; j < i; j++) {
                        free(g_path[j]);
                    }
                    free(g_path);
                    print_error();
                    exit(1);
                }
                DEBUG_PRINTF("Added path: %s\n", g_path[i]);
            }
        } else {
            g_path = NULL;
        }
        DEBUG_PRINTF("Path updated, new count: %d\n", g_path_count);
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
        int num = atoi(args[0] + 1);
        char *cmd = get_history_command(num);
        if (cmd == NULL) {
            print_error();
        } else {
            printf("%s\n", cmd);
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