#include "shell.h"

void print_error() {
    write(STDERR_FILENO, ERROR_MSG, strlen(ERROR_MSG));
}

void debug_print(const char *msg) {
    write(STDERR_FILENO, msg, strlen(msg));
}