/*
 *	Open With  Context Menu extension
 *
 * Copyright 2007 Johannes Anderwald <janderwald@reactos.org>
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

#include <string.h>

#define COBJMACROS
#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include "winerror.h"
#include "wine/debug.h"

#include "windef.h"
#include "wingdi.h"
#include "pidl.h"
#include "undocshell.h"
#include "shlobj.h"
#include "objbase.h"
#include "commdlg.h"

#include "shell32_main.h"
#include "shellfolder.h"
#include "shresdef.h"
#include "stdio.h"

WINE_DEFAULT_DEBUG_CHANNEL (shell);

const GUID CLSID_OpenWith = { 0x09799AFB, 0xAD67, 0x11d1, {0xAB,0xCD,0x00,0xC0,0x4F,0xC3,0x09,0x36} };

///
/// [HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\policies\system]
/// "NoInternetOpenWith"=dword:00000001
///

typedef BOOL (*LPFNOFN) (OPENFILENAMEW *) ;


typedef struct
{	
    const IContextMenu2Vtbl *lpVtblContextMenu;
	const IShellExtInitVtbl *lpvtblShellExtInit;
    LONG  wId;
    volatile LONG ref;
    WCHAR ** szArray;
    BOOL NoOpen;
    UINT size;
    UINT count;
    WCHAR szName[MAX_PATH];
    WCHAR szPath[MAX_PATH];
    INT iSelItem;
} SHEOWImpl;

static const IShellExtInitVtbl eivt;
static const IContextMenu2Vtbl cmvt;
static HRESULT WINAPI SHEOWCm_fnQueryInterface(IContextMenu2 *iface, REFIID riid, LPVOID *ppvObj);
static ULONG WINAPI SHEOWCm_fnRelease(IContextMenu2 *iface);

HRESULT WINAPI SHEOW_Constructor(IUnknown * pUnkOuter, REFIID riid, LPVOID *ppv)
{
    SHEOWImpl * ow;
    HRESULT res;

	ow = LocalAlloc(LMEM_ZEROINIT, sizeof(SHEOWImpl));
	if (!ow)
    {
        return E_OUTOFMEMORY;
    }

    ow->ref = 1;
    ow->lpVtblContextMenu = &cmvt;
    ow->lpvtblShellExtInit = &eivt;

    TRACE("(%p)->()\n",ow);

    res = SHEOWCm_fnQueryInterface( (IContextMenu2*)&ow->lpVtblContextMenu, riid, ppv );
    SHEOWCm_fnRelease( (IContextMenu2*)&ow->lpVtblContextMenu );
    return res;
}

static inline SHEOWImpl *impl_from_IShellExtInit( IShellExtInit *iface )
{
    return (SHEOWImpl *)((char*)iface - FIELD_OFFSET(SHEOWImpl, lpvtblShellExtInit));
}

static inline SHEOWImpl *impl_from_IContextMenu( IContextMenu2 *iface )
{
    return (SHEOWImpl *)((char*)iface - FIELD_OFFSET(SHEOWImpl, lpVtblContextMenu));
}

static HRESULT WINAPI SHEOWCm_fnQueryInterface(IContextMenu2 *iface, REFIID riid, LPVOID *ppvObj)
{
    SHEOWImpl *This = impl_from_IContextMenu(iface);

	TRACE("(%p)->(\n\tIID:\t%s,%p)\n",This,debugstr_guid(riid),ppvObj);

	*ppvObj = NULL;

     if(IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IContextMenu) ||
        IsEqualIID(riid, &IID_IContextMenu2))
	{
	  *ppvObj = &This->lpVtblContextMenu;
	}
	else if(IsEqualIID(riid, &IID_IShellExtInit))
	{
	  *ppvObj = &This->lpvtblShellExtInit;
	}

	if(*ppvObj)
	{
	  IUnknown_AddRef((IUnknown*)*ppvObj);
	  TRACE("-- Interface: (%p)->(%p)\n",ppvObj,*ppvObj);
	  return S_OK;
	}
	TRACE("-- Interface: E_NOINTERFACE\n");
	return E_NOINTERFACE;
}

static ULONG WINAPI SHEOWCm_fnAddRef(IContextMenu2 *iface)
{
    SHEOWImpl *This = impl_from_IContextMenu(iface);
	ULONG refCount = InterlockedIncrement(&This->ref);

	TRACE("(%p)->(count=%u)\n", This, refCount - 1);

	return refCount;
}

static ULONG WINAPI SHEOWCm_fnRelease(IContextMenu2 *iface)
{
    SHEOWImpl *This = impl_from_IContextMenu(iface);
	ULONG refCount = InterlockedDecrement(&This->ref);

	TRACE("(%p)->(count=%i)\n", This, refCount + 1);

	if (!refCount)
	{
	  TRACE(" destroying IContextMenu(%p)\n",This);
	  HeapFree(GetProcessHeap(),0,This);
	}
	return refCount;
}

static UINT
AddItems(SHEOWImpl *This, HMENU hMenu, UINT idCmdFirst)
{
    UINT count = 0;
    MENUITEMINFOW mii;
    WCHAR szBuffer[MAX_PATH];
    UINT index;
    static const WCHAR szChoose[] = { 'C','h','o','o','s','e',' ','P','r','o','g','r','a','m','.','.','.',0 };

    ZeroMemory(&mii, sizeof(mii));
    mii.cbSize = sizeof(mii);
    mii.fMask = MIIM_ID | MIIM_TYPE | MIIM_STATE;
    mii.fType = MFT_STRING;
    mii.fState = MFS_ENABLED;
    
    for (index = 0; index < This->count; index++)
    {
        mii.wID = idCmdFirst;
        mii.dwTypeData = (LPWSTR)This->szArray[index];
        if (InsertMenuItemW(hMenu, -1, TRUE, &mii))
        {
            idCmdFirst++;
            count++;
        }
    }
    
    mii.fMask = MIIM_TYPE | MIIM_ID;
    mii.fType = MFT_SEPARATOR;
    mii.wID = -1;
    InsertMenuItemW(hMenu, -1, TRUE, &mii);

    mii.fMask = MIIM_ID | MIIM_TYPE | MIIM_STATE;
    mii.fType = MFT_STRING;

    if (!LoadStringW(shell32_hInstance, IDS_OPEN_WITH_CHOOSE, szBuffer, sizeof(szBuffer) / sizeof(WCHAR)))
    {
       ERR("Failed to load string\n");
       wcscpy(szBuffer, szChoose);
    }

    mii.wID = idCmdFirst;
    mii.dwTypeData = (LPWSTR)szBuffer;
    if (InsertMenuItemW(hMenu, -1, TRUE, &mii))
        count++;

    return count;
}


static HRESULT WINAPI SHEOWCm_fnQueryContextMenu(
	IContextMenu2 *iface,
	HMENU hmenu,
	UINT indexMenu,
	UINT idCmdFirst,
	UINT idCmdLast,
	UINT uFlags)
{
    MENUITEMINFOW	mii;
    USHORT items = 0;
    WCHAR szBuffer[100];
    INT pos;

    HMENU hSubMenu = NULL;
    SHEOWImpl *This = impl_from_IContextMenu(iface);
   
    if (LoadStringW(shell32_hInstance, IDS_OPEN_WITH, szBuffer, 100) < 0)
    {
       TRACE("failed to load string\n");
       return E_FAIL;
    }
    if (This->count > 1)
    {
        hSubMenu = CreatePopupMenu();
        if (hSubMenu == NULL)
        {
            ERR("failed to create submenu");
            return E_FAIL;
        }
        items = AddItems(This, hSubMenu, idCmdFirst + 1);
    }
    pos = GetMenuDefaultItem(hmenu, TRUE, 0) + 1;

    ZeroMemory(&mii, sizeof(mii));
	mii.cbSize = sizeof(mii);
    mii.fMask = MIIM_ID | MIIM_TYPE | MIIM_STATE;
    if (hSubMenu)
    {
       mii.fMask |= MIIM_SUBMENU;
       mii.hSubMenu = hSubMenu;
    }
    mii.dwTypeData = (LPWSTR) szBuffer;
	mii.fState = MFS_ENABLED;
    if (!pos)
    {
        mii.fState |= MFS_DEFAULT;
    }

	mii.wID = idCmdFirst;
    This->wId = 0;

	mii.fType = MFT_STRING;
	if (InsertMenuItemW( hmenu, pos, TRUE, &mii))
        items++;

    TRACE("items %x\n",items);
    return MAKE_HRESULT(SEVERITY_SUCCESS, 0, items);
}

static void AddListViewItem(HWND hwndDlg, WCHAR * item, UINT state, UINT index)
{
    LV_ITEMW listItem;
    HWND hList;
    WCHAR * ptr;

    hList = GetDlgItem(hwndDlg, 14002);


    ptr = wcsrchr(item, L'\\') + 1;
    ZeroMemory(&listItem, sizeof(LV_ITEM));
    listItem.mask       = LVIF_TEXT | LVIF_PARAM | LVIF_STATE | LVIF_IMAGE;
    listItem.state      = state;
    listItem.pszText    = ptr;
    listItem.iImage     = -1;
    listItem.iItem      = index;
    listItem.lParam     = (LPARAM)item;

    (void)ListView_InsertItemW(hList, &listItem);
}

static void AddListViewItems(HWND hwndDlg, SHEOWImpl * This)
{
    HWND hList;
    RECT clientRect;
    LV_COLUMN col;

    hList = GetDlgItem(hwndDlg, 14002);

    GetClientRect(hList, &clientRect);

    ZeroMemory(&col, sizeof(LV_COLUMN));
    col.mask      = LVCF_SUBITEM | LVCF_WIDTH;
    col.iSubItem  = 0;
    col.cx        = (clientRect.right - clientRect.left) - GetSystemMetrics(SM_CXVSCROLL);
    (void)ListView_InsertColumn(hList, 0, &col);

    /* FIXME 
     * add default items
     */
    This->iSelItem = -1;
}
static void FreeListViewItems(HWND hwndDlg)
{
    HWND hList;
    int iIndex, iCount;
    LVITEM lvItem;

    hList = GetDlgItem(hwndDlg, 14002);

    iCount = ListView_GetItemCount(hList);
    ZeroMemory(&lvItem, sizeof(LVITEM));

    for (iIndex = 0; iIndex < iCount; iIndex++)
    {
        lvItem.mask = LVIF_PARAM;
        lvItem.iItem = iIndex;
        if (ListView_GetItem(GetDlgItem(hwndDlg, 14002), &lvItem))
        {
            free((void*)lvItem.lParam);
        }
    }
}

