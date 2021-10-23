#pragma once

#include <common/spinlock.h>

typedef struct {
    void *memmory_manager;
    void (*page_init)(void *this, void *start, void *end);
    void *(*page_alloc)(void *this);
    void (*page_free)(void *this, void *v);
    SpinLock lock;
} MemmoryManagerTable;

typedef struct {
    void *next;
    void *start, *end;
} FreeList;

void init_memory_manager();
void free_range(void *start, void *end);
void *kalloc();
void kfree(void *va);
