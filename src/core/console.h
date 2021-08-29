#pragma once

#ifndef _CORE_CONSOLE_H_
#define _CORE_CONSOLE_H_

#include <common/spinlock.h>
#include <common/variadic.h>
#include <core/char_device.h>

typedef struct {
    SpinLock lock;
    CharDevice device;
} ConsoleContext;

void init_console();

void puts(const char *str);
void vprintf(const char *fmt, va_list arg);
void printf(const char *fmt, ...);

#endif
