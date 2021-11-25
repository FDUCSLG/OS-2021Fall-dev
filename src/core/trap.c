#include <aarch64/arm.h>
#include <aarch64/intrinsic.h>
#include <core/console.h>
#include <core/proc.h>
#include <core/syscall.h>
#include <core/trap.h>
#include <driver/clock.h>
#include <driver/interrupt.h>
#include <driver/irq.h>

void init_trap() {
    extern char exception_vector[];
    arch_set_vbar(exception_vector);
    arch_reset_esr();
}

void trap_global_handler(Trapframe *frame) {
    u64 esr = arch_get_esr();
    u64 ec = esr >> ESR_EC_SHIFT;
    u64 iss = esr & ESR_ISS_MASK;
    u64 ir = esr & ESR_IR_MASK;

    // u32 src = get32(IRQ_SRC_CORE(cpuid()));

    switch (ec) {
        case ESR_EC_UNKNOWN: {
            if (ir)
                PANIC("unknown error");
            else
                interrupt_global_handler();
        } break;

        case ESR_EC_SVC64: {
            arch_reset_esr();
            frame->x[0] = syscall_dispatch(frame);

            // TODO: warn if `iss` is not zero.
            (void)iss;
        } break;

        default: {
            // TODO: should exit current process here.
            // exit(1);
        }
    }
}

NO_RETURN void trap_error_handler(u64 type) {
    PANIC("unknown trap type: %d", type);
}
