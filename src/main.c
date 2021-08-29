#include <core/char_device.h>

void init_system() {
    init_char_device();
}

void main() {
    init_system();

    ICharDevice uart = get_uart_char_device();
    uart.put('?');

    while (1) {
    }
}
