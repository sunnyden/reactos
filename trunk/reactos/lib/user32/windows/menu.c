/*
 *  ReactOS kernel
 *  Copyright (C) 1998, 1999, 2000, 2001 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/* $Id: menu.c,v 1.18 2003/08/06 11:32:17 weiden Exp $
 *
 * PROJECT:         ReactOS user32.dll
 * FILE:            lib/user32/windows/menu.c
 * PURPOSE:         Menus
 * PROGRAMMER:      Casper S. Hornstrup (chorns@users.sourceforge.net)
 * UPDATE HISTORY:
 *      09-05-2001  CSH  Created
 */

/* INCLUDES ******************************************************************/

#include <windows.h>
#include <user32.h>
#include <debug.h>
#include <string.h>
#include <draw.h>
#include <window.h>

/* TYPES *********************************************************************/

#define MENU_TYPE_MASK ((MF_STRING | MF_BITMAP | MF_OWNERDRAW | MF_SEPARATOR))

#define MENU_ITEM_TYPE(flags) \
  ((flags) & (MF_STRING | MF_BITMAP | MF_OWNERDRAW | MF_SEPARATOR))
  
#define MENU_BAR_ITEMS_SPACE (12)
#define SEPARATOR_HEIGHT (5)
#define MENU_TAB_SPACE (8)

#define MENU_MAGIC (0x554D)

#define NO_SELECTED_ITEM (0xffff)

#ifndef MF_END
#define MF_END             (0x0080)
#endif

#ifndef MIIM_STRING
#define MIIM_STRING      (0x00000040)
#endif


/* INTERNAL FUNCTIONS ********************************************************/

/* Rip the fun and easy to use and fun WINE unicode string manipulation routines. 
 * Of course I didnt copy the ASM code because we want this to be portable 
 * and it needs to go away.
 */

static inline unsigned int strlenW( const WCHAR *str )
{
    const WCHAR *s = str;
    while (*s) s++;
    return s - str;
}

static inline WCHAR *strncpyW( WCHAR *str1, const WCHAR *str2, int n )
{
    WCHAR *ret = str1;
    while (n-- > 0) if (!(*str1++ = *str2++)) break;
    while (n-- > 0) *str1++ = 0;
    return ret;
}

static inline WCHAR *strcpyW( WCHAR *dst, const WCHAR *src )
{
    WCHAR *p = dst;
    while ((*p++ = *src++));
    return dst;
}

static inline WCHAR *strcatW( WCHAR *dst, const WCHAR *src )
{
    strcpyW( dst + strlenW(dst), src );
    return dst;
}

NTSTATUS
STATIC HEAP_strdupA2W ( HANDLE hHeap, LPWSTR* ppszW, LPCSTR lpszA, UINT* NewLen )
{
  ULONG len;
  NTSTATUS Status;
  *ppszW = NULL;
  if ( !lpszA )
    return STATUS_SUCCESS;
  len = lstrlenA(lpszA);
  *ppszW = RtlAllocateHeap ( hHeap, 0, (len+1) * sizeof(WCHAR) );
  if ( !*ppszW )
    return STATUS_NO_MEMORY;
  Status = RtlMultiByteToUnicodeN ( *ppszW, len*sizeof(WCHAR), NULL, (PCHAR)lpszA, len ); 
  (*ppszW)[len] = L'\0';
  if(NewLen) (*NewLen) = (UINT)len;
  return Status;
}

#ifndef GET_WORD
#define GET_WORD(ptr)  (*(WORD *)(ptr))
#endif
#ifndef GET_DWORD
#define GET_DWORD(ptr) (*(DWORD *)(ptr))
#endif

/**********************************************************************
 *         MENUEX_ParseResource
 *
 * Parse an extended menu resource and add items to the menu.
 * Return a pointer to the end of the resource.
 *
 * FIXME - should we be passing an LPCSTR to a predominantly UNICODE function?
 */
