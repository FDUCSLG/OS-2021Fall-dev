#pragma once

#include "device.hpp"

static MockBlockDevice mock;
static SuperBlock sblock;
static BlockDevice device;

static void stub_read(usize block_no, u8 *buffer) {
    mock.read(block_no, buffer);
}

static void stub_write(usize block_no, u8 *buffer) {
    mock.write(block_no, buffer);
}

[[maybe_unused]] static void init_mock(usize log_size, usize num_data_blocks) {
    sblock.log_start = 2;
    sblock.inode_start = sblock.log_start + 1 + log_size;
    sblock.bitmap_start = sblock.inode_start + 1;
    sblock.num_inodes = 0;
    sblock.num_log_blocks = 1 + log_size;
    sblock.num_data_blocks = num_data_blocks;
    sblock.num_blocks = 1 + 1 + 1 + log_size + 1 +
                        ((num_data_blocks + BIT_PER_BLOCK - 1) / BIT_PER_BLOCK) + num_data_blocks;

    mock.initialize(sblock);

    device.read = stub_read;
    device.write = stub_write;
}
