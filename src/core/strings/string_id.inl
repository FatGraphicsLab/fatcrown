/*
 * Copyright (C) 2021-2077 FATCROWN Team.
 * License: https://github.com/FatGraphicsLab/fatcrown/blob/main/LICENSE
 *
 * @author   kasicass@gmail.com
 * @date     2021-06-05
 */

#pragma once

#include "core/functional.h"
#include "core/murmur.h"
#include "core/strings/string.inl"
#include "core/strings/string_id.h"

namespace crown
{
    // StringId32
    inline bool operator==(const StringId32& a, const StringId32& b)
    {
        return a._id == b._id;
    }

    inline bool operator==(const StringId32& a, const u32 b)
    {
        return a._id == b;
    }

    inline bool operator!=(const StringId32& a, const StringId32& b)
    {
        return a._id != b._id;
    }

    inline bool operator<(const StringId32& a, const StringId32& b)
    {
        return a._id < b._id;
    }

    template <>
    struct hash<StringId32>
    {
        u32 operator()(const StringId32& id) const
        {
            return id._id;
        }
    };

#if CROWN_DEBUG
    inline u32 STRING_ID_32(const char* str, const u32 id)
    {
        CE_ASSERT(murmur32(str, strlen32(str), 0) == id, "Hash mismatch");
        return id;
    }
#endif


    // StringId64
    inline bool operator==(const StringId64& a, const StringId64& b)
    {
        return a._id == b._id;
    }

    inline bool operator==(const StringId64& a, const u64 b)
    {
        return a._id == b;
    }

    inline bool operator!=(const StringId64& a, const StringId64& b)
    {
        return a._id != b._id;
    }

    inline bool operator<(const StringId64& a, const StringId64& b)
    {
        return a._id < b._id;
    }

    template <>
    struct hash<StringId64>
    {
        u64 operator()(const StringId64& id) const
        {
            return id._id;
        }
    };

#if CROWN_DEBUG
    inline u64 STRING_ID_64(const char* str, const u64 id)
    {
        CE_ASSERT(murmur64(str, strlen32(str), 0) == id, "Hash mismatch");
        return id;
    }
#endif

} // namespace crown