BOOL HideApplicationFromList(WCHAR * pFileName)
{
    WCHAR szBuffer[100];
    DWORD dwSize;

    wcscpy(szBuffer, L"Applications\\");
    wcscat(szBuffer, pFileName);

    if (RegGetValueW(HKEY_CLASSES_ROOT, szBuffer, L"NoOpenWith", RRF_RT_REG_SZ, NULL, NULL, &dwSize) == ERROR_SUCCESS)
        return TRUE;
    else
        return FALSE;
}

BOOL WriteStaticShellExtensionKey(HKEY hRootKey, WCHAR * pVerb, WCHAR *pFullPath)
{
    HKEY hShell;
    HKEY hVerb;
    HKEY hCmd;
    LONG result;

    if (RegCreateKeyExW(hRootKey, L"shell", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hShell, NULL) != ERROR_SUCCESS)
    {
        return FALSE;
    }

    result = RegCreateKeyExW(hShell, pVerb, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hVerb, NULL);
    RegCloseKey(hShell);
    if (result != ERROR_SUCCESS)
    {
        return FALSE;
    }

    result = RegCreateKeyExW(hVerb, L"command", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hCmd, NULL);
    RegCloseKey(hVerb);
    if(result != ERROR_SUCCESS)
    {
        return FALSE;
    }

    result = RegSetValueExW(hCmd, NULL, 0, REG_SZ, (const BYTE*)pFullPath, (strlenW(pFullPath)+1)* sizeof(WCHAR));
    RegCloseKey(hCmd);
    if (result == ERROR_SUCCESS)
        return TRUE;
    else
        return FALSE;
}

