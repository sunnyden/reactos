/* $Id$
 *
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS
 * FILE:                 lib/ddraw/main/surface.c
 * PURPOSE:              IDirectDrawSurface7 Implementation 
 * PROGRAMMER:           Magnus Olsen, Maarten Bosma
 *
 */

#include "rosdraw.h"


HRESULT WINAPI Main_DDrawSurface_Initialize (LPDIRECTDRAWSURFACE7 iface, LPDIRECTDRAW pDD, LPDDSURFACEDESC2 pDDSD2)
{
    IDirectDrawSurfaceImpl* This = (IDirectDrawSurfaceImpl*)iface;

	if(sizeof(DDSURFACEDESC2) != pDDSD2->dwSize)
		return DDERR_UNSUPPORTED;


	This->owner = (IDirectDrawImpl*)pDD;	

	 /************ fill the discription of our primary surface ***********************/  
	 DDSURFACEDESC ddsd; 	
	 memset (&ddsd, 0, sizeof(DDSURFACEDESC));
	 ddsd.dwSize = sizeof(DDSURFACEDESC);

    /* FIXME Fill the rest from ddsd2 to ddsd */

	 RtlCopyMemory(&ddsd.ddckCKDestBlt,&pDDSD2->ddckCKDestBlt,sizeof(ddsd.ddckCKDestBlt));
	 RtlCopyMemory(&ddsd.ddckCKDestOverlay,&pDDSD2->ddckCKDestOverlay,sizeof(ddsd.ddckCKDestOverlay));
	 RtlCopyMemory(&ddsd.ddckCKSrcBlt,&pDDSD2->ddckCKSrcBlt,sizeof(ddsd.ddckCKSrcBlt));
	 RtlCopyMemory(&ddsd.ddckCKSrcOverlay,&pDDSD2->ddckCKSrcOverlay,sizeof(ddsd.ddckCKSrcOverlay));
	 RtlCopyMemory(&ddsd.ddpfPixelFormat,&pDDSD2->ddpfPixelFormat,sizeof(ddsd.ddpfPixelFormat));
	 RtlCopyMemory(&ddsd.ddsCaps,&pDDSD2->ddsCaps,sizeof(ddsd.ddsCaps));

	 ddsd.dwAlphaBitDepth = pDDSD2->dwAlphaBitDepth;
	 ddsd.dwBackBufferCount = pDDSD2->dwBackBufferCount; 
	 ddsd.dwFlags       = pDDSD2->dwFlags;
	 ddsd.dwHeight      = pDDSD2->dwHeight;
	 /* FIXME ddsd.dwLinearSize  = pDDSD2->dwLinearSize; Problem with our header for dx */
	 ddsd.dwMipMapCount = pDDSD2->dwMipMapCount;
	 ddsd.dwRefreshRate = pDDSD2->dwRefreshRate;
	 ddsd.dwReserved    = pDDSD2->dwReserved;
	 ddsd.dwWidth       = pDDSD2->dwWidth;
	 /* FIXME ddsd.dwZBufferBitDepth where in pDDSD2 */
	 /* FIXME ddsd.lPitch        = pDDSD2->lPitch;  Problem with our header for dx */
	 ddsd.lpSurface     = pDDSD2->lpSurface;



	 
    /************ Test see if we can Create Surface ***********************/
	if (This->owner->DirectDrawGlobal.lpDDCBtmp->HALDD.dwFlags & DDHAL_CB32_CANCREATESURFACE)
	{
		/* can the driver create the surface */
		DDHAL_CANCREATESURFACEDATA CanCreateData;
		memset(&CanCreateData, 0, sizeof(DDHAL_CANCREATESURFACEDATA));
		CanCreateData.lpDD = &This->owner->DirectDrawGlobal; 
		CanCreateData.lpDDSurfaceDesc = (LPDDSURFACEDESC)&ddsd;
			
		if (This->owner->DirectDrawGlobal.lpDDCBtmp->HALDD.CanCreateSurface(&CanCreateData) == DDHAL_DRIVER_NOTHANDLED)
			return DDERR_INVALIDPARAMS;
		
	   if(CanCreateData.ddRVal != DD_OK)
			return CanCreateData.ddRVal;
	}


   /************ Create Surface ***********************/

	/* FIXME we are skipping filling in some data, I do not care for now */

	LPDDRAWI_DIRECTDRAW_GBL pDirectDrawGlobal = &This->owner->DirectDrawGlobal; 	
	ddsd.dwFlags = DDSD_CAPS;
	ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
	 
	/* surface global struct */	
	memset(&This->Global, 0, sizeof(DDRAWI_DDRAWSURFACE_GBL));	
	This->Global.lpDD = &This->owner->DirectDrawGlobal;	
	This->Global.wHeight = This->owner->DirectDrawGlobal.vmiData.dwDisplayHeight;
	This->Global.wWidth = This->owner->DirectDrawGlobal.vmiData.dwDisplayWidth;
	This->Global.dwLinearSize =  This->owner->DirectDrawGlobal.vmiData.lDisplayPitch;
    
	
	/* surface more struct */	
	memset(&This->More, 0, sizeof(DDRAWI_DDRAWSURFACE_MORE));
	This->More.dwSize = sizeof(DDRAWI_DDRAWSURFACE_MORE);
 
	/* surface local struct */
	
	memset(&This->Local, 0, sizeof(DDRAWI_DDRAWSURFACE_LCL));
	This->Local.lpGbl = &This->Global;
	This->Local.lpSurfMore = &This->More;

	/* FIXME do a memcopy */
	This->Local.ddsCaps = *(DDSCAPS*)&ddsd.ddsCaps;
 
	/* for the double pointer below */
	DDRAWI_DDRAWSURFACE_LCL *pLocal[2]; 
	pLocal[0] = &This->Local; 
    pLocal[1] = NULL;  
 
	/* the parameter struct */
	DDHAL_CREATESURFACEDATA CreateData;
	memset(&CreateData, 0, sizeof(DDHAL_CREATESURFACEDATA));
	CreateData.lpDD = pDirectDrawGlobal;
	CreateData.lpDDSurfaceDesc = (LPDDSURFACEDESC) &ddsd; 
	CreateData.dwSCnt = 1;
	CreateData.lplpSList = pLocal;	
	CreateData.ddRVal	= DD_FALSE;
		
		
	/* this is the call we were waiting for */
	if(This->owner->DirectDrawGlobal.lpDDCBtmp->HALDD.CreateSurface(&CreateData) == DDHAL_DRIVER_NOTHANDLED)
		return DDERR_INVALIDPARAMS;

	/* FIXME remove the if and debug string*/
	if(CreateData.ddRVal != DD_OK)
		return CreateData.ddRVal;
	
	OutputDebugString(L"This does hit By Ati Readon but not for nvida :( ");
	OutputDebugString(L"Yet ;)");

   	return DD_OK;
}

