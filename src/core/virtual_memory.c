#include <aarch64/intrinsic.h>
#include <aarch64/mmu.h>
#include <common/string.h>
#include <core/virtual_memory.h>


/* For simplicity, we only support 4k pages in user pgdir. */

extern PTEntries kpgdir;
VirtualMemoryTable vmt;

PTEntriesPtr pgdir_init() {
    return vmt.pgdir_init();
}

PTEntriesPtr pgdir_walk(PTEntriesPtr pgdir, void *vak, int alloc) {
    return vmt.pgdir_walk(pgdir, vak, alloc);
}

PTEntriesPtr uvm_copy(PTEntriesPtr pgdir) {
    return vmt.uvm_copy(pgdir);
}

void vm_free(PTEntriesPtr pgdir) {
    vmt.vm_free(pgdir);
}

int uvm_map(PTEntriesPtr pgdir, void *va, size_t sz, uint64_t pa) {
    return vmt.uvm_map(pgdir, va, sz, pa);
}

int uvm_alloc(PTEntriesPtr pgdir, size_t base, size_t stksz, size_t oldsz, size_t newsz) {
    return vmt.uvm_alloc(pgdir, base, stksz, oldsz, newsz);
}

int uvm_dealloc(PTEntriesPtr pgdir, size_t base, size_t oldsz, size_t newsz) {
    return vmt.uvm_dealloc(pgdir, base, oldsz, newsz);
}

void uvm_switch(PTEntriesPtr pgdir) {
    // FIXME: Use NG and ASID for efficiency.
    arch_set_ttbr0(K2P(pgdir));
}

int copyout(PTEntriesPtr pgdir, void *va, void *p, size_t len) {
    return vmt.copyout(pgdir, va, p, len);
}


/*
 * generate a empty page as page directory
 */

static PTEntriesPtr my_pgdir_init() {
    PTEntriesPtr pgdir = kalloc();
    if (pgdir)
        memset(pgdir, 0, PAGE_SIZE);
    // else {
    //     panic("my_pgdir_init: No free page.");
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
            //     panic("my_pgdir_walk: No free page.");
            // }
        }
        pagetable = P2K(PTE_ADDRRESS(pagetable[page_index]));
    }
    return &pagetable[virtual_address_tag & 0x1FF];
}

/* Fork a process's page table. */

static PTEntriesPtr my_uvm_copy(PTEntriesPtr pgdir) {
    return recur_uvm_copy(pgdir, 0);
}

static PTEntriesPtr recur_uvm_copy(PTEntriesPtr pgdir, int level) {
    PTEntriesPtr newpgdir = my_pgdir_init();
    // if (!newpgdir) {
    //     panic("recur_uvm_copy: No page to alloc.");
    // }
    if (level < 3) {
        for (int i = 0; i < 512; i++) {
            if (pgdir[i] & PTE_VALID) {
                // assert(pgdir[i] & PTE_TABLE);
                PTEntriesPtr page_table = P2K(PTE_ADDRESS(pgdir[i]));
                PTEntriesPtr page_copy = recur_uvm_copy(page_table, level + 1);
                // if (!page_copy) {
                //     panic("recur_uvm_copy: No page to alloc.")
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
                PTEntriesPtr page_content_ptr = P2K(PTE_ADDRESS(pgdir[i]));
                PTEntriesPtr page_copy = kalloc();
                memmove(page_copy, page_content_ptr, PAGE_SIZE);
                // if (!page_copy) {
                //     panic("recur_uvm_copy: No page to alloc.")
                // }
                newpgdir[i] = K2P(page_copy) | PTE_FLAGS(pgdir[i]);
            }
        }
    }
    return newpgdir;
}


/* Free a user page table and all the physical memory pages. */

void my_vm_free(PTEntriesPtr pgdir) {
    recur_vm_free(pgdir, 0);
}

