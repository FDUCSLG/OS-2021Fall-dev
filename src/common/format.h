#pragma once

#ifndef _COMMON_PRINT_H_
#define _COMMON_PRINT_H_

#include <common/variadic.h>

typedef void (*PutCharFunc)(void *ctx, char c);

void format(PutCharFunc put_char, void *ctx, const char *fmt, va_list arg);

#endif
