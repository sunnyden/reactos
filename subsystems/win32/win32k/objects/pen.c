/*
 * ReactOS Win32 Subsystem
 *
 * Copyright (C) 1998 - 2004 ReactOS Team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * $Id$
 */

#include <w32k.h>

#define NDEBUG
#include <debug.h>

/* PRIVATE FUNCTIONS **********************************************************/

PGDIBRUSHOBJ
FASTCALL
PENOBJ_LockPen(HGDIOBJ hBMObj)
{
   if (GDI_HANDLE_GET_TYPE(hBMObj) == GDI_OBJECT_TYPE_EXTPEN)
      return GDIOBJ_LockObj( hBMObj, GDI_OBJECT_TYPE_EXTPEN);
   else
      return GDIOBJ_LockObj( hBMObj, GDI_OBJECT_TYPE_PEN);
}

HPEN STDCALL
IntGdiExtCreatePen(
   DWORD dwPenStyle,
   DWORD dwWidth,
   IN ULONG ulBrushStyle,
   IN ULONG ulColor,
   IN ULONG_PTR ulClientHatch,
   IN ULONG_PTR ulHatch,
   DWORD dwStyleCount,
   PULONG pStyle,
   IN ULONG cjDIB,     // FIXME! We are shipping this too!
   IN BOOL bOldStylePen,
   IN OPTIONAL HBRUSH hbrush)
{
   HPEN hPen;
   PGDIBRUSHOBJ PenObject;
   static const BYTE PatternAlternate[] = {0x55, 0x55, 0x55};
   static const BYTE PatternDash[] = {0xFF, 0xFF, 0xC0};
   static const BYTE PatternDot[] = {0xE3, 0x8E, 0x38};
   static const BYTE PatternDashDot[] = {0xFF, 0x81, 0xC0};
   static const BYTE PatternDashDotDot[] = {0xFF, 0x8E, 0x38};

   dwWidth = abs(dwWidth);

   if (bOldStylePen)
   {
      PenObject = PENOBJ_AllocPenWithHandle();
   }
   else
   {
      PenObject = PENOBJ_AllocExtPenWithHandle();
   }

   if (!PenObject)
   {
      SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
      DPRINT("Can't allocate pen\n");
      return 0;
   }
   hPen = PenObject->BaseObject.hHmgr;

   // If nWidth is zero, the pen is a single pixel wide, regardless of the current transformation.
   if ((bOldStylePen) && (!dwWidth)) dwWidth = 1;

   PenObject->ptPenWidth.x = dwWidth;
   PenObject->ptPenWidth.y = 0;
   PenObject->ulPenStyle = dwPenStyle;
   PenObject->BrushAttr.lbColor = ulColor;
   PenObject->ulStyle = ulBrushStyle;
   // FIXME: copy the bitmap first ?
   PenObject->hbmClient = (HANDLE)ulClientHatch;
   PenObject->dwStyleCount = dwStyleCount;
   PenObject->pStyle = pStyle;

   PenObject->flAttrs = bOldStylePen? GDIBRUSH_IS_OLDSTYLEPEN : GDIBRUSH_IS_PEN;

   // If dwPenStyle is PS_COSMETIC, the width must be set to 1.
   if ( !(bOldStylePen) && ((dwPenStyle & PS_TYPE_MASK) == PS_COSMETIC) && ( dwWidth != 1) )
      goto ExitCleanup;

   switch (dwPenStyle & PS_STYLE_MASK)
   {
      case PS_NULL:
         PenObject->flAttrs |= GDIBRUSH_IS_NULL;
         break;

      case PS_SOLID:
         PenObject->flAttrs |= GDIBRUSH_IS_SOLID;
         break;

      case PS_ALTERNATE:
         /* PS_ALTERNATE is applicable only for cosmetic pens */
         if ((dwPenStyle & PS_TYPE_MASK) == PS_GEOMETRIC) goto ExitCleanup;
         PenObject->flAttrs |= GDIBRUSH_IS_BITMAP;
         PenObject->hbmPattern = IntGdiCreateBitmap(24, 1, 1, 1, (LPBYTE)PatternAlternate);
         break;

      case PS_DOT:
         PenObject->flAttrs |= GDIBRUSH_IS_BITMAP;
         PenObject->hbmPattern = IntGdiCreateBitmap(24, 1, 1, 1, (LPBYTE)PatternDot);
         break;

      case PS_DASH:
         PenObject->flAttrs |= GDIBRUSH_IS_BITMAP;
         PenObject->hbmPattern = IntGdiCreateBitmap(24, 1, 1, 1, (LPBYTE)PatternDash);
         break;

      case PS_DASHDOT:
         PenObject->flAttrs |= GDIBRUSH_IS_BITMAP;
         PenObject->hbmPattern = IntGdiCreateBitmap(24, 1, 1, 1, (LPBYTE)PatternDashDot);
         break;

      case PS_DASHDOTDOT:
         PenObject->flAttrs |= GDIBRUSH_IS_BITMAP;
         PenObject->hbmPattern = IntGdiCreateBitmap(24, 1, 1, 1, (LPBYTE)PatternDashDotDot);
         break;

      case PS_INSIDEFRAME:
         /* FIXME: does it need some additional work? */
         /* PS_INSIDEFRAME is applicable only for geometric pens */
         if ((dwPenStyle & PS_TYPE_MASK) == PS_COSMETIC) goto ExitCleanup;
         PenObject->flAttrs |= (GDIBRUSH_IS_SOLID|GDIBRUSH_IS_INSIDEFRAME);
         break;

      case PS_USERSTYLE:
         if ((dwPenStyle & PS_TYPE_MASK) == PS_COSMETIC)
         {
            /* FIXME: PS_USERSTYLE workaround */
            DPRINT1("PS_COSMETIC | PS_USERSTYLE not handled\n");
            PenObject->flAttrs |= GDIBRUSH_IS_SOLID;
            break;
         }
         else
         {
            UINT i;
            BOOL has_neg = FALSE, all_zero = TRUE;

            for(i = 0; (i < dwStyleCount) && !has_neg; i++)
            {
                has_neg = has_neg || (((INT)(pStyle[i])) < 0);
                all_zero = all_zero && (pStyle[i] == 0);
            }

            if(all_zero || has_neg)
            {
                goto ExitCleanup;
            }
         }
         /* FIXME: what style here? */
         PenObject->flAttrs |= 0;
         break;

      default:
         DPRINT1("IntGdiExtCreatePen unknown penstyle %x\n", dwPenStyle);
   }
   PENOBJ_UnlockPen(PenObject);
   return hPen;

ExitCleanup:
   SetLastWin32Error(ERROR_INVALID_PARAMETER);
   if (PenObject->pStyle)
   {
      ExFreePool(PenObject->pStyle);
      PenObject->pStyle = NULL;
   }
   PENOBJ_UnlockPen(PenObject);
   if (bOldStylePen)
      PENOBJ_FreePenByHandle(hPen);
   else
      PENOBJ_FreeExtPenByHandle(hPen);
   return NULL;
}

