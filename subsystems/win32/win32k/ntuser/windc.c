/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            subsystems/win32/win32k/ntuser/windc.c
 * PURPOSE:         Keyboard layout management
 * COPYRIGHT:       Copyright 2007 ReactOS
 *
 */

/* INCLUDES ******************************************************************/

#include <w32k.h>

#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

/* NOTE - I think we should store this per window station (including gdi objects) */

static PDCE FirstDce = NULL;
static PDC defaultDCstate = NULL;
//static INT DCECount = 0; // Count of DCE in system.

#define DCX_CACHECOMPAREMASK (DCX_CLIPSIBLINGS | DCX_CLIPCHILDREN | \
                              DCX_CACHE | DCX_WINDOW | DCX_PARENTCLIP)

/* FUNCTIONS *****************************************************************/

//
// This should be moved to dc.c or dcutil.c.
//
HDC FASTCALL
DceCreateDisplayDC(VOID)
{
  HDC hDC;
  UNICODE_STRING DriverName;
  RtlInitUnicodeString(&DriverName, L"DISPLAY");
  hDC = IntGdiCreateDC(&DriverName, NULL, NULL, NULL, FALSE);
//
// If NULL, first time through! Build the default window dc!
//
  if (hDC && !defaultDCstate) // Ultra HAX! Dedicated to GvG!
  { // This is a cheesy way to do this.
      PDC dc = DC_LockDc ( hDC );
      defaultDCstate = ExAllocatePoolWithTag(PagedPool, sizeof(DC), TAG_DC);
      RtlZeroMemory(defaultDCstate, sizeof(DC));
      IntGdiCopyToSaveState(dc, defaultDCstate);
      DC_UnlockDc( dc );
  }
  return hDC;
}

static
HRGN FASTCALL
DceGetVisRgn(PWINDOW_OBJECT Window, ULONG Flags, HWND hWndChild, ULONG CFlags)
{
  HRGN VisRgn;

  VisRgn = VIS_ComputeVisibleRegion( Window,
                  0 == (Flags & DCX_WINDOW),
            0 != (Flags & DCX_CLIPCHILDREN),
            0 != (Flags & DCX_CLIPSIBLINGS));

  if (VisRgn == NULL)
      VisRgn = NtGdiCreateRectRgn(0, 0, 0, 0);

  return VisRgn;
}


PDCE FASTCALL
DceAllocDCE(PWINDOW_OBJECT Window OPTIONAL, DCE_TYPE Type)
{
  PDCE pDce;
  PWINDOW Wnd = NULL;
  PVOID Class = NULL;

  if (Window) Wnd = Window->Wnd;

  if (Wnd) Class = Wnd->Class;

  if (Type == DCE_CLASS_DC)
  {
     PDCE pdce;
     KeEnterCriticalRegion();
     pdce = FirstDce;
     do
     {
        if (pdce->Class == Class)
        {
           pdce->Count++;
           KeLeaveCriticalRegion();
           return pdce;
        }
        pdce = (PDCE)pdce->List.Flink;
     } while (pdce != FirstDce);
     KeLeaveCriticalRegion();
  }

  pDce = ExAllocatePoolWithTag(PagedPool, sizeof(DCE), TAG_PDCE);
  if(!pDce)
        return NULL;

  pDce->hDC = DceCreateDisplayDC();
  if (!pDce->hDC)
  {
      ExFreePoolWithTag(pDce, TAG_PDCE);
      return NULL;
  }

  pDce->hwndCurrent = (Window ? Window->hSelf : NULL);
  pDce->hClipRgn = NULL;
  pDce->pProcess = NULL;
  pDce->Class = Class;
  pDce->Count = 1;

  KeEnterCriticalRegion();
  if (FirstDce == NULL)
  {
     FirstDce = pDce;
     InitializeListHead(&FirstDce->List);
  }
  else
     InsertTailList(&FirstDce->List, &pDce->List);
  KeLeaveCriticalRegion();

  DCU_SetDcUndeletable(pDce->hDC);

  if (Type == DCE_WINDOW_DC || Type == DCE_CLASS_DC) //Window DCE have ownership.
  { // Process should already own it.
    pDce->pProcess = PsGetCurrentProcess();
  }
  else
  {
     PDC dc = DC_LockDc ( pDce->hDC );
     DPRINT("FREE DCATTR!!!! NOT DCE_WINDOW_DC!!!!! hDC-> %x\n", pDce->hDC);
     MmCopyFromCaller(&dc->Dc_Attr, dc->pDc_Attr, sizeof(DC_ATTR));
     DC_UnlockDc( dc );
     DC_FreeDcAttr(pDce->hDC);         // Free the dcattr!
     DC_SetOwnership(pDce->hDC, NULL); // This hDC is inaccessible!
  }

  if (Type == DCE_CACHE_DC)
  {
     pDce->DCXFlags = DCX_CACHE | DCX_DCEEMPTY;
  }
  else
  {
     pDce->DCXFlags = DCX_DCEBUSY;
     if (Wnd)
     {
        if (Type == DCE_WINDOW_DC)
        {
          if (Wnd->Style & WS_CLIPCHILDREN) pDce->DCXFlags |= DCX_CLIPCHILDREN;
          if (Wnd->Style & WS_CLIPSIBLINGS) pDce->DCXFlags |= DCX_CLIPSIBLINGS;
        }
     }
  }
  return(pDce);
}

