extern "C" {
#include <fs/inode.h>
}

#include <chrono>
#include <thread>

#include "assert.hpp"
#include "mock/cache.hpp"
#include "pause.hpp"
#include "runner.hpp"

int test_init() {
    init_inodes(&sblock, &cache);
    assert_eq(mock.count_inodes(), 1);
    assert_eq(mock.count_blocks(), 0);

    return 0;
}

namespace adhoc {

static OpContext _ctx, *ctx = &_ctx;

int test_alloc() {
    mock.begin_op(ctx);
    usize ino = inodes.alloc(ctx, INODE_REGULAR);

    assert_eq(mock.count_inodes(), 1);
    mock.end_op(ctx);

    mock.fence();
    assert_eq(mock.count_inodes(), 2);

    auto *p = inodes.get(ino);

    inodes.lock(p);
    inodes.unlock(p);

    mock.begin_op(ctx);
    inodes.put(ctx, p);

    assert_eq(mock.count_inodes(), 2);
    mock.end_op(ctx);

    mock.fence();
    assert_eq(mock.count_inodes(), 1);

    return 0;
}

int test_sync() {
    auto *p = inodes.get(1);
    assert_eq(p->entry.type, INODE_DIRECTORY);
    p->entry.major = 0x19;
    p->entry.minor = 0x26;
    p->entry.indirect = 0xa817;

    mock.begin_op(ctx);
    inodes.lock(p);
    inodes.sync(ctx, p, true);
    inodes.unlock(p);
    inodes.put(ctx, p);
    mock.end_op(ctx);

    mock.fence();
    auto *q = mock.inspect(1);
    assert_eq(q->type, INODE_DIRECTORY);
    assert_eq(q->major, 0x19);
    assert_eq(q->minor, 0x26);
    assert_eq(q->indirect, 0xa817);

    return 0;
}

int test_touch() {
    auto *p = inodes.get(1);
    inodes.lock(p);

    for (usize i = 2; i < mock.num_inodes; i++) {
        mock.begin_op(ctx);
        usize ino = inodes.alloc(ctx, INODE_REGULAR);
        inodes.insert(ctx, p, std::to_string(i).data(), ino);

        auto *q = inodes.get(ino);
        inodes.lock(q);
        assert_eq(q->entry.type, INODE_REGULAR);
        assert_eq(q->entry.major, 0);
        assert_eq(q->entry.minor, 0);
        assert_eq(q->entry.num_links, 0);
        assert_eq(q->entry.num_bytes, 0);
        assert_eq(q->entry.indirect, 0);
        for (usize j = 0; j < INODE_NUM_DIRECT; j++) {
            assert_eq(q->entry.addrs[j], 0);
        }

        q->entry.num_links++;
        inodes.sync(ctx, q, true);
        inodes.unlock(q);
        inodes.put(ctx, q);

        assert_eq(mock.count_inodes(), i - 1);
        mock.end_op(ctx);

        mock.fence();
        assert_eq(mock.count_inodes(), i);
    }

    usize n = mock.num_inodes - 1;
    for (usize i = 2; i < mock.num_inodes; i += 2, n--) {
        mock.begin_op(ctx);
        usize index = 10086;
        assert_ne(inodes.lookup(p, std::to_string(i).data(), &index), 0);
        assert_ne(index, 10086);
        inodes.remove(ctx, p, index);

        auto *q = inodes.get(i);
        inodes.lock(q);
        q->entry.num_links = 0;
        inodes.sync(ctx, q, true);
        inodes.unlock(q);
        inodes.put(ctx, q);

        assert_eq(mock.count_inodes(), n);
        mock.end_op(ctx);

        mock.fence();
        assert_eq(mock.count_inodes(), n - 1);
    }

    mock.begin_op(ctx);
    usize ino = inodes.alloc(ctx, INODE_DIRECTORY);
    auto *q = inodes.get(ino);
    assert_eq(q->entry.type, INODE_DIRECTORY);
    assert_eq(mock.count_inodes(), n);
    mock.end_op(ctx);

    mock.fence();
    assert_eq(mock.count_inodes(), n + 1);

    mock.begin_op(ctx);
    inodes.put(ctx, q);
    mock.end_op(ctx);

    mock.fence();
    assert_eq(mock.count_inodes(), n);

    for (usize i = 3; i < mock.num_inodes; i += 2, n--) {
        mock.begin_op(ctx);
        q = inodes.get(i);
        inodes.lock(q);
        q->entry.num_links = 0;
        inodes.sync(ctx, q, true);
        inodes.unlock(q);
        inodes.put(ctx, q);
        assert_eq(mock.count_inodes(), n);
        mock.end_op(ctx);

        mock.fence();
        assert_eq(mock.count_inodes(), n - 1);
    }

    inodes.unlock(p);
    return 0;
}

int test_share() {
    mock.begin_op(ctx);
    usize ino = inodes.alloc(ctx, INODE_REGULAR);
    mock.end_op(ctx);

    mock.fence();
    assert_eq(mock.count_inodes(), 2);

    auto *p = inodes.get(ino);
    auto *q = inodes.share(p);
    auto *r = inodes.get(ino);

    assert_eq(r->rc.count, 3);

    mock.begin_op(ctx);
    inodes.put(ctx, p);
    assert_eq(q->rc.count, 2);
    mock.end_op(ctx);

    mock.fence();
    assert_eq(mock.count_inodes(), 2);

    mock.begin_op(ctx);
    inodes.put(ctx, q);
    assert_eq(r->rc.count, 1);
    assert_eq(mock.count_inodes(), 2);
    inodes.put(ctx, r);
    assert_eq(mock.count_inodes(), 2);
    mock.end_op(ctx);

    mock.fence();
    assert_eq(mock.count_inodes(), 1);

    return 0;
}

int test_small_file() {
    mock.begin_op(ctx);
    usize ino = inodes.alloc(ctx, INODE_REGULAR);
    mock.end_op(ctx);

    u8 buf[1];
    auto *p = inodes.get(ino);
    inodes.lock(p);

    buf[0] = 0xcc;
    inodes.read(p, buf, 0, 0);
    assert_eq(buf[0], 0xcc);

    mock.begin_op(ctx);
    inodes.write(ctx, p, buf, 0, 1);
    assert_eq(mock.count_blocks(), 0);
    mock.end_op(ctx);

    mock.fence();
    auto *q = mock.inspect(ino);
    assert_eq(q->indirect, 0);
    assert_ne(q->addrs[0], 0);
    assert_eq(q->addrs[1], 0);
    assert_eq(q->num_bytes, 1);
    assert_eq(mock.count_blocks(), 1);

    mock.fill_junk();
    buf[0] = 0;
    inodes.read(p, buf, 0, 1);
    assert_eq(buf[0], 0xcc);

    inodes.unlock(p);

    inodes.lock(p);

    mock.begin_op(ctx);
    inodes.clear(ctx, p);
    mock.end_op(ctx);

    mock.fence();
    q = mock.inspect(ino);
    assert_eq(q->indirect, 0);
    assert_eq(q->addrs[0], 0);
    assert_eq(q->num_bytes, 0);
    assert_eq(mock.count_blocks(), 0);

    inodes.unlock(p);

    mock.begin_op(ctx);
    inodes.put(ctx, p);
    mock.end_op(ctx);

    mock.fence();
    assert_eq(mock.count_inodes(), 1);

    return 0;
}

int test_large_file() {
    mock.begin_op(ctx);
    usize ino = inodes.alloc(ctx, INODE_REGULAR);
    mock.end_op(ctx);

    constexpr usize max_size = 65535;
    u8 buf[max_size], copy[max_size];
    std::mt19937 gen(0x12345678);
    for (usize i = 0; i < max_size; i++) {
        copy[i] = buf[i] = gen() & 0xff;
    }

    auto *p = inodes.get(ino);

    inodes.lock(p);
    for (usize i = 0, n = 0; i < max_size; i += n) {
        n = std::min(static_cast<usize>(gen() % 10000), max_size - i);

        mock.begin_op(ctx);
        inodes.write(ctx, p, buf + i, i, n);
        auto *q = mock.inspect(ino);
        assert_eq(q->num_bytes, i);
        mock.end_op(ctx);

        mock.fence();
        assert_eq(q->num_bytes, i + n);
    }
    inodes.unlock(p);

    for (usize i = 0; i < max_size; i++) {
        buf[i] = 0;
    }

    inodes.lock(p);
    inodes.read(p, buf, 0, max_size);
    inodes.unlock(p);

    for (usize i = 0; i < max_size; i++) {
        assert_eq(buf[i], copy[i]);
    }

    inodes.lock(p);
    mock.begin_op(ctx);
    inodes.clear(ctx, p);
    inodes.unlock(p);
    mock.end_op(ctx);

    mock.fence();
    assert_eq(mock.count_inodes(), 2);
    assert_eq(mock.count_blocks(), 0);

    for (usize i = 0; i < max_size; i++) {
        copy[i] = buf[i] = gen() & 0xff;
    }

    inodes.lock(p);
    mock.begin_op(ctx);
    inodes.write(ctx, p, buf, 0, max_size);
    mock.end_op(ctx);
    inodes.unlock(p);

    mock.fence();
    auto *q = mock.inspect(ino);
    assert_eq(q->num_bytes, max_size);

    for (usize i = 0; i < max_size; i++) {
        buf[i] = 0;
    }

    inodes.lock(p);
    for (usize i = 0, n = 0; i < max_size; i += n) {
        n = std::min(static_cast<usize>(gen() % 10000), max_size - i);
        inodes.read(p, buf + i, i, n);
        for (usize j = 0; j < i + n; j++) {
            assert_eq(buf[j], copy[j]);
        }
    }
    inodes.unlock(p);

    mock.begin_op(ctx);
    inodes.put(ctx, p);
    mock.end_op(ctx);

    mock.fence();
    assert_eq(mock.count_inodes(), 1);
    assert_eq(mock.count_blocks(), 0);

    return 0;
}

int test_dir() {
    usize ino[5] = {1};

    mock.begin_op(ctx);
    ino[1] = inodes.alloc(ctx, INODE_DIRECTORY);
    ino[2] = inodes.alloc(ctx, INODE_REGULAR);
    ino[3] = inodes.alloc(ctx, INODE_REGULAR);
    ino[4] = inodes.alloc(ctx, INODE_REGULAR);
    assert_eq(mock.count_inodes(), 1);
    mock.end_op(ctx);

    mock.fence();
    assert_eq(mock.count_inodes(), 5);

    Inode *p[5];
    for (usize i = 0; i < 5; i++) {
        p[i] = inodes.get(ino[i]);
        inodes.lock(p[i]);
    }

    mock.begin_op(ctx);
    inodes.insert(ctx, p[0], "fudan", ino[1]);
    p[1]->entry.num_links++;
    inodes.sync(ctx, p[1], true);

    auto *q = mock.inspect(ino[0]);
    assert_eq(q->addrs[0], 0);
    assert_eq(inodes.lookup(p[0], "fudan", nullptr), ino[1]);
    mock.end_op(ctx);

    mock.fence();
    assert_eq(inodes.lookup(p[0], "fudan", nullptr), ino[1]);
    assert_eq(inodes.lookup(p[0], "sjtu", nullptr), 0);
    assert_eq(inodes.lookup(p[0], "pku", nullptr), 0);
    assert_eq(inodes.lookup(p[0], "tsinghua", nullptr), 0);

    mock.begin_op(ctx);
    inodes.insert(ctx, p[0], ".vimrc", ino[2]);
    inodes.insert(ctx, p[1], "alice", ino[3]);
    inodes.insert(ctx, p[1], "bob", ino[4]);
    p[2]->entry.num_links++;
    p[3]->entry.num_links++;
    p[4]->entry.num_links++;
    inodes.sync(ctx, p[2], true);
    inodes.sync(ctx, p[3], true);
    inodes.sync(ctx, p[4], true);
    mock.end_op(ctx);

    mock.fence();
    for (usize i = 1; i < 5; i++) {
        q = mock.inspect(ino[i]);
        assert_eq(q->num_links, 1);
    }

    usize index = 233;
    assert_eq(inodes.lookup(p[0], "vimrc", &index), 0);
    assert_eq(index, 233);
    assert_eq(inodes.lookup(p[0], ".vimrc", &index), ino[2]);
    assert_ne(index, 233);
    index = 244;
    assert_eq(inodes.lookup(p[1], "nano", &index), 0);
    assert_eq(index, 244);
    assert_eq(inodes.lookup(p[1], "alice", &index), ino[3]);
    usize index2 = 255;
    assert_eq(inodes.lookup(p[1], "bob", &index2), ino[4]);
    assert_ne(index, 244);
    assert_ne(index2, 255);
    assert_ne(index, index2);

    mock.begin_op(ctx);
    inodes.clear(ctx, p[1]);
    p[2]->entry.num_links = 0;
    inodes.sync(ctx, p[2], true);

    q = mock.inspect(ino[1]);
    assert_ne(q->addrs[0], 0);
    assert_eq(inodes.lookup(p[1], "alice", nullptr), 0);
    assert_eq(inodes.lookup(p[1], "bob", nullptr), 0);
    mock.end_op(ctx);

    mock.fence();
    assert_eq(q->addrs[0], 0);
    assert_eq(mock.count_inodes(), 5);
    assert_ne(mock.count_blocks(), 0);

    for (usize i = 0; i < 5; i++) {
        mock.begin_op(ctx);
        inodes.unlock(p[i]);
        inodes.put(ctx, p[i]);
        mock.end_op(ctx);

        mock.fence();
        assert_eq(mock.count_inodes(), (i < 2 ? 5 : 4));
    }

    return 0;
}

}  // namespace adhoc

