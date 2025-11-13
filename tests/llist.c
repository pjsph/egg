#include "llist.h"

#include "../src/defines.h"
#include "../src/assert.h"
#include "../src/llist.h"

struct llist_test {
    u64 count;
    struct llist_test_item {
        u64 item;
        struct llist_test_item *next;
    } *head;
};

static void llist_test_append() {
    struct llist_test ll = {0};
    llist_append(&ll, 64, u64);
    EASSERT(ll.count == 1);
    EASSERT(ll.head->item == 64);
    llist_free(&ll);
}

static void llist_test_append_many() {
    struct llist_test ll = {0};
    for(int i = 19; i >= 0; --i) {
        llist_append(&ll, i, u64);
    }
    EASSERT(ll.count == 20);
    llist_foreach(u64, it, &ll) {
        // also check that _idx actually increments by 1 each time
        EASSERT(*it == (u64)_idx);
    }
    llist_free(&ll);
}

static void llist_test_pop() {
    struct llist_test ll = {0};
    llist_append(&ll, 64, u64);
    llist_append(&ll, 32, u64);
    llist_pop(&ll);
    EASSERT(ll.count == 1);
    EASSERT(ll.head->item == 64);
    llist_free(&ll);
}

static void llist_test_insert() {
    struct llist_test ll = {0};
    llist_append(&ll, 64, u64);
    llist_append(&ll, 32, u64);
    llist_append(&ll, 15, u64);
    llist_insert(&ll, 8, 2, u64);
    EASSERT(ll.count == 4);
    u64 *third;
    llist_get(&ll, 2, &third);
    EASSERT(*third == 8);
    llist_free(&ll);
}

static void llist_test_remove() {
    struct llist_test ll = {0};
    llist_append(&ll, 64, u64);
    llist_append(&ll, 32, u64);
    llist_append(&ll, 15, u64);
    llist_remove(&ll, 1);
    EASSERT(ll.count == 2);
    EASSERT(ll.head->item == 15);
    EASSERT(ll.head->next->item == 64);
    llist_free(&ll);
}

static void llist_test_get() {
    struct llist_test ll = {0};
    llist_append(&ll, 64, u64);
    llist_append(&ll, 32, u64);
    u64 *last;
    llist_get(&ll, 1, &last);
    EASSERT(last != 0);
    EASSERT(*last == 64);
    llist_free(&ll);
}

static void llist_test_free() {
    struct llist_test ll = {0};
    llist_append(&ll, 64, u64);
    llist_append(&ll, 32, u64);
    llist_free(&ll);
    EASSERT(ll.count == 0);
    EASSERT(ll.head == 0);
}

static void llist_test_foreach() {
    struct llist_test ll = {0};
    llist_append(&ll, 64, u64);
    llist_append(&ll, 32, u64);
    llist_foreach(u64, it, &ll) {
        (void)it;
    }
    llist_free(&ll);
}

void llist_tests() {
    EINFO("-- llist_tests");
    llist_test_append();
    llist_test_append_many();
    llist_test_pop();
    llist_test_insert();
    llist_test_remove();
    llist_test_get();
    llist_test_free();
    llist_test_foreach();
}