static LPCSTR MENUEX_ParseResource( LPCSTR res, HMENU hMenu)
{
  WORD resinfo;
  
  do
    {
      MENUITEMINFOW mii;

      mii.cbSize = sizeof(mii);
      mii.fMask = MIIM_STATE | MIIM_ID | MIIM_TYPE;
      mii.fType = GET_DWORD(res);
      res += sizeof(DWORD);
      mii.fState = GET_DWORD(res);
      res += sizeof(DWORD);
      mii.wID = GET_DWORD(res);
      res += sizeof(DWORD);
      resinfo = GET_WORD(res);
      res += sizeof(WORD);
      /* Align the text on a word boundary.  */
      res += (~((int)res - 1)) & 1;
      mii.dwTypeData = (LPWSTR) res;
      res += (1 + strlenW(mii.dwTypeData)) * sizeof(WCHAR);
      /* Align the following fields on a dword boundary.  */
      res += (~((int)res - 1)) & 3;

      if (resinfo & 1) /* Pop-up? */
	{
	  /* DWORD helpid = GET_DWORD(res); FIXME: use this.  */
	  res += sizeof(DWORD);
	  mii.hSubMenu = CreatePopupMenu();
	  if (!mii.hSubMenu)
	      return NULL;
	  if (!(res = MENUEX_ParseResource(res, mii.hSubMenu)))
	  {
	      DestroyMenu(mii.hSubMenu);
	      return NULL;
	  }
	  mii.fMask |= MIIM_SUBMENU;
	  mii.fType |= MF_POPUP;
	}
      else if(!*mii.dwTypeData && !(mii.fType & MF_SEPARATOR))
	{
	  DbgPrint("WARN: Converting NULL menu item %04x, type %04x to SEPARATOR\n",
	      mii.wID, mii.fType);
	  mii.fType |= MF_SEPARATOR;
	}
      InsertMenuItemW(hMenu, -1, MF_BYPOSITION, &mii);
    }
  while (!(resinfo & MF_END));
  return res;
}


/**********************************************************************
 *         MENU_ParseResource
 *
 * Parse a standard menu resource and add items to the menu.
 * Return a pointer to the end of the resource.
 *
 * NOTE: flags is equivalent to the mtOption field
 */
static LPCSTR MENU_ParseResource( LPCSTR res, HMENU hMenu, BOOL unicode )
{
  WORD flags, id = 0;
  LPCSTR str;
  BOOL end = FALSE;

  do
  {
    flags = GET_WORD(res);

    /* remove MF_END flag before passing it to AppendMenu()! */
    end = (flags & MF_END);
    if(end) flags ^= MF_END;

    res += sizeof(WORD);
    if(!(flags & MF_POPUP))
    {
      id = GET_WORD(res);
      res += sizeof(WORD);
    }
    str = res;
    if(!unicode)
      res += strlen(str) + 1;
    else
      res += (strlenW((LPCWSTR)str) + 1) * sizeof(WCHAR);
    if (flags & MF_POPUP)
    {
      HMENU hSubMenu = CreatePopupMenu();
      if(!hSubMenu) return NULL;
      if(!(res = MENU_ParseResource(res, hSubMenu, unicode)))
        return NULL;
      if(!unicode)
        AppendMenuA(hMenu, flags, (UINT)hSubMenu, str);
      else
        AppendMenuW(hMenu, flags, (UINT)hSubMenu, (LPCWSTR)str);
    }
    else  /* Not a popup */
    {
      if(!unicode)
        AppendMenuA(hMenu, flags, id, *str ? str : NULL);
      else
        AppendMenuW(hMenu, flags, id,
                    *(LPCWSTR)str ? (LPCWSTR)str : NULL);
    }
  } while(!end);

  return res;
}


BOOL
MenuInit(VOID)
{
  return TRUE;
/*  NONCLIENTMETRICSW ncm;
  BITMAP bm;

  hStdMnArrow = LoadBitmapA(0, MAKEINTRESOURCEA(OBM_MNARROW));
  if (hStdMnArrow == NULL)
    {
      return(FALSE);
    }
  GetObjectA(hStdMnArrow, sizeof(bm), &bm);
  ArrowBitmapWidth = bm.bmWidth;
  ArrowBitmapHeight = bm.bmHeight;

  if (!SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0))
    {
      return(FALSE);
    }

  hMenuFont = CreateFontIndirect(&ncm.lfMenuFont);
  if (hMenuFont == NULL)
    {
      return(FALSE);
    }

  ncm.lfMenuFont.lfWeight = max(ncm.lfMenuFont.lfWeight + 300, 1000);
  
  hMenuFontBold = CreateFontIndirect(&ncm.lfMenuFont);
  if (hMenuFontBold == NULL)
    {
      return(FALSE);
    }

  return(TRUE);*/
}


