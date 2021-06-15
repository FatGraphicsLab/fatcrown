/*
 * Copyright (C) 2021-2077 FATCROWN Team.
 * License: https://github.com/FatGraphicsLab/fatcrown/blob/main/LICENSE
 *
 * @author   kasicass@gmail.com
 * @date     2021-03-29
 */

#include "core/error/callstack.h"
#include "core/error/error.h"
#include "core/memory/temp_allocator.inl"
#include "core/strings/string_stream.inl"
// #include "device/log.h" // TODO(kasicass)
#include <stdarg.h>
#include <stdlib.h> // exit

#define WIN32_LEAN_AND_MEAN
#include <Windows.h> // TODO: remove me

// LOG_SYSTEM(ERROR, "error")

namespace crown { namespace error {

    static void vabort(const char* format, va_list args)
    {
        char buf[1024];
        vsnprintf(buf, sizeof(buf), format, args);

        TempAllocator4096 ta;
        StringStream ss(ta);
        ss << buf;
        ss << "Stacktrack:\n";
        callstack(ss);

        // loge(ERROR, string_stream::c_str(ss));
        printf("%s\n", string_stream::c_str(ss)); // TODO: remove me
        OutputDebugStringA(string_stream::c_str(ss));
        exit(EXIT_FAILURE);
    }

    void abort(const char* format, ...)
    {
        va_list args;
        va_start(args, format);
        vabort(format, args);
        va_end(args);
    }

}} // namespace crown::error
