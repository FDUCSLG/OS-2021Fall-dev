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
    assert_eq(b->block_no, 1);
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
    bcache.end_op(&ctx);

    bcache.begin_op(&ctx);

    usize t = sblock.num_blocks - 1;
    auto *b = bcache.acquire(t);
    assert_eq(b->block_no, t);
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

void test_loop_read() {
    initialize(1, 128);

    constexpr int num_rounds = 10;
    for (int round = 0; round < num_rounds; round++) {
        std::vector<Block *> p;
        p.resize(sblock.num_blocks);

        for (usize i = 0; i < sblock.num_blocks; i++) {
            p[i] = bcache.acquire(i);
            assert_eq(p[i]->block_no, i);

            auto *d = mock.inspect(i);
            for (usize j = 0; j < BLOCK_SIZE; j++) {
                assert_eq(p[i]->data[j], d[j]);
            }
        }

        for (usize i = 0; i < sblock.num_blocks; i++) {
            assert_eq(p[i]->valid, true);
            bcache.release(p[i]);
        }
    }
}

void test_reuse() {
    initialize(1, 500);

    constexpr int num_rounds = 200;
    constexpr usize blocks[] = {1, 123, 233, 399, 415};

    auto matched = [&](usize bno) {
        for (usize b : blocks) {
            if (bno == b)
                return true;
        }
        return false;
    };

    usize rcnt = 0, wcnt = 0;
    mock.on_read = [&](usize bno, auto) {
        if (matched(bno))
            rcnt++;
    };
    mock.on_write = [&](usize bno, auto) {
        if (matched(bno))
            wcnt++;
    };

    for (int round = 0; round < num_rounds; round++) {
        std::vector<Block *> p;
        for (usize block_no : blocks) {
            p.push_back(bcache.acquire(block_no));
        }
        for (auto *b : p) {
            assert_eq(b->valid, true);
            bcache.release(b);
        }
    }

    assert_true(rcnt < 10);
    assert_eq(wcnt, 0);
}

void test_resident() {
    // NOTE: this test may be a little controversial.
    // the main idea is logging should not pollute block cache in most of time.

    initialize(OP_MAX_NUM_BLOCKS, 500);

    constexpr int num_rounds = 200;
    constexpr usize blocks[] = {1, 123, 233, 399, 415};

    auto matched = [&](usize bno) {
        for (usize b : blocks) {
            if (bno == b)
                return true;
        }
        return false;
    };

    usize rcnt = 0;
    mock.on_read = [&](usize bno, auto) {
        if (matched(bno))
            rcnt++;
    };

    for (int round = 0; round < num_rounds; round++) {
        OpContext ctx;
        bcache.begin_op(&ctx);

        for (usize block_no : blocks) {
            auto *b = bcache.acquire(block_no);
            assert_eq(b->valid, true);
            bcache.sync(&ctx, b);
            bcache.release(b);
        }

        bcache.end_op(&ctx);
    }

    assert_true(rcnt < 10);
}

void test_replay() {}

}  // namespace basic

int main() {
    std::vector<Testcase> tests = {
        {"init", basic::test_init},
        {"read_write", basic::test_read_write},
        {"atomic_op", basic::test_atomic_op},
        {"loop_read", basic::test_loop_read},
        {"reuse", basic::test_reuse},
        {"resident", basic::test_resident},
    };
    Runner(tests).run();

    return 0;
}