ULONG WINAPI Main_DDrawSurface_AddRef(LPDIRECTDRAWSURFACE7 iface)
{
    IDirectDrawSurfaceImpl* This = (IDirectDrawSurfaceImpl*)iface;
	
    return InterlockedIncrement((PLONG)&This->owner->DirectDrawGlobal.dsList->dwIntRefCnt);
}

ULONG WINAPI Main_DDrawSurface_Release(LPDIRECTDRAWSURFACE7 iface)
{
    IDirectDrawSurfaceImpl* This = (IDirectDrawSurfaceImpl*)iface;
    ULONG ref = InterlockedDecrement((PLONG)&This->owner->DirectDrawGlobal.dsList->dwIntRefCnt);
    
    if (ref == 0)
		HeapFree(GetProcessHeap(), 0, This);

    return ref;
}

/**** Stubs ****/

HRESULT WINAPI
Main_DDrawSurface_QueryInterface(LPDIRECTDRAWSURFACE7 iface, REFIID riid,
				      LPVOID* ppObj)
{
	return E_NOINTERFACE;
}

HRESULT WINAPI Main_DDrawSurface_Blt(LPDIRECTDRAWSURFACE7 iface, LPRECT rdst,
			  LPDIRECTDRAWSURFACE7 src, LPRECT rsrc, DWORD dwFlags, LPDDBLTFX lpbltfx)
{
	IDirectDrawImpl* This = (IDirectDrawImpl*)iface;
	
	DX_STUB;

	if (This->DirectDrawGlobal.lpDDCBtmp->HALDD.dwFlags & DDHAL_CB32_FLIPTOGDISURFACE) 
	{
		return Hal_DDrawSurface_Blt( iface,  rdst, src,  rsrc,  dwFlags,  lpbltfx);
	}

	return Hel_DDrawSurface_Blt( iface,  rdst, src,  rsrc,  dwFlags,  lpbltfx);
}


