/**
 @file DXCaps.cpp

 @created 2006-04-06
 @edited  2006-05-06

 @maintainer Corey Taylor

 Copyright 2000-2006, Morgan McGuire.
 All rights reserved.
*/

#include "GLG3D/DXCaps.h"
#include "G3D/platform.h"
#include "G3D/RegistryUtil.h"
#include "G3D/System.h"

// This file is only used on Windows
#ifdef G3D_WINDOWS
#include <objbase.h>
#include <initguid.h>

namespace G3D {


uint32 DXCaps::version() {
    static uint32 dxVersion = 0;
    static bool init = false;

    if (init) {
        return dxVersion;
    }

    if ( RegistryUtil::valueExists("HKEY_LOCAL_MACHINE\\Software\\Microsoft\\DirectX", "Version") ) {
        String versionString;
        if ( RegistryUtil::readString("HKEY_LOCAL_MACHINE\\Software\\Microsoft\\DirectX", "Version", versionString) ) {
            char major[3], minor[3];
            if ( versionString.size() >= 7 ) {
                major[0] = versionString[2];
                major[1] = versionString[3];
                major[2] = '\0';

                minor[0] = versionString[5];
                minor[1] = versionString[6];
                minor[2] = '\0';

                dxVersion = (atoi(major) * 100) + atoi(minor);
            }
        }
    }

    init = true;

    return dxVersion;
}

// IDirectDraw2 interface pieces required for getVideoMemorySize()
DEFINE_GUID( CLSID_DirectDraw,                  0xD7B70EE0,0x4340,0x11CF,0xB0,0x63,0x00,0x20,0xAF,0xC2,0xCD,0x35 );
DEFINE_GUID( IID_IDirectDraw2,                  0xB3A6F3E0,0x2B43,0x11CF,0xA2,0xDE,0x00,0xAA,0x00,0xB9,0x33,0x56 );

/*
 * DDSCAPS
 */
typedef struct _DDSCAPS
{
    DWORD       dwCaps;         // capabilities of surface wanted
} DDSCAPS;

typedef DDSCAPS FAR* LPDDSCAPS;

#undef INTERFACE
#define INTERFACE IDirectDraw2
DECLARE_INTERFACE_( IDirectDraw2, IUnknown )
{
    /*** IUnknown methods ***/
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;
    /*** IDirectDraw methods ***/
    STDMETHOD(Compact)(THIS) PURE;
    STDMETHOD(CreateClipper)(THIS_ DWORD, LPVOID FAR*, IUnknown FAR * ) PURE;
    STDMETHOD(CreatePalette)(THIS_ DWORD, LPVOID, LPVOID FAR*, IUnknown FAR * ) PURE;
    STDMETHOD(CreateSurface)(THIS_  LPVOID, LPVOID FAR *, IUnknown FAR *) PURE;
    STDMETHOD(DuplicateSurface)( THIS_ LPVOID, LPVOID FAR * ) PURE;
    STDMETHOD(EnumDisplayModes)( THIS_ DWORD, LPVOID, LPVOID, LPVOID ) PURE;
    STDMETHOD(EnumSurfaces)(THIS_ DWORD, LPVOID, LPVOID,LPVOID ) PURE;
    STDMETHOD(FlipToGDISurface)(THIS) PURE;
    STDMETHOD(GetCaps)( THIS_ LPVOID, LPVOID) PURE;
    STDMETHOD(GetDisplayMode)( THIS_ LPVOID) PURE;
    STDMETHOD(GetFourCCCodes)(THIS_  LPDWORD, LPDWORD ) PURE;
    STDMETHOD(GetGDISurface)(THIS_ LPVOID FAR *) PURE;
    STDMETHOD(GetMonitorFrequency)(THIS_ LPDWORD) PURE;
    STDMETHOD(GetScanLine)(THIS_ LPDWORD) PURE;
    STDMETHOD(GetVerticalBlankStatus)(THIS_ LPBOOL ) PURE;
    STDMETHOD(Initialize)(THIS_ GUID FAR *) PURE;
    STDMETHOD(RestoreDisplayMode)(THIS) PURE;
    STDMETHOD(SetCooperativeLevel)(THIS_ HWND, DWORD) PURE;
    STDMETHOD(SetDisplayMode)(THIS_ DWORD, DWORD,DWORD, DWORD, DWORD) PURE;
    STDMETHOD(WaitForVerticalBlank)(THIS_ DWORD, HANDLE ) PURE;
    /*** Added in the v2 interface ***/
    STDMETHOD(GetAvailableVidMem)(THIS_ LPDDSCAPS, LPDWORD, LPDWORD) PURE;
};

/*
 * Indicates that this surface exists in video memory.
 */
#define DDSCAPS_VIDEOMEMORY                     0x00004000l

/*
 * Indicates that a video memory surface is resident in true, local video
 * memory rather than non-local video memory. If this flag is specified then
 * so must DDSCAPS_VIDEOMEMORY. This flag is mutually exclusive with
 * DDSCAPS_NONLOCALVIDMEM.
 */
#define DDSCAPS_LOCALVIDMEM                     0x10000000l


uint64 DXCaps::videoMemorySize() {
    static DWORD totalVidSize = 0;
    static bool init = false;

    if (init) {
        return totalVidSize;
    }

    CoInitialize(NULL);
    IDirectDraw2* ddraw = NULL;

    DWORD freeVidSize  = 0;

    if ( !FAILED(CoCreateInstance( CLSID_DirectDraw, NULL, CLSCTX_INPROC_SERVER, IID_IDirectDraw2, reinterpret_cast<LPVOID*>(&ddraw))) ) {

        ddraw->Initialize(0);

        DDSCAPS ddsCaps;
        System::memset(&ddsCaps, 0, sizeof(DDSCAPS));

        ddsCaps.dwCaps = DDSCAPS_VIDEOMEMORY | DDSCAPS_LOCALVIDMEM;

        if ( FAILED(ddraw->GetAvailableVidMem(&ddsCaps, &totalVidSize, &freeVidSize)) ) {
            totalVidSize = 0;
            freeVidSize  = 0;
        }

        ddraw->Release();
    }

    CoUninitialize();
    init = true;

    return totalVidSize;
}


} // namespace G3D

#endif // G3D_WINDOWS
