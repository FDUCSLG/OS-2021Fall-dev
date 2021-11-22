#include <cstdio>

extern "C" {
void _fs_test_sleep(void *chan, struct SpinLock *lock) {
    printf("in _fs_test_sleep(chan = %p, lock = %p)\n", chan, lock);
}

void _fs_test_wakeup(void *chan) {
    printf("in _fs_test_wakeup(chan = %p)\n", chan);
}
}
