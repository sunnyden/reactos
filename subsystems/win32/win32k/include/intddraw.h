#pragma once

#include <ddrawint.h>
#include <ddkernel.h>
#include <reactos/drivers/directx/directxint.h>
#include <reactos/drivers/directx/dxg.h>
#include <reactos/drivers/directx/dxeng.h>

/* From ddraw.c */
BOOL intEnableReactXDriver(HDC);
NTSTATUS APIENTRY DxDdStartupDxGraphics(ULONG, PDRVENABLEDATA, ULONG, PDRVENABLEDATA, PULONG, PEPROCESS);
extern DRVFN gpDxFuncs[];

typedef BOOL (APIENTRY* PGD_DDSETGAMMARAMP)(HANDLE, HDC, LPVOID);
typedef BOOL (APIENTRY* PGD_DDRELEASEDC)(HANDLE);
typedef BOOL (APIENTRY* PGD_DDRESTVISRGN)(HANDLE, HWND);
typedef HANDLE (APIENTRY* PGD_DDGETDXHANDLE)(HANDLE, HANDLE, BOOL);
typedef HDC (APIENTRY *PGD_DDGETDC)(HANDLE, PALETTEENTRY *);
typedef DWORD (APIENTRY *PGD_DXDDREENABLEDIRECTDRAWOBJECT)(HANDLE, BOOL*);
typedef DWORD (APIENTRY *PGD_DXDDGETDRIVERINFO)(HANDLE, PDD_GETDRIVERINFODATA);
typedef DWORD (APIENTRY *PGD_DXDDSETEXCLUSIVEMODE)(HANDLE, PDD_SETEXCLUSIVEMODEDATA);
typedef NTSTATUS (APIENTRY *PGD_DXDDSTARTUPDXGRAPHICS) (ULONG, PDRVENABLEDATA, ULONG, PDRVENABLEDATA, PULONG, PEPROCESS);
typedef NTSTATUS (APIENTRY *PGD_DXDDCLEANUPDXGRAPHICS) (VOID);
typedef HANDLE (APIENTRY *PGD_DDCREATEDIRECTDRAWOBJECT) (HDC hdc);
typedef DWORD (APIENTRY *PGD_DDGETDRIVERSTATE)(PDD_GETDRIVERSTATEDATA);
typedef DWORD (APIENTRY *PGD_DDCOLORCONTROL)(HANDLE hSurface,PDD_COLORCONTROLDATA puColorControlData);
typedef HANDLE (APIENTRY *PGD_DXDDCREATESURFACEOBJECT)(HANDLE, HANDLE, PDD_SURFACE_LOCAL, PDD_SURFACE_MORE, PDD_SURFACE_GLOBAL, BOOL);
typedef BOOL (APIENTRY *PGD_DXDDDELETEDIRECTDRAWOBJECT)(HANDLE);
typedef BOOL (APIENTRY *PGD_DXDDDELETESURFACEOBJECT)(HANDLE);
typedef DWORD (APIENTRY *PGD_DXDDFLIPTOGDISURFACE)(HANDLE, PDD_FLIPTOGDISURFACEDATA);
typedef DWORD (APIENTRY *PGD_DXDDGETAVAILDRIVERMEMORY)(HANDLE , PDD_GETAVAILDRIVERMEMORYDATA);
typedef BOOL (APIENTRY *PGD_DXDDQUERYDIRECTDRAWOBJECT)(HANDLE, DD_HALINFO*, DWORD*,  LPD3DNTHAL_CALLBACKS, LPD3DNTHAL_GLOBALDRIVERDATA,
                                                    PDD_D3DBUFCALLBACKS, LPDDSURFACEDESC, DWORD *, VIDEOMEMORY *, DWORD *, DWORD *);


