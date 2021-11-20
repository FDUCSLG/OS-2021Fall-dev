#include <common/bitmap.h>
#include <common/string.h>
#include <core/arena.h>
#include <core/console.h>
#include <core/physical_memory.h>
#include <core/proc.h>
#include <fs/cache.h>

static const SuperBlock *sblock;
static const BlockDevice *device;

// TODO: add comments here.
static SpinLock lock;
static Arena arena;
static ListNode head;
static usize ts_oracle;
static usize log_start, log_size, log_used;
static usize op_count;
static LogHeader header;

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

    ArenaPageAllocator allocator = {.allocate = kalloc, .free = kfree};
    init_spinlock(&lock, "block cache");
    init_arena(&arena, sizeof(Block), allocator);
    init_list_node(&head);
    ts_oracle = 0;
    log_start = sblock->log_start + 1;
    log_size = MIN(sblock->num_log_blocks - 1, LOG_MAX_SIZE);
    log_used = 0;
    op_count = 0;

    read_header();

    // TODO: startup recovery.
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
Block *cache_acquire(usize block_no) {
    acquire_spinlock(&lock);

    Block *slot = NULL;
    for (ListNode *cur = head.next; cur != &head; cur = cur->next) {
        Block *block = container_of(cur, Block, node);
        if (block->block_no == block_no) {
            slot = block;
            break;
        }

        // find a candidate to be evicted.
        if (!slot && !block->acquired && !block->pinned)
            slot = block;
    }

    // when not found and no block can be evicted.
    if (!slot) {
        slot = alloc_object(&arena);
        assert(slot != NULL);
        init_block(slot);
        merge_list(&head, &slot->node);
    }

    // NOTE: set `acquired` before releasing cache lock to prevent someone evicting
    // this block in the window between `release` and `acquire`.
    slot->acquired = true;
    release_spinlock(&lock);
    acquire_sleeplock(&slot->lock);

    if (!slot->valid) {
        device_read(slot);
        slot->valid = true;
    }

    return slot;
}

// see `cache.h`.
void cache_release(Block *block) {
    release_sleeplock(&block->lock);
    acquire_spinlock(&lock);
    block->acquired = false;
    release_spinlock(&lock);
}

// see `cache.h`.
void cache_begin_op(OpContext *ctx) {
    acquire_spinlock(&lock);
    while (log_used + OP_MAX_NUM_BLOCKS > log_size) {
        sleep(&lock, &lock);
    }
    log_used += OP_MAX_NUM_BLOCKS;
    ctx->ts = ++ts_oracle;
    op_count++;
    release_spinlock(&lock);

    ctx->num_blocks = 0;
    memset(ctx->block_no, 0, sizeof(ctx->block_no));
}

// see `cache.h`.
void cache_end_op(OpContext *ctx) {
    acquire_spinlock(&lock);
    op_count--;
    release_spinlock(&lock);
}

// see `cache.h`.
void cache_sync(OpContext *ctx, Block *block) {
    if (ctx) {
        usize i = 0;
        for (; i < ctx->num_blocks; i++) {
            if (ctx->block_no[i] == block->block_no)
                break;
        }

        assert(i < OP_MAX_NUM_BLOCKS);
        ctx->block_no[i] = block->block_no;

        acquire_spinlock(&lock);
        block->pinned = true;
        release_spinlock(&lock);
    } else
        device_write(block);
}

// see `cache.h`.
// hint: you can use `cache_acquire`/`cache_sync` to read/write blocks.
usize cache_alloc(OpContext *ctx) {
    for (usize i = 0; i < sblock->num_blocks; i += BITS_PER_BLOCK) {
        usize block_no = sblock->bitmap_start + (i / BITS_PER_BLOCK);
        Block *block = cache_acquire(block_no);

        BitmapCell *bitmap = (BitmapCell *)block->data;
        for (usize j = 0; j < BITS_PER_BLOCK && i + j < sblock->num_blocks; j++) {
            if (!bitmap_get(bitmap, j)) {
                bitmap_set(bitmap, j);
                cache_sync(ctx, block);
                cache_release(block);
                return i + j;
            }
        }

        cache_release(block);
    }

    PANIC("cache_alloc: no free block");
}

// see `cache.h`.
// hint: you can use `cache_acquire`/`cache_sync` to read/write blocks.
void cache_free(OpContext *ctx, usize block_no) {
    usize i = block_no / BITS_PER_BLOCK, j = block_no % BITS_PER_BLOCK;
    Block *block = cache_acquire(sblock->bitmap_start + i);

    BitmapCell *bitmap = (BitmapCell *)block->data;
    assert(!bitmap_get(bitmap, j));
    bitmap_clear(bitmap, j);

    cache_sync(ctx, block);
    cache_release(block);
}

BlockCache bcache = {
    .acquire = cache_acquire,
    .release = cache_release,
    .begin_op = cache_begin_op,
    .end_op = cache_end_op,
    .sync = cache_sync,
    .alloc = cache_alloc,
    .free = cache_free,
};
