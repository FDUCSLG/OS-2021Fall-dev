extern "C" {
#include <fs/inode.h>
}

#include <thread>

#include "assert.hpp"
#include "mock/cache.hpp"
#include "pause.hpp"
#include "runner.hpp"

int init() {
    init_inodes(&sblock, &cache);
    return 0;
}

int alloc() {
    init_inodes(&sblock, &cache);

    assert_eq(mock->count_inodes(), 1ull);

    OpContext _ctx, *ctx = &_ctx;
    mock->begin_op(ctx);
    usize ino = inodes.alloc(ctx, INODE_REGULAR);

    assert_eq(mock->count_inodes(), 1ull);
    mock->end_op(ctx);

    mock->fence();
    assert_eq(mock->count_inodes(), 2ull);

    auto *p = inodes.get(ino);

    inodes.lock(p);
    inodes.unlock(p);

    mock->begin_op(ctx);
    inodes.put(ctx, p);

    assert_eq(mock->count_inodes(), 2ull);
    mock->end_op(ctx);

    mock->fence();
    assert_eq(mock->count_inodes(), 1ull);

    return 0;
}

int main() {
    Runner runner({
        {"init", init},
        {"alloc", alloc},
    });

    setup_instance();
    runner.run();

    return 0;
}
