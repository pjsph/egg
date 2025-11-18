#ifndef HEAP_H
#define HEAP_H

#include "defines.h"
#include "memlist.h"

typedef struct eheap {
    u64 size;
    ememlist memlist;
    void *memory;
} eheap;

EAPI u8 eheap_create(u64 size, void *memory, eheap *heap);
EAPI u8 eheap_destroy(eheap *heap);
EAPI void *eheap_alloc(eheap *heap, u64 size);
EAPI void *eheap_alloc_align(eheap *heap, u64 size, u64 align);
EAPI u8 eheap_free(eheap *heap, void *memory);
EAPI u64 eheap_get_usable_size(eheap *heap, void *memory);
EAPI u64 eheap_remaining_space(eheap *heap);

#endif // HEAP_H
