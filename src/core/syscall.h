#pragma once

#include <core/trapframe.h>

u64 syscall_dispatch(Trapframe *frame);
