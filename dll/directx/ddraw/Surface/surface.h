// public interfaces
ULONG   WINAPI Main_DDrawSurface_AddRef(LPDIRECTDRAWSURFACE7);
ULONG   WINAPI Main_DDrawSurface_Release(LPDIRECTDRAWSURFACE7);
HRESULT WINAPI Main_DDrawSurface_QueryInterface(LPDIRECTDRAWSURFACE7, REFIID, LPVOID*);
HRESULT WINAPI Main_DDrawSurface_ReleaseDC(LPDIRECTDRAWSURFACE7, HDC);
HRESULT WINAPI Main_DDrawSurface_Blt(LPDIRECTDRAWSURFACE7, LPRECT, LPDIRECTDRAWSURFACE7, LPRECT, DWORD, LPDDBLTFX);
HRESULT WINAPI Main_DDrawSurface_BltBatch(LPDIRECTDRAWSURFACE7, LPDDBLTBATCH, DWORD, DWORD);
HRESULT WINAPI Main_DDrawSurface_BltFast(LPDIRECTDRAWSURFACE7, DWORD, DWORD, LPDIRECTDRAWSURFACE7, LPRECT, DWORD);
HRESULT WINAPI Main_DDrawSurface_DeleteAttachedSurface(LPDIRECTDRAWSURFACE7, DWORD, LPDIRECTDRAWSURFACE7);
HRESULT WINAPI Main_DDrawSurface_EnumAttachedSurfaces(LPDIRECTDRAWSURFACE7, LPVOID, LPDDENUMSURFACESCALLBACK7);
HRESULT WINAPI Main_DDrawSurface_EnumOverlayZOrders(LPDIRECTDRAWSURFACE7, DWORD, LPVOID,LPDDENUMSURFACESCALLBACK7);
HRESULT WINAPI Main_DDrawSurface_Flip(LPDIRECTDRAWSURFACE7 , LPDIRECTDRAWSURFACE7, DWORD);
HRESULT WINAPI Main_DDrawSurface_FreePrivateData(LPDIRECTDRAWSURFACE7, REFGUID);
HRESULT WINAPI Main_DDrawSurface_GetAttachedSurface(LPDIRECTDRAWSURFACE7, LPDDSCAPS2, LPDIRECTDRAWSURFACE7*);
HRESULT WINAPI Main_DDrawSurface_GetBltStatus(LPDIRECTDRAWSURFACE7, DWORD dwFlags);
HRESULT WINAPI Main_DDrawSurface_GetCaps(LPDIRECTDRAWSURFACE7, LPDDSCAPS2 pCaps);
HRESULT WINAPI Main_DDrawSurface_GetClipper(LPDIRECTDRAWSURFACE7, LPDIRECTDRAWCLIPPER*);
HRESULT WINAPI Main_DDrawSurface_GetColorKey(LPDIRECTDRAWSURFACE7, DWORD, LPDDCOLORKEY);
HRESULT WINAPI Main_DDrawSurface_GetDC(LPDIRECTDRAWSURFACE7, HDC *);
HRESULT WINAPI Main_DDrawSurface_GetDDInterface(LPDIRECTDRAWSURFACE7, LPVOID*);
HRESULT WINAPI Main_DDrawSurface_GetFlipStatus(LPDIRECTDRAWSURFACE7, DWORD);
HRESULT WINAPI Main_DDrawSurface_GetLOD(LPDIRECTDRAWSURFACE7, LPDWORD);
HRESULT WINAPI Main_DDrawSurface_GetOverlayPosition(LPDIRECTDRAWSURFACE7, LPLONG, LPLONG);
HRESULT WINAPI Main_DDrawSurface_GetPalette(LPDIRECTDRAWSURFACE7, LPDIRECTDRAWPALETTE*);
HRESULT WINAPI Main_DDrawSurface_GetPixelFormat(LPDIRECTDRAWSURFACE7, LPDDPIXELFORMAT);
HRESULT WINAPI Main_DDrawSurface_GetPriority(LPDIRECTDRAWSURFACE7, LPDWORD);
HRESULT WINAPI Main_DDrawSurface_GetPrivateData(LPDIRECTDRAWSURFACE7, REFGUID, LPVOID, LPDWORD);
HRESULT WINAPI Main_DDrawSurface_GetSurfaceDesc(LPDIRECTDRAWSURFACE7, LPDDSURFACEDESC2);
HRESULT WINAPI Main_DDrawSurface_GetUniquenessValue(LPDIRECTDRAWSURFACE7, LPDWORD);
HRESULT WINAPI Main_DDrawSurface_IsLost(LPDIRECTDRAWSURFACE7);
HRESULT WINAPI Main_DDrawSurface_PageLock(LPDIRECTDRAWSURFACE7, DWORD);
HRESULT WINAPI Main_DDrawSurface_PageUnlock(LPDIRECTDRAWSURFACE7, DWORD);
HRESULT WINAPI Main_DDrawSurface_ReleaseDC(LPDIRECTDRAWSURFACE7, HDC);
HRESULT WINAPI Main_DDrawSurface_SetClipper (LPDIRECTDRAWSURFACE7, LPDIRECTDRAWCLIPPER);
HRESULT WINAPI Main_DDrawSurface_SetColorKey (LPDIRECTDRAWSURFACE7, DWORD, LPDDCOLORKEY);
HRESULT WINAPI Main_DDrawSurface_SetOverlayPosition (LPDIRECTDRAWSURFACE7, LONG, LONG);
HRESULT WINAPI Main_DDrawSurface_SetPalette (LPDIRECTDRAWSURFACE7, LPDIRECTDRAWPALETTE);
HRESULT WINAPI Main_DDrawSurface_SetPriority (LPDIRECTDRAWSURFACE7, DWORD);
HRESULT WINAPI Main_DDrawSurface_SetPrivateData (LPDIRECTDRAWSURFACE7, REFGUID, LPVOID, DWORD, DWORD);
HRESULT WINAPI Main_DDrawSurface_UpdateOverlayDisplay (LPDIRECTDRAWSURFACE7, DWORD);
HRESULT WINAPI Main_DDrawSurface_UpdateOverlayZOrder (LPDIRECTDRAWSURFACE7, DWORD, LPDIRECTDRAWSURFACE7);
HRESULT WINAPI Main_DDrawSurface_SetSurfaceDesc(LPDIRECTDRAWSURFACE7, DDSURFACEDESC2 *, DWORD);
HRESULT WINAPI Main_DDrawSurface_SetLOD(LPDIRECTDRAWSURFACE7, DWORD);
HRESULT WINAPI Main_DDrawSurface_Unlock (LPDIRECTDRAWSURFACE7, LPRECT);
HRESULT WINAPI Main_DDrawSurface_Initialize (LPDIRECTDRAWSURFACE7, LPDIRECTDRAW, LPDDSURFACEDESC2);
HRESULT WINAPI Main_DDrawSurface_Lock (LPDIRECTDRAWSURFACE7, LPRECT, LPDDSURFACEDESC2, DWORD, HANDLE);
HRESULT WINAPI Main_DDrawSurface_Restore(LPDIRECTDRAWSURFACE7);
HRESULT WINAPI Main_DDrawSurface_UpdateOverlay (LPDIRECTDRAWSURFACE7, LPRECT, LPDIRECTDRAWSURFACE7, LPRECT,
												DWORD, LPDDOVERLAYFX);
