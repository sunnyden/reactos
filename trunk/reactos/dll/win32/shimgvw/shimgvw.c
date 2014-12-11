/*
 *
 * PROJECT:         ReactOS Picture and Fax Viewer
 * FILE:            dll/win32/shimgvw/shimgvw.c
 * PURPOSE:         shimgvw.dll
 * PROGRAMMER:      Dmitry Chapyshev (dmitry@reactos.org)
 *
 * UPDATE HISTORY:
 *      28/05/2008  Created
 */

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#include <stdarg.h>

#include <windef.h>
#include <winbase.h>
#include <winnls.h>
#include <winreg.h>
#include <wingdi.h>
#include <objbase.h>
#include <commctrl.h>
#include <commdlg.h>
#include <gdiplus.h>
#include <tchar.h>
#include <strsafe.h>
#include <shlwapi.h>

#define NDEBUG
#include <debug.h>

#include "shimgvw.h"


HINSTANCE hInstance;
SHIMGVW_SETTINGS shiSettings;
SHIMGVW_FILENODE *currentFile;
GpImage *image;
WNDPROC PrevProc = NULL;

HWND hDispWnd, hToolBar;

/* ToolBar Buttons */
static const TBBUTTON Buttons [] =
{   /* iBitmap,     idCommand,   fsState,         fsStyle,     bReserved[2], dwData, iString */
    {TBICON_PREV,   IDC_PREV,    TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 0},
    {TBICON_NEXT,   IDC_NEXT,    TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 0},
    {15,            0,           TBSTATE_ENABLED, BTNS_SEP,    {0}, 0, 0},
    {TBICON_ZOOMP,  IDC_ZOOMP,   TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 0},
    {TBICON_ZOOMM,  IDC_ZOOMM,   TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 0},
    {15,            0,           TBSTATE_ENABLED, BTNS_SEP,    {0}, 0, 0},
    {TBICON_ROT1,   IDC_ROT1,    TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 0},
    {TBICON_ROT2,   IDC_ROT2,    TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 0},
    {15,            0,           TBSTATE_ENABLED, BTNS_SEP,    {0}, 0, 0},
    {TBICON_SAVE,   IDC_SAVE,    TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 0},
    {TBICON_PRINT,  IDC_PRINT,   TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 0},
};

static void pLoadImage(LPWSTR szOpenFileName)
{
    if (GetFileAttributesW(szOpenFileName) == 0xFFFFFFFF)
    {
        DPRINT1("File %s not found!\n", szOpenFileName);
        return;
    }

    GdipLoadImageFromFile(szOpenFileName, &image);
    if (!image)
    {
        DPRINT1("GdipLoadImageFromFile() failed\n");
    }
}

