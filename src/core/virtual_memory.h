#pragma once

#ifndef _CORE_VIRTUAL MEMORY_
#define _CORE_VIRTUAL MEMORY_

#define USERTOP  0x0001000000000000
#define KERNBASE 0xFFFF000000000000

/* 
 * uvm stands user vitual memory. 
 */

typedef struct {
    PTEntriesPtr (*pgdir_init)();
    PTEntriesPtr (*pgdir_walk)(PTEntriesPtr, void *, int);
    PTEntriesPtr (*uvm_copy)(PTEntriesPtr);
    void (*vm_free) (PTEntriesPtr);
    int (*uvm_map)(PTEntriesPtr, void *, size_t, uint64_t);
    int (*uvm_alloc) (PTEntriesPtr, size_t, size_t, size_t, size_t);
    int (*uvm_dealloc) (PTEntriesPtr, size_t, size_t, size_t);
    int (*copyout)(PTEntriesPtr, void *, void *, size_t);
} VirtualMemoryTable;


PTEntriesPtr pgdir_init();
PTEntriesPtr pgdir_walk(PTEntriesPtr, void *, int);
PTEntriesPtr uvm_copy(PTEntriesPtr);
void vm_free(PTEntriesPtr);
int uvm_map(PTEntriesPtr, void *, size_t, uint64_t);
int uvm_alloc(PTEntriesPtr, size_t, size_t, size_t, size_t);
int uvm_dealloc(PTEntriesPtr, size_t, size_t, size_t);
void uvm_switch(PTEntriesPtr);
int copyout(PTEntriesPtr, void *, void *, size_t);
void virtual_memory_init(VirtualMemoryTable *);

#endif