#include "map.hpp"

namespace {
struct Lock {
    bool locked;
    std::mutex mutex;
};

Map<void *, Lock> map;

static void add(void *lock, const char *name [[maybe_unused]]) {
    map.try_add(lock);
}

static void acquire(void *lock) {
    auto &m = map[lock];
    m.mutex.lock();
    m.locked = true;
}

static void release(void *lock) {
    auto &m = map[lock];
    m.locked = false;
    m.mutex.unlock();
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
    return map[lock].locked;
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
}