static void pSaveImageAs(HWND hwnd)
{
    OPENFILENAMEW sfn;
    ImageCodecInfo *codecInfo;
    WCHAR szSaveFileName[MAX_PATH];
    WCHAR *szFilterMask;
    GUID rawFormat;
    UINT num;
    UINT size;
    UINT sizeRemain;
    UINT j;
    WCHAR *c;

    GdipGetImageEncodersSize(&num, &size);
    codecInfo = malloc(size);
    if (!codecInfo)
    {
        DPRINT1("malloc() failed in pSaveImageAs()\n");
        return;
    }

    GdipGetImageEncoders(num, size, codecInfo);
    GdipGetImageRawFormat(image, &rawFormat);

    sizeRemain = 0;

    for (j = 0; j < num; ++j)
    {
        // Every pair needs space for the Description, twice the Extensions, 1 char for the space, 2 for the braces and 2 for the NULL terminators.
        sizeRemain = sizeRemain + (((wcslen(codecInfo[j].FormatDescription) + (wcslen(codecInfo[j].FilenameExtension) * 2) + 5) * sizeof(WCHAR)));
    }

    /* Add two more chars for the last terminator */
    sizeRemain = sizeRemain + (sizeof(WCHAR) * 2);

    szFilterMask = malloc(sizeRemain);
    if (!szFilterMask)
    {
        DPRINT1("cannot allocate memory for filter mask in pSaveImageAs()");
        free(codecInfo);
        return;
    }

    ZeroMemory(szSaveFileName, sizeof(szSaveFileName));
    ZeroMemory(szFilterMask, sizeRemain);
    ZeroMemory(&sfn, sizeof(sfn));
    sfn.lStructSize = sizeof(sfn);
    sfn.hwndOwner   = hwnd;
    sfn.hInstance   = hInstance;
    sfn.lpstrFile   = szSaveFileName;
    sfn.lpstrFilter = szFilterMask;
    sfn.nMaxFile    = MAX_PATH;
    sfn.Flags       = OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY;

    c = szFilterMask;

    for (j = 0; j < num; ++j)
    {
        StringCbPrintfExW(c, sizeRemain, &c, &sizeRemain, 0, L"%ls (%ls)", codecInfo[j].FormatDescription, codecInfo[j].FilenameExtension);

        /* Skip the NULL character */
        c++;
        sizeRemain -= sizeof(*c);

        StringCbPrintfExW(c, sizeRemain, &c, &sizeRemain, 0, L"%ls", codecInfo[j].FilenameExtension);

        /* Skip the NULL character */
        c++;
        sizeRemain -= sizeof(*c);

        if (IsEqualGUID(&rawFormat, &codecInfo[j].FormatID) == TRUE)
        {
            sfn.nFilterIndex = j + 1;
        }
    }

    if (GetSaveFileNameW(&sfn))
    {
        if (GdipSaveImageToFile(image, szSaveFileName, &codecInfo[sfn.nFilterIndex - 1].Clsid, NULL) != Ok)
        {
            DPRINT1("GdipSaveImageToFile() failed\n");
        }
    }

    free(szFilterMask);
    free(codecInfo);
}

static VOID
pLoadImageFromNode(SHIMGVW_FILENODE *node, HWND hwnd)
{
    WCHAR szTitleBuf[800];
    WCHAR szResStr[512];
    WCHAR *c;

    if (node)
    {
        c = wcsrchr(node->FileName, '\\');
        if (c)
        {
            c++;
        }
        
        LoadStringW(hInstance, IDS_APPTITLE, szResStr, 512);
        StringCbPrintfExW(szTitleBuf, 800, NULL, NULL, 0, L"%ls%ls%ls", szResStr, L" - ", c);
        SetWindowTextW(hwnd, szTitleBuf);

        if (image)
        {
            GdipDisposeImage(image);
        }

        pLoadImage(node->FileName);
        InvalidateRect(hDispWnd, NULL, TRUE);
        UpdateWindow(hDispWnd);
    }
}

