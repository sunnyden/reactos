/*
 *  ReactOS W32 Subsystem
 *  Copyright (C) 1998, 1999, 2000, 2001, 2002, 2003 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/* $Id$ */

#include <w32k.h>

#define NDEBUG
#include <debug.h>

// FIXME: Use PXLATEOBJ logicalToSystem instead of int *mapping

int COLOR_gapStart = 256;
int COLOR_gapEnd = -1;
int COLOR_gapFilled = 0;
int COLOR_max = 256;

#ifndef NO_MAPPING
static HPALETTE hPrimaryPalette = 0; // used for WM_PALETTECHANGED
#endif
//static HPALETTE hLastRealizedPalette = 0; // UnrealizeObject() needs it


static UINT SystemPaletteUse = SYSPAL_NOSTATIC;  /* the program need save the pallete and restore it */

const PALETTEENTRY COLOR_sysPalTemplate[NB_RESERVED_COLORS] =
{
  // first 10 entries in the system palette
  // red  green blue  flags
  { 0x00, 0x00, 0x00, PC_SYS_USED },
  { 0x80, 0x00, 0x00, PC_SYS_USED },
  { 0x00, 0x80, 0x00, PC_SYS_USED },
  { 0x80, 0x80, 0x00, PC_SYS_USED },
  { 0x00, 0x00, 0x80, PC_SYS_USED },
  { 0x80, 0x00, 0x80, PC_SYS_USED },
  { 0x00, 0x80, 0x80, PC_SYS_USED },
  { 0xc0, 0xc0, 0xc0, PC_SYS_USED },
  { 0xc0, 0xdc, 0xc0, PC_SYS_USED },
  { 0xd4, 0xd0, 0xc7, PC_SYS_USED },

  // ... c_min/2 dynamic colorcells
  // ... gap (for sparse palettes)
  // ... c_min/2 dynamic colorcells

  { 0xff, 0xfb, 0xf0, PC_SYS_USED },
  { 0x3a, 0x6e, 0xa5, PC_SYS_USED },
  { 0x80, 0x80, 0x80, PC_SYS_USED },
  { 0xff, 0x00, 0x00, PC_SYS_USED },
  { 0x00, 0xff, 0x00, PC_SYS_USED },
  { 0xff, 0xff, 0x00, PC_SYS_USED },
  { 0x00, 0x00, 0xff, PC_SYS_USED },
  { 0xff, 0x00, 0xff, PC_SYS_USED },
  { 0x00, 0xff, 0xff, PC_SYS_USED },
  { 0xff, 0xff, 0xff, PC_SYS_USED }     // last 10
};

const PALETTEENTRY* FASTCALL COLOR_GetSystemPaletteTemplate(void)
{
   return (const PALETTEENTRY*)&COLOR_sysPalTemplate;
}

UINT STDCALL
IntAnimatePalette(HPALETTE hPal,
                  UINT StartIndex,
                  UINT NumEntries,
                  CONST PPALETTEENTRY PaletteColors)
{
    UINT ret = 0;

    if( hPal != NtGdiGetStockObject(DEFAULT_PALETTE) )
    {
        PPALGDI palPtr;
        UINT pal_entries;
        HDC hDC;
        PDC dc;
        PWINDOW_OBJECT Wnd;
        const PALETTEENTRY *pptr = PaletteColors;

        palPtr = (PPALGDI)PALETTE_LockPalette(hPal);
        if (!palPtr) return FALSE;

        pal_entries = palPtr->NumColors;
        if (StartIndex >= pal_entries)
        {
            PALETTE_UnlockPalette(palPtr);
            return FALSE;
        }
        if (StartIndex+NumEntries > pal_entries) NumEntries = pal_entries - StartIndex;

        for (NumEntries += StartIndex; StartIndex < NumEntries; StartIndex++, pptr++)
        {
            /* According to MSDN, only animate PC_RESERVED colours */
            if (palPtr->IndexedColors[StartIndex].peFlags & PC_RESERVED)
            {
                memcpy( &palPtr->IndexedColors[StartIndex], pptr,
                        sizeof(PALETTEENTRY) );
                ret++;
                PALETTE_ValidateFlags(&palPtr->IndexedColors[StartIndex], 1);
            }
        }

        PALETTE_UnlockPalette(palPtr);

        /* Immediately apply the new palette if current window uses it */
        Wnd = UserGetDesktopWindow();
        hDC =  (HDC)UserGetWindowDC(Wnd);
        dc = DC_LockDc(hDC);
        if (NULL != dc)
        {
            if (dc->w.hPalette == hPal)
            {
                DC_UnlockDc(dc);
                NtGdiRealizePalette(hDC);
            }
            else
                DC_UnlockDc(dc);
        }
        UserReleaseDC(Wnd,hDC, FALSE);
    }
    return ret;
}

