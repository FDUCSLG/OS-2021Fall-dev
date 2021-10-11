#include <core/console.h>
#include <core/proc.h>
#include <core/sched.h>
#include <core/virtual_memory.h>

struct {
    struct proc proc[NPROC];
} ptable;

static void scheduler_simple();
static struct proc *alloc_pcb_simple();
static void sched_simple();
struct sched_op simple_op = {
    .scheduler = scheduler_simple, .alloc_pcb = alloc_pcb_simple, .sched = sched_simple};
struct scheduler simple_scheduler = {.op = &simple_op};

int nextpid = 1;
void swtch(struct context **, struct context *);
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
    }
}

/*
 * Enter scheduler.  Must hold only ptable.lock
 */
static void sched_simple() {
    /* TODO: Your code here. */
    if (thiscpu()->proc->state == RUNNING) {
        PANIC("sched: process running");
    }
    swtch(&thiscpu()->proc->context, thiscpu()->scheduler->context);
}

static struct proc *alloc_pcb_simple() {
    struct proc *p;
    int found = 0;
    for (p = ptable.proc; p < ptable.proc + NPROC; p++) {
        if (p->state == UNUSED) {
            found = 1;
            break;
        }
    }
    if (found == 0) {
        return NULL;
    }
    p->pid = nextpid++;
    return p;
}
