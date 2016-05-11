/*
 * Program Manager
 *
 * Copyright 1996 Ulrich Schmid
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

/*
 * PROJECT:         ReactOS Program Manager
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * FILE:            base/shell/progman/group.c
 * PURPOSE:         Program group files helper functions
 * PROGRAMMERS:     Ulrich Schmid
 *                  Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

#include "progman.h"

/***********************************************************************
 *
 *           GROUP_GroupWndProc
 */

static
LRESULT
CALLBACK
GROUP_GroupWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    PROGGROUP* group;

    group = (PROGGROUP*)GetWindowLongPtrW(hWnd, 0);

    switch (uMsg)
    {
        case WM_NCCREATE:
        {
            LPCREATESTRUCTW pcs = (LPCREATESTRUCTW)lParam;
            LPMDICREATESTRUCTW pMDIcs = (LPMDICREATESTRUCTW)pcs->lpCreateParams;
            group = (PROGGROUP*)pMDIcs->lParam;
            SetWindowLongPtrW(hWnd, 0, (LONG_PTR)group);

            if (group->bIsCommonGroup)
            {
                DefMDIChildProcW(hWnd, WM_SETICON, ICON_BIG,
                                 (LPARAM)CopyImage(Globals.hCommonGroupIcon,
                                                   IMAGE_ICON,
                                                   GetSystemMetrics(SM_CXICON),
                                                   GetSystemMetrics(SM_CYICON),
                                                   LR_COPYFROMRESOURCE));
                DefMDIChildProcW(hWnd, WM_SETICON, ICON_SMALL,
                                 (LPARAM)CopyImage(Globals.hCommonGroupIcon,
                                                   IMAGE_ICON,
                                                   GetSystemMetrics(SM_CXSMICON),
                                                   GetSystemMetrics(SM_CYSMICON),
                                                   LR_COPYFROMRESOURCE));
            }
            else
            {
                DefMDIChildProcW(hWnd, WM_SETICON, ICON_BIG,
                                 (LPARAM)CopyImage(Globals.hPersonalGroupIcon,
                                                   IMAGE_ICON,
                                                   GetSystemMetrics(SM_CXICON),
                                                   GetSystemMetrics(SM_CYICON),
                                                   LR_COPYFROMRESOURCE));
                DefMDIChildProcW(hWnd, WM_SETICON, ICON_SMALL,
                                 (LPARAM)CopyImage(Globals.hPersonalGroupIcon,
                                                   IMAGE_ICON,
                                                   GetSystemMetrics(SM_CXSMICON),
                                                   GetSystemMetrics(SM_CYSMICON),
                                                   LR_COPYFROMRESOURCE));
            }
            break;
        }

        case WM_NCDESTROY:
            SetWindowLongPtrW(hWnd, 0, 0);
            break;

        case WM_CLOSE:
            SendMessageW(hWnd, WM_SYSCOMMAND, SC_MINIMIZE, 0);
            break;

        case WM_SYSCOMMAND:
            if (wParam == SC_CLOSE) wParam = SC_MINIMIZE;
            break;

        case WM_CHILDACTIVATE:
        case WM_NCLBUTTONDOWN:
            Globals.hActiveGroup = (PROGGROUP*)GetWindowLongPtrW(hWnd, 0);
            Globals.hActiveGroup->hActiveProgram = NULL;
            break;
    }

    return DefMDIChildProcW(hWnd, uMsg, wParam, lParam);
}

/***********************************************************************
 *
 *           GROUP_RegisterGroupWinClass
 */

ATOM GROUP_RegisterGroupWinClass(VOID)
{
    WNDCLASSW wndClass;

    wndClass.style         = CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW;
    wndClass.lpfnWndProc   = GROUP_GroupWndProc;
    wndClass.cbClsExtra    = 0;
    wndClass.cbWndExtra    = sizeof(LONG_PTR);
    wndClass.hInstance     = Globals.hInstance;
    wndClass.hIcon         = LoadIconW(Globals.hInstance, MAKEINTRESOURCEW(IDI_GROUP_ICON));
    wndClass.hCursor       = LoadCursorW(NULL, MAKEINTRESOURCEW(IDC_ARROW));
    wndClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wndClass.lpszMenuName  = NULL;
    wndClass.lpszClassName = STRING_GROUP_WIN_CLASS_NAME;

    return RegisterClassW(&wndClass);
}

/***********************************************************************
 *
 *           GROUP_NewGroup
 */

