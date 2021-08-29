# Operating System

Fall 2021, Fudan University.

This project is based on <https://github.com/Hongqin-Li/rpi-os>.

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

Run rpi-os in QEMU:

```shell
$ mkdir build
$ cd build
$ cmake .. -DCMAKE_BUILD_TYPE=Debug
$ make qemu
...
Hello, rpi-os!
Hello, world!
```
