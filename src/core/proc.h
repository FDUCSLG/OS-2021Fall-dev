#pragma once
#include <stdint.h>
#include "sched.h"

#define NCPU       4    /* maximum number of CPUs */
#define NPROC      64   /* maximum number of processes */
#define NOFILE     16   /* open files per process */
#define KSTACKSIZE 4096 /* size of per-process kernel stack */

enum procstate { UNUSED, EMBRYO, SLEEPING, RUNNABLE, RUNNING, ZOMBIE };

struct proc {
    struct sched_obj sched;
    uint64_t sz;             /* Size of process memory (bytes)          */
    uint64_t *pgdir;         /* Page table                              */
    char *kstack;            /* Bottom of kernel stack for this process */
    enum procstate state;    /* Process state                           */
    int pid;                 /* Process ID                              */
    struct proc *parent;     /* Parent process                          */
    struct trapframe *tf;    /* Trapframe for current syscall           */
    struct context *context; /* swtch() here to run process             */
    void *chan;              /* If non-zero, sleeping on chan           */
    int killed;              /* If non-zero, have been killed           */
    char name[16];           /* Process name (debugging)                */

    struct file *ofile[NOFILE]; /* Open files */
    struct inode *cwd;          /* Current directory */
};
typedef struct proc proc;
void init_proc();
void spawn_init_process();
void yield();
void exit();