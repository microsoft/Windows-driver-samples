/**************************************************************************
*
*  Copyright (c) 2003  Microsoft Corporation
*
*  Title: wiahelpers.h
*
*  Description: This contains the WIA driver class helper functions.
*
***************************************************************************/
#pragma once

#define NO_FIXED_FRAME 0xFFFFFFFF

typedef struct _FILM_FRAME {
    LONG    XPOS;
    LONG    YPOS;
    LONG    XEXTENT;
    LONG    YEXTENT;
} FILM_FRAME;

HRESULT MakeFullItemName(
    _In_    IWiaDrvItem *pParent,
    _In_    BSTR        bstrItemName,
    _Out_   BSTR        *pbstrFullItemName);

HRESULT CreateWIAChildItem(
    _In_            LPOLESTR    wszItemName,
    _In_            IWiaMiniDrv *pIWiaMiniDrv,
    _In_            IWiaDrvItem *pParent,
                    LONG        lItemFlags,
                    GUID        guidItemCategory,
   _Out_opt_        IWiaDrvItem **ppChild       = NULL,
   _In_opt_         PCWSTR      wszStoragePath = NULL);
HRESULT CreateWIAFlatbedItem(
    _In_    LPOLESTR    wszItemName,
    _In_    IWiaMiniDrv *pIWiaMiniDrv,
    _In_    IWiaDrvItem *pParent);

HRESULT CreateWIAFeederItem(
    _In_    LPOLESTR    wszItemName,
    _In_    IWiaMiniDrv *pIWiaMiniDrv,
    _In_    IWiaDrvItem *pParent);

HRESULT CreateWIAFilmItem(
    _In_    LPOLESTR    wszItemName,
    _In_    IWiaMiniDrv *pIWiaMiniDrv,
    _In_    IWiaDrvItem *pParent);

HRESULT CreateWIAStorageItem(
    _In_    LPOLESTR    wszItemName,
    _In_    IWiaMiniDrv *pIWiaMiniDrv,
    _In_    IWiaDrvItem *pParent,
    _In_    const WCHAR *wszStoragePath);

HRESULT InitializeRootItemProperties(
    _In_    BYTE        *pWiasContext);

HRESULT InitializeWIAItemProperties(
    _In_    BYTE        *pWiasContext,
    _In_    HINSTANCE   hInstance,
            UINT        uiResourceID);

HRESULT InitializeWIAStorageItemProperties(
    _In_    BYTE        *pWiasContext,
            BOOL        bRootItem,
            BOOL        bFolderItem);

HRESULT wiasGetDriverItemPrivateContext(
    _In_    BYTE *pWiasContext,
    _Out_   BYTE **ppWiaDriverItemContext);

HRESULT wiasGetAppItemParent(
    _In_    BYTE *pWiasContext,
    _Out_   BYTE **ppWiasContext);

LONG ConvertTo1000thsOfAnInch(
    LONG lPixelSize,
    LONG lResolution);

HRESULT GetBitmapHeaderFromBitmapData(
    _In_    Gdiplus::BitmapData *pGDIPlusBitmapData,
    _Out_ BITMAPINFOHEADER    *pBitmapInfoHeader);

HRESULT GetSelectionAreaRect(
    _In_    BYTE            *pWiasContext,
    _Out_   Gdiplus::Rect   *pRect);

HRESULT LockSelectionAreaOnBitmap(
    _In_    BYTE                    *pWiasContext,
    _In_    Gdiplus::Bitmap         *pBitmap,
    _Out_   Gdiplus::BitmapData     *pBitmapData,
    _In_    BITMAPINFOHEADER        *pbmih,
    _Outptr_result_maybenull_
            BYTE                    **ppBitmapBits);

void UnlockSelectionAreaOnBitmap(
    _In_    Gdiplus::Bitmap         *pBitmap,
    _In_    Gdiplus::BitmapData     *pBitmapData);

HRESULT GetTransferCallback(
    _In_        PMINIDRV_TRANSFER_CONTEXT       pmdtc,
    __callback  IWiaMiniDrvTransferCallback     **ppIWiaMiniDrvTransferCallback);

HRESULT AllocateTransferBuffer(
    _In_    BYTE                    *pWiasContext,
    _Out_   BYTE                    **ppBuffer,
    _In_    ULONG                    *pulBufferSize);

void FreeTransferBuffer(
    _In_    BYTE                    *pBuffer);

HRESULT GetFileExtensionFromPath(
    _In_    BSTR    bstrFullPath,
    _Out_   BSTR    *pbstrExtension);

bool IsProgrammableItem(
    _In_    BYTE    *pWiasContext);

void QueueWIAEvent(
    _In_    BYTE        *pWiasContext,
            const GUID  &guidWIAEvent);