static VOID STDCALL
DceSetDrawable(PWINDOW_OBJECT Window OPTIONAL, HDC hDC, ULONG Flags,
               BOOL SetClipOrigin)
{
  PWINDOW Wnd;
  DC *dc = DC_LockDc(hDC);
  if(!dc)
      return;

  if (Window == NULL)
  {
      dc->w.DCOrgX = 0;
      dc->w.DCOrgY = 0;
  }
  else
  {
      Wnd = Window->Wnd;
      if (Flags & DCX_WINDOW)
      {
         dc->w.DCOrgX = Wnd->WindowRect.left;
         dc->w.DCOrgY = Wnd->WindowRect.top;
      }
      else
      {
         dc->w.DCOrgX = Wnd->ClientRect.left;
         dc->w.DCOrgY = Wnd->ClientRect.top;
      }
  }
  DC_UnlockDc(dc);
}


static VOID FASTCALL
DceDeleteClipRgn(DCE* Dce)
{
   Dce->DCXFlags &= ~(DCX_EXCLUDERGN | DCX_INTERSECTRGN);

   if (Dce->DCXFlags & DCX_KEEPCLIPRGN )
   {
      Dce->DCXFlags &= ~DCX_KEEPCLIPRGN;
   }
   else if (Dce->hClipRgn != NULL)
   {
      NtGdiDeleteObject(Dce->hClipRgn);
   }

   Dce->hClipRgn = NULL;

   /* make it dirty so that the vis rgn gets recomputed next time */
   Dce->DCXFlags |= DCX_DCEDIRTY;
}

static INT FASTCALL
DceReleaseDC(DCE* dce, BOOL EndPaint)
{
   if (DCX_DCEBUSY != (dce->DCXFlags & (DCX_DCEEMPTY | DCX_DCEBUSY)))
   {
      return 0;
   }

   /* restore previous visible region */

   if ((dce->DCXFlags & (DCX_INTERSECTRGN | DCX_EXCLUDERGN)) &&
         ((dce->DCXFlags & DCX_CACHE) || EndPaint))
   {
      DceDeleteClipRgn(dce);
   }

   if (dce->DCXFlags & DCX_CACHE)
   {
     if (!(dce->DCXFlags & DCX_NORESETATTRS))
     {
       PDC dc;
       /* make the DC clean so that SetDCState doesn't try to update the vis rgn */
       IntGdiSetHookFlags(dce->hDC, DCHF_VALIDATEVISRGN);

       dc = DC_LockDc ( dce->hDC );
       // Clean the DC
       IntGdiCopyFromSaveState(dc, defaultDCstate, dce->hDC ); // Was SetDCState.

       dce->DCXFlags &= ~DCX_DCEBUSY;
       if (dce->DCXFlags & DCX_DCEDIRTY)
       {
         /* don't keep around invalidated entries
          * because SetDCState() disables hVisRgn updates
          * by removing dirty bit. */
         dce->hwndCurrent = 0;
         dce->DCXFlags &= DCX_CACHE;
         dce->DCXFlags |= DCX_DCEEMPTY;
       }
     }
     else
     { // Save Users Dc_Attr.
       PDC dc = DC_LockDc(dce->hDC);
       if(dc)
       {
         PDC_ATTR Dc_Attr = dc->pDc_Attr;
         if(Dc_Attr) MmCopyFromCaller(&dc->Dc_Attr, Dc_Attr, sizeof(DC_ATTR));
         DC_UnlockDc(dc);
       }
     }
     DPRINT("Exit!!!!! DCX_CACHE!!!!!!   hDC-> %x \n", dce->hDC);
     DC_FreeDcAttr(dce->hDC);         // Free the dcattr.
     DC_SetOwnership(dce->hDC, NULL); // Set hDC inaccessible mode.
     dce->pProcess = NULL;            // Reset ownership.
   }
   return 1;
}

