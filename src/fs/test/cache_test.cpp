extern "C" {
#include <fs/cache.h>
}

#include "assert.hpp"
#include "pause.hpp"
#include "runner.hpp"

#include "mock/device.hpp"

namespace basic {

void test_init() {
    initialize(1, 1);
}

void test_read_write() {
    initialize(1, 1);

    auto *b = bcache.acquire(1);
    auto *d = mock.inspect(1);
    assert_eq(b->acquired, true);
    assert_eq(b->block_no, 1);
    assert_eq(b->pinned, false);
    assert_eq(b->valid, true);
    for (usize i = 0; i < BLOCK_SIZE; i++) {
        assert_eq(b->data[i], d[i]);
    }

    u8 value = b->data[128];
    b->data[128] = ~value;
    bcache.sync(NULL, b);
    assert_eq(d[128], ~value);

    bcache.release(b);
    b = bcache.acquire(1);
}

void test_atomic_op() {
    initialize(32, 64);

    OpContext ctx;
    bcache.begin_op(&ctx);

    usize t = sblock.num_blocks - 1;
    auto *b = bcache.acquire(t);
    assert_eq(b->block_no, t);
    assert_eq(b->acquired, true);
    assert_eq(b->pinned, false);
    assert_eq(b->valid, true);

    auto *d = mock.inspect(t);
    u8 v = d[128];
    assert_eq(b->data[128], v);

    b->data[128] = ~v;
    bcache.sync(&ctx, b);
    bcache.release(b);

    assert_eq(d[128], v);
    bcache.end_op(&ctx);
    assert_eq(d[128], ~v);

    bcache.begin_op(&ctx);

    auto *b1 = bcache.acquire(t - 1);
    auto *b2 = bcache.acquire(t - 2);
    assert_eq(b1->block_no, t - 1);
    assert_eq(b2->block_no, t - 2);

    auto *d1 = mock.inspect(t - 1);
    auto *d2 = mock.inspect(t - 2);
    u8 v1 = d1[500];
    u8 v2 = d2[10];
    assert_eq(b1->data[500], v1);
    assert_eq(b2->data[10], v2);

    b1->data[500] = ~v1;
    b2->data[10] = ~v2;
    bcache.sync(&ctx, b1);
    bcache.release(b1);
    bcache.sync(&ctx, b2);
    bcache.release(b2);

    assert_eq(d1[500], v1);
    assert_eq(d2[10], v2);
    bcache.end_op(&ctx);
    assert_eq(d1[500], ~v1);
    assert_eq(d2[10], ~v2);
}

}  // namespace basic

int main() {
    std::vector<Testcase> tests = {
        {"init", basic::test_init},
        {"read_write", basic::test_read_write},
        {"atomic_op", basic::test_atomic_op},
    };
    Runner(tests).run();

    return 0;
}
