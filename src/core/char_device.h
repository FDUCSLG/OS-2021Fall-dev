#pragma once

#ifndef __CORE_CHAR_DEVICE__
#define __CORE_CHAR_DEVICE__

#include <common/types.h>

typedef struct {
    uint8_t (*get)();
    void (*put)(uint8_t c);
} ICharDevice;

void init_char_device();
ICharDevice get_uart_char_device();

#endif
