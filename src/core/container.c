#include <common/string.h>
#include <core/arena.h>
#include <core/container.h>
#include <core/physical_memory.h>
#include <core/sched.h>
#include <core/virtual_memory.h>

struct container *root_container = 0;
static Arena arena;
bool do_cont_test = false;

extern void add_loop_test(int times);

static NO_RETURN void container_entry() {
    release_sched_lock();
    thiscpu()->scheduler = &((struct container *)(thiscpu()->proc->cont))->scheduler;
    if (do_cont_test) {
        do_cont_test = false;
        add_loop_test(8);
    }

    enter_scheduler();
    PANIC("scheduler should not return");
}
struct container *alloc_container(bool root) {
    struct container *c = 0;
    struct proc *p;

    c = alloc_object(&arena);
    if (c == 0) {
        goto ret;
    }
    memset(c, 0, sizeof(*c));
    c->scheduler.pid = 0;
    if (root) {
        goto ret;
    }
    p = alloc_pcb();
    if (p == 0) {
        goto ret;
    }

    p->pgdir = pgdir_init();

    // kstack
    // char *sp = kalloc();
    // if (sp == NULL) {
    //     PANIC("cont_alloc: cannot alloc kstack");
    // }
    // p->kstack = sp;
    // sp += KSTACKSIZE;

    // sp -= sizeof(*(p->context));
    for (int i = 0; i < NCPU; i++) {
        char *sp = kalloc();
        if (sp == NULL) {
            PANIC("cont_alloc: cannot alloc kstack");
        }
        sp += KSTACKSIZE;

        sp -= sizeof(*(p->context));
        c->scheduler.context[i] = (struct context *)sp;
        memset(c->scheduler.context[i], 0, sizeof(*c->scheduler.context[i]));
        c->scheduler.context[i]->lr0 = (u64)container_entry;
    }
    // p->context = (struct context *)sp;
    // memset(p->context, 0, sizeof(*(p->context)));
    // p->context->lr0 = container_entry;
    c->p = p;
    p->cont = c;
    p->is_scheduler = true;
    p->state = RUNNABLE;
ret:
    return c;
}

void init_container() {
    ArenaPageAllocator allocator = {.allocate = kalloc, .free = kfree};

    init_arena(&arena, sizeof(struct container), allocator);

    root_container = alloc_container(true);

    root_container->parent = root_container;
    init_spinlock(&root_container->lock, "root container");
    init_spinlock(&root_container->scheduler.ptable.lock, "root scheduler");
    root_container->p = 0;
    root_container->scheduler.op = &simple_op;
    root_container->scheduler.cont = root_container;
}

void *alloc_resource(struct container *this, struct proc *p, resource_t resource) {
    int pid;
    acquire_spinlock(&this->lock);
    switch (resource) {
        case MEMORY: break;
        case PID:
            if (this->scheduler.pid == NPID) {
                return (void *)-1;
            }
            pid = ++this->scheduler.pid;
            this->pmap[pid].valid = true;
            this->pmap[pid].p = p;
            this->pmap[pid].pid_local = pid;
            break;
        case INODE: break;
        default:;
    }
    release_spinlock(&this->lock);

    if (this != root_container) {
        alloc_resource(this->parent, p, resource);
    }
    return 0;
}

struct container *spawn_container(struct container *this, struct sched_op *op) {
    if (this == 0) {
        return 0;
    }
    struct container *c = alloc_container(false);
    if (c == 0) {
        goto ret;
    }
    c->scheduler.op = op;
    c->scheduler.cont = c;
    c->scheduler.parent = &this->scheduler;
    c->parent = this;
    init_spinlock(&c->lock, "cont lock");
    init_spinlock(&c->scheduler.ptable.lock, "p1 lock");

ret:
    return c;
}

/*
 * Add containers
 */
void container_test_init() {
    struct container *c;

    do_cont_test = true;
    add_loop_test(1);
    c = spawn_container(root_container, &simple_op);
    assert(c != NULL);
}
