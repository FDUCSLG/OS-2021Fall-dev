#pragma once

#ifndef __cplusplus

typedef _Bool bool;
static const bool true = 1;
static const bool false = 0;

#ifndef NULL
#define NULL ((void *)0)
#endif

#define NO_RETURN _Noreturn

#else

#ifndef NULL
#define NULL 0
#endif

#define NO_RETURN [[noreturn]]

#define restrict __restrict__

#endif

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

#define BIT(i) (1ull << (i))

#define INLINE        inline __attribute__((unused))
#define ALWAYS_INLINE inline __attribute__((unused, always_inline))
#define NO_INLINE     __attribute__((noinline))

// NOTE: no_return will disable traps.
NO_RETURN NO_INLINE void no_return();

// `offset_of` returns the offset of `member` inside struct `type`.
#define offset_of(type, member) ((usize)(&((type *)NULL)->member))

// assuming `mptr` is a pointer to `member` inside struct `type`, this
// macro returns the pointer to the "container" struct `type`.
//
// this is useful for lists. We often embed a `ListNode` inside a struct:
//
// > typedef struct {
// >     u64 data;
// >     ListNode node;
// > } Container;
// > Container a;
// > ListNode *b = &a.node;
//
// then `container_of(b, Container, node)` will be the same as `&a`.
#define container_of(mptr, type, member)                                                           \
    ({                                                                                             \
        const typeof(((type *)NULL)->member) *_mptr = (mptr);                                      \
        (type *)((u8 *)_mptr - offset_of(type, member));                                           \
    })

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