HRESULT WINAPI Main_DDrawSurface_Lock (LPDIRECTDRAWSURFACE7 iface, LPRECT prect,
				LPDDSURFACEDESC2 pDDSD, DWORD flags, HANDLE event)
{
    DX_STUB;
}

HRESULT WINAPI Main_DDrawSurface_Unlock (LPDIRECTDRAWSURFACE7 iface, LPRECT pRect)
{
    DX_STUB;
}

HRESULT WINAPI
Main_DDrawSurface_AddAttachedSurface(LPDIRECTDRAWSURFACE7 iface,
					  LPDIRECTDRAWSURFACE7 pAttach)
{
    DX_STUB;
}

/* MSDN: "not currently implemented." */
HRESULT WINAPI
Main_DDrawSurface_AddOverlayDirtyRect(LPDIRECTDRAWSURFACE7 iface,
					   LPRECT pRect)
{
    DX_STUB;
}

HRESULT WINAPI
Main_DDrawSurface_BltFast(LPDIRECTDRAWSURFACE7 iface, DWORD dstx,
			      DWORD dsty, LPDIRECTDRAWSURFACE7 src,
			      LPRECT rsrc, DWORD trans)
{
    DX_STUB;
}

HRESULT WINAPI
Main_DDrawSurface_Restore(LPDIRECTDRAWSURFACE7 iface)
{
    DX_STUB;
}

/* MSDN: "not currently implemented." */
HRESULT WINAPI
Main_DDrawSurface_BltBatch(LPDIRECTDRAWSURFACE7 iface,
				LPDDBLTBATCH pBatch, DWORD dwCount,
				DWORD dwFlags)
{
    DX_STUB;
}

HRESULT WINAPI
Main_DDrawSurface_ChangeUniquenessValue(LPDIRECTDRAWSURFACE7 iface)
{
    DX_STUB;
}

HRESULT WINAPI
Main_DDrawSurface_DeleteAttachedSurface(LPDIRECTDRAWSURFACE7 iface,
					     DWORD dwFlags,
					     LPDIRECTDRAWSURFACE7 pAttach)
{
    DX_STUB;
}

HRESULT WINAPI
Main_DDrawSurface_EnumAttachedSurfaces(LPDIRECTDRAWSURFACE7 iface,
					    LPVOID context,
					    LPDDENUMSURFACESCALLBACK7 cb)
{
    DX_STUB;
}

HRESULT WINAPI
Main_DDrawSurface_EnumOverlayZOrders(LPDIRECTDRAWSURFACE7 iface,
					  DWORD dwFlags, LPVOID context,
					  LPDDENUMSURFACESCALLBACK7 cb)
{
    DX_STUB;
}

HRESULT WINAPI
Main_DDrawSurface_Flip(LPDIRECTDRAWSURFACE7 iface,
			    LPDIRECTDRAWSURFACE7 override, DWORD dwFlags)
{
    DX_STUB;
}

HRESULT WINAPI
Main_DDrawSurface_FreePrivateData(LPDIRECTDRAWSURFACE7 iface, REFGUID tag)
{
    DX_STUB;
}

HRESULT WINAPI
Main_DDrawSurface_GetAttachedSurface(LPDIRECTDRAWSURFACE7 iface,
					  LPDDSCAPS2 pCaps,
					  LPDIRECTDRAWSURFACE7* ppSurface)
{
    DX_STUB;
}

