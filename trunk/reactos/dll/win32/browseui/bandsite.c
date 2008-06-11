/*
 *	Rebar band site
 *
 *	Copyright 2007	Herv� Poussineua
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

#include "config.h"

#include <stdarg.h>

#define COBJMACROS

#include "wine/debug.h"
#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "winuser.h"
#include "shlwapi.h"
#include "winerror.h"
#include "objbase.h"
#include "commctrl.h"

#include "docobj.h"
#include "shlguid.h"
#include "shlobj.h"
#include "shobjidl.h"
#include "todo.h"
#include "undoc.h"

#include "wine/unicode.h"

#include "browseui.h"

WINE_DEFAULT_DEBUG_CHANNEL(browseui);

#ifndef ASSERT
#define ASSERT(cond) \
    if (!(cond)) \
        ERR ("ASSERTION %s AT %s:%d FAILED!\n", #cond, __FILE__, __LINE__)
#endif

extern HINSTANCE browseui_hinstance;

struct BandObject {
    IDeskBand *DeskBand;
};

typedef struct tagBandSite {
    const IBandSiteVtbl *vtbl;
    const IWindowEventHandlerVtbl *eventhandlerVtbl;
    const IDeskBarClientVtbl *deskbarVtbl;
    const IOleCommandTargetVtbl *oletargetVtbl;
    LONG refCount;
    LONG BandsCount;
    LONG BandsAllocated;
    struct BandObject *Bands;
    IUnknown *pUnkOuter;
    HWND hWndRebar;
    IOleWindow *OleWindow;
} BandSite;

static const IBandSiteVtbl BandSiteVtbl;
static const IWindowEventHandlerVtbl BandSite_EventHandlerVtbl;
static const IDeskBarClientVtbl BandSite_DeskBarVtbl;
static const IOleCommandTargetVtbl BandSite_OleTargetVtbl;

static inline BandSite *impl_from_IWindowEventHandler(IWindowEventHandler *iface)
{
    return (BandSite *)((char *)iface - FIELD_OFFSET(BandSite, eventhandlerVtbl));
}

static inline BandSite *impl_from_IDeskBarClient(IDeskBarClient *iface)
{
    return (BandSite *)((char *)iface - FIELD_OFFSET(BandSite, deskbarVtbl));
}

static inline BandSite *impl_from_IOleCommandTarget(IOleCommandTarget *iface)
{
    return (BandSite *)((char *)iface - FIELD_OFFSET(BandSite, oletargetVtbl));
}

static struct BandObject* GetBandByID(BandSite *This, DWORD dwBandID)
{
    if (dwBandID >= This->BandsAllocated)
        return NULL;

    if (This->Bands[dwBandID].DeskBand == NULL)
        return NULL;

    return &This->Bands[dwBandID];
}

static void FreeBand(BandSite *This, struct BandObject *Band)
{
    IUnknown_Release(Band->DeskBand);
    ZeroMemory(Band, sizeof(*Band));
    This->BandsCount--;
}

static DWORD GetBandSiteViewMode(BandSite *This)
{
    DWORD dwStyle;

    /* FIXME: What about DBIF_VIEWMODE_FLOATING and DBIF_VIEWMODE_TRANSPARENT? */
    dwStyle = GetWindowLong(This->hWndRebar,
                            GWL_STYLE);

    if (dwStyle & CCS_VERT)
        return DBIF_VIEWMODE_VERTICAL;
    else
        return DBIF_VIEWMODE_NORMAL;
}

static HRESULT UpdateAllBands(BandSite *This)
{
    DESKBANDINFO dbi;
    DWORD dwViewMode;
    LONG i;
    HRESULT hRet;

    dwViewMode = GetBandSiteViewMode(This);

    for (i = 0; i < This->BandsAllocated; i++)
    {
        if (This->Bands[i].DeskBand != NULL)
        {
            ZeroMemory (&dbi, sizeof(dbi));
            dbi.dwMask = 0;
            /* FIXME */

            hRet = IDeskBand_GetBandInfo(This->Bands[i].DeskBand,
                                         i,
                                         dwViewMode,
                                         &dbi);
            if (!SUCCEEDED(hRet))
                return hRet;
        }
    }

    return S_OK;
}

