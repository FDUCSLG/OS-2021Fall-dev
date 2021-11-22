extern "C" {
#include <fs/cache.h>
}

#include "assert.hpp"
#include "pause.hpp"
#include "runner.hpp"

#include "mock/device.hpp"

namespace basic {

int test_init() {
    init_mock(1, 1);
    init_bcache(&sblock, &device);
    return 0;
}

}  // namespace basic

int main() {
    std::vector<Testcase> tests = {
        {"init", basic::test_init},
    };
    Runner(tests).run();

    return 0;
}
