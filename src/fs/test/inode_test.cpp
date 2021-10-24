extern "C" {
#include <fs/inode.h>
}

#include <thread>

#include "mock/cache.hpp"
#include "runner.hpp"

static MockBlockCache proxy;
static SuperBlock sblock = proxy.get_sblock();
static BlockCache cache = proxy.get_instance();

int init() {
    init_inodes(&sblock, &cache);
    return 0;
}

int adhoc() {
    // TODO
    return -1;
}

int main() {
    Runner runner({
        {"init", init},
        {"adhoc", adhoc},
    });

    proxy.initialize();
    runner.run();

    return 0;
}
