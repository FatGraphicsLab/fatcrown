/*
 * Copyright (C) 2021-2077 FATCROWN Team.
 * License: https://github.com/FatGraphicsLab/fatcrown/blob/main/LICENSE
 *
 * @author   kasicass@gmail.com
 * @date     2021-06-03
 */

#pragma once

#include "core/containers/array.inl"
#include "core/strings/string.inl"
#include "core/strings/string_stream.h"

namespace crown
{
    inline StringStream& operator<<(StringStream& s, char val)
    {
        array::push_back(s, val);
        return s;
    }

    inline StringStream& operator<<(StringStream& s, s16 val)
    {
        return string_stream::stream_printf(s, "%hd", val);
    }

    inline StringStream& operator<<(StringStream& s, u16 val)
    {
        return string_stream::stream_printf(s, "%hu", val);
    }

    inline StringStream& operator<<(StringStream& s, s32 val)
    {
        return string_stream::stream_printf(s, "%d", val);
    }

    inline StringStream& operator<<(StringStream& s, u32 val)
    {
        return string_stream::stream_printf(s, "%u", val);
    }

    inline StringStream& operator<<(StringStream& s, s64 val)
    {
        return string_stream::stream_printf(s, "%lld", val);
    }

    inline StringStream& operator<<(StringStream& s, u64 val)
    {
        return string_stream::stream_printf(s, "%llu", val);
    }

    inline StringStream& operator<<(StringStream& s, f32 val)
    {
        return string_stream::stream_printf(s, "%.8g", val);
    }

    inline StringStream& operator<<(StringStream& s, f64 val)
    {
        return string_stream::stream_printf(s, "%.16g", val);
    }

    inline StringStream& operator<<(StringStream& s, const char* str)
    {
        array::push(s, str, strlen32(str));
        return s;
    }

    namespace string_stream
    {
        inline const char* c_str(StringStream& s)
        {
            array::push_back(s, '\0');
            array::pop_back(s);
            return array::begin(s);
        }

        template <typename T>
        inline StringStream& stream_printf(StringStream& s, const char* format, T& val)
        {
            char buf[32];
            snprintf(buf, sizeof(buf), format, val);
            return s << buf;
        }

    } // namespace string_stream

} // namespace crown
