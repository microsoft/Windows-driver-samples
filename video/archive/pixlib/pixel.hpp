/*==========================================================================;
 *
 *  Copyright (C) 1999-2002 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       pixel.hpp
 *  Content:    Utility class for working with pixel formats
 *
 *
 ***************************************************************************/

#ifdef __cplusplus

#ifndef __PIXEL_HPP_CPP__
#define __PIXEL_HPP_CPP__

#ifndef DXPIXELVER
#define DXPIXELVER 100
#endif

#ifndef DXGASSERT
#define DXGASSERT( x ) _Analysis_assume_( x );
#endif

struct IHVFormatInfo
{
    D3DFORMAT       m_Format;
    DWORD           m_BPP;
    IHVFormatInfo  *m_pNext;
};

//2GB max allocation size
#define MAXALLOCSIZE 0x80000000

// This is a utility class that implements useful helpers for
// allocating and accessing various pixel formats. All methods
// are static and hence should be accessed as follows:
//  e.g. CPixel::LockOffset(...)
//

class CPixel
{
public:
    // Allocate helpers

    // Determine the amount of memory that is needed to
    // allocate various things..
    static UINT ComputeSurfaceSize(UINT            cpWidth,
                                   UINT            cpHeight,
                                   D3DFORMAT       Format);

    static UINT ComputeVolumeSize(UINT             cpWidth,
                                  UINT             cpHeight,
                                  UINT             cpDepth,
                                  D3DFORMAT        Format);


    static UINT ComputeMipMapSize(UINT             cpWidth,
                                  UINT             cpHeight,
                                  UINT             cLevels,
                                  D3DFORMAT        Format);

    static UINT ComputeMipVolumeSize(UINT          cpWidth,
                                     UINT          cpHeight,
                                     UINT          cpDepth,
                                     UINT          cLevels,
                                     D3DFORMAT     Format);

    static BOOL ComputeSurfaceSizeChecked(UINT            cpWidth,
                                          UINT            cpHeight,
                                          D3DFORMAT       Format,
                                          UINT            *pSize);

    static BOOL ComputeVolumeSizeChecked(UINT             cpWidth,
                                         UINT             cpHeight,
                                         UINT             cpDepth,
                                         D3DFORMAT        Format,
                                         UINT             *pSize);


    static BOOL ComputeMipMapSizeChecked(UINT             cpWidth,
                                         UINT             cpHeight,
                                         UINT             cLevels,
                                         D3DFORMAT        Format,
                                         UINT             *pSize);

    static BOOL ComputeMipVolumeSizeChecked(UINT          cpWidth,
                                            UINT          cpHeight,
                                            UINT          cpDepth,
                                            UINT          cLevels,
                                            D3DFORMAT     Format,
                                            UINT          *pSize);


    // Lock helpers

    // Given a surface desc, a level, and pointer to
    // bits (pBits in the LockedRectData) and a sub-rect,
    // this will fill in the pLockedRectData structure
    static void ComputeMipMapOffset(const D3DSURFACE_DESC  *pDescTopLevel,
                                    UINT                    iLevel,
                                    BYTE                   *pBits,
                                    CONST RECT             *pRect,
                                    D3DLOCKED_RECT         *pLockedRectData);

    // MipVolume version of ComputeMipMapOffset
    static void ComputeMipVolumeOffset(const D3DVOLUME_DESC  *pDescTopLevel,
                                       UINT                   iLevel,
                                       BYTE                  *pBits,
                                       CONST D3DBOX          *pBox,
                                       D3DLOCKED_BOX         *pLockedBoxData);

    // Surface version of ComputeMipMapOffset
    static void ComputeSurfaceOffset(const D3DSURFACE_DESC  *pDesc,
                                     BYTE                   *pBits,
                                     CONST RECT             *pRect,
                                     D3DLOCKED_RECT         *pLockedRectData);

    // Pixel Stride will return negative for DXT formats
    // Call AdjustForDXT to work with things at the block level
    static UINT ComputePixelStride(D3DFORMAT Format);

