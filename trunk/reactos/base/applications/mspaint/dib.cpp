/*
 * PROJECT:     PAINT for ReactOS
 * LICENSE:     LGPL
 * FILE:        base/applications/mspaint/dib.cpp
 * PURPOSE:     Some DIB related functions
 * PROGRAMMERS: Benedikt Freisen
 */

/* INCLUDES *********************************************************/

#include "precomp.h"

/* FUNCTIONS ********************************************************/

HBITMAP
CreateDIBWithProperties(int width, int height)
{
    BITMAPINFO bmi;
    ZeroMemory(&bmi, sizeof(BITMAPINFO));
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = width;
    bmi.bmiHeader.biHeight = height;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 24;
    bmi.bmiHeader.biCompression = BI_RGB;
    return CreateDIBSection(NULL, &bmi, DIB_RGB_COLORS, NULL, NULL, 0);
}

int
GetDIBWidth(HBITMAP hBitmap)
{
    BITMAP bm;
    GetObject(hBitmap, sizeof(BITMAP), &bm);
    return bm.bmWidth;
}

int
GetDIBHeight(HBITMAP hBitmap)
{
    BITMAP bm;
    GetObject(hBitmap, sizeof(BITMAP), &bm);
    return bm.bmHeight;
}

void
SaveDIBToFile(HBITMAP hBitmap, LPTSTR FileName, HDC hDC, LPSYSTEMTIME time, int *size, int hRes, int vRes)
{
    CImage img;
    img.Attach(hBitmap);
    img.Save(FileName);  // TODO: error handling
    img.Detach();

    // update time and size

    HANDLE hFile =
        CreateFile(FileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
        return;

    if (time)
    {
        FILETIME ft;
        GetFileTime(hFile, NULL, NULL, &ft);
        FileTimeToSystemTime(&ft, time);
    }
    if (size)
        *size = GetFileSize(hFile, NULL);

    // TODO: update hRes and vRes

    CloseHandle(hFile);
}

void ShowFileLoadError(LPCTSTR name)
{
    TCHAR programname[20];
    TCHAR loaderrortext[100];
    TCHAR temptext[500];
    LoadString(hProgInstance, IDS_PROGRAMNAME, programname, SIZEOF(programname));
    LoadString(hProgInstance, IDS_LOADERRORTEXT, loaderrortext, SIZEOF(loaderrortext));
    _stprintf(temptext, loaderrortext, name);
    mainWindow.MessageBox(temptext, programname, MB_OK | MB_ICONEXCLAMATION);
}

void
LoadDIBFromFile(HBITMAP * hBitmap, LPCTSTR name, LPSYSTEMTIME time, int *size, int *hRes, int *vRes)
{
    CImage img;
    img.Load(name);  // TODO: error handling

    if (!hBitmap)
    {
        ShowFileLoadError(name);
        return;
    }

    *hBitmap = img.Detach();

    // update time and size
    HANDLE hFile =
        CreateFile(name, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        ShowFileLoadError(name);
        return;
    }

    if (time)
    {
        FILETIME ft;
        GetFileTime(hFile, NULL, NULL, &ft);
        FileTimeToSystemTime(&ft, time);
    }
    if (size)
        *size = GetFileSize(hFile, NULL);

    // TODO: update hRes and vRes

    CloseHandle(hFile);
}
