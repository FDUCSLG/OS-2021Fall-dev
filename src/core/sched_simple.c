#include <common/spinlock.h>
#include <core/console.h>
#include <core/proc.h>
#include <core/sched.h>
#include <core/virtual_memory.h>

#ifndef MULTI_SCHEDULER
struct {
    struct proc proc[NPROC];
    SpinLock lock;
} ptable;

static void scheduler_simple();
static struct proc *alloc_pcb_simple();
static void sched_simple();
static void init_sched_simple();
static void acquire_ptable_lock();
static void release_ptable_lock();
static void proc_wakeup();
static void proc_sleep();
struct sched_op simple_op = {.scheduler = scheduler_simple,
                             .alloc_pcb = alloc_pcb_simple,
                             .sched = sched_simple,
                             .init = init_sched_simple,
                             .acquire_lock = acquire_ptable_lock,
                             .release_lock = release_ptable_lock,
                             .wakeup = proc_wakeup,
                             .sleep = proc_sleep};
struct scheduler simple_scheduler = {.op = &simple_op};

int nextpid = 1;
void swtch(struct context **, struct context *);

static void init_sched_simple() {
    init_spinlock(&ptable.lock, "ptable");
}

static void acquire_ptable_lock() {
    acquire_spinlock(&ptable.lock);
}

static void release_ptable_lock() {
    release_spinlock(&ptable.lock);
}
/*
 * Per-CPU process scheduler
 * Each CPU calls scheduler() after setting itself up.
 * Scheduler never returns.  It loops, doing:
 *  - choose a process to run
 *  - swtch to start running that process
 *  - eventually that process transfers control
 *        via swtch back to the scheduler.
 */
static void scheduler_simple() {
    struct proc *p;
    struct cpu *c = thiscpu();
    c->proc = NULL;

    for (;;) {
        /* Loop over process table looking for process to run. */
        /* TODO: Your code here. */
        acquire_ptable_lock();
        for (p = ptable.proc; p < ptable.proc + NPROC; p++) {
            if (p->state == RUNNABLE) {
                uvm_switch(p->pgdir);
                c->proc = p;
                p->state = RUNNING;
                swtch(&c->scheduler->context, p->context);
                // back
                c->proc = NULL;
            }
        }
        release_ptable_lock();
    }
}

/*
 * Enter scheduler.  Must hold only ptable.lock
 */
static void sched_simple() {
    /* TODO: Your code here. */
    if (!holding_spinlock(&ptable.lock)) {
        PANIC("sched: not holding ptable lock");
    }
    if (thiscpu()->proc->state == RUNNING) {
        PANIC("sched: process running");
    }
    swtch(&thiscpu()->proc->context, thiscpu()->scheduler->context);
}

static struct proc *alloc_pcb_simple() {
    struct proc *p;
    acquire_ptable_lock();
    int found = 0;
    for (p = ptable.proc; p < ptable.proc + NPROC; p++) {
        if (p->state == UNUSED) {
            found = 1;
            break;
        }
    }
    if (found == 0) {
        release_ptable_lock();
        return NULL;
    }
    p->pid = nextpid++;
    p->state = EMBRYO;
    release_ptable_lock();
    return p;
}
#endif
