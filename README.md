# Operating System

Fall 2021, Fudan University.

(WIP)

## Prerequisites

* Ubuntu 20.04
* CMake
* `gcc-aarch64-linux-gnu`
* `gdb-multiarch`
* `qemu-system-arm`
* `qemu-efi-aarch64`
* `qemu-utils`
* `mtools`
* `dosfstools`
* (not required) `git submodule update --init --recursive`
* (not required) `(cd libc && export CROSS_COMPILE=$(CROSS) && ./configure --target=$(ARCH))`

## Quick Start

Run QEMU with GDB server:

```shell
$ mkdir build
$ cd build
$ cmake .. -DCMAKE_BUILD_TYPE=Debug
$ make qemu-debug
```

Run GDB in another terminal:

```shell
$ cd build
$ make debug
...
Remote debugging using localhost:1234
0x0000000000000000 in ?? ()
(gdb) continue
Continuing.
^C
Thread 1 received signal SIGINT, Interrupt.
main () at /path/to/OS-2021Fall-dev/src/main.c:2
2           while (1) {}
(gdb) backtrace
#0  main () at /path/to/OS-2021Fall-dev/src/main.c:2
```
