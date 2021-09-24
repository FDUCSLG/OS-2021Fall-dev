#include <aarch64/intrinsic.h>
#include <core/console.h>
#include <core/memory_manage.h>
#include <core/virtual_memory.h>

MemmoryManagerTable *mmt_ptr;
VirtualMemoryTable *vmt_ptr;

NORETURN void main() {
    init_char_device();
    init_console();
	/* TODO: Lab1 print */
    printf("Hello world!\n");
    init_memory_manager();
    init_virtual_memory();    
    vm_test();
}
