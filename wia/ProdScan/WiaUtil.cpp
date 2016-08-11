/**************************************************************************
*
*  Copyright © Microsoft Corporation
*
*  File Title:  WiaUtil.cpp
*
*  Project:     Production Scanner Driver Sample
*
*  Description: This file contains implementation of helper functions
*               declared in wiautil.h
*
***************************************************************************/

#include "stdafx.h"

/**************************************************************************\
*
* This function creates a full WIA item name from a given WIA item name.
* The new full item name is created by concatenating the WIA item name
* with the parent's full item name.
*
* (e.g. 0000\Root + Flatbed = 0000\Root\Flatbed)
*
*
* Parameters:
*
*  pParent           - IWiaDrvItem interface of the parent WIA driver item
*  bstrItemName      - Name of the WIA item
*  pbstrFullItemName - Returned full item name.
*
* Return Value:
*
*    S_OK or a standard COM error code
*
\**************************************************************************/

HRESULT MakeFullItemName(
    _In_ IWiaDrvItem* pParent,
    _In_ BSTR bstrItemName,
    _Out_ BSTR* pbstrFullItemName)
{
    HRESULT hr = S_OK;

    if (pParent && bstrItemName && pbstrFullItemName)
    {
        BSTR bstrParentFullItemName = NULL;
        hr = pParent->GetFullItemName(&bstrParentFullItemName);
        if (SUCCEEDED(hr))
        {
            WCHAR wFullItemName[MAX_PATH * 2] = {};

            hr = StringCbPrintf(wFullItemName, sizeof(wFullItemName), TEXT("%ws\\%ws"), bstrParentFullItemName, bstrItemName);
            if (SUCCEEDED(hr))
            {
                *pbstrFullItemName = SysAllocString(wFullItemName);
                if (*pbstrFullItemName)
                {
                    hr = S_OK;
                }
                else
                {
                    hr = E_OUTOFMEMORY;
                    WIAEX_ERROR((g_hInst, "Failed to allocate memory for BSTR full item name, hr = 0x%08X", hr));
                }
            }
            else
            {
                WIAEX_ERROR((g_hInst, "Failed to allocate memory for BSTR full item name, hr = 0x%08X", hr));
            }

            SysFreeString(bstrParentFullItemName);
            bstrParentFullItemName = NULL;
        }
        else
        {
            WIAEX_ERROR((g_hInst, "Failed to produce the full item name, hr = 0x%08X", hr));
        }
    }
    else
    {
        hr = E_INVALIDARG;
        WIAEX_ERROR((g_hInst, "Invalid parameter, hr = 0x%08X", hr));
    }
    return hr;
}

/**************************************************************************\
*
* This function creates a WIA child item
*
* Parameters:
*
*    wszItemName      - Item name
*    pIWiaMiniDrv     - WIA minidriver interface
*    pParent          - Parent's WIA driver item interface
*    lItemFlags       - Item flags
*    guidItemCategory - Item category
*    ppChild          - Pointer to the newly created child item
*
* Return Value:
*
*    S_OK or a standard COM error code
*
\**************************************************************************/

