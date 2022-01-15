#pragma once

#include <common/defines.h>
// #include <core/sched.h>
#include <common/spinlock.h>
#include <core/trapframe.h>
#include <fs/inode.h>

#define NPROC      14   /* maximum number of processes */
#define NOFILE     8    /* open files per process */
#define KSTACKSIZE 4096 /* size of per-process kernel stack */

enum procstate { UNUSED, EMBRYO, SLEEPING, RUNNABLE, RUNNING, ZOMBIE };

/* Stack must always be 16 bytes aligned. */
struct context {
    u64 lr0, lr, fp;
    u64 x[10]; /* X28 ... X19 */
    u64 padding;
    // u64 q0[2];             /* V0 */
};

struct proc {
    // struct sched_obj sched;
    u64 sz;                  /* Size of process memory (bytes)          */
    u64 *pgdir;              /* Page table                              */
    char *kstack;            /* Bottom of kernel stack for this process */
    enum procstate state;    /* Process state                           */
    int pid;                 /* Process ID                              */
    struct proc *parent;     /* Parent process                          */
    Trapframe *tf;           /* Trapframe for current syscall           */
    struct context *context; /* swtch() here to run process             */
    void *chan;              /* If non-zero, sleeping on chan           */
    int killed;              /* If non-zero, have been killed           */
    char name[16];           /* Process name (debugging)                */
    void *cont;
    bool is_scheduler;

    struct file *ofile[NOFILE]; /* Open files */
    Inode *cwd;                 /* Current directory */
    u64 stksz, base;
};

typedef struct proc proc;

void init_proc();
void spawn_init_process();
void yield();
NO_RETURN void exit();
void sleep(void *chan, SpinLock *lock);
void wakeup(void *chan);
void idle_init();
int growproc(int n);
int wait();
int fork();