namespace concurrent {

using namespace std::chrono_literals;

template <typename T>
struct Cell {
    std::mutex mutex;
    T value;

    Cell() = default;
    Cell(const T &_value) : value(_value) {}

    operator T() const {
        return value;
    }

    auto lock() {
        return std::unique_lock(mutex);
    }
};

int test_lifetime() {
    constexpr bool verbose = false;
    constexpr usize num_workers = 4;

    std::atomic<bool> stopped;
    stopped.store(false);

    struct Pointer {
        Inode *ptr = nullptr;
        int refcnt = 0;
    };

    Cell<int> inode_cnt = 2;
    static Cell<Pointer> mem[mock.num_inodes];

    std::vector<std::thread> workers;
    workers.reserve(num_workers);

    for (int i = 0; i < num_workers; i++) {
        workers.emplace_back([&, i] {
            enum Op : int {
                OP_ALLOC,
                OP_GET,
                OP_PUT,
                NUM_OP,
            };

            std::mt19937_64 gen(0xab784cd2u * i);
            std::vector<Inode *> ptr;  // inode pointers owned by this worker.
            ptr.reserve(mock.num_inodes * 2);

            while (!stopped.load()) {
                Op op = static_cast<Op>(gen() % NUM_OP);

                switch (op) {
                    case OP_ALLOC: {
                        auto lock = inode_cnt.lock();
                        if (inode_cnt < static_cast<int>(mock.num_inodes))
                            inode_cnt.value++;
                        else
                            break;
                        lock.unlock();

                        OpContext ctx;
                        mock.begin_op(&ctx);
                        usize ino = inodes.alloc(&ctx, INODE_REGULAR);
                        mock.end_op(&ctx);
                        if constexpr (verbose)
                            printf("[%d] alloc ⇒ %d\n", i, ino);

                        assert_true(1 < ino);
                        assert_true(ino < mock.num_inodes);
                        auto *p = inodes.get(ino);
                        assert_ne(p, nullptr);
                        ptr.push_back(p);
                        if constexpr (verbose)
                            printf("[%d] get %d\n", i, ino);

                        lock = mem[ino].lock();
                        // assert_eq(mem[ino].value.refcnt, 0);
                        // assert_eq(mem[ino].value.ptr, nullptr);
                        mem[ino].value.refcnt++;
                        mem[ino].value.ptr = p;
                    } break;

                    case OP_GET: {
                        usize ino = gen() % (mock.num_inodes - 1) + 1;
                        auto lock = mem[ino].lock();
                        if (ino == 1 || mem[ino].value.refcnt > 0) {
                            OpContext ctx;
                            auto *q = mem[ino].value.ptr;
                            if constexpr (verbose)
                                printf("[%d] get %d\n", i, ino);

                            // prevent inode deallocation during `get`.
                            if (q) {
                                inodes.lock(q);
                                q->entry.num_links++;
                                mock.begin_op(&ctx);
                                inodes.sync(&ctx, q, true);
                                mock.end_op(&ctx);
                                inodes.unlock(q);  // assert failed: rc == 0
                            }

                            mem[ino].value.refcnt++;
                            lock.unlock();

                            auto *p = inodes.get(ino);

                            lock.lock();
                            assert_ne(p, nullptr);
                            if (q) {
                                inodes.lock(p);
                                p->entry.num_links--;
                                mock.begin_op(&ctx);
                                inodes.sync(&ctx, p, true);
                                mock.end_op(&ctx);
                                inodes.unlock(p);
                            }
                            mem[ino].value.ptr = p;
                            lock.unlock();

                            ptr.push_back(p);
                        } else if (!ptr.empty()) {
                            lock.unlock();

                            usize i = gen() % ptr.size();
                            usize ino = ptr[i]->inode_no;
                            if constexpr (verbose)
                                printf("[%d] share %d\n", i, ino);

                            ptr.push_back(inodes.share(ptr[i]));
                            lock = mem[ino].lock();
                            mem[ino].value.refcnt++;
                        }
                    } break;

                    case OP_PUT: {
                        for (int k = 0; k < 3; k++) {
                            if (!ptr.empty()) {
                                usize j = gen() % ptr.size();
                                usize ino = ptr[j]->inode_no;
                                if constexpr (verbose)
                                    printf("[%d] put %d\n", i, ino);

                                auto lock = mem[ino].lock();
                                mem[ino].value.refcnt--;
                                if (mem[ino].value.refcnt == 0)
                                    mem[ino].value.ptr = nullptr;
                                lock.unlock();

                                OpContext ctx;
                                mock.begin_op(&ctx);
                                inodes.put(&ctx, ptr[j]);
                                mock.end_op(&ctx);

                                ptr.erase(ptr.begin() + j);
                                if (mock.inspect(ino)->type == INODE_INVALID) {
                                    lock = inode_cnt.lock();
                                    inode_cnt.value--;
                                }
                            }
                        }
                    } break;

                    default: fprintf(stderr, "(error) unknown op %d\n", op);
                }

                if constexpr (verbose) {
                    printf("[%d] ptr = ", i);
                    for (auto *p : ptr) {
                        printf("%d ", p->inode_no);
                    }
                    puts("");
                }
            }

            if (!ptr.empty()) {
                OpContext ctx;
                mock.begin_op(&ctx);

                for (auto *p : ptr) {
                    usize ino = p->inode_no;
                    inodes.put(&ctx, p);

                    auto lock = mem[ino].lock();
                    mem[ino].value.refcnt--;
                }

                assert_true(mock.count_inodes() > 1);
                mock.end_op(&ctx);
                ptr.clear();
            }
        });
    }

    std::this_thread::sleep_for(10s);
    stopped.store(true);
    for (auto &worker : workers) {
        worker.join();
    }

    for (const auto &m : mem) {
        assert_eq(m.value.refcnt, 0);
    }

    mock.fence();
    assert_eq(mock.count_inodes(), 1);

    return 0;
}

}  // namespace concurrent

int main() {
    if (Runner::run({"init", test_init}))
        init_inodes(&sblock, &cache);
    else
        return -1;

    while (true)
        Runner::run({"lifetime", concurrent::test_lifetime});

    std::vector<Testcase> tests = {
        {"alloc", adhoc::test_alloc},
        {"sync", adhoc::test_sync},
        {"touch", adhoc::test_touch},
        {"share", adhoc::test_share},
        {"small_file", adhoc::test_small_file},
        {"large_file", adhoc::test_large_file},
        {"dir", adhoc::test_dir},
        {"lifetime", concurrent::test_lifetime},
    };
    Runner(tests).run();

    return 0;
}
