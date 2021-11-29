#include <common/bitmap.h>
#include <common/string.h>
#include <core/arena.h>
#include <core/console.h>
#include <core/physical_memory.h>
#include <core/proc.h>
#include <fs/cache.h>

static const SuperBlock *sblock;
static const BlockDevice *device;

static SpinLock lock;     // protects block cache.
static Arena arena;       // memory pool for `Block` struct.
static ListNode head;     // the list of all allocated in-memory block.
static LogHeader header;  // in-memory copy of log header block.

// hint: you may need some other variables. Just add them here.

// read the content from disk.
static INLINE void device_read(Block *block) {
    device->read(block->block_no, block->data);
}

// write the content back to disk.
static INLINE void device_write(Block *block) {
    device->write(block->block_no, block->data);
}

// read log header from disk.
static INLINE void read_header() {
    device->read(sblock->log_start, (u8 *)&header);
}

// write log header back to disk.
static INLINE void write_header() {
    device->write(sblock->log_start, (u8 *)&header);
}

// initialize block cache.
void init_bcache(const SuperBlock *_sblock, const BlockDevice *_device) {
    sblock = _sblock;
    device = _device;

    // TODO
}

// initialize a block struct.
static void init_block(Block *block) {
    block->block_no = 0;
    init_list_node(&block->node);
    block->acquired = false;
    block->pinned = false;

    init_sleeplock(&block->lock, "block");
    block->valid = false;
    memset(block->data, 0, sizeof(block->data));
}

// see `cache.h`.
static usize get_num_cached_blocks() {
    // TODO
}

// see `cache.h`.
static Block *cache_acquire(usize block_no) {
    // TODO
}

// see `cache.h`.
static void cache_release(Block *block) {
    // TODO
}

// see `cache.h`.
static void cache_begin_op(OpContext *ctx) {
    // TODO
}

// see `cache.h`.
static void cache_sync(OpContext *ctx, Block *block) {
    if (ctx) {
        // TODO
    } else
        device_write(block);
}

// see `cache.h`.
static void cache_end_op(OpContext *ctx) {
    // TODO
}

// see `cache.h`.
// hint: you can use `cache_acquire`/`cache_sync` to read/write blocks.
static usize cache_alloc(OpContext *ctx) {
    // TODO

    PANIC("cache_alloc: no free block");
}

// see `cache.h`.
// hint: you can use `cache_acquire`/`cache_sync` to read/write blocks.
static void cache_free(OpContext *ctx, usize block_no) {
    // TODO
}

BlockCache bcache = {
    .get_num_cached_blocks = get_num_cached_blocks,
    .acquire = cache_acquire,
    .release = cache_release,
    .begin_op = cache_begin_op,
    .sync = cache_sync,
    .end_op = cache_end_op,
    .alloc = cache_alloc,
    .free = cache_free,
};
