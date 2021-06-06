/*
 * Copyright (C) 2021-2077 FATCROWN Team.
 * License: https://github.com/FatGraphicsLab/fatcrown/blob/main/LICENSE
 *
 * @author   kasicass@gmail.com
 * @date     2021-06-06
 */

#pragma once

#include "core/types.h"

namespace crown
{
    struct StringView
    {
        u32 _length;
        const char* _data;

        StringView();
        StringView(const char* str);
        StringView(const char* str, u32 len);
        StringView& operator=(const char* str);

        u32 length() const;
        const char* data() const;
    };

} // namespace crown