    // This will adjust cbPixel
    // to pixels per block; and width and height will
    // be adjusted to pixels. Assumes the IsDXT(cbPixel).
    static void AdjustForDXT(UINT *pcpWidth,
                             UINT *pcpHeight,
                             UINT *pcbPixel);

    // returns TRUE if cbPixel is "negative" i.e. DXT/V group
    static BOOL IsDXT(UINT cbPixel);

    // returns TRUE if format is one of the DXT/V group
    static BOOL IsDXT(D3DFORMAT Format);

    static UINT BytesPerPixel(D3DFORMAT Format);

    // Register format for later lookup
    static HRESULT Register(D3DFORMAT Format, DWORD BPP);

    // Cleanup registry
    static void Cleanup();

    static UINT ComputeSurfaceStride(UINT cpWidth, UINT cbPixel);

    static BOOL ComputeSurfaceStrideChecked(UINT cpWidth, UINT cbPixel, UINT *pStride);


private:
    // Internal functions

    static UINT ComputeSurfaceSize(UINT            cpWidth,
                                   UINT            cpHeight,
                                   UINT            cbPixel);

    static BOOL ComputeSurfaceSizeChecked(UINT            cpWidth,
                                          UINT            cpHeight,
                                          UINT            cbPixel,
                                          UINT            *pSize);


    static IHVFormatInfo *m_pFormatList;

}; // CPixel


#undef DPF_MODNAME
#define DPF_MODNAME "CPixel::ComputeSurfaceOffset"

inline void CPixel::ComputeSurfaceOffset(const D3DSURFACE_DESC  *pDesc,
                                         BYTE                   *pBits,
                                         CONST RECT             *pRect,
                                         D3DLOCKED_RECT         *pLockedRectData)
{
    ComputeMipMapOffset(pDesc, 0, pBits, pRect, pLockedRectData);
} // ComputeSurfaceOffset


#undef DPF_MODNAME
#define DPF_MODNAME "CPixel::ComputeSurfaceSize"

inline UINT CPixel::ComputeSurfaceSize(UINT            cpWidth,
                                       UINT            cpHeight,
                                       D3DFORMAT       Format)
{
    UINT uSize = 0; // initialize to zero to keep prefix happy win7: 185026
    ComputeSurfaceSizeChecked(cpWidth, cpHeight, Format, &uSize);
    return uSize;
} // ComputeSurfaceSize

#undef DPF_MODNAME
#define DPF_MODNAME "CPixel::ComputeSurfaceSizeChecked"

inline BOOL CPixel::ComputeSurfaceSizeChecked(UINT            cpWidth,
                                              UINT            cpHeight,
                                              D3DFORMAT       Format,
                                              UINT           *uSize)
{
    UINT cbPixel = ComputePixelStride(Format);

    // Adjust pixel->block if necessary
    BOOL isDXT = IsDXT(cbPixel);
    if (isDXT)
    {
        AdjustForDXT(&cpWidth, &cpHeight, &cbPixel);
    }
#if (DXPIXELVER > 8)
    else
    if (Format == D3DFMT_A1)
    {
        cpWidth = (cpWidth + 7) >> 3;
        cbPixel = 1;
    }
#endif

    return ComputeSurfaceSizeChecked(cpWidth,
                                     cpHeight,
                                     cbPixel,
                                     uSize);
} // ComputeSurfaceSize


#undef DPF_MODNAME
#define DPF_MODNAME "CPixel::AdjustForDXT"
inline void CPixel::AdjustForDXT(UINT *pcpWidth,
                                 UINT *pcpHeight,
                                 UINT *pcbPixel)
{
    DXGASSERT(pcbPixel);
    DXGASSERT(pcpWidth);
    DXGASSERT(pcpHeight);
    DXGASSERT(IsDXT(*pcbPixel));

    // Adjust width and height for DXT formats to be in blocks
    // instead of pixels. Blocks are 4x4 pixels.
    *pcpWidth  = (*pcpWidth  + 3) / 4;
    *pcpHeight = (*pcpHeight + 3) / 4;

    // Negate the pcbPixel to determine bytes per block
    *pcbPixel *= (UINT)-1;

    // We only know of two DXT formats right now...
    DXGASSERT(*pcbPixel == 8 || *pcbPixel == 16);

} // CPixel::AdjustForDXT