static HRESULT UpdateBand(BandSite *This, DWORD dwBandID)
{
    struct BandObject *Band;
    DESKBANDINFO dbi;
    DWORD dwViewMode;

    FIXME("UpdateBand(%x)\n", dwBandID);

    Band = GetBandByID(This,
                       dwBandID);
    if (Band == NULL)
        return E_FAIL;

    /* FIXME */
    ZeroMemory (&dbi, sizeof(dbi));
    dbi.dwMask = 0;

    dwViewMode = GetBandSiteViewMode(This);

    return IDeskBand_GetBandInfo(Band->DeskBand,
                                 dwBandID,
                                 dwViewMode,
                                 &dbi);
}

HRESULT WINAPI BandSite_Constructor(IUnknown *pUnkOuter, IUnknown **ppOut)
{
    BandSite *This;

    if (!pUnkOuter)
        return E_POINTER;

    This = CoTaskMemAlloc(sizeof(BandSite));
    if (This == NULL)
        return E_OUTOFMEMORY;
    ZeroMemory(This, sizeof(*This));
    This->pUnkOuter = pUnkOuter;
    IUnknown_AddRef(pUnkOuter);
    This->vtbl = &BandSiteVtbl;
    This->eventhandlerVtbl = &BandSite_EventHandlerVtbl;
    This->deskbarVtbl = &BandSite_DeskBarVtbl;
    This->oletargetVtbl = &BandSite_OleTargetVtbl;
    This->refCount = 1;

    TRACE("returning %p\n", This);
    *ppOut = (IUnknown *)This;
    BROWSEUI_refCount++;
    return S_OK;
}

static void WINAPI BandSite_Destructor(BandSite *This)
{
    int i;
    TRACE("destroying %p\n", This);
    
    if (This->hWndRebar != NULL)
    {
        DestroyWindow(This->hWndRebar);
        This->hWndRebar = NULL;
    }

    if (This->pUnkOuter != NULL)
    {
        IUnknown_Release(This->pUnkOuter);
        This->pUnkOuter = NULL;
    }
    
    if (This->Bands != NULL)
    {
        for (i = 0; i < This->BandsAllocated; i++)
            FreeBand(This, &This->Bands[i]);
        CoTaskMemFree(This->Bands);
        This->Bands = NULL;
    }
    
    if (This->OleWindow != NULL)
    {
        This->OleWindow->lpVtbl->Release(This->OleWindow);
        This->OleWindow = NULL;
    }
    
    CoTaskMemFree(This);
    BROWSEUI_refCount--;
}

static HRESULT WINAPI BandSite_QueryInterface(IBandSite *iface, REFIID iid, LPVOID *ppvOut)
{
    BandSite *This = (BandSite *)iface;
    *ppvOut = NULL;

    TRACE("(%p, %s, %p)\n", iface, debugstr_guid(iid), ppvOut);

    if (IsEqualIID(iid, &IID_IUnknown) || IsEqualIID(iid, &IID_IBandSite))
    {
        *ppvOut = &This->vtbl;
    }
    else if (IsEqualIID(iid, &IID_IWindowEventHandler))
    {
        *ppvOut = &This->eventhandlerVtbl;
    }
    else if (IsEqualIID(iid, &IID_IOleWindow) || IsEqualIID(iid, &IID_IDeskBarClient))
    {
        *ppvOut = &This->deskbarVtbl;
    }
    else if (IsEqualIID(iid, &IID_IOleCommandTarget))
    {
        *ppvOut = &This->oletargetVtbl;
    }

    if (*ppvOut)
    {
        IUnknown_AddRef(iface);
        return S_OK;
    }

    WARN("unsupported interface: %s\n", debugstr_guid(iid));
    return E_NOINTERFACE;
}

