/*
 * Copyright (C) 2021-2077 FATCROWN Team.
 * License: https://github.com/FatGraphicsLab/fatcrown/blob/main/LICENSE
 *
 * @author   kasicass@gmail.com
 * @date     2021-03-27
 */

#include "config.h"
#include "core/error/error.inl"
#include "core/unit_tests.h"

#if CROWN_PLATFORM_WINDOWS

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include <stdio.h>
#include <stdlib.h>


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrecInstance, LPSTR lpCmdLine, int nShowCmd)
{
    if (AttachConsole(ATTACH_PARENT_PROCESS) != 0)
    {
        freopen("CONIN$", "r", stdin);
        freopen("CONOUT$", "w", stdout);
        freopen("CONOUT$", "w", stderr);
    }

    // TODO: WSAStartup()
    // CE_ASSERT(10 > 1, "hello");

    // Sleep(5*1000);

    crown::main_unit_tests();

    return 0;
}

#endif
