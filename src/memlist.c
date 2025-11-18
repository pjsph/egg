#include "memlist.h"

#include "llist.h"

void ememlist_create(u64 size, ememlist *out) {
    ememlist_item item = { .offset = 0, .size = size };
    llist_append(out, item, ememlist_item);
}

void ememlist_destroy(ememlist *list) {
    llist_free(list);
}

u8 ememlist_allocate(ememlist *list, u64 size, u64 *offset) {
    if(!list || !offset) {
        return false;
    }

    ememlist_item *previous = 0;
    llist_foreach(ememlist_item, it, list) {
        if(it->size == size) {
            *offset = it->offset;
            // llist_remove(list, (u64)_idx) would use another loop,
            // instead, but do it by hand
            if(previous == 0) {
                llist_pop(list);
            } else {
                llist_next_of(list, previous) = llist_next_of(list, it);
                efree(it);
                --(list)->count;
            }
            return true;
        } else if(it->size > size) {
            *offset = it->offset;
            it->size -= size;
            it->offset += size;
            return true;
        }
        previous = it;
    }

    EWARN("cannot allocate space, no block with enough space found. Requested: %llu, available: %llu", size, ememlist_free_space(list));
    return false;
}

u8 ememlist_free(ememlist *list, u64 size, u64 offset) {
    if(!list || !size) {
        return false;
    }

    if(list->count == 0) {
        ememlist_item item = { .size = size, .offset = offset };
        llist_append(list, item, ememlist_item);
        return true;
    }

    u64 i = 0;
    ememlist_item *it = &list->head->item;
    ememlist_item *previous = 0;
    while(i < list->count) {
        if(it->offset + it->size == offset) {
            // merge with left node (expand right)
            it->size += size;
            ememlist_item *next = llist_next_of(list, it);
            if(next && next->offset == it->offset + it->size) {
                // connects with the next node, merge with it too and delete it (expand left)
                it->size += next->size;
                llist_remove_after(list, it);
            }
            return true;
        }
        if(it->offset == offset) {
            EFATAL("tried to free a block of memory that was previously freed");
            return false;
        }
        if(it->offset > offset) {
            ememlist_item item = { .size = size, .offset = offset };
            if(previous == 0) {
                llist_append(list, item, ememlist_item);
            } else {
                llist_insert_after(list, item, previous, ememlist_item);
            }

            // check if it can be merged with next node
            ememlist_item *next = it;
            ememlist_item *it;
            if(previous == 0) {
                // it is head
                it = &list->head->item;
            } else {
                it = llist_next_of(list, previous);
            }
            if(next->offset == it->offset + it->size) {
                // connects with the next node, merge with it too and delete it (expand left)
                it->size += next->size;
                // llist_remove(list, i + 1);
                llist_remove_after(list, it);
            }
            // check if it can be merged with previous node
            if(previous && it->offset == previous->offset + previous->size) {
                previous->size += it->size;
                // llist_remove(list, i);
                llist_remove_after(list, previous);
            }
            return true;
        }
        if(i + 1 >= list->count && it->offset + it->size < offset) {
            // create a new node at the end
            ememlist_item item = { .size = size, .offset = offset };
            llist_insert_after(list, item, it, ememlist_item);
            return true;
        }

        previous = it;
        it = llist_next_of(list, it);
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
    llist_foreach(ememlist_item, it, list) {
        space += it->size;
    }
    return space;
}

void ememlist_print(ememlist *list) {
    llist_foreach(ememlist_item, it, list) {
        EDEBUG("%llu) .offset = %llu, .size = %llu", (u64)_idx, it->offset, it->size);
    }
}