static VOID FASTCALL
DceUpdateVisRgn(DCE *Dce, PWINDOW_OBJECT Window, ULONG Flags)
{
   HANDLE hRgnVisible = NULL;
   ULONG DcxFlags;
   PWINDOW_OBJECT DesktopWindow;

   if (Flags & DCX_PARENTCLIP)
   {
      PWINDOW_OBJECT Parent;
      PWINDOW ParentWnd;

      Parent = Window->Parent;
      if(!Parent)
      {
         hRgnVisible = NULL;
         goto noparent;
      }

      ParentWnd = Parent->Wnd;

      if (ParentWnd->Style & WS_CLIPSIBLINGS)
      {
         DcxFlags = DCX_CLIPSIBLINGS |
                    (Flags & ~(DCX_CLIPCHILDREN | DCX_WINDOW));
      }
      else
      {
         DcxFlags = Flags & ~(DCX_CLIPSIBLINGS | DCX_CLIPCHILDREN | DCX_WINDOW);
      }
      hRgnVisible = DceGetVisRgn(Parent, DcxFlags, Window->hSelf, Flags);
   }
   else if (Window == NULL)
   {
      DesktopWindow = UserGetWindowObject(IntGetDesktopWindow());
      if (NULL != DesktopWindow)
      {
         hRgnVisible = UnsafeIntCreateRectRgnIndirect(&DesktopWindow->Wnd->WindowRect);
      }
      else
      {
         hRgnVisible = NULL;
      }
   }
   else
   {
      hRgnVisible = DceGetVisRgn(Window, Flags, 0, 0);
   }

noparent:
   if (Flags & DCX_INTERSECTRGN)
   {
      if(Dce->hClipRgn != NULL)
      {
         NtGdiCombineRgn(hRgnVisible, hRgnVisible, Dce->hClipRgn, RGN_AND);
      }
      else
      {
         if(hRgnVisible != NULL)
         {
            NtGdiDeleteObject(hRgnVisible);
         }
         hRgnVisible = NtGdiCreateRectRgn(0, 0, 0, 0);
      }
   }

   if (Flags & DCX_EXCLUDERGN && Dce->hClipRgn != NULL)
   {
      NtGdiCombineRgn(hRgnVisible, hRgnVisible, Dce->hClipRgn, RGN_DIFF);
   }

   Dce->DCXFlags &= ~DCX_DCEDIRTY;
   IntGdiSelectVisRgn(Dce->hDC, hRgnVisible);

   if (Window != NULL)
   {
      IntEngWindowChanged(Window, WOC_RGN_CLIENT);
   }

   if (hRgnVisible != NULL)
   {
      NtGdiDeleteObject(hRgnVisible);
   }
}

