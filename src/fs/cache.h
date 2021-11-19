#pragma once

#include <common/defines.h>
#include <common/list.h>
#include <common/rc.h>
#include <core/sleeplock.h>
#include <fs/block_device.h>
#include <fs/fs.h>

#define MAX_OP_SIZE 12

typedef struct {
    SleepLock lock;
    ListNode node;
    RefCount rc;
    usize block_no;
    bool valid;
    u8 data[BLOCK_SIZE];
} Block;

typedef struct {
    usize ts;
    usize num_blocks;
    usize block_no[MAX_OP_SIZE];
} OpContext;

typedef struct BlockCache {
    // read the block at `block_no` from disk, and lock the block, then
    // incrementing its reference count by one.
    Block *(*acquire)(usize block_no);

    // unlock `block` and decrement its reference count by one.
    // `release` does not have to write the block back to disk.
    void (*release)(Block *block);

    // begin a new atomic operation and initialize `ctx`.
    // `OpContext` represents an outstanding atomic operation. You can mark the
    // end of atomic operation by `end_op`.
    void (*begin_op)(OpContext *ctx);

    // end the atomic operation managed by `ctx`.
    // it returns when all associated blocks are synchronized to disk.
    void (*end_op)(OpContext *ctx);

    // synchronize the content of `block` to disk.
    // `ctx` can be NULL, which indicates this operation does not belong to any
    // atomic operation and it immediately writes all content back to disk. However
    // this is very dangerous, since it may break atomicity of concurrent atomic
    // operations. YOU SHOULD USE THIS MODE WITH CARE.
    // if `ctx` is not NULL, the actual writeback is delayed until `end_op`.
    void (*sync)(OpContext *ctx, Block *block);

    // NOTE: every block on disk has a bit in bitmap area, including blocks inside
    // bitmap!
    // usually, MBR block, super block, inode blocks, log blocks and bitmap blocks
    // are preallocated on disk, i.e. those bits for them are already set in bitmap.
    // therefore when we allocate a new block, it usually returns a data block.
    // nobody can prevent you freeing a non-data block :)

    // allocate a new zero-initialized block.
    // block number is returned.
    usize (*alloc)(OpContext *ctx);

    // mark block at `block_no` is free in bitmap.
    void (*free)(OpContext *ctx, usize block_no);
} BlockCache;

extern BlockCache bcache;

void init_bcache(const SuperBlock *sblock, const BlockDevice *device);
