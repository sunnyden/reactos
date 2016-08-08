/*
 * PROJECT:         input.dll
 * FILE:            dll/cpl/input/locale_list.c
 * PURPOSE:         input.dll
 * PROGRAMMER:      Dmitry Chapyshev (dmitry@reactos.org)
 */

#include "locale_list.h"


static LOCALE_LIST_NODE *_LocaleList = NULL;


static LOCALE_LIST_NODE*
LocaleList_Append(DWORD dwId, const WCHAR *pszName, const WCHAR *pszIndicator)
{
    LOCALE_LIST_NODE *pCurrent;
    LOCALE_LIST_NODE *pNew;

    if (pszName == NULL || pszIndicator == NULL)
        return NULL;

    pCurrent = _LocaleList;

    pNew = (LOCALE_LIST_NODE*) malloc(sizeof(LOCALE_LIST_NODE));
    if (pNew == NULL)
        return NULL;

    memset(pNew, 0, sizeof(LOCALE_LIST_NODE));

    pNew->pszName = DublicateString(pszName);
    if (pNew->pszName == NULL)
    {
        free(pNew);
        return NULL;
    }

    pNew->pszIndicator = DublicateString(pszIndicator);
    if (pNew->pszIndicator == NULL)
    {
        free(pNew->pszName);
        free(pNew);
        return NULL;
    }

    pNew->dwId = dwId;

    if (pCurrent == NULL)
    {
        _LocaleList = pNew;
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
LocaleList_Destroy(VOID)
{
    LOCALE_LIST_NODE *pCurrent;

    if (_LocaleList == NULL)
        return;

    pCurrent = _LocaleList;

    while (pCurrent != NULL)
    {
        LOCALE_LIST_NODE *pNext = pCurrent->pNext;

        free(pCurrent->pszName);
        free(pCurrent->pszIndicator);
        free(pCurrent);

        pCurrent = pNext;
    }

    _LocaleList = NULL;
}


LOCALE_LIST_NODE*
LocaleList_Create(VOID)
{
    WCHAR szValue[MAX_PATH];
    DWORD dwIndex;
    DWORD dwSize;
    HKEY hKey;

    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                      L"SYSTEM\\CurrentControlSet\\Control\\Nls\\Language",
                      0,
                      KEY_ENUMERATE_SUB_KEYS | KEY_QUERY_VALUE,
                      &hKey) != ERROR_SUCCESS)
    {
        return NULL;
    }

    dwSize = sizeof(szValue);
    dwIndex = 0;

    while (RegEnumValueW(hKey, dwIndex, szValue, &dwSize,
                         NULL, NULL, NULL, NULL) == ERROR_SUCCESS)
    {
        WCHAR szName[MAX_STR_LEN];
        DWORD dwId;

        dwId = DWORDfromString(szValue);

        if (GetLocaleInfoW(LOWORD(dwId),
                           LOCALE_SLANGUAGE,
                           szName, ARRAYSIZE(szName)))
        {
            WCHAR szIndicator[MAX_STR_LEN] = { 0 };

            if (GetLocaleInfoW(LOWORD(dwId),
                               LOCALE_SABBREVLANGNAME | LOCALE_NOUSEROVERRIDE,
                               szIndicator,
                               ARRAYSIZE(szIndicator)))
            {
                size_t len = wcslen(szIndicator);

                if (len > 0)
                {
                    szIndicator[len - 1] = 0;
                }
            }

            LocaleList_Append(dwId, szName, szIndicator);
        }

        dwSize = sizeof(szValue);
        ++dwIndex;
    }

    return _LocaleList;
}


LOCALE_LIST_NODE*
LocaleList_GetByHkl(HKL hkl)
{
    LOCALE_LIST_NODE *pCurrent;

    for (pCurrent = _LocaleList; pCurrent != NULL; pCurrent = pCurrent->pNext)
    {
        if (LOWORD(pCurrent->dwId) == LOWORD(hkl))
        {
            return pCurrent;
        }
    }

    return NULL;
}


LOCALE_LIST_NODE*
LocaleList_Get(VOID)
{
    return _LocaleList;
}