VOID FASTCALL
IntGdiSetSolidPenColor(HPEN hPen, COLORREF Color)
{
  PGDIBRUSHOBJ PenObject;

  PenObject = PENOBJ_LockPen(hPen);
  if (PenObject)
  {
    if (PenObject->flAttrs & GDIBRUSH_IS_SOLID)
    {
      PenObject->BrushAttr.lbColor = Color & 0xFFFFFF;
    }
    PENOBJ_UnlockPen(PenObject);
  }
}

INT STDCALL
PEN_GetObject(PGDIBRUSHOBJ pPenObject, INT cbCount, PLOGPEN pBuffer)
{
   PLOGPEN pLogPen;
   PEXTLOGPEN pExtLogPen;
   INT cbRetCount;

   if (pPenObject->flAttrs & GDIBRUSH_IS_OLDSTYLEPEN)
   {
      cbRetCount = sizeof(LOGPEN);
      if (pBuffer)
      {
         if (cbCount < cbRetCount) return 0;
         pLogPen = (PLOGPEN)pBuffer;
         pLogPen->lopnWidth = pPenObject->ptPenWidth;
         pLogPen->lopnStyle = pPenObject->ulPenStyle;
         pLogPen->lopnColor = pPenObject->BrushAttr.lbColor;
      }
   }
   else
   {
      // FIXME: Can we trust in dwStyleCount being <= 16?
      cbRetCount = sizeof(EXTLOGPEN) - sizeof(DWORD) + pPenObject->dwStyleCount * sizeof(DWORD);
      if (pBuffer)
      {
         INT i;

         if (cbCount < cbRetCount) return 0;
         pExtLogPen = (PEXTLOGPEN)pBuffer;
         pExtLogPen->elpPenStyle = pPenObject->ulPenStyle;
         pExtLogPen->elpWidth = pPenObject->ptPenWidth.x;
         pExtLogPen->elpBrushStyle = pPenObject->ulStyle;
         pExtLogPen->elpColor = pPenObject->BrushAttr.lbColor;
         pExtLogPen->elpHatch = (ULONG_PTR)pPenObject->hbmClient;
         pExtLogPen->elpNumEntries = pPenObject->dwStyleCount;
         for (i = 0; i < pExtLogPen->elpNumEntries; i++)
         {
            pExtLogPen->elpStyleEntry[i] = pPenObject->pStyle[i];
         }
      }
   }

   return cbRetCount;
}

