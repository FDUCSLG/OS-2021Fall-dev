extern "C" {
#include <fs/inode.h>
}

#include <thread>

#include "assert.hpp"
#include "mock/cache.hpp"
#include "pause.hpp"
#include "runner.hpp"

int test_init() {
    init_inodes(&sblock, &cache);
    assert_eq(mock->count_inodes(), 1ull);

    return 0;
}

int test_alloc() {
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

int test_sync() {
    return 0;
}

int main() {
    setup_instance();

    if (Runner::run({"init", test_init}))
        init_inodes(&sblock, &cache);

    std::vector<Testcase> tests = {
        {"alloc", test_alloc},
        {"sync", test_sync},
    };
    Runner(tests).run();

    return 0;
}
