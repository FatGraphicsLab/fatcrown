/*
 * Copyright (C) 2021-2077 FATCROWN Team.
 * License: https://github.com/FatGraphicsLab/fatcrown/blob/main/LICENSE
 *
 * @author   kasicass@gmail.com
 * @date     2021-06-15
 */

#include "core/platform.h"

#if CROWN_PLATFORM_WINDOWS

#include "core/strings/string_stream.inl"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

namespace crown
{
    namespace error
    {
        void callstack(StringStream& ss)
        {

        }

    } // namespace error
} // namespace crown

#endif
