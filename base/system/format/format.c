//======================================================================
//
// Formatx
//
// Copyright (c) 1998 Mark Russinovich
// Systems Internals
// http://www.sysinternals.com
//
// Format clone that demonstrates the use of the FMIFS file system
// utility library.
//
// --------------------------------------------------------------------
//
// This software is free software; you can redistribute it and/or
// modify it under the terms of the GNU Library General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This software is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// You should have received a copy of the GNU Library General Public
// License along with this software; see the file COPYING.LIB. If
// not, write to the Free Software Foundation, Inc., 675 Mass Ave,
// Cambridge, MA 02139, USA.
//
// --------------------------------------------------------------------
//
// 1999 February (Emanuele Aliberti)
//      Adapted for ReactOS and lcc-win32.
//
// 1999 April (Emanuele Aliberti)
//      Adapted for ReactOS and egcs.
//
// 2003 April (Casper S. Hornstrup)
//      Reintegration.
//
//======================================================================

#include <stdio.h>
#include <tchar.h>

/* PSDK/NDK Headers */
#define WIN32_NO_STATUS
#include <windef.h>
#include <winbase.h>
#include <winnls.h>
#include <winuser.h>

#define NTOS_MODE_USER
#include <ndk/rtlfuncs.h>

/* FMIFS Public Header */
#include <fmifs/fmifs.h>

#include "resource.h"

#define FMIFS_IMPORT_DLL

// Globals
BOOL    Error = FALSE;

// switches
BOOL    QuickFormat = FALSE;
DWORD   ClusterSize = 0;
BOOL    CompressDrive = FALSE;
BOOL    GotALabel = FALSE;
PWCHAR  Label = L"";
PWCHAR  Drive = NULL;
PWCHAR  FileSystem = L"FAT";

WCHAR   RootDirectory[MAX_PATH];
WCHAR   LabelString[12];

#ifndef FMIFS_IMPORT_DLL
//
// Functions in FMIFS.DLL
//
PFORMATEX FormatEx;
PENABLEVOLUMECOMPRESSION EnableVolumeCompression;
PQUERYAVAILABLEFILESYSTEMFORMAT QueryAvailableFileSystemFormat;
#endif


//
// Size array
//
typedef struct {
    WCHAR SizeString[16];
    DWORD ClusterSize;
} SIZEDEFINITION, *PSIZEDEFINITION;

SIZEDEFINITION LegalSizes[] = {
    { L"512", 512 },
    { L"1024", 1024 },
    { L"2048", 2048 },
    { L"4096", 4096 },
    { L"8192", 8192 },
    { L"16K", 16384 },
    { L"32K", 32768 },
    { L"64K", 65536 },
    { L"128K", 65536 * 2 },
    { L"256K", 65536 * 4 },
    { L"", 0 },
};


VOID PrintStringV(LPWSTR szStr, va_list args)
{
    WCHAR bufFormatted[RC_STRING_MAX_SIZE];
    CHAR bufFormattedOem[RC_STRING_MAX_SIZE];

    _vsnwprintf(bufFormatted, ARRAYSIZE(bufFormatted), szStr, args);

    CharToOemW(bufFormatted, bufFormattedOem);
    puts(bufFormattedOem);
}

VOID PrintString(LPWSTR szStr, ...)
{
    va_list args;

    va_start(args, szStr);
    PrintStringV(szStr, args);
    va_end(args);
}

VOID PrintResourceString(UINT uID, ...)
{
    WCHAR bufSrc[RC_STRING_MAX_SIZE];
    va_list args;

    LoadStringW(GetModuleHandleW(NULL), uID, bufSrc, ARRAYSIZE(bufSrc));

    va_start(args, uID);
    PrintStringV(bufSrc, args);
    va_end(args);
}


//----------------------------------------------------------------------
//
// PrintWin32Error
//
// Takes the win32 error code and prints the text version.
//
//----------------------------------------------------------------------
static VOID PrintWin32Error(LPWSTR Message, DWORD ErrorCode)
{
    LPWSTR lpMsgBuf;

    FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                   NULL, ErrorCode,
                   MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                   (LPWSTR)&lpMsgBuf, 0, NULL);

    PrintString(L"%s: %s\n", Message, lpMsgBuf);
    LocalFree(lpMsgBuf);
}