HPALETTE STDCALL NtGdiCreateHalftonePalette(HDC  hDC)
{
    int i, r, g, b;
    struct {
        WORD Version;
        WORD NumberOfEntries;
        PALETTEENTRY aEntries[256];
        } Palette;

    Palette.Version = 0x300;
    Palette.NumberOfEntries = 256;
    if (IntGetSystemPaletteEntries(hDC, 0, 256, Palette.aEntries) == 0)
    {
        /* from wine, more that 256 color math */
        Palette.NumberOfEntries = 20;
        for (i = 0; i < Palette.NumberOfEntries; i++)
        {
            Palette.aEntries[i].peRed=0xff;
            Palette.aEntries[i].peGreen=0xff;
            Palette.aEntries[i].peBlue=0xff;
            Palette.aEntries[i].peFlags=0x00;
        }

        Palette.aEntries[0].peRed=0x00;
        Palette.aEntries[0].peBlue=0x00;
        Palette.aEntries[0].peGreen=0x00;

        /* the first 6 */
        for (i=1; i <= 6; i++)
        {
            Palette.aEntries[i].peRed=(i%2)?0x80:0;
            Palette.aEntries[i].peGreen=(i==2)?0x80:(i==3)?0x80:(i==6)?0x80:0;
            Palette.aEntries[i].peBlue=(i>3)?0x80:0;
        }

        for (i=7;  i <= 12; i++)
        {
            switch(i)
            {
                case 7:
                    Palette.aEntries[i].peRed=0xc0;
                    Palette.aEntries[i].peBlue=0xc0;
                    Palette.aEntries[i].peGreen=0xc0;
                    break;
                case 8:
                    Palette.aEntries[i].peRed=0xc0;
                    Palette.aEntries[i].peGreen=0xdc;
                    Palette.aEntries[i].peBlue=0xc0;
                    break;
                case 9:
                    Palette.aEntries[i].peRed=0xa6;
                    Palette.aEntries[i].peGreen=0xca;
                    Palette.aEntries[i].peBlue=0xf0;
                    break;
                case 10:
                    Palette.aEntries[i].peRed=0xff;
                    Palette.aEntries[i].peGreen=0xfb;
                    Palette.aEntries[i].peBlue=0xf0;
                    break;
                case 11:
                    Palette.aEntries[i].peRed=0xa0;
                    Palette.aEntries[i].peGreen=0xa0;
                    Palette.aEntries[i].peBlue=0xa4;
                    break;
            case 12:
                Palette.aEntries[i].peRed=0x80;
                Palette.aEntries[i].peGreen=0x80;
                Palette.aEntries[i].peBlue=0x80;
            }
        }

        for (i=13; i <= 18; i++)
        {
            Palette.aEntries[i].peRed=(i%2)?0xff:0;
            Palette.aEntries[i].peGreen=(i==14)?0xff:(i==15)?0xff:(i==18)?0xff:0;
            Palette.aEntries[i].peBlue=(i>15)?0xff:0x00;
        }
    }
    else
    {
        /* 256 color table */
        for (r = 0; r < 6; r++)
            for (g = 0; g < 6; g++)
                for (b = 0; b < 6; b++)
                {
                    i = r + g*6 + b*36 + 10;
                    Palette.aEntries[i].peRed = r * 51;
                    Palette.aEntries[i].peGreen = g * 51;
                    Palette.aEntries[i].peBlue = b * 51;
                }

        for (i = 216; i < 246; i++)
        {
            int v = (i - 216) << 3;
            Palette.aEntries[i].peRed = v;
            Palette.aEntries[i].peGreen = v;
            Palette.aEntries[i].peBlue = v;
        }
    }

   return NtGdiCreatePaletteInternal((LOGPALETTE *)&Palette, Palette.NumberOfEntries);
}



/*
 * @implemented
 */
