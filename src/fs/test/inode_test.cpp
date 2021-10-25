extern "C" {
#include <fs/inode.h>
}

#include <thread>

#include "mock/cache.hpp"
#include "runner.hpp"

int init() {
    init_inodes(&sblock, &cache);
    return 0;
}

int alloc() {
    init_inodes(&sblock, &cache);
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
