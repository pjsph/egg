#include "memlist.h"

#include "llist.h"

typedef struct ememlist_item {
    u64 offset;
    u64 size;
} ememlist_item;

typedef struct llist_memlist {
    u64 count;
    struct memlist_node_t {
        ememlist_item item;
        struct memlist_node_t *next;
    } *head;
} llist_memlist;

void ememlist_create(u64 size, ememlist *out) {
    ememlist_item item = { .offset = 0, .size = size };
    llist_append((llist_memlist*)out, item, ememlist_item);
}

void ememlist_destroy(ememlist *list) {
    llist_free((llist_memlist*)list);
}

u8 ememlist_allocate(ememlist *list, u64 size, u64 *offset) {
    if(!list || !offset) {
        return false;
    }

    llist_memlist *ll = (llist_memlist*)list;

    llist_foreach(ememlist_item, it, ll) {
        if(it->size == size) {
            *offset = it->offset;
            llist_remove(ll, (u64)_idx);
            return true;
        } else if(it->size > size) {
            *offset = it->offset;
            it->size -= size;
            it->offset += size;
            return true;
        }
    }

    EWARN("cannot allocate space, no block with enough space found. Requested: %llu, available: %llu", size, ememlist_free_space(list));
    return false;
}

u8 ememlist_free(ememlist *list, u64 size, u64 offset) {
    if(!list || !size) {
        return false;
    }

    llist_memlist *ll = (llist_memlist*)list;

    if(ll->count == 0) {
        ememlist_item item = { .size = size, .offset = offset };
        llist_append(ll, item, ememlist_item);
        return true;
    }

    u64 i = 0;
    ememlist_item *it = &ll->head->item;
    ememlist_item *previous = 0;
    while(i < ll->count) {
        if(it->offset + it->size == offset) {
            // merge with left node (expand right)
            it->size += size;
            ememlist_item *next = llist_next_of(ll, it);
            if(next && next->offset == it->offset + it->size) {
                // connects with the next node, merge with it too and delete it (expand left)
                it->size += next->size;
                llist_remove(ll, i + 1);
            }
            return true;
        }
        if(it->offset == offset) {
            EFATAL("tried to free a block of memory that was previously freed");
            return false;
        }
        if(it->offset > offset) {
            ememlist_item item = { .size = size, .offset = offset };
            llist_insert(ll, item, i, ememlist_item);

            // check if it can be merged with next node
            ememlist_item *next = it;
            ememlist_item *it;
            if(previous == 0) {
                // it is head
                it = &ll->head->item;
            } else {
                it = llist_next_of(ll, previous);
            }
            if(next->offset == it->offset + it->size) {
                // connects with the next node, merge with it too and delete it (expand left)
                it->size += next->size;
                llist_remove(ll, i + 1);
            }
            // check if it can be merged with previous node
            if(previous && it->offset == previous->offset + previous->size) {
                previous->size += it->size;
                llist_remove(ll, i);
            }
            return true;
        }
        if(i + 1 >= ll->count && it->offset + it->size < offset) {
            // create a new node at the end
            ememlist_item item = { .size = size, .offset = offset };
            llist_insert(ll, item, i + 1, ememlist_item);
            return true;
        }

        previous = it;
        it = llist_next_of(ll, it);
        ++i;
    }

    EWARN("can't find the block to free");
    return false;
}

u64 ememlist_free_space(ememlist *list) {
    if(!list) {
        return 0;
    }
    u64 space = 0;
    llist_foreach(ememlist_item, it, (llist_memlist*)list) {
        space += it->size;
    }
    return space;
}

void ememlist_print(ememlist *list) {
    llist_foreach(ememlist_item, it, (llist_memlist*)list) {
        EDEBUG("%llu) .offset = %llu, .size = %llu", (u64)_idx, it->offset, it->size);
    }
}
