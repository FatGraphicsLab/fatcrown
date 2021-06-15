/*
 * Copyright (C) 2021-2077 FATCROWN Team.
 * License: https://github.com/FatGraphicsLab/fatcrown/blob/main/LICENSE
 *
 * @author   kasicass@gmail.com
 * @date     2021-06-15
 */

#include "core/platform.h"

#if CROWN_PLATFORM_WINDOWS

#include "core/strings/string_stream.inl"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <DbgHelp.h>

namespace crown { namespace error {

    void callstack(StringStream& ss)
    {
        CONTEXT ctx;
        STACKFRAME64 stack;
        DWORD machineType;
        HANDLE hProcess;

        hProcess = GetCurrentProcess();

        SymInitializeW(hProcess, NULL, TRUE);
        SymSetOptions(SYMOPT_LOAD_LINES | SYMOPT_UNDNAME);

        ZeroMemory(&ctx, sizeof(ctx));
        ctx.ContextFlags = CONTEXT_CONTROL;
        RtlCaptureContext(&ctx);

        ZeroMemory(&stack, sizeof(stack));
#if defined(_M_IX86)
        machineType = IMAGE_FILE_MACHINE_I386;
        stack.AddrPC.Offset    = ctx.Eip;
        stack.AddrPC.Mode      = AddrModeFlat;
        stack.AddrFrame.Offset = ctx.Ebp;
        stack.AddrFrame.Mode   = AddrModeFlat;
        stack.AddrStack.Offset = ctx.Esp;
        stack.AddrStack.Mode   = AddrModeFlat;
#elif defined(_M_X64)
            machineType = IMAGE_FILE_MACHINE_AMD64;
        stack.AddrPC.Offset    = ctx.Rip;
        stack.AddrPC.Mode      = AddrModeFlat;
        stack.AddrFrame.Offset = ctx.Rbp;
        stack.AddrFrame.Mode   = AddrModeFlat;
        stack.AddrStack.Offset = ctx.Rsp;
        stack.AddrStack.Mode   = AddrModeFlat;
#endif

        DWORD dwDisplacement;
        IMAGEHLP_LINE64 line;
        ZeroMemory(&line, sizeof(line));
        line.SizeOfStruct = sizeof(line);

        SYMBOL_INFO_PACKAGE sym_package;
        ZeroMemory(&sym_package, sizeof(sym_package));
        SYMBOL_INFO* sym  = &sym_package.si;
        sym->SizeOfStruct = sizeof(SYMBOL_INFO);
        sym->MaxNameLen   = MAX_SYM_NAME;
        
        UINT num = 0;
        while (StackWalk64(machineType, hProcess, GetCurrentThread(),
            &stack, &ctx, NULL, SymFunctionTableAccess64, SymGetModuleBase64, NULL))
        {
            if (stack.AddrPC.Offset == 0)
                break;

            ++num;

            BOOL ok = SymGetLineFromAddr64(hProcess, stack.AddrPC.Offset, &dwDisplacement, &line);
            ok = ok && SymFromAddr(hProcess, stack.AddrPC.Offset, NULL, sym);

            char linestr[1024];
            if (ok)
            {
                snprintf(linestr, sizeof(linestr), "    [%2i] %s in %s:%d\n", num,
                    sym->Name, line.FileName, line.LineNumber);
            }
            else
            {
                snprintf(linestr, sizeof(linestr), "    [%2i] 0x%p\n", num, stack.AddrPC.Offset);
            }

            ss << linestr;
        }
    }

}} // namespace crown::error

#endif
