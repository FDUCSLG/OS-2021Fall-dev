#include <aarch64/mmu.h>
#include <common/defines.h>
#include <common/spinlock.h>
#include <core/console.h>
#include <core/physical_memory.h>

extern char end[];

MemmoryManagerTable mmt;

/*
 * Editable, as long as it works as a memory manager.
 */
FreeList freelist;

/*
 * Allocate one 4096-byte page of physical memory.
 * Returns a pointer that the kernel can use.
 * Returns 0 if the memory cannot be allocated.
 */
static void *freelist_alloc(void *this) {
    FreeList *f = (FreeList *)this;
    void *p = f->next;
    if (p)
        f->next = *(void **)p;
    // else
    //     PANIC;
    return p;
}

/*
 * Free the page of physical memory pointed at by v.
 */
static void freelist_free(void *this, void *v) {
    FreeList *f = (FreeList *)this;
    *(void **)v = f->next;
    f->next = v;
}

/*
 * Record all memory from start to end to freelist as initialization.
 */

static void init_freelist(void *this, void *start, void *end) {
    FreeList *f = (FreeList *)this;
    for (void *p = start; p + PAGE_SIZE <= end; p += PAGE_SIZE) {
        freelist_free(f, p);
    }
}

static void init_memmory_manager_table(MemmoryManagerTable *mmt_ptr) {
    mmt_ptr->memmory_manager = (void *)&freelist;
    mmt_ptr->page_init = init_freelist;
    mmt_ptr->page_alloc = freelist_alloc;
    mmt_ptr->page_free = freelist_free;
}

void init_memory_manager() {
    // HACK Raspberry pi 4b.
    // usize phystop = MIN(0x3F000000, mbox_get_arm_memory());
    usize phystop = 0x3F000000;

    // notice here for roundup.
    void *roundup_end = (void *)round_up((u64)end, PAGE_SIZE);
    init_memmory_manager_table(&mmt);
    mmt.page_init(mmt.memmory_manager, roundup_end, (void *)P2K(phystop));

    init_spinlock(&mmt.lock, "memory");
}

/*
 * Record all memory from start to end to memory manager.
 */
void free_range(void *start, void *end) {
    for (void *p = start; p + PAGE_SIZE <= end; p += PAGE_SIZE)
        mmt.page_free(mmt.memmory_manager, p);
}

/*
 * Allocate a page of physical memory.
 * Returns 0 if failed else a pointer.
 * Corrupt the page by filling non-zero value in it for debugging.
 */
void *kalloc() {
    acquire_spinlock(&mmt.lock);
    void *p = mmt.page_alloc(mmt.memmory_manager);

    release_spinlock(&mmt.lock);
    return p;
}

/* Free the physical memory pointed at by v. */
void kfree(void *va) {
    acquire_spinlock(&mmt.lock);
    mmt.page_free(mmt.memmory_manager, va);
    release_spinlock(&mmt.lock);
}