HDC FASTCALL
UserGetDCEx(PWINDOW_OBJECT Window OPTIONAL, HANDLE ClipRegion, ULONG Flags)
{
   PWINDOW_OBJECT Parent;
   ULONG DcxFlags;
   DCE* Dce;
   BOOL UpdateVisRgn = TRUE;
   BOOL UpdateClipOrigin = FALSE;
   PWINDOW Wnd = NULL;

   if (NULL == Window)
   {  // Do the same as GetDC with a NULL.
//      Window = UserGetWindowObject(IntGetDesktopWindow());
//      if (Window) Wnd = Window->Wnd;
//      else
         Flags &= ~DCX_USESTYLE;
   }
   else
       Wnd = Window->Wnd;

   if (NULL == Window || NULL == Window->Dce)
   {
      Flags |= DCX_CACHE;
   }

   if (Flags & DCX_USESTYLE)
   {
      Flags &= ~(DCX_CLIPCHILDREN | DCX_CLIPSIBLINGS | DCX_PARENTCLIP);

      if (Wnd->Style & WS_CLIPSIBLINGS)
      {
         Flags |= DCX_CLIPSIBLINGS;
      }

      if (!(Flags & DCX_WINDOW))
      {
         if (Wnd->Class->Style & CS_PARENTDC)
         {
            Flags |= DCX_PARENTCLIP;
         }

         if (Wnd->Style & WS_CLIPCHILDREN &&
             !(Wnd->Style & WS_MINIMIZE))
         {
            Flags |= DCX_CLIPCHILDREN;
         }
      }
      else
      {
         Flags |= DCX_CACHE;
      }
   }

   if (Flags & DCX_NOCLIPCHILDREN)
   {
      Flags |= DCX_CACHE;
      Flags &= ~(DCX_PARENTCLIP | DCX_CLIPCHILDREN);
   }

   if (Flags & DCX_WINDOW)
   {
      Flags = (Flags & ~DCX_CLIPCHILDREN) | DCX_CACHE;
   }

   Parent = (Window ? Window->Parent : NULL);

   if (NULL == Window || !(Wnd->Style & WS_CHILD) || NULL == Parent)
   {
      Flags &= ~DCX_PARENTCLIP;
   }
   else if (Flags & DCX_PARENTCLIP)
   {
      Flags |= DCX_CACHE;
      if ((Wnd->Style & WS_VISIBLE) &&
          (Parent->Wnd->Style & WS_VISIBLE))
      {
         Flags &= ~DCX_CLIPCHILDREN;
         if (Parent->Wnd->Style & WS_CLIPSIBLINGS)
         {
            Flags |= DCX_CLIPSIBLINGS;
         }
      }
   }

   DcxFlags = Flags & DCX_CACHECOMPAREMASK;

   if (Flags & DCX_CACHE)
   {
      DCE* DceEmpty = NULL;
      DCE* DceUnused = NULL;
      KeEnterCriticalRegion();
      Dce = FirstDce;
      do
      {
// The reason for this you may ask?
// Well, it seems ReactOS calls GetDC with out first creating a desktop DC window!
// Need to test for null here. Not sure if this is a bug or a feature.
         if (!Dce) break;

         if ((Dce->DCXFlags & (DCX_CACHE | DCX_DCEBUSY)) == DCX_CACHE)
         {
            DceUnused = Dce;
            if (Dce->DCXFlags & DCX_DCEEMPTY)
            {
               DceEmpty = Dce;
            }
            else if (Dce->hwndCurrent == (Window ? Window->hSelf : NULL) &&
                     ((Dce->DCXFlags & DCX_CACHECOMPAREMASK) == DcxFlags))
            {
#if 0 /* FIXME */
               UpdateVisRgn = FALSE;
#endif

               UpdateClipOrigin = TRUE;
               break;
            }
         }
        Dce = (PDCE)Dce->List.Flink;
      } while (Dce != FirstDce);
      KeLeaveCriticalRegion();

      Dce = (DceEmpty == NULL) ? DceUnused : DceEmpty;

      if (Dce == NULL)
      {
         Dce = DceAllocDCE(NULL, DCE_CACHE_DC);
      }
   }
   else
   {
      Dce = Window->Dce;
      if (Dce->hwndCurrent == Window->hSelf)
      {
         UpdateVisRgn = FALSE; /* updated automatically, via DCHook() */
      }
      else
      {
      /* we should free dce->clip_rgn here, but Windows apparently doesn't */
         Dce->DCXFlags &= ~(DCX_EXCLUDERGN | DCX_INTERSECTRGN);
         Dce->hClipRgn = NULL;
      }
#if 1 /* FIXME */
      UpdateVisRgn = TRUE;
#endif

   }

   if (NULL == Dce)
   {
      return(NULL);
   }

   if (!GDIOBJ_ValidateHandle(Dce->hDC, GDI_OBJECT_TYPE_DC))
   {
      DPRINT1("FIXME: Got DCE with invalid hDC!\n");
      Dce->hDC = DceCreateDisplayDC();
      /* FIXME: Handle error */
   }

   Dce->hwndCurrent = (Window ? Window->hSelf : NULL);
   Dce->DCXFlags = Flags | DCX_DCEBUSY;

   if (0 == (Flags & (DCX_EXCLUDERGN | DCX_INTERSECTRGN)) && NULL != ClipRegion)
   {
      if (!(Flags & DCX_KEEPCLIPRGN))
         NtGdiDeleteObject(ClipRegion);
      ClipRegion = NULL;
   }

#if 0
   if (NULL != Dce->hClipRgn)
   {
      DceDeleteClipRgn(Dce);
   }
#endif

   if (0 != (Flags & DCX_INTERSECTUPDATE) && NULL == ClipRegion)
   {
      Flags |= DCX_INTERSECTRGN | DCX_KEEPCLIPRGN;
      Dce->DCXFlags |= DCX_INTERSECTRGN | DCX_KEEPCLIPRGN;
      ClipRegion = Window->UpdateRegion;
   }

   if (ClipRegion == (HRGN) 1)
   {
      if (!(Flags & DCX_WINDOW))
      {
         Dce->hClipRgn = UnsafeIntCreateRectRgnIndirect(&Window->Wnd->ClientRect);
      }
      else
      {
         Dce->hClipRgn = UnsafeIntCreateRectRgnIndirect(&Window->Wnd->WindowRect);
      }
      Dce->DCXFlags &= ~DCX_KEEPCLIPRGN;
   }
   else if (ClipRegion != NULL)
   {
      Dce->hClipRgn = ClipRegion;
   }

   DceSetDrawable(Window, Dce->hDC, Flags, UpdateClipOrigin);

   //  if (UpdateVisRgn)
   {
      DceUpdateVisRgn(Dce, Window, Flags);
   }

   if (Dce->DCXFlags & DCX_CACHE)
   {
      DPRINT("ENTER!!!!!! DCX_CACHE!!!!!!   hDC-> %x\n", Dce->hDC);
      // Need to set ownership so Sync dcattr will work.
      DC_SetOwnership( Dce->hDC, PsGetCurrentProcess());
      DC_AllocateDcAttr( Dce->hDC );         // Allocate new dcattr
      DCU_SynchDcAttrtoUser( Dce->hDC);      // Copy data from dc to dcattr
      Dce->pProcess = PsGetCurrentProcess(); // Set the temp owning process
   }
   return(Dce->hDC);
}