static SHIMGVW_FILENODE*
pBuildFileList(LPWSTR szFirstFile)
{
    HANDLE hFindHandle;
    WCHAR *extension;
    WCHAR szSearchPath[MAX_PATH];
    WCHAR szSearchMask[MAX_PATH];
    WCHAR szFileTypes[MAX_PATH];
    WIN32_FIND_DATAW findData;
    SHIMGVW_FILENODE *currentNode;
    SHIMGVW_FILENODE *root;
    SHIMGVW_FILENODE *conductor;
    ImageCodecInfo *codecInfo;
    UINT num;
    UINT size;
    UINT j;


    wcscpy(szSearchPath, szFirstFile);
    PathRemoveFileSpecW(szSearchPath);

    GdipGetImageDecodersSize(&num, &size);
    codecInfo = malloc(size);
    if (!codecInfo)
    {
        DPRINT1("malloc() failed in pLoadFileList()\n");
        return NULL;
    }

    GdipGetImageDecoders(num, size, codecInfo);

    root = malloc(sizeof(SHIMGVW_FILENODE));
    if (!root)
    {
        DPRINT1("malloc() failed in pLoadFileList()\n");
        free(codecInfo);
        return NULL;
    }

    conductor = root;

    for (j = 0; j < num; ++j)
    {
        StringCbPrintfExW(szFileTypes, MAX_PATH, NULL, NULL, 0, L"%ls", codecInfo[j].FilenameExtension);
        
        extension = wcstok(szFileTypes, L";");
        while (extension != NULL)
        {
            StringCbPrintfExW(szSearchMask, MAX_PATH, NULL, NULL, 0, L"%ls%ls%ls", szSearchPath, L"\\", extension);

            hFindHandle = FindFirstFileW(szSearchMask, &findData);
            if (hFindHandle != INVALID_HANDLE_VALUE)
            {
                do
                {
                    StringCbPrintfExW(conductor->FileName, MAX_PATH, NULL, NULL, 0, L"%ls%ls%ls", szSearchPath, L"\\", findData.cFileName);

                    // compare the name of the requested file with the one currently found.
                    // if the name matches, the current node is returned by the function.
                    if (wcscmp(szFirstFile, conductor->FileName) == 0)
                    {
                        currentNode = conductor;
                    }

                    conductor->Next = malloc(sizeof(SHIMGVW_FILENODE));

                    // if malloc fails, make circular what we have and return it
                    if (!conductor->Next)
                    {
                        DPRINT1("malloc() failed in pLoadFileList()\n");
                        
                        conductor->Next = root;
                        root->Prev = conductor;

                        FindClose(hFindHandle);
                        free(codecInfo);
                        return conductor;
                    }

                    conductor->Next->Prev = conductor;
                    conductor = conductor->Next;
                }
                while (FindNextFileW(hFindHandle, &findData) != 0);

                FindClose(hFindHandle);
            }

            extension = wcstok(NULL, L";");
        }
    }

    // we now have a node too much in the list. In case the requested file was not found,
    // we use this node to store the name of it, otherwise we free it.
    if (currentNode == NULL)
    {
        StringCbPrintfExW(conductor->FileName, MAX_PATH, NULL, NULL, 0, L"%ls", szFirstFile);
        currentNode = conductor;
    }
    else
    {
        conductor = conductor->Prev;
        free(conductor->Next);
    }

    // link the last node with the first one to make the list circular
    conductor->Next = root;
    root->Prev = conductor;
    conductor = currentNode;

    free(codecInfo);

    return conductor;
}

static VOID
pFreeFileList(SHIMGVW_FILENODE *root)
{
    SHIMGVW_FILENODE *conductor;

    root->Prev->Next = NULL;
    root->Prev = NULL;

    while (root)
    {
        conductor = root;
        root = conductor->Next;
        free(conductor);
    }
}

static VOID
ImageView_UpdateWindow(HWND hwnd)
{
    InvalidateRect(hwnd, NULL, FALSE);
    UpdateWindow(hwnd);
}

static VOID
ImageView_DrawImage(HWND hwnd)
{
    GpGraphics *graphics;
    UINT uImgWidth, uImgHeight;
    UINT height = 0, width = 0, x = 0, y = 0;
    PAINTSTRUCT ps;
    RECT rect;
    HDC hdc;

    hdc = BeginPaint(hwnd, &ps);
    if (!hdc)
    {
        DPRINT1("BeginPaint() failed\n");
        return;
    }

    GdipCreateFromHDC(hdc, &graphics);
    if (!graphics)
    {
        DPRINT1("GdipCreateFromHDC() failed\n");
        return;
    }

    GdipGetImageWidth(image, &uImgWidth);
    GdipGetImageHeight(image, &uImgHeight);

    if (GetClientRect(hwnd, &rect))
    {
        FillRect(hdc, &rect, (HBRUSH)GetStockObject(WHITE_BRUSH));

        if ((rect.right >= uImgWidth)&&(rect.bottom >= uImgHeight))
        {
            width = uImgWidth;
            height = uImgHeight;
        }
        else
        {
            height = uImgHeight * (UINT)rect.right / uImgWidth;
            if (height <= rect.bottom)
            {
                width = rect.right;
            }
            else
            {
                width = uImgWidth * (UINT)rect.bottom / uImgHeight;
                height = rect.bottom;
            }
        }

        y = (rect.bottom / 2) - (height / 2);
        x = (rect.right / 2) - (width / 2);

        DPRINT("x = %d\ny = %d\nWidth = %d\nHeight = %d\n\nrect.right = %d\nrect.bottom = %d\n\nuImgWidth = %d\nuImgHeight = %d\n", x, y, width, height, rect.right, rect.bottom, uImgWidth, uImgHeight);
        Rectangle(hdc, x - 1, y - 1, x + width + 1, y + height + 1);
        GdipDrawImageRect(graphics, image, x, y, width, height);
    }
    GdipDeleteGraphics(graphics);
    EndPaint(hwnd, &ps);
}