//----------------------------------------------------------------------
//
// ParseCommandLine
//
// Get the switches.
//
//----------------------------------------------------------------------
static int ParseCommandLine(int argc, WCHAR *argv[])
{
    int i, j;
    BOOLEAN gotFormat = FALSE;
    BOOLEAN gotQuick = FALSE;
    BOOLEAN gotSize = FALSE;
    BOOLEAN gotLabel = FALSE;
    BOOLEAN gotCompressed = FALSE;

    for (i = 1; i < argc; i++)
    {
        switch (argv[i][0])
        {
            case L'-': case L'/':

                if (!_wcsnicmp(&argv[i][1], L"FS:", 3))
                {
                    if (gotFormat) return -1;
                    FileSystem = &argv[i][4];
                    gotFormat = TRUE;
                }
                else if (!_wcsnicmp(&argv[i][1], L"A:", 2))
                {
                    if (gotSize) return -1;
                    j = 0;
                    while (LegalSizes[j].ClusterSize &&
                           wcsicmp(LegalSizes[j].SizeString, &argv[i][3]))
                    {
                        j++;
                    }

                    if (!LegalSizes[j].ClusterSize) return i;
                    ClusterSize = LegalSizes[j].ClusterSize;
                    gotSize = TRUE;
                }
                else if (!_wcsnicmp(&argv[i][1], L"V:", 2))
                {
                    if (gotLabel) return -1;
                    Label = &argv[i][3];
                    gotLabel = TRUE;
                    GotALabel = TRUE;
                }
                else if (!wcsicmp(&argv[i][1], L"Q"))
                {
                    if (gotQuick) return -1;
                    QuickFormat = TRUE;
                    gotQuick = TRUE;
                }
                else if (!wcsicmp(&argv[i][1], L"C"))
                {
                    if (gotCompressed) return -1;
                    CompressDrive = TRUE;
                    gotCompressed = TRUE;
                }
                else
                {
                    return i;
                }
                break;

            default:
            {
                if (Drive) return i;
                if (argv[i][1] != L':') return i;

                Drive = argv[i];
                break;
            }
        }
    }
    return 0;
}

//----------------------------------------------------------------------
//
// FormatExCallback
//
// The file system library will call us back with commands that we
// can interpret. If we wanted to halt the chkdsk we could return FALSE.
//
//----------------------------------------------------------------------
BOOLEAN WINAPI
FormatExCallback(
    CALLBACKCOMMAND Command,
    ULONG Modifier,
    PVOID Argument)
{
    PDWORD      percent;
    PTEXTOUTPUT output;
    PBOOLEAN    status;

    //
    // We get other types of commands, but we don't have to pay attention to them
    //
    switch (Command)
    {
        case PROGRESS:
            percent = (PDWORD)Argument;
            PrintResourceString(STRING_COMPLETE, *percent);
            break;

        case OUTPUT:
            output = (PTEXTOUTPUT)Argument;
            wprintf(L"%S", output->Output);
            break;

        case DONE:
            status = (PBOOLEAN)Argument;
            if (*status == FALSE)
            {
                PrintResourceString(STRING_FORMAT_FAIL);
                Error = TRUE;
            }
            break;

        case DONEWITHSTRUCTURE:
        case UNKNOWN2:
        case UNKNOWN3:
        case UNKNOWN4:
        case UNKNOWN5:
        case INSUFFICIENTRIGHTS:
        case FSNOTSUPPORTED:
        case VOLUMEINUSE:
        case UNKNOWN9:
        case UNKNOWNA:
        case UNKNOWNC:
        case UNKNOWND:
        case STRUCTUREPROGRESS:
        case CLUSTERSIZETOOSMALL:
            PrintResourceString(STRING_NO_SUPPORT);
            return FALSE;
    }
    return TRUE;
}

