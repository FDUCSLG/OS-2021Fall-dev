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
    assert_eq(mock.count_inodes(), 1);
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
    auto *r = inodes.get(ino);

    assert_eq(r->rc.count, 3);

    mock.begin_op(ctx);
    inodes.put(ctx, p);
    assert_eq(q->rc.count, 2);
    mock.end_op(ctx);

    mock.fence();
    assert_eq(mock.count_inodes(), 2);

    mock.begin_op(ctx);
    inodes.put(ctx, q);
    assert_eq(r->rc.count, 1);
    assert_eq(mock.count_inodes(), 2);
    inodes.put(ctx, r);
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

int test_large_file() {
    mock.begin_op(ctx);
    usize ino = inodes.alloc(ctx, INODE_REGULAR);
    mock.end_op(ctx);

    constexpr usize max_size = 65535;
    u8 buf[max_size], copy[max_size];
    std::mt19937 gen(0x12345678);
    for (usize i = 0; i < max_size; i++) {
        copy[i] = buf[i] = gen() & 0xff;
    }

    auto *p = inodes.get(ino);

    inodes.lock(p);
    for (usize i = 0, n = 0; i < max_size; i += n) {
        n = std::min(static_cast<usize>(gen() % 10000), max_size - i);

        mock.begin_op(ctx);
        inodes.write(ctx, p, buf + i, i, n);
        auto *q = mock.inspect(ino);
        assert_eq(q->num_bytes, i);
        mock.end_op(ctx);

        mock.fence();
        assert_eq(q->num_bytes, i + n);
    }
    inodes.unlock(p);

    for (usize i = 0; i < max_size; i++) {
        buf[i] = 0;
    }

    inodes.lock(p);
    inodes.read(p, buf, 0, max_size);
    inodes.unlock(p);

    for (usize i = 0; i < max_size; i++) {
        assert_eq(buf[i], copy[i]);
    }

    inodes.lock(p);
    mock.begin_op(ctx);
    inodes.clear(ctx, p);
    inodes.unlock(p);
    mock.end_op(ctx);

    mock.fence();
    assert_eq(mock.count_inodes(), 2);
    assert_eq(mock.count_blocks(), 0);

    for (usize i = 0; i < max_size; i++) {
        copy[i] = buf[i] = gen() & 0xff;
    }

    inodes.lock(p);
    mock.begin_op(ctx);
    inodes.write(ctx, p, buf, 0, max_size);
    mock.end_op(ctx);
    inodes.unlock(p);

    mock.fence();
    auto *q = mock.inspect(ino);
    assert_eq(q->num_bytes, max_size);

    for (usize i = 0; i < max_size; i++) {
        buf[i] = 0;
    }

    inodes.lock(p);
    for (usize i = 0, n = 0; i < max_size; i += n) {
        n = std::min(static_cast<usize>(gen() % 10000), max_size - i);
        inodes.read(p, buf + i, i, n);
        for (usize j = 0; j < i + n; j++) {
            assert_eq(buf[j], copy[j]);
        }
    }
    inodes.unlock(p);

    mock.begin_op(ctx);
    inodes.put(ctx, p);
    mock.end_op(ctx);

    mock.fence();
    assert_eq(mock.count_inodes(), 1);
    assert_eq(mock.count_blocks(), 0);

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
        {"large_file", adhoc::test_large_file},
    };
    Runner(tests).run();

    return 0;
}
