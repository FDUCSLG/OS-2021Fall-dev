#include <aarch64/intrinsic.h>
#include <core/console.h>

void init_system_per_core() {}

NORETURN void main() {
    init_char_device();
    init_console();
    puts("Hello, world!");
}
