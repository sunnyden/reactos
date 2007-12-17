/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS ReactX
 * FILE:            dll/directx/d3d9/d3d9_helpers.c
 * PURPOSE:         d3d9.dll helper functions
 * PROGRAMERS:      Gregor Brunmar <gregor (dot) brunmar (at) home (dot) se>
 */

#include "d3d9_helpers.h"
#include <stdio.h>
#include <ddraw.h>

#include "d3d9_private.h"

static LPCSTR D3dDebugRegPath = "Software\\Microsoft\\Direct3D";


BOOL ReadRegistryValue(IN DWORD ValueType, IN LPCSTR ValueName, OUT LPBYTE DataBuffer, IN OUT LPDWORD DataBufferSize)
{
    HKEY hKey;
    DWORD Type;
    LONG Ret;

    if (ERROR_SUCCESS != RegOpenKeyEx(HKEY_LOCAL_MACHINE, D3dDebugRegPath, 0, KEY_QUERY_VALUE, &hKey))
        return FALSE;

    Ret = RegQueryValueEx(hKey, ValueName, 0, &Type, DataBuffer, DataBufferSize);

    RegCloseKey(hKey);

    if (ERROR_SUCCESS != Ret)
        return FALSE;

    if (Type != ValueType)
        return FALSE;

    return TRUE;
}

HRESULT FormatDebugString(IN OUT LPSTR Buffer, IN LONG BufferSize, IN LPCSTR FormatString, ... )
{
    int BytesWritten;
    va_list vargs;

    if (BufferSize == 0)
        return DDERR_INVALIDPARAMS;

    va_start(vargs, FormatString);
    BytesWritten = _vsnprintf(Buffer, BufferSize-1, FormatString, vargs);

    if (BytesWritten < BufferSize)
        return DDERR_GENERIC;

    Buffer[BufferSize-1] = '\0';

    return 0;
}

HRESULT CreateD3D9(IDirect3D9** ppDirect3D9)
{
    LPDIRECTD3D9_INT pDirect3D9;

    if (ppDirect3D9 == 0)
        return DDERR_INVALIDPARAMS;

    pDirect3D9 = HeapAlloc(GetProcessHeap(), 0, sizeof(DIRECTD3D9_INT));

    if (0 == pDirect3D9)
        return DDERR_OUTOFMEMORY;

    pDirect3D9->unknown000007 = 0;
    pDirect3D9->lpInt = 0;

    //pDirect3D9->lpVtbl = &IDirect3D3_Vtbl;
    pDirect3D9->dwProcessId = GetCurrentThreadId();
    pDirect3D9->dwIntRefCnt = 1;

    *ppDirect3D9 = (IDirect3D9*)pDirect3D9;

    return ERROR_SUCCESS;
}