ULONG
MenuGetMenuBarHeight(HWND hWnd, ULONG MenuBarWidth, LONG OrgX, LONG OrgY)
{
  /*ULONG MenuId;
  PPOPUP_MENU Menu;
  RECT Rect;
  HDC hDC;

  MenuId = GetWindowLong(hWnd, GWL_ID);
  Menu = MenuGetMenu((HMENU)MenuId);
  if (Menu == NULL)
    {
      return(0);
    }
  hDC = GetDCEx(hWnd, 0, DCX_CACHE | DCX_WINDOW);
  SelectObject(hDC, hMenuFont);
  SetRect(&Rect, OrgX, OrgY, OrgX + MenuBarWidth, 
	  OrgY + GetSystemMetrics(SM_CYMENU));
  MenuMenuBarCalcSize(hDC, &Rect, Menu, hWnd);
  ReleaseDC(hWnd, hDC);*/
  return(GetSystemMetrics(SM_CYMENU));
}


UINT
MenuDrawMenuBar(HDC hDC, LPRECT Rect, HWND hWnd, BOOL Draw)
{
  return(GetSystemMetrics(SM_CYMENU));
  /*
  ULONG MenuID;
  PPOPUP_MENU Menu;
  HFONT hFontOld;
  ULONG i;

  MenuID = GetWindowLong(hWnd, GWL_ID);
  Menu = MenuGetMenu((HMENU)MenuID);

  if (Menu == NULL || Rect == NULL)
    {
      return(GetSystemMetrics(SM_CYMENU));
    }

  hFontOld = SelectObject(hDC, hMenuFont);

  if (Menu->Height == 0)
    {
      MenuMenuBarCalcSize(hDC, Rect, Menu, hWnd);
    }

  Rect->bottom = Rect->top + Menu->Height;

  if (!Draw)
    {
      SelectObject(hDC, hFontOld);
      return(Menu->Height);
    }

  FillRect(hDC, Rect, GetSysColorBrush(COLOR_MENU));

  SelectObject(hDC, GetSysColorPen(COLOR_WINDOWFRAME));
  MoveToEx(hDC, Rect->left, Rect->bottom, NULL);
  LineTo(hDC, Rect->right, Rect->bottom);

  if (Menu->NrItems == 0)
    {
      SelectObject(hDC, hFontOld);
      return(GetSystemMetrics(SM_CYMENU));
    }

  for (i = 0; i < Menu->NrItems; i++)
    {
      MenuDrawMenuItem(hWnd, (HMENU)MenuID, hWnd, hDC,
		       Menu->Items + i, Menu->Height, TRUE, ODA_DRAWENTIRE);
    }

  SelectObject(hDC, hFontOld);
  return(Menu->Height);*/
}


VOID
MenuTrackMouseMenuBar(HWND hWnd, ULONG Ht, POINT Pt)
{
}


VOID
MenuTrackKbdMenuBar(HWND hWnd, ULONG wParam, ULONG Key)
{
}

/* FUNCTIONS *****************************************************************/

/*static BOOL
MenuIsStringItem(ULONG TypeData)
{
  return((TypeData & MENU_TYPE_MASK) == MF_STRING);
}*/


/*
 * @implemented
 */
WINBOOL STDCALL
AppendMenuA(HMENU hMenu,
	    UINT uFlags,
	    UINT_PTR uIDNewItem,
	    LPCSTR lpNewItem)
{
  return(InsertMenuA(hMenu, -1, uFlags | MF_BYPOSITION, uIDNewItem, 
		     lpNewItem));
}


/*
 * @implemented
 */
WINBOOL STDCALL
AppendMenuW(HMENU hMenu,
	    UINT uFlags,
	    UINT_PTR uIDNewItem,
	    LPCWSTR lpNewItem)
{
  return(InsertMenuW(hMenu, -1, uFlags | MF_BYPOSITION, uIDNewItem, 
		     lpNewItem));
}


/*
 * @implemented
 */
DWORD STDCALL
CheckMenuItem(HMENU hmenu,
	      UINT uIDCheckItem,
	      UINT uCheck)
{
  return NtUserCheckMenuItem(hmenu, uIDCheckItem, uCheck);
}


