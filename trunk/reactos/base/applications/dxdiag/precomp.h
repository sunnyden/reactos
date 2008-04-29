#ifndef PRECOMP_H__
#define PRECOMP_H__

#define DIRECTINPUT_VERSION 0x0800
#define DIRECTSOUND_VERSION 0x0800
#define D3D_OVERLOADS
#define _SETUPAPI_VER _WIN32_WINNT

#include <stdio.h>
#include <windows.h>
#include <limits.h>
#include <setupapi.h>
#include <commctrl.h>
#include <dinput.h>
#include <ddraw.h>

#include <dsound.h>
#include "resource.h"

typedef struct
{
    HWND hMainDialog;
    HWND hDialogs[7];

}DXDIAG_CONTEXT, *PDXDIAG_CONTEXT;



/* globals */
extern HINSTANCE hInst;

/* dialog wnd proc */
INT_PTR CALLBACK SystemPageWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK DisplayPageWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK SoundPageWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK MusicPageWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK InputPageWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK NetworkPageWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK HelpPageWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
BOOL GetRegValue(HKEY hBaseKey, LPWSTR SubKey, LPWSTR ValueName, DWORD Type, LPWSTR Result, DWORD Size);

/* DirectDraw tests */
BOOL StartDDTest(HWND hWnd, HINSTANCE hInstance, INT resTestDescription, INT resResult, INT TestNr);


BOOL GetFileVersion(LPCWSTR szAppName, WCHAR * szVer, DWORD szVerSize);
BOOL GetFileModifyTime(LPCWSTR pFullPath, WCHAR * szTime, int szTimeSize);
#endif
