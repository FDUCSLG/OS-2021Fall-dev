#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    int fd;
    if (argc <= 1) {
        fprintf(stderr, "mkdir: missing operand\n");
        return 0;
    }

    for (int i = 1; i < argc; i++) {
        if (mkdir(argv[i], 0) < 0) {
            fprintf(stderr, "mkdir: cannot mkdir %s\n", argv[i]);
            exit(0);
        }
    }
    exit(0);
}