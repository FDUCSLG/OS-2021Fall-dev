#pragma once

#ifndef __DRIVER_UART_H__
#define __DRIVER_UART_H__

#include <common/types.h>

void init_uart();
uint8_t uart_get_char();
void uart_put_char(uint8_t c);

#endif