#undef DPF_MODNAME
#define DPF_MODNAME "CPixel::ComputeVolumeSize"

inline UINT CPixel::ComputeVolumeSize(UINT             cpWidth,
                                      UINT             cpHeight,
                                      UINT             cpDepth,
                                      D3DFORMAT        Format)
{
    UINT cbPixel = ComputePixelStride(Format);

    if (IsDXT(cbPixel))
    {
            AdjustForDXT(&cpWidth, &cpHeight, &cbPixel);
    }

    return cpDepth * ComputeSurfaceSize(cpWidth, cpHeight, cbPixel);
} // ComputeVolumeSize


#undef DPF_MODNAME
#define DPF_MODNAME "CPixel::IsDXT(cbPixel)"

// returns TRUE if cbPixel is "negative"
inline BOOL CPixel::IsDXT(UINT cbPixel)
{
    if (((INT)cbPixel) < 0)
        return TRUE;
    else
        return FALSE;
} // IsDXT

#undef DPF_MODNAME
#define DPF_MODNAME "CPixel::IsDXT(format)"

// returns TRUE if this is a linear format
// i.e. DXT or DXV
inline BOOL CPixel::IsDXT(D3DFORMAT Format)
{
    // CONSIDER: This is a duplication of Requires4x4 function
    switch (Format)
    {
        // normal DXTs
    case D3DFMT_DXT1:
    case D3DFMT_DXT2:
    case D3DFMT_DXT3:
    case D3DFMT_DXT4:
    case D3DFMT_DXT5:
        return TRUE;
    }

    return FALSE;
} // IsDXT


#endif // __PIXEL_HPP_CPP__

#else

#ifndef __PIXEL_HPP_C__
#define __PIXEL_HPP_C__

UINT CPixel__ComputeSurfaceSize(UINT            cpWidth,
                                UINT            cpHeight,
                                D3DFORMAT       Format);

UINT CPixel__ComputeVolumeSize(UINT             cpWidth,
                               UINT             cpHeight,
                               UINT             cpDepth,
                               D3DFORMAT        Format);

UINT CPixel__ComputeMipMapSize(UINT             cpWidth,
                               UINT             cpHeight,
                               UINT             cLevels,
                               D3DFORMAT        Format);

UINT CPixel__ComputeMipVolumeSize(UINT          cpWidth,
                                  UINT          cpHeight,
                                  UINT          cpDepth,
                                  UINT          cLevels,
                                  D3DFORMAT     Format);

void CPixel__ComputeMipMapOffset(const D3DSURFACE_DESC  *pDescTopLevel,
                                 UINT                    iLevel,
                                 BYTE                   *pBits,
                                 CONST RECT             *pRect,
                                 D3DLOCKED_RECT         *pLockedRectData);

void CPixel__ComputeMipVolumeOffset(const D3DVOLUME_DESC  *pDescTopLevel,
                                    UINT                   iLevel,
                                    BYTE                  *pBits,
                                    CONST D3DBOX          *pBox,
                                    D3DLOCKED_BOX         *pLockedBoxData);

void CPixel__ComputeSurfaceOffset(const D3DSURFACE_DESC  *pDesc,
                                  BYTE                   *pBits,
                                  CONST RECT             *pRect,
                                  D3DLOCKED_RECT         *pLockedRectData);

UINT CPixel__ComputePixelStride(D3DFORMAT Format);

void CPixel__AdjustForDXT(UINT *pcpWidth,
                          UINT *pcpHeight,
                          UINT *pcbPixel);

BOOL CPixel__IsDXT(D3DFORMAT Format);

UINT CPixel__BytesPerPixel(D3DFORMAT Format);

HRESULT CPixel__Register(D3DFORMAT Format, DWORD BPP);

void CPixel__Cleanup();

UINT CPixel__ComputeSurfaceStride(UINT cpWidth, UINT cbPixel);


#endif // __PIXEL_HPP_C__

#endif


