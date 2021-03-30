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

    template <typename T>
    struct equal_to
    {
        bool operator()(const T& a, const T& b) const;
    };

    template <typename T>
    struct not_equal_to
    {
        bool operator()(const T& a, const T& b) const;
    };

    template <typename T>
    struct greater
    {
        bool operator()(const T& a, const T& b) const;
    };

    template <typename T>
    struct less
    {
        bool operator()(const T& a, const T& b) const;
    };

    template <typename T>
    struct greater_equal
    {
        bool operator()(const T& a, const T& b) const;
    };

    template <typename T>
    struct less_equal
    {
        bool operator()(const T& a, const T& b) const;
    };

    template <typename T>
    struct hash;

    template<>
    struct hash<bool>
    {
        u32 operator()(const bool val) const;
    };

    template<>
    struct hash<s8>
    {
        u32 operator()(const s8 val) const;
    };

    template<>
    struct hash<u8>
    {
        u32 operator()(const u8 val) const;
    };

    template<>
    struct hash<s16>
    {
        u32 operator()(const s16 val) const;
    };

    template<>
    struct hash<u16>
    {
        u32 operator()(const u16 val) const;
    };

    template<>
    struct hash<s32>
    {
        u32 operator()(const s32 val) const;
    };

    template<>
    struct hash<u32>
    {
        u32 operator()(const u32 val) const;
    };

    template<>
    struct hash<s64>
    {
        u32 operator()(const s64 val) const;
    };

    template<>
    struct hash<u64>
    {
        u32 operator()(const u64 val) const;
    };

    template<>
    struct hash<f32>
    {
        u32 operator()(const f32 val) const;
    };

    template<>
    struct hash<f64>
    {
        u32 operator()(const f64 val) const;
    };

} // namespace crown