HPALETTE STDCALL
NtGdiCreatePaletteInternal ( IN LPLOGPALETTE pLogPal, IN UINT cEntries )
{
    PPALGDI PalGDI;
    HPALETTE NewPalette;

    pLogPal->palNumEntries = cEntries;
    NewPalette = PALETTE_AllocPalette( PAL_INDEXED,
                                       cEntries,
                                       (PULONG)pLogPal->palPalEntry,
                                       0, 0, 0);

    if (NewPalette == NULL)
    {
        return NULL;
    }

    PalGDI = (PPALGDI) PALETTE_LockPalette(NewPalette);
    if (PalGDI != NULL)
    {
        PALETTE_ValidateFlags(PalGDI->IndexedColors, PalGDI->NumColors);
        PalGDI->logicalToSystem = NULL;
        PALETTE_UnlockPalette(PalGDI);
    }
    else
    {
        /* FIXME - Handle PalGDI == NULL!!!! */
        DPRINT1("waring PalGDI is NULL \n");
    }
  return NewPalette;
}


BOOL STDCALL NtGdiGetColorAdjustment(HDC  hDC,
                             LPCOLORADJUSTMENT  ca)
{
   UNIMPLEMENTED;
   return FALSE;
}

unsigned short GetNumberOfBits(unsigned int dwMask)
{
   unsigned short wBits;
   for (wBits = 0; dwMask; dwMask = dwMask & (dwMask - 1))
      wBits++;
   return wBits;
}

COLORREF STDCALL NtGdiGetNearestColor(HDC hDC, COLORREF Color)
{
   COLORREF nearest = CLR_INVALID;
   PDC dc;
   PPALGDI palGDI;
   LONG RBits, GBits, BBits;

   dc = DC_LockDc(hDC);
   if (NULL != dc)
   {
      HPALETTE hpal = dc->w.hPalette;
      palGDI = (PPALGDI) PALETTE_LockPalette(hpal);
      if (!palGDI)
      {
         DC_UnlockDc(dc);
         return nearest;
      }

      switch (palGDI->Mode)
      {
         case PAL_INDEXED:
            nearest = COLOR_LookupNearestColor(palGDI->IndexedColors,
               palGDI->NumColors, Color);
            break;
         case PAL_BGR:
         case PAL_RGB:
            nearest = Color;
            break;
         case PAL_BITFIELDS:
            RBits = 8 - GetNumberOfBits(palGDI->RedMask);
            GBits = 8 - GetNumberOfBits(palGDI->GreenMask);
            BBits = 8 - GetNumberOfBits(palGDI->BlueMask);
            nearest = RGB(
              (GetRValue(Color) >> RBits) << RBits,
              (GetGValue(Color) >> GBits) << GBits,
              (GetBValue(Color) >> BBits) << BBits);
            break;
      }
      PALETTE_UnlockPalette(palGDI);
      DC_UnlockDc(dc);
   }

   return nearest;
}

UINT STDCALL NtGdiGetNearestPaletteIndex(HPALETTE  hpal,
                                 COLORREF  Color)
{
  PPALGDI palGDI = (PPALGDI) PALETTE_LockPalette(hpal);
  UINT index  = 0;

  if (NULL != palGDI)
    {
      /* Return closest match for the given RGB color */
      index = COLOR_PaletteLookupPixel(palGDI->IndexedColors, palGDI->NumColors, NULL, Color, FALSE);
      PALETTE_UnlockPalette(palGDI);
    }

  return index;
}

UINT STDCALL
IntGetPaletteEntries(HPALETTE  hpal,
                     UINT  StartIndex,
                     UINT  Entries,
                     LPPALETTEENTRY  pe)
{
    PPALGDI palGDI;
    UINT numEntries;

    palGDI = (PPALGDI) PALETTE_LockPalette(hpal);
    if (NULL == palGDI)
    {
        return 0;
    }

    numEntries = palGDI->NumColors;
    if (NULL != pe)
    {
        if (numEntries < StartIndex + Entries)
        {
            Entries = numEntries - StartIndex;
        }
        if (numEntries <= StartIndex)
        {
            PALETTE_UnlockPalette(palGDI);
            return 0;
        }
        memcpy(pe, palGDI->IndexedColors + StartIndex, Entries * sizeof(PALETTEENTRY));
        for (numEntries = 0; numEntries < Entries; numEntries++)
        {
            if (pe[numEntries].peFlags & 0xF0)
            {
                pe[numEntries].peFlags = 0;
            }
        }
    }
    else
    {
        Entries = numEntries;
    }

    PALETTE_UnlockPalette(palGDI);
    return Entries;
}