HRESULT WINAPI
Main_DDrawSurface_GetBltStatus(LPDIRECTDRAWSURFACE7 iface, DWORD dwFlags)
{
    DX_STUB;
}

HRESULT WINAPI
Main_DDrawSurface_GetCaps(LPDIRECTDRAWSURFACE7 iface, LPDDSCAPS2 pCaps)
{
    DX_STUB;
}

HRESULT WINAPI
Main_DDrawSurface_GetClipper(LPDIRECTDRAWSURFACE7 iface,
				  LPDIRECTDRAWCLIPPER* ppClipper)
{
    DX_STUB;
}

HRESULT WINAPI
Main_DDrawSurface_GetColorKey(LPDIRECTDRAWSURFACE7 iface, DWORD dwFlags,
				   LPDDCOLORKEY pCKey)
{
    DX_STUB;
}

HRESULT WINAPI
Main_DDrawSurface_GetDC(LPDIRECTDRAWSURFACE7 iface, HDC *phDC)
{
    DX_STUB;
}

HRESULT WINAPI
Main_DDrawSurface_GetDDInterface(LPDIRECTDRAWSURFACE7 iface, LPVOID* pDD)
{
    DX_STUB;
}
HRESULT WINAPI
Main_DDrawSurface_GetFlipStatus(LPDIRECTDRAWSURFACE7 iface, DWORD dwFlags)
{
    DX_STUB;
}

HRESULT WINAPI
Main_DDrawSurface_GetLOD(LPDIRECTDRAWSURFACE7 iface, LPDWORD pdwMaxLOD)
{
    DX_STUB;
}

HRESULT WINAPI
Main_DDrawSurface_GetOverlayPosition(LPDIRECTDRAWSURFACE7 iface,
					  LPLONG pX, LPLONG pY)
{
    DX_STUB;
}
HRESULT WINAPI
Main_DDrawSurface_GetPalette(LPDIRECTDRAWSURFACE7 iface,
				  LPDIRECTDRAWPALETTE* ppPalette)
{
    DX_STUB;
}

HRESULT WINAPI
Main_DDrawSurface_GetPixelFormat(LPDIRECTDRAWSURFACE7 iface,
				      LPDDPIXELFORMAT pDDPixelFormat)
{
    DX_STUB;
}

HRESULT WINAPI
Main_DDrawSurface_GetPriority(LPDIRECTDRAWSURFACE7 iface,
				   LPDWORD pdwPriority)
{
    DX_STUB;
}

HRESULT WINAPI
Main_DDrawSurface_GetPrivateData(LPDIRECTDRAWSURFACE7 iface,
				      REFGUID tag, LPVOID pBuffer,
				      LPDWORD pcbBufferSize)
{
    DX_STUB;
}

HRESULT WINAPI
Main_DDrawSurface_GetSurfaceDesc(LPDIRECTDRAWSURFACE7 iface,
				      LPDDSURFACEDESC2 pDDSD)
{
    DX_STUB;
}

HRESULT WINAPI
Main_DDrawSurface_GetUniquenessValue(LPDIRECTDRAWSURFACE7 iface,
					  LPDWORD pValue)
{
    DX_STUB;
}

HRESULT WINAPI
Main_DDrawSurface_IsLost(LPDIRECTDRAWSURFACE7 iface)
{
    DX_STUB;
}

HRESULT WINAPI
Main_DDrawSurface_PageLock(LPDIRECTDRAWSURFACE7 iface, DWORD dwFlags)
{
    DX_STUB;
}

HRESULT WINAPI
Main_DDrawSurface_PageUnlock(LPDIRECTDRAWSURFACE7 iface, DWORD dwFlags)
{
    DX_STUB;
}

HRESULT WINAPI
Main_DDrawSurface_ReleaseDC(LPDIRECTDRAWSURFACE7 iface, HDC hDC)
{
    DX_STUB;
}

HRESULT WINAPI
Main_DDrawSurface_SetClipper (LPDIRECTDRAWSURFACE7 iface,
				  LPDIRECTDRAWCLIPPER pDDClipper)
{
    DX_STUB;
}

