/*
 * Copyright (C) 2021-2077 FATCROWN Team.
 * License: https://github.com/FatGraphicsLab/fatcrown/blob/main/LICENSE
 *
 * @author   kasicass@gmail.com
 * @date     2021-06-03
 */

#pragma once

#include "core/containers/types.h"

namespace crown
{
    typedef Array<char> StringStream;

    namespace string_stream
    {
        // Retruns the stream as a NUL-terminated string.
        const char* c_str(StringStream& s);

        template <typename T> StringStream& stream_printf(StringStream& s, const char* format, T& val);

    } // namespace string_stream

} // namespace crown
