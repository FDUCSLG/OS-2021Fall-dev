#include <fs/cache.h>

static const SuperBlock *sblock;

void init_bcache(const SuperBlock *_sblock) {
    sblock = _sblock;
}

void cache_begin_op(OpContext *ctx) {}

void cache_end_op(OpContext *ctx) {}

usize cache_alloc(OpContext *ctx) {}

void cache_free(OpContext *ctx, usize block_no) {}

Block *cache_acquire(usize block_no) {}

void cache_release(Block *block) {}

void cache_sync(OpContext *ctx, Block *block) {}

BlockCache bcache = {
    .begin_op = cache_begin_op,
    .end_op = cache_end_op,
    .alloc = cache_alloc,
    .free = cache_free,
    .acquire = cache_acquire,
    .release = cache_release,
    .sync = cache_sync,
};