BOOL StoreApplicationsPathForUser(WCHAR *pFileName, WCHAR* pFullPath)
{
    HKEY hKey;
    HKEY hRootKey;
    LONG result;

    if (RegCreateKeyExW(HKEY_CURRENT_USER, L"SOFTWARE\\Classes\\Applications", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hRootKey, NULL) != ERROR_SUCCESS)
    {
        ERR("Error: failed to open HKCU\\Software\\Classes\\Applications key\n");
        return FALSE;
    }

    if (RegCreateKeyExW(hRootKey, pFileName, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL) != ERROR_SUCCESS)
    {
        ERR("Error: failed to create HKCR\\Software\\Classes\\Applications\\%s key\n", debugstr_w(pFileName));
        RegCloseKey(hRootKey);
        return FALSE;
    }
    RegCloseKey(hKey);
    RegCloseKey(hRootKey);

    result = WriteStaticShellExtensionKey(hKey, L"open", pFullPath);
    RegCloseKey(hKey);

    return (BOOL)result;
}
BOOL StoreNewSettings(WCHAR * pExt, WCHAR * pFileName, WCHAR * pFullPath)
{
    WCHAR szBuffer[70];
    WCHAR szVal[100];
    DWORD dwVal;
    HKEY hRootKey;
    HKEY hKey;
    DWORD dwDisposition;
    LONG result;
    DWORD dwBuffer;
    DWORD dwIndex;
    DWORD dwMask;
    WCHAR CurChar[2];

    wcscpy(szBuffer, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\FileExts\\");
    wcscat(szBuffer, pExt);

    if (RegCreateKeyExW(HKEY_CURRENT_USER,
                        szBuffer,
                        0,
                        NULL,
                        REG_OPTION_NON_VOLATILE,
                        KEY_WRITE | KEY_QUERY_VALUE,
                        NULL,
                        &hRootKey,
                        NULL) != ERROR_SUCCESS)
    {
        ERR("Error: failed to create/open %s\n", szBuffer);
        return FALSE;
    }

    if (RegCreateKeyExW(hRootKey,
                        L"OpenWithList",
                        0,
                        NULL,
                        REG_OPTION_NON_VOLATILE,
                        KEY_WRITE | KEY_QUERY_VALUE,
                        NULL,
                        &hKey,
                        &dwDisposition) != ERROR_SUCCESS)
    {
        ERR("Error: failed to create/open key dwDisposition %x\n", dwDisposition);
        RegCloseKey(hRootKey);
        return FALSE;
    }

    RegCloseKey(hRootKey);

    if (dwDisposition & REG_CREATED_NEW_KEY)
    {
        result = RegSetValueExW(hKey, L"a", 0, REG_SZ, (const BYTE*)pFileName, (strlenW(pFileName)+1) * sizeof(WCHAR));

        if (result != ERROR_SUCCESS)
        {
            ERR("Error: failed to set value\n");
            RegCloseKey(hKey);
            return FALSE;
        }
        
        result = RegSetValueExW(hKey, L"MRUList", 0, REG_SZ, (const BYTE*)L"a", 2 * sizeof(WCHAR));
        RegCloseKey(hKey);
        if (result == ERROR_SUCCESS)
            return TRUE;
        else
            return FALSE;
    }

    dwBuffer = sizeof(szBuffer);
    result = RegGetValueW(hKey, NULL, L"MRUList", RRF_RT_REG_SZ, NULL, szBuffer, &dwBuffer);
    if (result != ERROR_SUCCESS)
    {
        /* FIXME
         * recreate info
         */
        ERR("Failed to get value of MRUList\n");
        RegCloseKey(hKey);
        return FALSE;
    }
    dwMask = 0;
    CurChar[1] = 0;
    dwBuffer = (dwBuffer / sizeof(WCHAR));
    for(dwIndex = 0; dwIndex < dwBuffer - 1; dwIndex++)
    {
        CurChar[0] = szBuffer[dwIndex];
        dwMask |= (1 << (CurChar[0] - L'a'));

        dwVal = sizeof(szVal);
        if (RegGetValueW(hKey, NULL, CurChar, RRF_RT_REG_SZ, NULL, szVal, &dwVal) == ERROR_SUCCESS)
        {
            if (!wcsicmp(szVal, pFileName))
            {
                memmove(&szBuffer[0], &szBuffer[1], dwIndex * sizeof(WCHAR));
                szBuffer[0] = CurChar[0];
                result = RegSetValueExW(hKey, L"MRUList", 0, REG_SZ, (const BYTE*)szBuffer, dwBuffer * sizeof(WCHAR));
                RegCloseKey(hKey);
                if (result == ERROR_SUCCESS)
                    return TRUE;
                else
                    return FALSE;
            }
        }
    }

    dwIndex = 0;
    while(dwMask & (1 << dwIndex))
        dwIndex++;

    if (dwIndex >= sizeof(DWORD) * 8)
    {
        /* more than 32 progs in list */
        TRACE("no entry index available\n");
        RegCloseKey(hKey);
        return FALSE;
    }
    
    CurChar[0] = L'a' + dwIndex;
    result = RegSetValueExW(hKey, CurChar, 0, REG_SZ, (const BYTE*)pFileName, (strlenW(pFileName) + 1) * sizeof(WCHAR));
    if (result == ERROR_SUCCESS)
    {
        memmove(&szBuffer[0], &szBuffer[1], dwBuffer * sizeof(WCHAR));
        szBuffer[0] = CurChar[0];
        result = RegSetValueExW(hKey, L"MRUList", 0, REG_SZ, (const BYTE*)szBuffer, dwBuffer * sizeof(WCHAR));
        if (result == ERROR_SUCCESS)
        {
            StoreApplicationsPathForUser(pFileName, pFullPath);
        }
    }

    RegCloseKey(hKey);

    if (result == ERROR_SUCCESS)
        return TRUE;
    else
        return FALSE;
}
BOOL
SetProgrammAsDefaultHandler(WCHAR *pFileExt, WCHAR* pFullPath)
{
    HKEY hExtKey;
    HKEY hProgKey;
    DWORD dwDisposition;
    WCHAR szBuffer[20];
    DWORD dwSize;
    BOOL result;

    if (RegCreateKeyExW(HKEY_CLASSES_ROOT, pFileExt, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hExtKey, &dwDisposition) != ERROR_SUCCESS)
    {
        ERR("Error: failed to create HKCR\\%s key\n", debugstr_w(pFileExt));
        return FALSE;
    }

    if (dwDisposition & REG_CREATED_NEW_KEY)
    {
        wcscpy(szBuffer, &pFileExt[1]);
        wcscat(szBuffer, L"_auto_file");
    }
    else
    {
        dwSize = sizeof(szBuffer);
        if (!RegGetValueW(hExtKey, NULL, NULL, RRF_RT_REG_SZ, NULL, szBuffer, &dwSize))
        {
            ERR("Error: failed to retrieve subkey\n");
            RegCloseKey(hExtKey);
            return FALSE;
        }
    }

    RegCloseKey(hExtKey);
    TRACE("progkey %s\n", debugstr_w(szBuffer));
    if (RegCreateKeyExW(HKEY_CLASSES_ROOT, szBuffer, 0, NULL, 0, KEY_WRITE, NULL, &hProgKey, &dwDisposition) != ERROR_SUCCESS)
    {
        ERR("Error: failed to set progid key %s\n", debugstr_w(szBuffer));
        return FALSE;
    }

    /* FIXME
     * Should copy all verbs from HKCR\Classes\Applications\foo.exe\shell\*
     */

    result = WriteStaticShellExtensionKey(hProgKey, L"open", pFullPath);
    RegCloseKey(hProgKey);

    return result;
}


static BOOL CALLBACK OpenWithProgrammDlg(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    OPENFILENAMEW ofn;
    static HMODULE hComdlg = NULL;
    LPFNOFN ofnProc;
    WCHAR szBuffer[MAX_PATH + 30] = { 0 };
    WCHAR szPath[MAX_PATH * 2 +1] = { 0 };
    WCHAR * pExt;
    int res;
    LVITEMW lvItem;
    STARTUPINFOW si;
    PROCESS_INFORMATION pi;

    SHEOWImpl *This = (SHEOWImpl*) GetWindowLong(hwndDlg, DWLP_USER);

    switch(uMsg)
    {
    case WM_INITDIALOG:
        SetWindowLong(hwndDlg, DWLP_USER, (LONG)lParam);
        This = (SHEOWImpl*)lParam;
        res = SendDlgItemMessageW(hwndDlg, 14001, WM_GETTEXT, (sizeof(szBuffer) / sizeof(WCHAR)), (LPARAM)szBuffer);
        if (res < sizeof(szBuffer) / sizeof(WCHAR))
        {
            wcsncat(szBuffer, This->szPath, (sizeof(szBuffer) / sizeof(WCHAR)) - res);
            SendDlgItemMessageW(hwndDlg, 14001, WM_SETTEXT, 0, (LPARAM)szBuffer);
        }
        AddListViewItems(hwndDlg, This);
        return TRUE;
    case WM_COMMAND:
        switch(LOWORD(wParam))
		{
        case 14004: /* browse */
            res = LoadStringW(shell32_hInstance, IDS_OPEN_WITH, szBuffer, sizeof(szBuffer) / sizeof(WCHAR));
            if (res < sizeof(szBuffer))
            {
                ofn.lpstrTitle = szBuffer;
                ofn.nMaxFileTitle = strlenW(szBuffer);
            }
            
            ZeroMemory(&ofn, sizeof(OPENFILENAMEW));
            ofn.lStructSize  = sizeof(OPENFILENAMEW);
            ofn.hInstance = shell32_hInstance;
            ofn.lpstrFilter = L"Executable Files\0*.exe\0\0\0"; //FIXME
            ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
            ofn.nMaxFile = (sizeof(szPath) / sizeof(WCHAR));
            ofn.lpstrFile = szPath;
            
            if (!hComdlg)
                hComdlg = LoadLibraryExW (L"comdlg32", NULL, 0);
            if (!hComdlg)
                return TRUE;

            ofnProc = (LPFNOFN)GetProcAddress (hComdlg, "GetOpenFileNameW");
            if (!ofnProc)
                return TRUE;

            if (!ofnProc (&ofn))
                return TRUE;
      
            /* FIXME
             * check for duplicates 
             */
            AddListViewItem(hwndDlg, wcsdup(szPath), LVIS_FOCUSED | LVIS_SELECTED, 0);
            This->iSelItem = 0;
            return TRUE;
        case 14005: /* ok */
            ZeroMemory(&lvItem, sizeof(LVITEMW));

            lvItem.mask = LVIF_PARAM | LVIF_TEXT;
            lvItem.iItem = This->iSelItem;
            lvItem.pszText = szBuffer;
            if (!ListView_GetItemW(GetDlgItem(hwndDlg, 14002), &lvItem))
            {
                ERR("Failed to get item index %d\n", This->iSelItem);
                DestroyWindow(hwndDlg);
                return FALSE;
            }
            pExt = wcsrchr(szBuffer, L'.');

            if (!HideApplicationFromList(szBuffer))
            {
#if 0
                if (!StoreNewSettings(pExt, szBuffer, (WCHAR*)lvItem.lParam))
                {
                    /* failed to store setting */
                    WARN("Error: failed to store settings for app %s\n", debugstr_w(szBuffer));
                }
#endif
            }

            if (SendDlgItemMessage(hwndDlg, 14003, BM_GETCHECK, 0, 0) == BST_CHECKED)
            {
#if 0
                if (!SetProgrammAsDefaultHandler(pExt, (WCHAR*)lvItem.lParam))
                {
                    /* failed to associate programm */
                    WARN("Error: failed to associate programm\n");
                }
#endif
            }
            ZeroMemory(&si, sizeof(STARTUPINFOW));
            si.cb = sizeof(STARTUPINFOW);
            wcscpy(szPath, (WCHAR*)lvItem.lParam);
            wcscat(szPath, L" ");
            wcscat(szPath, This->szPath);

            TRACE("exe: %s path %s\n", debugstr_w((WCHAR*)lvItem.lParam), debugstr_w(This->szPath)); 
            if (CreateProcessW(NULL, szPath, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
            {
                CloseHandle(pi.hThread);
                CloseHandle(pi.hProcess);
            }
            else
            {
                ERR("failed to execute with err %x\n", GetLastError());
                return FALSE;
            }
            FreeListViewItems(hwndDlg);
            DestroyWindow(hwndDlg);
            return TRUE;
        case 14006: /* cancel */
            DestroyWindow(hwndDlg);
            return FALSE;
        default:
            break;
        }
        break;
    case WM_NOTIFY:
        {
            LPNMHDR lpnm = (LPNMHDR)lParam;

            switch(lpnm->code)
            {
                case LVN_ITEMCHANGED:
                {
                    LPNMLISTVIEW nm = (LPNMLISTVIEW)lParam;

                    if ((nm->uNewState & LVIS_SELECTED) == 0)
                        return FALSE;

                    This->iSelItem = nm->iItem;        
                }
            }
            break;
        }
    default:
        break;
    }
    return FALSE;
}

static HRESULT WINAPI
SHEOWCm_fnInvokeCommand( IContextMenu2* iface, LPCMINVOKECOMMANDINFO lpici )
{
    SHEOWImpl *This = impl_from_IContextMenu(iface);
    TRACE("This %p wId %x count %u verb %x\n", This, This->wId, This->count, LOWORD(lpici->lpVerb));    

    if (This->wId > LOWORD(lpici->lpVerb) || This->count + This->wId < LOWORD(lpici->lpVerb))
       return E_FAIL;

    if (This->NoOpen)
    {
        /* FIXME
         * show warning open dialog 
         */
    }
    
    if (This->wId == LOWORD(lpici->lpVerb))
    {
        MSG msg;
        BOOL bRet;
        HWND hwnd = CreateDialogParam(shell32_hInstance, MAKEINTRESOURCE(OPEN_WITH_PROGRAMM_DLG), lpici->hwnd, OpenWithProgrammDlg, (LPARAM)This);
        ShowWindow(hwnd, SW_SHOW);

        while ((bRet = GetMessage(&msg, NULL, 0, 0)) != 0) 
        { 
            if (bRet == -1)
            {
                // Handle the error and possibly exit
            }
            else if (!IsWindow(hwnd) || !IsDialogMessage(hwnd, &msg)) 
            { 
                TranslateMessage(&msg); 
                DispatchMessage(&msg); 
            } 
        } 
        return S_OK;
    }


    if (This->wId == LOWORD(lpici->lpVerb))
    {
        /* show Open As dialog */
        return S_OK;
    }
    else 
    {
        /* show program select dialog */
        return S_OK;
    }
}

static HRESULT WINAPI
SHEOWCm_fnGetCommandString( IContextMenu2* iface, UINT_PTR idCmd, UINT uType,
                            UINT* pwReserved, LPSTR pszName, UINT cchMax )
{
    SHEOWImpl *This = impl_from_IContextMenu(iface);

    FIXME("%p %lu %u %p %p %u\n", This,
          idCmd, uType, pwReserved, pszName, cchMax );

    return E_NOTIMPL;
}

static HRESULT WINAPI SHEOWCm_fnHandleMenuMsg(
	IContextMenu2 *iface,
	UINT uMsg,
	WPARAM wParam,
	LPARAM lParam)
{
    SHEOWImpl *This = impl_from_IContextMenu(iface);

    TRACE("This %p uMsg %x\n",This, uMsg);

    return E_NOTIMPL;
}

static const IContextMenu2Vtbl cmvt =
{
	SHEOWCm_fnQueryInterface,
	SHEOWCm_fnAddRef,
	SHEOWCm_fnRelease,
	SHEOWCm_fnQueryContextMenu,
	SHEOWCm_fnInvokeCommand,
	SHEOWCm_fnGetCommandString,
	SHEOWCm_fnHandleMenuMsg
};

BOOL
SHEOW_ResizeArray(SHEOWImpl *This)
{
  WCHAR ** new_array;
  UINT ncount;

  if (This->count == 0)
      ncount = 10;
  else
      ncount = This->count * 2;

  new_array = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, ncount * sizeof(WCHAR*));

  if (!new_array)
      return FALSE;

  if (This->szArray)
  {
    memcpy(new_array, This->szArray, This->count * sizeof(WCHAR*));
    HeapFree(GetProcessHeap(), 0, This->szArray);
  }

  This->szArray = new_array;
  This->size = ncount;
  return TRUE;
}


void
SHEOW_AddOWItem(SHEOWImpl *This, WCHAR * szAppName)
{
   UINT index;
   WCHAR * szPtr;

   if (This->count + 1 >= This->size || !This->szArray)
   {
        if (!SHEOW_ResizeArray(This))
            return;
   }

   szPtr = wcsrchr(szAppName, '.');
   if (szPtr)
   {
        szPtr[0] = 0;
   }

   for(index = 0; index < This->count; index++)
   {
        if (!wcsicmp(This->szArray[index], szAppName))
            return;
   }
   This->szArray[This->count] = wcsdup(szAppName);

    if (This->szArray[This->count])
        This->count++;
}

UINT
SHEW_AddOpenWithProgId(SHEOWImpl *This, HKEY hKey)
{
   FIXME("implement me :)))\n");
   return 0;
}


UINT
SHEW_AddOpenWithItem(SHEOWImpl *This, HKEY hKey)
{
  
    UINT NumItems = 0;
    DWORD dwIndex = 0;
    DWORD dwName, dwValue;
    LONG result = ERROR_SUCCESS;
    WCHAR szName[10];
    WCHAR szValue[MAX_PATH];
    WCHAR szMRUList[MAX_PATH] = {0};

    static const WCHAR szMRU[] = {'M','R','U','L','i','s','t', 0 };

    while(result == ERROR_SUCCESS)
    {
        dwName = sizeof(szName);
        dwValue = sizeof(szValue);
        

        result = RegEnumValueW(hKey, 
                               dwIndex, 
                               szName, 
                               &dwName, 
                               NULL,
                               NULL,
                               (LPBYTE)szValue,
                               &dwValue);
        szName[9] = 0;
        szValue[MAX_PATH-1] = 0;

        if (result == ERROR_SUCCESS)
        {
            if (wcsicmp(szValue, szMRU))
            {
                SHEOW_AddOWItem(This, szValue);    
                NumItems++;
            }
            else
            {
                wcscpy(szMRUList, szValue);
            }
        }
        dwIndex++;
    }

    if (szMRUList[0])
    {
        FIXME("handle MRUList\n");
    }
    return NumItems;
}



UINT 
SHEOW_LoadItemFromHKCR(SHEOWImpl *This, WCHAR * szExt)
{
    HKEY hKey;
    HKEY hSubKey;
    LONG result;
    UINT NumKeys = 0;
    WCHAR szBuffer[30];
    WCHAR szResult[70];
    DWORD dwSize;

    static const WCHAR szCROW[] = { 'O','p','e','n','W','i','t','h','L','i','s','t', 0 };
    static const WCHAR szCROP[] = { 'O','p','e','n','W','i','t','h','P','r','o','g','I','D','s',0 };
    static const WCHAR szPT[] = { 'P','e','r','c','e','i','v','e','d','T','y','p','e', 0 };
    static const WCHAR szSys[] = { 'S','y','s','t','e','m','F','i','l','e','A','s','s','o','c','i','a','t','i','o','n','s','\\','%','s','\\','O','p','e','n','W','i','t','h','L','i','s','t', 0 };


    TRACE("SHEOW_LoadItemFromHKCR entered with This %p szExt %s\n",This, debugstr_w(szExt));

    result = RegOpenKeyExW(HKEY_CLASSES_ROOT,
                          szExt,
                          0,
                          KEY_READ | KEY_QUERY_VALUE,
                          &hKey);
    if (result != ERROR_SUCCESS)
        return NumKeys;

    result = RegOpenKeyExW(hKey,
                          szCROW,
                          0,
                          KEY_READ | KEY_QUERY_VALUE,
                          &hSubKey);

    if (result == ERROR_SUCCESS)
    {
        NumKeys = SHEW_AddOpenWithItem(This, hSubKey);
        RegCloseKey(hSubKey);
    }

    result = RegOpenKeyExW(hKey,
                          szCROP,
                          0,
                          KEY_READ | KEY_QUERY_VALUE,
                          &hSubKey);

    if (result == ERROR_SUCCESS)
    {
        NumKeys += SHEW_AddOpenWithProgId(This, hSubKey);
        RegCloseKey(hSubKey);
    }

    dwSize = sizeof(szBuffer);

    result = RegGetValueW(hKey,
                          NULL,
                          szPT,
                          RRF_RT_REG_SZ,
                          NULL,
                          szBuffer,
                          &dwSize);
    szBuffer[29] = 0;

    if (result != ERROR_SUCCESS)
    {
        RegCloseKey(hKey);
        return NumKeys;
    }

    sprintfW(szResult, szSys, szExt);
    result = RegOpenKeyExW(hKey,
                          szCROW,
                          0,
                          KEY_READ | KEY_QUERY_VALUE,
                          &hSubKey);

    if (result == ERROR_SUCCESS)
    {
        NumKeys += SHEW_AddOpenWithProgId(This, hSubKey);
        RegCloseKey(hSubKey);
    }

    if (RegGetValueW(HKEY_CLASSES_ROOT, szExt, L"NoOpen", RRF_RT_REG_SZ, NULL, NULL, &dwSize) == ERROR_SUCCESS)
    {
        This->NoOpen = TRUE;
    }

    RegCloseKey(hKey);
    return NumKeys;
}

UINT
SHEOW_LoadItemFromHKCU(SHEOWImpl *This, WCHAR * szExt)
{
    WCHAR szBuffer[MAX_PATH];
    HKEY hKey;
    UINT KeyCount = 0;
    LONG result;
    
    static const WCHAR szOWPL[] = { 'S','o','f','t','w','a','r','e','\\','M','i','c','r','o','s','o','f','t','\\','W','i','n','d','o','w','s',
        '\\','C','u','r','r','e','n','t','V','e','r','s','i','o','n','\\','E','x','p','l','o','r','e','r','\\','F','i','l','e','E','x','t','s',
        '\\','%','s','\\','O','p','e','n','W','i','t','h','P','r','o','g','I','D','s',0 };

    static const WCHAR szOpenWith[] = { 'S','o','f','t','w','a','r','e','\\','M','i','c','r','o','s','o','f','t','\\','W','i','n','d','o','w','s',
        '\\','C','u','r','r','e','n','t','V','e','r','s','i','o','n','\\','E','x','p','l','o','r','e','r','\\','F','i','l','e','E','x','t','s',
        '\\','%','s','\\','O','p','e','n','W','i','t','h','L','i','s','t', 0 };

    TRACE("SHEOW_LoadItemFromHKCU entered with This %p szExt %s\n",This, debugstr_w(szExt));

   /* process HKCU settings */
   sprintfW(szBuffer, szOWPL, szExt);
   TRACE("szBuffer %s\n", debugstr_w(szBuffer));
   result = RegOpenKeyExW(HKEY_CURRENT_USER,
                          szBuffer,
                          0,
                          KEY_READ | KEY_QUERY_VALUE,
                          &hKey);

   if (result == ERROR_SUCCESS)
   {
       KeyCount = SHEW_AddOpenWithProgId(This, hKey);
       RegCloseKey(hKey);
   }

   sprintfW(szBuffer, szOpenWith, szExt);
   TRACE("szBuffer %s\n", debugstr_w(szBuffer));
   result = RegOpenKeyExW(HKEY_CURRENT_USER,
                          szBuffer,
                          0,
                          KEY_READ | KEY_QUERY_VALUE,
                          &hKey);

   if (result == ERROR_SUCCESS)
   {
       KeyCount += SHEW_AddOpenWithItem(This, hKey);
       RegCloseKey(hKey);
   }
   return KeyCount;
}

HRESULT
SHEOW_LoadOpenWithItems(SHEOWImpl *This, IDataObject *pdtobj)
{
    STGMEDIUM medium;
    FORMATETC fmt;
    HRESULT hr;
    LPIDA pida;
    LPCITEMIDLIST pidl_folder;
    LPCITEMIDLIST pidl_child; 
    LPCITEMIDLIST pidl; 
    DWORD dwPath;
    LPWSTR szPtr;
    WCHAR szPath[100];
    static const WCHAR szShortCut[] = { '.','l','n','k', 0 };

    fmt.cfFormat = RegisterClipboardFormatA(CFSTR_SHELLIDLIST);
    fmt.ptd = NULL;
    fmt.dwAspect = DVASPECT_CONTENT;
    fmt.lindex = -1;
    fmt.tymed = TYMED_HGLOBAL;

    hr = IDataObject_GetData(pdtobj, &fmt, &medium);

    if (FAILED(hr))
    {
        ERR("IDataObject_GetData failed with 0x%x\n", hr);
        return hr;
    }

        /*assert(pida->cidl==1);*/
    pida = (LPIDA)GlobalLock(medium.u.hGlobal);

    pidl_folder = (LPCITEMIDLIST) ((LPBYTE)pida+pida->aoffset[0]);
    pidl_child = (LPCITEMIDLIST) ((LPBYTE)pida+pida->aoffset[1]);

    pidl = ILCombine(pidl_folder, pidl_child);

    GlobalUnlock(medium.u.hGlobal);
    GlobalFree(medium.u.hGlobal);

    if (!pidl)
    {
        ERR("no mem\n");
        return E_OUTOFMEMORY;
    }
    if (_ILIsFolder(pidl_child))
    {
        TRACE("pidl is a folder\n");
        SHFree((void*)pidl);
        return E_FAIL;
    }

    if (!SHGetPathFromIDListW(pidl, This->szPath))
    {
        SHFree((void*)pidl);
        ERR("SHGetPathFromIDListW failed\n");
        return E_FAIL;
    }
    
    SHFree((void*)pidl);    
    TRACE("szPath %s\n", debugstr_w(This->szPath));

    szPtr = wcschr(This->szPath, '.');
    if (szPtr)
    {
        if (!_wcsicmp(szPtr, szShortCut))
        {
            TRACE("pidl is a shortcut\n");
            return E_FAIL;
        }

        SHEOW_LoadItemFromHKCU(This, szPtr);
        SHEOW_LoadItemFromHKCR(This, szPtr);
        dwPath = sizeof(szPath);
        if (RegGetValueW(HKEY_CLASSES_ROOT, szPtr, NULL, RRF_RT_REG_SZ, NULL, szPath, &dwPath) == ERROR_SUCCESS)
        {
            SHEOW_LoadItemFromHKCU(This, szPath);
        }

    }
    TRACE("count %u\n", This->count);
    return S_OK;
}




static HRESULT WINAPI
SHEOW_ExtInit_Initialize( IShellExtInit* iface, LPCITEMIDLIST pidlFolder,
                              IDataObject *pdtobj, HKEY hkeyProgID )
{
    SHEOWImpl *This = impl_from_IShellExtInit(iface);

    TRACE("This %p\n", This);

    return SHEOW_LoadOpenWithItems(This, pdtobj);
}

static ULONG WINAPI SHEOW_ExtInit_AddRef(IShellExtInit *iface)
{
    SHEOWImpl *This = impl_from_IShellExtInit(iface);
	ULONG refCount = InterlockedIncrement(&This->ref);

	TRACE("(%p)->(count=%u)\n", This, refCount - 1);

	return refCount;
}

static ULONG WINAPI SHEOW_ExtInit_Release(IShellExtInit *iface)
{
    SHEOWImpl *This = impl_from_IShellExtInit(iface);
	ULONG refCount = InterlockedDecrement(&This->ref);

	TRACE("(%p)->(count=%i)\n", This, refCount + 1);

	if (!refCount)
	{
	  HeapFree(GetProcessHeap(),0,This);
	}
	return refCount;
}

static HRESULT WINAPI
SHEOW_ExtInit_QueryInterface( IShellExtInit* iface, REFIID riid, void** ppvObject )
{
    SHEOWImpl *This = impl_from_IShellExtInit(iface);
    return SHEOWCm_fnQueryInterface((IContextMenu2*)This, riid, ppvObject);
}

static const IShellExtInitVtbl eivt =
{
    SHEOW_ExtInit_QueryInterface,
    SHEOW_ExtInit_AddRef,
    SHEOW_ExtInit_Release,
    SHEOW_ExtInit_Initialize
};
