/*
 * Copyright (C) 2021-2077 FATCROWN Team.
 * License: https://github.com/FatGraphicsLab/fatcrown/blob/main/LICENSE
 *
 * @author   kasicass@gmail.com
 * @date     2021-03-31
 */

#include "config.h"
#include "core/containers/array.inl"
#include "core/containers/pair.inl"
#include "core/memory/memory.inl"
#include "core/memory/temp_allocator.inl"
#include "core/strings/string.inl"
#include "core/strings/string_stream.inl"

#include <stdlib.h> // EXIT_SUCCESS, EXIT_FAILURE
#include <stdio.h>
#include <string.h>

#undef CE_ASSERT
#undef CE_ENSURE
#undef CE_FATAL
#define ENSURE(condition)                       \
    do                                          \
    {                                           \
        if (!(condition))                       \
        {                                       \
            printf("FAILED: '%s' in %s:%d\n"    \
                , #condition                    \
                , __FILE__                      \
                , __LINE__                      \
                );                              \
            exit(EXIT_FAILURE);                 \
        }                                       \
    }                                           \
    while (0)

namespace crown
{
    static void test_default_allocator()
    {
        memory_globals::init();
        Allocator& a = default_allocator();

        void* p = a.allocate(32);
        ENSURE(a.allocated_size(p) >= 32);
        a.deallocate(p);

        memory_globals::shutdown();
    }

    // TODO(kasicass): unittest for temp_allocator

    static void test_new_delete()
    {
#if 0
        class MyClass
        {
        public:
            MyClass() {}
            ~MyClass() {}
        };

        class MyClassAllocatorAware
        {
        public:
            ALLOCATOR_AWARE;

            MyClassAllocatorAware(Allocator& a) : _allocator(a) {}
            ~MyClassAllocatorAware() {}

        private:
            Allocator& _allocator;
        };

        memory_globals::init();
        Allocator& a = default_allocator();

        MyClass *obj1 = CE_NEW(a, MyClass);
        ENSURE(a.allocated_size(obj1) >= sizeof(MyClass));
        CE_DELETE(a, obj1);

        MyClassAllocatorAware *obj2 = CE_NEW(a, MyClassAllocatorAware)(a);
        ENSURE(a.allocated_size(obj2) >= sizeof(MyClassAllocatorAware));
        CE_DELETE(a, obj2);

        memory_globals::shutdown();
#endif
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

            array::push_back(v, 2);             // [1,2]
            ENSURE(array::empty(v) == false);
            ENSURE(array::size(v) == 2);
            ENSURE(v[1] == 2);
        }

        // set_capacity()
        {
            Array<int> v(a);

            array::set_capacity(v, 10);
            ENSURE(array::size(v) == 0);
            ENSURE(array::capacity(v) == 10);

            array::push_back(v, 1);             // [1,]
            array::set_capacity(v, 10);
            ENSURE(array::size(v) == 1);
            ENSURE(array::capacity(v) == 10);

            array::set_capacity(v, 5);
            ENSURE(array::size(v) == 1);
            ENSURE(array::capacity(v) == 5);

            array::push_back(v, 2);             // [1,2]
            array::push_back(v, 3);             // [1,2,3]
            array::push_back(v, 4);             // [1,2,3,4]
            ENSURE(array::size(v) == 4);
            ENSURE(array::capacity(v) == 5);

            array::set_capacity(v, 2);
            ENSURE(array::size(v) == 2);
            ENSURE(array::capacity(v) == 2);

            array::set_capacity(v, 0);          // set capacity to 0 don't realloc memory
            ENSURE(array::size(v) == 0);
            ENSURE(array::capacity(v) == 2);
        }

        // resize()
        {
            Array<int> v(a);

            array::resize(v, 2);
            ENSURE(array::size(v) == 2);
            ENSURE(array::capacity(v) == 2);

            array::resize(v, 4);
            ENSURE(array::size(v) == 4);
            ENSURE(array::capacity(v) == 4);

            array::resize(v, 3);
            ENSURE(array::size(v) == 3);
            ENSURE(array::capacity(v) == 4);
        }

        // grow()
        {
            Array<int> v(a);

            array::grow(v, 0);
            ENSURE(array::capacity(v) == 1);

            array::grow(v, 0);
            ENSURE(array::capacity(v) == 3);

            array::grow(v, 0);
            ENSURE(array::capacity(v) == 7);

            array::grow(v, 20);
            ENSURE(array::capacity(v) == 20);

            array::grow(v, 0);
            ENSURE(array::capacity(v) == 41);
        }

