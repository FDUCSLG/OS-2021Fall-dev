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

}  // namespace basic

int main() {
    std::vector<Testcase> tests = {
        {"init", basic::test_init},
        {"read_write", basic::test_read_write},
    };
    Runner(tests).run();

    return 0;
}
