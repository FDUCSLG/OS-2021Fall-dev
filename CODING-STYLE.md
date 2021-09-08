# 代码风格规范

仓库中有 `.clang-format`，大部分代码风格的问题可以使用 Clang Format 这个工具解决。以下是附加规范：

* 文件名使用小写和下划线。头文件后缀名为 `.h`，源代码文件后缀名为 `.c`

* 注释使用双斜杠 `//` 或者 `/** ... */`。注释首字母不大写。每一句话后面需要有句点（`.`）。

```c
// one line comment.

/* one line comment. */

// multi
// line
// comment.

/**
 * multi
 * line
 * comment. Multi line comment.
 */
```

* 结构体使用 `typedef struct { ... } StructName`。

* 变量名、函数名使用 `snake_case`，类型名、结构体名使用 `CamelCase`。

```c
void hello_world() {
    int some_count = 0;
}

typedef struct {
    int a;
} HelloWorld;

typedef HelloWorld AnotherOne;
```

* 如非名称过长，请尽量避免使用缩写。

* 无论什么名称，尽量避免使用双下划线（`__`），包括开头和结尾。

```c
#define __CORE_AHA_H__  // NO
#define _CORE_AHA_H_    // YES
```

* 希望对其它编译单元隐藏的函数、变量，请在名称前加上下划线（`_`）。

```c
static int _hidden;
static inline _helper_function();
```

* 没有用到的函数参数，请使用 `(void)arg_name`。

```c
int64_t add(int64_t a, int64_t b, bool flag) {
    (void)flag;
    return a + b;
}
```

* 使用头文件时，请使用尖括号 `<...>`。

```c
#include "some_header.h"          // NO
#include <package/some_header.h>  // YES
```

* 头文件的第一行使用 `#pragma once`。

* 避免直接使用 `short`、`int`、`long long` 等类型，使用 `bool`、`int64_t`、`int32_t`、`int64_t` 等。

```c
int is_okay();   // NO
bool is_okay();  // YES
```

* 函数声明中请把参数名称带上。

```c
int foo(int);        // NO
int foo(int value);  // YES
```

* 如果 `typedef` 只用到了一次，可以不用。

```c
// NO
typedef void (*SomeHandler)(void);

typedef struct {
    SomeHandler handler;
} Context;

// YES
typedef struct {
    void (*handler)(void);
} Context;
```

* 声明函数指针时请把参数名称也带上。

```c
// NO
typedef struct {
    void (*func)(int);
} Interface;

// YES
typedef struct {
    void (*func)(int value);
} Interface;
```

* 所有 `inline` 函数使用 `static inline`。

```c
inline int add(int a, int b);         // NO
static inline int add(int a, int b);  // YES
```

* 不会返回的函数（如 `panic`、`fork`），请在前面标注 `NORETURN`。

```c
NORETURN void panic();

NORETURN void panic() {
    while (1) {}
}
```

* 对于一个模块（如内存分配器），建议将所有全局变量放到一个结构体内。如果不需要被其它模块引用，请标记为 `static`。

```c
// NO
SpinLock mem_lock;
size_t mem_size;

// YES
typedef struct {
    SpinLock lock;
    size_t size;
} MemoryContext;

static MemoryContext ctx;
```

* 请使用 `asm volatile` 而不是 `asm`。

* 请尽可能消除所有的编译器警告，而不是忽略它们。

欢迎对当前的代码风格提出意见。
