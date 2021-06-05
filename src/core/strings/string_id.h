/*
 * Copyright (C) 2021-2077 FATCROWN Team.
 * License: https://github.com/FatGraphicsLab/fatcrown/blob/main/LICENSE
 *
 * @author   kasicass@gmail.com
 * @date     2021-06-05
 */

#pragma once

#include "core/types.h"

#define STRING_ID32_BUF_LEN 7
#define STRING_ID64_BUF_LEN 17

namespace crown
{
    // Hashed string.
    struct StringId32
    {
        u32 _id;

        StringId32() : _id(0) {}
        StringId32(u32 idx) : _id(idx) {}
        explicit StringId32(const char* str);
        explicit StringId32(const char* str, u32 len);

        void hash(const char* str, u32 len);

        // Parses the id from `str`.
        void parse(const char* str);

        // Returns this string converted to ASSII.
        // `buf` size must be greater than or equal to STRING_ID32_BUF_LEN or the
        // returned string will be truncated.
        const char* to_string(char* buf, u32 len) const;
    };

    // Hashed string.
    struct StringId64
    {
        u64 _id;

        StringId64() : _id(0) {}
        StringId64(u64 idx) : _id(idx) {}
        explicit StringId64(const char* str);
        explicit StringId64(const char* str, u32 len);

        void hash(const char* str, u32 len);

        // Parses the id from `str`.
        void parse(const char* str);

        // Returns this string converted to ASSII.
        // `buf` size must be greater than or equal to STRING_ID64_BUF_LEN or the
        // returned string will be truncated.
        const char* to_string(char* buf, u32 len) const;
    };

} // namespace crown
