#include <core/syscall.h>

u64 syscall_dispatch(Trapframe *frame) {
    // TODO.
    return frame->x[0];
}