static ULONG WINAPI BandSite_AddRef(IBandSite *iface)
{
    BandSite *This = (BandSite *)iface;
    TRACE("(%p)\n", iface);
    return InterlockedIncrement(&This->refCount);
}

static ULONG WINAPI BandSite_Release(IBandSite *iface)
{
    BandSite *This = (BandSite *)iface;
    ULONG ret;

    TRACE("(%p)\n", iface);

    ret = InterlockedDecrement(&This->refCount);
    if (ret == 0)
        BandSite_Destructor(This);
    return ret;
}

static HRESULT WINAPI BandSite_AddBand(IBandSite *iface, IUnknown *punk)
{
    BandSite *This = (BandSite *)iface;
    INT i;
    LONG NewAllocated;
    struct BandObject *NewBand = NULL;
    IDeskBand *DeskBand = NULL;
    IObjectWithSite *ObjWithSite = NULL;
    HRESULT hRet;

    TRACE("(%p, %p)\n", iface, punk);

    if (punk == NULL)
        return E_FAIL;

    hRet = IUnknown_QueryInterface(punk,
                                   &IID_IDeskBand,
                                   (PVOID*)&DeskBand);
    if (!SUCCEEDED(hRet) || DeskBand == NULL)
        goto Cleanup;
    hRet = IUnknown_QueryInterface(punk,
                                   &IID_IObjectWithSite,
                                   (PVOID*)&ObjWithSite);
    if (!SUCCEEDED(hRet) || ObjWithSite == NULL)
        goto Cleanup;

    hRet = S_OK;
    if (This->BandsAllocated > This->BandsCount)
    {
        /* Search for a free band object */
        for (i = 0; i < This->BandsAllocated; i++)
        {
            if (This->Bands[i].DeskBand == NULL)
            {
                NewBand = &This->Bands[i];
                break;
            }
        }
    }
    else if (This->BandsAllocated > 0)
    {
        ASSERT (This->Bands != NULL);

        /* Reallocate the band object array */
        NewAllocated = This->BandsAllocated + 8;
        if (NewAllocated > 0xFFFF)
            NewAllocated = 0xFFFF;
        if (NewAllocated == This->BandsAllocated)
        {
            hRet = E_OUTOFMEMORY;
            goto Cleanup;
        }
        

        NewBand = CoTaskMemAlloc(NewAllocated * sizeof(struct BandObject));
        if (NewBand == NULL)
        {
            hRet = E_OUTOFMEMORY;
            goto Cleanup;
        }

        /* Copy the old array */
        CopyMemory(NewBand, This->Bands, This->BandsAllocated * sizeof(struct BandObject));

        /* Initialize the added bands */
        ZeroMemory(&NewBand[This->BandsAllocated], (NewAllocated - This->BandsAllocated) * sizeof(struct BandObject));

        NewBand = &This->Bands[This->BandsAllocated];
        This->BandsAllocated = NewAllocated;
        CoTaskMemFree(This->Bands);
        This->Bands = NewBand;
    }
    else
    {
        ASSERT(This->Bands == NULL);
        ASSERT(This->BandsAllocated == 0);
        ASSERT(This->BandsCount == 0);

        /* Allocate new array */
        This->Bands = CoTaskMemAlloc(8 * sizeof(struct BandObject));
        if (This->Bands == NULL)
        {
            hRet = E_OUTOFMEMORY;
            goto Cleanup;
        }

        /* Initialize the added bands */
        ZeroMemory(This->Bands, 8 * sizeof(struct BandObject));

        This->BandsAllocated += 8;
        NewBand = &This->Bands[0];
    }

    if (SUCCEEDED(hRet))
    {
        ASSERT(NewBand != NULL);
        NewBand->DeskBand = DeskBand;

        This->BandsCount++;

        hRet = ObjWithSite->lpVtbl->SetSite(ObjWithSite,
                                            (IUnknown*)iface);
        if (!SUCCEEDED(hRet))
            ERR("IBandSite::AddBand(): Call to IDeskBand::SetSite() failed: %x\n", hRet);

        if (ObjWithSite != NULL)
            ObjWithSite->lpVtbl->Release(ObjWithSite);

        return (HRESULT)((SHORT)(NewBand - This->Bands));
    }
Cleanup:
    if (DeskBand != NULL)
        DeskBand->lpVtbl->Release(DeskBand);
    if (ObjWithSite != NULL)
        ObjWithSite->lpVtbl->Release(ObjWithSite);
    return hRet;
}

