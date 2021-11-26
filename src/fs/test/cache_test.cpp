extern "C" {
#include <fs/cache.h>
}

#include "assert.hpp"
#include "pause.hpp"
#include "runner.hpp"

#include "mock/block_device.hpp"

#include <thread>

namespace basic {

void test_init() {
    initialize(1, 1);
}

// targets: `acquire`, `release`, `sync(NULL, ...)`.

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

void test_loop_read() {
    initialize(1, 128);

    constexpr usize num_rounds = 10;
    for (usize round = 0; round < num_rounds; round++) {
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

    constexpr usize num_rounds = 200;
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

    for (usize round = 0; round < num_rounds; round++) {
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

// targets: `begin_op`, `end_op`, `sync`.

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

void test_overflow() {
    initialize(100, 100);

    OpContext ctx;
    bcache.begin_op(&ctx);

    usize t = sblock.num_blocks - 1;
    for (usize i = 0; i < OP_MAX_NUM_BLOCKS; i++) {
        auto *b = bcache.acquire(t - i);
        b->data[0] = 0xaa;
        bcache.sync(&ctx, b);
        bcache.release(b);
    }

    bool panicked = false;

    auto *b = bcache.acquire(t - OP_MAX_NUM_BLOCKS);
    b->data[128] = 0x88;
    try {
        bcache.sync(&ctx, b);
    } catch (const Panic &) { panicked = true; }

    assert_eq(panicked, true);
}

void test_resident() {
    // NOTE: this test may be a little controversial.
    // the main idea is logging should not pollute block cache in most of time.

    initialize(OP_MAX_NUM_BLOCKS, 500);

    constexpr usize num_rounds = 200;
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

    for (usize round = 0; round < num_rounds; round++) {
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

void test_local_absorption() {
    constexpr usize num_rounds = 1000;

    initialize(100, 100);

    OpContext ctx;
    bcache.begin_op(&ctx);
    usize t = sblock.num_blocks - 1;
    for (usize i = 0; i < num_rounds; i++) {
        for (usize j = 0; j < OP_MAX_NUM_BLOCKS; j++) {
            auto *b = bcache.acquire(t - j);
            b->data[0] = 0xcd;
            bcache.sync(&ctx, b);
            bcache.release(b);
        }
    }
    bcache.end_op(&ctx);

    assert_true(mock.read_count < OP_MAX_NUM_BLOCKS * 5);
    assert_true(mock.write_count < OP_MAX_NUM_BLOCKS * 5);
    for (usize j = 0; j < OP_MAX_NUM_BLOCKS; j++) {
        auto *b = mock.inspect(t - j);
        assert_eq(b[0], 0xcd);
    }
}

void test_global_absorption() {
    constexpr usize op_size = 3;
    constexpr usize num_workers = 100;

    initialize(2 * OP_MAX_NUM_BLOCKS + op_size, 100);
    usize t = sblock.num_blocks - 1;

    OpContext out;
    bcache.begin_op(&out);

    for (usize i = 0; i < OP_MAX_NUM_BLOCKS; i++) {
        auto *b = bcache.acquire(t - i);
        b->data[0] = 0xcc;
        bcache.sync(&out, b);
        bcache.release(b);
    }

    std::vector<OpContext> ctx;
    std::vector<std::thread> workers;
    ctx.resize(num_workers);
    workers.reserve(num_workers);

    for (usize i = 0; i < num_workers; i++) {
        bcache.begin_op(&ctx[i]);
        for (usize j = 0; j < op_size; j++) {
            auto *b = bcache.acquire(t - j);
            b->data[0] = 0xdd;
            bcache.sync(&ctx[i], b);
            bcache.release(b);
        }
        workers.emplace_back([&, i] { bcache.end_op(&ctx[i]); });
    }

    workers.emplace_back([&] { bcache.end_op(&out); });

    for (auto &worker : workers) {
        worker.join();
    }

    for (usize i = 0; i < op_size; i++) {
        auto *b = mock.inspect(t - i);
        assert_eq(b[0], 0xdd);
    }

    for (usize i = op_size; i < OP_MAX_NUM_BLOCKS; i++) {
        auto *b = mock.inspect(t - i);
        assert_eq(b[0], 0xcc);
    }
}

// targets: replay at initialization.

void test_replay() {
    initialize_mock(50, 1000);

    auto *header = mock.inspect_log_header();
    header->num_blocks = 5;
    for (usize i = 0; i < 5; i++) {
        usize v = 500 + i;
        header->block_no[i] = v;
        auto *b = mock.inspect_log(i);
        for (usize j = 0; j < BLOCK_SIZE; j++) {
            b[j] = v & 0xff;
        }
    }

    init_bcache(&sblock, &device);

    assert_eq(header->num_blocks, 0);
    for (usize i = 0; i < 5; i++) {
        usize v = 500 + i;
        auto *b = mock.inspect(v);
        for (usize j = 0; j < BLOCK_SIZE; j++) {
            assert_eq(b[j], v & 0xff);
        }
    }
}

// targets: `alloc`, `free`.

void test_alloc_free() {
    constexpr usize num_rounds = 5;
    constexpr usize num_data_blocks = 1000;

    initialize(100, num_data_blocks);

    for (usize round = 0; round < num_rounds; round++) {
        std::vector<usize> bno;
        for (usize i = 0; i < num_data_blocks; i++) {
            OpContext ctx;
            bcache.begin_op(&ctx);
            bno.push_back(bcache.alloc(&ctx));
            bcache.end_op(&ctx);
        }

        for (usize i = 0; i < num_data_blocks; i += 2) {
            usize no = bno[i];
            assert_ne(no, 0);

            OpContext ctx;
            bcache.begin_op(&ctx);
            bcache.free(&ctx, no);
            bcache.end_op(&ctx);
        }

        OpContext ctx;
        bcache.begin_op(&ctx);
        usize no = bcache.alloc(&ctx);
        assert_ne(no, 0);
        for (usize i = 1; i < num_data_blocks; i += 2) {
            assert_ne(bno[i], no);
        }
        bcache.free(&ctx, no);
        bcache.end_op(&ctx);

        for (usize i = 1; i < num_data_blocks; i += 2) {
            bcache.begin_op(&ctx);
            bcache.free(&ctx, bno[i]);
            bcache.end_op(&ctx);
        }
    }
}

}  // namespace basic

namespace crash {

void test_simple_crash() {}

}  // namespace crash

int main() {
    std::vector<Testcase> tests = {
        {"init", basic::test_init},
        {"read_write", basic::test_read_write},
        {"loop_read", basic::test_loop_read},
        {"reuse", basic::test_reuse},
        {"atomic_op", basic::test_atomic_op},
        {"overflow", basic::test_overflow},
        {"resident", basic::test_resident},
        {"local_absorption", basic::test_local_absorption},
        {"global_absorption", basic::test_global_absorption},
        {"replay", basic::test_replay},
        {"alloc_free", basic::test_alloc_free},

        {"simple_crash", crash::test_simple_crash},
    };
    Runner(tests).run();

    printf("(info) OK: %zu tests passed.\n", tests.size());

    return 0;
}
