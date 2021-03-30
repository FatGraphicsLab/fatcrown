/*
 * Copyright (C) 2021-2077 FATCROWN Team.
 * License: https://github.com/FatGraphicsLab/fatcrown/blob/main/LICENSE
 *
 * @author   kasicass@gmail.com
 * @date     2021-03-30
 */

#pragma once

#include "core/thread/mutex.h"

namespace crown
{

    // Automatically locks a mutex when created and unlocks when destroyed.
    struct ScopedMutex
    {
        Mutex& _mutex;

        ScopedMutex(Mutex& m) : _mutex(m)
        {
            _mutex.lock();
        }

        ~ScopedMutex()
        {
            _mutex.unlock();
        }

        ScopedMutex(const ScopedMutex&) = delete;
        ScopedMutex& operator=(const ScopedMutex&) = delete;
    };

} // namespace crown