UINT STDCALL
IntGetSystemPaletteEntries(HDC  hDC,
                           UINT  StartIndex,
                           UINT  Entries,
                           LPPALETTEENTRY  pe)
{
    PPALGDI palGDI = NULL;
    PDC dc = NULL;
    UINT EntriesSize = 0;
    UINT Ret = 0;

    if (Entries == 0)
    {
        SetLastWin32Error(ERROR_INVALID_PARAMETER);
        return 0;
    }

    if (pe != NULL)
    {
        EntriesSize = Entries * sizeof(pe[0]);
        if (Entries != EntriesSize / sizeof(pe[0]))
        {
            /* Integer overflow! */
            SetLastWin32Error(ERROR_INVALID_PARAMETER);
            return 0;
        }
    }

    if (!(dc = DC_LockDc(hDC)))
    {
        SetLastWin32Error(ERROR_INVALID_HANDLE);
        return 0;
    }

    palGDI = PALETTE_LockPalette(dc->w.hPalette);
    if (palGDI != NULL)
    {
        if (pe != NULL)
        {
            UINT CopyEntries;

            if (StartIndex + Entries < palGDI->NumColors)
                CopyEntries = StartIndex + Entries;
            else
                CopyEntries = palGDI->NumColors - StartIndex;

            memcpy(pe,
                   palGDI->IndexedColors + StartIndex,
                   CopyEntries * sizeof(pe[0]));

            Ret = CopyEntries;
        }
        else
        {
            Ret = ((PGDIDEVICE)dc->pPDev)->GDIInfo.ulNumPalReg;
        }
    }

    if (palGDI != NULL)
        PALETTE_UnlockPalette(palGDI);

    if (dc != NULL)
        DC_UnlockDc(dc);

    return Ret;
}

UINT STDCALL NtGdiGetSystemPaletteUse(HDC  hDC)
{
  return SystemPaletteUse;
}

/*!
The RealizePalette function modifies the palette for the device associated with the specified device context. If the device context is a memory DC, the color table for the bitmap selected into the DC is modified. If the device context is a display DC, the physical palette for that device is modified.

A logical palette is a buffer between color-intensive applications and the system, allowing these applications to use as many colors as needed without interfering with colors displayed by other windows.

1= IF DRAWING TO A DEVICE
-- If it is a paletted bitmap, and is not an identity palette, then an XLATEOBJ is created between the logical palette and
   the system palette.
-- If it is an RGB palette, then an XLATEOBJ is created between the RGB values and the system palette.

2= IF DRAWING TO A MEMORY DC\BITMAP
-- If it is a paletted bitmap, and is not an identity palette, then an XLATEOBJ is created between the logical palette and
   the dc palette.
-- If it is an RGB palette, then an XLATEOBJ is created between the RGB values and the dc palette.
*/
UINT STDCALL NtGdiRealizePalette(HDC hDC)
{
  /*
   * This function doesn't do any real work now and there's plenty
   * of bugd in it (calling SetPalette for high/true-color modes,
   * using DEFAULT_PALETTE instead of the device palette, ...).
   */

  PALOBJ *palPtr, *sysPtr;
  PPALGDI palGDI, sysGDI;
  int realized = 0;
  PDC dc;
  HPALETTE systemPalette;
  BOOLEAN success;
  USHORT sysMode, palMode;

  dc = DC_LockDc(hDC);
  if (!dc)
  	return 0;

  systemPalette = NtGdiGetStockObject((INT)DEFAULT_PALETTE);
  palGDI = PALETTE_LockPalette(dc->w.hPalette);
  palPtr = (PALOBJ*) palGDI;

  if (palGDI == NULL)
  {
	 /* FIXME - Handle palGDI == NULL!!!!
	    we should not unlock dc and return 0 ??
		shall we create the pallete ??
	 */
     DC_UnlockDc(dc);
	 return 0;
  }

  sysGDI = PALETTE_LockPalette(systemPalette);
  sysPtr = (PALOBJ*) sysGDI;

  if (sysGDI == NULL)
  {
	 /* FIXME - Handle sysGDI == NULL!!!!!
	    we should not unlock dc and return 0 ??
		shall we create the pallete ??
	 */
     PALETTE_UnlockPalette(palGDI);
     DC_UnlockDc(dc);
	 return 0;
  }


  // Step 1: Create mapping of system palette\DC palette
#ifndef NO_MAPPING
  realized = PALETTE_SetMapping(palPtr, 0, palGDI->NumColors,
               (dc->w.hPalette != hPrimaryPalette) ||
               (dc->w.hPalette == NtGdiGetStockObject(DEFAULT_PALETTE)));
#else
  realized = 0;
#endif



  // Step 2:
  // The RealizePalette function modifies the palette for the device associated with the specified device context. If the
  // device context is a memory DC, the color table for the bitmap selected into the DC is modified. If the device
  // context is a display DC, the physical palette for that device is modified.
  if(dc->w.flags == DC_MEMORY)
  {
    // Memory managed DC
	ASSERT(sysGDI->NumColors <= 256);
	success = ((GDIDEVICE *)dc->pPDev)->DriverFunctions.SetPalette(
		dc->PDev, sysPtr, 0, 0, sysGDI->NumColors);

  } else {
    if( ((GDIDEVICE *)dc->pPDev)->DriverFunctions.SetPalette)
    {
      ASSERT(palGDI->NumColors <= 256);
      success = ((GDIDEVICE *)dc->pPDev)->DriverFunctions.SetPalette(
        dc->PDev, palPtr, 0, 0, palGDI->NumColors);
    }
  }

  // need to pass this to IntEngCreateXlate with palettes unlocked
  sysMode = sysGDI->Mode;
  palMode = palGDI->Mode;
  PALETTE_UnlockPalette(sysGDI);
  PALETTE_UnlockPalette(palGDI);

  // Step 3: Create the XLATEOBJ for device managed DCs
  if(dc->w.flags != DC_MEMORY)
  {
    // Device managed DC
    palGDI->logicalToSystem = IntEngCreateXlate(sysMode, palMode, systemPalette, dc->w.hPalette);
  }

  DC_UnlockDc(dc);

  return realized;
}

