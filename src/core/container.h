#pragma once

#include <common/spinlock.h>
#include <core/proc.h>
#include <core/sched.h>

#define NPID 64
struct pid_mapping {
    bool valid;
    int pid_local;
    struct proc *p;
};
typedef struct pid_mapping pid_mapping;

struct container {
    struct proc proc;
    struct scheduler scheduler;
    struct pid_mapping pm[NPID];
};

typedef struct container container;

extern struct container *root_container;

void spawn_init_container();

int alloc_resource();

void trace_usage();
