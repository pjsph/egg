#ifndef LLIST_H
#define LLIST_H

/*
 * struct {
 *     u64 count;
 *     struct {
 *         item;
 *         *next;
 *     } *head;
 * }
 */

#include <stdlib.h>
#include "assert.h"

#define llist_next_of(ll, node) *(void**)((u8*)(node) + sizeof(*(ll)->head) - sizeof(void*))

#define llist_append(ll, item, Type)                                                \
    do {                                                                            \
        void *node = malloc(sizeof(Type) + sizeof(void*));                          \
        EASSERT_MSG(node != 0, "couldn't allocate more memory for linked list");    \
        *(Type*)node = (item);                                                      \
        llist_next_of(ll, node) = (ll)->head;                                       \
        (ll)->head = node;                                                          \
        ++(ll)->count;                                                              \
    } while(0)

#define llist_pop(ll)                                                                   \
    do {                                                                                \
        EASSERT((ll)->count > 0);                                                       \
        void *next = llist_next_of(ll, (ll)->head);                                     \
        free((ll)->head);                                                               \
        (ll)->head = next;                                                              \
        --(ll)->count;                                                                  \
    } while(0)

#define llist_insert(ll, item, idx, Type)                                               \
    do {                                                                                \
        EASSERT_MSG((idx) <= (ll)->count, "Out of bound insert");                       \
        EASSERT((idx) >= 0);                                                            \
        if(idx == 0) {                                                                  \
            llist_append(ll, item, Type);                                               \
        } else {                                                                        \
            void *node = malloc(sizeof(Type) + sizeof(void*));                          \
            EASSERT_MSG(node != 0, "couldn't allocate more memory for linked list");    \
            *(Type*)node = (item);                                                      \
            void *previous;                                                             \
            llist_get(ll, idx - 1, &previous);                                          \
            void *next = llist_next_of(ll, previous);                                   \
            llist_next_of(ll, previous) = node;                                         \
            llist_next_of(ll, node) = next;                                             \
            ++(ll)->count;                                                              \
        }                                                                               \
    } while(0)

#define llist_remove(ll, idx)                                                                   \
    do {                                                                                        \
        EASSERT_MSG((idx) < (ll)->count, "Out of bound remove");                                \
        EASSERT((idx) >= 0);                                                                    \
        if(idx == 0) {                                                                          \
            llist_pop(ll);                                                                      \
        } else {                                                                                \
            void *previous;                                                                     \
            llist_get(ll, idx - 1, &previous);                                                  \
            void *current = llist_next_of(ll, previous);                                        \
            llist_next_of(ll, previous) = llist_next_of(ll, current);                           \
            free(current);                                                                      \
            --(ll)->count;                                                                      \
        }                                                                                       \
    } while(0)

#define llist_get(ll, idx, out)                                                     \
    do {                                                                            \
        EASSERT_MSG((idx) < (ll)->count, "Out of bound access");                    \
        EASSERT((idx) >= 0);                                                        \
        void *current = (ll)->head;                                                 \
        for(u64 _i = 1; _i <= (idx); ++_i) {                                        \
            current = llist_next_of(ll, current);                                   \
        }                                                                           \
        *out = current;                                                             \
    } while(0)

#define llist_free(ll)                                      \
    do {                                                    \
        u64 i = 0;                                          \
        void *current;                                      \
        while(i++ < (ll)->count) {                          \
            current = (ll)->head;                           \
            (ll)->head = llist_next_of(ll, (ll)->head);     \
            free(current);                                  \
        }                                                   \
        (ll)->count = 0;                                    \
        (ll)->head = 0;                                     \
    } while(0)

#define llist_foreach(Type, it, ll) for(Type *it = (Type*)(ll)->head, *_idx = 0; (u64)_idx < (ll)->count; _idx = (void*)((u64)_idx + 1), it = *(void**)((u8*)it + sizeof(Type)))

// #undef next_of

#endif // LLIST_H
