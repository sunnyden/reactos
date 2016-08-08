/*
* PROJECT:         input.dll
* FILE:            dll/cpl/input/input_list.c
* PURPOSE:         input.dll
* PROGRAMMER:      Dmitry Chapyshev (dmitry@reactos.org)
*/

#include "input_list.h"


static INPUT_LIST_NODE *_InputList = NULL;


static INPUT_LIST_NODE*
InputList_AppendNode(VOID)
{
    INPUT_LIST_NODE *pCurrent;
    INPUT_LIST_NODE *pNew;

    pCurrent = _InputList;

    pNew = (INPUT_LIST_NODE*)malloc(sizeof(INPUT_LIST_NODE));
    if (pNew == NULL)
        return NULL;

    memset(pNew, 0, sizeof(INPUT_LIST_NODE));

    if (pCurrent == NULL)
    {
        _InputList = pNew;
    }
    else
    {
        while (pCurrent->pNext != NULL)
        {
            pCurrent = pCurrent->pNext;
        }

        pNew->pPrev = pCurrent;
        pCurrent->pNext = pNew;
    }

    return pNew;
}


VOID
InputList_Remove(INPUT_LIST_NODE *pNode)
{
    INPUT_LIST_NODE *pCurrent = pNode;

    if (_InputList == NULL)
        return;

    if (pCurrent != NULL)
    {
        INPUT_LIST_NODE *pNext = pCurrent->pNext;
        INPUT_LIST_NODE *pPrev = pCurrent->pPrev;

        free(pCurrent);

        if (pNext != NULL)
            pNext->pPrev = pPrev;

        if (pPrev != NULL)
            pPrev->pNext = pNext;
        else
            _InputList = pNext;
    }
}


VOID
InputList_Destroy(VOID)
{
    INPUT_LIST_NODE *pCurrent;

    if (_InputList == NULL)
        return;

    pCurrent = _InputList;

    while (pCurrent != NULL)
    {
        INPUT_LIST_NODE *pNext = pCurrent->pNext;

        free(pCurrent);

        pCurrent = pNext;
    }

    _InputList = NULL;
}


static BOOL
InputList_PrepareUserRegistry(VOID)
{
    BOOL bResult = FALSE;
    HKEY hTempKey = NULL;
    HKEY hKey = NULL;

    if (RegOpenKeyExW(HKEY_CURRENT_USER,
                      L"Keyboard Layout",
                      0,
                      KEY_ALL_ACCESS,
                      &hKey) == ERROR_SUCCESS)
    {
        RegDeleteKeyW(hKey, L"Preload");
        RegDeleteKeyW(hKey, L"Substitutes");
        //RegDeleteKeyW(hKey, L"Toggle");

        RegCloseKey(hKey);

        RegDeleteKeyW(HKEY_CURRENT_USER, L"Keyboard Layout");
    }

    if (RegCreateKeyW(HKEY_CURRENT_USER, L"Keyboard Layout", &hKey) != ERROR_SUCCESS)
    {
        goto Cleanup;
    }

    if (RegCreateKeyW(hKey, L"Preload", &hTempKey) != ERROR_SUCCESS)
    {
        goto Cleanup;
    }

    RegCloseKey(hTempKey);

    if (RegCreateKeyW(hKey, L"Substitutes", &hTempKey) != ERROR_SUCCESS)
    {
        goto Cleanup;
    }

    RegCloseKey(hTempKey);

    if (RegCreateKeyW(hKey, L"Toggle", &hTempKey) != ERROR_SUCCESS)
    {
        goto Cleanup;
    }

    RegCloseKey(hTempKey);

    bResult = TRUE;

Cleanup:
    if (hTempKey != NULL)
        RegCloseKey(hTempKey);
    if (hKey != NULL)
        RegCloseKey(hKey);

    return bResult;
}


