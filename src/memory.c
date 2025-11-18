#include "memory.h"

#include "heap.h"
#include "darray.h"

#include "logger.h"
#include <sys/mman.h>

void *esysalloc(u64 size);
void esysfree(void *memory);
void esysmemcpy(void *dest, const void *src, u64 size);
void *esysmap(void *addr, u64 length, u32 prot, u32 flags, u32 fd, u32 offset);
void esysunmap(void *addr, u64 length);

typedef struct memory_stats_region {
    u64 size;
    void *start;
} memory_stats_region;

typedef struct da_memory_stats_regions {
    memory_stats_region *items;
    u32 count;
    u32 capacity;
} da_memory_stats_regions;

typedef struct memory_stats {
     da_memory_stats_regions mapped_regions;
     u64 custom_allocations_count;
     u64 system_allocations_count;
} memory_stats;

typedef struct memory_state {
    u64 size;
    void *memory;
    eheap *heap;
    memory_stats stats;
    ememory_allocator allocator;
} memory_state;

memory_state memstate;

u8 ememory_init(u64 size, eheap *heap_ptr) {
    memstate = (memory_state){0};
    memstate.size = size;
    memstate.heap = heap_ptr;
    memstate.memory = esysmap(0, size, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    eheap_create(memstate.size, memstate.memory, memstate.heap);
    // append stats with the custom allocator just created
    memory_stats_region item = { .size = size, .start = memstate.memory };
    darray_append(&memstate.stats.mapped_regions, item);
    return true;
}

u8 ememory_uninit() {
    eheap_destroy(memstate.heap);
    eunmap(memstate.memory, memstate.size);
    return true;
}

void ememory_set_allocator(ememory_allocator allocator) {
    memstate.allocator = allocator;
}

void ememory_report() {
    EDEBUG("Showing memory usage:");
    EDEBUG("%d mmapped regions:", memstate.stats.mapped_regions.count);
    darray_foreach(memory_stats_region, it, &memstate.stats.mapped_regions) {
        EDEBUG("- size = %llu, start = %p", it->size, it->start);
    }
    EDEBUG("%d allocated regions on custom heap (ealloc)", memstate.stats.custom_allocations_count);
    EDEBUG("%d allocated regions on system heap (ealloc)", memstate.stats.system_allocations_count);
}

void *ealloc(u64 size) {
    switch(memstate.allocator) {
        case EMEMORY_ALLOCATOR_SYSTEM:
            ++memstate.stats.system_allocations_count;
            return esysalloc(size);
        default:
        case EMEMORY_ALLOCATOR_CUSTOM:
            ++memstate.stats.custom_allocations_count;
            return eheap_alloc(memstate.heap, size);
    }
}

void efree(void *memory) {
    switch(memstate.allocator) {
        case EMEMORY_ALLOCATOR_SYSTEM:
            --memstate.stats.system_allocations_count;
            esysfree(memory);
            break;
        default:
        case EMEMORY_ALLOCATOR_CUSTOM:
            --memstate.stats.custom_allocations_count;
            eheap_free(memstate.heap, memory);
            break;
    }
}

void *erealloc(void *memory, u64 size) {
    void *new_memory = ealloc(size);
    if(memory && new_memory) {
        u64 old_size = eheap_get_usable_size(memstate.heap, memory);
        EINFO("reallocating from %llu to %llu", old_size, size);
        ememcpy(new_memory, memory, old_size);
        efree(memory);
    }
    return new_memory;
}

void ememcpy(void *dest, const void *src, u64 size) {
    esysmemcpy(dest, src, size);
}

void *emap(void *addr, u64 length, u32 prot, u32 flags, u32 fd, u32 offset) {
    void *ptr = esysmap(addr, length, prot, flags, fd, offset);
    memory_stats_region item = { .size = length, .start = ptr };
    darray_append(&memstate.stats.mapped_regions, item);
    return ptr;
}

void eunmap(void *addr, u64 length) {
    darray_foreach(memory_stats_region, it, &memstate.stats.mapped_regions) {
        if(it->start == addr) {
            // delete it
            u64 i = it - memstate.stats.mapped_regions.items;
            darray_remove(&memstate.stats.mapped_regions, i);
            break;
        }
    }
    return esysunmap(addr, length);
}
