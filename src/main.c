#include <aarch64/intrinsic.h>
#include <common/string.h>
#include <core/arena.h>
#include <core/console.h>
#include <core/container.h>
#include <core/physical_memory.h>
#include <core/proc.h>
#include <core/sched.h>
#include <core/trap.h>
#include <core/virtual_memory.h>
#include <driver/clock.h>
#include <driver/interrupt.h>

static SpinLock init_lock = {.locked = 0};

void init_system_once() {
    if (!try_acquire_spinlock(&init_lock))
        return;

    // clear BSS section.
    extern char edata[], end[];
    memset(edata, 0, (usize)(end - edata));

    init_interrupt();
    init_char_device();
    init_console();
    init_sched();

    init_memory_manager();
    init_virtual_memory();

    vm_test();
    arena_test();
    init_container();

    release_spinlock(&init_lock);
}

void hello() {
    // printf("CPU %d: HELLO!\n", cpuid());
    reset_clock(1000);
    yield();
}

void init_system_per_cpu() {
    init_clock();
    set_clock_handler(hello);
    init_trap();

    // arch_enable_trap();
    init_cpu(&root_container->scheduler);
}

NO_RETURN void main() {
    init_system_once();

    wait_spinlock(&init_lock);
    init_system_per_cpu();

    // if (cpuid() == 0)
    //     puts("Hello, world!");
    // else if (cpuid() == 1)
    //     puts("Hello, rpi-os!");
    // else if (cpuid() == 2)
    //     printf("Hello, printf: %? %% %s %s %u %llu %d %lld %x %llx %p %c\n",
    //            NULL,
    //            "(aha)",
    //            0u,
    //            1llu,
    //            -2,
    //            -3ll,
    //            4u,
    //            5llu,
    //            printf,
    //            '!');
    // else
    //     delay_us(10000);

    // PANIC("TODO: add %s. CPUID = %zu", "scheduler", cpuid());

    if (cpuid() == 0) {
        spawn_init_process();
        for (int i = 0; i < 5; i++) {
            idle_init();
        }
        // container_test_init();
        enter_scheduler();
    } else {
        enter_scheduler();
    }

    no_return();
}
