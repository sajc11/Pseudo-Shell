#include "shell.h"

// parse_line() tokenizes the input string and sets flags for redirection, background processing, and pipe count.
char **parse_line(char *line, int *background, char **input_file, char **output_file, int *pipe_count) {
    char **tokens = malloc(sizeof(char*) * MAX_ARGS);
    if (!tokens) {
        print_error();
        exit(1);
    }
    int pos = 0;
    char *token = strtok(line, " \t\r\n");
    while (token != NULL && pos < MAX_ARGS - 1) {
        tokens[pos++] = token;
        token = strtok(NULL, " \t\r\n");
    }
    tokens[pos] = NULL;

    // Initialize flags and redirection filenames
    *background = 0;
    *input_file = NULL;
    *output_file = NULL;
    *pipe_count = 0;

    // Scan tokens for special symbols
    for (int i = 0; i < pos; i++) {
        if (tokens[i] == NULL) continue;
        if (strcmp(tokens[i], "&") == 0) {
            *background = 1;
            tokens[i] = NULL;
        }
        if (strcmp(tokens[i], "<") == 0) {
            if (i + 1 < pos && tokens[i+1] != NULL) {
                *input_file = tokens[i+1];
                tokens[i] = NULL;
                tokens[i+1] = NULL;
                i++;
            } else {
                print_error();
                break;
            }
        }
        if (strcmp(tokens[i], ">") == 0) {
            if (i + 1 < pos && tokens[i+1] != NULL) {
                *output_file = tokens[i+1];
                tokens[i] = NULL;
                tokens[i+1] = NULL;
                i++;
            } else {
                print_error();
                break;
            }
        }
        if (strcmp(tokens[i], "|") == 0) {
            (*pipe_count)++;
        }
    }
    return tokens;
}
