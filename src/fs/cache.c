#include <common/string.h>
#include <core/arena.h>
#include <core/console.h>
#include <core/physical_memory.h>
#include <fs/cache.h>

static const SuperBlock *sblock;
static const BlockDevice *device;

static SpinLock lock;
static Arena arena;
static ListNode head;
static usize ts_oracle;

void init_bcache(const SuperBlock *_sblock, const BlockDevice *_device) {
    sblock = _sblock;
    device = _device;

    ArenaPageAllocator allocator = {.allocate = kalloc, .free = kfree};
    init_spinlock(&lock, "block cache");
    init_arena(&arena, sizeof(Block), allocator);
    init_list_node(&head);
    ts_oracle = 0;

    // TODO: startup recovery.
}

static void init_block(Block *block) {
    init_sleeplock(&block->lock, "block");
    init_list_node(&block->node);
    init_rc(&block->rc);
    block->block_no = 0;
    block->valid = false;
    memset(block->data, 0, sizeof(block->data));
}

static ALWAYS_INLINE void device_read(Block *block) {
    device->read(block->block_no, block->data);
}

static ALWAYS_INLINE void device_write(Block *block) {
    device->write(block->block_no, block->data);
}

Block *cache_acquire(usize block_no) {
    acquire_spinlock(&lock);

    Block *slot = NULL;
    for (ListNode *cur = head.next; cur != &head; cur = cur->next) {
        Block *b = container_of(cur, Block, node);
        if (b->block_no == block_no) {
            slot = b;
            break;
        }

        if (!slot && b->rc.count == 0)
            slot = b;
    }

    if (!slot) {
        slot = alloc_object(&arena);
        init_block(slot);
        merge_list(&head, &slot->node);
    }

    release_spinlock(&lock);
    acquire_sleeplock(&slot->lock);

    if (!slot->valid) {
        device_read(slot);
        slot->valid = true;
    }

    increment_rc(&slot->rc);
    return slot;
}

void cache_release(Block *block) {
    decrement_rc(&block->rc);
    release_sleeplock(&block->lock);
}

void cache_begin_op(OpContext *ctx) {
    // TODO: wait for log reservation.
    acquire_spinlock(&lock);
    ctx->ts = ++ts_oracle;
    ctx->num_blocks = 0;
    memset(ctx->block_no, 0, sizeof(ctx->block_no));
    release_spinlock(&lock);
}

void cache_end_op(OpContext *ctx) {}

void cache_sync(OpContext *ctx, Block *block) {
    if (ctx) {
        // TODO
    } else
        device_write(block);
}

usize cache_alloc(OpContext *ctx) {
    for (usize i = 0; i < sblock->num_blocks; i++) {}

    PANIC("cache_alloc: no free block");
}

void cache_free(OpContext *ctx, usize block_no) {}

BlockCache bcache = {
    .acquire = cache_acquire,
    .release = cache_release,
    .begin_op = cache_begin_op,
    .end_op = cache_end_op,
    .sync = cache_sync,
    .alloc = cache_alloc,
    .free = cache_free,
};