static BOOL
ImageView_LoadSettings()
{
    HKEY hKey;
    DWORD dwSize;

    if (RegOpenKeyEx(HKEY_CURRENT_USER, _T("Software\\ReactOS\\shimgvw"), 0, KEY_READ, &hKey) == ERROR_SUCCESS)
    {
        dwSize = sizeof(SHIMGVW_SETTINGS);
        if (RegQueryValueEx(hKey, _T("Settings"), NULL, NULL, (LPBYTE)&shiSettings, &dwSize) == ERROR_SUCCESS)
        {
            RegCloseKey(hKey);
            return TRUE;
        }

        RegCloseKey(hKey);
    }

    return FALSE;
}

static VOID
ImageView_SaveSettings(HWND hwnd)
{
    WINDOWPLACEMENT wp;
    HKEY hKey;

    ShowWindow(hwnd, SW_HIDE);
    wp.length = sizeof(WINDOWPLACEMENT);
    GetWindowPlacement(hwnd, &wp);

    shiSettings.Left = wp.rcNormalPosition.left;
    shiSettings.Top  = wp.rcNormalPosition.top;
    shiSettings.Right  = wp.rcNormalPosition.right;
    shiSettings.Bottom = wp.rcNormalPosition.bottom;
    shiSettings.Maximized = (IsZoomed(hwnd) || (wp.flags & WPF_RESTORETOMAXIMIZED));

    if (RegCreateKeyEx(HKEY_CURRENT_USER, _T("Software\\ReactOS\\shimgvw"), 0, NULL,
        REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS)
    {
        RegSetValueEx(hKey, _T("Settings"), 0, REG_BINARY, (LPBYTE)&shiSettings, sizeof(SHIMGVW_SETTINGS));
        RegCloseKey(hKey);
    }
}

static BOOL
ImageView_CreateToolBar(HWND hwnd)
{
    INT numButtons = sizeof(Buttons) / sizeof(Buttons[0]);

    hToolBar = CreateWindowEx(0, TOOLBARCLASSNAME, NULL,
                              WS_CHILD | WS_VISIBLE | TBSTYLE_FLAT | CCS_BOTTOM | TBSTYLE_TOOLTIPS,
                              0, 0, 0, 0, hwnd,
                              0, hInstance, NULL);
    if(hToolBar != NULL)
    {
        HIMAGELIST hImageList;

        SendMessage(hToolBar, TB_SETEXTENDEDSTYLE,
                    0, TBSTYLE_EX_HIDECLIPPEDBUTTONS);

        SendMessage(hToolBar, TB_BUTTONSTRUCTSIZE,
                    sizeof(Buttons[0]), 0);

        hImageList = ImageList_Create(TB_IMAGE_WIDTH, TB_IMAGE_HEIGHT, ILC_MASK | ILC_COLOR24, 1, 1);

        ImageList_AddMasked(hImageList, LoadImage(hInstance, MAKEINTRESOURCE(IDB_PREVICON), IMAGE_BITMAP,
                      TB_IMAGE_WIDTH, TB_IMAGE_HEIGHT, LR_DEFAULTCOLOR), RGB(255, 255, 255));

        ImageList_AddMasked(hImageList, LoadImage(hInstance, MAKEINTRESOURCE(IDB_NEXTICON), IMAGE_BITMAP,
                      TB_IMAGE_WIDTH, TB_IMAGE_HEIGHT, LR_DEFAULTCOLOR), RGB(255, 255, 255));

        ImageList_AddMasked(hImageList, LoadImage(hInstance, MAKEINTRESOURCE(IDB_ZOOMPICON), IMAGE_BITMAP,
                      TB_IMAGE_WIDTH, TB_IMAGE_HEIGHT, LR_DEFAULTCOLOR), RGB(255, 255, 255));

        ImageList_AddMasked(hImageList, LoadImage(hInstance, MAKEINTRESOURCE(IDB_ZOOMMICON), IMAGE_BITMAP,
                      TB_IMAGE_WIDTH, TB_IMAGE_HEIGHT, LR_DEFAULTCOLOR), RGB(255, 255, 255));

        ImageList_AddMasked(hImageList, LoadImage(hInstance, MAKEINTRESOURCE(IDB_SAVEICON), IMAGE_BITMAP,
                      TB_IMAGE_WIDTH, TB_IMAGE_HEIGHT, LR_DEFAULTCOLOR), RGB(255, 255, 255));

        ImageList_AddMasked(hImageList, LoadImage(hInstance, MAKEINTRESOURCE(IDB_PRINTICON), IMAGE_BITMAP,
                      TB_IMAGE_WIDTH, TB_IMAGE_HEIGHT, LR_DEFAULTCOLOR), RGB(255, 255, 255));

        ImageList_AddMasked(hImageList, LoadImage(hInstance, MAKEINTRESOURCE(IDB_ROT1ICON), IMAGE_BITMAP,
                      TB_IMAGE_WIDTH, TB_IMAGE_HEIGHT, LR_DEFAULTCOLOR), RGB(255, 255, 255));

        ImageList_AddMasked(hImageList, LoadImage(hInstance, MAKEINTRESOURCE(IDB_ROT2ICON), IMAGE_BITMAP,
                      TB_IMAGE_WIDTH, TB_IMAGE_HEIGHT, LR_DEFAULTCOLOR), RGB(255, 255, 255));

        if (hImageList == NULL) return FALSE;

        ImageList_Destroy((HIMAGELIST)SendMessage(hToolBar, TB_SETIMAGELIST,
                                                  0, (LPARAM)hImageList));

        SendMessage(hToolBar, TB_ADDBUTTONS,
                    numButtons, (LPARAM)Buttons);

        return TRUE;
    }

    return FALSE;
}

LRESULT CALLBACK
ImageView_DispWndProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
    switch (Message)
    {
        case WM_PAINT:
        {
            ImageView_DrawImage(hwnd);
            return 0L;
        }
	}
    return CallWindowProc(PrevProc, hwnd, Message, wParam, lParam);
}

