#pragma once

#ifndef _CORE_SYSCALL_H_
#define _CORE_SYSCALL_H_

#include <core/trapframe.h>

u64 syscall_dispatch(Trapframe *frame);

#endif
