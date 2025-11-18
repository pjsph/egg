#ifndef MEMORY_H
#define MEMORY_H

#include "defines.h"
#include "heap.h"

typedef enum ememory_allocator {
    EMEMORY_ALLOCATOR_SYSTEM,
    EMEMORY_ALLOCATOR_CUSTOM,
} ememory_allocator;

EAPI u8 ememory_init(u64 size, eheap *heap_ptr);
EAPI u8 ememory_uninit();
EAPI void ememory_set_allocator(ememory_allocator allocator);
EAPI void ememory_report();
EAPI void *ealloc(u64 size);
EAPI void efree(void *memory);
EAPI void *erealloc(void *memory, u64 size);
EAPI void ememcpy(void *dest, const void *src, u64 size);
EAPI void *emap(void *addr, u64 length, u32 prot, u32 flags, u32 fd, u32 offset);
EAPI void eunmap(void *addr, u64 length);

#endif // MEMORY_H
