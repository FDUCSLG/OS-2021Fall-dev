static void freelist_init(void *);
static void *freelist_alloc(void *);
static void freelist_free(void *, void *);

typedef void (*page_init)(void *);
typedef void *(*page_alloc)(void *);
typedef void (*page_free)(void *, void *);

struct memmory_manager_table {
    void *memmory_manager;
    page_init init;
    page_alloc alloc_page;
    page_free free_page;
} mmt;

extern char end[];

/*
 * Editable, as long as it works as a memory manager.
 */
struct freelist {
    void *next;
    void *start, *end;
} freelist;

static struct spinlock memlock;

/*
 * Allocate one 4096-byte page of physical memory.
 * Returns a pointer that the kernel can use.
 * Returns 0 if the memory cannot be allocated.
 */
static void *
freelist_alloc(void *fl)
{
    struct freelist* f = (struct freelist*) fl; 
    void *p = f->next;  
    if (p)
        f->next = *(void **)p;
    return p;
}

/*
 * Free the page of physical memory pointed at by v.
 */
static void
freelist_free(void *fl, void *v)
{
    struct freelist *f = (struct freelist*) fl; 
    *(void **)v = f->next;
    f->next = v;
}

/*
 * Record all memory from start to end to freelist as initialization.
 */

static void 
freelist_init(void *fl, void *start, void *end)
{
    struct freelist *f = (struct freelist*) fl; 
    for (void *p = start; p + PGSIZE <= end; p += PGSIZE)
        freelist_free(f, p);
}

void
mm_init()
{
    // HACK Raspberry pi 4b.
    size_t phystop = MIN(0x3F000000, mbox_get_arm_memory());
    mmt.memmory_manager = (void *)&freelist;
    mmt.page_init = freelist_init;
    mmt.page_alloc = freelist_alloc;
    mmt.page_free = freelist_free;
    mmt->page_init(mmt.memmory_manager, ROUNDUP((void *)end, PGSIZE), P2V(phystop));
}

/*
 * Record all memory from start to end to memory manager.
 */

void free_range(void *start, void *end) {
    void *mm = mmt.mm;
    for (void *p = start; p + PGSIZE <= end; p += PGSIZE)
        mmt->free_page(mm, p);
}

/*
 * Allocate a page of physical memory.
 * Returns 0 if failed else a pointer.
 * Corrupt the page by filling non-zero value in it for debugging.
 */
void *
kalloc()
{
    acquire(&memlock);
    void *p = mmt->alloc_page(mmt.memmory_manager);
    release(&memlock);
    return p;
}

/* Free the physical memory pointed at by v. */
void
kfree(void *va)
{
    acquire(&memlock);
    mmt->free_page(mmt.memmory_manager, va);
    release(&memlock);
}