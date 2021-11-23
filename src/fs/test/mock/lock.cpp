#include "map.hpp"

#include <condition_variable>

namespace {

struct Mutex {
    bool locked;
    std::mutex mutex;
};

Map<void *, Mutex> mtx_map;
Map<void *, std::condition_variable_any> cv_map;

static void add(void *lock, const char *name [[maybe_unused]]) {
    mtx_map.try_add(lock);
}

static void acquire(void *lock) {
    auto &mtx = mtx_map[lock];
    mtx.mutex.lock();
    mtx.locked = true;
}

static void release(void *lock) {
    auto &mtx = mtx_map[lock];
    mtx.locked = false;
    mtx.mutex.unlock();
}

}  // namespace

extern "C" {
void init_spinlock(struct SpinLock *lock, const char *name) {
    add(lock, name);
}

void acquire_spinlock(struct SpinLock *lock) {
    acquire(lock);
}

void release_spinlock(struct SpinLock *lock) {
    release(lock);
}

bool holding_spinlock(struct SpinLock *lock) {
    return mtx_map[lock].locked;
}

void init_sleeplock(struct SleepLock *lock, const char *name) {
    add(lock, name);
}

void acquire_sleeplock(struct SleepLock *lock) {
    acquire(lock);
}

void release_sleeplock(struct SleepLock *lock) {
    release(lock);
}

void _fs_test_sleep(void *chan, struct SpinLock *lock) {
    cv_map[chan].wait(mtx_map[lock]);
}

void _fs_test_wakeup(void *chan) {
    cv_map[chan].notify_all();
}
}
