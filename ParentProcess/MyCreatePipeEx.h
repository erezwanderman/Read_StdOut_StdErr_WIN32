#pragma once
#include <windows.h>
#include <stdio.h>
#include <tchar.h>

BOOL
APIENTRY
MyCreatePipeEx(
    OUT PHANDLE hReadPipe,
    OUT PHANDLE hWritePipe,
    IN LPSECURITY_ATTRIBUTES lpPipeAttributes,
    IN DWORD nSize,
    DWORD dwReadMode,
    DWORD dwWriteMode
);