HRESULT WINAPI Main_DDrawSurface_ChangeUniquenessValue(LPDIRECTDRAWSURFACE7 iface);
HRESULT WINAPI Main_DDrawSurface_AddAttachedSurface(LPDIRECTDRAWSURFACE7 iface, LPDIRECTDRAWSURFACE7 pAttach);
HRESULT WINAPI Main_DDrawSurface_AddOverlayDirtyRect(LPDIRECTDRAWSURFACE7 iface, LPRECT pRect);
HRESULT WINAPI Main_DDrawSurface_GetSurfaceDesc(LPDIRECTDRAWSURFACE7 iface, LPDDSURFACEDESC2 pDDSD);
HRESULT WINAPI Main_DirectDraw_EnumDisplayModes(LPDIRECTDRAW7 iface, DWORD dwFlags, LPDDSURFACEDESC2 pDDSD, LPVOID context, LPDDENUMMODESCALLBACK2 callback);
HRESULT WINAPI Main_DDrawSurface_SetSurfaceDesc(LPDIRECTDRAWSURFACE7 iface, DDSURFACEDESC2 *DDSD, DWORD Flags);
HRESULT WINAPI Main_DirectDraw_GetAvailableVidMem(LPDIRECTDRAW7 iface, LPDDSCAPS2 ddscaps, LPDWORD total, LPDWORD free);

// hel callbacks
DWORD CALLBACK  HelDdDestroyDriver(LPDDHAL_DESTROYDRIVERDATA lpDestroyDriver);
DWORD CALLBACK  HelDdCreateSurface(LPDDHAL_CREATESURFACEDATA lpCreateSurface);
DWORD CALLBACK  HelDdSetColorKey(LPDDHAL_DRVSETCOLORKEYDATA lpSetColorKey);
DWORD CALLBACK  HelDdSetMode(LPDDHAL_SETMODEDATA SetMode);
DWORD CALLBACK  HelDdWaitForVerticalBlank(LPDDHAL_WAITFORVERTICALBLANKDATA lpWaitForVerticalBlank);
DWORD CALLBACK  HelDdCanCreateSurface(LPDDHAL_CANCREATESURFACEDATA lpCanCreateSurface);
DWORD CALLBACK  HelDdCreatePalette(LPDDHAL_CREATEPALETTEDATA lpCreatePalette);
DWORD CALLBACK  HelDdGetScanLine(LPDDHAL_GETSCANLINEDATA lpGetScanLine);
DWORD CALLBACK  HelDdSetExclusiveMode(LPDDHAL_SETEXCLUSIVEMODEDATA lpSetExclusiveMode);
DWORD CALLBACK  HelDdFlipToGDISurface(LPDDHAL_FLIPTOGDISURFACEDATA lpFlipToGDISurface);

// internal functions
HRESULT CreateOverlaySurface(LPDDRAWI_DIRECTDRAW_INT This, LPDDRAWI_DDRAWSURFACE_INT *That, LPDDSURFACEDESC2 pDDSD);
HRESULT CreateBackBufferSurface(LPDDRAWI_DIRECTDRAW_INT This, LPDDRAWI_DDRAWSURFACE_INT *That, LPDDRAWI_DDRAWSURFACE_LCL *lpLcl, LPDDSURFACEDESC2 pDDSD);
HRESULT CreatePrimarySurface(LPDDRAWI_DIRECTDRAW_INT This, LPDDRAWI_DDRAWSURFACE_INT *That,LPDDRAWI_DDRAWSURFACE_LCL *lpLcl, LPDDSURFACEDESC2 pDDSD);
