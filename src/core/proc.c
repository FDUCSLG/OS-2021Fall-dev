
#include <aarch64/mmu.h>
#include <common/string.h>
#include <core/console.h>
#include <core/physical_memory.h>
#include <core/proc.h>
#include <core/sched.h>
#include <core/virtual_memory.h>

void forkret();
extern void trap_return();
/*
 * Look through the process table for an UNUSED proc.
 * If found, change state to EMBRYO and initialize
 * state (allocate stack, clear trapframe, set context for switch...)
 * required to run in the kernel. Otherwise return 0.
 */
static struct proc *alloc_proc() {
    struct proc *p;
    p = alloc_pcb();
    if (p == NULL) {
        PANIC("no free pcbs\n");
    }

    p->pgdir = pgdir_init();
    // memset(p->pgdir, 0, PGSIZE);

    // kstack
    char *sp = kalloc();
    if (sp == NULL) {
        PANIC("proc_alloc: cannot alloc kstack");
    }
    p->kstack = sp;
    sp += KSTACKSIZE;
    // trapframe
    sp -= sizeof(*(p->tf));
    p->tf = (Trapframe *)sp;
    memset(p->tf, 0, sizeof(*(p->tf)));
    // trapret
    // sp -= 8;
    // *(u64*)sp = (u64)trap_return;
    // // sp
    // sp -= 8;
    // *(u64*)sp = (u64)p->kstack + KSTACKSIZE;
    // context
    sp -= sizeof(*(p->context));
    p->context = (struct context *)sp;
    memset(p->context, 0, sizeof(*(p->context)));
    p->context->lr0 = (u64)forkret;
    p->context->lr = (u64)trap_return;

    // other settings
    // p->state = EMBRYO;
    return p;
}

/*
 * Set up first user process(Only used once).
 * Set trapframe for the new process to run
 * from the beginning of the user process determined
 * by uvm_init
 */
void spawn_init_process() {
    struct proc *p;
    extern char icode[], eicode[];
    p = alloc_proc();

    char *r = kalloc();
    if (r == NULL) {
        PANIC("uvm_init: cannot alloc a page");
    }
    memset(r, 0, PAGE_SIZE);
    uvm_map(p->pgdir, (void *)0, PAGE_SIZE, K2P(r));
    memmove(r, (void *)icode, eicode - icode);

    memset(p->tf, 0, sizeof(*(p->tf)));
    p->tf->spsr = 0;
    p->tf->sp = PAGE_SIZE;
    p->tf->x[30] = 0;
    p->tf->elr = 0;

    p->state = RUNNABLE;
}

/*
 * A fork child will first swtch here, and then "return" to user space.
 */
void forkret() {
    release_sched_lock();
}

/*
 * Exit the current process.  Does not return.
 * An exited process remains in the zombie state
 * until its parent calls wait() to find out it exited.
 */
NO_RETURN void exit() {
    struct proc *p = thiscpu()->proc;
    acquire_sched_lock();
    p->state = ZOMBIE;
    sched();

    PANIC("exit should not return\n");
}

void
sleep(void* chan, struct Spinlock* lk)
{
    /* TODO: Your code here. */
    struct proc* p = thiscpu()->proc;
    if (p == 0) {
        panic("sleep");
    }

    // if (lk == 0) {
    //     panic("sleep without lk");
    // }

    if (lk != &ptable.lock) {
        acquire_ptable_lock();
        release_spinlock(lk);
    }

    p->chan = chan;
    p->state = SLEEPING;

    sched();
    p->chan = 0;

    if (lk != &ptable.lock) {
        release_ptable_lock();
        acquire_spinlock(lk);
    }

}
