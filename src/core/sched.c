#include <common/defines.h>
#include <core/console.h>
#include <core/container.h>
#include <core/sched.h>
#include <core/virtual_memory.h>

#ifdef MULTI_SCHEDULER

struct cpu cpus[NCPU];
static void scheduler_simple(struct scheduler *this);
static struct proc *alloc_pcb_simple(struct scheduler *this);
static void sched_simple(struct scheduler *this);
static void init_sched_simple(struct scheduler *this);
static void acquire_ptable_lock(struct scheduler *this);
static void release_ptable_lock(struct scheduler *this);
struct sched_op simple_op = {.scheduler = scheduler_simple,
                             .alloc_pcb = alloc_pcb_simple,
                             .sched = sched_simple,
                             .init = init_sched_simple,
                             .acquire_lock = acquire_ptable_lock,
                             .release_lock = release_ptable_lock};
struct scheduler simple_scheduler = {.op = &simple_op};

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

static inline struct context *get_context(struct proc *p) {
    return p->is_scheduler ? ((struct container *)p->cont)->scheduler.context[cpuid()] : p->context;
}

static inline struct context **get_context_address(struct proc *p) {
    return p->is_scheduler ? &((struct container *)p->cont)->scheduler.context[cpuid()]
                           : &p->context;
}

void yield_scheduler(struct scheduler *this) {
    if (this == &root_container->scheduler)
        return;
    acquire_ptable_lock(this->parent);
    thiscpu()->proc->state = RUNNABLE;
    thiscpu()->scheduler = this->parent;
    assert(holding_spinlock(&this->ptable.lock));

    release_ptable_lock(this);
    // swtch(get_context_address(this), get_context(this->parent));
    sched_simple(this->parent);
    acquire_ptable_lock(this);
    release_ptable_lock(this->parent);
    thiscpu()->scheduler = this;
}

NO_RETURN void scheduler_simple(struct scheduler *this) {
    struct proc *p;
    // struct cpu *c = thiscpu();
    // c->proc = NULL;
    assert(thiscpu()->scheduler == this);
    assert(thiscpu()->proc == this->cont->p || this == &root_container->scheduler);

    for (;;) {
        /* Loop over process table looking for process to run. */
        /* TODO: Your code here. */
        acquire_ptable_lock(this);
        for (p = this->ptable.proc; p < this->ptable.proc + NPROC; p++) {
            if (p->state == RUNNABLE) {
                uvm_switch(p->pgdir);
                thiscpu()->proc = p;
                p->state = RUNNING;
                assert(this == thiscpu()->scheduler);
                swtch(&this->context[cpuid()], get_context(p));
                if (p->is_scheduler) {
                    // release_ptable_lock(&((struct container *)p->cont)->scheduler);
                }
                thiscpu()->proc = this->cont->p;
                thiscpu()->scheduler = this;
                assert(thiscpu()->scheduler == this);
                assert(thiscpu()->proc == this->cont->p || this == &root_container->scheduler);
                assert(holding_spinlock(&this->ptable.lock));
                yield_scheduler(this);
                assert(thiscpu()->scheduler == this);
                assert(thiscpu()->proc == this->cont->p || this == &root_container->scheduler);
                assert(holding_spinlock(&this->ptable.lock));
                // back
                // c->proc = NULL;
            }
        }
        assert(thiscpu()->scheduler == this);
        assert(thiscpu()->proc == this->cont->p || this == &root_container->scheduler);
        assert(holding_spinlock(&this->ptable.lock));
        yield_scheduler(this);
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
    swtch(get_context_address(thiscpu()->proc), this->context[cpuid()]);
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
    alloc_resource(this->cont, p, PID);
    p->pid = this->pid;
    p->state = EMBRYO;
    release_ptable_lock(this);

    return p;
}

#endif
