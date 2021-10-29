#include <common/string.h>
#include <core/arena.h>
#include <core/console.h>
#include <core/physical_memory.h>
#include <fs/inode.h>

// this lock mainly prevents concurrent access to inode list `head`, reference
// count increment and decrement.
static SpinLock lock;
static ListNode head;

static const SuperBlock *sblock;
static const BlockCache *cache;
static Arena arena;

// return which block `inode_no` lives on.
static INLINE usize to_block_no(usize inode_no) {
    return sblock->inode_start + (inode_no / (INODE_PER_BLOCK));
}

// return the pointer to on-disk inode.
static INLINE InodeEntry *get_entry(Block *block, usize inode_no) {
    return ((InodeEntry *)block->data) + (inode_no % INODE_PER_BLOCK);
}

// return address array in indirect block.
static INLINE u32 *get_addrs(Block *block) {
    return ((IndirectBlock *)block->data)->addrs;
}

// used by `inode_map`.
static INLINE void set_flag(bool *flag) {
    if (flag != NULL)
        *flag = true;
}

// initialize inode tree.
void init_inodes(const SuperBlock *_sblock, const BlockCache *_cache) {
    ArenaPageAllocator allocator = {.allocate = kalloc, .free = kfree};

    init_spinlock(&lock, "InodeTree");
    init_list_node(&head);
    sblock = _sblock;
    cache = _cache;
    init_arena(&arena, sizeof(Inode), allocator);

    inodes.root = inodes.get(ROOT_INODE_NO);
}

// initialize in-memory inode.
static void init_inode(Inode *inode) {
    init_spinlock(&inode->lock, "Inode");
    init_rc(&inode->rc);
    init_list_node(&inode->node);
    inode->inode_no = 0;
    inode->valid = false;
}

// see `inode.h`.
static usize inode_alloc(OpContext *ctx, InodeType type) {
    assert(type != INODE_INVALID);

    for (usize ino = 1; ino < sblock->num_inodes; ino++) {
        usize block_no = to_block_no(ino);
        Block *block = cache->acquire(block_no);
        InodeEntry *inode = get_entry(block, ino);

        if (inode->type == INODE_INVALID) {
            memset(inode, 0, sizeof(InodeEntry));
            inode->type = type;
            cache->sync(ctx, block);
            cache->release(block);
            return ino;
        }

        cache->release(block);
    }

    PANIC("failed to allocate inode on disk");
}

// see `inode.h`.
static void inode_lock(Inode *inode) {
    assert(inode->rc.count > 0);
    acquire_spinlock(&inode->lock);
}

// see `inode.h`.
static void inode_unlock(Inode *inode) {
    assert(holding_spinlock(&inode->lock));
    assert(inode->rc.count > 0);
    release_spinlock(&inode->lock);
}

// see `inode.h`.
static void inode_sync(OpContext *ctx, Inode *inode, bool do_write) {
    usize block_no = to_block_no(inode->inode_no);
    Block *block = cache->acquire(block_no);
    InodeEntry *entry = get_entry(block, inode->inode_no);

    if (inode->valid && do_write) {
        memcpy(entry, &inode->entry, sizeof(InodeEntry));
        cache->sync(ctx, block);
    } else if (!inode->valid) {
        memcpy(&inode->entry, entry, sizeof(InodeEntry));
        inode->valid = true;
    }

    cache->release(block);
}

// see `inode.h`.
static Inode *inode_get(usize inode_no) {
    assert(inode_no > 0);
    assert(inode_no < sblock->num_inodes);
    acquire_spinlock(&lock);

    Inode *inode = NULL;
    for (ListNode *cur = head.next; cur != &head; cur = cur->next) {
        Inode *inst = container_of(cur, Inode, node);
        if (inst->rc.count > 0 && inst->inode_no == inode_no) {
            increment_rc(&inst->rc);
            inode = inst;
            break;
        }
    }

    if (inode == NULL) {
        inode = alloc_object(&arena);
        assert(inode != NULL);
        init_inode(inode);
        inode->inode_no = inode_no;
        increment_rc(&inode->rc);
        merge_list(&head, &inode->node);
    }

    release_spinlock(&lock);

    inode_lock(inode);
    inode_sync(NULL, inode, false);
    assert(inode->entry.type != INODE_INVALID);
    inode_unlock(inode);
    return inode;
}

// see `inode.h`.
static void inode_clear(OpContext *ctx, Inode *inode) {
    InodeEntry *entry = &inode->entry;

    for (usize i = 0; i < INODE_NUM_DIRECT; i++) {
        usize addr = entry->addrs[i];
        if (addr != 0)
            cache->free(ctx, addr);
    }
    memset(entry->addrs, 0, sizeof(entry->addrs));

    usize iaddr = entry->indirect;
    if (iaddr != 0) {
        Block *block = cache->acquire(iaddr);
        u32 *addrs = get_addrs(block);
        for (usize i = 0; i < INODE_NUM_INDIRECT; i++) {
            if (addrs[i] != 0)
                cache->free(ctx, addrs[i]);
        }

        cache->release(block);
        cache->free(ctx, iaddr);
        entry->indirect = 0;
    }

    entry->num_bytes = 0;
    inode_sync(ctx, inode, true);
}

// see `inode.h`.
static Inode *inode_share(Inode *inode) {
    acquire_spinlock(&lock);
    increment_rc(&inode->rc);
    release_spinlock(&lock);
    return inode;
}

