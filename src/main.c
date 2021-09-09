#include <aarch64/intrinsic.h>
#include <core/console.h>

static SpinLock init_lock = {.locked = 0};

void init_system_once() {
    if (!try_acquire_spinlock(&init_lock))
        return;

    init_char_device();
    init_console();

    release_spinlock(&init_lock);
}

void init_system_per_core() {}

NORETURN void main() {
    init_system_once();

    wait_spinlock(&init_lock);
    init_system_per_core();

    if (cpuid() == 0)
        puts("Hello, world!");
    else if (cpuid() == 1)
        puts("Hello, rpi-os!");
    else if (cpuid() == 2)
        printf("Hello, printf: %? %% %s %s %u %llu %d %lld %x %llx %p %c\n",
               NULL,
               "(aha)",
               0u,
               1llu,
               -2,
               -3ll,
               4u,
               5llu,
               printf,
               '!');
    else
        delay_us(10000);

    PANIC("TODO: test %s and add %s. CPUID = %zu", "memory", "scheduler", cpuid());
}