/*
 * @unimplemented
 */
WINBOOL STDCALL
CheckMenuRadioItem(HMENU hmenu,
		   UINT idFirst,
		   UINT idLast,
		   UINT idCheck,
		   UINT uFlags)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @implemented
 */
HMENU STDCALL
CreateMenu(VOID)
{
  return NtUserCreateMenu();
}


/*
 * @implemented
 */
HMENU STDCALL
CreatePopupMenu(VOID)
{
  /* FIXME - add MF_POPUP style? */
  return NtUserCreateMenu();
}


/*
 * @implemented
 */
WINBOOL STDCALL
DeleteMenu(HMENU hMenu,
	   UINT uPosition,
	   UINT uFlags)
{
  return NtUserDeleteMenu(hMenu, uPosition, uFlags);
}


/*
 * @implemented
 */
WINBOOL STDCALL
DestroyMenu(HMENU hMenu)
{
    return NtUserDestroyMenu(hMenu);
}


/*
 * @unimplemented
 */
WINBOOL STDCALL
DrawMenuBar(HWND hWnd)
{
  UNIMPLEMENTED
  return FALSE;
}


/*
 * @implemented
 */
UINT STDCALL
EnableMenuItem(HMENU hMenu,
	       UINT uIDEnableItem,
	       UINT uEnable)
{
  return NtUserEnableMenuItem(hMenu, uIDEnableItem, uEnable);
}

/*
 * @unimplemented
 */
WINBOOL STDCALL
EndMenu(VOID)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
HMENU STDCALL
GetMenu(HWND hWnd)
{
  return (HMENU)NtUserCallOneParam((DWORD)hWnd, ONEPARAM_ROUTINE_GETMENU);
}


/*
 * @unimplemented
 */
WINBOOL STDCALL
GetMenuBarInfo(HWND hwnd,
	       LONG idObject,
	       LONG idItem,
	       PMENUBARINFO pmbi)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @implemented
 */
LONG STDCALL
GetMenuCheckMarkDimensions(VOID)
{
  return(MAKELONG(GetSystemMetrics(SM_CXMENUCHECK), 
		  GetSystemMetrics(SM_CYMENUCHECK)));
}


/*
 * @unimplemented
 */
UINT STDCALL
GetMenuDefaultItem(HMENU hMenu,
		   UINT fByPos,
		   UINT gmdiFlags)
{
  UNIMPLEMENTED;
  return -1;
}


/*
 * @implemented
 */
WINBOOL STDCALL
GetMenuInfo(HMENU hmenu,
	    LPMENUINFO lpcmi)
{
  MENUINFO mi;
  BOOL res = FALSE;
  
  if(!lpcmi || (lpcmi->cbSize != sizeof(MENUINFO)))
    return FALSE;
  
  RtlZeroMemory(&mi, sizeof(MENUINFO));
  mi.cbSize = sizeof(MENUINFO);
  mi.fMask = lpcmi->fMask;
  
  res = NtUserMenuInfo(hmenu, &mi, FALSE);
  
  memcpy(lpcmi, &mi, sizeof(MENUINFO));
  return res;
}


/*
 * @implemented
 */
int STDCALL
GetMenuItemCount(HMENU hMenu)
{
  return NtUserBuildMenuItemList(hMenu, NULL, 0, 0);
}


/*
 * @implemented
 */
UINT STDCALL
GetMenuItemID(HMENU hMenu,
	      int nPos)
{
  MENUITEMINFOW mii;
  
  mii.cbSize = sizeof(MENUITEMINFOW);
  mii.fMask = MIIM_ID | MIIM_SUBMENU;
  
  if(!NtUserMenuItemInfo(hMenu, nPos, MF_BYPOSITION, &mii, FALSE))
  {
    return -1;
  }
  
  if(mii.hSubMenu) return -1;
  if(mii.wID == 0) return -1;
  
  return mii.wID;
}


/*
 * @unimplemented
 */
WINBOOL STDCALL
GetMenuItemInfoA(
  HMENU hMenu,
  UINT uItem,
  WINBOOL fByPosition,
  LPMENUITEMINFOA lpmii)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
WINBOOL
STDCALL
GetMenuItemInfoW(
  HMENU hMenu,
  UINT uItem,
  WINBOOL fByPosition,
  LPMENUITEMINFOW lpmii)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
