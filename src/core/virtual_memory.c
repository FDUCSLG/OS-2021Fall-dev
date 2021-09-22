#include <aarch64/intrinsic.h>
#include <aarch64/mmu.h>
#include <common/string.h>
#include <core/virtual_memory.h>
#include <core/memory_manage.h>
#include <common/types.h>
#include <core/console.h>

/* For simplicity, we only support 4k pages in user pgdir. */

extern PTEntries kpgdir;
VMemory vmem;

PTEntriesPtr pgdir_init(void) {
    return vmem.pgdir_init();
}

PTEntriesPtr pgdir_walk(PTEntriesPtr pgdir, void *kernel_address, int alloc) {
    return vmem.pgdir_walk(pgdir, kernel_address, alloc);
}

PTEntriesPtr uvm_copy(PTEntriesPtr pgdir) {
    return vmem.uvm_copy(pgdir);
}

NORETURN void vm_free(PTEntriesPtr pgdir) {
    vmem.vm_free(pgdir);
}

int uvm_map(PTEntriesPtr pgdir, void *kernel_address, size_t size, uint64_t physical_address) {
    return vmem.uvm_map(pgdir, kernel_address, size, physical_address);
}

int uvm_alloc(PTEntriesPtr pgdir, size_t base, size_t stksz, size_t oldsz, size_t newsz) {
    return vmem.uvm_alloc(pgdir, base, stksz, oldsz, newsz);
}

int uvm_dealloc(PTEntriesPtr pgdir, size_t base, size_t oldsz, size_t newsz) {
    return vmem.uvm_dealloc(pgdir, base, oldsz, newsz);
}

void uvm_switch(PTEntriesPtr pgdir) {
    // FIXME: Use NG and ASID for efficiency.
    arch_set_ttbr0(K2P(pgdir));
}

int copyout(PTEntriesPtr pgdir, void *tgt_address, void *src_address, size_t len) {
    return vmem.copyout(pgdir, tgt_address, src_address, len);
}


/*
 * generate a empty page as page directory
 */

static PTEntriesPtr my_pgdir_init(void) {
    PTEntriesPtr pgdir = kalloc();
    if (pgdir)
        memset(pgdir, 0, PAGE_SIZE);
    // else {
    //     PANIC("my_pgdir_init: No free page.");
    // }
    return pgdir;
}


/*
 * return the address of the pte in user page table
 * pgdir that corresponds to virtual address va.
 * if alloc != 0, create any required page table pages.
 */

static PTEntriesPtr my_pgdir_walk(PTEntriesPtr pgdir, void *vak, int alloc) {
    PTEntriesPtr pagetable = pgdir;
    uint64_t virtual_address_tag = ((uint64_t) vak) >> 12;    
    for (int i = 0; i < 3; i++) {
        int page_index = (int) (virtual_address_tag >> ((3 - i) * 9)) & 0x1FF;
        if (!(pagetable[page_index] & PTE_VALID)) {
            void *p;
            /* FIXME Free allocated pages and restore modified pgt */
            if (alloc && (p = kalloc())) {
                memset(p, 0, PAGE_SIZE);
                pagetable[page_index] = K2P(p) | PTE_TABLE;
            }
            // else {
            //     PANIC("my_pgdir_walk: No free page.");
            // }
        }
        pagetable = (PTEntriesPtr)(P2K(PTE_ADDRESS(pagetable[page_index])));
    }
    return &pagetable[virtual_address_tag & 0x1FF];
}

/* Fork a process's page table. */

