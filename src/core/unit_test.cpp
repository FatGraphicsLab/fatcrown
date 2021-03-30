#include "config.h"

#if CROWN_BUILD_UNIT_TESTS

#include "core/memory/memory.inl"
#include "core/memory/temp_allocator.inl"

#include <stdlib.h> // EXIT_SUCCESS, EXIT_FAILURE
#include <stdio.h>

#undef CE_ASSERT
#undef CE_ENSURE
#undef CE_FATAL
#define ENSURE(condition)                                   \
    do                                                      \
    {                                                       \
        if (!(condition))                                   \
        {                                                   \
            printf("Assertion failed: '%s' in %s:%d\n\n"    \
                , #condition                                \
                , __FILE__                                  \
                , __LINE__                                  \
                );                                          \
            exit(EXIT_FAILURE);                             \
        }                                                   \
    }                                                       \
    while (0)

namespace crown
{

static void test_memory()
{
    memory_globals::init();
    Allocator& a = default_allocator();

    void* p = a.allocate(32);
    ENSURE(a.allocated_size(p) >= 32);
    a.deallocate(p);

    memory_globals::shutdown();
}

#define RUN_TEST(name)      \
    do {                    \
        printf(#name "\n"); \
        name();             \
    } while (0)

int main_unit_tests()
{
    RUN_TEST(test_memory);
    return EXIT_SUCCESS;
}

} // namespace crown

#endif // CROWN_BUILD_UNIT_TESTS