WINBOOL STDCALL
GetMenuItemRect(HWND hWnd,
		HMENU hMenu,
		UINT uItem,
		LPRECT lprcItem)
{
  UNIMPLEMENTED;
  return(FALSE);
}


/*
 * @implemented
 */
UINT
STDCALL
GetMenuState(
  HMENU hMenu,
  UINT uId,
  UINT uFlags)
{
  MENUITEMINFOW mii;
  mii.cbSize = sizeof(MENUITEMINFOW);
  mii.fMask = MIIM_STATE | MIIM_TYPE | MIIM_SUBMENU;
  
  SetLastError(0);
  if(NtUserMenuItemInfo(hMenu, uId, uFlags, &mii, FALSE))
  {
    UINT nSubItems = 0;
    if(mii.hSubMenu)
    {
      nSubItems = (UINT)NtUserBuildMenuItemList(mii.hSubMenu, NULL, 0, 0);
      
      /* FIXME - ported from wine, does that work (0xff)? */
      if(GetLastError() != ERROR_INVALID_MENU_HANDLE)
        return (nSubItems << 8) | ((mii.fState | mii.fType) & 0xff);

      return (UINT)-1; /* Invalid submenu */
    }
    
    /* FIXME - ported from wine, does that work? */
    return (mii.fType | mii.fState);
  }
  
  return (UINT)-1;
}


/*
 * @unimplemented
 */
int
STDCALL
GetMenuStringA(
  HMENU hMenu,
  UINT uIDItem,
  LPSTR lpString,
  int nMaxCount,
  UINT uFlag)
{
  UNIMPLEMENTED;
  return 0;
}


/*
 * @unimplemented
 */
int
STDCALL
GetMenuStringW(
  HMENU hMenu,
  UINT uIDItem,
  LPWSTR lpString,
  int nMaxCount,
  UINT uFlag)
{
  UNIMPLEMENTED;
  return 0;
}


/*
 * @implemented
 */
HMENU
STDCALL
GetSubMenu(
  HMENU hMenu,
  int nPos)
{
  MENUITEMINFOW mi;
  mi.cbSize = sizeof(MENUITEMINFO);
  mi.fMask = MIIM_SUBMENU;
  if(NtUserMenuItemInfo(hMenu, (UINT)nPos, MF_BYPOSITION, &mi, FALSE))
  {
    return mi.hSubMenu;
  }
  return (HMENU)0;
}


/*
 * @implemented
 */
WINBOOL
STDCALL
HiliteMenuItem(
  HWND hwnd,
  HMENU hmenu,
  UINT uItemHilite,
  UINT uHilite)
{
  return NtUserHiliteMenuItem(hwnd, hmenu, uItemHilite, uHilite);
}


/*
 * @unimplemented
 */
WINBOOL
STDCALL
InsertMenuA(
  HMENU hMenu,
  UINT uPosition,
  UINT uFlags,
  UINT_PTR uIDNewItem,
  LPCSTR lpNewItem)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @implemented
 */
WINBOOL
STDCALL
InsertMenuItemA(
  HMENU hMenu,
  UINT uItem,
  WINBOOL fByPosition,
  LPCMENUITEMINFOA lpmii)
{
  MENUITEMINFOW mi;
  WINBOOL res = FALSE;
  BOOL CleanHeap = FALSE;
  NTSTATUS Status;
  HANDLE hHeap = RtlGetProcessHeap();

  if((lpmii->cbSize == sizeof(MENUITEMINFO)) || 
     (lpmii->cbSize == sizeof(MENUITEMINFO) - sizeof(HBITMAP)))
  {
    memcpy(&mi, lpmii, lpmii->cbSize);
    
    /* copy the text string */
    if((mi.fMask & (MIIM_TYPE | MIIM_STRING)) && 
      (MENU_ITEM_TYPE(mi.fType) == MF_STRING) && mi.dwTypeData)
    {
      Status = HEAP_strdupA2W (hHeap, &mi.dwTypeData, (LPCSTR)mi.dwTypeData, &mi.cch);
      if (!NT_SUCCESS (Status))
      {
        SetLastError (RtlNtStatusToDosError(Status));
        return FALSE;
      }
      CleanHeap = TRUE;
    }
    
    res = NtUserInsertMenuItem(hMenu, uItem, fByPosition, &mi);
    
    if(CleanHeap) RtlFreeHeap (hHeap, 0, mi.dwTypeData);
  }
  return res;
}


