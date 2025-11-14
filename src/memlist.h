#ifndef MEMLIST_H
#define MEMLIST_H

#include "defines.h"

typedef struct ememlist_item {
    u64 offset;
    u64 size;
} ememlist_item;

typedef struct ememlist {
    u64 count;
    struct _memlist_node_t {
        ememlist_item item;
        struct _memlist_node_t *next;
    } *head;
} ememlist;

void ememlist_create(u64 size, ememlist *out);
void ememlist_destroy(ememlist *list);
u8 ememlist_allocate(ememlist *list, u64 size, u64 *offset);
u8 ememlist_free(ememlist *list, u64 size, u64 offset);
u64 ememlist_free_space(ememlist *list);

void ememlist_print(ememlist *list);

#endif // MEMLIST_H
