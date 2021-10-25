#pragma once

extern "C" {
#include <fs/inode.h>
}

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <random>
#include <unordered_map>

#include "../exception.hpp"

struct MockBlockCache {
    static constexpr usize num_blocks = 2000;
    static constexpr usize inode_start = 200;
    static constexpr usize block_start = 1000;

    static auto get_sblock() -> SuperBlock {
        SuperBlock sblock;
        sblock.num_blocks = num_blocks;
        sblock.num_data_blocks = num_blocks - block_start;
        sblock.num_inodes = 1000;
        sblock.num_log_blocks = 50;
        sblock.log_start = 100;
        sblock.inode_start = inode_start;
        sblock.bitmap_start = 2;
        return sblock;
    }

    struct Meta {
        std::mutex mutex;
        bool used;

        auto operator=(const Meta &rhs) -> Meta & {
            used = rhs.used;
            return *this;
        }
    };

    struct Cell {
        usize index;
        std::mutex mutex;
        Block block;

        auto operator=(const Cell &rhs) -> Cell & {
            block = rhs.block;
            return *this;
        }

        void zero() {
            for (usize i = 0; i < BLOCK_SIZE; i++) {
                block.data[i] = 0;
            }
        }

        void random(std::mt19937 &gen) {
            for (usize i = 0; i < BLOCK_SIZE; i++) {
                block.data[i] = gen() & 0xff;
            }
        }
    };

    std::mutex mutex;
    std::condition_variable cv;
    std::atomic<usize> oracle, top;
    std::unordered_map<usize, bool> board;
    Meta tmpv[num_blocks], memv[num_blocks];
    Cell tmp[num_blocks], mem[num_blocks];

    MockBlockCache() {
        oracle.store(1);
        top.store(0);

        // fill disk with junk.
        std::mt19937 gen(0x19260817);
        for (usize i = 0; i < num_blocks; i++) {
            tmpv[i].used = false;
            tmp[i].index = i;
            tmp[i].random(gen);
            memv[i].used = false;
            mem[i].index = i;
            mem[i].random(gen);
        }

        // mock superblock.
        auto sblock = get_sblock();
        u8 *buf = reinterpret_cast<u8 *>(&sblock);
        for (usize i = 0; i < sizeof(sblock); i++) {
            mem[1].block.data[i] = buf[i];
        }

        // mock root inode.
        InodeEntry node[2];  // node[0] is reserved.
        node[1].type = INODE_DIRECTORY;
        node[1].major = 0;
        node[1].minor = 0;
        node[1].num_links = 1;
        node[1].num_bytes = 0;
        for (usize i = 0; i < INODE_NUM_DIRECT; i++) {
            node[1].addrs[i] = 0;
        }
        node[1].indirect = 0;
        buf = reinterpret_cast<u8 *>(&node);
        for (usize i = 0; i < sizeof(node); i++) {
            mem[inode_start].block.data[i] = buf[i];
        }
    }

    void check_block_no(usize i) {
        if (i >= num_blocks)
            throw Panic("block number out of range");
    }

    auto check_and_get_cell(Block *b) -> Cell * {
        Cell *p = container_of(b, Cell, block);
        isize i = p - tmp;
        if (i < 0 || static_cast<usize>(i) >= num_blocks)
            throw Panic("block is not managed by cache");

        return p;
    }

    void begin_op(OpContext *ctx) {
        std::unique_lock lock(mutex);
        ctx->id = oracle.fetch_add(1);
        board[ctx->id] = false;
    }

    void end_op(OpContext *ctx) {
        std::unique_lock lock(mutex);
        board[ctx->id] = true;

        bool do_checkpoint = true;
        for (const auto &e : board) {
            do_checkpoint &= e.second;
        }

        if (do_checkpoint) {
            for (usize i = 0; i < num_blocks; i++) {
                std::scoped_lock guard(tmpv[i].mutex, memv[i].mutex);
                memv[i] = tmpv[i];
            }
            for (usize i = 0; i < num_blocks; i++) {
                std::scoped_lock guard(tmp[i].mutex, mem[i].mutex);
                mem[i] = tmp[i];
            }

            usize max_oracle = 0;
            for (const auto &e : board) {
                max_oracle = std::max(max_oracle, e.first);
            }
            top.store(max_oracle);
            board.clear();

            cv.notify_all();
        } else
            cv.wait(lock, [&] { return ctx->id <= top.load(); });
    }

    auto alloc(OpContext *ctx) -> usize {
        for (usize i = block_start; i < num_blocks; i++) {
            std::scoped_lock guard(tmpv[i].mutex, memv[i].mutex);
            tmpv[i] = memv[i];

            if (!tmpv[i].used) {
                tmpv[i].used = true;
                if (!ctx)
                    memv[i] = tmpv[i];

                std::scoped_lock guard(tmp[i].mutex, mem[i].mutex);
                tmp[i] = mem[i];
                tmp[i].zero();
                if (!ctx)
                    mem[i] = tmp[i];

                return i;
            }
        }

        throw Panic("no free block");
    }

    void free(OpContext *ctx, usize i) {
        check_block_no(i);

        std::scoped_lock guard(tmpv[i].mutex, memv[i].mutex);
        tmpv[i] = memv[i];
        if (!tmpv[i].used)
            throw Panic("free unused block");

        tmpv[i].used = false;
        if (!ctx)
            memv[i] = tmpv[i];
    }

    auto acquire(usize i) -> Block * {
        check_block_no(i);

        tmp[i].mutex.lock();

        {
            std::scoped_lock guard(mem[i].mutex);
            tmp[i] = mem[i];
        }

        return &tmp[i].block;
    }

    void release(Block *b) {
        auto *p = check_and_get_cell(b);
        p->mutex.unlock();
    }

    void sync(OpContext *ctx, Block *b) {
        if (!ctx) {
            auto *p = check_and_get_cell(b);
            usize i = p->index;

            std::scoped_lock guard(mem[i].mutex);
            mem[i] = tmp[i];
        }
    }
};

namespace {
#include "cache.ipp"
}  // namespace
