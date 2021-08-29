#pragma once

#ifndef _COMMON_STRING_H_
#define _COMMON_STRING_H_

#include <common/types.h>

void *memcpy(void *restrict dest, const void *restrict src, size_t n);
int memcmp(const void *s1, const void *s2, size_t n);

// warning: please do not use strcmp and specify `n` explicitly.
int strncmp(const char *s1, const char *s2, size_t n);

#endif
