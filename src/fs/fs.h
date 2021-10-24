#pragma once

#include <common/defines.h>
#include <fs/block.h>

/**
 * this file contains on-disk representations of primitives in our filesystem.
 */

#define INODE_NUM_DIRECT   12
#define INODE_NUM_INDIRECT (BLOCK_SIZE / sizeof(u32))
#define INODE_PER_BLOCK    (BLOCK_SIZE / sizeof(InodeEntry))
#define INODE_MAX_BLOCKS   (INODE_NUM_DIRECT + INODE_NUM_INDIRECT)
#define INODE_MAX_BYTES    (INODE_MAX_BLOCKS * BLOCK_SIZE)

// the maximum length of file names, including trailing '\0'.
#define FILE_NAME_MAX_LENGTH 14

// disk layout:
// [ MBR block | super block | log blocks | inode blocks | bitmap blocks | data blocks ]
//
// `mkfs` generates the super block and builds an initial filesystem. The
// super block describes the disk layout.
typedef struct {
    u32 num_blocks;  // total number of blocks in filesystem.
    u32 num_data_blocks;
    u32 num_inodes;
    u32 num_log_blocks;  // number of blocks for logging, including log header.
    u32 log_start;       // the first block of logging area.
    u32 inode_start;     // the first block of inode area.
    u32 bitmap_start;    // the first block of bitmap area.
} SuperBlock;

typedef enum : u16 {
    INODE_INVALID = 0,
    INODE_DIRECTORY = 1,
    INODE_REGULAR = 2,  // regular file
    INODE_DEVICE = 3,
} InodeType;

// `type == INODE_INVALID` implies this inode is free.
typedef struct {
    InodeType type;
    u16 major;                    // major device id, for INODE_DEVICE only.
    u16 minor;                    // minor device id, for INODE_DEVICE only.
    u16 num_links;                // number of hard links to this inode in the filesystem.
    u16 num_bytes;                // number of bytes in the file, i.e. the size of file.
    u32 addrs[INODE_NUM_DIRECT];  // direct addresses/block numbers.
    u32 indirect;                 // the indirect address block.
} InodeEntry;

// the block pointed by `InodeEntry.indirect`.
typedef struct {
    u32 addrs[INODE_NUM_INDIRECT];
} IndirectBlock;

// directory entry. `inode_no == 0` implies this entry is free.
typedef struct {
    u16 inode_no;
    char name[FILE_NAME_MAX_LENGTH];
} DirEntry;