static VOID
InputList_AddInputMethodToUserRegistry(DWORD dwIndex, INPUT_LIST_NODE *pNode)
{
    WCHAR szMethodIndex[MAX_PATH];
    WCHAR szPreload[MAX_PATH];
    BOOL bIsImeMethod = FALSE;
    HKEY hKey;

    StringCchPrintfW(szMethodIndex, ARRAYSIZE(szMethodIndex), L"%lu", dwIndex);

    /* Check is IME method */
    if ((HIWORD(pNode->pLayout->dwId) & 0xF000) == 0xE000)
    {
        StringCchPrintfW(szPreload, ARRAYSIZE(szPreload), L"%08X", pNode->pLayout->dwId);
        bIsImeMethod = TRUE;
    }
    else
    {
        StringCchPrintfW(szPreload, ARRAYSIZE(szPreload), L"%08X", pNode->pLocale->dwId);
    }

    if (RegOpenKeyExW(HKEY_CURRENT_USER,
                      L"Keyboard Layout\\Preload",
                      0,
                      KEY_SET_VALUE,
                      &hKey) == ERROR_SUCCESS)
    {
        RegSetValueExW(hKey,
                       szMethodIndex,
                       0,
                       REG_SZ,
                       (LPBYTE)szPreload,
                       (wcslen(szPreload) + 1) * sizeof(WCHAR));

        RegCloseKey(hKey);
    }

    if (pNode->pLocale->dwId != pNode->pLayout->dwId && bIsImeMethod == FALSE)
    {
        if (RegOpenKeyExW(HKEY_CURRENT_USER,
                          L"Keyboard Layout\\Substitutes",
                          0,
                          KEY_SET_VALUE,
                          &hKey) == ERROR_SUCCESS)
        {
            WCHAR szSubstitues[MAX_PATH];

            StringCchPrintfW(szSubstitues, ARRAYSIZE(szSubstitues), L"%08X", pNode->pLayout->dwId);

            RegSetValueExW(hKey,
                           szPreload,
                           0,
                           REG_SZ,
                           (LPBYTE)szSubstitues,
                           (wcslen(szSubstitues) + 1) * sizeof(WCHAR));

            RegCloseKey(hKey);
        }
    }

    if (pNode->dwFlags & INPUT_LIST_NODE_FLAG_ADDED)
    {
        pNode->hkl = LoadKeyboardLayoutW(szPreload, KLF_SUBSTITUTE_OK | KLF_NOTELLSHELL);
    }
}


VOID
InputList_Process(VOID)
{
    INPUT_LIST_NODE *pCurrent;
    DWORD dwIndex;

    /* Process deleted input methods */
    for (pCurrent = _InputList; pCurrent != NULL; pCurrent = pCurrent->pNext)
    {
        if (!(pCurrent->dwFlags & INPUT_LIST_NODE_FLAG_DELETED))
            continue;

        if (UnloadKeyboardLayout(pCurrent->hkl))
        {
            InputList_Remove(pCurrent);
        }
    }

    InputList_PrepareUserRegistry();

    /* Find default input method */
    for (pCurrent = _InputList; pCurrent != NULL; pCurrent = pCurrent->pNext)
    {
        if (pCurrent->dwFlags & INPUT_LIST_NODE_FLAG_DEFAULT)
        {
            InputList_AddInputMethodToUserRegistry(1, pCurrent);
            break;
        }
    }

    if (SystemParametersInfoW(SPI_SETDEFAULTINPUTLANG,
                              0,
                              (LPVOID)((LPDWORD)&pCurrent->hkl),
                              0))
    {
        DWORD dwRecipients;

        dwRecipients = BSM_ALLCOMPONENTS;

        BroadcastSystemMessageW(BSF_POSTMESSAGE,
                                &dwRecipients,
                                WM_INPUTLANGCHANGEREQUEST,
                                0,
                                (LPARAM)pCurrent->hkl);
    }

    /* Add methods to registry */
    for (pCurrent = _InputList, dwIndex = 2; pCurrent != NULL; pCurrent = pCurrent->pNext, dwIndex++)
    {
        if (pCurrent->dwFlags & INPUT_LIST_NODE_FLAG_DEFAULT)
            continue;

        InputList_AddInputMethodToUserRegistry(dwIndex, pCurrent);
    }
}


VOID
InputList_Add(LOCALE_LIST_NODE *pLocale, LAYOUT_LIST_NODE *pLayout)
{
    INPUT_LIST_NODE *pInput;

    if (pLocale == NULL || pLayout == NULL)
    {
        return;
    }

    pInput = InputList_AppendNode();

    pInput->dwFlags |= INPUT_LIST_NODE_FLAG_ADDED;

    pInput->pLocale = pLocale;
    pInput->pLayout = pLayout;
}


VOID
InputList_Create(VOID)
{
    INT iLayoutCount;
    HKL *pLayoutList;

    iLayoutCount = GetKeyboardLayoutList(0, NULL);
    pLayoutList = (HKL*)malloc(iLayoutCount * sizeof(HKL));

    if (pLayoutList != NULL)
    {
        if (GetKeyboardLayoutList(iLayoutCount, pLayoutList) > 0)
        {
            INT iIndex;

            for (iIndex = 0; iIndex < iLayoutCount; iIndex++)
            {
                LOCALE_LIST_NODE *pLocale = LocaleList_GetByHkl(pLayoutList[iIndex]);
                LAYOUT_LIST_NODE *pLayout = LayoutList_GetByHkl(pLayoutList[iIndex]);

                if (pLocale != NULL && pLayout != NULL)
                {
                    INPUT_LIST_NODE *pInput;

                    pInput = InputList_AppendNode();

                    pInput->dwFlags = 0;
                    pInput->pLocale = pLocale;
                    pInput->pLayout = pLayout;
                    pInput->hkl     = pLayoutList[iIndex];
                }
            }
        }

        free(pLayoutList);
    }
}


INPUT_LIST_NODE*
InputList_Get(VOID)
{
    return _InputList;
}
