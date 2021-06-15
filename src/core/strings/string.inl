/*
 * Copyright (C) 2021-2077 FATCROWN Team.
 * License: https://github.com/FatGraphicsLab/fatcrown/blob/main/LICENSE
 *
 * @author   kasicass@gmail.com
 * @date     2021-04-01
 */

#pragma once

#include "core/error/error.inl"
#include "core/platform.h"
#include "core/types.h"
#include <ctype.h>   // isspace
#include <stdarg.h>
#include <stdio.h>   // sscanf, vsnprintf
#include <string.h>

namespace crown
{

inline s32 vsnprintf(char* s, size_t n, const char* fmt, va_list args)
{
#if CROWN_COMPILER_MSVC
    s32 len = _vsnprintf_s(s, n, _TRUNCATE, fmt, args);
    return (len == 1) ? _vscprintf(fmt, args) : len;
#else
    return ::vsnprintf(s, n, fmt, args);
#endif
}

inline s32 snprintf(char* s, size_t n, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    s32 len = vsnprintf(s, n, fmt, args);
    va_end(args);
    return len;
}

inline u32 strlen32(const char* s)
{
    return (u32)strlen(s);
}

inline const char* skip_spaces(const char* s)
{
    while (isspace(*s))
        ++s;
    return s;
}

inline const char* skip_block(const char* s, char a, char b)
{
    u32 num = 0;

    for (char ch = *s++; ch != '\0'; ch = *s++)
    {
        if (ch == a)
        {
            ++num;
        }
        else if (ch == b)
        {
            if (--num == 0)
            {
                return s;
            }
        }
    }

    return NULL;
}

// Returns pointer after EOL.
inline const char* strnl(const char* s)
{
    const char* eol = strchr(s, '\n');
    return eol ? eol + 1 : s + strlen(s);
}

// Wildcard match, returns 1 if matched, or 0 otherwise.
inline int wildcmp(const char* wild, const char* s)
{
    const char *cp = NULL, *mp = NULL;

    while (*s && *wild != '*')
    {
        if (*wild != *s && *wild != '?')
            return 0;
        ++wild;
        ++s;
    }

    while (*s)
    {
        if (*wild == '*')
        {
            if (!*++wild)
                return 1;
            mp = wild;
            cp = s + 1;
        }
        else if (*wild == *s || *wild == '?')
        {
            ++wild;
            ++s;
        }
        else
        {
            wild = mp;
            s = cp++;
        }
    }

    while (*wild == '*')
        ++wild;

    return !*wild;
}

inline bool str_has_prefix(const char* s, const char* prefix)
{
    CE_ENSURE(NULL != s);
    CE_ENSURE(NULL != prefix);
    return strncmp(s, prefix, strlen32(prefix)) == 0;
}

inline bool str_has_suffix(const char* s, const char* suffix)
{
    CE_ENSURE(NULL != s);
    CE_ENSURE(NULL != suffix);
    const u32 ml = strlen32(s);
    const u32 sl = strlen32(suffix);
    return (sl <= ml) && (strncmp(s + ml - sl, suffix, sl) == 0);
}

} // namespace crown