/* From d3d.c */
typedef DWORD (APIENTRY *PGD_DXDDDESTROYD3DBUFFER)(HANDLE);
typedef DWORD (APIENTRY *PGD_DDCANCREATED3DBUFFER)(HANDLE, PDD_CANCREATESURFACEDATA);
typedef DWORD (APIENTRY *PGD_DXDDUNLOCKD3D)(HANDLE, PDD_UNLOCKDATA);
typedef DWORD (APIENTRY *PGD_DXDDLOCKD3D)(HANDLE, PDD_LOCKDATA);
typedef DWORD (APIENTRY *PGD_D3DVALIDATETEXTURESTAGESTATE)(LPD3DNTHAL_VALIDATETEXTURESTAGESTATEDATA);
typedef DWORD (APIENTRY *PGD_D3DDRAWPRIMITIVES2)(HANDLE, HANDLE, LPD3DNTHAL_DRAWPRIMITIVES2DATA, FLATPTR *, DWORD *, FLATPTR *, DWORD *);
typedef DWORD (APIENTRY *PGD_DDCREATED3DBUFFER)(HANDLE, HANDLE *, DDSURFACEDESC *, DD_SURFACE_GLOBAL *, DD_SURFACE_LOCAL *, DD_SURFACE_MORE *, PDD_CREATESURFACEDATA , HANDLE *);
typedef BOOL (APIENTRY *PGD_D3DCONTEXTCREATE)(HANDLE, HANDLE, HANDLE, LPD3DNTHAL_CONTEXTCREATEDATA);
typedef DWORD (APIENTRY *PGD_D3DCONTEXTDESTROY)(LPD3DNTHAL_CONTEXTDESTROYDATA);
typedef DWORD (APIENTRY *PGD_D3DCONTEXTDESTROYALL)(LPD3DNTHAL_CONTEXTDESTROYALLDATA);

/* From dvp.c */
typedef DWORD (APIENTRY* PGD_DVPCANCREATEVIDEOPORT)(HANDLE, PDD_CANCREATEVPORTDATA);
typedef DWORD (APIENTRY* PGD_DVPCOLORCONTROL)(HANDLE, PDD_VPORTCOLORDATA);
typedef HANDLE (APIENTRY* PGD_DVPCREATEVIDEOPORT)(HANDLE, PDD_CREATEVPORTDATA);
typedef DWORD (APIENTRY* PGD_DVPDESTROYVIDEOPORT)(HANDLE, PDD_DESTROYVPORTDATA);
typedef DWORD (APIENTRY* PGD_DVPFLIPVIDEOPORT)(HANDLE,HANDLE,HANDLE,PDD_FLIPVPORTDATA);
typedef DWORD (APIENTRY* PGD_DVPGETVIDEOPORTBANDWITH)(HANDLE, PDD_GETVPORTBANDWIDTHDATA);
typedef DWORD (APIENTRY *PGD_DXDVPGETVIDEOPORTFLIPSTATUS)(HANDLE, PDD_GETVPORTFLIPSTATUSDATA);
typedef DWORD (APIENTRY *PGD_DXDVPGETVIDEOPORTINPUTFORMATS)(HANDLE, PDD_GETVPORTINPUTFORMATDATA);
typedef DWORD (APIENTRY *PGD_DXDVPGETVIDEOPORTLINE)(HANDLE, PDD_GETVPORTLINEDATA);
typedef DWORD (APIENTRY *PGD_DXDVPGETVIDEOPORTOUTPUTFORMATS)(HANDLE, PDD_GETVPORTOUTPUTFORMATDATA);
typedef DWORD (APIENTRY *PGD_DXDVPGETVIDEOPORTCONNECTINFO)(HANDLE, PDD_GETVPORTCONNECTDATA);
typedef DWORD (APIENTRY *PGD_DXDVPGETVIDEOSIGNALSTATUS)(HANDLE, PDD_GETVPORTSIGNALDATA);
typedef DWORD (APIENTRY *PGD_DXDVPUPDATEVIDEOPORT)(HANDLE, HANDLE*, HANDLE*, PDD_UPDATEVPORTDATA);
typedef DWORD (APIENTRY *PGD_DXDVPWAITFORVIDEOPORTSYNC)(HANDLE, PDD_WAITFORVPORTSYNCDATA);
typedef DWORD (APIENTRY *PGD_DXDVPACQUIRENOTIFICATION)(HANDLE, HANDLE*, LPDDVIDEOPORTNOTIFY);
typedef DWORD (APIENTRY *PGD_DXDVPRELEASENOTIFICATION)(HANDLE, HANDLE);
typedef DWORD (APIENTRY *PGD_DXDVPGETVIDEOPORTFIELD)(HANDLE, PDD_GETVPORTFIELDDATA);

