/*
 * Copyright (C) 2021-2077 FATCROWN Team.
 * License: https://github.com/FatGraphicsLab/fatcrown/blob/main/LICENSE
 *
 * @author   kasicass@gmail.com
 * @date     2021-03-31
 */

#pragma once

#include "core/functional.h"
#include "core/memory/types.h"
#include "core/pair.h"

namespace crown
{

    // Dynamic array of POD items.
    //
    // Does not call constructors/destructors, uses
    // memcpy to move stuff around.
    template <typename T>
    struct Array
    {
        ALLOCATOR_AWARE;

        Allocator* _allocator;
        u32 _capacity;
        u32 _size;
        T* _data;

        Array(Allocator& a);
        Array(const Array<T>& other);
        ~Array();
        T& operator[](u32 index);
        const T& operator[](u32 index) const;
        Array<T>& operator=(const Array<T>& other);
    };

    typedef Array<char> Buffer;



} // namespace crown
