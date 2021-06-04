/*
 * Copyright (C) 2021-2077 FATCROWN Team.
 * License: https://github.com/FatGraphicsLab/fatcrown/blob/main/LICENSE
 *
 * @author   kasicass@gmail.com
 * @date     2021-03-31
 */

#pragma once

#include "core/memory/types.h"

namespace crown
{
    template <typename T1, typename T2, int T1Aware, int T2Aware>
    struct Pair
    {
    };

    template <typename T1, typename T2>
    struct Pair<T1, T2, 0, 0>
    {
        ALLOCATOR_AWARE;

        T1 first;
        T2 second;

        Pair(T1& f, T2& s);
        Pair(Allocator& /*a*/);
    };

    template <typename T1, typename T2>
    struct Pair<T1, T2, 1, 0>
    {
        ALLOCATOR_AWARE;

        T1 first;
        T2 second;

        Pair(T1& f, T2& s);
        Pair(Allocator& a);
    };

    template <typename T1, typename T2>
    struct Pair<T1, T2, 0, 1>
    {
        ALLOCATOR_AWARE;

        T1 first;
        T2 second;

        Pair(T1& f, T2& s);
        Pair(Allocator& a);
    };

    template <typename T1, typename T2>
    struct Pair<T1, T2, 1, 1>
    {
        ALLOCATOR_AWARE;

        T1 first;
        T2 second;

        Pair(T1& f, T2& s);
        Pair(Allocator& a);
    };

} // namespace crown


#define PAIR(first, second) crown::Pair<first, second, IS_ALLOCATOR_AWARE(first), IS_ALLOCATOR_AWARE(second)>
