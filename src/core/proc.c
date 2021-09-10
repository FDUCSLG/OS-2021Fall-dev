#include <core/proc.h>

static struct proc *alloc_proc()
{
    
}

void spawn_init_process()
{
    struct proc *p;
    /* for why our symbols differ from xv6, please refer https://stackoverflow.com/questions/10486116/what-does-this-gcc-error-relocation-truncated-to-fit-mean */
    extern char _binary_obj_user_initcode_start[], _binary_obj_user_initcode_size[];
    
}

void forkret()
{

}

void exit()
{

}