        // reserve()
        {
            Array<int> v(a);

            array::reserve(v, 10);
            ENSURE(array::size(v) == 0);
            ENSURE(array::capacity(v) == 10);

            array::reserve(v, 5);
            ENSURE(array::size(v) == 0);
            ENSURE(array::capacity(v) == 10);

            array::reserve(v, 15);
            ENSURE(array::size(v) == 0);
            ENSURE(array::capacity(v) == 21);
        }

        // shrink_to_fit()
        {
            Array<int> v(a);

            array::reserve(v, 10);
            array::push_back(v, 1);
            array::push_back(v, 2);
            ENSURE(array::size(v) == 2);
            ENSURE(array::capacity(v) == 10);

            array::shrink_to_fit(v);
            ENSURE(array::size(v) == 2);
            ENSURE(array::capacity(v) == 2);
        }

        // push_back() / pop_back()
        {
            u32 i;
            Array<int> v(a);

            i = array::push_back(v, 1);
            ENSURE(i == 0);
            i = array::push_back(v, 2);
            ENSURE(i == 1);
            i = array::push_back(v, 3);
            ENSURE(i == 2);
            i = array::push_back(v, 4);
            ENSURE(i == 3);

            array::pop_back(v);
            ENSURE(array::size(v) == 3);
            ENSURE(v[0] == 1);
            ENSURE(v[1] == 2);
            ENSURE(v[2] == 3);
        }

        // push() / clear()
        {
            u32 size;
            Array<int> v(a);
            int items[] = {1,2,3,4,5};

            size = array::push(v, items, countof(items));
            ENSURE(size == 5);
            ENSURE(array::size(v) == 5);
            ENSURE(array::capacity(v) == 5);
            ENSURE(v[0] == 1);
            ENSURE(v[1] == 2);
            ENSURE(v[2] == 3);
            ENSURE(v[3] == 4);
            ENSURE(v[4] == 5);

            array::clear(v);
            ENSURE(array::size(v) == 0);
            ENSURE(array::capacity(v) == 5);
        }

        // begin() / end()
        {
            const int *cit;
            int *it;
            Array<int> v(a);
            int items[] = { 1,1,1,1,1 };
            
            array::push(v, items, countof(items));
            
            for (cit = array::begin(v); cit != array::end(v); ++cit)
            {
                ENSURE(*cit == 1);
            }

            for (it = array::begin(v); it != array::end(v); ++it)
            {
                ENSURE(*it == 1);
            }
        }

        // front() / back()
        {
            Array<int> v(a);
            int items[] = { 1,2,3,4,5 };

            array::push(v, items, countof(items));

            ENSURE(array::front(v) == 1);
            ENSURE(array::back(v) == 5);

            array::pop_back(v);
            ENSURE(array::back(v) == 4);
        }

        // ctor / copy ctor
        {
            Array<int> v1(a);
            int items[] = { 1,2,3,4,5 };
            array::push(v1, items, countof(items));

            Array<int> v2(v1);
            ENSURE(array::size(v2) == 5);
            ENSURE(array::capacity(v2) == 5);
            ENSURE(v2[0] == 1);
            ENSURE(v2[1] == 2);
            ENSURE(v2[2] == 3);
            ENSURE(v2[3] == 4);
            ENSURE(v2[4] == 5);

            Array<int> v3 = v1;
            ENSURE(array::size(v3) == 5);
            ENSURE(array::capacity(v3) == 5);
            ENSURE(v3[0] == 1);
            ENSURE(v3[1] == 2);
            ENSURE(v3[2] == 3);
            ENSURE(v3[3] == 4);
            ENSURE(v3[4] == 5);
        }

