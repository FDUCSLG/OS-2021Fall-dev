#include <aarch64/mmu.h>
#include <common/spinlock.h>
#include <core/memory_manage.h>

extern char end[];

MemmoryManagerTable mmt;
SpinLock memmory_manager_lock;

/*
 * Editable, as long as it works as a memory manager.
 */
FreeList freelist;
static void init_freelist(void *, void *, void *);
static void *freelist_page_alloc(void *);
static void freelist_page_free(void *, void *);

/*
 * Allocate one 4096-byte page of physical memory.
 * Returns a pointer that the kernel can use.
 * Returns 0 if the memory cannot be allocated.
 */
static void *freelist_alloc(void *freelist_ptr) {
    Freelist* f = (Freelist*) freelist_ptr; 
    void *p = f->next;  
    if (p)
        f->next = *(void **)p;
    // else
    //     panic;
    return p;
}

/*
 * Free the page of physical memory pointed at by v.
 */
static void freelist_free(void *freelist_ptr, void *v) {
    Freelist* f = (Freelist*) freelist_ptr; 
    *(void **)v = f->next;
    f->next = v;
}

/*
 * Record all memory from start to end to freelist as initialization.
 */

static void init_freelist(void *freelist_ptr, void *start, void *end) {
    Freelist* f = (Freelist*) freelist_ptr; 
    for (void *p = start; p + PAGE_SIZE <= end; p += PAGE_SIZE)
        freelist_free(f, p);
}

static void init_memmory_manager_table(MemmoryManagerTable *mmt_ptr) {
    mmt_ptr->memmory_manager = (void *)&freelist;
    mmt_ptr->page_init = init_freelist;
    mmt_ptr->page_alloc = freelist_alloc;
    mmt_ptr->page_free = freelist_free;
}

void init_memory_manager() {
    // HACK Raspberry pi 4b.
    // size_t phystop = MIN(0x3F000000, mbox_get_arm_memory());
    size_t phystop = 0x3F000000;
    size_t ROUNDUP_end = end + (PAGE_SIZE - (end % PAGE_SIZE)) % PAGE_SIZE;
    init_memmory_manager_table(mmt);
    mmt.page_init(mmt.memmory_manager, ROUNDUP_end, P2K(phystop));
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
    acquire_spinlock(&memmory_manager_lock);
    void *p = mmt.page_alloc(mmt.memmory_manager);
    release_spinlock(&memmory_manager_lock);
    return p;
}

/* Free the physical memory pointed at by v. */
void kfree(void *va) {
    acquire_spinlock(&memmory_manager_lock);
    mmt.page_free(mmt.memmory_manager, va);
    release_spinlock(&memmory_manager_lock);
}