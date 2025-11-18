#include "heap.h"

#include "../src/defines.h"
#include "../src/assert.h"
#include "../src/heap.h"

#include <sys/mman.h>

static void heap_test_create() {
    eheap heap = {0};
    void *memory = mmap(0, 64, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    eheap_create(64, memory, &heap);
    EASSERT(heap.size == 64);
    EASSERT(heap.memlist.count == 1);
    eheap_destroy(&heap);
    EASSERT(heap.memlist.count == 0);
    munmap(memory, 64);
}

static void heap_test_alloc() {
    eheap heap = {0};
    // actually neads 80 bytes to account for header
    void *memory = mmap(0, 80, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    eheap_create(80, memory, &heap);
    void *ptr = eheap_alloc(&heap, 64);
    EASSERT(ptr != 0);
    EASSERT(heap.memlist.count == 0);
    EASSERT(eheap_remaining_space(&heap) == 0);
    // test write
    *(u64*)ptr = 46;
    EASSERT(*(u64*)ptr == 46);
    // free
    eheap_free(&heap, ptr);
    EASSERT(heap.memlist.count == 1);
    EASSERT(eheap_remaining_space(&heap) == 80);
    eheap_destroy(&heap);
    munmap(memory, 80);
}

static void heap_test_alloc_many() {
    eheap heap = {0};
    // actually neads 133 bytes to account for header
    void *memory = mmap(0, 133, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    eheap_create(133, memory, &heap);
    void *ptr1 = eheap_alloc(&heap, 64);
    EASSERT(ptr1 != 0);
    EASSERT(heap.memlist.count == 1);
    EASSERT(eheap_remaining_space(&heap) == 53);
    void *ptr2 = eheap_alloc(&heap, 37);
    EASSERT(ptr2 != 0);
    EASSERT(heap.memlist.count == 0);
    EASSERT(eheap_remaining_space(&heap) == 0);
    eheap_free(&heap, ptr1);
    EASSERT(heap.memlist.count == 1);
    EASSERT(eheap_remaining_space(&heap) == 80);
    eheap_destroy(&heap);
    munmap(memory, 133);
}

static void heap_test_alloc_too_much() {
    eheap heap = {0};
    void *memory = mmap(0, 64, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    eheap_create(64, memory, &heap);
    void *ptr1 = eheap_alloc(&heap, 32);
    EASSERT(ptr1 != 0);
    EASSERT(heap.memlist.count == 1);
    EASSERT(eheap_remaining_space(&heap) == 16);
    EINFO("*** following error is expected, do not take into account");
    void *ptr2 = eheap_alloc(&heap, 24);
    EASSERT(ptr2 == 0);
    EASSERT(eheap_remaining_space(&heap) == 16);
    // free ptr1
    eheap_free(&heap, ptr1);
    EASSERT(heap.memlist.count == 1);
    EASSERT(eheap_remaining_space(&heap) == 64);
    // retry ptr2 allocation
    ptr2 = eheap_alloc(&heap, 24);
    EASSERT(ptr2 != 0);
    EASSERT(heap.memlist.count == 1);
    EASSERT(eheap_remaining_space(&heap) == 24);
    eheap_destroy(&heap);
    munmap(memory, 133);
}

void heap_tests() {
    EINFO("-- heap_tests");
    heap_test_create();
    heap_test_alloc();
    heap_test_alloc_many();
    heap_test_alloc_too_much();
}
