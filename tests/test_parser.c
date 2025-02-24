// tests/test_parser_advanced.c
#include "shell.h"
#include <stdio.h>
#include <stdlib.h>

int main() {
    const char *inputs[] = {
        "ls -l",
        "grep \"Joy\" message.txt",
        "cat < message.txt | grep Joy",
        "cd /usr/local",
        "exit",
        "ls -l > output.txt",
        "ps aux | grep sbin | wc -l",
        "ls -l | grep Joy > out.txt",
        NULL
    };

    for (int i = 0; inputs[i] != NULL; i++) {
        // Copy input to a mutable string since parse_line_advanced modifies it.
        char line[MAX_LINE];
        snprintf(line, sizeof(line), "%s", inputs[i]);

        // Parse the line using the advanced parser.
        CommandList *cmdList = parse_line_advanced(line);
        if (!cmdList) {
            fprintf(stderr, "Parsing failed for input: %s\n", inputs[i]);
            continue;
        }

        printf("Input: %s\n", inputs[i]);
        printf("Total Commands: %d\n", cmdList->count);
        for (int j = 0; j < cmdList->count; j++) {
            Command *cmd = cmdList->commands[j];
            printf("Command %d:\n", j + 1);
            printf("  Background: %s\n", cmd->background ? "yes" : "no");
            if (cmd->input_file)
                printf("  Input redirection file: %s\n", cmd->input_file);
            if (cmd->output_file)
                printf("  Output redirection file: %s\n", cmd->output_file);
            printf("  Tokens (%d):\n", cmd->token_count);
            for (int k = 0; k < cmd->token_count; k++) {
                printf("    token[%d]: %s\n", k, cmd->tokens[k]);
            }
        }
        printf("\n");
        free_command_list(cmdList);
    }

    return 0;
}