/***********************************************************************
 *           DceFreeDCE
 */
PDCE FASTCALL
DceFreeDCE(PDCE pdce, BOOLEAN Force)
{
  DCE *ret;
  BOOL Hit = FALSE;

  if (NULL == pdce) return NULL;

  ret = (PDCE) pdce->List.Flink;

#if 0 /* FIXME */
  SetDCHook(pdce->hDC, NULL, 0L);
#endif

  if (Force && !GDIOBJ_OwnedByCurrentProcess(pdce->hDC))
  {
     DPRINT1("Change ownership for DCE!\n");

     if (!IsObjectDead((HGDIOBJ) pdce->hDC))
         DC_SetOwnership( pdce->hDC, PsGetCurrentProcess());
     else
     {
         DPRINT1("Attempted to change ownership of an DCEhDC 0x%x currently being destroyed!!!\n",pdce->hDC);
         Hit = TRUE;
     }
  }

  if (!Hit) IntGdiDeleteDC(pdce->hDC, TRUE);

  if (pdce->hClipRgn && ! (pdce->DCXFlags & DCX_KEEPCLIPRGN))
  {
      NtGdiDeleteObject(pdce->hClipRgn);
  }

  RemoveEntryList(&pdce->List);
  ExFreePoolWithTag(pdce, TAG_PDCE);

  return ret;
}

/***********************************************************************
 *           DceFreeWindowDCE
 *
 * Remove owned DCE and reset unreleased cache DCEs.
 */
void FASTCALL
DceFreeWindowDCE(PWINDOW_OBJECT Window)
{
  PDCE pDCE;

  pDCE = FirstDce;
  KeEnterCriticalRegion();
  do
  {
      if (pDCE->hwndCurrent == Window->hSelf)
      {
         if (!(pDCE->DCXFlags & DCX_CACHE)) /* owned or Class DCE*/
         {
            if (Window->Wnd->Class->Style & CS_CLASSDC ||
                Window->Wnd->Style & CS_CLASSDC) /* Test Class first */
            {
               if (pDCE->DCXFlags & (DCX_INTERSECTRGN | DCX_EXCLUDERGN)) /* Class DCE*/
                   DceDeleteClipRgn(pDCE);
               // Update and reset Vis Rgn and clear the dirty bit.
               // Should release VisRgn than reset it to default.
               DceUpdateVisRgn(pDCE, Window, pDCE->DCXFlags);
               pDCE->DCXFlags = DCX_DCEEMPTY;
               pDCE->hwndCurrent = 0;
            }
            else if (Window->Wnd->Class->Style & CS_OWNDC ||
                     Window->Wnd->Style & CS_OWNDC) /* owned DCE*/
            {
               pDCE = DceFreeDCE(pDCE, FALSE);
               Window->Dce = NULL;
               continue;
            }
            else
            {
               ASSERT(FALSE);
            }
         }
         else
         {
            if (pDCE->DCXFlags & DCX_DCEBUSY) /* shared cache DCE */
            {
               /* FIXME: AFAICS we are doing the right thing here so
                * this should be a DPRINT. But this is best left as an ERR
                * because the 'application error' is likely to come from
                * another part of Wine (i.e. it's our fault after all).
                * We should change this to DPRINT when ReactOS is more stable
                * (for 1.0?).
                */
               DPRINT1("[%p] GetDC() without ReleaseDC()!\n", Window->hSelf);
               DceReleaseDC(pDCE, FALSE);
            }
            pDCE->DCXFlags |= DCX_DCEEMPTY;
            pDCE->hwndCurrent = 0;
         }
      }
      pDCE = (PDCE) pDCE->List.Flink;
  } while (pDCE != FirstDce);
  KeLeaveCriticalRegion();
}

VOID FASTCALL
DceEmptyCache()
{
   while (FirstDce != NULL)
   {
      FirstDce = DceFreeDCE(FirstDce, TRUE);
   }
}

