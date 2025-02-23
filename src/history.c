#include "shell.h"

// Global history array and counters (private to this module)
static char *history[HISTORY_SIZE] = {0};
static int history_count = 0;
static int history_index = 0;

void add_history(const char *line) {
    if (!line) return;
    // Duplicate the command string for storage
    char *cmd_copy = strdup(line);
    if (!cmd_copy) {
        print_error();
        return;
    }
    // Free the oldest entry if needed (circular buffer)
    if (history[history_index]) {
        free(history[history_index]);
    }
    history[history_index] = cmd_copy;
    history_index = (history_index + 1) % HISTORY_SIZE;
    history_count++;
}

void print_history() {
    int count = history_count < HISTORY_SIZE ? history_count : HISTORY_SIZE;
    int start = (history_count < HISTORY_SIZE) ? 0 : history_index;
    for (int i = 0; i < count; i++) {
        int index = (start + i) % HISTORY_SIZE;
        printf("%d %s\n", history_count - count + i + 1, history[index]);
    }
}

char *get_history_command(int num) {
    int count = history_count < HISTORY_SIZE ? history_count : HISTORY_SIZE;
    if (num <= 0 || num > history_count ||
        (history_count > HISTORY_SIZE && num <= history_count - HISTORY_SIZE)) {
        return NULL;
    }
    int index;
    if (history_count < HISTORY_SIZE) {
        index = num - 1;
    } else {
        index = (history_index + (num - (history_count - HISTORY_SIZE) - 1)) % HISTORY_SIZE;
    }
    return history[index];
}

// Cleanup function to free all history entries.
void free_history_entries() {
    for (int i = 0; i < HISTORY_SIZE; i++) {
        if (history[i]) {
            free(history[i]);
            history[i] = NULL;
        }
    }
}