static PTEntriesPtr recur_uvm_copy(PTEntriesPtr pgdir, int level) {
    PTEntriesPtr newpgdir = my_pgdir_init();
    // if (!newpgdir) {
    //     PANIC("recur_uvm_copy: No page to alloc.");
    // }
    if (level < 3) {
        for (int i = 0; i < 512; i++) {
            if (pgdir[i] & PTE_VALID) {
                // assert(pgdir[i] & PTE_TABLE);
                PTEntriesPtr page_table = (PTEntriesPtr)(P2K(PTE_ADDRESS(pgdir[i])));
                PTEntriesPtr page_copy = recur_uvm_copy(page_table, level + 1);
                // if (!page_copy) {
                //     PANIC("recur_uvm_copy: No page to alloc.")
                // }
                newpgdir[i] = K2P(page_copy) | PTE_FLAGS(pgdir[i]);
            }
        }
    } 
    else {
        for (int i = 0; i < 512; i++) {
            if (pgdir[i] & PTE_VALID) {
                // assert(pgdir[i] & PTE_TABLE);
                // assert(pgdir[i] & PTE_PAGE);
                // assert(pgdir[i] & PTE_USER);
                // assert(pgdir[i] & PTE_NORMAL);
                // assert(PTE_ADDRESS(pgdir[i]) < KERNBASE);
                PTEntriesPtr page_content_ptr = (PTEntriesPtr)(P2K(PTE_ADDRESS(pgdir[i])));
                PTEntriesPtr page_copy = kalloc();
                // if (!page_copy) {
                //     PANIC("recur_uvm_copy: No page to alloc.")
                // }
                memmove(page_copy, page_content_ptr, PAGE_SIZE);
                newpgdir[i] = K2P(page_copy) | PTE_FLAGS(pgdir[i]);
            }
        }
    }
    return newpgdir;
}

static PTEntriesPtr my_uvm_copy(PTEntriesPtr pgdir) {
    return recur_uvm_copy(pgdir, 0);
}

/* Free a user page table and all the physical memory pages. */


NORETURN static void recur_vm_free(PTEntriesPtr pgdir, int level) {
    if (level < 3) {
        for (int i = 0; i < 512; i++) {
            if (pgdir[i] & PTE_VALID) {
                // assert(pgdir[i] & PTE_TABLE);
                PTEntriesPtr page_table = (PTEntriesPtr)(P2K(PTE_ADDRESS(pgdir[i])));
                recur_vm_free(page_table, level + 1);
            }
        }
        kfree(pgdir);
    } 
    else {
        for (int i = 0; i < 512; i++) {
            if (pgdir[i] & PTE_VALID) {
                PTEntriesPtr page_content_ptr = (PTEntriesPtr)(P2K(PTE_ADDRESS(pgdir[i])));
                kfree(page_content_ptr);
            }
        }
        kfree(pgdir);
    }
}

NORETURN static void my_vm_free(PTEntriesPtr pgdir) {
    recur_vm_free(pgdir, 0);
}

/*
 * Create PTEs for virtual addresses starting at va that refer to
 * physical addresses starting at pa. va and size might not
 * be page-aligned.
 * Return -1 if failed else 0.
 */

static int my_uvm_map(PTEntriesPtr pgdir, void *va, size_t sz, uint64_t pa) {
    void *ptr = ROUNDDOWN(va,PAGE_SIZE);
    void *end = (void *)((uint64_t)va + sz);
    pa = ROUNDDOWN(pa,PAGE_SIZE);
    for (; ptr < end; ptr += PAGE_SIZE, pa += PAGE_SIZE) {
        PTEntriesPtr page_content_ptr = pgdir_walk(pgdir, ptr, 1);
        if (!page_content_ptr) {
            PANIC("uvm_map: pgdir_walk failed.\n");
            return -1;
        }
        // if (*page_content_ptr & PTE_VALID)
        //     PANIC("uvm_map: remap.");
        *page_content_ptr = pa | PTE_USER_DATA;
    }
    return 0;
}

/*
 * Allocate page tables and physical memory to grow process
 * from oldsz to newsz, which need not be page aligned.
 * Stack size stksz should be page aligned.
 * Returns new size or 0 on error.
 */

