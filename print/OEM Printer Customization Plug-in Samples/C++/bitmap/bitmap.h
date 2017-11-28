//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//  ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//  PARTICULAR PURPOSE.
//
//  Copyright  1997 - 2003  Microsoft Corporation.  All Rights Reserved.
//
//  FILE:   BITMAP.H
//    
//
//  PURPOSE:    Define the COemPDEV class which stores the private
//          PDEV for the driver.
//


#pragma once

#define OEM_SIGNATURE   'MSFT'
#define OEM_VERSION     0x00000001L

#define RGB_BLACK   RGB(0, 0, 0)
#define RGB_WHITE   RGB(255, 255, 255)

class COemPDEV
{
public:
    COemPDEV(void) :
        m_pfnDrvEndDoc(NULL),
        pBufStart(NULL),
        dwBufSize(0),
        cPalColors(0),
        hpalDefault(),
        prgbq(NULL),
        cbHeaderOffBits(0),
        bHeadersFilled(FALSE),
        bColorTable(FALSE)  
    {
        memset(&bmFileHeader, 0, sizeof(bmFileHeader));
        memset(&bmInfoHeader, 0, sizeof(bmInfoHeader));
    }

    virtual ~COemPDEV(void)
    {
    }

    void InitializeDDITable(DRVENABLEDATA* pded)
    {
        UINT   iDrvFn;
        UINT   cDrvFn = pded->c;
        PDRVFN pDrvFn = pded->pdrvfn;

        for (iDrvFn = 0; iDrvFn < cDrvFn; ++iDrvFn, ++pDrvFn)
        {
            if (pDrvFn->iFunc == INDEX_DrvEndDoc)
            {
                m_pfnDrvEndDoc = (PFN_DrvEndDoc)(pDrvFn->pfn);
            }
        }
    }

public:

    PFN_DrvEndDoc       m_pfnDrvEndDoc;     // Unidrv function to call back into from the DDI hook
    PBYTE               pBufStart;          // Pointer to start of buffer to hold the bitmap data
    DWORD               dwBufSize;          // Buffer size
    BITMAPFILEHEADER    bmFileHeader;       // BitmapFileHeader for each dump
    BITMAPINFOHEADER    bmInfoHeader;       // BitmapInfoHeader for each dump
    int                 cPalColors;         // Count of colors in palette.
    HPALETTE            hpalDefault;            // Default palette to pass to GDI.
    _Field_size_(cPalColors) RGBQUAD * prgbq; // Color table
    INT                 cbHeaderOffBits;    // Offset to bits in the file
    BOOL                bHeadersFilled;     // Flag to indicate if the header stuff has been filled
    BOOL                bColorTable;        // Flag to indicate if color table needs to be dumped

};

typedef COemPDEV* POEMPDEV;

BOOL bGrowBuffer(POEMPDEV, DWORD);      // Function to enlarge the buffer for holding the bitmap data
VOID vFreeBuffer(POEMPDEV);             // Function to free the buffer for holding the bitmap data
BOOL bFillColorTable(POEMPDEV);         // Function to fill the color table for the bitmap data