/*
 * @implemented
 */
WINBOOL
STDCALL
InsertMenuItemW(
  HMENU hMenu,
  UINT uItem,
  WINBOOL fByPosition,
  LPCMENUITEMINFOW lpmii)
{
  MENUITEMINFOW mi;
  WINBOOL res = FALSE;
  BOOL CleanHeap = FALSE;
  ULONG len = 0;
  HANDLE hHeap = RtlGetProcessHeap();

  if((lpmii->cbSize == sizeof(MENUITEMINFO)) || 
     (lpmii->cbSize == sizeof(MENUITEMINFO) - sizeof(HBITMAP)))
  {
    memcpy(&mi, lpmii, lpmii->cbSize);
    
    /* copy the text string */
    if((mi.fMask & (MIIM_TYPE | MIIM_STRING)) && 
      (MENU_ITEM_TYPE(mi.fType) == MF_STRING) && mi.dwTypeData)
    {
      len = lstrlenW(lpmii->dwTypeData);

      mi.dwTypeData = RtlAllocateHeap(hHeap, 0, (len + 1) * sizeof(WCHAR));
      if(!mi.dwTypeData)
      {
        SetLastError (RtlNtStatusToDosError(STATUS_NO_MEMORY));
        return FALSE;
      }
      memcpy(&mi.dwTypeData, &lpmii->dwTypeData, len);
      CleanHeap = TRUE;
      mi.cch = len;
    }
    
    res = NtUserInsertMenuItem(hMenu, uItem, fByPosition, &mi);
    
    if(CleanHeap) RtlFreeHeap (hHeap, 0, mi.dwTypeData);
  }
  return res;
}


/*
 * @unimplemented
 */
WINBOOL
STDCALL
InsertMenuW(
  HMENU hMenu,
  UINT uPosition,
  UINT uFlags,
  UINT_PTR uIDNewItem,
  LPCWSTR lpNewItem)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @implemented
 */
WINBOOL
STDCALL
IsMenu(
  HMENU hMenu)
{
  SetLastError(ERROR_SUCCESS);
  DWORD ret = NtUserBuildMenuItemList(hMenu, NULL, 0, 0);
  return ((ret == -1) || (GetLastError() == ERROR_INVALID_MENU_HANDLE));
}


/*
 * @implemented
 */
HMENU STDCALL
LoadMenuA(HINSTANCE hInstance,
	  LPCSTR lpMenuName)
{
  HANDLE Resource = FindResourceA(hInstance, lpMenuName, MAKEINTRESOURCEA(4));
  if (Resource == NULL)
    {
      return(NULL);
    }
  return(LoadMenuIndirectA((PVOID)LoadResource(hInstance, Resource)));
}


/*
 * @implemented
 */
HMENU STDCALL
LoadMenuIndirectA(CONST MENUTEMPLATE *lpMenuTemplate)
{
  return(LoadMenuIndirectW(lpMenuTemplate));
}


/*
 * @implemented
 */
HMENU STDCALL
LoadMenuIndirectW(CONST MENUTEMPLATE *lpMenuTemplate)
{
  HMENU hMenu;
  WORD version, offset;
  LPCSTR p = (LPCSTR)lpMenuTemplate;

  version = GET_WORD(p);
  p += sizeof(WORD);

  switch (version)
  {
    case 0: /* standard format is version of 0 */
      offset = GET_WORD(p);
      p += sizeof(WORD) + offset;
      if (!(hMenu = CreateMenu())) return 0;
      if (!MENU_ParseResource(p, hMenu, TRUE))
      {
        DestroyMenu(hMenu);
        return 0;
      }
      return hMenu;
    case 1: /* extended format is version of 1 */
      offset = GET_WORD(p);
      p += sizeof(WORD) + offset;
      if (!(hMenu = CreateMenu())) return 0;
      if (!MENUEX_ParseResource(p, hMenu))
      {
        DestroyMenu( hMenu );
        return 0;
      }
      return hMenu;
    default:
      DbgPrint("LoadMenuIndirectW(): version %d not supported.\n", version);
      return 0;
  }
}


