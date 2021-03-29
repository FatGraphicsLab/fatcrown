/*
 * Copyright (C) 2021-2077 FATCROWN Team.
 * License: https://github.com/FatGraphicsLab/fatcrown/blob/main/LICENSE
 *
 * @author   kasicass@gmail.com
 * @date     2021-03-29
 */

#pragma once

#include "core/error/error.h"

#if CROWN_DEBUG
#  define CE_ASSERT(condition, msg, ...)                        \
        do                                                      \
        {                                                       \
            if (CE_UNLIKELY(!(condition)))                      \
            {                                                   \
                crown::error::abort("Assertion failed: %s\n"    \
                    "    In: %s:%d\n"                           \
                    "    " msg "\n"                             \
                    , #condition                                \
                    , __FILE__                                  \
                    , __LINE__                                  \
                    , ##__VA_ARGS__                             \
                    );                                          \
                CE_UNREACHABLE();                               \
            }                                                   \
        } while (0)
#else
#  define CE_ASSERT(...) CE_NOOP()
#endif

#define CE_FATAL(msg, ...) CE_ASSERT(false, msg, ##__VA_ARGS__)
#define CE_ENSURE(condition) CE_ASSERT(condition, "")
