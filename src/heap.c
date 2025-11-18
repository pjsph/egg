#include "heap.h"

#include "assert.h"

#include "memlist.h"
#include "memory.h"

typedef struct block_header {
    void *start;
    u32 size;
    u16 alignement;
} block_header;

static inline void *align_address(u64 base, u64 align);

u8 eheap_create(u64 size, void *memory, eheap *heap) {
    EASSERT(size > 0);
    EASSERT(memory != 0);
    EASSERT(heap != 0);
    
    heap->size = size;
    heap->memory = memory;
    ememory_set_allocator(EMEMORY_ALLOCATOR_SYSTEM);
    ememlist_create(size, &heap->memlist);
    ememory_set_allocator(EMEMORY_ALLOCATOR_CUSTOM);

    return true;
}

u8 eheap_destroy(eheap *heap) {
    EASSERT(heap != 0);
    ememory_set_allocator(EMEMORY_ALLOCATOR_SYSTEM);
    ememlist_destroy(&heap->memlist);
    ememory_set_allocator(EMEMORY_ALLOCATOR_CUSTOM);
    return true;
}

void *eheap_alloc(eheap *heap, u64 size) {
    return eheap_alloc_align(heap, size, 1);
}

void *eheap_alloc_align(eheap *heap, u64 size, u64 align) {
    u64 total_size = align - 1 + sizeof(block_header) + size;
    u64 offset = 0;
    ememory_set_allocator(EMEMORY_ALLOCATOR_SYSTEM);
    if(ememlist_allocate(&heap->memlist, total_size, &offset)) {
        void *base = heap->memory + offset;
        void *aligned_address = align_address((u64)base + sizeof(block_header), align);
        block_header *header = aligned_address - sizeof(block_header);
        header->start = base;
        header->size = size;
        header->alignement= align;
        ememory_set_allocator(EMEMORY_ALLOCATOR_CUSTOM);
        return aligned_address;
    }
    ememory_set_allocator(EMEMORY_ALLOCATOR_CUSTOM);
    EERROR("couldn't allocate memory");
    return 0;
}

u8 eheap_free(eheap *heap, void *memory) {
    EASSERT(memory >= heap->memory);
    EASSERT(memory < heap->memory + heap->size);
    block_header *header = memory - sizeof(block_header);
    u64 offset = header->start - heap->memory;
    u64 total_size = header->alignement - 1 + sizeof(block_header) + header->size;
    ememory_set_allocator(EMEMORY_ALLOCATOR_SYSTEM);
    u8 res = ememlist_free(&heap->memlist, total_size, offset);
    ememory_set_allocator(EMEMORY_ALLOCATOR_CUSTOM);
    return res;
}

u64 eheap_get_usable_size(eheap *heap, void *memory) {
    block_header *header = memory - sizeof(block_header);
    return header->size;
}

u64 eheap_remaining_space(eheap *heap) {
    return ememlist_free_space(&heap->memlist);
}

static void *align_address(u64 base, u64 align) {
    const u64 mask = align - 1;
    // align should be a power of 2
    EASSERT((align & mask) == 0);
    return (void*)((base + mask) & ~mask);
}
