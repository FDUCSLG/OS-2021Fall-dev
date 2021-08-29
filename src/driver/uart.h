#pragma once

#ifndef _DRIVER_UART_H_
#define _DRIVER_UART_H_

#include <common/types.h>

void init_uart();
uint8_t uart_get_char();
void uart_put_char(uint8_t c);

#endif
