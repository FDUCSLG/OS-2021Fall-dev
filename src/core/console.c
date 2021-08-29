#include <aarch64/intrinsic.h>
#include <common/format.h>
#include <core/console.h>

static ConsoleContext ctx;

#define PANIC_BAR_CHAR '='
#define PANIC_BAR_LENGTH 32

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
    ctx.device.put(NEWLINE);

    release_spinlock(&ctx.lock);
}

static void _put_char(void *_ctx, char c) {
    (void)_ctx;
    ctx.device.put(c);
}

void vprintf(const char *fmt, va_list arg) {
    acquire_spinlock(&ctx.lock);
    vformat(_put_char, NULL, fmt, arg);
    release_spinlock(&ctx.lock);
}

void printf(const char *fmt, ...) {
    va_list arg;
    va_start(arg, fmt);
    vprintf(fmt, arg);
    va_end(arg);
}

NORETURN void _panic(const char *file, size_t line, const char *fmt, ...) {
    // we hold `ctx.lock` here and will not release it.
    // in order to prevent other cores printing messages.
    acquire_spinlock(&ctx.lock);

    for (size_t i = 0; i < PANIC_BAR_LENGTH; i++)
        ctx.device.put(PANIC_BAR_CHAR);
    ctx.device.put(NEWLINE);

    format(_put_char, NULL, "KERNEL PANIC at core %zu:\n", cpuid());
    format(_put_char, NULL, "file: %s\n", file);
    format(_put_char, NULL, "line: %zu\n", line);

    va_list arg;
    va_start(arg, fmt);
    vformat(_put_char, NULL, fmt, arg);
    va_end(arg);

    // add a trailing newline for message.
    ctx.device.put(NEWLINE);

    while (1) {}
}
