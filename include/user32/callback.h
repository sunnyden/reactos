#ifndef __INCLUDE_USER32_CALLBACK_H
#define __INCLUDE_USER32_CALLBACK_H

#define USER32_CALLBACK_WINDOWPROC            (0)
#define USER32_CALLBACK_SENDASYNCPROC         (1)
#define USER32_CALLBACK_SENDNCCREATE          (2)
#define USER32_CALLBACK_SENDNCCALCSIZE        (3)
#define USER32_CALLBACK_SENDCREATE            (4)
#define USER32_CALLBACK_SENDGETMINMAXINFO     (5)
#define USER32_CALLBACK_SENDWINDOWPOSCHANGING (6)
#define USER32_CALLBACK_SENDWINDOWPOSCHANGED  (7)
#define USER32_CALLBACK_SENDSTYLECHANGING     (8)
#define USER32_CALLBACK_SENDSTYLECHANGED      (9)
#define USER32_CALLBACK_LOADSYSMENUTEMPLATE   (10)
#define USER32_CALLBACK_MAXIMUM               (10)

typedef struct _WINDOWPROC_CALLBACK_ARGUMENTS
{
  WNDPROC Proc;
  HWND Wnd;
  UINT Msg;
  WPARAM wParam;
  LPARAM lParam;
} WINDOWPROC_CALLBACK_ARGUMENTS, *PWINDOWPROC_CALLBACK_ARGUMENTS;

typedef struct _SENDASYNCPROC_CALLBACK_ARGUMENTS
{
  SENDASYNCPROC Callback;
  HWND Wnd;
  UINT Msg;
  ULONG_PTR Context;
  LRESULT Result;
} SENDASYNCPROC_CALLBACK_ARGUMENTS, *PSENDASYNCPROC_CALLBACK_ARGUMENTS;

typedef struct _SENDNCCREATEMESSAGE_CALLBACK_ARGUMENTS
{
  HWND Wnd;
  CREATESTRUCTW CreateStruct;
} SENDNCCREATEMESSAGE_CALLBACK_ARGUMENTS, 
  *PSENDNCCREATEMESSAGE_CALLBACK_ARGUMENTS;

typedef struct _SENDCREATEMESSAGE_CALLBACK_ARGUMENTS
{
  HWND Wnd;
  CREATESTRUCTW CreateStruct;
} SENDCREATEMESSAGE_CALLBACK_ARGUMENTS, *PSENDCREATEMESSAGE_CALLBACK_ARGUMENTS;

typedef struct _SENDNCCALCSIZEMESSAGE_CALLBACK_ARGUMENTS
{
  HWND Wnd;
  BOOL Validate;
  RECT Rect;
  NCCALCSIZE_PARAMS Params;
} SENDNCCALCSIZEMESSAGE_CALLBACK_ARGUMENTS, 
  *PSENDNCCALCSIZEMESSAGE_CALLBACK_ARGUMENTS;

typedef struct _SENDNCCALCSIZEMESSAGE_CALLBACK_RESULT
{
  LRESULT Result;
  RECT Rect;
  NCCALCSIZE_PARAMS Params;
} SENDNCCALCSIZEMESSAGE_CALLBACK_RESULT,
  *PSENDNCCALCSIZEMESSAGE_CALLBACK_RESULT;

typedef struct _SENDGETMINMAXINFO_CALLBACK_ARGUMENTS
{
  HWND Wnd;
  MINMAXINFO MinMaxInfo;
} SENDGETMINMAXINFO_CALLBACK_ARGUMENTS, *PSENDGETMINMAXINFO_CALLBACK_ARGUMENTS;

typedef struct _SENDGETMINMAXINFO_CALLBACK_RESULT
{
  LRESULT Result;
  MINMAXINFO MinMaxInfo;
} SENDGETMINMAXINFO_CALLBACK_RESULT, *PSENDGETMINMAXINFO_CALLBACK_RESULT;

typedef struct _SENDWINDOWPOSCHANGING_CALLBACK_ARGUMENTS
{
  HWND Wnd;
  WINDOWPOS WindowPos;
} SENDWINDOWPOSCHANGING_CALLBACK_ARGUMENTS, *PSENDWINDOWPOSCHANGING_CALLBACK_ARGUMENTS;

typedef struct _SENDWINDOWPOSCHANGED_CALLBACK_ARGUMENTS
{
  HWND Wnd;
  WINDOWPOS WindowPos;
} SENDWINDOWPOSCHANGED_CALLBACK_ARGUMENTS, *PSENDWINDOWPOSCHANGED_CALLBACK_ARGUMENTS;

typedef struct _SENDSTYLECHANGING_CALLBACK_ARGUMENTS
{
  HWND Wnd;
  STYLESTRUCT Style;
  DWORD WhichStyle;
} SENDSTYLECHANGING_CALLBACK_ARGUMENTS, *PSENDSTYLECHANGING_CALLBACK_ARGUMENTS;

typedef struct _SENDSTYLECHANGED_CALLBACK_ARGUMENTS
{
  HWND Wnd;
  STYLESTRUCT Style;
  DWORD WhichStyle;
} SENDSTYLECHANGED_CALLBACK_ARGUMENTS, *PSENDSTYLECHANGED_CALLBACK_ARGUMENTS;

NTSTATUS STDCALL
User32CallWindowProcFromKernel(PVOID Arguments, ULONG ArgumentLength);
NTSTATUS STDCALL
User32CallSendAsyncProcForKernel(PVOID Arguments, ULONG ArgumentLength);
NTSTATUS STDCALL
User32SendNCCREATEMessageForKernel(PVOID Arguments, ULONG ArgumentLength);
NTSTATUS STDCALL
User32SendCREATEMessageForKernel(PVOID Arguments, ULONG ArgumentLength);
NTSTATUS STDCALL
User32SendGETMINMAXINFOMessageForKernel(PVOID Arguments, ULONG ArgumentLength);
NTSTATUS STDCALL
User32SendNCCALCSIZEMessageForKernel(PVOID Arguments, ULONG ArgumentLength);
NTSTATUS STDCALL
User32SendWINDOWPOSCHANGINGMessageForKernel(PVOID Arguments, ULONG ArgumentLength);
NTSTATUS STDCALL
User32SendWINDOWPOSCHANGEDMessageForKernel(PVOID Arguments, ULONG ArgumentLength);
NTSTATUS STDCALL
User32SendSTYLECHANGINGMessageForKernel(PVOID Arguments, ULONG ArgumentLength);
NTSTATUS STDCALL
User32SendSTYLECHANGEDMessageForKernel(PVOID Arguments, ULONG ArgumentLength);
NTSTATUS STDCALL
User32LoadSysMenuTemplateForKernel(PVOID Arguments, ULONG ArgumentLength);

#endif /* __INCLUDE_USER32_CALLBACK_H */
