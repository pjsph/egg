#include "memlist.h"

#include "../src/defines.h"
#include "../src/assert.h"
#include "../src/memlist.h"

static void memlist_test_create() {
    ememlist list;
    ememlist_create(64, &list);
    EASSERT(list.count == 1);
    ememlist_destroy(&list);
}

static void memlist_test_allocate() {
    ememlist list;
    ememlist_create(64, &list);
    u64 offset;
    u8 res;
    res = ememlist_allocate(&list, 32, &offset);
    EASSERT(res == true);
    u64 remaining_space = ememlist_free_space(&list);
    EASSERT(remaining_space == 32);
    res = ememlist_free(&list, 32, offset);
    EASSERT(res == true);
    remaining_space = ememlist_free_space(&list);
    EASSERT(remaining_space == 64);
    ememlist_destroy(&list);
}

static void memlist_test_allocate_many() {
    ememlist list;
    ememlist_create(64, &list);
    u8 res;

    u64 offset1;
    res = ememlist_allocate(&list, 32, &offset1);
    EASSERT(res == true);
    u64 remaining_space = ememlist_free_space(&list);
    EASSERT(remaining_space == 32);

    u64 offset2;
    res = ememlist_allocate(&list, 16, &offset2);
    EASSERT(res == true);
    remaining_space = ememlist_free_space(&list);
    EASSERT(remaining_space == 16);

    u64 offset3;
    res = ememlist_allocate(&list, 4, &offset3);
    EASSERT(res == true);
    remaining_space = ememlist_free_space(&list);
    EASSERT(remaining_space == 12);

    u64 offset4;
    res = ememlist_allocate(&list, 12, &offset4);
    EASSERT(res == true);
    remaining_space = ememlist_free_space(&list);
    EASSERT(remaining_space == 0);

    res = ememlist_free(&list, 32, offset1);
    EASSERT(res == true);
    remaining_space = ememlist_free_space(&list);
    EASSERT(remaining_space == 32);

    res = ememlist_free(&list, 12, offset4);
    EASSERT(res == true);
    remaining_space = ememlist_free_space(&list);
    EASSERT(remaining_space == 44);

    // count = 2
    EASSERT(list.count == 2);

    // will link to the right
    res = ememlist_free(&list, 4, offset3);
    EASSERT(res == true);
    remaining_space = ememlist_free_space(&list);
    EASSERT(remaining_space == 48);

    // count = 2 still
    EASSERT(list.count == 2);

    // will link to the left and right
    res = ememlist_free(&list, 16, offset2);
    EASSERT(res == true);
    remaining_space = ememlist_free_space(&list);
    EASSERT(remaining_space == 64);

    // count = 1
    EASSERT(list.count == 1);

    ememlist_destroy(&list);
}

void memlist_tests() {
    EINFO("-- memlist_tests");
    memlist_test_create();
    memlist_test_allocate();
    memlist_test_allocate_many();
}
