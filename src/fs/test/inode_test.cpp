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
    assert_eq(mock.count_inodes(), 1ull);
    assert_eq(mock.count_blocks(), 0);

    return 0;
}

namespace adhoc {

static OpContext _ctx, *ctx = &_ctx;

int test_alloc() {
    mock.begin_op(ctx);
    usize ino = inodes.alloc(ctx, INODE_REGULAR);

    assert_eq(mock.count_inodes(), 1);
    mock.end_op(ctx);

    mock.fence();
    assert_eq(mock.count_inodes(), 2);

    auto *p = inodes.get(ino);

    inodes.lock(p);
    inodes.unlock(p);

    mock.begin_op(ctx);
    inodes.put(ctx, p);

    assert_eq(mock.count_inodes(), 2);
    mock.end_op(ctx);

    mock.fence();
    assert_eq(mock.count_inodes(), 1);

    return 0;
}

int test_sync() {
    auto *p = inodes.get(1);
    assert_eq(p->entry.type, INODE_DIRECTORY);
    p->entry.major = 0x19;
    p->entry.minor = 0x26;
    p->entry.indirect = 0xa817;

    mock.begin_op(ctx);
    inodes.lock(p);
    inodes.sync(ctx, p, true);
    inodes.unlock(p);
    inodes.put(ctx, p);
    mock.end_op(ctx);

    mock.fence();
    auto *q = mock.inspect(1);
    assert_eq(q->type, INODE_DIRECTORY);
    assert_eq(q->major, 0x19);
    assert_eq(q->minor, 0x26);
    assert_eq(q->indirect, 0xa817);

    return 0;
}

int test_share() {
    mock.begin_op(ctx);
    usize ino = inodes.alloc(ctx, INODE_REGULAR);
    mock.end_op(ctx);

    mock.fence();
    assert_eq(mock.count_inodes(), 2);

    auto *p = inodes.get(ino);
    auto *q = inodes.share(p);

    mock.begin_op(ctx);
    inodes.put(ctx, p);
    mock.end_op(ctx);

    mock.fence();
    assert_eq(mock.count_inodes(), 2);

    mock.begin_op(ctx);
    inodes.put(ctx, q);
    assert_eq(mock.count_inodes(), 2);
    mock.end_op(ctx);

    mock.fence();
    assert_eq(mock.count_inodes(), 1);

    return 0;
}

int test_small_file() {
    mock.begin_op(ctx);
    usize ino = inodes.alloc(ctx, INODE_REGULAR);
    mock.end_op(ctx);

    u8 buf[1];
    auto *p = inodes.get(ino);
    inodes.lock(p);

    buf[0] = 0xcc;
    inodes.read(p, buf, 0, 0);
    assert_eq(buf[0], 0xcc);

    mock.begin_op(ctx);
    inodes.write(ctx, p, buf, 0, 1);
    assert_eq(mock.count_blocks(), 0);
    mock.end_op(ctx);

    mock.fence();
    auto *q = mock.inspect(ino);
    assert_eq(q->indirect, 0);
    assert_ne(q->addrs[0], 0);
    assert_eq(q->addrs[1], 0);
    assert_eq(q->num_bytes, 1);
    assert_eq(mock.count_blocks(), 1);

    mock.fill_junk();
    buf[0] = 0;
    inodes.read(p, buf, 0, 1);
    assert_eq(buf[0], 0xcc);

    inodes.unlock(p);

    inodes.lock(p);

    mock.begin_op(ctx);
    inodes.clear(ctx, p);
    mock.end_op(ctx);

    mock.fence();
    q = mock.inspect(ino);
    assert_eq(q->indirect, 0);
    assert_eq(q->addrs[0], 0);
    assert_eq(q->num_bytes, 0);
    assert_eq(mock.count_blocks(), 0);

    inodes.unlock(p);

    mock.begin_op(ctx);
    inodes.put(ctx, p);
    mock.end_op(ctx);

    mock.fence();
    assert_eq(mock.count_inodes(), 1);

    return 0;
}

}  // namespace adhoc

int main() {
    if (Runner::run({"init", test_init}))
        init_inodes(&sblock, &cache);

    std::vector<Testcase> tests = {
        {"alloc", adhoc::test_alloc},
        {"sync", adhoc::test_sync},
        {"share", adhoc::test_share},
        {"small_file", adhoc::test_small_file},
    };
    Runner(tests).run();

    return 0;
}
