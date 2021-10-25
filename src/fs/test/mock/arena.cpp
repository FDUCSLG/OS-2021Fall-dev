extern "C" {
#include <common/defines.h>
}

#include "map.hpp"

namespace {
Map<struct Arena *, usize> map;
}

extern "C" {
typedef struct {
    void *(*allocate)();
    void (*free)(void *page);
} ArenaPageAllocator;

void *kalloc() {
    return malloc(4096);
}

void kfree(void *ptr) {
    free(ptr);
}

void init_arena(Arena *arena, usize object_size, ArenaPageAllocator allocator [[maybe_unused]]) {
    map.add(arena, object_size);
}

void *alloc_object(Arena *arena) {
    return malloc(map[arena]);
}

void free_object(void *object) {
    free(object);
}
}