BOOL STDCALL NtGdiResizePalette(HPALETTE  hpal,
                        UINT  Entries)
{
/*  PALOBJ *palPtr = (PALOBJ*)AccessUserObject(hPal);
  UINT cPrevEnt, prevVer;
  INT prevsize, size = sizeof(LOGPALETTE) + (cEntries - 1) * sizeof(PALETTEENTRY);
  XLATEOBJ *XlateObj = NULL;

  if(!palPtr) return FALSE;
  cPrevEnt = palPtr->logpalette->palNumEntries;
  prevVer = palPtr->logpalette->palVersion;
  prevsize = sizeof(LOGPALETTE) + (cPrevEnt - 1) * sizeof(PALETTEENTRY) + sizeof(int*) + sizeof(GDIOBJHDR);
  size += sizeof(int*) + sizeof(GDIOBJHDR);
  XlateObj = palPtr->logicalToSystem;

  if (!(palPtr = GDI_ReallocObject(size, hPal, palPtr))) return FALSE;

  if(XlateObj)
  {
    XLATEOBJ *NewXlateObj = (int*) HeapReAlloc(GetProcessHeap(), 0, XlateObj, cEntries * sizeof(int));
    if(NewXlateObj == NULL)
    {
      ERR("Can not resize logicalToSystem -- out of memory!");
      GDI_ReleaseObj( hPal );
      return FALSE;
    }
    palPtr->logicalToSystem = NewXlateObj;
  }

  if(cEntries > cPrevEnt)
  {
    if(XlateObj) memset(palPtr->logicalToSystem + cPrevEnt, 0, (cEntries - cPrevEnt)*sizeof(int));
    memset( (BYTE*)palPtr + prevsize, 0, size - prevsize );
    PALETTE_ValidateFlags((PALETTEENTRY*)((BYTE*)palPtr + prevsize), cEntries - cPrevEnt );
  }
  palPtr->logpalette->palNumEntries = cEntries;
  palPtr->logpalette->palVersion = prevVer;
//    GDI_ReleaseObj( hPal );
  return TRUE; */

  UNIMPLEMENTED;
  return FALSE;
}

BOOL STDCALL NtGdiSetColorAdjustment(HDC  hDC,
                             CONST LPCOLORADJUSTMENT  ca)
{
   UNIMPLEMENTED;
   return FALSE;
}

