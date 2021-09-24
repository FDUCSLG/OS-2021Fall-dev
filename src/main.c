#include <aarch64/intrinsic.h>
#include <core/console.h>
#include <core/virtual_memory.h>

VirtualMemoryTable *vmt_ptr;

NORETURN void main() {
    init_char_device();
    init_console();
	/* TODO: Lab1 print */
    printf("Hello world!");
    virtual_memory_init(vmt_ptr);
    

}
