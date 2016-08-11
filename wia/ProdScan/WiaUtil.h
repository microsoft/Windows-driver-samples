/**************************************************************************
*
*  Copyright © Microsoft Corporation
*
*  File Title:  WiaUtil.h
*
*  Project:     Production Scanning Driver Sample
*
*  Description: This file contains various helper functions for the driver.
*
***************************************************************************/

#pragma once

HRESULT
MakeFullItemName(
    _In_ IWiaDrvItem* pParent,
    _In_ BSTR bstrItemName,
    _Out_ BSTR* pbstrFullItemName);

HRESULT
CreateWIAChildItem(
    _In_ LPOLESTR pszItemName,
    _In_ IWiaMiniDrv *pIWiaMiniDrv,
    _In_ IWiaDrvItem *pParent,
    LONG lItemFlags,
    GUID guidItemCategory,
    _Inout_opt_ IWiaDrvItem **ppChild = NULL);

HRESULT
wiasGetDriverItemPrivateContext(
    _In_ BYTE* pWiasContext,
    _Out_ BYTE** ppWiaDriverItemContext);

HRESULT AllocateTransferBuffer(
    _Outptr_result_bytebuffer_(*pulBufferSize) BYTE** ppBuffer,
    _Out_ ULONG* pulBufferSize);

void
FreeTransferBuffer(
    _In_opt_ BYTE* pBuffer);

void
QueueWIAEvent(
    _In_ BYTE* pWiasContext,
    const GUID& guidWIAEvent);

inline LONG
BytesPerLine(
    LONG lImageWidth,
    LONG lBitDepth)
{
    //
    // The number of bytes per line include the padding necessary to make each uncompressed
    // line (DIB or Raw) DWORD aligned. When the image data is compressed the number of bytes
    // per line calculated here describes the original uncompressed image:
    //
    return (((lImageWidth * lBitDepth) + 31) / 32) * 4;
};
