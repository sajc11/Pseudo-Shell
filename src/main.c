#include "shell.h"

char **g_path = NULL;
int g_path_count = 0;

int main(int argc, char *argv[]) {
    FILE *input = stdin;
    int interactive = 1;
    
    // Initialize default path to /bin
    g_path_count = 1;
    g_path = malloc(sizeof(char*) * 1);
    if (!g_path) {
        print_error();
        exit(1);
    }
    g_path[0] = strdup("/bin");
    
    // If a batch file is provided, run in batch mode (no prompt)
    if (argc == 2) {
        interactive = 0;
        input = fopen(argv[1], "r");
        if (!input) {
            print_error();
            exit(1);
        }
    } else if (argc > 2) {
        print_error();
        exit(1);
    }
    
    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    
    while (1) {
        if (interactive) {
            printf("gush> ");
        }
        read = getline(&line, &len, input);
        if (read == -1) {
            break;
        }
        if (line[read - 1] == '\n') {
            line[read - 1] = '\0';
        }
        if (line[0] == '\0') continue;
        add_history(line);
        process_line(line);
    }
    
    free(line);
    // Use the encapsulated history cleanup function.
    free_history_entries();
    // Free search path memory
    for (int i = 0; i < g_path_count; i++) {
        free(g_path[i]);
    }
    free(g_path);
    if (!interactive) {
        fclose(input);
    }
    return 0;
}

// process_line() dispatches a single input line by parsing it and then
// either calling the built-in or external command routines (including pipelines).
void process_line(char *line) {
    int background;
    char *input_file, *output_file;
    int pipe_count;
    char **tokens = parse_line(line, &background, &input_file, &output_file, &pipe_count);
    
    if (tokens[0] == NULL) {
        free(tokens);
        return;
    }
    
    if (pipe_count == 0) {
        if (is_builtin(tokens)) {
            execute_builtin(tokens);
        } else {
            execute_external(tokens, background, input_file, output_file);
        }
    } else {
        // Split tokens into separate commands for each pipeline segment.
        int num_cmds = pipe_count + 1;
        char ***commands = malloc(sizeof(char**) * num_cmds);
        if (!commands) {
            print_error();
            free(tokens);
            return;
        }
        int cmd_index = 0;
        commands[cmd_index] = &tokens[0];
        for (int i = 0; tokens[i] != NULL; i++) {
            if (strcmp(tokens[i], "|") == 0) {
                tokens[i] = NULL;
                cmd_index++;
                commands[cmd_index] = &tokens[i+1];
            }
        }
        execute_pipeline(commands, num_cmds, background);
        free(commands);
    }
    free(tokens);
}
