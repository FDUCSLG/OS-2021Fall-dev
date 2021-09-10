#pragma once

#include <common/defines.h>

// "IRQ" is the shorthand for "interrupt".
#define NUM_IRQ_TYPES 64

typedef enum {
    IRQ_AUX = 29,
    IRQ_SDIO = 56,
    IRQ_ARASANSDIO = 62,
} InterruptType;

typedef void (*InterruptHandler)(void);

void init_interrupt();
void set_interrupt_handler(InterruptType type, InterruptHandler handler);
void interrupt_global_handler();
