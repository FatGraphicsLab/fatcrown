#include "config.h"

#if CROWN_PLATFORM_WINDOWS

#define WIN32_LEAN_AND_MEAN
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

    Sleep(5*1000);

    return 0;
}

#endif

