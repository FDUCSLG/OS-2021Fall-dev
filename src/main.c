#include <aarch64/intrinsic.h>
#include <core/console.h>

void init_system() {
    init_char_device();
    init_console();
}

void main() {
    init_system();

    if (cpuid() == 0)
        kernel_puts("Hello, world!");
    else if (cpuid() == 1)
        kernel_puts("Hello, rpi-os!");

    while (1) {}
}