static VOID
ImageView_InitControls(HWND hwnd)
{
    MoveWindow(hwnd, shiSettings.Left, shiSettings.Top,
               shiSettings.Right - shiSettings.Left,
               shiSettings.Bottom - shiSettings.Top, TRUE);

    if (shiSettings.Maximized) ShowWindow(hwnd, SW_MAXIMIZE);

    hDispWnd = CreateWindowEx(0, _T("STATIC"), _T(""),
                              WS_CHILD | WS_VISIBLE,
                              0, 0, 0, 0, hwnd, NULL, hInstance, NULL);

    SetClassLongPtr(hDispWnd, GCL_STYLE, CS_HREDRAW | CS_VREDRAW);
    PrevProc = (WNDPROC) SetWindowLongPtr(hDispWnd, GWL_WNDPROC, (LPARAM) ImageView_DispWndProc);

    ImageView_CreateToolBar(hwnd);
}

LRESULT CALLBACK
ImageView_WndProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
    switch (Message)
    {
        case WM_CREATE:
        {
            ImageView_InitControls(hwnd);
            return 0L;
        }
        case WM_COMMAND:
        {
            switch (wParam)
            {
                case IDC_PREV:
                {
                    currentFile = currentFile->Prev;
                    pLoadImageFromNode(currentFile, hwnd);
                }

                break;
                case IDC_NEXT:
                {
                    currentFile = currentFile->Next;
                    pLoadImageFromNode(currentFile, hwnd);
                }

                break;
                case IDC_ZOOMP:

                break;
                case IDC_ZOOMM:

                break;
                case IDC_SAVE:
                    pSaveImageAs(hwnd);

                break;
                case IDC_PRINT:

                break;
                case IDC_ROT1:
                {
                    GdipImageRotateFlip(image, Rotate270FlipNone);
                    ImageView_UpdateWindow(hwnd);
                }

                break;
                case IDC_ROT2:
                {
                    GdipImageRotateFlip(image, Rotate90FlipNone);
                    ImageView_UpdateWindow(hwnd);
                }

                break;
            }
        }
        break;

        case WM_NOTIFY:
        {
            LPNMHDR pnmhdr = (LPNMHDR)lParam;

            switch (pnmhdr->code)
            {
                case TTN_GETDISPINFO:
                {
                    LPTOOLTIPTEXT lpttt;
                    UINT idButton;

                    lpttt = (LPTOOLTIPTEXT)lParam;
                    idButton = (UINT)lpttt->hdr.idFrom;
                    lpttt->hinst = hInstance;

                    switch (idButton)
                    {
                        case IDC_PREV:
                            lpttt->lpszText = MAKEINTRESOURCE(IDS_TOOLTIP_PREV_PIC);
                        break;
                        case IDC_NEXT:
                            lpttt->lpszText = MAKEINTRESOURCE(IDS_TOOLTIP_NEXT_PIC);
                        break;
                        case IDC_ZOOMP:
                            lpttt->lpszText = MAKEINTRESOURCE(IDS_TOOLTIP_ZOOM_IN);
                        break;
                        case IDC_ZOOMM:
                            lpttt->lpszText = MAKEINTRESOURCE(IDS_TOOLTIP_ZOOM_OUT);
                        break;
                        case IDC_SAVE:
                            lpttt->lpszText = MAKEINTRESOURCE(IDS_TOOLTIP_SAVEAS);
                        break;
                        case IDC_PRINT:
                            lpttt->lpszText = MAKEINTRESOURCE(IDS_TOOLTIP_PRINT);
                        break;
                        case IDC_ROT1:
                            lpttt->lpszText = MAKEINTRESOURCE(IDS_TOOLTIP_ROT_COUNCW);
                        break;
                        case IDC_ROT2:
                            lpttt->lpszText = MAKEINTRESOURCE(IDS_TOOLTIP_ROT_CLOCKW);
                        break;
                    }
                    return TRUE;
                }
            }
            break;
        }
        case WM_SIZING:
        {
            LPRECT pRect = (LPRECT)lParam;
            if (pRect->right-pRect->left < 350)
                pRect->right = pRect->left + 350;

            if (pRect->bottom-pRect->top < 290)
                pRect->bottom = pRect->top + 290;
            return TRUE;
        }
        case WM_SIZE:
        {
            RECT rc;
            SendMessage(hToolBar, TB_AUTOSIZE, 0, 0);
            SendMessage(hToolBar, TB_GETITEMRECT, 1, (LPARAM)&rc);
            MoveWindow(hDispWnd, 1, 1, LOWORD(lParam)-1, HIWORD(lParam)-rc.bottom, TRUE);
            return 0L;
        }
        case WM_DESTROY:
        {
            ImageView_SaveSettings(hwnd);
            SetWindowLongPtr(hDispWnd, GWL_WNDPROC, (LPARAM) PrevProc);
            PostQuitMessage(0);
            break;
        }
    }

    return DefWindowProc(hwnd, Message, wParam, lParam);
}

