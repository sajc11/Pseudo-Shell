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

// Debug macros
#ifdef DEBUG
    #define DEBUG_PRINT(msg) debug_print(msg)
    #define DEBUG_PRINTF(...) { \
        char debug_buf[1024]; \
        snprintf(debug_buf, sizeof(debug_buf), __VA_ARGS__); \
        debug_print(debug_buf); \
    }
#else
    #define DEBUG_PRINT(msg)
    #define DEBUG_PRINTF(...)
#endif

// Error message (printed on any error)
#define ERROR_MSG "An error has occurred\n"

// Maximum command line length and maximum arguments per command
#define MAX_LINE 1024
#define MAX_ARGS 128

// Size of the circular command history
#define HISTORY_SIZE 10

// Global variables for the shell search path (defined in main.c).
extern char **g_path;
extern int g_path_count;

// ------------------------
// Advanced Parser Data Structures
// ------------------------

// Represents a single command (one pipeline segment)
typedef struct Command {
    char **tokens;      // Array of token strings
    int token_count;    // Number of tokens
    int background;     // 1 if command should run in background
    char *input_file;   // Filename for input redirection, if any
    char *output_file;  // Filename for output redirection, if any
} Command;

// Represents a list of commands (separated by semicolons or pipelines)
typedef struct CommandList {
    Command **commands; // Array of Command pointers
    int count;          // Number of commands
} CommandList;

// ------------------------
// Function Prototypes
// ------------------------

// Utils
void print_error();
void debug_print(const char *msg);

// History management
void add_history(const char *line);
void print_history();
char *get_history_command(int num);
void free_history_entries();

// Simple parsing
char **parse_line(char *line, int *background, char **input_file, char **output_file, int *pipe_count);

// Built-in command processing
int is_builtin(char **args);
void execute_builtin(char **args);

// External command execution (including redirection and pipes)
char *search_executable(char *command);
void execute_external(char **args, int background, char *input_file, char *output_file);
void execute_pipeline(Command **commands, int num_cmds, int background);

// Process a single command line (dispatch built-in vs. external commands)
void process_line(char *line);

// Advanced parsing
CommandList *parse_line_advanced(char *line);
void free_command_list(CommandList *cmd_list);

#endif // SHELL_H
