#pragma once

#include <common/defines.h>
#include <core/sleeplock.h>
#include <fs/defines.h>
#include <fs/fs.h>
#include <fs/inode.h>
#include <sys/stat.h>

#define NFILE 100  // Open files per system

struct file {
    enum { FD_NONE, FD_PIPE, FD_INODE } type;
    int ref;
    char readable;
    char writable;
    struct pipe *pipe;
    Inode *ip;
    usize off;
};

/* In-memory copy of an inode. */
// struct inode {
//     u32 dev;             // Device number
//     u32 inum;            // Inode number
//     int ref;                  // Reference count
//     struct SleepLock lock;    // Protects everything below here
//     int valid;                // Inode has been read from disk?

//     u16 type;            // Copy of disk inode
//     u16 major;
//     u16 minor;
//     u16 nlink;
//     u32 size;
//     u32 addrs[INODE_NUM_DIRECT+1];
// };

/*
 * Table mapping major device number to
 * device functions
 */
struct devsw {
    isize (*read)(Inode *, char *, isize);
    isize (*write)(Inode *, char *, isize);
};

extern struct devsw devsw[];

// void readsb(int, struct SuperBlock *);
// int             inodes.insert(&ctx, struct inode*, char*, u32);
// int             dirunlink(Inode *, char *, u32);
// Inode *  dirlookup(Inode *, char *, usize *);
// Inode *  ialloc(u32, short);
// Inode *  idup(Inode *);
// void            iinit(int dev);
// void            ilock(Inode *);
// void            iput(Inode *);
// void            inodes.unlock(Inode *);
// void            iunlockput(Inode *);
// void            iupdate(Inode *);
// int             namecmp(const char *, const char *);
// Inode *  namei(char *);
// Inode *  nameiparent(char *, char *);
// void            stati(Inode *, struct stat *);
// isize         inodes.read(Inode *, char *, usize, usize);
// isize         writei(Inode *, char *, usize, usize);

void fileinit();
struct file *filealloc();
struct file *filedup(struct file *f);
void fileclose(struct file *f);
int filestat(struct file *f, struct stat *st);
isize fileread(struct file *f, char *addr, isize n);
isize filewrite(struct file *f, char *addr, isize n);

int sys_dup();
isize sys_read();
isize sys_write();
isize sys_writev();
int sys_close();
int sys_fstat();
int sys_fstatat();
int sys_openat();
int sys_mkdirat();
int sys_mknodat();
int sys_chdir();
int sys_exec();