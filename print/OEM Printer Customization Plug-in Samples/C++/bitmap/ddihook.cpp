//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//  ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//  PARTICULAR PURPOSE.
//
//  Copyright  1998 - 2003  Microsoft Corporation.  All Rights Reserved.
//
//  FILE:    DDIHook.cpp
//    
//
//  PURPOSE:  Implementation of DDI Hook OEMEndDoc. This function
//          dumps the buffered bitmap data out. 
//


#include "precomp.h"
#include "bitmap.h"
#include "debug.h"

// This indicates to Prefast that this is a usermode driver file.
_Analysis_mode_(_Analysis_code_type_user_driver_);

//
// Function prototype is defined in printoem.h
//
BOOL APIENTRY
OEMEndDoc(
    SURFOBJ     *pso,
    FLONG       fl
    )

/*++

Routine Description:

    Implementation of DDI hook for DrvEndDoc.

    DrvEndDoc is called by GDI when it has finished 
    sending a document to the driver for rendering.
    
    Please refer to DDK documentation for more details.

    This particular implementation of OEMEndDoc performs
    the following operations:
    - Dump the bitmap file header
    - Dump the bitmap info header
    - Dump the color table if one exists
    - Dump the buffered bitmap data
    - Free the memory for the data buffers

Arguments:

    pso - Defines the surface object
    flags - A set of flag bits

Return Value:

    TRUE if successful, FALSE if there is an error

--*/

{
    PDEVOBJ pDevObj = (PDEVOBJ)pso->dhpdev;
    POEMPDEV pOemPDEV = (POEMPDEV)pDevObj->pdevOEM;
    DWORD dwWritten;
    INT cScans;

    if (pOemPDEV->pBufStart)
    {
        // Fill BitmapFileHeader
        //
        DWORD dwTotalBytes = pOemPDEV->cbHeaderOffBits + pOemPDEV->bmInfoHeader.biSizeImage;        // File size
    
        pOemPDEV->bmFileHeader.bfType = 0x4d42;     // Signature = 'BM'
        pOemPDEV->bmFileHeader.bfSize = dwTotalBytes;  // Bytes in whole file.
        pOemPDEV->bmFileHeader.bfReserved1 = 0;
        pOemPDEV->bmFileHeader.bfReserved2 = 0;
        pOemPDEV->bmFileHeader.bfOffBits   = pOemPDEV->cbHeaderOffBits; // Offset to bits in file.

        if (pOemPDEV->bColorTable)
            pOemPDEV->bmFileHeader.bfOffBits += pOemPDEV->cPalColors * sizeof(ULONG);

        // Num of scanlines
        //
        cScans = pOemPDEV->bmInfoHeader.biHeight;

        // Flip the biHeight member so that it denotes top-down bitmap 
        //
        pOemPDEV->bmInfoHeader.biHeight = cScans * -1;

        // Dump headers first
        //
        dwWritten = pDevObj->pDrvProcs->DrvWriteSpoolBuf(pDevObj, (void*)&(pOemPDEV->bmFileHeader), sizeof(BITMAPFILEHEADER));
        dwWritten = pDevObj->pDrvProcs->DrvWriteSpoolBuf(pDevObj, (void*)&(pOemPDEV->bmInfoHeader), sizeof(BITMAPINFOHEADER));
        if (pOemPDEV->bColorTable)
        {
            dwWritten = pDevObj->pDrvProcs->DrvWriteSpoolBuf(pDevObj, pOemPDEV->prgbq, pOemPDEV->cPalColors * sizeof(ULONG));
            LocalFree(pOemPDEV->prgbq);
        }

        // Dump the data now
        //
        dwWritten = pDevObj->pDrvProcs->DrvWriteSpoolBuf(pDevObj, pOemPDEV->pBufStart, pOemPDEV->bmInfoHeader.biSizeImage);

        // Free memory for the data buffers
        //
        vFreeBuffer(pOemPDEV);
    }
    
    // Punt call back to UNIDRV.
    //
    return (pOemPDEV->m_pfnDrvEndDoc)(pso, fl);
}

