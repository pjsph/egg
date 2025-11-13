#include "../src/logger.h"

#include "llist.h"
#include "memlist.h"

int main(void) {
    EINFO("Starting tests");

    llist_tests();
    memlist_tests();

    EINFO("Successfully finished tests");

    return 0;
}