        memory_globals::shutdown();
    }

    static void test_containers_pair()
    {
        memory_globals::init();
        Allocator& a = default_allocator();

        struct NoAware
        {
            int _value;

            NoAware() {}
            NoAware(int v) : _value(v) {}
            ~NoAware() {}

            int GetValue() const { return _value; }
        };

        struct HasAware
        {
            ALLOCATOR_AWARE;

            Allocator& _allocator;
            int _value;

            HasAware(Allocator& a) : _allocator(a) {}
            ~HasAware() {}

            int GetValue() const { return _value; }
            void SetValue(int v) { _value = v; }
        };

        // Pair<T1, T2, 0, 0>
        {
            NoAware no1(1);
            NoAware no2(2);
            NoAware no3(3);
            NoAware no4(4);

            PAIR(NoAware, NoAware) pair1(no1, no2);
            ENSURE(pair1.first.GetValue() == 1);
            ENSURE(pair1.second.GetValue() == 2);

            PAIR(NoAware, NoAware) pair2(no3, no4);
            ENSURE(pair2.first.GetValue() == 3);
            ENSURE(pair2.second.GetValue() == 4);

            swap(pair1, pair2);
            ENSURE(pair1.first.GetValue() == 3);
            ENSURE(pair1.second.GetValue() == 4);
            ENSURE(pair2.first.GetValue() == 1);
            ENSURE(pair2.second.GetValue() == 2);
        }

        // Pair<T1, T2, 1, 0>
        {
            HasAware has1(a); has1.SetValue(1);
            NoAware no2(2);
            HasAware has3(a); has3.SetValue(3);
            NoAware no4(4);

            PAIR(HasAware, NoAware) pair1(has1, no2);
            ENSURE(pair1.first.GetValue() == 1);
            ENSURE(pair1.second.GetValue() == 2);

            PAIR(HasAware, NoAware) pair2(has3, no4);
            ENSURE(pair2.first.GetValue() == 3);
            ENSURE(pair2.second.GetValue() == 4);

            swap(pair1, pair2);
            ENSURE(pair1.first.GetValue() == 3);
            ENSURE(pair1.second.GetValue() == 4);
            ENSURE(pair2.first.GetValue() == 1);
            ENSURE(pair2.second.GetValue() == 2);
        }

        // Pair<T1, T2, 0, 1>
        {
            NoAware no1(1);
            HasAware has2(a); has2.SetValue(2);
            NoAware no3(3);
            HasAware has4(a); has4.SetValue(4);

            PAIR(NoAware, HasAware) pair1(no1, has2);
            ENSURE(pair1.first.GetValue() == 1);
            ENSURE(pair1.second.GetValue() == 2);

            PAIR(NoAware, HasAware) pair2(no3, has4);
            ENSURE(pair2.first.GetValue() == 3);
            ENSURE(pair2.second.GetValue() == 4);

            swap(pair1, pair2);
            ENSURE(pair1.first.GetValue() == 3);
            ENSURE(pair1.second.GetValue() == 4);
            ENSURE(pair2.first.GetValue() == 1);
            ENSURE(pair2.second.GetValue() == 2);
        }

        // Pair<T1, T2, 1, 1>
        {
            HasAware has1(a); has1.SetValue(1);
            HasAware has2(a); has2.SetValue(2);
            HasAware has3(a); has3.SetValue(3);
            HasAware has4(a); has4.SetValue(4);

            PAIR(HasAware, HasAware) pair1(has1, has2);
            ENSURE(pair1.first.GetValue() == 1);
            ENSURE(pair1.second.GetValue() == 2);

            PAIR(HasAware, HasAware) pair2(has3, has4);
            ENSURE(pair2.first.GetValue() == 3);
            ENSURE(pair2.second.GetValue() == 4);

            swap(pair1, pair2);
            ENSURE(pair1.first.GetValue() == 3);
            ENSURE(pair1.second.GetValue() == 4);
            ENSURE(pair2.first.GetValue() == 1);
            ENSURE(pair2.second.GetValue() == 2);
        }

        // Pair<T1, T2, 1, 0>::Pair(Allocator& a)
        // Pair<T1, T2, 1, 1>::Pair(Allocator& a)
        {
            PAIR(HasAware, NoAware) pair1(a);
            ENSURE(&pair1.first._allocator == &a);

            PAIR(HasAware, HasAware) pair2(a);
            ENSURE(&pair2.first._allocator == &a);
            ENSURE(&pair2.second._allocator == &a);
        }

        memory_globals::shutdown();
    }

    static void test_string_inline()
    {
        // snprintf()
        {
            char s[128];
            snprintf(s, 128, "Hello %d %s", 10, "Boy!");
            ENSURE(strcmp(s, "Hello 10 Boy!") == 0);
        }

        // strlen32()
        {
            ENSURE(strlen32("Hello!") == 6);
            ENSURE(strlen32("") == 0);
        }

        // skip_spaces()
        {
            char s1[] = " \tHello Boy!\tCoool!";
            const char* s2;
            
            s2 = skip_spaces(s1);
            ENSURE(*s2 == 'H');
            
            while (!isspace(*s2)) ++s2;
            s2 = skip_spaces(s2);
            ENSURE(*s2 == 'B');

            while (!isspace(*s2)) ++s2;
            s2 = skip_spaces(s2);
            ENSURE(*s2 == 'C');
        }

        // skip_block()
        {
            const char* s;

            s = skip_block("[Hello]Baby!", '[', ']');
            ENSURE(*s == 'B');

            s = skip_block("[Hello Baby!", '[', ']');
            ENSURE(s == NULL);

            s = skip_block("[Hello[Baby!]Woo!]NoMan!", '[', ']');
            ENSURE(*s == 'N');
        }

        // strnl()
        {
            const char* s;

            s = strnl("Hello!\nBoy!");
            ENSURE(*s == 'B');

            s = strnl("Hello!");
            ENSURE(s[-1] == '!');
        }

        // wildcmp()
        {
            ENSURE(wildcmp("a.jpg", "b.jpg") == 0);
            ENSURE(wildcmp("?.jpg", "b.jpg") == 1);
            ENSURE(wildcmp("*.jpg", "Hello.jpg") == 1);
            ENSURE(wildcmp("*l*.jpg", "Hello.jpg") == 1);
            ENSURE(wildcmp("*lo.jpg", "Hello.jpg") == 1);
            ENSURE(wildcmp("*llo.jpg", "Hello.jpg") == 1);
            ENSURE(wildcmp("*?.jpg", "Hello.jpg") == 1);
            ENSURE(wildcmp("Hello.*", "Hello.jpg") == 1);
            ENSURE(wildcmp("H*llo.*", "Hello.jpg") == 1);
            ENSURE(wildcmp("He*o.j?g", "Hello.jpg") == 1);
            ENSURE(wildcmp("H*.jpg", "Hello.jpa") == 0);
        }

        // str_has_prefix()
        {
            ENSURE(str_has_prefix("Hello!Boy!", "Hello!") == true);
            ENSURE(str_has_prefix("Hello!Boy!", "Abc!") == false);
        }

        // str_has_suffix()
        {
            ENSURE(str_has_suffix("Hello!Boy!", "Boy!") == true);
            ENSURE(str_has_suffix("Hello!Boy!", "Goy!") == false);
        }
    }

    static void test_string_stream()
    {
        memory_globals::init();

        // char
        {
            TempAllocator1024 ta;
            StringStream ss(ta);
            ss << 'B' << 'a' << 'b' << 'y';
            ENSURE(strcmp(string_stream::c_str(ss), "Baby") == 0);
        }

        // const char*
        {
            TempAllocator1024 ta;
            StringStream ss(ta);
            ss << "Baby!";
            ENSURE(strcmp(string_stream::c_str(ss), "Baby!") == 0);
        }

        // s16/u16
        {
            TempAllocator1024 ta;
            StringStream ss(ta);
            s16 a = -10;
            u16 b = 24;
            ss << a << b;
            ENSURE(strcmp(string_stream::c_str(ss), "-1024") == 0);
        }

        // s32/u32
        {
            TempAllocator1024 ta;
            StringStream ss(ta);
            s32 a = -65537;
            u32 b = 65538;
            ss << a << b;
            ENSURE(strcmp(string_stream::c_str(ss), "-6553765538") == 0);
        }

        // s64/u64
        {
            TempAllocator1024 ta;
            StringStream ss(ta);
            s64 a = -655370;
            u64 b = 655380;
            ss << a << b;
            ENSURE(strcmp(string_stream::c_str(ss), "-655370655380") == 0);
        }

        // f32/f64
        {
            TempAllocator1024 ta;
            StringStream ss(ta);
            f32 a = 1.2f;
            f64 b = -6.466;
            ss << a << b;
            ENSURE(strcmp(string_stream::c_str(ss), "1.2-6.466") == 0);
        }

        memory_globals::shutdown();
    }

#define RUN_TEST(name)      \
    do {                    \
        name();             \
    } while (0)

    int main_unit_tests()
    {
        RUN_TEST(test_default_allocator);
        RUN_TEST(test_new_delete);
        RUN_TEST(test_array);
        RUN_TEST(test_containers_pair);
        RUN_TEST(test_string_inline);
        RUN_TEST(test_string_stream);
        return EXIT_SUCCESS;
    }

} // namespace crown

int main(int argc, char* argv[])
{
    return crown::main_unit_tests();
}
