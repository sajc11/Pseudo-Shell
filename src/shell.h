#ifndef SHELL_H
#define SHELL_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>
#include <ctype.h>

// Error message (printed on any error)
#define ERROR_MSG "An error has occurred\n"

// Maximum command line length and maximum arguments per command
#define MAX_LINE 1024
#define MAX_ARGS 128

// Size of the circular command history
#define HISTORY_SIZE 10

// Global variables for the shell search path.
// (These are defined in main.c.)
extern char **g_path;
extern int g_path_count;

// ------------------------
// Function Prototypes
// ------------------------

// Utils
void print_error();

// History management
void add_history(const char *line);
void print_history();
char *get_history_command(int num);
void free_history_entries();  // New: cleanup function for history entries

// Parsing: tokenizes a line and extracts flags for background, redirection, and pipes.
char **parse_line(char *line, int *background, char **input_file, char **output_file, int *pipe_count);

// Built-in command processing
int is_builtin(char **args);
void execute_builtin(char **args);

// External command execution (including redirection and pipes)
char *search_executable(char *command);
void execute_external(char **args, int background, char *input_file, char *output_file);
void execute_pipeline(char ***commands, int num_cmds, int background);

// Process a single command line (dispatches built-in vs. external and handles pipelines)
void process_line(char *line);

#endif // SHELL_H
