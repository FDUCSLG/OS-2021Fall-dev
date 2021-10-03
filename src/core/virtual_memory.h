#include <aarch64/mmu.h>
#pragma once

#ifndef _CORE_VIRTUAL_MEMORY_
#define _CORE_VIRTUAL_MEMORY_

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
    int (*uvm_map)(PTEntriesPtr, void *, usize, u64);
    int (*uvm_alloc) (PTEntriesPtr, usize, usize, usize, usize);
    int (*uvm_dealloc) (PTEntriesPtr, usize, usize, usize);
    int (*copyout)(PTEntriesPtr, void *, void *, usize);
} VirtualMemoryTable;


PTEntriesPtr pgdir_init();
PTEntriesPtr pgdir_walk(PTEntriesPtr, void *, int);
PTEntriesPtr uvm_copy(PTEntriesPtr);
void vm_free(PTEntriesPtr);
int uvm_map(PTEntriesPtr, void *, usize, u64);
int uvm_alloc(PTEntriesPtr, usize, usize, usize, usize);
int uvm_dealloc(PTEntriesPtr, usize, usize, usize);
void uvm_switch(PTEntriesPtr);
int copyout(PTEntriesPtr, void *, void *, usize);
void virtual_memory_init(VirtualMemoryTable *);
void init_virtual_memory();
void vm_test();

#endif