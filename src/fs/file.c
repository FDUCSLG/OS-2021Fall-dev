/* File descriptors */

#include "file.h"
#include "fs.h"
#include <common/defines.h>
#include <common/spinlock.h>
#include <core/console.h>
#include <core/sleeplock.h>
#include <fs/inode.h>

// struct devsw devsw[NDEV];
struct {
    struct SpinLock lock;
    struct file file[NFILE];
} ftable;

/* Optional since BSS is zero-initialized. */
void fileinit() {
    init_spinlock(&ftable.lock, "file table");
}

/* Allocate a file structure. */
struct file *filealloc() {
    struct file *f;

    acquire_spinlock(&ftable.lock);
    for (f = ftable.file; f < ftable.file + NFILE; f++) {
        if (f->ref == 0) {
            f->ref = 1;
            release_spinlock(&ftable.lock);
            return f;
        }
    }
    release_spinlock(&ftable.lock);
    return 0;
}

/* Increment ref count for file f. */
struct file *filedup(struct file *f) {
    acquire_spinlock(&ftable.lock);
    if (f->ref < 1)
        PANIC("filedup");
    f->ref++;
    release_spinlock(&ftable.lock);
    return f;
}

/* Close file f. (Decrement ref count, close when reaches 0.) */
void fileclose(struct file *f) {
    struct file ff;

    acquire_spinlock(&ftable.lock);
    if (f->ref < 1)
        PANIC("fileclose");
    if (--f->ref > 0) {
        release_spinlock(&ftable.lock);
        return;
    }
    ff = *f;
    f->ref = 0;
    f->type = FD_NONE;
    release_spinlock(&ftable.lock);

    if (ff.type == FD_PIPE)
        ;  // pipeclose(ff.pipe, ff.writable);
    else if (ff.type == FD_INODE) {
        OpContext ctx;
        bcache.begin_op(&ctx);
        inodes.put(&ctx, ff.ip);
        bcache.end_op(&ctx);
    }
}

/* Get metadata about file f. */
int filestat(struct file *f, struct stat *st) {
    if (f->type == FD_INODE) {
        inodes.lock(f->ip);
        stati(f->ip, st);
        inodes.unlock(f->ip);
        return 0;
    }
    return -1;
}

/* Read from file f. */
isize fileread(struct file *f, char *addr, isize n) {
    isize r;

    if (f->readable == 0)
        return -1;

    if (f->type == FD_PIPE) {
        // return piperead(f->pipe, addr, n);
    }

    if (f->type == FD_INODE) {
        inodes.lock(f->ip);
        r = (isize)inodes.read(f->ip, (u8 *)addr, f->off, (usize)n);
        f->off += (u64)r;
        inodes.unlock(f->ip);
        return r;
    }
    PANIC("fileread");
    return -1;
}

/* Write to file f. */
isize filewrite(struct file *f, char *addr, isize n) {
    isize r;

    if (f->writable == 0)
        return -1;
    // if (f->type == FD_PIPE)
    //     return pipewrite(f->pipe, addr, n);
    if (f->type == FD_INODE) {
        /*
         * Write a few blocks at a time to avoid exceeding
         * the maximum log transaction size, including
         * i-node, indirect block, allocation blocks,
         * and 2 blocks of slop for non-aligned writes.
         * This really belongs lower down, since writei()
         * might be writing a device like the console.
         */
        isize max = ((INODE_MAX_BLOCKS - 1 - 1 - 2) / 2) * 512;
        isize i = 0;
        while (i < n) {
            isize n1 = n - i;
            if (n1 > max)
                n1 = max;

            OpContext ctx;
            bcache.begin_op(&ctx);
            inodes.lock(f->ip);

            r = (isize)inodes.write(&ctx, f->ip, (u8 *)(addr + i), f->off, (usize)n1);
            f->off += (u64)r;
            inodes.unlock(f->ip);
            bcache.end_op(&ctx);

            if (r < 0)
                break;
            if (r != n1)
                PANIC("short filewrite");
            i += r;
        }
        return i == n ? n : -1;
    }
    PANIC("filewrite");
    return -1;
}
