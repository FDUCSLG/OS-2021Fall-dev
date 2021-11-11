#include <core/console.h>
#include <core/syscall.h>

u64 syscall_dispatch(Trapframe *frame) {
    switch (frame->x[8]) {
        case SYS_myexecve: sys_myexecve((char *)frame->x[0]); break;
        case SYS_myexit: sys_myexit(); break;
        case SYS_myprint: sys_myprint(frame->x[0]); break;
        default: PANIC("Unknown syscall!\n");
    }

    return frame->x[0];
}
