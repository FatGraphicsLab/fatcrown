/*
 * Copyright (C) 2021-2077 FATCROWN Team.
 * License: https://github.com/FatGraphicsLab/fatcrown/blob/main/LICENSE
 *
 * @author   kasicass@gmail.com
 * @date     2021-03-31
 */

#include "config.h"
#include "core/containers/array.inl"
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

    static void test_array()
    {
        memory_globals::init();
        Allocator& a = default_allocator();

        // basic
        {
            Array<int> v(a);

            ENSURE(array::empty(v) == true);
            ENSURE(array::size(v) == 0);
            ENSURE(array::capacity(v) == 0);

            array::push_back(v, 1);             // [1,]
            ENSURE(array::empty(v) == false);
            ENSURE(array::size(v) == 1);
            ENSURE(v[0] == 1);

            array::push_back(v, 2);            // [1,2]
            ENSURE(array::empty(v) == false);
            ENSURE(array::size(v) == 2);
            ENSURE(v[1] == 2);
        }

        // set_capacity
        {
            Array<int> v(a);

            array::set_capacity(v, 10);
            ENSURE(array::size(v) == 0);
            ENSURE(array::capacity(v) == 10);

            array::push_back(v, 1);
            array::set_capacity(v, 10);
            ENSURE(array::size(v) == 1);
            ENSURE(array::capacity(v) == 1);

#if 0
            array::set_capacity(v, 5);
            ENSURE(array::size(v) == 1);
            ENSURE(array::capacity(v) == 56);
#endif
        }

        memory_globals::shutdown();
    }

#define RUN_TEST(name)      \
    do {                    \
        name();             \
    } while (0)

    int main_unit_tests()
    {
        printf("\n");
        RUN_TEST(test_memory);
        RUN_TEST(test_array);
        return EXIT_SUCCESS;
    }

} // namespace crown

int main(int argc, char* argv[])
{
    return crown::main_unit_tests();
}
