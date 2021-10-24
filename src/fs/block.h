#pragma once

#include <common/defines.h>
#include <common/rc.h>
#include <common/spinlock.h>
#include <fs/log.h>

#define BLOCK_SIZE 512

typedef enum : u64 {
    BLOCK_VALID = 1 << 0,
    BLOCK_DIRTY = 1 << 1,
} BlockFlags;

struct BlockCache;

typedef struct {
    struct BlockCache *parent;
    SpinLock lock;
    RefCount rc;
    BlockFlags flags;
    usize block_no;
    u8 data[BLOCK_SIZE];
} Block;

typedef struct BlockCache {
    // allocate a new zero-initialized block.
    // block number is returned.
    usize (*alloc)(OpContext *ctx);

    // mark block at `block_no` is free.
    void (*free)(OpContext *ctx, usize block_no);

    // lock the block at `block_no` increment its reference count by one.
    Block *(*acquire)(usize block_no);

    // unlock `block` and decrement its reference count by one.
    void (*release)(Block *block);

    // begin a new atomic operation and initialize `ctx`.
    // `OpContext` represents an outstanding atomic operation. You can mark the
    // end of atomic operation by `end_op`.
    void (*begin_op)(OpContext *ctx);

    // end the atomic operation managed by `ctx`.
    void (*end_op)(OpContext *ctx);

    // synchronize the content of `block` to disk.
    // `ctx` can be NULL, which indicates this operation does not belong to any
    // atomic operation.
    void (*sync)(OpContext *ctx, Block *block);
} BlockCache;
