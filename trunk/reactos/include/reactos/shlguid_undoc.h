/*
 * Copyright (C) 1999 Juergen Schmied
 * Copyright (C) 2009 Andrew Hill
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

#ifndef __SHLGUID_UNDOC_H
#define __SHLGUID_UNDOC_H

DEFINE_GUID(CLSID_RebarBandSite,         0xECD4FC4D, 0x521C, 0x11D0, 0xB7, 0x92, 0x00, 0xA0, 0xC9, 0x03, 0x12, 0xE1);
DEFINE_GUID(CLSID_BandSiteMenu,          0xECD4FC4E, 0x521C, 0x11D0, 0xB7, 0x92, 0x00, 0xA0, 0xC9, 0x03, 0x12, 0xE1);
DEFINE_GUID(IID_IBandSiteHelper,         0xD1E7AFEA, 0x6A2E, 0x11D0, 0x8C, 0x78, 0x00, 0xC0, 0x4F, 0xD9, 0x18, 0xB4);
DEFINE_GUID(CLSID_PersonalStartMenu,     0x3F6953F0, 0x5359, 0x47FC, 0xBD, 0x99, 0x9F, 0x2C, 0xB9, 0x5A, 0x62, 0xFD);
DEFINE_GUID(IID_ITrayPriv,               0x4622AD10, 0xFF23, 0x11D0, 0x8D, 0x34, 0x00, 0xA0, 0xC9, 0x0F, 0x27, 0x19);
DEFINE_GUID(IID_ITrayPriv2,              0x9E83C057, 0xFF23, 0x6823, 0x1F, 0x4F, 0xBF, 0xA3, 0x74, 0x61, 0xD4, 0x0A);
DEFINE_GUID(IID_IShellMenu2,             0x6F51C646, 0x0EFE, 0x4370, 0x88, 0x2A, 0xC1, 0xF6, 0x1C, 0xB2, 0x7C, 0x3B);
DEFINE_GUID(IID_IWinEventHandler,        0xEA5F2D61, 0xE008, 0x11CF, 0x99, 0xCB, 0x00, 0xC0, 0x4F, 0xD6, 0x44, 0x97);
DEFINE_GUID(IID_IShellMenuAcc,           0xFAF6FE96, 0xCE5E, 0x11D1, 0x83, 0x71, 0x00, 0xC0, 0x4F, 0xD9, 0x18, 0xD0);
DEFINE_GUID(IID_IShellBrowserService,    0x1307EE17, 0xEA83, 0x49EB, 0x96, 0xB2, 0x3A, 0x28, 0xE2, 0xD7, 0x04, 0x8A);
DEFINE_GUID(IID_IShellFolderView,        0x37A378C0, 0xF82D, 0x11CE, 0xAE, 0x65, 0x08, 0x00, 0x2B, 0x2E, 0x12, 0x62);

DEFINE_GUID(SID_SProxyBrowser,             0x20C46561, 0x8491, 0x11CF, 0x96, 0x0C, 0x00, 0x80, 0xC7, 0xF4, 0xEE, 0x85);

// this class lives in shell32.dll
DEFINE_GUID(IID_IGlobalFolderSettings,     0xEF8AD2D3, 0xAE36, 0x11D1, 0xB2, 0xD2, 0x00, 0x60, 0x97, 0xDF, 0x8C, 0x11);
DEFINE_GUID(CLSID_GlobalFolderSettings,    0xEF8AD2D1, 0xAE36, 0x11D1, 0xB2, 0xD2, 0x00, 0x60, 0x97, 0xDF, 0x8C, 0x11);
DEFINE_GUID(IID_IRegTreeOptions,           0xAF4F6511, 0xF982, 0x11D0, 0x85, 0x95, 0x00, 0xAA, 0x00, 0x4C, 0xD6, 0xD8);
DEFINE_GUID(CLSID_CRegTreeOptions,         0xAF4F6510, 0xF982, 0x11D0, 0x85, 0x95, 0x00, 0xAA, 0x00, 0x4C, 0xD6, 0xD8);
DEFINE_GUID(IID_IExplorerToolbar,          0x8455F0C1, 0x158F, 0x11D0, 0x89, 0xAE, 0x00, 0xA0, 0xC9, 0x0A, 0x90, 0xAC);

// not registered, lives in browseui.dll
DEFINE_GUID(CLSID_BrowserBar,              0x9581015C, 0xD08E, 0x11D0, 0x8D, 0x36, 0x00, 0xA0, 0xC9, 0x2D, 0xBF, 0xE8);

DEFINE_GUID(CGID_DefViewFrame,             0x710EB7A1, 0x45ED, 0x11D0, 0x92, 0x4A, 0x00, 0x20, 0xAF, 0xC7, 0xAC, 0x4D);

// browseui.dll
DEFINE_GUID(CLSID_SH_AddressBand,          0x01E04581, 0x4EEE, 0x11D0, 0xBF, 0xE9, 0x00, 0xAA, 0x00, 0x5B, 0x43, 0x83);
DEFINE_GUID(CLSID_AddressEditBox,          0xA08C11D2, 0xA228, 0x11D0, 0x82, 0x5B, 0x00, 0xAA, 0x00, 0x5B, 0x43, 0x83);
DEFINE_GUID(IID_IAddressEditBox,           0xA08C11D1, 0xA228, 0x11D0, 0x82, 0x5B, 0x00, 0xAA, 0x00, 0x5B, 0x43, 0x83);

DEFINE_GUID(IID_IAddressBand,              0x106E86E1, 0x52B5, 0x11D0, 0xBF, 0xED, 0x00, 0xAA, 0x00, 0x5B, 0x43, 0x83);
DEFINE_GUID(CLSID_BrandBand,               0x22BF0C20, 0x6DA7, 0x11D0, 0xB3, 0x73, 0x00, 0xA0, 0xC9, 0x03, 0x49, 0x38);
DEFINE_GUID(SID_SBrandBand,                0x82A62DE8, 0x32AC, 0x4E4A, 0x99, 0x35, 0x90, 0x46, 0xC3, 0x78, 0xCF, 0x90);
DEFINE_GUID(CLSID_InternetToolbar,         0x5E6AB780, 0x7743, 0x11CF, 0xA1, 0x2B, 0x00, 0xAA, 0x00, 0x4A, 0xE8, 0x37);

DEFINE_GUID(CGID_PrivCITCommands,          0x67077B95, 0x4F9D, 0x11D0, 0xB8, 0x84, 0x00, 0xAA, 0x00, 0xB6, 0x01, 0x04);
DEFINE_GUID(CGID_Theater,                  0x0F12079C, 0xC193, 0x11D0, 0x8D, 0x49, 0x00, 0xC0, 0x4F, 0xC9, 0x9D, 0x61);
DEFINE_GUID(CGID_ShellBrowser,             0x3531F060, 0x22B3, 0x11D0, 0x96, 0x9E, 0x00, 0xAA, 0x00, 0xB6, 0x01, 0x04);

DEFINE_GUID(CLSID_SearchBand,              0x2559A1F0, 0x21D7, 0x11D4, 0xBD, 0xAF, 0x00, 0xC0, 0x4F, 0x60, 0xB9, 0xF0);
DEFINE_GUID(CLSID_TipOfTheDayBand,         0x4D5C8C25, 0xD075, 0x11D0, 0xB4, 0x16, 0x00, 0xC0, 0x4F, 0xB9, 0x03, 0x76);
DEFINE_GUID(CLSID_DiscussBand,             0xBDEADE7F, 0xC265, 0x11D0, 0xBC, 0xED, 0x00, 0xA0, 0xC9, 0x0A, 0xB5, 0x0F);
DEFINE_GUID(CLSID_SH_FavBand,              0xEFA24E61, 0xB078, 0x11D0, 0x89, 0xE4, 0x00, 0xC0, 0x4F, 0xC9, 0xE2, 0x6E);
DEFINE_GUID(CLSID_SH_HistBand,             0xEFA24E62, 0xB078, 0x11D0, 0x89, 0xE4, 0x00, 0xC0, 0x4F, 0xC9, 0xE2, 0x6E);
DEFINE_GUID(CLSID_ExplorerBand,            0xEFA24E64, 0xB078, 0x11D0, 0x89, 0xE4, 0x00, 0xC0, 0x4F, 0xC9, 0xE2, 0x6E);
DEFINE_GUID(CLSID_SH_SearchBand,           0x21569614, 0xB795, 0x46B1, 0x85, 0xF4, 0xE7, 0x37, 0xA8, 0xDC, 0x09, 0xAD);
DEFINE_GUID(CLSID_FileSearchBand,          0xC4EE31F3, 0x4768, 0x11D2, 0x5C, 0xBE, 0x00, 0xA0, 0xC9, 0xA8, 0x3D, 0xA1);
// missing ResearchBand

DEFINE_GUID(IID_IBandNavigate,             0x3697C30B, 0xCD88, 0x11D0, 0x8A, 0x3E, 0x00, 0xC0, 0x4F, 0xC9, 0xE2, 0x6E);
DEFINE_GUID(IID_INamespaceProxy,           0xCF1609EC, 0xFA4B, 0x4818, 0xAB, 0x01, 0x55, 0x64, 0x33, 0x67, 0xE6, 0x6D);
DEFINE_GUID(IID_IBandProxy,                0x208CE801, 0x754B, 0x11D0, 0x80, 0xCA, 0x00, 0xAA, 0x00, 0x5B, 0x43, 0x83);
DEFINE_GUID(CLSID_BandProxy,               0xF61FFEC1, 0x754F, 0x11D0, 0x80, 0xCA, 0x00, 0xAA, 0x00, 0x5B, 0x43, 0x83);
DEFINE_GUID(SID_IBandProxy,                0x80243AC1, 0x0569, 0x11D1, 0xA7, 0xAE, 0x00, 0x60, 0x97, 0xDF, 0x5B, 0xD4);
DEFINE_GUID(CLSID_ShellSearchExt,          0x169A0691, 0x8DF9, 0x11D1, 0xA1, 0xC4, 0x00, 0xC0, 0x4F, 0xD7, 0x5D, 0x13);

DEFINE_GUID(CLSID_CommonButtons,           0x1E79697E, 0x9CC5, 0x11D1, 0xA8, 0x3F, 0x00, 0xC0, 0x4F, 0xC9, 0x9D, 0x61);
DEFINE_GUID(CLSID_CCommonBrowser,          0xAF604EFE, 0x8897, 0x11D1, 0xB9, 0x44, 0x00, 0xA0, 0xC9, 0x03, 0x12, 0xE1);
DEFINE_GUID(CLSID_DeskBar,                 0xecd4fc4c, 0x521c, 0x11d0, 0xb7, 0x92, 0x00, 0xa0, 0xc9, 0x03, 0x12, 0xe1);
DEFINE_GUID(CLSID_DeskBarApp,              0x3ccf8a41, 0x5c85, 0x11d0, 0x97, 0x96, 0x00, 0xaa, 0x00, 0xb9, 0x0a, 0xdf);

DEFINE_GUID(CGID_BrandCmdGroup,            0x25019D8C, 0x9EE0, 0x45C0, 0x88, 0x3B, 0x97, 0x2D, 0x48, 0x32, 0x5E, 0x18);

DEFINE_GUID(IID_INSCTree,                  0x43A8F463, 0x4222, 0x11D2, 0xB6, 0x41, 0x00, 0x60, 0x97, 0xDF, 0x5B, 0xD4);
DEFINE_GUID(IID_INSCTree2,                 0x801C1AD5, 0xC47C, 0x428C, 0x97, 0xAF, 0xE9, 0x91, 0xE4, 0x85, 0x7D, 0x97);

DEFINE_GUID(IID_IInitializeObject,         0x4622AD16, 0xFF23, 0x11D0, 0x8D, 0x34, 0x00, 0xA0, 0xC9, 0x0F, 0x27, 0x19);
DEFINE_GUID(IID_IBanneredBar,              0x596A9A94, 0x013E, 0x11D1, 0x8D, 0x34, 0x00, 0xA0, 0xC9, 0x0F, 0x27, 0x19);

DEFINE_GUID(CLSID_StartMenu,               0x4622AD11, 0xFF23, 0x11D0, 0x8D, 0x34, 0x00, 0xA0, 0xC9, 0x0F, 0x27, 0x19);
DEFINE_GUID(CLSID_MenuBandSite,            0xE13EF4E4, 0xD2F2, 0x11D0, 0x98, 0x16, 0x00, 0xC0, 0x4F, 0xD9, 0x19, 0x72);
DEFINE_GUID(SHELL32_AdvtShortcutProduct,   0x9DB1186F, 0x40DF, 0x11D1, 0xAA, 0x8C, 0x00, 0xC0, 0x4F, 0xB6, 0x78, 0x63);
DEFINE_GUID(SHELL32_AdvtShortcutComponent, 0x9DB1186E, 0x40DF, 0x11D1, 0xAA, 0x8C, 0x00, 0xC0, 0x4F, 0xB6, 0x78, 0x63);
DEFINE_GUID(CLSID_OpenWithMenu,            0x09799AFB, 0xAD67, 0x11D1, 0xAB, 0xCD, 0x00, 0xC0, 0x4F, 0xC3, 0x09, 0x36);

DEFINE_GUID(CLSID_FontsFolderShortcut,     0xD20EA4E1, 0x3957, 0x11D2, 0xA4, 0x0B, 0x0C, 0x50, 0x20, 0x52, 0x41, 0x52);
DEFINE_GUID(CLSID_AdminFolderShortcut,     0xD20EA4E1, 0x3957, 0x11D2, 0xA4, 0x0B, 0x0C, 0x50, 0x20, 0x52, 0x41, 0x53);

DEFINE_GUID(CLSID_FolderOptions,           0x6DFD7C5C, 0x2451, 0x11D3, 0xA2, 0x99, 0x00, 0xC0, 0x4F, 0x8E, 0xF6, 0xAF);

DEFINE_GUID(CLSID_ShellFileDefExt,         0x21B22460, 0x3AEA, 0x1069, 0xA2, 0xDC, 0x08, 0x00, 0x2B, 0x30, 0x30, 0x9D);
DEFINE_GUID(CLSID_ShellDrvDefExt,          0x5F5295E0, 0x429F, 0x1069, 0xA2, 0xE2, 0x08, 0x00, 0x2B, 0x30, 0x30, 0x9D);
DEFINE_GUID(CLSID_ShellNetDefExt,          0x86422020, 0x42A0, 0x1069, 0xA2, 0xE5, 0x08, 0x00, 0x2B, 0x30, 0x30, 0x9D);

DEFINE_GUID(CLSID_ExeDropHandler, 		   0x86C86720, 0x42A0, 0x1069, 0xA2, 0xE8, 0x08, 0x00, 0x2B, 0x30, 0x30, 0x9D);

DEFINE_GUID(IID_IAugmentedShellFolder,     0x91EA3F8C, 0xC99B, 0x11D0, 0x98, 0x15, 0x00, 0xC0, 0x4F, 0xD9, 0x19, 0x72);
DEFINE_GUID(CLSID_MergedFolder,            0x26FDC864, 0xBE88, 0x46E7, 0x92, 0x35, 0x03, 0x2D, 0x8E, 0xA5, 0x16, 0x2E);


#define CGID_IExplorerToolbar IID_IExplorerToolbar
#define SID_IExplorerToolbar IID_IExplorerToolbar
#define SID_ITargetFrame2 IID_ITargetFrame2
#define SID_IWebBrowserApp IID_IWebBrowserApp
#define CGID_IDeskBand IID_IDeskBand
#define CGID_MenuBand CLSID_MenuBand
#define SID_STravelLogCursor IID_ITravelLogStg
#define SID_IBandSite IID_IBandSite
#define SID_IFolderView IID_IFolderView
#define SID_IShellBrowser IID_IShellBrowser

#endif // __SHLGUID_UNDOC_H