static HRESULT WINAPI BandSite_EnumBands(IBandSite *iface, UINT uBand, DWORD *pdwBandID)
{
    BandSite *This = (BandSite *)iface;
    DWORD i;

    TRACE("(%p, %u, %p)\n", iface, uBand, pdwBandID);

    if (uBand == (UINT)-1)
        return (UINT)This->BandsCount;

    if (uBand >= This->BandsCount)
        return E_FAIL;

    for (i = 0; i < This->BandsAllocated; i++)
    {
        if (This->Bands[i].DeskBand != NULL)
        {
            if (uBand == 0)
            {
                *pdwBandID = i;
                return S_OK;
            }

            uBand--;
        }
    }

    return E_FAIL;
}

static HRESULT WINAPI BandSite_QueryBand(IBandSite *iface, DWORD dwBandID, IDeskBand **ppstb, DWORD *pdwState, LPWSTR pszName, int cchName)
{
    BandSite *This = (BandSite *)iface;
    struct BandObject *Band;

    TRACE("(%p, %u, %p, %p, %p, %d)\n", iface, dwBandID, ppstb, pdwState, pszName, cchName);

    Band = GetBandByID(This, dwBandID);
    if (Band == NULL)
        return E_FAIL;

    if (ppstb != NULL)
    {
        Band->DeskBand->lpVtbl->AddRef(Band->DeskBand);
        *ppstb = Band->DeskBand;
    }

    if (pdwState != NULL)
    {
        FIXME("IBandSite::QueryBand() requests band state!\n");
        *pdwState = 0;
    }

    if (pszName != NULL && cchName > 0)
    {
        FIXME("IBandSite::QueryBand() requests band name!\n");
        pszName[0] = 0;
    }
    return S_OK;
}