VOID GROUP_NewGroup(GROUPFORMAT format, BOOL bIsCommonGroup)
{
    HANDLE hFile;
    WCHAR szGrpFile[MAX_PATHNAME_LEN] = L"";
    WCHAR szTitle[MAX_PATHNAME_LEN]   = L"";

    // ZeroMemory(szTitle, sizeof(szTitle));
    // ZeroMemory(szGrpFile, sizeof(szGrpFile));

    if (!DIALOG_GroupAttributes(format, szTitle, szGrpFile, MAX_PATHNAME_LEN))
        return;

    /*
     * Just check whether the group file does exist. If it does, close the handle, because GRPFILE_ReadGroupFile will
     * reopen the file for loading. If it doesn't exist, we create a new one.
     */
    hFile = CreateFileW(szGrpFile, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        /* File doesn't exist */
        PROGGROUP* hGroup = GROUP_AddGroup(format, bIsCommonGroup, szTitle, szGrpFile,
                                           DEF_GROUP_WIN_XPOS, DEF_GROUP_WIN_YPOS,
                                           DEF_GROUP_WIN_XPOS + DEF_GROUP_WIN_WIDTH, DEF_GROUP_WIN_YPOS + DEF_GROUP_WIN_HEIGHT,
                                           0, 0, SW_SHOWNORMAL, 0, 0, FALSE, FALSE);
        if (hGroup)
            GRPFILE_WriteGroupFile(hGroup);
    }
    else
    {
        /* File exist */
        CloseHandle(hFile);
        GRPFILE_ReadGroupFile(szGrpFile, bIsCommonGroup);
    }

    /* FIXME Update progman.ini */
}

/***********************************************************************
 *
 *           GROUP_AddGroup
 */

PROGGROUP*
GROUP_AddGroup(GROUPFORMAT format, BOOL bIsCommonGroup, LPCWSTR lpszName, LPCWSTR lpszGrpFile,
               INT left, INT top, INT right, INT bottom, INT xMin, INT yMin, INT nCmdShow,
               WORD cxIcon, WORD cyIcon, BOOL bOverwriteFileOk,
               /* FIXME shouldn't be necessary */
               BOOL bSuppressShowWindow)
{
    PROGGROUP* hGroup;
    PROGGROUP* hPrior;
    PROGGROUP** p;
    LPWSTR hName;
    LPWSTR hGrpFile;
    LPCWSTR GroupFileName;
    INT skip;
    INT width;
    INT height;
    INT seqnum;
    MDICREATESTRUCTW mcs;
    WINDOWPLACEMENT WndPl;

    WndPl.length = sizeof(WndPl);

    // FIXME: Use system default position in case we don't place the window at a given (x,y) coordinate.

    if (bIsCommonGroup)
    {
        if (swscanf(lpszGrpFile,
                    L"%d %d %d %d %d %d %d %n",
                    &WndPl.rcNormalPosition.left,
                    &WndPl.rcNormalPosition.top,
                    &WndPl.rcNormalPosition.right,
                    &WndPl.rcNormalPosition.bottom,
                    &WndPl.ptMinPosition,
                    &WndPl.ptMinPosition.y,
                    &WndPl.showCmd,
                    &skip) == 7)
        {
            WndPl.flags = WPF_SETMINPOSITION;
            width  = WndPl.rcNormalPosition.right  - WndPl.rcNormalPosition.left;
            height = WndPl.rcNormalPosition.bottom - WndPl.rcNormalPosition.top;
            GroupFileName = &lpszGrpFile[skip];
        }
        else
        {
#if 0 // FIXME!
            WndPl.rcNormalPosition.top    = CW_USEDEFAULT;
            WndPl.rcNormalPosition.left   = CW_USEDEFAULT;
            WndPl.rcNormalPosition.right  = 0;
            WndPl.rcNormalPosition.bottom = 0;
            width  = CW_USEDEFAULT;
            height = CW_USEDEFAULT;
            WndPl.showCmd = SW_SHOWNORMAL;
            GroupFileName = lpszGrpFile;
#else
        WndPl.flags = WPF_SETMINPOSITION;
        WndPl.ptMinPosition.x = xMin;
        WndPl.ptMinPosition.y = yMin;
        WndPl.rcNormalPosition.left   = left;
        WndPl.rcNormalPosition.top    = top;
        WndPl.rcNormalPosition.right  = right;
        WndPl.rcNormalPosition.bottom = bottom;
        width  = right  - left;
        height = bottom - top;
        WndPl.showCmd = nCmdShow;
        GroupFileName = lpszGrpFile;
#endif
        }
    }
    else
    {
        WndPl.flags = WPF_SETMINPOSITION;
        WndPl.ptMinPosition.x = xMin;
        WndPl.ptMinPosition.y = yMin;
        WndPl.rcNormalPosition.left   = left;
        WndPl.rcNormalPosition.top    = top;
        WndPl.rcNormalPosition.right  = right;
        WndPl.rcNormalPosition.bottom = bottom;
        width  = right  - left;
        height = bottom - top;
        WndPl.showCmd = nCmdShow;
        GroupFileName = lpszGrpFile;
    }

    hGroup   = Alloc(HEAP_ZERO_MEMORY, sizeof(*hGroup));
    hName    = Alloc(HEAP_ZERO_MEMORY, (wcslen(lpszName)      + 1) * sizeof(WCHAR));
    hGrpFile = Alloc(HEAP_ZERO_MEMORY, (wcslen(GroupFileName) + 1) * sizeof(WCHAR));
    if (!hGroup || !hName || !hGrpFile)
    {
        MAIN_MessageBoxIDS(IDS_OUT_OF_MEMORY, IDS_ERROR, MB_OK);
        if (hGroup)   Free(hGroup);
        if (hName)    Free(hName);
        if (hGrpFile) Free(hGrpFile);
        return NULL;
    }
    memcpy(hName   , lpszName     , (wcslen(lpszName)      + 1) * sizeof(WCHAR));
    memcpy(hGrpFile, GroupFileName, (wcslen(GroupFileName) + 1) * sizeof(WCHAR));

    Globals.hActiveGroup = hGroup;

    seqnum = 1;
    hPrior = NULL;
    for (p = &Globals.hGroups; *p; p = &hPrior->hNext)
    {
        hPrior = *p;
        if (hPrior->seqnum >= seqnum)
            seqnum = hPrior->seqnum + 1;
    }
    *p = hGroup;

    hGroup->hPrior           = hPrior;
    hGroup->hNext            = NULL;
    hGroup->format           = format;
    hGroup->bIsCommonGroup   = bIsCommonGroup;
    hGroup->hName            = hName;
    hGroup->hGrpFile         = hGrpFile;
    hGroup->bOverwriteFileOk = bOverwriteFileOk;
    hGroup->seqnum           = seqnum;
    hGroup->nCmdShow         = nCmdShow;
#if 0
    hGroup->x         = x;
    hGroup->y         = y;
    hGroup->width     = width;
    hGroup->height    = height;
#endif
    hGroup->iconx            = cxIcon;
    hGroup->icony            = cyIcon;
    hGroup->hPrograms        = NULL;
    hGroup->hActiveProgram   = NULL;
    hGroup->TagsSize         = 0;
    hGroup->Tags             = NULL;

    mcs.szClass = STRING_GROUP_WIN_CLASS_NAME;
    mcs.szTitle = lpszName;
    mcs.hOwner  = NULL;
    mcs.x       = WndPl.rcNormalPosition.left;
    mcs.y       = WndPl.rcNormalPosition.top;
    mcs.cx      = width;
    mcs.cy      = height;
    mcs.style   = 0;
    mcs.lParam  = (LPARAM)hGroup;

    hGroup->hWnd = (HWND)SendMessageW(Globals.hMDIWnd, WM_MDICREATE, 0, (LPARAM)&mcs);

    SetWindowPlacement(hGroup->hWnd, &WndPl);

#if 1
    if (!bSuppressShowWindow) /* FIXME shouldn't be necessary */
#endif
        UpdateWindow(hGroup->hWnd);

    return hGroup;
}





