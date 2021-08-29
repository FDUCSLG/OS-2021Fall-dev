#pragma once

#ifndef _CORE_CHAR_DEVICE_
#define _CORE_CHAR_DEVICE_

#include <common/types.h>

typedef struct {
    uint8_t (*get)();
    void (*put)(uint8_t c);
} ICharDevice;

void init_char_device();
ICharDevice get_uart_char_device();

#endif