#ifndef FMIFS_IMPORT_DLL
//----------------------------------------------------------------------
//
// LoadFMIFSEntryPoints
//
// Loads FMIFS.DLL and locates the entry point(s) we are going to use
//
//----------------------------------------------------------------------
static BOOLEAN LoadFMIFSEntryPoints(VOID)
{
    HMODULE hFmifs = LoadLibraryW( L"fmifs.dll");
    if (hFmifs == NULL)
        return FALSE;

    FormatEx = (PFORMATEX)GetProcAddress(hFmifs, "FormatEx");
    if (!FormatEx)
    {
        FreeLibrary(hFmifs);
        return FALSE;
    }

    EnableVolumeCompression = (PENABLEVOLUMECOMPRESSION)GetProcAddress(hFmifs, "EnableVolumeCompression");
    if (!EnableVolumeCompression)
    {
        FreeLibrary(hFmifs);
        return FALSE;
    }

    QueryAvailableFileSystemFormat = (PQUERYAVAILABLEFILESYSTEMFORMAT)GetProcAddress(hFmifs, "QueryAvailableFileSystemFormat");
    if (!QueryAvailableFileSystemFormat)
    {
        FreeLibrary(hFmifs);
        return FALSE;
    }

    return TRUE;
}
#endif


//----------------------------------------------------------------------
//
// Usage
//
// Tell the user how to use the program
//
//----------------------------------------------------------------------
static VOID Usage(LPWSTR ProgramName)
{
    WCHAR szMsg[RC_STRING_MAX_SIZE];
    WCHAR szFormats[MAX_PATH];
    WCHAR szFormatW[MAX_PATH];
    DWORD Index = 0;
    BYTE dummy;
    BOOLEAN latestVersion;

    LoadStringW(GetModuleHandle(NULL), STRING_HELP, szMsg, ARRAYSIZE(szMsg));

#ifndef FMIFS_IMPORT_DLL
    if (!LoadFMIFSEntryPoints())
    {
        PrintString(szMsg, ProgramName, L"");
        return;
    }
#endif

    szFormats[0] = 0;
    while (QueryAvailableFileSystemFormat(Index++, szFormatW, &dummy, &dummy, &latestVersion))
    {
        if (!latestVersion)
            continue;
        if (szFormats[0])
            wcscat(szFormats, L", ");

        wcscat(szFormats, szFormatW);
    }
    PrintString(szMsg, ProgramName, szFormats);
}


