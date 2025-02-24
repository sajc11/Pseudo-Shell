#include "shell.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>

// Maximum tokens per command segment
#define MAX_TOKENS 128

/* Helper: Remove surrounding quotes and handle backslash escapes.
 * This function creates a new string where:
 *   - If a token starts and ends with matching quotes (either " or '),
 *     those quotes are removed.
 *   - Backslash escapes are handled in a simple way.
 */
static char *process_token(const char *token) {
    size_t len = strlen(token);
    char *result = malloc(len + 1);
    if (!result) {
        print_error();
        exit(1);
    }
    int ri = 0;
    int in_quote = 0;
    char quote_char = '\0';
    for (size_t i = 0; i < len; i++) {
        char c = token[i];
        // If not inside a quote and we see a quote, begin quoted mode.
        if (!in_quote && (c == '"' || c == '\'')) {
            in_quote = 1;
            quote_char = c;
            continue;
        }
        // If inside a quote and we see the matching end-quote, exit quoted mode.
        if (in_quote && c == quote_char) {
            in_quote = 0;
            continue;
        }
        // Handle backslash escapes (very basic)
        if (c == '\\' && i + 1 < len) {
            i++;
            c = token[i];
        }
        result[ri++] = c;
    }
    result[ri] = '\0';
    return result;
}

/* Helper: Expand environment variables.
 * - If a token starts with '$', replace it with its environment variable value.
 * - This simple implementation handles tokens that begin with '$' entirely.
 */
static char *expand_env(const char *token) {
    if (token[0] != '$') {
        return strdup(token);
    }
    // Extract variable name (alphanumeric and underscore)
    const char *p = token + 1;
    char varname[256];
    int vi = 0;
    while (*p && (isalnum(*p) || *p == '_') && vi < 255) {
        varname[vi++] = *p;
        p++;
    }
    varname[vi] = '\0';
    char *value = getenv(varname);
    if (!value)
        value = "";
    return strdup(value);
}

/* Helper: Perform a simple command substitution.
 * - Looks for the first occurrence of "$(" and a matching ")".
 * - Executes the inner command using popen, reads its output, and replaces that portion.
 * 
 * Note: This implementation supports only one substitution per token.
 */

static char *command_substitute(const char *token) {
    char *start = strstr(token, "$(");
    if (!start)
        return strdup(token);
    char *end = strchr(start, ')');
    if (!end)
        return strdup(token); // No matching ')' found.
    
    size_t prefix_len = start - token;
    size_t cmd_len = end - start - 2; // Exclude "$(" and ")"
    char *cmd = malloc(cmd_len + 1);
    if (!cmd) {
        print_error();
        exit(1);
    }
    strncpy(cmd, start + 2, cmd_len);
    cmd[cmd_len] = '\0';
    
    FILE *fp = popen(cmd, "r");
    free(cmd);
    if (!fp)
        return strdup(token);
    
    char output[1024] = {0};
    size_t out_len = fread(output, 1, sizeof(output) - 1, fp);
    output[out_len] = '\0';
    pclose(fp);
    // Remove trailing newline if present.
    if (out_len > 0 && output[out_len - 1] == '\n')
        output[out_len - 1] = '\0';
    
    size_t suffix_len = strlen(end + 1);
    size_t new_len = prefix_len + strlen(output) + suffix_len;
    char *new_token = malloc(new_len + 1);
    if (!new_token) {
        print_error();
        exit(1);
    }
    strncpy(new_token, token, prefix_len);
    new_token[prefix_len] = '\0';
    strcat(new_token, output);
    strcat(new_token, end + 1);
    return new_token;
}

/* Advanced parser: parse_line_advanced()
 * Implements:
 * - Splitting by semicolons (multiple commands)
 * - Splitting each command by pipe (pipeline segments)
 * - Tokenizing each command segment with advanced quote/escape handling
 * - Environment variable expansion and command substitution on tokens
 * - Detection of background operator (&), input redirection (<), and output redirection (>)
 */