static HRESULT WINAPI BandSite_SetBandState(IBandSite *iface, DWORD dwBandID, DWORD dwMask, DWORD dwState)
{
    BandSite *This = (BandSite *)iface;
    struct BandObject *Band;

    TRACE("(%p, %u, %x, %x)\n", iface, dwBandID, dwMask, dwState);

    Band = GetBandByID(This, dwBandID);
    if (Band == NULL)
        return E_FAIL;

    FIXME("Stub\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI BandSite_RemoveBand(IBandSite *iface, DWORD dwBandID)
{
    BandSite *This = (BandSite *)iface;
    struct BandObject *Band;

    TRACE("(%p, %u)\n", iface, dwBandID);

    Band = GetBandByID(This, dwBandID);
    if (Band == NULL)
        return E_FAIL;

    FreeBand(This, Band);
    return S_OK;
}

static HRESULT WINAPI BandSite_GetBandObject(IBandSite *iface, DWORD dwBandID, REFIID riid, VOID **ppv)
{
    BandSite *This = (BandSite *)iface;
    struct BandObject *Band;

    TRACE("(%p, %u, %s, %p)\n", iface, dwBandID, debugstr_guid(riid), ppv);

    Band = GetBandByID(This, dwBandID);
    if (Band == NULL)
    {
        *ppv = NULL;
        return E_FAIL;
    }

    return Band->DeskBand->lpVtbl->QueryInterface(Band->DeskBand, riid, ppv);
}

static HRESULT WINAPI BandSite_SetBandSiteInfo(IBandSite *iface, const BANDSITEINFO *pbsinfo)
{
    FIXME("(%p, %p)\n", iface, pbsinfo);
    return E_NOTIMPL;
}

static HRESULT WINAPI BandSite_GetBandSiteInfo(IBandSite *iface, BANDSITEINFO *pbsinfo)
{
    FIXME("(%p, %p)\n", iface, pbsinfo);
    return E_NOTIMPL;
}

static const IBandSiteVtbl BandSiteVtbl =
{
    BandSite_QueryInterface,
    BandSite_AddRef,
    BandSite_Release,

    BandSite_AddBand,
    BandSite_EnumBands,
    BandSite_QueryBand,
    BandSite_SetBandState,
    BandSite_RemoveBand,
    BandSite_GetBandObject,
    BandSite_SetBandSiteInfo,
    BandSite_GetBandSiteInfo,
};

static HRESULT WINAPI BandSite_IWindowEventHandler_QueryInterface(IWindowEventHandler *iface, REFIID iid, LPVOID *ppvOut)
{
    BandSite *This = impl_from_IWindowEventHandler(iface);
    TRACE("(%p, %s, %p)\n", iface, debugstr_guid(iid), ppvOut);
    return BandSite_QueryInterface((IBandSite *)This, iid, ppvOut);
}

static ULONG WINAPI BandSite_IWindowEventHandler_AddRef(IWindowEventHandler *iface)
{
    BandSite *This = impl_from_IWindowEventHandler(iface);
    TRACE("(%p)\n", iface);
    return BandSite_AddRef((IBandSite *)This);
}

static ULONG WINAPI BandSite_IWindowEventHandler_Release(IWindowEventHandler *iface)
{
    BandSite *This = impl_from_IWindowEventHandler(iface);
    TRACE("(%p)\n", iface);
    return BandSite_Release((IBandSite *)This);
}

static HRESULT WINAPI BandSite_ProcessMessage(IWindowEventHandler *iface, HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *plrResult)
{
    FIXME("(%p, %p, %u, %p, %p, %p)\n", iface, hWnd, uMsg, wParam, lParam, plrResult);
    return E_NOTIMPL;
}

static HRESULT WINAPI BandSite_ContainsWindow(IWindowEventHandler *iface, HWND hWnd)
{
    FIXME("(%p, %p)\n", iface, hWnd);
    return E_NOTIMPL;
}

static const IWindowEventHandlerVtbl BandSite_EventHandlerVtbl =
{
    BandSite_IWindowEventHandler_QueryInterface,
    BandSite_IWindowEventHandler_AddRef,
    BandSite_IWindowEventHandler_Release,

    BandSite_ProcessMessage,
    BandSite_ContainsWindow,
};

static HRESULT WINAPI BandSite_IDeskBarClient_QueryInterface(IDeskBarClient *iface, REFIID iid, LPVOID *ppvOut)
{
    BandSite *This = impl_from_IDeskBarClient(iface);
    TRACE("(%p, %s, %p)\n", iface, debugstr_guid(iid), ppvOut);
    return BandSite_QueryInterface((IBandSite *)This, iid, ppvOut);
}

static ULONG WINAPI BandSite_IDeskBarClient_AddRef(IDeskBarClient *iface)
{
    BandSite *This = impl_from_IDeskBarClient(iface);
    TRACE("(%p)\n", iface);
    return BandSite_AddRef((IBandSite *)This);
}

static ULONG WINAPI BandSite_IDeskBarClient_Release(IDeskBarClient *iface)
{
    BandSite *This = impl_from_IDeskBarClient(iface);
    TRACE("(%p)\n", iface);
    return BandSite_Release((IBandSite *)This);
}

static HRESULT WINAPI BandSite_IDeskBarClient_GetWindow(IDeskBarClient *iface, HWND *phWnd)
{
    BandSite *This = impl_from_IDeskBarClient(iface);

    TRACE("(%p, %p)\n", iface, phWnd);

    *phWnd = This->hWndRebar;
    if (This->hWndRebar != NULL)
        return S_OK;

    return E_FAIL;
}

static HRESULT WINAPI BandSite_IDeskBarClient_ContextSensitiveHelp(IDeskBarClient *iface, BOOL fEnterMode)
{
    FIXME("(%p, %d)\n", iface, fEnterMode);
    return E_NOTIMPL;
}

static HRESULT WINAPI BandSite_IDeskBarClient_SetDeskBarSite(IDeskBarClient *iface, IUnknown *pUnk)
{
    BandSite *This = impl_from_IDeskBarClient(iface);
    HWND hWndParent;
    HRESULT hRet;

    TRACE("(%p, %p)\n", iface, pUnk);

    if (This->OleWindow != NULL)
    {
        This->OleWindow->lpVtbl->Release(This->OleWindow);
        This->OleWindow = NULL;
    }

    hRet = IUnknown_QueryInterface(pUnk,
                                   &IID_IOleWindow,
                                   (PVOID*)&This->OleWindow);
    if (!SUCCEEDED(hRet))
    {
        This->OleWindow = NULL;
        return E_FAIL;
    }

    hRet = IOleWindow_GetWindow(This->OleWindow,
                                &hWndParent);
    if (!SUCCEEDED(hRet))
        return E_FAIL;

    This->hWndRebar = CreateWindowExW(WS_EX_TOOLWINDOW,
                                      REBARCLASSNAMEW,
                                      NULL,
                                      WS_CHILD | WS_CLIPSIBLINGS |
                                          WS_CLIPCHILDREN | RBS_VARHEIGHT |
                                          CCS_NODIVIDER | RBS_BANDBORDERS,
                                      0,
                                      0,
                                      0,
                                      0,
                                      hWndParent,
                                      NULL,
                                      browseui_hinstance,
                                      NULL);
    if (This->hWndRebar == NULL)
    {
        IOleWindow_Release(This->OleWindow);
        This->OleWindow = NULL;
        WARN("IDeskbarClient::SetDeskBarSite() failed to create ReBar control!\n");
        return E_FAIL;
    }

    return S_OK;
}

static HRESULT WINAPI BandSite_IDeskBarClient_SetModeDBC(IDeskBarClient *iface, DWORD dwMode)
{
    BandSite *This = impl_from_IDeskBarClient(iface);
    LONG dwStyle, dwPrevStyle;

    TRACE("(%p, %x)\n", iface, dwMode);

    if (This->hWndRebar == NULL)
        return E_FAIL;

    dwStyle = dwPrevStyle = GetWindowLong(This->hWndRebar,
                                          GWL_STYLE);
    if (dwMode & DBIF_VIEWMODE_VERTICAL)
        dwStyle |= CCS_VERT;

    if (dwMode & ~DBIF_VIEWMODE_VERTICAL)
        FIXME("IDeskBarClient::SetModeDBC() unhandled modes: %x\n", dwStyle & ~DBIF_VIEWMODE_VERTICAL);

    if (dwStyle != dwPrevStyle)
    {
        SetWindowLong(This->hWndRebar,
                      GWL_STYLE,
                      dwPrevStyle);
    }
    
    return S_OK;
}

static HRESULT WINAPI BandSite_IDeskBarClient_UIActivateDBC(IDeskBarClient *iface, DWORD dwState)
{
    BandSite *This = impl_from_IDeskBarClient(iface);

    TRACE("(%p, %x)\n", iface, dwState);

    if (This->hWndRebar == NULL)
        return E_FAIL;

    ShowWindow(This->hWndRebar, (dwState & DBC_SHOW) ? SW_SHOW : SW_HIDE);
    FIXME("IDeskBarClient::UIActivateDBC() Properly notify bands?\n");
    return S_OK;
}

static HRESULT WINAPI BandSite_IDeskBarClient_GetSize(IDeskBarClient *iface, DWORD unknown1, LPRECT unknown2)
{
    FIXME("(%p, %x, %p)\n", iface, unknown1, unknown2);
    return E_NOTIMPL;
}

static const IDeskBarClientVtbl BandSite_DeskBarVtbl =
{
    BandSite_IDeskBarClient_QueryInterface,
    BandSite_IDeskBarClient_AddRef,
    BandSite_IDeskBarClient_Release,

    BandSite_IDeskBarClient_GetWindow,
    BandSite_IDeskBarClient_ContextSensitiveHelp,

    BandSite_IDeskBarClient_SetDeskBarSite,
    BandSite_IDeskBarClient_SetModeDBC,
    BandSite_IDeskBarClient_UIActivateDBC,
    BandSite_IDeskBarClient_GetSize,
};

static HRESULT WINAPI BandSite_IOleCommandTarget_QueryInterface(IOleCommandTarget *iface, REFIID iid, LPVOID *ppvOut)
{
    BandSite *This = impl_from_IOleCommandTarget(iface);
    TRACE("(%p, %s, %p)\n", iface, debugstr_guid(iid), ppvOut);
    return BandSite_QueryInterface((IBandSite *)This, iid, ppvOut);
}

static ULONG WINAPI BandSite_IOleCommandTarget_AddRef(IOleCommandTarget *iface)
{
    BandSite *This = impl_from_IOleCommandTarget(iface);
    TRACE("(%p)\n", iface);
    return BandSite_AddRef((IBandSite *)This);
}

static ULONG WINAPI BandSite_IOleCommandTarget_Release(IOleCommandTarget *iface)
{
    BandSite *This = impl_from_IOleCommandTarget(iface);
    TRACE("(%p)\n", iface);
    return BandSite_Release((IBandSite *)This);
}

static HRESULT WINAPI BandSite_IOleCommandTarget_QueryStatus(IOleCommandTarget *iface, const GUID *pguidCmdGroup, DWORD cCmds, OLECMD *prgCmds, OLECMDTEXT *pCmdText)
{
    FIXME("(%p, %p, %u, %p, %p)\n", iface, pguidCmdGroup, cCmds, prgCmds, pCmdText);
    return E_NOTIMPL;
}

static HRESULT WINAPI BandSite_IOleCommandTarget_Exec(IOleCommandTarget *iface, const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdExecOpt, VARIANTARG *pvaIn, VARIANTARG *pvaOut)
{
    BandSite *This = impl_from_IOleCommandTarget(iface);
    HRESULT hRet = S_OK;

    TRACE("(%p, %p, %u, %u, %p, %p)\n", iface, pguidCmdGroup, nCmdID, nCmdExecOpt, pvaIn, pvaOut);

    if (This->hWndRebar == NULL)
        return E_FAIL;

    if (IsEqualIID(pguidCmdGroup, &IID_IDeskBand))
    {
        switch (nCmdID)
        {
            case DBID_BANDINFOCHANGED:
                if (pvaIn == NULL)
                    hRet = UpdateAllBands(This);
                else
                {
                    /* Update a single band */
                    if (pvaIn->n1.n2.vt == VT_I4)
                        hRet = UpdateBand(This, pvaIn->n1.n2.n3.lVal);
                    else
                        hRet = E_FAIL;
                }
                break;

            case DBID_SHOWONLY:
            case DBID_MAXIMIZEBAND:
            case DBID_PUSHCHEVRON:
                FIXME("IOleCommandTarget::Exec(): Unsupported command ID %d\n", nCmdID);
                return E_NOTIMPL;
            default:
                return E_FAIL;
        }
        return S_OK;
    }
    else
        WARN("IOleCommandTarget::Exec(): Unsupported command group GUID\n");

    return E_NOTIMPL;
}

static const IOleCommandTargetVtbl BandSite_OleTargetVtbl =
{
    BandSite_IOleCommandTarget_QueryInterface,
    BandSite_IOleCommandTarget_AddRef,
    BandSite_IOleCommandTarget_Release,

    BandSite_IOleCommandTarget_QueryStatus,
    BandSite_IOleCommandTarget_Exec,
};