VOID FASTCALL
DceResetActiveDCEs(PWINDOW_OBJECT Window)
{
   DCE *pDCE;
   PDC dc;
   PWINDOW_OBJECT CurrentWindow;
   INT DeltaX;
   INT DeltaY;

   if (NULL == Window)
   {
      return;
   }
   pDCE = FirstDce;
   if(!pDCE) return; // Another null test!
   do
   {
      if (0 == (pDCE->DCXFlags & DCX_DCEEMPTY))
      {
         if (Window->hSelf == pDCE->hwndCurrent)
         {
            CurrentWindow = Window;
         }
         else
         {
            CurrentWindow = UserGetWindowObject(pDCE->hwndCurrent);
            if (NULL == CurrentWindow)
            {
               pDCE = (PDCE) pDCE->List.Flink;
               continue;
            }
         }

         if (!GDIOBJ_ValidateHandle(pDCE->hDC, GDI_OBJECT_TYPE_DC) ||
             (dc = DC_LockDc(pDCE->hDC)) == NULL)
         {
            pDCE = (PDCE) pDCE->List.Flink;
            continue;
         }
         if (Window == CurrentWindow || IntIsChildWindow(Window, CurrentWindow))
         {
            if (pDCE->DCXFlags & DCX_WINDOW)
            {
               DeltaX = CurrentWindow->Wnd->WindowRect.left - dc->w.DCOrgX;
               DeltaY = CurrentWindow->Wnd->WindowRect.top - dc->w.DCOrgY;
               dc->w.DCOrgX = CurrentWindow->Wnd->WindowRect.left;
               dc->w.DCOrgY = CurrentWindow->Wnd->WindowRect.top;
            }
            else
            {
               DeltaX = CurrentWindow->Wnd->ClientRect.left - dc->w.DCOrgX;
               DeltaY = CurrentWindow->Wnd->ClientRect.top - dc->w.DCOrgY;
               dc->w.DCOrgX = CurrentWindow->Wnd->ClientRect.left;
               dc->w.DCOrgY = CurrentWindow->Wnd->ClientRect.top;
            }
            if (NULL != dc->w.hClipRgn)
            {
               int FASTCALL CLIPPING_UpdateGCRegion(DC* Dc);
               NtGdiOffsetRgn(dc->w.hClipRgn, DeltaX, DeltaY);
               CLIPPING_UpdateGCRegion(dc);
            }
            if (NULL != pDCE->hClipRgn)
            {
               NtGdiOffsetRgn(pDCE->hClipRgn, DeltaX, DeltaY);
            }
         }
         DC_UnlockDc(dc);

         DceUpdateVisRgn(pDCE, CurrentWindow, pDCE->DCXFlags);

         if (Window->hSelf != pDCE->hwndCurrent)
         {
//            IntEngWindowChanged(CurrentWindow, WOC_RGN_CLIENT);
//            UserDerefObject(CurrentWindow);
         }
      }
      pDCE =(PDCE) pDCE->List.Flink;
   } while (pDCE != FirstDce);
}

HDC
FASTCALL
IntGetDC(PWINDOW_OBJECT Window)
{
  if (!Window)
  {  // MSDN:
     //"hWnd [in] Handle to the window whose DC is to be retrieved.
     // If this value is NULL, GetDC retrieves the DC for the entire screen."
     Window = UserGetWindowObject(IntGetDesktopWindow());
     if (Window)
        return UserGetDCEx(Window, NULL, DCX_CACHE | DCX_WINDOW);
     else
        return NULL;
  }
  return UserGetDCEx(Window, NULL, DCX_USESTYLE);
}

HWND FASTCALL
IntWindowFromDC(HDC hDc)
{
  DCE *Dce;
  HWND Ret = NULL;
  KeEnterCriticalRegion();
  Dce = FirstDce;
  do
  {
      if(Dce->hDC == hDc)
      {
         Ret = Dce->hwndCurrent;
         break;
      }
      Dce = (PDCE)Dce->List.Flink;
  } while (Dce != FirstDce);
  KeLeaveCriticalRegion();
  return Ret;
}

INT FASTCALL
UserReleaseDC(PWINDOW_OBJECT Window, HDC hDc, BOOL EndPaint)
{
  PDCE dce;
  INT nRet = 0;
  BOOL Hit = FALSE;

  DPRINT("%p %p\n", Window, hDc);
  dce = FirstDce;
  KeEnterCriticalRegion();
  do
  {
     if (dce->hDC == hDc)
     {
        Hit = TRUE;
        break;
     }

     dce = (PDCE) dce->List.Flink;
  }
  while (dce != FirstDce );
  KeLeaveCriticalRegion();

  if ( Hit && (dce->DCXFlags & DCX_DCEBUSY))
  {
     nRet = DceReleaseDC(dce, EndPaint);
  }

  return nRet;
}

DWORD FASTCALL
UserGetWindowDC(PWINDOW_OBJECT Wnd)
{
  return (DWORD)UserGetDCEx(Wnd, 0, DCX_USESTYLE | DCX_WINDOW);
}