//----------------------------------------------------------------------
//
// WMain
//
// Engine. Just get command line switches and fire off a format. This
// could also be done in a GUI like Explorer does when you select a
// drive and run a check on it.
//
// We do this in UNICODE because the chkdsk command expects PWCHAR
// arguments.
//
//----------------------------------------------------------------------
int wmain(int argc, WCHAR *argv[])
{
    int badArg;
    DWORD media = FMIFS_HARDDISK;
    DWORD driveType;
    WCHAR fileSystem[1024];
    WCHAR volumeName[1024];
    WCHAR input[1024];
    DWORD serialNumber;
    DWORD flags, maxComponent;
    ULARGE_INTEGER freeBytesAvailableToCaller, totalNumberOfBytes, totalNumberOfFreeBytes;
    WCHAR szMsg[RC_STRING_MAX_SIZE];

    wprintf(L"\n"
            L"Formatx v1.0 by Mark Russinovich\n"
            L"Systems Internals - http://www.sysinternals.com\n"
            L"ReactOS adaptation 1999 by Emanuele Aliberti\n\n");

#ifndef FMIFS_IMPORT_DLL
    //
    // Get function pointers
    //
    if (!LoadFMIFSEntryPoints())
    {
        PrintResourceString(STRING_FMIFS_FAIL);
        return -1;
    }
#endif

    //
    // Parse command line
    //
    badArg = ParseCommandLine(argc, argv);
    if (badArg)
    {
        PrintResourceString(STRING_UNKNOW_ARG, argv[badArg]);
        Usage(argv[0]);
        return -1;
    }

    //
    // Get the drive's format
    //
    if (!Drive)
    {
        PrintResourceString(STRING_DRIVE_PARM);
        Usage(argv[0]);
        return -1;
    }
    else
    {
        wcscpy(RootDirectory, Drive);
    }
    RootDirectory[2] = L'\\';
    RootDirectory[3] = L'\0';

    //
    // See if the drive is removable or not
    //
    driveType = GetDriveTypeW(RootDirectory);
    switch (driveType)
    {
        case DRIVE_UNKNOWN :
            LoadStringW(GetModuleHandle(NULL), STRING_ERROR_DRIVE_TYPE, szMsg, ARRAYSIZE(szMsg));
            PrintWin32Error(szMsg, GetLastError());
            return -1;

        case DRIVE_REMOTE:
        case DRIVE_CDROM:
            PrintResourceString(STRING_NO_SUPPORT);
            return -1;

        case DRIVE_NO_ROOT_DIR:
            LoadStringW(GetModuleHandle(NULL), STRING_NO_VOLUME, szMsg, ARRAYSIZE(szMsg));
            PrintWin32Error(szMsg, GetLastError());
            return -1;

        case DRIVE_REMOVABLE:
            PrintResourceString(STRING_INSERT_DISK, RootDirectory[0]);
            fgetws(input, ARRAYSIZE(input), stdin);
            media = FMIFS_FLOPPY;
            break;

        case DRIVE_FIXED:
        case DRIVE_RAMDISK:
            media = FMIFS_HARDDISK;
            break;
    }

    // Reject attempts to format the system drive
    {
        WCHAR path[MAX_PATH + 1];
        UINT rc;
        rc = GetWindowsDirectoryW(path, MAX_PATH);
        if (rc == 0 || rc > MAX_PATH)
            // todo: Report "Unable to query system directory"
            return -1;
        if (towlower(path[0]) == towlower(Drive[0]))
        {
            // todo: report "Cannot format system drive"
            PrintResourceString(STRING_NO_SUPPORT);
            return -1;
        }
    }

    //
    // Determine the drive's file system format
    //
    if (!GetVolumeInformationW(RootDirectory,
                               volumeName, ARRAYSIZE(volumeName),
                               &serialNumber, &maxComponent, &flags,
                               fileSystem, ARRAYSIZE(fileSystem)))
    {
        LoadStringW(GetModuleHandle(NULL), STRING_NO_VOLUME, szMsg, ARRAYSIZE(szMsg));
        PrintWin32Error(szMsg, GetLastError());
        return -1;
    }

    if (!GetDiskFreeSpaceExW(RootDirectory,
                             &freeBytesAvailableToCaller,
                             &totalNumberOfBytes,
                             &totalNumberOfFreeBytes))
    {
        LoadStringW(GetModuleHandle(NULL), STRING_NO_VOLUME_SIZE, szMsg, ARRAYSIZE(szMsg));
        PrintWin32Error(szMsg, GetLastError());
        return -1;
    }
    PrintResourceString(STRING_FILESYSTEM, fileSystem);

    //
    // Make sure they want to do this
    //
    if (driveType == DRIVE_FIXED)
    {
        if (volumeName[0])
        {
            while (TRUE)
            {
                PrintResourceString(STRING_LABEL_NAME_EDIT, RootDirectory[0]);
                fgetws(input, ARRAYSIZE(input), stdin);
                input[wcslen(input) - 1] = 0;

                if (!wcsicmp(input, volumeName))
                    break;

                PrintResourceString(STRING_ERROR_LABEL);
            }
        }

        PrintResourceString(STRING_YN_FORMAT, RootDirectory[0]);

        LoadStringW(GetModuleHandle(NULL), STRING_YES_NO_FAQ, szMsg, ARRAYSIZE(szMsg));
        while (TRUE)
        {
            fgetws(input, ARRAYSIZE(input), stdin);
            if (_wcsnicmp(&input[0], &szMsg[0], 1) == 0) break;
            if (_wcsnicmp(&input[0], &szMsg[1], 1) == 0)
            {
                wprintf(L"\n");
                return 0;
            }
        }
    }

    //
    // Tell the user we're doing a long format if appropriate
    //
    if (!QuickFormat)
    {
        LoadStringW(GetModuleHandle(NULL), STRING_VERIFYING, szMsg, ARRAYSIZE(szMsg));
        if (totalNumberOfBytes.QuadPart > 1024*1024*10)
        {
            PrintString(L"%s %luM\n", szMsg, (DWORD)(totalNumberOfBytes.QuadPart/(1024*1024)));
        }
        else
        {
            PrintString(L"%s %.1fM\n", szMsg,
                ((float)(LONGLONG)totalNumberOfBytes.QuadPart)/(float)(1024.0*1024.0));
        }
    }
    else
    {
        LoadStringW(GetModuleHandle(NULL), STRING_FAST_FMT, szMsg, ARRAYSIZE(szMsg));
        if (totalNumberOfBytes.QuadPart > 1024*1024*10)
        {
            PrintString(L"%s %luM\n", szMsg, (DWORD)(totalNumberOfBytes.QuadPart/(1024*1024)));
        }
        else
        {
            PrintString(L"%s %.2fM\n", szMsg,
                ((float)(LONGLONG)totalNumberOfBytes.QuadPart)/(float)(1024.0*1024.0));
        }
        PrintResourceString(STRING_CREATE_FSYS);
    }

    //
    // Format away!
    //
    FormatEx(RootDirectory, media, FileSystem, Label, QuickFormat,
             ClusterSize, FormatExCallback);
    if (Error) return -1;
    PrintResourceString(STRING_FMT_COMPLETE);

    //
    // Enable compression if desired
    //
    if (CompressDrive)
    {
        if (!EnableVolumeCompression(RootDirectory, TRUE))
            PrintResourceString(STRING_VOL_COMPRESS);
    }

    //
    // Get the label if we don't have it
    //
    if (!GotALabel)
    {
        PrintResourceString(STRING_ENTER_LABEL);
        fgetws(input, ARRAYSIZE(LabelString), stdin);

        input[wcslen(input) - 1] = 0;
        if (!SetVolumeLabelW(RootDirectory, input))
        {
            LoadStringW(GetModuleHandle(NULL), STRING_NO_LABEL, szMsg, ARRAYSIZE(szMsg));
            PrintWin32Error(szMsg, GetLastError());
            return -1;
        }
    }

    if (!GetVolumeInformationW(RootDirectory,
                               volumeName, ARRAYSIZE(volumeName),
                               &serialNumber, &maxComponent, &flags,
                               fileSystem, ARRAYSIZE(fileSystem)))
    {
        LoadStringW(GetModuleHandle(NULL), STRING_NO_VOLUME, szMsg, ARRAYSIZE(szMsg));
        PrintWin32Error(szMsg, GetLastError());
        return -1;
    }

    //
    // Print out some stuff including the formatted size
    //
    if (!GetDiskFreeSpaceExW(RootDirectory,
                             &freeBytesAvailableToCaller,
                             &totalNumberOfBytes,
                             &totalNumberOfFreeBytes))
    {
        LoadStringW(GetModuleHandle(NULL), STRING_NO_VOLUME_SIZE, szMsg, ARRAYSIZE(szMsg));
        PrintWin32Error(szMsg, GetLastError());
        return -1;
    }

    PrintResourceString(STRING_FREE_SPACE, totalNumberOfBytes.QuadPart,
                                           totalNumberOfFreeBytes.QuadPart);

    //
    // Get the drive's serial number
    //
    if (!GetVolumeInformationW(RootDirectory,
                               volumeName, ARRAYSIZE(volumeName),
                               &serialNumber, &maxComponent, &flags,
                               fileSystem, ARRAYSIZE(fileSystem)))
    {
        LoadStringW(GetModuleHandle(NULL), STRING_NO_VOLUME, szMsg, ARRAYSIZE(szMsg));
        PrintWin32Error(szMsg, GetLastError());
        return -1;
    }
    PrintResourceString(STRING_SERIAL_NUMBER,
                        (unsigned int)(serialNumber >> 16),
                        (unsigned int)(serialNumber & 0xFFFF));

    return 0;
}

/* EOF */
