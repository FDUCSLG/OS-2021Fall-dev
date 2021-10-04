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

// this is compatible with C++: <https://en.cppreference.com/w/c/types/NULL>.
#define NULL 0

#define INLINE        inline __attribute__((unused))
#define ALWAYS_INLINE inline __attribute__((unused, always_inline))
#define NO_INLINE     __attribute__((noinline))

#define NO_RETURN _Noreturn

NO_INLINE NO_RETURN void no_return();

#define MIN(a, b)                                                                                  \
    ({                                                                                             \
        typeof(a) _a = (a);                                                                        \
        typeof(b) _b = (b);                                                                        \
        _a <= _b ? _a : _b;                                                                        \
    })

#define MAX(a, b)                                                                                  \
    ({                                                                                             \
        typeof(a) _a = (a);                                                                        \
        typeof(b) _b = (b);                                                                        \
        _a >= _b ? _a : _b;                                                                        \
    })

// return the largest c that c is a multiple of b and c <= a.
static INLINE u64 round_down(u64 a, u64 b) {
    return a - a % b;
}

// return the smallest c that c is a multiple of b and c >= a.
static INLINE u64 round_up(u64 a, u64 b) {
    return round_down(a + b - 1, b);
}