CommandList *parse_line_advanced(char *line) {
    CommandList *cmd_list = malloc(sizeof(CommandList));
    if (!cmd_list) {
        print_error();
        exit(1);
    }
    cmd_list->commands = NULL;
    cmd_list->count = 0;
    
    // Split input by semicolons for multiple commands.
    char *cmd_str = strtok(line, ";");
    while (cmd_str) {
        // Trim leading whitespace.
        while (isspace(*cmd_str)) cmd_str++;
        if (*cmd_str == '\0') {
            cmd_str = strtok(NULL, ";");
            continue;
        }
        
        // Check for background operator at end (e.g., "ls &")
        int background = 0;
        size_t len = strlen(cmd_str);
        if (len > 0 && cmd_str[len - 1] == '&') {
            background = 1;
            cmd_str[len - 1] = '\0';  // Remove the '&'
        }
        
        // Split the command by pipe '|' to create pipeline segments.
        char **pipe_segments = malloc(sizeof(char*) * MAX_TOKENS);
        if (!pipe_segments) {
            print_error();
            exit(1);
        }
        int seg_count = 0;
        char *segment = strtok(cmd_str, "|");
        while (segment && seg_count < MAX_TOKENS) {
            // Trim leading whitespace from each segment.
            while (isspace(*segment)) segment++;
            pipe_segments[seg_count++] = segment;
            segment = strtok(NULL, "|");
        }
        
        // For each pipeline segment, tokenize into arguments.
        for (int s = 0; s < seg_count; s++) {
            Command *cmd = malloc(sizeof(Command));
            if (!cmd) {
                print_error();
                exit(1);
            }
            cmd->background = background;
            cmd->input_file = NULL;
            cmd->output_file = NULL;
            cmd->tokens = malloc(sizeof(char*) * MAX_TOKENS);
            if (!cmd->tokens) {
                print_error();
                exit(1);
            }
            cmd->token_count = 0;
            
            // Tokenize the segment by whitespace.
            char *raw_token = strtok(pipe_segments[s], " \t\r\n");
            while (raw_token && cmd->token_count < MAX_TOKENS - 1) {
                // Process the token: strip quotes and handle escapes.
                char *proc = process_token(raw_token);
                // Expand environment variables if token begins with '$'.
                if (proc[0] == '$') {
                    char *expanded = expand_env(proc);
                    free(proc);
                    proc = expanded;
                }
                // Perform command substitution if token contains "$(".
                if (strstr(proc, "$(")) {
                    char *substituted = command_substitute(proc);
                    free(proc);
                    proc = substituted;
                }
                
                // Check for redirection operators.
                if (strcmp(proc, "<") == 0) {
                    raw_token = strtok(NULL, " \t\r\n");
                    if (!raw_token) {
                        print_error();
                        break;
                    }
                    char *in_tok = process_token(raw_token);
                    cmd->input_file = in_tok;
                    free(proc);
                } else if (strcmp(proc, ">") == 0) {
                    raw_token = strtok(NULL, " \t\r\n");
                    if (!raw_token) {
                        print_error();
                        break;
                    }
                    char *out_tok = process_token(raw_token);
                    cmd->output_file = out_tok;
                    free(proc);
                } else if (strcmp(proc, "&") == 0) {
                    // If found within a pipeline segment, mark as background.
                    cmd->background = 1;
                    free(proc);
                } else {
                    // Regular token: store it.
                    cmd->tokens[cmd->token_count++] = proc;
                }
                raw_token = strtok(NULL, " \t\r\n");
            }
            cmd->tokens[cmd->token_count] = NULL;
            // Add this command to the CommandList.
            cmd_list->count++;
            cmd_list->commands = realloc(cmd_list->commands, sizeof(Command*) * cmd_list->count);
            if (!cmd_list->commands) {
                print_error();
                exit(1);
            }
            cmd_list->commands[cmd_list->count - 1] = cmd;
        }
        free(pipe_segments);
        cmd_str = strtok(NULL, ";");
    }
    return cmd_list;
}

// Helper to free a Command structure.
void free_command(Command *cmd) {
    if (!cmd) return;
    if (cmd->tokens) {
        for (int i = 0; i < cmd->token_count; i++) {
            if (cmd->tokens[i]) {
                free(cmd->tokens[i]);
            }
        }
        free(cmd->tokens);
    }
    if (cmd->input_file)
        free(cmd->input_file);
    if (cmd->output_file)
        free(cmd->output_file);
    free(cmd);
}

// Free an entire CommandList.
void free_command_list(CommandList *cmd_list) {
    if (!cmd_list) return;
    for (int i = 0; i < cmd_list->count; i++) {
        free_command(cmd_list->commands[i]);
    }
    free(cmd_list->commands);
    free(cmd_list);
}

// Basic parser implementation
char **parse_line(char *line, int *background, char **input_file, char **output_file, int *pipe_count) {
    DEBUG_PRINTF("\nParsing line: %s\n", line);

    char **tokens = malloc(sizeof(char*) * MAX_ARGS);
    if (!tokens) {
        DEBUG_PRINT("Token allocation failed\n");
        print_error();
        exit(1);
    }

    // Initialize flags
    *background = 0;
    *input_file = NULL;
    *output_file = NULL;
    *pipe_count = 0;

    int pos = 0;
    char *saveptr;
    char *token = strtok_r(line, " \t\r\n", &saveptr);
    
    // Tokenize the input
    while (token != NULL && pos < MAX_ARGS - 1) {
        DEBUG_PRINTF("Found token: %s\n", token);
        tokens[pos++] = token;
        token = strtok_r(NULL, " \t\r\n", &saveptr);
    }
    tokens[pos] = NULL;

    DEBUG_PRINTF("Total tokens found: %d\n", pos);

    // Scan tokens for special symbols
    for (int i = 0; i < pos; i++) {
        if (tokens[i] == NULL) continue;

        if (strcmp(tokens[i], "&") == 0) {
            DEBUG_PRINT("Found background operator\n");
            *background = 1;
            tokens[i] = NULL;
        } else if (strcmp(tokens[i], "<") == 0) {
            DEBUG_PRINT("Found input redirection\n");
            if (i + 1 < pos && tokens[i+1] != NULL) {
                *input_file = tokens[i+1];
                tokens[i] = NULL;
                tokens[i+1] = NULL;
                i++;
            } else {
                DEBUG_PRINT("Invalid input redirection\n");
                print_error();
                break;
            }
        } else if (strcmp(tokens[i], ">") == 0) {
            DEBUG_PRINT("Found output redirection\n");
            if (i + 1 < pos && tokens[i+1] != NULL) {
                *output_file = tokens[i+1];
                tokens[i] = NULL;
                tokens[i+1] = NULL;
                i++;
            } else {
                DEBUG_PRINT("Invalid output redirection\n");
                print_error();
                break;
            }
        } else if (strcmp(tokens[i], "|") == 0) {
            DEBUG_PRINT("Found pipe operator\n");
            (*pipe_count)++;
        }
    }

    DEBUG_PRINTF("Parsing complete. Background: %s\n", *background ? "yes" : "no");
    return tokens;
}