/* From mocomp.c */
typedef DWORD (APIENTRY *PGD_DDBEGINMOCOMPFRAME)(HANDLE, PDD_BEGINMOCOMPFRAMEDATA);
typedef HANDLE (APIENTRY *PGD_DXDDCREATEMOCOMP)(HANDLE, PDD_CREATEMOCOMPDATA );
typedef DWORD (APIENTRY *PGD_DXDDDESTROYMOCOMP)(HANDLE, PDD_DESTROYMOCOMPDATA);
typedef DWORD (APIENTRY *PGD_DXDDENDMOCOMPFRAME)(HANDLE, PDD_ENDMOCOMPFRAMEDATA);
typedef DWORD (APIENTRY *PGD_DXDDGETINTERNALMOCOMPINFO)(HANDLE, PDD_GETINTERNALMOCOMPDATA);
typedef DWORD (APIENTRY *PGD_DXDDGETMOCOMPBUFFINFO)(HANDLE, PDD_GETMOCOMPCOMPBUFFDATA);
typedef DWORD (APIENTRY *PGD_DXDDGETMOCOMPGUIDS)(HANDLE, PDD_GETMOCOMPGUIDSDATA);
typedef DWORD (APIENTRY *PGD_DXDDGETMOCOMPFORMATS)(HANDLE, PDD_GETMOCOMPFORMATSDATA);
typedef DWORD (APIENTRY *PGD_DXDDQUERYMOCOMPSTATUS)(HANDLE, PDD_QUERYMOCOMPSTATUSDATA);
typedef DWORD (APIENTRY *PGD_DXDDRENDERMOCOMP)(HANDLE, PDD_RENDERMOCOMPDATA);

/* From dd.c */
typedef DWORD (APIENTRY *PGD_DDCREATESURFACE)(HANDLE, HANDLE *, DDSURFACEDESC *, DD_SURFACE_GLOBAL *, DD_SURFACE_LOCAL *, DD_SURFACE_MORE *, PDD_CREATESURFACEDATA , HANDLE *);
typedef DWORD (APIENTRY *PGD_DXDDWAITFORVERTICALBLANK)(HANDLE, PDD_WAITFORVERTICALBLANKDATA);
typedef DWORD (APIENTRY *PGD_DDCANCREATESURFACE)(HANDLE hDirectDrawLocal, PDD_CANCREATESURFACEDATA puCanCreateSurfaceData);
typedef DWORD (APIENTRY *PGD_DXDDGETSCANLINE)(HANDLE, PDD_GETSCANLINEDATA);
typedef DWORD (APIENTRY *PGD_DXDDCREATESURFACEEX)(HANDLE,HANDLE,DWORD);