UINT STDCALL
IntSetPaletteEntries(HPALETTE  hpal,
                      UINT  Start,
                      UINT  Entries,
                      CONST LPPALETTEENTRY  pe)
{
    PPALGDI palGDI;
    WORD numEntries;

    if ((UINT)hpal & GDI_HANDLE_STOCK_MASK)
    {
    	return 0;
    }

    palGDI = PALETTE_LockPalette(hpal);
    if (!palGDI) return 0;

    numEntries = palGDI->NumColors;
    if (Start >= numEntries)
    {
        PALETTE_UnlockPalette(palGDI);
        return 0;
    }
    if (numEntries < Start + Entries)
    {
        Entries = numEntries - Start;
    }
    memcpy(palGDI->IndexedColors + Start, pe, Entries * sizeof(PALETTEENTRY));
    PALETTE_ValidateFlags(palGDI->IndexedColors, palGDI->NumColors);
    ExFreePool(palGDI->logicalToSystem);
    palGDI->logicalToSystem = NULL;
    PALETTE_UnlockPalette(palGDI);

    return Entries;
}

UINT STDCALL
NtGdiSetSystemPaletteUse(HDC hDC, UINT Usage)
{
    UINT old = SystemPaletteUse;

    /* Device doesn't support colour palettes */
    if (!(NtGdiGetDeviceCaps(hDC, RASTERCAPS) & RC_PALETTE)) {
        return SYSPAL_ERROR;
    }

    switch (Usage)
	{
		case SYSPAL_NOSTATIC:
        case SYSPAL_NOSTATIC256:
        case SYSPAL_STATIC:
				SystemPaletteUse = Usage;
				break;

        default:
				old=SYSPAL_ERROR;
				break;
	}

 return old;
}

/*
   Win 2k Graphics API, Black Book. by coriolis.com
   Page 62, Note that Steps 3, 5, and 6 are not required for Windows NT(tm)
   and Windows 2000(tm).

   Step 5. UnrealizeObject(hTrackBrush);
 */
BOOL STDCALL
NtGdiUnrealizeObject(HGDIOBJ hgdiobj)
{

   GDIOBJHDR * ptr;
   DWORD objectType;
   BOOL Ret = FALSE;

   /* From Wine: UnrealizeObject does not SetLastError() on a null object */
   if(!hgdiobj)
     return Ret;

   ptr = GDIOBJ_LockObj(GdiHandleTable, hgdiobj, GDI_OBJECT_TYPE_DONTCARE);
   if (ptr == 0)
     {
        SetLastWin32Error(ERROR_INVALID_HANDLE);
        return Ret;
     }
   objectType = GDIOBJ_GetObjectType(hgdiobj);
   switch(objectType)
     {
/*
    msdn.microsoft.com,
    "Windows 2000/XP: If hgdiobj is a brush, UnrealizeObject does nothing,
    and the function returns TRUE. Use SetBrushOrgEx to set the origin of
    a brush."
 */
         case GDI_OBJECT_TYPE_BRUSH:
           {
              DPRINT("GDI_OBJECT_TYPE_BRUSH\n");
              Ret = TRUE;
              break;
           }
         default:
           DPRINT1("Magic 0x%08x not implemented\n", objectType);
           break;
     }

   GDIOBJ_UnlockObjByPtr(GdiHandleTable, ptr);
   return Ret;
}

BOOL STDCALL
NtGdiUpdateColors(HDC hDC)
{
   PWINDOW_OBJECT Wnd;
   BOOL calledFromUser, ret;
   USER_REFERENCE_ENTRY Ref;

   calledFromUser = UserIsEntered();

   if (!calledFromUser){
      UserEnterExclusive();
   }

   Wnd = UserGetWindowObject(IntWindowFromDC(hDC));
   if (Wnd == NULL)
   {
      SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);

      if (!calledFromUser){
         UserLeave();
      }

      return FALSE;
   }

   UserRefObjectCo(Wnd, &Ref);
   ret = co_UserRedrawWindow(Wnd, NULL, 0, RDW_INVALIDATE);
   UserDerefObjectCo(Wnd);

   if (!calledFromUser){
      UserLeave();
   }

   return ret;
}