/***********************************************************************
 *
 *           GROUP_ModifyGroup
 */

VOID GROUP_ModifyGroup(PROGGROUP* hGroup)
{
    WCHAR Dest[MAX_PATHNAME_LEN]; // szName
    WCHAR szGrpFile[MAX_PATHNAME_LEN]; // szFile

    wcsncpy(Dest, hGroup->hName, ARRAYSIZE(Dest));
    wcsncpy(szGrpFile, hGroup->hGrpFile, ARRAYSIZE(szGrpFile));

    if (!DIALOG_GroupAttributes(hGroup->format, Dest, szGrpFile, MAX_PATHNAME_LEN))
        return;

    if (wcscmp(szGrpFile, hGroup->hGrpFile))
        hGroup->bOverwriteFileOk = FALSE;

    MAIN_ReplaceString(&hGroup->hName, Dest);
    MAIN_ReplaceString(&hGroup->hGrpFile, szGrpFile);

    GRPFILE_WriteGroupFile(hGroup);

    /* FIXME Delete old GrpFile if GrpFile changed */

    /* FIXME Update progman.ini */

    SetWindowTextW(hGroup->hWnd, Dest);
}

/***********************************************************************
 *
 *           GROUP_DeleteGroup
 */

VOID GROUP_DeleteGroup(PROGGROUP* hGroup)
{
    if (Globals.hActiveGroup == hGroup)
        Globals.hActiveGroup = NULL;

    if (hGroup->hPrior)
        hGroup->hPrior->hNext = hGroup->hNext;
    else
        Globals.hGroups = hGroup->hNext;

    if (hGroup->hNext)
        hGroup->hNext->hPrior = hGroup->hPrior;

    while (hGroup->hPrograms)
        PROGRAM_DeleteProgram(hGroup->hPrograms, FALSE);

    /* FIXME Update progman.ini */

    SendMessageW(Globals.hMDIWnd, WM_MDIDESTROY, (WPARAM)hGroup->hWnd, 0);

    if (hGroup->Tags)
        Free(hGroup->Tags);
    Free(hGroup->hName);
    Free(hGroup->hGrpFile);
    Free(hGroup);
}

/***********************************************************************
 *
 *           GROUP_ShowGroupWindow
 */

/* FIXME shouldn't be necessary */
VOID GROUP_ShowGroupWindow(PROGGROUP* hGroup)
{
    ShowWindow(hGroup->hWnd, hGroup->nCmdShow);
    UpdateWindow(hGroup->hWnd);
}

/***********************************************************************
 *
 *           GROUP_ActiveGroup
 */

PROGGROUP* GROUP_ActiveGroup(VOID)
{
    return Globals.hActiveGroup;
}
