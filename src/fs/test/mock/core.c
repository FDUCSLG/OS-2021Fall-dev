#include <core/sched.h>

struct cpu cpus[NCPU];

isize console_write(Inode *ip, char *buf, isize n) {
    (void)ip;
    (void)buf;
    (void)n;
    return 0;
}

isize console_read(Inode *ip, char *dst, isize n) {
    (void)ip;
    (void)dst;
    (void)n;
    return 0;
}