INT STDCALL COLOR_PaletteLookupPixel(PALETTEENTRY *palPalEntry, INT size,
                             XLATEOBJ *XlateObj, COLORREF col, BOOL skipReserved)
{
  int i, best = 0, diff = 0x7fffffff;
  int r, g, b;

  for( i = 0; i < size && diff ; i++ )
  {
#if 0
    if(!(palPalEntry[i].peFlags & PC_SYS_USED) || (skipReserved && palPalEntry[i].peFlags  & PC_SYS_RESERVED))
      continue;
#endif

    r = abs((SHORT)palPalEntry[i].peRed - GetRValue(col));
    g = abs((SHORT)palPalEntry[i].peGreen - GetGValue(col));
    b = abs((SHORT)palPalEntry[i].peBlue - GetBValue(col));

    r = r*r + g*g + b*b;

    if( r < diff ) { best = i; diff = r; }
  }

  if (XlateObj == NULL)
    return best;
  else
    return (XlateObj->pulXlate) ? (INT)XlateObj->pulXlate[best] : best;
}

COLORREF STDCALL COLOR_LookupNearestColor( PALETTEENTRY* palPalEntry, int size, COLORREF color )
{
   INT index;

   index = COLOR_PaletteLookupPixel(palPalEntry, size, NULL, color, FALSE);
   return RGB(
      palPalEntry[index].peRed,
      palPalEntry[index].peGreen,
      palPalEntry[index].peBlue);
}

int STDCALL COLOR_PaletteLookupExactIndex( PALETTEENTRY* palPalEntry, int size,
                                   COLORREF col )
{
  int i;
  BYTE r = GetRValue(col), g = GetGValue(col), b = GetBValue(col);
  for( i = 0; i < size; i++ )
  {
    if( palPalEntry[i].peFlags & PC_SYS_USED )  /* skips gap */
      if(palPalEntry[i].peRed == r && palPalEntry[i].peGreen == g && palPalEntry[i].peBlue == b) return i;
  }
  return -1;
}


W32KAPI
LONG
APIENTRY
NtGdiDoPalette(
    IN HGDIOBJ hObj,
    IN WORD iStart,
    IN WORD cEntries,
    IN LPVOID pUnsafeEntries,
    IN DWORD iFunc,
    IN BOOL bInbound)
{
	LONG ret;

	/* FIXME: Handle bInbound correctly */

	if (bInbound &&
	    (pUnsafeEntries == NULL || cEntries == 0))
	{
		return 0;
	}

	_SEH_TRY
	{
		switch(iFunc)
		{
			case GdiPalAnimate:
				ProbeForRead(pUnsafeEntries, cEntries * sizeof(PALETTEENTRY), 1);
				ret = IntAnimatePalette((HPALETTE)hObj, iStart, cEntries, pUnsafeEntries);
				break;

			case GdiPalSetEntries:
				ProbeForRead(pUnsafeEntries, cEntries * sizeof(PALETTEENTRY), 1);
				ret = IntSetPaletteEntries((HPALETTE)hObj, iStart, cEntries, pUnsafeEntries);
				break;

			case GdiPalGetEntries:
				if (pUnsafeEntries)
				{
					ProbeForWrite(pUnsafeEntries, cEntries * sizeof(PALETTEENTRY), 1);
				}
				ret = IntGetPaletteEntries((HPALETTE)hObj, iStart, cEntries, pUnsafeEntries);
				break;

			case GdiPalGetSystemEntries:
				if (pUnsafeEntries)
				{
					ProbeForWrite(pUnsafeEntries, cEntries * sizeof(PALETTEENTRY), 1);
				}
				ret = IntGetSystemPaletteEntries((HDC)hObj, iStart, cEntries, pUnsafeEntries);
				break;

			case GdiPalSetColorTable:
				ProbeForRead(pUnsafeEntries, cEntries * sizeof(PALETTEENTRY), 1);
				ret = IntSetDIBColorTable((HDC)hObj, iStart, cEntries, (RGBQUAD*)pUnsafeEntries);
				break;

			case GdiPalGetColorTable:
				if (pUnsafeEntries)
				{
					ProbeForWrite(pUnsafeEntries, cEntries * sizeof(PALETTEENTRY), 1);
				}
				ret = IntGetDIBColorTable((HDC)hObj, iStart, cEntries, (RGBQUAD*)pUnsafeEntries);
				break;

			default:
				ret = 0;
		}
	}
	_SEH_HANDLE
	{
		ret = 0;
	}
	_SEH_END

	return ret;
}

/* EOF */
