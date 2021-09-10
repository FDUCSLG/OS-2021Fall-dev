#pragma once

#include "defines.h"
#include "cpu.h"

#define NPROC           100
#define NCPU            4
#define NOFILE          64      // Open files per sched_obj

struct scheduler;

/* Stack must always be 16 bytes aligned. */
struct context {
    uint64_t lr0, lr, fp;
    uint64_t x[10];             /* X28 ... X19 */
    uint64_t padding;
    // uint64_t q0[2];             /* V0 */
};

enum sched_obj_type {
    PROCESS,
    SCHEDULER
};

struct sched_obj {
    enum sched_obj_type type;
};

struct sched_op {
    void (*scheduler)();
};

NORETURN void default_scheduler();
struct sched_op default_op = {
    .scheduler = default_scheduler
};

struct scheduler {
    struct sched_obj sched;
    struct sched_op op;
};

struct scheduler *root_scheduler = NULL;

void sched_init();

#define scheduler()                                                                                \
    assert(thiscpu()->scheduler != NULL);                                                          \
    thiscpu()->scheduler->op.scheduler();