// see `inode.h`.
static void inode_put(OpContext *ctx, Inode *inode) {
    acquire_spinlock(&lock);
    bool is_last = inode->rc.count <= 1;
    release_spinlock(&lock);

    if (is_last && inode->entry.num_links == 0) {
        inode_lock(inode);
        inode_clear(ctx, inode);
        inode->entry.type = INODE_INVALID;
        inode_sync(ctx, inode, true);
        inode_unlock(inode);

        free_object(inode);
    }

    acquire_spinlock(&lock);
    if (decrement_rc(&inode->rc))
        detach_from_list(&inode->node);
    release_spinlock(&lock);
}

// this function is private to inode interface, because it can allocate block
// at arbitrary offset, which breaks the usual file abstraction.
//
// retrieve the block in `inode` where offset lives. If the block is not
// allocated, `inode_map` will allocate a new block and update `inode`. at
// which time, `*modified` will be set to true.
// the block number is returned.
//
// NOTE: caller must hold `inode`'s lock.
static usize inode_map(OpContext *ctx, Inode *inode, usize offset, bool *modified) {
    InodeEntry *entry = &inode->entry;
    usize index = offset / BLOCK_SIZE;

    if (index < INODE_NUM_DIRECT) {
        if (entry->addrs[index] == 0) {
            entry->addrs[index] = cache->alloc(ctx);
            set_flag(modified);
        }

        return entry->addrs[index];
    }

    index -= INODE_NUM_DIRECT;
    assert(index < INODE_NUM_INDIRECT);

    if (entry->indirect == 0) {
        entry->indirect = cache->alloc(ctx);
        set_flag(modified);
    }

    Block *block = cache->acquire(entry->indirect);
    u32 *addrs = get_addrs(block);

    if (addrs[index] == 0) {
        addrs[index] = cache->alloc(ctx);
        cache->sync(ctx, block);
        set_flag(modified);
    }

    usize addr = addrs[index];
    cache->release(block);
    return addr;
}

// see `inode.h`.
static void inode_read(Inode *inode, u8 *dest, usize offset, usize count) {
    InodeEntry *entry = &inode->entry;
    usize end = offset + count;
    assert(offset <= entry->num_bytes);
    assert(end <= entry->num_bytes);
    assert(offset <= end);

    usize step = 0;
    for (usize begin = offset; begin < end; begin += step, dest += step) {
        bool modified = false;
        usize block_no = inode_map(NULL, inode, begin, &modified);
        assert(!modified);

        Block *block = cache->acquire(block_no);
        usize index = begin % BLOCK_SIZE;
        step = MIN(end - begin, BLOCK_SIZE - index);
        memmove(dest, block->data + index, step);
        cache->release(block);
    }
}

// see `inode.h`.
static void inode_write(OpContext *ctx, Inode *inode, u8 *src, usize offset, usize count) {
    InodeEntry *entry = &inode->entry;
    usize end = offset + count;
    assert(offset <= entry->num_bytes);
    assert(end <= INODE_MAX_BYTES);
    assert(offset <= end);

    usize step = 0;
    bool modified = false;
    for (usize begin = offset; begin < end; begin += step, src += step) {
        usize block_no = inode_map(ctx, inode, begin, &modified);
        Block *block = cache->acquire(block_no);
        usize index = begin % BLOCK_SIZE;
        step = MIN(end - begin, BLOCK_SIZE - index);
        memmove(block->data + index, src, step);
        cache->sync(ctx, block);
        cache->release(block);
    }

    if (end > entry->num_bytes) {
        entry->num_bytes = end;
        modified = true;
    }
    if (modified)
        inode_sync(ctx, inode, true);
}

// see `inode.h`.
static usize inode_lookup(Inode *inode, const char *name, usize *index) {
    InodeEntry *entry = &inode->entry;
    assert(entry->type == INODE_DIRECTORY);

    DirEntry dentry;
    for (usize offset = 0; offset < entry->num_bytes; offset += sizeof(dentry)) {
        inode_read(inode, (u8 *)&dentry, offset, sizeof(dentry));
        if (dentry.inode_no != 0 && strncmp(name, dentry.name, FILE_NAME_MAX_LENGTH) == 0) {
            if (index != NULL)
                *index = offset / sizeof(dentry);
            return dentry.inode_no;
        }
    }

    return 0;
}

// see `inode.h`.
static usize inode_insert(OpContext *ctx, Inode *inode, const char *name, usize inode_no) {
    InodeEntry *entry = &inode->entry;
    assert(entry->type == INODE_DIRECTORY);

    DirEntry dentry;
    usize offset = 0;
    for (; offset < entry->num_bytes; offset += sizeof(dentry)) {
        inode_read(inode, (u8 *)&dentry, offset, sizeof(dentry));
        if (dentry.inode_no == 0)
            break;
    }

    dentry.inode_no = inode_no;
    strncpy(dentry.name, name, FILE_NAME_MAX_LENGTH);
    inode_write(ctx, inode, (u8 *)&dentry, offset, sizeof(dentry));
    return offset / sizeof(dentry);
}

// see `inode.h`.
static void inode_remove(OpContext *ctx, Inode *inode, usize index) {
    InodeEntry *entry = &inode->entry;
    assert(entry->type == INODE_DIRECTORY);

    DirEntry dentry;
    usize offset = index * sizeof(dentry);
    if (offset >= entry->num_bytes)
        return;

    memset(&dentry, 0, sizeof(dentry));
    inode_write(ctx, inode, (u8 *)&dentry, offset, sizeof(dentry));
}

InodeTree inodes = {
    .alloc = inode_alloc,
    .lock = inode_lock,
    .unlock = inode_unlock,
    .sync = inode_sync,
    .get = inode_get,
    .clear = inode_clear,
    .share = inode_share,
    .put = inode_put,
    .read = inode_read,
    .write = inode_write,
    .lookup = inode_lookup,
    .insert = inode_insert,
    .remove = inode_remove,
};