HRESULT WINAPI
Main_DDrawSurface_SetColorKey (LPDIRECTDRAWSURFACE7 iface,
				   DWORD dwFlags, LPDDCOLORKEY pCKey)
{
    DX_STUB;
}

HRESULT WINAPI
Main_DDrawSurface_SetLOD (LPDIRECTDRAWSURFACE7 iface, DWORD dwMaxLOD)
{
    DX_STUB;
}

HRESULT WINAPI
Main_DDrawSurface_SetOverlayPosition (LPDIRECTDRAWSURFACE7 iface,
					  LONG X, LONG Y)
{
    DX_STUB;
}

HRESULT WINAPI
Main_DDrawSurface_SetPalette (LPDIRECTDRAWSURFACE7 iface,
				  LPDIRECTDRAWPALETTE pPalette)
{
    DX_STUB;
}

HRESULT WINAPI
Main_DDrawSurface_SetPriority (LPDIRECTDRAWSURFACE7 iface,
				   DWORD dwPriority)
{
    DX_STUB;
}

HRESULT WINAPI
Main_DDrawSurface_SetPrivateData (LPDIRECTDRAWSURFACE7 iface,
				      REFGUID tag, LPVOID pData,
				      DWORD cbSize, DWORD dwFlags)
{
    DX_STUB;
}

HRESULT WINAPI
Main_DDrawSurface_UpdateOverlay (LPDIRECTDRAWSURFACE7 iface,
				     LPRECT pSrcRect,
				     LPDIRECTDRAWSURFACE7 pDstSurface,
				     LPRECT pDstRect, DWORD dwFlags,
				     LPDDOVERLAYFX pFX)
{
    DX_STUB;
}

/* MSDN: "not currently implemented." */
HRESULT WINAPI
Main_DDrawSurface_UpdateOverlayDisplay (LPDIRECTDRAWSURFACE7 iface,
					    DWORD dwFlags)
{
    DX_STUB;
}

HRESULT WINAPI Main_DDrawSurface_UpdateOverlayZOrder (LPDIRECTDRAWSURFACE7 iface,
					   DWORD dwFlags, LPDIRECTDRAWSURFACE7 pDDSRef)
{
    DX_STUB;
}

IDirectDrawSurface7Vtbl DirectDrawSurface7_Vtable =
{
    Main_DDrawSurface_QueryInterface,
    Main_DDrawSurface_AddRef,
    Main_DDrawSurface_Release,
    Main_DDrawSurface_AddAttachedSurface,
    Main_DDrawSurface_AddOverlayDirtyRect,
    Main_DDrawSurface_Blt,
    Main_DDrawSurface_BltBatch,
    Main_DDrawSurface_BltFast,
    Main_DDrawSurface_DeleteAttachedSurface,
    Main_DDrawSurface_EnumAttachedSurfaces,
    Main_DDrawSurface_EnumOverlayZOrders,
    Main_DDrawSurface_Flip,
    Main_DDrawSurface_GetAttachedSurface,
    Main_DDrawSurface_GetBltStatus,
    Main_DDrawSurface_GetCaps,
    Main_DDrawSurface_GetClipper,
    Main_DDrawSurface_GetColorKey,
    Main_DDrawSurface_GetDC,
    Main_DDrawSurface_GetFlipStatus,
    Main_DDrawSurface_GetOverlayPosition,
    Main_DDrawSurface_GetPalette,
    Main_DDrawSurface_GetPixelFormat,
    Main_DDrawSurface_GetSurfaceDesc,
    Main_DDrawSurface_Initialize,
    Main_DDrawSurface_IsLost,
    Main_DDrawSurface_Lock,
    Main_DDrawSurface_ReleaseDC,
    Main_DDrawSurface_Restore,
    Main_DDrawSurface_SetClipper,
    Main_DDrawSurface_SetColorKey,
    Main_DDrawSurface_SetOverlayPosition,
    Main_DDrawSurface_SetPalette,
    Main_DDrawSurface_Unlock,
    Main_DDrawSurface_UpdateOverlay,
    Main_DDrawSurface_UpdateOverlayDisplay,
    Main_DDrawSurface_UpdateOverlayZOrder
};
