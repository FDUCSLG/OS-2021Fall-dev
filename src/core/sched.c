#include "defines.h"
#include "sched.h"
#include "console.h"

struct scheduler *alloc_sched()
{

}

void init_sched()
{
    asserts(root_scheduler == NULL, "Root scheduler alloc more than once");
    root_scheduler = alloc_sched();
    asserts(root_scheduler != NULL, "Root scheduler not alloc yet");
}

NORETURN void default_scheduler()
{

}

