#ifndef DARRAY_H
#define DARRAY_H

#include "assert.h"
#include "memory.h"

#define DARRAY_INIT_CAP 256

#define darray_reserve_ext(da, asked_capacity, min_is_init_cap)                                     \
    do {                                                                                            \
        if((asked_capacity) > (da)->capacity) {                                                     \
            if((da)->capacity == 0) {                                                               \
                if(min_is_init_cap) {                                                               \
                    (da)->capacity = DARRAY_INIT_CAP;                                               \
                } else {                                                                            \
                    (da)->capacity = (asked_capacity);                                              \
                }                                                                                   \
            }                                                                                       \
            while((asked_capacity) > (da)->capacity) {                                              \
                (da)->capacity *= 2;                                                                \
            }                                                                                       \
            (da)->items = erealloc((da)->items, (da)->capacity * sizeof(*(da)->items));              \
            EASSERT_MSG((da)->items != 0, "couldn't allocate more memory for dynamic array");    \
        }                                                                                           \
    } while(0)

#define darray_reserve(da, asked_capacity) darray_reserve_ext(da, asked_capacity, 0)

#define darray_append(da, item)                         \
    do {                                                \
        darray_reserve_ext((da), (da)->count + 1, 1);   \
        (da)->items[(da)->count++] = (item);            \
    } while(0)

#define darray_remove(da, i)                            \
    do {                                                \
        i32 j = (i);                                    \
        EASSERT(j < (da)->count);                       \
        (da)->items[j] = (da)->items[--(da)->count];    \
    } while(0)

#define darray_last(da) (da)->items[(da)->count - 1];

#define darray_foreach(Type, it, da) for(Type *it = (da)->items; it < (da)->items + (da)->count; ++it)

#endif // DARRAY_H
