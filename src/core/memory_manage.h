#pragma once

#ifndef _CORE_MEMORY_MANAGE_
#define _CORE_MEMORY_MANAGE_

#include <common/spinlock.h>

typedef struct {
    SpinLock lock;
    void *struct_ptr;
    NORETURN void (*page_init)(void *datastructure_ptr, void *start, void *end);
    void *(*page_alloc)(void *datastructure_ptr);
    NORETURN void (*page_free)(void *datastructure_ptr, void *page_address);
} PMemory;

typedef struct {
    void *next;
} FreeListNode;

NORETURN void init_memory_manager(void);
NORETURN void free_range(void *start, void *end);
void *kalloc(void);
NORETURN void kfree(void *page_address);

#endif