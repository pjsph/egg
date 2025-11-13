#ifndef MEMLIST_H
#define MEMLIST_H

#include "defines.h"

// tmp
#define EMEMLIST_SIZE 16

typedef struct ememlist {
    u8 internal[EMEMLIST_SIZE];
} ememlist;

void ememlist_create(u64 size, ememlist *out);
void ememlist_destroy(ememlist *list);
u8 ememlist_allocate(ememlist *list, u64 size, u64 *offset);
u8 ememlist_free(ememlist *list, u64 size, u64 offset);
u64 ememlist_free_space(ememlist *list);

void ememlist_print(ememlist *list);

#endif // MEMLIST_H
