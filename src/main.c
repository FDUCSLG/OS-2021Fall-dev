#include <aarch64/intrinsic.h>
#include <common/string.h>
#include <core/console.h>
#include <core/trap.h>
#include <driver/clock.h>
#include <driver/interrupt.h>
#include <core/memory_manage.h>
#include <core/virtual_memory.h>
#include <core/sched.h>
#include <core/cpu.h>

static SpinLock init_lock = {.locked = 0};

void init_system_once() {
    if (!try_acquire_spinlock(&init_lock))
        return;

    // clear BSS section.
    extern char edata[], end[];
    memset(edata, 0, end - edata);

    init_interrupt();
    init_char_device();
    init_console();
    init_sched();
    spawn_init_process();

    init_memory_manager();
    init_virtual_memory();

    vm_test();

    release_spinlock(&init_lock);
}

void hello() {
    printf("CPU %d: HELLO!\n", cpuid());
    reset_clock(1000);
}

void init_system_per_cpu() {
    init_clock();
    set_clock_handler(hello);
    init_trap();

    arch_enable_trap();
    init_cpu();
}

NO_RETURN void main() {
    init_system_once();

    wait_spinlock(&init_lock);
    init_system_per_cpu();

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

    while (true) {
        arch_wfi();
    }
}
