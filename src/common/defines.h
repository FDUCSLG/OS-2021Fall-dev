#pragma once

typedef _Bool bool;

#define true 1
#define false 0

typedef signed char i8;
typedef unsigned char u8;
typedef signed short i16;
typedef unsigned short u16;
typedef signed int i32;
typedef unsigned int u32;
typedef signed long long i64;
typedef unsigned long long u64;

typedef i64 isize;
typedef u64 usize;

#define MIN(_a, _b)                 \
({                                  \
    typeof(_a) __a = (_a);          \
    typeof(_b) __b = (_b);          \
    __a <= __b ? __a : __b;         \
})

#define MAX(_a, _b)                 \
({                                  \
    typeof(_a) __a = (_a);          \
    typeof(_b) __b = (_b);          \
    __a >= __b ? __a : __b;         \
})

#define ROUNDDOWN(a, n)                 \
({                                      \
    u64 __a = (u64) (a);      \
    (typeof(a)) (__a - __a % (n));      \
})

#define ROUNDUP(a, n)                                           \
({                                                              \
    u64 __n = (u64) (n);                              \
    (typeof(a)) (ROUNDDOWN((u64) (a) + __n - 1, __n));     \
})

// this is compatible with C++: <https://en.cppreference.com/w/c/types/NULL>.
#define NULL 0

#define NORETURN _Noreturn

NORETURN void no_return();
