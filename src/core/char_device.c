#include <core/char_device.h>
#include <driver/uart.h>

void init_char_device() {
    init_uart();
}

ICharDevice get_uart_char_device() {
    ICharDevice device = {.get = uart_get_char, .put = uart_put_char};
    return device;
}
