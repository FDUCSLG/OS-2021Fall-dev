#pragma once

#ifndef _CORE_CONSOLE_H_
#define _CORE_CONSOLE_H_

#include <common/spinlock.h>
#include <core/char_device.h>

typedef struct {
    SpinLock lock;
    ICharDevice device;
} ConsoleContext;

void init_console();

void kernel_puts(const char *str);

#endif