static int my_uvm_alloc(PTEntriesPtr pgdir, size_t base, size_t stksz, size_t oldsz, size_t newsz) {
    // assert(stksz % PAGE_SIZE == 0);
    if (!(stksz < USERTOP && base <= oldsz && oldsz <= newsz && newsz < USERTOP - stksz)) {
        PANIC("my_uvm_alloc: invalid arguments.");
        return 0;
    }

    for (size_t a = ROUNDUP(oldsz,PAGE_SIZE); a < newsz; a += PAGE_SIZE) {
        void *page = kalloc();
        if (!page) {
            printf("my_uvm_alloc: kalloc failed.");
            uvm_dealloc(pgdir, base, a-PAGE_SIZE, oldsz);
            return 0;
        }
        if (uvm_map(pgdir, (void *)a, PAGE_SIZE, K2P((uint64_t) page)) < 0) {
            printf("my_uvm_alloc: uvm_map failed.");
            kfree(page);
            uvm_dealloc(pgdir, base, a-PAGE_SIZE, oldsz);
            return 0;
        }
    }

    return newsz;
}


/*
 * Deallocate user pages to bring the process size from oldsz to
 * newsz.  oldsz and newsz need not be page-aligned, nor does newsz
 * need to be less than oldsz.  oldsz can be larger than the actual
 * process size.  Returns the new process size.
 */

static int my_uvm_dealloc(PTEntriesPtr pgdir, size_t base, size_t oldsz, size_t newsz) {
    if (newsz >= oldsz || newsz < base)
        return oldsz;

    for (size_t a = ROUNDUP(newsz, PAGE_SIZE); a < oldsz; a += PAGE_SIZE) {
        PTEntriesPtr page_content_ptr = pgdir_walk(pgdir, (void *)a, 0);
        if (page_content_ptr && (*page_content_ptr & PTE_VALID)) {
            uint64_t pa = PTE_ADDRESS(*page_content_ptr);
            // if (!pa) {
            //     PANIC("GG");
            // }
            kfree((void *)P2K(pa));
            *page_content_ptr = 0;
        }
        // else {
        //     PANIC("attempt to free unallocated page");
        // }
    }

    return newsz;
}


/*
 * Copy len bytes from p to user address va in page table pgdir.
 * Allocate physical pages if required.
 * Useful when pgdir is not the current page table.
 */

static int my_copyout(PTEntriesPtr pgdir, void *va, void *p, size_t len) {
    void *page;
    size_t n, page_offset;
    PTEntriesPtr page_content_ptr;
    if ((size_t)va + len > USERTOP)
        return -1;
    for (; len; len -= n, va += n) {
        page_offset = (size_t)va % PAGE_SIZE;
        page_content_ptr = pgdir_walk(pgdir, va, 1);
        if (!page_content_ptr) {
            printf("my_copyout: pgdir_walk failed");
            return -1;
        }
        if (*page_content_ptr & PTE_VALID) {
            page = (void *)(P2K(PTE_ADDRESS(*page_content_ptr)));
        } 
        else {
            page = kalloc();
            if (!page) {
                printf("my_copyout: kalloc failed.");
                return -1;
            }
            *page_content_ptr = K2P(page) | PTE_USER_DATA;
        }
        n = PAGE_SIZE - page_offset;
        n = n < len ? n : len;
        if (p) {
            memmove(page + page_offset, p, n);
            p += n;
        } else
            memset(page + page_offset, 0, n);
        // disb();
        // Flush to memory to sync with icache.
        // dccivac(page + pgoff, n);
        // disb();
    }
    return 0;
}


NORETURN void vmemory_init(VMemory *vmem_ptr) {
    vmem_ptr->pgdir_init = my_pgdir_init;
    vmem_ptr->pgdir_walk = my_pgdir_walk;
    vmem_ptr->uvm_copy = my_uvm_copy;
    vmem_ptr->vm_free = my_vm_free;
    vmem_ptr->uvm_map = my_uvm_map;
    vmem_ptr->uvm_alloc = my_uvm_alloc;
    vmem_ptr->uvm_dealloc = my_uvm_dealloc;
    vmem_ptr->copyout = my_copyout;
}