void recur_vm_free(PTEntriesPtr pgdir, int level) {
    if (level < 3) {
        for (int i = 0; i < 512; i++) {
            if (pgdir[i] & PTE_VALID) {
                // assert(pgdir[i] & PTE_TABLE);
                PTEntriesPtr page_table = P2K(PTE_ADDRESS(pgdir[i]));
                recur_vm_free(page_table, level + 1);
            }
        }
        kfree(pgdir);
    } 
    else {
        for (int i = 0; i < 512; i++) {
            if (pgdir[i] & PTE_VALID) {
                PTEntriesPtr page_content_ptr = P2K(PTE_ADDRESS(pgdir[i]));
                kfree(page_content_ptr);
            }
        }
        kfree(pgdir);
    }
}

/*
 * Create PTEs for virtual addresses starting at va that refer to
 * physical addresses starting at pa. va and size might not
 * be page-aligned.
 * Return -1 if failed else 0.
 */

int my_uvm_map(PTEntriesPtr pgdir, void *va, size_t sz, uint64_t pa) {
    void *ptr = va - va % PAGE_SIZE;
    void *end = va + sz;
    pa = pa - pa % PAGE_SIZE;
    for (; ptr < end; ptr += PAGE_SIZE, pa += PAGE_SIZE) {
        PTEntriesPtr page_content_ptr = pgdir_walk(pgdir, ptr, 1);
        if (!page_content_ptr) {
            printf("uvm_map: pgdir_walk failed.\n");
            return -1;
        }
        // if (*page_content_ptr & PTE_VALID)
        //     panic("uvm_map: remap.");
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

int my_uvm_alloc(PTEntriesPtr pgdir, size_t base, size_t stksz, size_t oldsz, size_t newsz) {
    // assert(stksz % PAGE_SIZE == 0);
    if (!(stksz < USERTOP && base <= oldsz && oldsz <= newsz && newsz < USERTOP - stksz)) {
        printf("my_uvm_alloc: invalid arguments.");
        return 0;
    }

    for (size_t a = oldsz + (PAGE_SIZE - oldsz % PAGE_SIZE) % PAGE_SIZE; a < newsz; a += PAGE_SIZE) {
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

int my_uvm_dealloc(PTEntriesPtr pgdir, size_t base, size_t oldsz, size_t newsz) {
    if (newsz >= oldsz || newsz < base)
        return oldsz;

    for (size_t a = ROUNDUP(newsz, PGSIZE); a < oldsz; a += PGSIZE) {
        PTEntriesPtr page_content_ptr = pgdir_walk(pgdir, ptr, 0);
        if (pte && (*pte & PTE_VALID)) {
            uint64_t pa = PTE_ADDRESS(*pte);
            // if (!pa) {
            //     panic("GG");
            // }
            kfree(P2K(pa));
            *pte = 0;
        }
        // else {
        //     panic("attempt to free unallocated page");
        // }
    }

    return newsz;
}


/*
 * Copy len bytes from p to user address va in page table pgdir.
 * Allocate physical pages if required.
 * Useful when pgdir is not the current page table.
 */

int my_copyout(PTEntriesPtr pgdir, void *va, void *p, size_t len) {
    void *page;
    size_t n, page_offset;
    PTEntriesPtr page_content_ptr;
    if ((size_t)va + len > USERTOP)
        return -1;
    for (; len; len -= n, va += n) {
        page_offset = va % PAGE_SIZE;
        page_content_ptr = pgdir_walk(pgdir, va, 1);
        if (!page_content_ptr) {
            printf("my_copyout: pgdir_walk failed");
            return -1;
        }
        if (*page_content_ptr & PTE_VALID) {
            page = P2K(PTE_ADDRESS(*pte));
        } 
        else {
            page = kalloc();
            if (!page) {
                printf("my_copyout: kalloc failed.");
                return -1;
            }
            *page_content_ptr = V2P(page) | PTE_USER_DATA;
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


void virtual_memory_init(VirtualMemoryTable *vmt_ptr) {
    vmt_ptr->pgdir_init = my_pgdir_init;
    vmt_ptr->pgdir_walk = my_pgdir_walk;
    vmt_ptr->uvm_copy = my_uvm_copy;
    vmt_ptr->vm_free = vm_free;
    vmt_ptr->uvm_map = my_uvm_map;
    vmt_ptr->uvm_alloc = my_uvm_alloc;
    vmt_ptr->uvm_dealloc = my_uvm_dealloc;
    vmt_ptr->copyout = my_copyout;
}