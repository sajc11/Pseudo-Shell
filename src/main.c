#include "shell.h"

char **g_path = NULL;
int g_path_count = 0;

static int initialize_path(void) {
    // Keep existing initialization code
    const char *default_paths[] = {
        "/bin",
        "/usr/bin",
        "/usr/local/bin",
        "/sbin",
        "/usr/sbin"
    };
    
    g_path_count = sizeof(default_paths) / sizeof(default_paths[0]);
    g_path = malloc(sizeof(char*) * g_path_count);
    if (!g_path) {
        print_error();
        return 0;
    }
    
    for (int i = 0; i < g_path_count; i++) {
        g_path[i] = strdup(default_paths[i]);
        if (!g_path[i]) {
            for (int j = 0; j < i; j++) {
                free(g_path[j]);
            }
            free(g_path);
            g_path = NULL;
            print_error();
            return 0;
        }
        DEBUG_PRINTF("Added path: %s\n", g_path[i]);
    }
    DEBUG_PRINT("Path initialized successfully\n");
    return 1;
}

static void cleanup_shell(char *line, FILE *input, int interactive) {
    DEBUG_PRINT("Starting shell cleanup\n");
    free(line);
    free_history_entries();
    for (int i = 0; i < g_path_count; i++) {
        free(g_path[i]);
    }
    free(g_path);
    if (!interactive && input != stdin) {
        fclose(input);
    }
    DEBUG_PRINT("Shell cleanup complete\n");
}

void process_line(char *line) {
    DEBUG_PRINTF("Processing line: %s\n", line);
    
    // Skip empty lines and comments
    if (!line || line[0] == '\0' || line[0] == '#') {
        return;
    }
    
    CommandList *cmdList = parse_line_advanced(line);
    if (!cmdList) {
        DEBUG_PRINT("Parsing failed\n");
        return;
    }

    // Check if this is a pipeline (multiple commands)
    if (cmdList->count > 1) {
        DEBUG_PRINTF("Processing pipeline with %d commands\n", cmdList->count);
        execute_pipeline(cmdList->commands, cmdList->count, cmdList->commands[0]->background);
    } else if (cmdList->count == 1) {
        Command *cmd = cmdList->commands[0];
        if (!cmd->tokens[0]) {
            DEBUG_PRINT("Empty command\n");
            free_command_list(cmdList);
            return;
        }
        
        // Single command processing
        if (is_builtin(cmd->tokens)) {
            DEBUG_PRINT("Executing builtin command\n");
            execute_builtin(cmd->tokens);
        } else {
            DEBUG_PRINT("Executing external command\n");
            execute_external(cmd->tokens, cmd->background, 
                           cmd->input_file, cmd->output_file);
        }
    }

    free_command_list(cmdList);
}

int main(int argc, char *argv[]) {
    FILE *input = stdin;
    int interactive = 1;
    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    
    DEBUG_PRINT("Shell starting\n");
    
    if (!initialize_path()) {
        return 1;
    }
    
    if (argc > 2) {
        DEBUG_PRINT("Too many arguments\n");
        print_error();
        return 1;
    }
    
    if (argc == 2) {
        DEBUG_PRINTF("Opening batch file: %s\n", argv[1]);
        interactive = 0;
        input = fopen(argv[1], "r");
        if (!input) {
            DEBUG_PRINT("Failed to open batch file\n");
            print_error();
            return 1;
        }
    }
    
    // Main command loop
    while (1) {
        if (interactive) {
            printf("gush> ");
            fflush(stdout);
        }
        
        read = getline(&line, &len, input);
        if (read == -1) {
            break;  // End of file or error
        }
        
        // Remove trailing newline
        if (read > 0 && line[read - 1] == '\n') {
            line[read - 1] = '\0';
            read--;
        }

        // Skip empty lines and comments
        if (read == 0 || line[0] == '#') {
            continue;
        }

        if (!interactive) {
            DEBUG_PRINTF("Batch processing line: %s\n", line);
        }

        // Check if the command is "history" (or starts with "history" and is only that command)
        // If so, do not add it to history.
        if (strncmp(line, "history", 7) != 0 || (line[7] != '\0' && !isspace(line[7]))) {
            add_history(line);
        }
        
        process_line(line);
        
        if (interactive) {
            fflush(stdout);
        }
    }
    
    cleanup_shell(line, input, interactive);
    DEBUG_PRINT("Shell exiting\n");
    return 0;
}