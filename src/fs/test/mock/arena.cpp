#include "map.hpp"

namespace {
Map<struct Arena *, size_t> map;
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

void init_arena(Arena *arena, size_t object_size, ArenaPageAllocator allocator [[maybe_unused]]) {
    map.add(arena, object_size);
}

void *alloc_object(Arena *arena) {
    return malloc(map[arena]);
}

void free_object(void *object) {
    free(object);
}
}
