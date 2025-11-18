#include "../defines.h"
#include "../assert.h"

#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

void *esysalloc(u64 size) {
    void *ptr = malloc(size);
    EASSERT_MSG(ptr != 0, "couldn't allocate memory");
    return ptr;
}

void esysfree(void *memory) {
    free(memory);
}

void esysmemcpy(void *dest, const void *src, u64 size) {
    memcpy(dest, src, size);
}

void *esysmap(void *addr, u64 length, u32 prot, u32 flags, u32 fd, u32 offset) {
    void *ptr = mmap(addr, length, prot, flags, fd, offset);
    EASSERT_MSG(ptr != 0, "couldn't allocate memory");
    return ptr;
}

void esysunmap(void *addr, u64 length) {
    munmap(addr, length);
}
