#pragma once

#ifndef _CORE_MEMORY_MANAGE_
#define _CORE_MEMORY_MANAGE_

typedef struct {
    void *memmory_manager;
    void (*page_init)(void *);
    void *(*page_alloc)(void *);
    void (*page_free)(void *, void *);
} MemmoryManagerTable;

typedef struct  {
    void *next;
    void *start, *end;
} FreeList;

void init_memory_manager();
void free_range(void *, void *);
void *kalloc();
void kfree(void *);

#endif