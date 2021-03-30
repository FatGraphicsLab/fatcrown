/*
 * Copyright (C) 2021-2077 FATCROWN Team.
 * License: https://github.com/FatGraphicsLab/fatcrown/blob/main/LICENSE
 *
 * @author   kasicass@gmail.com
 * @date     2021-03-31
 */

#pragma once

#include "core/types.h"

namespace crown
{

    u32 murmur32(const void* key, u32 len, u32 seed);
    u64 murmur64(const void* key, u32 len, u64 seed);

} // namespace crown