HRESULT CreateWIAChildItem(
    _In_ LPOLESTR pszItemName,
    _In_ IWiaMiniDrv *pIWiaMiniDrv,
    _In_ IWiaDrvItem *pParent,
    LONG lItemFlags,
    GUID guidItemCategory,
    _Inout_opt_ IWiaDrvItem **ppChild)
{
    UNREFERENCED_PARAMETER(guidItemCategory);

    HRESULT hr = E_INVALIDARG;

    if (pszItemName && pIWiaMiniDrv && pParent)
    {
        BSTR bstrItemName = SysAllocString(pszItemName);
        BSTR bstrFullItemName = NULL;
        IWiaDrvItem *pIWiaDrvItem = NULL;

        if (bstrItemName)
        {
            hr = MakeFullItemName(pParent, bstrItemName, &bstrFullItemName);
            if (SUCCEEDED(hr))
            {
                WIA_DRIVER_ITEM_CONTEXT *pWiaDriverItemContext = NULL;
                hr = wiasCreateDrvItem(lItemFlags,
                    bstrItemName,
                    bstrFullItemName,
                    pIWiaMiniDrv,
                    sizeof(WIA_DRIVER_ITEM_CONTEXT),
                    (BYTE **)&pWiaDriverItemContext,
                    &pIWiaDrvItem);

                if (SUCCEEDED(hr))
                {
                    //
                    // Initialize the item context data:
                    //
                    memset(pWiaDriverItemContext, 0, sizeof(WIA_DRIVER_ITEM_CONTEXT));
                    pWiaDriverItemContext->m_pUploadedImage = NULL;

                    hr = pIWiaDrvItem->AddItemToFolder(pParent);
                    if (FAILED(hr))
                    {
                        WIAEX_ERROR((g_hInst, "Failed to add the new WIA item (%ws) to the specified parent item, hr = 0x%08X",
                            bstrFullItemName, hr));
                        pIWiaDrvItem->Release();
                        pIWiaDrvItem = NULL;
                    }

                    if (SUCCEEDED(hr))
                    {
                        //
                        // If a child iterface pointer parameter was specified, then the caller
                        // expects to have the newly created child interface pointer returned to
                        // them (do not release the newly created item in this case).
                        //

                        if (ppChild)
                        {
                            *ppChild = pIWiaDrvItem;
                            pIWiaDrvItem = NULL;
                        }
                        else if (pIWiaDrvItem)
                        {
                            //
                            // The newly created child has been added to the tree, and is no longer
                            // needed.  Release it.
                            //

                            pIWiaDrvItem->Release();
                            pIWiaDrvItem = NULL;
                        }
                    }
                }
                else
                {
                    WIAEX_ERROR((g_hInst, "Failed to create the new WIA driver item, hr = 0x%08X", hr));
                }

                SysFreeString(bstrItemName);
                bstrItemName = NULL;
                SysFreeString(bstrFullItemName);
                bstrFullItemName = NULL;
            }
            else
            {
                WIAEX_ERROR((g_hInst, "Failed to create the new WIA item's full item name, hr = 0x%08X", hr));
            }
        }
        else
        {
            //
            // Failed to allocate memory for bstrItemName.
            //
            hr = E_OUTOFMEMORY;
            WIAEX_ERROR((g_hInst, "Failed to allocate memory for BSTR storage item name"));
        }

    }
    else
    {
        WIAEX_ERROR((g_hInst, "Invalid parameter"));
    }
    return hr;
}

/**************************************************************************\
*
* This function returns the WIA driver item context data stored with the
* driver item. The context is initialized and stored at WIA item creation.
* See CreateWIAChildItem function.
*
* Parameters:
*
*     pWiasContext           - Pointer to the WIA item context
*     ppWiaDriverItemContext - Pointer to the WIA driver item context data
*
* Return Value:
*
*    S_OK or a standard COM error code
*
\**************************************************************************/

HRESULT wiasGetDriverItemPrivateContext(
    _In_ BYTE* pWiasContext,
    _Out_ BYTE** ppWiaDriverItemContext)
{
    HRESULT hr = E_INVALIDARG;

    if (pWiasContext && ppWiaDriverItemContext)
    {
        IWiaDrvItem *pIWiaDrvItem = NULL;

        hr = wiasGetDrvItem(pWiasContext, &pIWiaDrvItem);

        if (SUCCEEDED(hr))
        {
            hr = pIWiaDrvItem->GetDeviceSpecContext(ppWiaDriverItemContext);

            //
            // The caller will handle the failure case.  A failure, may mean that
            // the the WIA item does not have a private device specific context
            // stored.  This is OK, because is is not required.
            //
        }
        else
        {
            WIAEX_ERROR((g_hInst, "Failed to get the WIA driver item from the application item, hr = 0x%08X", hr));
        }
    }
    else
    {
        WIAEX_ERROR((g_hInst, "Invalid parameter"));
    }
    return hr;
}