LONG WINAPI
ImageView_CreateWindow(HWND hwnd, LPWSTR szFileName)
{
    struct GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    WNDCLASS WndClass = {0};
    TCHAR szBuf[512];
    WCHAR szInitialFile[MAX_PATH];
    HWND hMainWnd;
    MSG msg;

    if (!ImageView_LoadSettings())
    {
        shiSettings.Maximized = FALSE;
        shiSettings.Left      = 0;
        shiSettings.Top       = 0;
        shiSettings.Right     = 520;
        shiSettings.Bottom    = 400;
    }

    // Initialize GDI+
    gdiplusStartupInput.GdiplusVersion              = 1;
    gdiplusStartupInput.DebugEventCallback          = NULL;
    gdiplusStartupInput.SuppressBackgroundThread    = FALSE;
    gdiplusStartupInput.SuppressExternalCodecs      = FALSE;

    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
    pLoadImage(szFileName);

    // Create the window
    WndClass.lpszClassName  = _T("shimgvw_window");
    WndClass.lpfnWndProc    = ImageView_WndProc;
    WndClass.hInstance      = hInstance;
    WndClass.style          = CS_HREDRAW | CS_VREDRAW;
    WndClass.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_APPICON));
    WndClass.hCursor        = LoadCursor(hInstance, IDC_ARROW);
    WndClass.hbrBackground  = (HBRUSH)COLOR_WINDOW;

    if (!RegisterClass(&WndClass)) return -1;

    LoadString(hInstance, IDS_APPTITLE, szBuf, sizeof(szBuf) / sizeof(TCHAR));
    hMainWnd = CreateWindow(_T("shimgvw_window"), szBuf,
                            WS_OVERLAPPEDWINDOW | WS_VISIBLE | WS_CAPTION,
                            CW_USEDEFAULT, CW_USEDEFAULT,
                            0, 0, NULL, NULL, hInstance, NULL);

    // make sure the path has no quotes on it
    wcscpy(szInitialFile, szFileName);
    PathUnquoteSpacesW(szInitialFile);

    currentFile = pBuildFileList(szInitialFile);
    if (currentFile)
    {
        pLoadImageFromNode(currentFile, hMainWnd);
    }

    // Show it
    ShowWindow(hMainWnd, SW_SHOW);
    UpdateWindow(hMainWnd);

    // Message Loop
    while(GetMessage(&msg,NULL,0,0))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    pFreeFileList(currentFile);

    if (image)
        GdipDisposeImage(image);
    GdiplusShutdown(gdiplusToken);
    return -1;
}