/*
 * @implemented
 */
HMENU STDCALL
LoadMenuW(HINSTANCE hInstance,
	  LPCWSTR lpMenuName)
{
  HANDLE Resource = FindResourceW(hInstance, lpMenuName, RT_MENU);
  if (Resource == NULL)
    {
      return(NULL);
    }
  return(LoadMenuIndirectW((PVOID)LoadResource(hInstance, Resource)));
}


/*
 * @unimplemented
 */
int
STDCALL
MenuItemFromPoint(
  HWND hWnd,
  HMENU hMenu,
  POINT ptScreen)
{
  UNIMPLEMENTED;
  return 0;
}


/*
 * @unimplemented
 */
WINBOOL
STDCALL
ModifyMenuA(
  HMENU hMnu,
  UINT uPosition,
  UINT uFlags,
  UINT_PTR uIDNewItem,
  LPCSTR lpNewItem)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
WINBOOL
STDCALL
ModifyMenuW(
  HMENU hMnu,
  UINT uPosition,
  UINT uFlags,
  UINT_PTR uIDNewItem,
  LPCWSTR lpNewItem)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @implemented
 */
WINBOOL
STDCALL
RemoveMenu(
  HMENU hMenu,
  UINT uPosition,
  UINT uFlags)
{
  return NtUserRemoveMenu(hMenu, uPosition, uFlags);
}


/*
 * @unimplemented
 */
WINBOOL STDCALL
SetMenu(HWND hWnd,
	HMENU hMenu)
{
  UNIMPLEMENTED;
  /* from wine
  if (IsWindowVisible(hWnd))
    {
      SetWindowPos(hWnd, 0, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE |
		   SWP_NOACTIVATE | SWP_NOZORDER | SWP_FRAMECHANGED);
    }
  */
  return FALSE;
}


/*
 * @implemented
 */
WINBOOL
STDCALL
SetMenuDefaultItem(
  HMENU hMenu,
  UINT uItem,
  UINT fByPos)
{
  return NtUserSetMenuDefaultItem(hMenu, uItem, fByPos);
}


/*
 * @implemented
 */
WINBOOL
STDCALL
SetMenuInfo(
  HMENU hmenu,
  LPCMENUINFO lpcmi)
{
  MENUINFO mi;
  BOOL res = FALSE;
  if(lpcmi->cbSize != sizeof(MENUINFO))
    return res;
    
  memcpy(&mi, lpcmi, sizeof(MENUINFO));
  return NtUserMenuInfo(hmenu, &mi, TRUE);
}


/*
 * @unimplemented
 */
WINBOOL
STDCALL
SetMenuItemBitmaps(
  HMENU hMenu,
  UINT uPosition,
  UINT uFlags,
  HBITMAP hBitmapUnchecked,
  HBITMAP hBitmapChecked)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
WINBOOL
STDCALL
SetMenuItemInfoA(
  HMENU hMenu,
  UINT uItem,
  WINBOOL fByPosition,
  LPMENUITEMINFOA lpmii)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
WINBOOL
STDCALL
SetMenuItemInfoW(
  HMENU hMenu,
  UINT uItem,
  WINBOOL fByPosition,
  LPMENUITEMINFOW lpmii)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
WINBOOL
STDCALL
TrackPopupMenu(
  HMENU hMenu,
  UINT uFlags,
  int x,
  int y,
  int nReserved,
  HWND hWnd,
  CONST RECT *prcRect)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
WINBOOL
STDCALL
TrackPopupMenuEx(
  HMENU hmenu,
  UINT fuFlags,
  int x,
  int y,
  HWND hwnd,
  LPTPMPARAMS lptpm)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @implemented
 */
WINBOOL
STDCALL
SetMenuContextHelpId(HMENU hmenu,
          DWORD dwContextHelpId)
{
  return NtUserSetMenuContextHelpId(hmenu, dwContextHelpId);
}


/*
 * @implemented
 */
DWORD
STDCALL
GetMenuContextHelpId(HMENU hmenu)
{
  MENUINFO mi;
  mi.cbSize = sizeof(MENUINFO);
  mi.fMask = MIM_HELPID;
  
  if(NtUserMenuInfo(hmenu, &mi, FALSE))
  {
    return mi.dwContextHelpID;
  }
  return 0;
}

