#include <common/defines.h>
#include <core/console.h>
#include <core/sched.h>
#ifdef MULTI_SCHEDULER

static void scheduler_simple();
static struct proc *alloc_pcb_simple();
static void sched_simple();
static void init_sched_simple();
static void acquire_ptable_lock();
static void release_ptable_lock();
struct sched_op simple_op = {.scheduler = scheduler_simple,
                             .alloc_pcb = alloc_pcb_simple,
                             .sched = sched_simple,
                             .init = init_sched_simple,
                             .acquire_lock = acquire_ptable_lock,
                             .release_lock = release_ptable_lock};
struct scheduler simple_scheduler = {.op = &simple_op};

int nextpid = 1;
void swtch(struct context **, struct context *);

static void init_sched_simple(struct scheduler *this) {
    init_spinlock(&this->ptable.lock, "ptable");
}

static void acquire_ptable_lock(struct scheduler *this) {
    acquire_spinlock(&this->ptable.lock);
}

static void release_ptable_lock(struct scheduler *this) {
    release_spinlock(&this->ptable.lock);
}

void init_sched()
{
    // asserts(root_scheduler == NULL, "Root scheduler alloc more than once");
    // root_scheduler = alloc_sched();
    // asserts(root_scheduler != NULL, "Root scheduler not alloc yet");
}

NO_RETURN void scheduler_simple(struct scheduler *this)
{
	struct proc *p;
    struct cpu *c = thiscpu();
    c->proc = NULL;

    for (;;) {
        /* Loop over process table looking for process to run. */
        /* TODO: Your code here. */
        acquire_ptable_lock(this);
        for (p = this->ptable.proc; p < this->ptable.proc + NPROC; p++) {
            if (p->state == RUNNABLE) {
                uvm_switch(p->pgdir);
                c->proc = p;
                p->state = RUNNING;
                swtch(&c->scheduler->context, p->context);
                // back
                c->proc = NULL;
            }
        }
        release_ptable_lock(this);
    }
}

/*
 * Enter scheduler.  Must hold only ptable.lock
 */
static void sched_simple(struct scheduler *this) {
    /* TODO: Your code here. */
    if (!holding_spinlock(&this->ptable.lock)) {
        PANIC("sched: not holding ptable lock");
    }
    if (thiscpu()->proc->state == RUNNING) {
        PANIC("sched: process running");
    }
    swtch(&thiscpu()->proc->context, thiscpu()->scheduler->context);
}

static struct proc *alloc_pcb_simple(struct scheduler *this) {
    struct proc *p;
    acquire_ptable_lock(this);
    int found = 0;
    for (p = this->ptable.proc; p < this->ptable.proc + NPROC; p++) {
        if (p->state == UNUSED) {
            found = 1;
            break;
        }
    }
    if (found == 0) {
        release_ptable_lock(this);
        return NULL;
    }
    p->pid = nextpid++;
    p->state = EMBRYO;
    release_ptable_lock(this);
    return p;
}
#endif