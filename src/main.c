#include <aarch64/intrinsic.h>
#include <core/console.h>

void init_system() {
    init_char_device();
    init_console();
}

void main() {
    init_system();

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

    while (1) {}
}
