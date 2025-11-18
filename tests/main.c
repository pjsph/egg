#include "../src/logger.h"

#include "llist.h"
#include "memlist.h"
#include "heap.h"

int main(void) {
    EINFO("Starting tests");

    llist_tests();
    memlist_tests();
    heap_tests();

    EINFO("Successfully finished tests");

    return 0;
}
