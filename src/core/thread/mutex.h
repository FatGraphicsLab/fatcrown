/*
 * Copyright (C) 2021-2077 FATCROWN Team.
 * License: https://github.com/FatGraphicsLab/fatcrown/blob/main/LICENSE
 *
 * @author   kasicass@gmail.com
 * @date     2021-03-30
 */

#pragma once

#include "core/types.h"

namespace crown
{

    struct Mutex
    {
        struct Private* _priv;
        CE_ALIGN_DECL(16, u8 _data[64]);

        Mutex();
        ~Mutex();

        Mutex(const Mutex&) = delete;
        Mutex& operator=(const Mutex&) = delete;

        void lock();
        void unlock();
        void* native_handle();
    };

} // namespace crown
