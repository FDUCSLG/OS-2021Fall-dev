extern "C" {
#include "inode.h"
}

int main() {
    SuperBlock sblock;
    BlockCache cache;
    init_inodes(&sblock, &cache);
    return 0;
}