/* PUBLIC FUNCTIONS ***********************************************************/

HPEN STDCALL
NtGdiCreatePen(
   INT PenStyle,
   INT Width,
   COLORREF Color,
   IN HBRUSH hbr)
{
   if ( PenStyle < PS_SOLID || PenStyle > PS_INSIDEFRAME )
   {
      SetLastWin32Error(ERROR_INVALID_PARAMETER);
      return NULL;
   }

   return IntGdiExtCreatePen(PenStyle,
                             Width,
                             BS_SOLID,
                             Color,
                             0,
                             0,
                             0,
                             NULL,
                             0,
                             TRUE,
                             0);
}

HPEN STDCALL
NtGdiExtCreatePen(
   DWORD dwPenStyle,
   DWORD ulWidth,
   IN ULONG ulBrushStyle,
   IN ULONG ulColor,
   IN ULONG_PTR ulClientHatch,
   IN ULONG_PTR ulHatch,
   DWORD dwStyleCount,
   PULONG pUnsafeStyle,
   IN ULONG cjDIB,
   IN BOOL bOldStylePen,
   IN OPTIONAL HBRUSH hBrush)
{
   NTSTATUS Status = STATUS_SUCCESS;
   DWORD* pSafeStyle = NULL;
   HPEN hPen;

   if (dwStyleCount < 0)  return 0;
   if (dwStyleCount > 16)
   {
      SetLastWin32Error(ERROR_INVALID_PARAMETER);
      return 0;
   }

   if (dwStyleCount > 0)
   {
      pSafeStyle = ExAllocatePoolWithTag(NonPagedPool, dwStyleCount * sizeof(DWORD), TAG_PENSTYLES);
      if (!pSafeStyle)
      {
         SetLastNtError(ERROR_NOT_ENOUGH_MEMORY);
         return 0;
      }
      _SEH_TRY
      {
         ProbeForRead(pUnsafeStyle, dwStyleCount * sizeof(DWORD), 1);
         RtlCopyMemory(pSafeStyle,
                       pUnsafeStyle,
                       dwStyleCount * sizeof(DWORD));
      }
      _SEH_HANDLE
      {
         Status = _SEH_GetExceptionCode();
      }
      _SEH_END
      if(!NT_SUCCESS(Status))
      {
         SetLastNtError(Status);
         ExFreePool(pSafeStyle);
         return 0;
      }
   }

   hPen = IntGdiExtCreatePen(dwPenStyle,
                             ulWidth,
                             ulBrushStyle,
                             ulColor,
                             ulClientHatch,
                             ulHatch,
                             dwStyleCount,
                             pSafeStyle,
                             cjDIB,
                             bOldStylePen,
                             hBrush);

   return hPen;
}


/* EOF */
