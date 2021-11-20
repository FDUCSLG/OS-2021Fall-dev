#pragma once

#include <common/defines.h>

#define PAGE_SIZE 4096

/* memory region attributes */
#define MT_DEVICE_nGnRnE       0x0
#define MT_NORMAL              0x1
#define MT_NORMAL_NC           0x2
#define MT_DEVICE_nGnRnE_FLAGS 0x00
#define MT_NORMAL_FLAGS        0xFF /* Inner/Outer Write-Back Non-Transient RW-Allocate */
#define MT_NORMAL_NC_FLAGS     0x44 /* Inner/Outer Non-Cacheable */

#define SH_OUTER (2 << 8)
#define SH_INNER (3 << 8)

#define AF_USED (1 << 10)

#define PTE_NORMAL_NC ((MT_NORMAL_NC << 2) | AF_USED | SH_OUTER)
#define PTE_NORMAL    ((MT_NORMAL << 2) | AF_USED | SH_OUTER)
#define PTE_DEVICE    ((MT_DEVICE_nGnRnE << 2) | AF_USED)

#define PTE_VALID 0x1

#define PTE_TABLE 0x3
#define PTE_BLOCK 0x1
#define PTE_PAGE  0x3

#define PTE_KERNEL (0 << 6)
#define PTE_USER   (1 << 6)

#define PTE_KERNEL_DATA   (PTE_KERNEL | PTE_NORMAL | PTE_BLOCK)
#define PTE_KERNEL_DEVICE (PTE_KERNEL | PTE_DEVICE | PTE_BLOCK)
#define PTE_USER_DATA     (PTE_USER | PTE_NORMAL | PTE_PAGE)

#define N_PTE_PER_TABLE 512

#define KSPACE_MASK 0xffff000000000000

// convert kernel address into physical address.
#define K2P(addr) ((u64)(addr) - (KSPACE_MASK))

// convert physical address into kernel address.
#define P2K(addr) ((u64)(addr) + (KSPACE_MASK))

// convert any address into kernel address space.
#define KSPACE(addr) ((u64)(addr) | (KSPACE_MASK))

// conver any address into physical address space.
#define PSPACE(addr) ((u64)(addr) & (~KSPACE_MASK))

typedef u64 PTEntry;
typedef PTEntry PTEntries[N_PTE_PER_TABLE];
typedef PTEntry *PTEntriesPtr;

#define PTE_ADDRESS(pte) ((pte) & (~0xFFFu))
#define PTE_FLAGS(pte)   ((pte) & (0xFFFu))
