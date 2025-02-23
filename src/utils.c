#include "shell.h"

void print_error() {
    write(STDERR_FILENO, ERROR_MSG, strlen(ERROR_MSG));
}
