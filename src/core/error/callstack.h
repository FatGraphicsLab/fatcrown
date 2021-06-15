/*
 * Copyright (C) 2021-2077 FATCROWN Team.
 * License: https://github.com/FatGraphicsLab/fatcrown/blob/main/LICENSE
 *
 * @author   kasicass@gmail.com
 * @date     2021-06-15
 */

#pragma once

#include "core/strings/string_stream.h"
#include "core/strings/types.h"

namespace crown
{
    namespace error
    {
        // Fills `ss` with the current call stack.
        void callstack(StringStream& ss);

    } // namespace error

} // namespace crown