VOID WINAPI
ImageView_FullscreenW(HWND hwnd, HINSTANCE hInst, LPCWSTR path, int nShow)
{
    ImageView_CreateWindow(hwnd, (LPWSTR)path);
}

VOID WINAPI
ImageView_Fullscreen(HWND hwnd, HINSTANCE hInst, LPCWSTR path, int nShow)
{
    ImageView_CreateWindow(hwnd, (LPWSTR)path);
}

VOID WINAPI
ImageView_FullscreenA(HWND hwnd, HINSTANCE hInst, LPCSTR path, int nShow)
{
    WCHAR szFile[MAX_PATH];

    if (MultiByteToWideChar(CP_ACP, 0, (char*)path, strlen((char*)path)+1, szFile, MAX_PATH))
    {
        ImageView_CreateWindow(hwnd, (LPWSTR)szFile);
    }
}

VOID WINAPI
ImageView_PrintTo(HWND hwnd, HINSTANCE hInst, LPCWSTR path, int nShow)
{
    DPRINT("ImageView_PrintTo() not implemented\n");
}

VOID WINAPI
ImageView_PrintToA(HWND hwnd, HINSTANCE hInst, LPCSTR path, int nShow)
{
    DPRINT("ImageView_PrintToA() not implemented\n");
}

VOID WINAPI
ImageView_PrintToW(HWND hwnd, HINSTANCE hInst, LPCWSTR path, int nShow)
{
    DPRINT("ImageView_PrintToW() not implemented\n");
}

BOOL WINAPI
DllMain(IN HINSTANCE hinstDLL,
        IN DWORD dwReason,
        IN LPVOID lpvReserved)
{
    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH:
        case DLL_THREAD_ATTACH:
            hInstance = hinstDLL;
            break;
    }

    return TRUE;
}