HDC STDCALL
NtUserGetDCEx(HWND hWnd OPTIONAL, HANDLE ClipRegion, ULONG Flags)
{
  PWINDOW_OBJECT Wnd=NULL;
  DECLARE_RETURN(HDC);

  DPRINT("Enter NtUserGetDCEx\n");
  UserEnterExclusive();

  if (hWnd && !(Wnd = UserGetWindowObject(hWnd)))
  {
      RETURN(NULL);
  }

  RETURN( UserGetDCEx(Wnd, ClipRegion, Flags));

CLEANUP:
  DPRINT("Leave NtUserGetDCEx, ret=%i\n",_ret_);
  UserLeave();
  END_CLEANUP;
}

/*
 * NtUserGetWindowDC
 *
 * The NtUserGetWindowDC function retrieves the device context (DC) for the
 * entire window, including title bar, menus, and scroll bars. A window device
 * context permits painting anywhere in a window, because the origin of the
 * device context is the upper-left corner of the window instead of the client
 * area.
 *
 * Status
 *    @implemented
 */
DWORD STDCALL
NtUserGetWindowDC(HWND hWnd)
{
  return (DWORD)NtUserGetDCEx(hWnd, 0, DCX_USESTYLE | DCX_WINDOW);
}

HDC STDCALL
NtUserGetDC(HWND hWnd)
{
// We have a problem here! Should call IntGetDC.
  return NtUserGetDCEx(hWnd, NULL, NULL == hWnd ? DCX_CACHE | DCX_WINDOW : DCX_USESTYLE);
}

#define COPY_DEVMODE_VALUE_TO_CALLER(dst, src, member) \
    Status = MmCopyToCaller(&(dst)->member, &(src)->member, sizeof ((src)->member)); \
    if (!NT_SUCCESS(Status)) \
    { \
      SetLastNtError(Status); \
      ExFreePool(src); \
      return FALSE; \
    }

BOOL
STDCALL
NtUserEnumDisplaySettings(
   PUNICODE_STRING lpszDeviceName,
   DWORD iModeNum,
   LPDEVMODEW lpDevMode, /* FIXME is this correct? */
   DWORD dwFlags )
{
   NTSTATUS Status;
   LPDEVMODEW pSafeDevMode;
   PUNICODE_STRING pSafeDeviceName = NULL;
   UNICODE_STRING SafeDeviceName;
   USHORT Size = 0, ExtraSize = 0;

   /* Copy the devmode */
   Status = MmCopyFromCaller(&Size, &lpDevMode->dmSize, sizeof (Size));
   if (!NT_SUCCESS(Status))
   {
      SetLastNtError(Status);
      return FALSE;
   }
   Status = MmCopyFromCaller(&ExtraSize, &lpDevMode->dmDriverExtra, sizeof (ExtraSize));
   if (!NT_SUCCESS(Status))
   {
      SetLastNtError(Status);
      return FALSE;
   }
   pSafeDevMode = ExAllocatePool(PagedPool, Size + ExtraSize);
   if (pSafeDevMode == NULL)
   {
      SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
      return FALSE;
   }
   pSafeDevMode->dmSize = Size;
   pSafeDevMode->dmDriverExtra = ExtraSize;

   /* Copy the device name */
   if (lpszDeviceName != NULL)
   {
      Status = IntSafeCopyUnicodeString(&SafeDeviceName, lpszDeviceName);
      if (!NT_SUCCESS(Status))
      {
         ExFreePool(pSafeDevMode);
         SetLastNtError(Status);
         return FALSE;
      }
      pSafeDeviceName = &SafeDeviceName;
   }

   /* Call internal function */
   if (!IntEnumDisplaySettings(pSafeDeviceName, iModeNum, pSafeDevMode, dwFlags))
   {
      if (pSafeDeviceName != NULL)
         RtlFreeUnicodeString(pSafeDeviceName);
      ExFreePool(pSafeDevMode);
      return FALSE;
   }
   if (pSafeDeviceName != NULL)
      RtlFreeUnicodeString(pSafeDeviceName);

   /* Copy some information back */
   COPY_DEVMODE_VALUE_TO_CALLER(lpDevMode, pSafeDevMode, dmPelsWidth);
   COPY_DEVMODE_VALUE_TO_CALLER(lpDevMode, pSafeDevMode, dmPelsHeight);
   COPY_DEVMODE_VALUE_TO_CALLER(lpDevMode, pSafeDevMode, dmBitsPerPel);
   COPY_DEVMODE_VALUE_TO_CALLER(lpDevMode, pSafeDevMode, dmDisplayFrequency);
   COPY_DEVMODE_VALUE_TO_CALLER(lpDevMode, pSafeDevMode, dmDisplayFlags);

   /* output private/extra driver data */
   if (ExtraSize > 0)
   {
      Status = MmCopyToCaller((PCHAR)lpDevMode + Size, (PCHAR)pSafeDevMode + Size, ExtraSize);
      if (!NT_SUCCESS(Status))
      {
         SetLastNtError(Status);
         ExFreePool(pSafeDevMode);
         return FALSE;
      }
   }

   ExFreePool(pSafeDevMode);
   return TRUE;
}