/**************************************************************************\
*
* This function allocates a buffer to be used during data transfers.
* FreeTransferBuffer should be called to free the memory allocated by
* this function.
*
* Parameters:
*
*     ppBuffer      - Pointer to the allocated buffer; the caller must
*                     call FreeTransferBuffer() when finished with this buffer
*     pulBufferSize - Size of the buffer allocated
*
* Return Value:
*
*    S_OK or a standard COM error code
*
\**************************************************************************/

HRESULT AllocateTransferBuffer(
    _Outptr_result_bytebuffer_(*pulBufferSize) BYTE** ppBuffer,
    _Out_ ULONG* pulBufferSize)
{
    HRESULT hr = S_OK;

    if (ppBuffer && pulBufferSize)
    {
        //
        // Set the buffer size to DEFAULT_BUFFER_SIZE
        //
        *pulBufferSize  = DEFAULT_BUFFER_SIZE;

        //
        // Allocate the memory
        //
        *ppBuffer = (BYTE*) CoTaskMemAlloc(*pulBufferSize);
        if (*ppBuffer)
        {
            hr = S_OK;
        }
        else
        {
            hr = E_OUTOFMEMORY;
            WIAEX_ERROR((g_hInst, "Failed to allocate memory for transfer buffer, hr = 0x%08X", hr));
        }
    }
    else
    {
        WIAEX_ERROR((g_hInst, "Invalid parameter"));
        hr = E_INVALIDARG;
    }

    return hr;
}

/**************************************************************************\
*
* This function frees any memory allocated using AllocateTransferBuffer()
*
* Parameters:
*
*     pBuffer - Pointer to a buffer allocated with an AllocateTransferBuffer
*               call. If NULL, this call does nothing.
*
* Return Value:
*
*    None
*
\**************************************************************************/

void FreeTransferBuffer(
    _In_opt_ BYTE* pBuffer)
{
    if (pBuffer)
    {
        CoTaskMemFree(pBuffer);
    }
}

/**************************************************************************\
*
* This function queues a WIA event using the passed in WIA item context.
*
* Parameters:
*
*     pWiasContext - Pointer to the WIA item context
*     guidWIAEvent - WIA event to queue
*
* Return Value:
*
*    None
*
\**************************************************************************/

void QueueWIAEvent(
    _In_ BYTE* pWiasContext,
    const GUID& guidWIAEvent)
{
    HRESULT hr = S_OK;
    BSTR bstrDeviceID = NULL;
    BSTR bstrFullItemName = NULL;
    BYTE* pRootItemContext = NULL;

    hr = wiasReadPropStr(pWiasContext, WIA_IPA_FULL_ITEM_NAME, &bstrFullItemName, NULL, TRUE);

    if (SUCCEEDED(hr))
    {
        hr = wiasGetRootItem(pWiasContext, &pRootItemContext);
        if (SUCCEEDED(hr))
        {
            hr = wiasReadPropStr(pRootItemContext, WIA_DIP_DEV_ID, &bstrDeviceID, NULL, TRUE);
            if (SUCCEEDED(hr))
            {
                hr = wiasQueueEvent(bstrDeviceID, &guidWIAEvent, bstrFullItemName);
                if (FAILED(hr))
                {
                    WIAEX_ERROR((g_hInst, "Failed to queue WIA event, hr = 0x%08X", hr));
                }
            }
            else
            {
                WIAEX_ERROR((g_hInst, "Failed to read the WIA_DIP_DEV_ID property, hr = 0x%08X", hr));
            }
        }
        else
        {
            WIAEX_ERROR((g_hInst, "Failed to get the Root item from child item, using wiasGetRootItem, hr = 0x%08X", hr));
        }
    }
    else
    {
        WIAEX_ERROR((g_hInst, "Failed to read WIA_IPA_FULL_ITEM_NAME property, hr = 0x%08X", hr));
    }

    if (bstrFullItemName)
    {
        SysFreeString(bstrFullItemName);
    }

    if (bstrDeviceID)
    {
        SysFreeString(bstrDeviceID);
    }
}
