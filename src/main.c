#include <aarch64/intrinsic.h>
#include <common/string.h>
#include <core/console.h>
#include <core/trap.h>
#include <driver/clock.h>
#include <driver/interrupt.h>

static SpinLock init_lock = {.locked = 0};

void init_system_once() {
    if (!try_acquire_spinlock(&init_lock))
        return;

    // initialize BSS sections.
    extern char edata[], end[];
    memset(edata, 0, end - edata);

    init_interrupt();
    init_char_device();
    init_console();

    release_spinlock(&init_lock);
}

void hello() {
    reset_clock(1000);
    printf("CPU %d: HELLO!\n", cpuid());
}

void init_system_per_core() {
    init_clock();
    set_clock_handler(hello);
    init_trap();

    arch_enable_trap();
}

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

    // PANIC("TODO: add %s. CPUID = %zu", "scheduler", cpuid());
    init_memory_manager();
    init_virtual_memory();
    no_return();
}