/* From ddsurf.c */
typedef DWORD (APIENTRY *PGD_DDALPHABLT)(HANDLE, HANDLE, PDD_BLTDATA);
typedef BOOL (APIENTRY *PGD_DDATTACHSURFACE)(HANDLE, HANDLE);
typedef DWORD (APIENTRY *PGD_DXDDUNATTACHSURFACE)(HANDLE, HANDLE);
typedef DWORD (APIENTRY *PGD_DXDDDESTROYSURFACE)(HANDLE, BOOL);
typedef DWORD (APIENTRY *PGD_DXDDFLIP)(HANDLE, HANDLE, HANDLE, HANDLE, PDD_FLIPDATA);
typedef DWORD (APIENTRY *PGD_DXDDLOCK)(HANDLE, PDD_LOCKDATA, HDC);
typedef DWORD (APIENTRY *PGD_DXDDUNLOCK)(HANDLE, PDD_UNLOCKDATA );
typedef DWORD (APIENTRY *PGD_DDBLT)(HANDLE, HANDLE, PDD_BLTDATA);
typedef DWORD (APIENTRY *PGD_DXDDSETCOLORKEY)(HANDLE, PDD_SETCOLORKEYDATA);
typedef DWORD (APIENTRY *PGD_DDADDATTACHEDSURFACE)(HANDLE, HANDLE,PDD_ADDATTACHEDSURFACEDATA);
typedef DWORD (APIENTRY *PGD_DXDDGETBLTSTATUS)(HANDLE, PDD_GETBLTSTATUSDATA);
typedef DWORD (APIENTRY *PGD_DXDDGETFLIPSTATUS)(HANDLE, PDD_GETFLIPSTATUSDATA);
typedef DWORD (APIENTRY *PGD_DXDDUPDATEOVERLAY)(HANDLE, HANDLE, PDD_UPDATEOVERLAYDATA);
typedef DWORD (APIENTRY *PGD_DXDDSETOVERLAYPOSITION)(HANDLE, HANDLE, PDD_SETOVERLAYPOSITIONDATA);

/* From eng.c */
typedef FLATPTR (APIENTRY *PGD_HEAPVIDMEMALLOCALIGNED)(LPVIDMEM, DWORD, DWORD, LPSURFACEALIGNMENT, LPLONG);
typedef VOID (APIENTRY *PGD_VIDMEMFREE)(LPVMEMHEAP, FLATPTR);
typedef PVOID (APIENTRY *PGD_ENGALLOCPRIVATEUSERMEM)(PDD_SURFACE_LOCAL, SIZE_T, ULONG) ;
typedef VOID (APIENTRY *PGD_ENGFREEPRIVATEUSERMEM)(PDD_SURFACE_LOCAL, PVOID);
typedef PDD_SURFACE_LOCAL (APIENTRY *PGD_ENGLOCKDIRECTDRAWSURFACE)(HANDLE);
typedef BOOL (APIENTRY *PGD_ENGUNLOCKDIRECTDRAWSURFACE)(PDD_SURFACE_LOCAL);

/* Gammaramp internal prototype */
BOOL FASTCALL IntGetDeviceGammaRamp(HDEV hPDev, PGAMMARAMP Ramp);
BOOL FASTCALL IntSetDeviceGammaRamp(HDEV hPDev, PGAMMARAMP Ramp, BOOL);

/* Debug function oly for win32k dx */
void dump_edd_directdraw_global(EDD_DIRECTDRAW_GLOBAL *pEddgbl);
void dump_edd_directdraw_local(PEDD_DIRECTDRAW_LOCAL pEddlcl);
void dump_halinfo(DD_HALINFO *pHalInfo);

#define checkflag(dwflag,dwvalue,text) \
        if (dwflag & dwvalue) \
        { \
            if (count!=0) \
            { \
                DPRINT1("| "); \
            } \
            dwflag = (ULONG)dwflag - (ULONG)dwvalue; \
            DPRINT1("%s ",text); \
            count++; \
        }

#define endcheckflag(dwflag,text) \
    if (count==0) \
        DPRINT1("0x%08lx\n", (ULONG) dwflag);\
    else \
        DPRINT1("\n");\
    if (flag != 0) \
        DPRINT1("undoc value in %s flags value %08lx\n",text, (ULONG) dwflag);
