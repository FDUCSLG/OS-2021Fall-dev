#include <common/format.h>
#include <core/console.h>

static ConsoleContext ctx;

void init_console() {
    init_spinlock(&ctx.lock);
    init_uart_char_device(&ctx.device);
}

void puts(const char *str) {
    acquire_spinlock(&ctx.lock);

    while (*str != '\0') {
        ctx.device.put(*str++);
    }

    // add a trailing newline.
    ctx.device.put('\n');

    release_spinlock(&ctx.lock);
}

static void _put_char(void *_ctx, char c) {
    (void)_ctx;
    ctx.device.put(c);
}

void vprintf(const char *fmt, va_list arg) {
    acquire_spinlock(&ctx.lock);
    format(_put_char, NULL, fmt, arg);
    release_spinlock(&ctx.lock);
}

void printf(const char *fmt, ...) {
    va_list arg;
    va_start(arg, fmt);
    vprintf(fmt, arg);
    va_end(arg);
}
