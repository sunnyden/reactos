#ifndef _INPUT_H
#define _INPUT_H

#include <stdarg.h>

#define WIN32_NO_STATUS
#include <windef.h>
#include <winbase.h>
#include <winnls.h>
#include <winreg.h>
#include <winuser.h>
#include <wingdi.h>
#include <commctrl.h>
#include <windowsx.h>
#include <setupapi.h>
#include <wchar.h>
#include <strsafe.h>
#include <stdlib.h>

#include "resource.h"

typedef LONG (CALLBACK *CPLAPPLET_PROC)(VOID);

typedef struct
{
    int idIcon;
    int idName;
    int idDescription;
    CPLAPPLET_PROC AppletProc;
} APPLET, *PAPPLET;

extern HINSTANCE hApplet;

// Character Count of a layout ID like "00000409"
#define CCH_LAYOUT_ID    8

// Maximum Character Count of a ULONG in decimal
#define CCH_ULONG_DEC    10

#define MAX_STR_LEN      256

/* settings_page.c */
INT_PTR CALLBACK
SettingsPageProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

/* advanced_settings_page.c */
INT_PTR CALLBACK
AdvancedSettingsPageProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

/* add_dialog.c */
INT_PTR CALLBACK
AddDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

/* edit_dialog.c */
INT_PTR CALLBACK
EditDialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

/* key_settings_dialog.c */

typedef struct
{
    DWORD dwAttributes;
    DWORD dwLanguage;
    DWORD dwLayout;
} KEY_SETTINGS;

INT_PTR CALLBACK
KeySettingsDialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

/* key_sequence_dialog.c */
INT_PTR CALLBACK
ChangeKeySeqDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);


static inline WCHAR*
DublicateString(const WCHAR *pszString)
{
    WCHAR *pszDublicate;
    size_t size;

    size = (wcslen(pszString) + 1) * sizeof(WCHAR);

    pszDublicate = (WCHAR*) malloc(size);
    if (pszDublicate != NULL)
    {
        StringCbCopyW(pszDublicate, size, pszString);
    }

    return pszDublicate;
}

static inline DWORD
DWORDfromString(const WCHAR *pszString)
{
    WCHAR *pszEnd;

    return wcstoul(pszString, &pszEnd, 16);
}

#endif /* _INPUT_H */
