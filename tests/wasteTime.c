#include <stdio.h>
#include <unistd.h>

int main() {
    // Simulate CPU-bound work with a sleep.
    sleep(2);
    printf("wasteTime completed\n");
    return 0;
}
