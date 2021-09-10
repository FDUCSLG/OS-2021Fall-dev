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

#define NORETURN _Noreturn

NORETURN void no_return();
