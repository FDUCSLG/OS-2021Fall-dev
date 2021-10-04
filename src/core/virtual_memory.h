#pragma once

#include <aarch64/mmu.h>
#include <driver/base.h>

#define USPACE_TOP 0x0001000000000000

/*
 * uvm stands user vitual memory.
 */

typedef struct {
    PTEntriesPtr (*pgdir_init)();
    PTEntriesPtr (*pgdir_walk)(PTEntriesPtr pgdir, void *vak, int alloc);
    PTEntriesPtr (*uvm_copy)(PTEntriesPtr pgdir);
    void (*vm_free)(PTEntriesPtr pgdir);
    int (*uvm_map)(PTEntriesPtr pgdir, void *va, usize sz, u64 pa);
    int (*uvm_alloc)(PTEntriesPtr pgdir, usize base, usize stksz, usize oldsz, usize newsz);
    int (*uvm_dealloc)(PTEntriesPtr pgdir, usize base, usize oldsz, usize newsz);
    int (*copyout)(PTEntriesPtr pgdir, void *va, void *p, usize len);
} VirtualMemoryTable;

PTEntriesPtr pgdir_init();
PTEntriesPtr pgdir_walk(PTEntriesPtr pgdir, void *vak, int alloc);
PTEntriesPtr uvm_copy(PTEntriesPtr pgdir);
void vm_free(PTEntriesPtr pgdir);
int uvm_map(PTEntriesPtr pgdir, void *va, usize sz, u64 pa);
int uvm_alloc(PTEntriesPtr pgdir, usize base, usize stksz, usize oldsz, usize newsz);
int uvm_dealloc(PTEntriesPtr pgdir, usize base, usize oldsz, usize newsz);
void uvm_switch(PTEntriesPtr pgdir);
int copyout(PTEntriesPtr pgdir, void *va, void *p, usize len);
void virtual_memory_init(VirtualMemoryTable *vmt_ptr);
void init_virtual_memory();
void vm_test();