#undef COPY_DEVMODE_VALUE_TO_CALLER

LONG
STDCALL
NtUserChangeDisplaySettings(
   PUNICODE_STRING lpszDeviceName,
   LPDEVMODEW lpDevMode,
   HWND hwnd,
   DWORD dwflags,
   LPVOID lParam)
{
   NTSTATUS Status;
   DEVMODEW DevMode;
   PUNICODE_STRING pSafeDeviceName = NULL;
   UNICODE_STRING SafeDeviceName;
   LONG Ret;

   /* Check arguments */
#ifdef CDS_VIDEOPARAMETERS

   if (dwflags != CDS_VIDEOPARAMETERS && lParam != NULL)
#else

   if (lParam != NULL)
#endif

   {
      SetLastWin32Error(ERROR_INVALID_PARAMETER);
      return DISP_CHANGE_BADPARAM;
   }
   if (hwnd != NULL)
   {
      SetLastWin32Error(ERROR_INVALID_PARAMETER);
      return DISP_CHANGE_BADPARAM;
   }

   /* Copy devmode */
   Status = MmCopyFromCaller(&DevMode.dmSize, &lpDevMode->dmSize, sizeof (DevMode.dmSize));
   if (!NT_SUCCESS(Status))
   {
      SetLastNtError(Status);
      return DISP_CHANGE_BADPARAM;
   }
   DevMode.dmSize = min(sizeof (DevMode), DevMode.dmSize);
   Status = MmCopyFromCaller(&DevMode, lpDevMode, DevMode.dmSize);
   if (!NT_SUCCESS(Status))
   {
      SetLastNtError(Status);
      return DISP_CHANGE_BADPARAM;
   }
   if (DevMode.dmDriverExtra > 0)
   {
      DbgPrint("(%s:%i) WIN32K: %s lpDevMode->dmDriverExtra is IGNORED!\n", __FILE__, __LINE__, __FUNCTION__);
      DevMode.dmDriverExtra = 0;
   }

   /* Copy the device name */
   if (lpszDeviceName != NULL)
   {
      Status = IntSafeCopyUnicodeString(&SafeDeviceName, lpszDeviceName);
      if (!NT_SUCCESS(Status))
      {
         SetLastNtError(Status);
         return DISP_CHANGE_BADPARAM;
      }
      pSafeDeviceName = &SafeDeviceName;
   }

   /* Call internal function */
   Ret = IntChangeDisplaySettings(pSafeDeviceName, &DevMode, dwflags, lParam);

   if (pSafeDeviceName != NULL)
      RtlFreeUnicodeString(pSafeDeviceName);

   return Ret;
}

/*!
 * Select logical palette into device context.
 * \param	hDC 				handle to the device context
 * \param	hpal				handle to the palette
 * \param	ForceBackground 	If this value is FALSE the logical palette will be copied to the device palette only when the applicatioon
 * 								is in the foreground. If this value is TRUE then map the colors in the logical palette to the device
 * 								palette colors in the best way.
 * \return	old palette
 *
 * \todo	implement ForceBackground == TRUE
*/
HPALETTE STDCALL NtUserSelectPalette(HDC  hDC,
                            HPALETTE  hpal,
                            BOOL  ForceBackground)
{
  PDC dc;
  HPALETTE oldPal = NULL;
  PPALGDI PalGDI;

  // FIXME: mark the palette as a [fore\back]ground pal
  dc = DC_LockDc(hDC);
  if (NULL != dc)
    {
      /* Check if this is a valid palette handle */
      PalGDI = PALETTE_LockPalette(hpal);
      if (NULL != PalGDI)
	{
          /* Is this a valid palette for this depth? */
          if ((dc->w.bitsPerPixel <= 8 && PAL_INDEXED == PalGDI->Mode)
              || (8 < dc->w.bitsPerPixel && PAL_INDEXED != PalGDI->Mode))
            {
              PALETTE_UnlockPalette(PalGDI);
              oldPal = dc->w.hPalette;
              dc->w.hPalette = hpal;
            }
          else if (8 < dc->w.bitsPerPixel && PAL_INDEXED == PalGDI->Mode)
            {
              PALETTE_UnlockPalette(PalGDI);
              oldPal = dc->PalIndexed;
              dc->PalIndexed = hpal;
            }
          else
            {
              PALETTE_UnlockPalette(PalGDI);
              oldPal = NULL;
            }
	}
      else
	{
	  oldPal = NULL;
	}
      DC_UnlockDc(dc);
    }

  return oldPal;
}


/* EOF */
