/**************************************************************************
*
*  Copyright (c) 2003  Microsoft Corporation
*
*  Title: wiahelpers.cpp
*
*  Description: This file contains a number of helper functions
*               for child item creation etc.
*
***************************************************************************/
#include "stdafx.h"
#include <strsafe.h>

static FILM_FRAME g_FilmFrames[] = { { 36, 27,  222, 167 },
                                     { 36, 221, 222, 167 },
                                     { 37, 418, 222, 167 },
                                     { 37, 614, 221, 172 } };


/**
 * This function creates a full WIA item name
 * from a given WIA item name.
 *
 * The new full item name is created by concatinating
 * the WIA item name with the parent's full item name.
 *
 * (e.g. 0000\Root + MyItem = 0000\Root\MyItem)
 *
 * @param pParent IWiaDrvItem interface of the parent WIA driver item
 * @param bstrItemName
 *                Name of the WIA item
 * @param pbstrFullItemName
 *                Returned full item name.  This parameter
 *                cannot be NULL.
 * @return S_OK - if successful
 *         E_XXXXXXXX - failure result
 */
HRESULT MakeFullItemName(
    _In_    IWiaDrvItem *pParent,
    _In_    BSTR        bstrItemName,
    _Out_   BSTR        *pbstrFullItemName)
{
    HRESULT hr = S_OK;
    if((pParent)&&(bstrItemName)&&(pbstrFullItemName))
    {
        BSTR bstrParentFullItemName = NULL;
        hr = pParent->GetFullItemName(&bstrParentFullItemName);
        if(SUCCEEDED(hr))
        {
            CBasicStringWide cswFullItemName;
            cswFullItemName.Format(TEXT("%ws\\%ws"),bstrParentFullItemName,bstrItemName);
            *pbstrFullItemName = SysAllocString(cswFullItemName.String());
            if(*pbstrFullItemName)
            {
                hr = S_OK;
            }
            else
            {
                hr = E_OUTOFMEMORY;
                WIAS_ERROR((g_hInst, "Failed to allocate memory for BSTR full item name, hr = 0x%lx",hr));
            }
            SysFreeString(bstrParentFullItemName);
            bstrParentFullItemName = NULL;
        }
        else
        {
            WIAS_ERROR((g_hInst, "Failed to get full item name from parent IWiaDrvItem, hr = 0x%lx",hr));
        }
    }
    else
    {
        hr = E_INVALIDARG;
        WIAS_ERROR((g_hInst, "Invalid parameters were passed, hr = 0x%lx",hr));
    }
    return hr;
}

/**
 * This function creates a WIA child item
 *
 * @param wszItemName
 *                   Item name
 * @param pIWiaMiniDrv
 *                   WIA minidriver interface
 * @param pParent Parent's WIA driver item interface
 * @param lItemFlags Item flags
 * @param guidItemCategory
 *                   Item category
 * @param ppChild Pointer to the newly created child item
 * @param wszStoragePath
 *                   Storage data path
 * @return
 */
HRESULT CreateWIAChildItem(
    _In_            LPOLESTR    wszItemName,
    _In_            IWiaMiniDrv *pIWiaMiniDrv,
    _In_            IWiaDrvItem *pParent,
                    LONG        lItemFlags,
                    GUID        guidItemCategory,
    _Out_opt_       IWiaDrvItem **ppChild,
    _In_opt_        PCWSTR      wszStoragePath)
{
    HRESULT hr = E_INVALIDARG;
    if((wszItemName)&&(pIWiaMiniDrv)&&(pParent))
    {
        BSTR        bstrItemName        = SysAllocString(wszItemName);
        BSTR        bstrFullItemName    = NULL;
        IWiaDrvItem *pIWiaDrvItem       = NULL;

        if (bstrItemName)
        {
            hr = MakeFullItemName(pParent,bstrItemName,&bstrFullItemName);
            if(SUCCEEDED(hr))
            {
                WIA_DRIVER_ITEM_CONTEXT *pWiaDriverItemContext = NULL;
                hr = wiasCreateDrvItem(lItemFlags,
                                    bstrItemName,
                                    bstrFullItemName,
                                    pIWiaMiniDrv,
                                    sizeof(WIA_DRIVER_ITEM_CONTEXT),
                                    (BYTE **)&pWiaDriverItemContext,
                                    &pIWiaDrvItem);
                if(SUCCEEDED(hr))
                {
                    pWiaDriverItemContext->ulFeederTransferCount = 0;
                    pWiaDriverItemContext->guidItemCategory = guidItemCategory;
                    if(wszStoragePath)
                    {
                        pWiaDriverItemContext->bstrStorageDataPath = SysAllocString(wszStoragePath);
                        if(!pWiaDriverItemContext->bstrStorageDataPath)
                        {
                            hr = E_OUTOFMEMORY;
                            WIAS_ERROR((g_hInst, "Failed to allocate memory for BSTR storage item path, hr = 0x%lx",hr));
                        }
                    }

                    if(SUCCEEDED(hr))
                    {
                        hr = pIWiaDrvItem->AddItemToFolder(pParent);
                        if(FAILED(hr))
                        {
                            WIAS_ERROR((g_hInst, "Failed to add the new WIA item (%ws) to the specified parent item, hr = 0x%lx",bstrFullItemName,hr));
                            pIWiaDrvItem->Release();
                            pIWiaDrvItem = NULL;
                        }

                        //
                        // If a child iterface pointer parameter was specified, then the caller
                        // expects to have the newly created child interface pointer returned to
                        // them. (do not release the newly created item, in this case)
                        //

                        if(ppChild)
                        {
                            *ppChild        = pIWiaDrvItem;
                            pIWiaDrvItem    = NULL;
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
                    WIAS_ERROR((g_hInst, "Failed to create the new WIA driver item, hr = 0x%lx",hr));
                }

                SysFreeString(bstrItemName);
                bstrItemName = NULL;
                SysFreeString(bstrFullItemName);
                bstrFullItemName = NULL;
            }
            else
            {
                WIAS_ERROR((g_hInst, "Failed to create the new WIA item's full item name, hr = 0x%lx",hr));
            }
        }
        else
        {
            //
            // Failed to allocate memory for bstrItemName.
            //
            hr = E_OUTOFMEMORY;
            WIAS_ERROR((g_hInst, "Failed to allocate memory for BSTR storage item name"));
        }

    }
    else
    {
        WIAS_ERROR((g_hInst, "Invalid parameters were passed"));
    }
    return hr;
}

/**
 * This function creates a WIA flatbed item.  WIA
 * flatbed items will automatically have the
 * WIA category setting of WIA_CATEGORY_FLATBED.
 *
 * @param wszItemName
 *                   Item name
 * @param pIWiaMiniDrv
 *                   WIA minidriver interface
 * @param pParent Parent's WIA driver item interface
 * @return
 */
HRESULT CreateWIAFlatbedItem(
    _In_        LPOLESTR    wszItemName,
    _In_        IWiaMiniDrv *pIWiaMiniDrv,
    _In_        IWiaDrvItem *pParent)
{
    LONG lItemFlags  = WiaItemTypeImage | WiaItemTypeTransfer | WiaItemTypeFile | WiaItemTypeProgrammableDataSource | WiaItemTypeFolder;
    return CreateWIAChildItem(wszItemName,pIWiaMiniDrv,pParent,lItemFlags, WIA_CATEGORY_FLATBED,NULL);
}

/**
 * This function creates a WIA feeder item.  WIA
 * feeder items will automatically have the
 * WIA category setting of WIA_CATEGORY_FEEDER.
 *
 * @param wszItemName
 *                   Item name
 * @param pIWiaMiniDrv
 *                   WIA minidriver interface
 * @param pParent Parent's WIA driver item interface
 * @return
 */
HRESULT CreateWIAFeederItem(
    _In_        LPOLESTR    wszItemName,
    _In_        IWiaMiniDrv *pIWiaMiniDrv,
    _In_        IWiaDrvItem *pParent)
{
    LONG lItemFlags  = WiaItemTypeImage | WiaItemTypeTransfer | WiaItemTypeFile | WiaItemTypeProgrammableDataSource;
    return CreateWIAChildItem(wszItemName,pIWiaMiniDrv,pParent,lItemFlags, WIA_CATEGORY_FEEDER,NULL);
}

/**
 * This function creates a WIA film item.  WIA
 * film items will automatically have the
 * WIA category setting of WIA_CATEGORY_FILM.
 *
 * @param wszItemName
 *                   Item name
 * @param pIWiaMiniDrv
 *                   WIA minidriver interface
 * @param pParent Parent's WIA driver item interface
 * @return
 */
HRESULT CreateWIAFilmItem(
    _In_        LPOLESTR    wszItemName,
    _In_        IWiaMiniDrv *pIWiaMiniDrv,
    _In_        IWiaDrvItem *pParent)
{
    LONG        lItemFlags  = WiaItemTypeImage | WiaItemTypeTransfer | WiaItemTypeFolder | WiaItemTypeProgrammableDataSource;
    IWiaDrvItem *pChild     = NULL;
    HRESULT hr = S_OK;
    hr = CreateWIAChildItem(wszItemName,pIWiaMiniDrv,pParent,lItemFlags, WIA_CATEGORY_FILM,&pChild);
    if(SUCCEEDED(hr))
    {
        if(pChild)
        {
            lItemFlags = WiaItemTypeImage | WiaItemTypeTransfer | WiaItemTypeFile | WiaItemTypeProgrammableDataSource;
            hr = CreateWIAChildItem(L"Frame1",pIWiaMiniDrv,pChild,lItemFlags, WIA_CATEGORY_FILM,NULL);
            if(SUCCEEDED(hr))
            {
                hr = CreateWIAChildItem(L"Frame2",pIWiaMiniDrv,pChild,lItemFlags, WIA_CATEGORY_FILM,NULL);
            }
            if(SUCCEEDED(hr))
            {
                hr = CreateWIAChildItem(L"Frame3",pIWiaMiniDrv,pChild,lItemFlags, WIA_CATEGORY_FILM,NULL);
            }
            if(SUCCEEDED(hr))
            {
                hr = CreateWIAChildItem(L"Frame4",pIWiaMiniDrv,pChild,lItemFlags, WIA_CATEGORY_FILM,NULL);
            }

            pChild->Release();
            pChild = NULL;
        }
    }
    return hr;
}

/**
 * This function creates the main sample WIA storage item.
 * WIA storage items should have either the WIA category
 * of WIA_CATEGORY_FINISHED_FILE (for stored image files)
 * or WIA_CATEGORY_FOLDER (for storage folder items).
 *
 * @param wszItemName
 *                   Item name
 * @param pIWiaMiniDrv
 *                   WIA minidriver interface
 * @param pParent
 *                   Parent's WIA driver item interface
 * @return
 */
HRESULT CreateWIAStorageItem(
    _In_    LPOLESTR    wszItemName,
    _In_    IWiaMiniDrv *pIWiaMiniDrv,
    _In_    IWiaDrvItem *pParent,
    _In_    const WCHAR * wszStoragePath)
{
    LONG lItemFlags  = WiaItemTypeFolder | WiaItemTypeStorage;

    IWiaDrvItem *pChild = NULL;
    HRESULT     hr      = S_OK;

    hr = CreateWIAChildItem(wszItemName, pIWiaMiniDrv, pParent, lItemFlags, WIA_CATEGORY_FOLDER, &pChild);
    if (SUCCEEDED(hr))
    {
        if (pChild)
        {
            lItemFlags = WiaItemTypeTransfer | WiaItemTypeFile;

            //TBD: This function only searches the first level for
            //     content.  It ignores any directories found.

            CBasicStringWide cswSearchPath = wszStoragePath;
            cswSearchPath += L"\\*.*";

            WIN32_FIND_DATA *pFindData = (WIN32_FIND_DATA*) LocalAlloc(LPTR, sizeof(WIN32_FIND_DATA));

            if(!pFindData)
            {
                hr = E_OUTOFMEMORY;
            }
            else
            {
                HANDLE hFindFile = FindFirstFile(cswSearchPath.String(),pFindData);
                if(INVALID_HANDLE_VALUE != hFindFile)
                {
                    do
                    {
                        if(!(pFindData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
                        {
                            CBasicStringWide cswFileDataPath   = wszStoragePath;
                            cswFileDataPath                     += L"\\";
                            cswFileDataPath                     += pFindData->cFileName;
                            hr = CreateWIAChildItem(pFindData->cFileName,
                                                    pIWiaMiniDrv,
                                                    pChild,
                                                    lItemFlags,
                                                    WIA_CATEGORY_FINISHED_FILE,
                                                    NULL,
                                                    cswFileDataPath.String());
                            if(FAILED(hr))
                            {
                                WIAS_ERROR((g_hInst, "Failed to create WIA child storage item, hr = 0x%lx",hr));
                                break;
                            }
                        }

                    } while(FindNextFile(hFindFile,pFindData));

                    FindClose(hFindFile);
                }

                LocalFree(pFindData);
                pFindData = NULL;
            }

            pChild->Release();
            pChild = NULL;
        }
    }

    return hr;
}

/**
 * This function initializes any root item properties
 * needed for this WIA driver.
 *
 * @param pWiasContext
 *               Pointer to the WIA item context
 * @return
 */
HRESULT InitializeRootItemProperties(
    _In_    BYTE        *pWiasContext)
{
    HRESULT hr = E_INVALIDARG;
    if(pWiasContext)
    {
        CWIAPropertyManager PropertyManager;
        GUID guidItemCategory = WIA_CATEGORY_ROOT;
        hr = PropertyManager.AddProperty(WIA_IPA_ITEM_CATEGORY, WIA_IPA_ITEM_CATEGORY_STR, RN, guidItemCategory);
        if(FAILED(hr))
        {
            WIAS_ERROR((g_hInst, "Failed to add WIA_IPA_ITEM_CATEGORY property to the property manager, hr = 0x%lx", hr));
        }

        if(SUCCEEDED(hr))
        {
            LONG lAccessRights = WIA_ITEM_READ;
            hr = PropertyManager.AddProperty(WIA_IPA_ACCESS_RIGHTS ,WIA_IPA_ACCESS_RIGHTS_STR ,RF, lAccessRights, lAccessRights);
            if(FAILED(hr))
            {
                WIAS_ERROR((g_hInst, "Failed to add WIA_IPA_ACCESS_RIGHTS property to the property manager, hr = 0x%lx", hr));
            }
        }

        if(SUCCEEDED(hr))
        {
            LONG lDocumentHandlingCapabilities = FLAT | FEED | DUP | FILM_TPA | STOR;
            hr = PropertyManager.AddProperty(WIA_DPS_DOCUMENT_HANDLING_CAPABILITIES,
                WIA_DPS_DOCUMENT_HANDLING_CAPABILITIES_STR , RN, lDocumentHandlingCapabilities);
            if(FAILED(hr))
            {
                WIAS_ERROR((g_hInst, "Failed to add WIA_DPS_DOCUMENT_HANDLING_CAPABILITIES property to the property manager, hr = 0x%lx", hr));
            }
        }

        if(SUCCEEDED(hr))
        {
            LONG lDocumentHandlingStatus = FEED_READY | FILM_TPA_READY | STORAGE_READY | FLAT_READY;
            hr = PropertyManager.AddProperty(WIA_DPS_DOCUMENT_HANDLING_STATUS,
                WIA_DPS_DOCUMENT_HANDLING_STATUS_STR, RN, lDocumentHandlingStatus);
            if(FAILED(hr))
            {
                WIAS_ERROR((g_hInst, "Failed to add WIA_DPS_DOCUMENT_HANDLING_STATUS property to the property manager, hr = 0x%lx", hr));
            }
        }

        if (SUCCEEDED(hr))
        {
            BSTR bstrFirmware = SysAllocString(L"0.9.1");
            if ( bstrFirmware )
            {
                hr = PropertyManager.AddProperty(WIA_DPA_FIRMWARE_VERSION, WIA_DPA_FIRMWARE_VERSION_STR, RN, bstrFirmware);
                if (FAILED(hr))
                {
                    WIAS_ERROR((g_hInst, "Failed to add WIA_DPA_FIRMWARE_VERSION to prop manager, hr = 0x%lx", hr));
                }
                SysFreeString(bstrFirmware);
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }

        if(SUCCEEDED(hr))
        {
            hr = PropertyManager.SetItemProperties(pWiasContext);
            if(FAILED(hr))
            {
                WIAS_ERROR((g_hInst, "CWIAPropertyManager::SetItemProperties failed to set WIA root item properties, hr = 0x%lx",hr));
            }
        }
        else
        {
            WIAS_ERROR((g_hInst, "Failed to add WIA_IPA_ITEM_CATEGORY property to the property manager, hr = 0x%lx",hr));
        }

    }
    else
    {
        WIAS_ERROR((g_hInst, "Invalid parameters were passed"));
    }
    return hr;
}

/**
 * This function initializes child item properties
 * needed for this WIA driver.  The uiResourceID parameter
 * determines what image properties will be used.
 *
 * @param pWiasContext
 *                  Pointer to the WIA item context
 * @param hInstance HINSTANCE of the resource location containing uiResourceIDs
 * @param uiResourceID
 *                  Resource ID of a bitmap resource loaded as source data
 *                  and a source of WIA item properties.
 *                  FALSE - Child item WIA properties will be added to the item.
 * @return
 */
HRESULT InitializeWIAItemProperties(
    _In_    BYTE        *pWiasContext,
    _In_    HINSTANCE   hInstance,
            UINT        uiResourceID)
{
    // WARNING: No checks for failed CBasicDynamicArray::Append() calls.
    // For robustness, error handling should be added.

    HRESULT hr = E_INVALIDARG;
    BOOL bRootFilm = FALSE;

    if((pWiasContext)&&(hInstance))
    {
        //
        // Reset the feeder image transfer count:
        //
        WIA_DRIVER_ITEM_CONTEXT *pWiaDriverItemContext = NULL;
        hr = wiasGetDriverItemPrivateContext(pWiasContext, (BYTE**)&pWiaDriverItemContext);
        if ((SUCCEEDED(hr)) && (!pWiaDriverItemContext))
        {
            hr = E_POINTER;
        }
        if (SUCCEEDED(hr))
        {
            pWiaDriverItemContext->ulFeederTransferCount = 0;
        }

        if (SUCCEEDED(hr))
        {
            CWIAPropertyManager PropertyManager;

            LONG lXPosition         = 0;
            LONG lYPosition         = 0;
            LONG lXExtent           = 0;
            LONG lYExtent           = 0;
            LONG lPixWidth          = 0;
            LONG lPixHeight         = 0;
            LONG lXResolution       = 75;  // Our sample images are 75 dpi
            LONG lYResolution       = 75;  // Our sample images are 75 dpi
            LONG lHorizontalSize    = 0;
            LONG lVerticalSize      = 0;
            LONG lMinHorizontalSize = 1; //0.001"
            LONG lMinVerticalSize   = 1; //0.001"
            LONG lItemType          = 0;

            HBITMAP hBitmap = static_cast<HBITMAP>(LoadImage(hInstance, MAKEINTRESOURCE(uiResourceID), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION));

            if (hBitmap)
            {
                Bitmap *pBitmap = Bitmap::FromHBITMAP(hBitmap, NULL);

                if (pBitmap)
                {
                    lXExtent        = (LONG)pBitmap->GetWidth();
                    lYExtent        = (LONG)pBitmap->GetHeight();
                    lPixWidth       = lXExtent;
                    lPixHeight      = lYExtent;
                    lHorizontalSize = ConvertTo1000thsOfAnInch(lXExtent, lXResolution);
                    lVerticalSize   = ConvertTo1000thsOfAnInch(lYExtent, lYResolution);

                    SAFE_DELETE (pBitmap);
                }

                DeleteObject(hBitmap);
                hBitmap = NULL;
            }

            //
            // Set coordinates for fixed frames
            //
            if (IDB_FILM == uiResourceID)
            {
                ULONG   ulFrame      = NO_FIXED_FRAME;
                BSTR    bstrItemName = NULL;
                //  Get the item name
                hr = wiasReadPropStr(pWiasContext, WIA_IPA_ITEM_NAME, &bstrItemName, NULL, TRUE);

                if (S_OK == hr)
                {
                    if (!lstrcmp(bstrItemName, L"Frame1"))
                    {
                        ulFrame = 0;
                    }
                    else if (!lstrcmp(bstrItemName, L"Frame2"))
                    {
                        ulFrame = 1;
                    }
                    else if (!lstrcmp(bstrItemName, L"Frame3"))
                    {
                        ulFrame = 2;
                    }
                    else if (!lstrcmp(bstrItemName, L"Frame4"))
                    {
                        ulFrame = 3;
                    }
                    else
                    {
                        bRootFilm = TRUE;
                    }

                    if (ulFrame != NO_FIXED_FRAME)
                    {
                        lXPosition = g_FilmFrames[ulFrame].XPOS;
                        lYPosition = g_FilmFrames[ulFrame].YPOS;
                        lXExtent   = g_FilmFrames[ulFrame].XEXTENT;
                        lYExtent   = g_FilmFrames[ulFrame].YEXTENT;
                    }

                    SysFreeString(bstrItemName);
                    bstrItemName = NULL;
                }
            }

            hr = wiasGetItemType(pWiasContext,&lItemType);
            if(SUCCEEDED(hr))
            {
                if(lItemType & WiaItemTypeGenerated)
                {
                    WIAS_TRACE((g_hInst,"WIA item was created by application."));
                }
            }
            else
            {
                WIAS_ERROR((g_hInst, "Failed to get the WIA item type, hr = 0x%lx",hr));
            }

            //
            // Add all common item properties first
            //

            if((lXExtent)&&(lYExtent)&&(lXResolution)&&(lYResolution)&&(lHorizontalSize)&&(lVerticalSize))
            {
                LONG lAccessRights = WIA_ITEM_READ;
                hr = PropertyManager.AddProperty(WIA_IPA_ACCESS_RIGHTS ,WIA_IPA_ACCESS_RIGHTS_STR ,RF, lAccessRights, lAccessRights);

                if(SUCCEEDED(hr))
                {
                    LONG lOpticalXResolution = lXResolution;
                    hr = PropertyManager.AddProperty(WIA_IPS_OPTICAL_XRES ,WIA_IPS_OPTICAL_XRES_STR ,RN,lOpticalXResolution);
                }

                if(SUCCEEDED(hr))
                {
                    LONG lOpticalYResolution = lYResolution;
                    hr = PropertyManager.AddProperty(WIA_IPS_OPTICAL_YRES ,WIA_IPS_OPTICAL_YRES_STR ,RN,lOpticalYResolution);
                }

                if(SUCCEEDED(hr))
                {
                    CBasicDynamicArray<LONG> lPreviewArray;
                    lPreviewArray.Append(WIA_FINAL_SCAN);
                    lPreviewArray.Append(WIA_PREVIEW_SCAN);
                    hr = PropertyManager.AddProperty(WIA_IPS_PREVIEW ,WIA_IPS_PREVIEW_STR ,RWLC,lPreviewArray[0],lPreviewArray[0],&lPreviewArray);
                }

                if(SUCCEEDED(hr))
                {
                    LONG lShowPreviewControl = WIA_SHOW_PREVIEW_CONTROL;
                    hr = PropertyManager.AddProperty(WIA_IPS_SHOW_PREVIEW_CONTROL ,WIA_IPS_SHOW_PREVIEW_CONTROL_STR ,RN,lShowPreviewControl);
                }

                if(SUCCEEDED(hr))
                {
                    //
                    // Support creation of child items underneath the base flatbed item:
                    //
                    BOOL bChildItemCreation = FALSE;

                    if(uiResourceID == IDB_FLATBED)
                    {
                        LONG lItemFlags = 0;
                        hr = wiasReadPropLong(pWiasContext, WIA_IPA_ITEM_FLAGS, &lItemFlags, NULL, TRUE);
                        if ((S_OK == hr) && (lItemFlags & WiaItemTypeFolder))
                        {
                            bChildItemCreation = TRUE;
                        }
                    }

                    hr = PropertyManager.AddProperty(WIA_IPS_SUPPORTS_CHILD_ITEM_CREATION, WIA_IPS_SUPPORTS_CHILD_ITEM_CREATION_STR, RN, bChildItemCreation);
                }

                if((uiResourceID == IDB_FLATBED) || (uiResourceID == IDB_FILM))
                {
                    if(SUCCEEDED(hr))
                    {
                        hr = PropertyManager.AddProperty(WIA_IPS_MAX_HORIZONTAL_SIZE ,WIA_IPS_MAX_HORIZONTAL_SIZE_STR ,RN,lHorizontalSize);
                    }

                    if(SUCCEEDED(hr))
                    {
                        hr = PropertyManager.AddProperty(WIA_IPS_MAX_VERTICAL_SIZE ,WIA_IPS_MAX_VERTICAL_SIZE_STR ,RN,lVerticalSize);
                    }

                    if(SUCCEEDED(hr))
                    {
                        hr = PropertyManager.AddProperty(WIA_IPS_MIN_HORIZONTAL_SIZE ,WIA_IPS_MIN_HORIZONTAL_SIZE_STR ,RN,lMinHorizontalSize);
                    }

                    if(SUCCEEDED(hr))
                    {
                        hr = PropertyManager.AddProperty(WIA_IPS_MIN_VERTICAL_SIZE ,WIA_IPS_MIN_VERTICAL_SIZE_STR ,RN,lMinVerticalSize);
                    }

                    if(SUCCEEDED(hr))
                    {
                        LONG lSegmentation = (IDB_FLATBED == uiResourceID) ? WIA_USE_SEGMENTATION_FILTER : WIA_DONT_USE_SEGMENTATION_FILTER;
                        hr = PropertyManager.AddProperty(WIA_IPS_SEGMENTATION ,WIA_IPS_SEGMENTATION_STR ,RN, lSegmentation);
                    }

                    if (SUCCEEDED(hr) && (IDB_FILM == uiResourceID))
                    {
                        if (bRootFilm)
                        {
                            CBasicDynamicArray<LONG> lFilmScanModeArray;
                            lFilmScanModeArray.Append(WIA_FILM_COLOR_SLIDE);
                            hr = PropertyManager.AddProperty(WIA_IPS_FILM_SCAN_MODE, WIA_IPS_FILM_SCAN_MODE_STR, RWL, lFilmScanModeArray[0], lFilmScanModeArray[0], &lFilmScanModeArray);
                        }
                    }
                }
                else if(uiResourceID == IDB_FEEDER)
                {
                    if(SUCCEEDED(hr))
                    {
                        hr = PropertyManager.AddProperty(WIA_IPS_MAX_HORIZONTAL_SIZE ,WIA_IPS_MAX_HORIZONTAL_SIZE_STR ,RN,lHorizontalSize);
                    }

                    if(SUCCEEDED(hr))
                    {
                        hr = PropertyManager.AddProperty(WIA_IPS_MAX_VERTICAL_SIZE ,WIA_IPS_MAX_VERTICAL_SIZE_STR ,RN,lVerticalSize);
                    }

                    if(SUCCEEDED(hr))
                    {
                        hr = PropertyManager.AddProperty(WIA_IPS_MIN_HORIZONTAL_SIZE ,WIA_IPS_MIN_HORIZONTAL_SIZE_STR ,RN,lMinHorizontalSize);
                    }

                    if(SUCCEEDED(hr))
                    {
                        hr = PropertyManager.AddProperty(WIA_IPS_MIN_VERTICAL_SIZE ,WIA_IPS_MIN_VERTICAL_SIZE_STR ,RN,lMinVerticalSize);
                    }

                    if(SUCCEEDED(hr))
                    {
                        LONG lSheetFeederRegistration = LEFT_JUSTIFIED;
                        hr = PropertyManager.AddProperty(WIA_IPS_SHEET_FEEDER_REGISTRATION ,WIA_IPS_SHEET_FEEDER_REGISTRATION_STR ,RN,lSheetFeederRegistration);
                    }

                    if(SUCCEEDED(hr))
                    {
                        //
                        // Just basic duplex mode supported (no single back side scan):
                        //
                        LONG lDocumentHandlingSelect            = FRONT_ONLY;
                        LONG lDocumentHandlingSelectValidValues = FRONT_ONLY |DUPLEX;
                        hr = PropertyManager.AddProperty(WIA_IPS_DOCUMENT_HANDLING_SELECT ,WIA_IPS_DOCUMENT_HANDLING_SELECT_STR ,RWF,lDocumentHandlingSelect,lDocumentHandlingSelectValidValues);
                    }

                    if(SUCCEEDED(hr))
                    {
                        LONG lMaxPages            = 100;
                        LONG lDefaultPagesSetting = 1;
                        LONG lPages               = 1;
                        hr = PropertyManager.AddProperty(WIA_IPS_PAGES ,WIA_IPS_PAGES_STR ,RWR,lDefaultPagesSetting,lDefaultPagesSetting,0,lMaxPages,lPages);
                    }

                    //
                    // For the Feeder item implement support for WIA_IPS_PAGE_SIZE (just AUTO supported for now,
                    // in a real case standard and possibly document sizes would have to be added here):
                    //
                    if (SUCCEEDED(hr))
                    {
                        LONG lAutoPageSize = WIA_PAGE_AUTO;
                        hr = PropertyManager.AddProperty(WIA_IPS_PAGE_SIZE, WIA_IPS_PAGE_SIZE_STR, RN, lAutoPageSize);

                        CBasicDynamicArray<LONG> lPageSizeArray;
                        lPageSizeArray.Append(WIA_PAGE_AUTO);
                        hr = PropertyManager.AddProperty(WIA_IPS_PAGE_SIZE, WIA_IPS_PAGE_SIZE_STR, RWL, lPageSizeArray[0], lPageSizeArray[0], &lPageSizeArray);
                    }

                    //
                    // WIA_IPS_PAGE_WIDTH and WIA_IPS_PAGE_HEIGHT are required for feeder item
                    //
                    if (SUCCEEDED(hr)) {
                        hr = PropertyManager.AddProperty(WIA_IPS_PAGE_WIDTH, WIA_IPS_PAGE_WIDTH_STR, RN, lHorizontalSize);
                    }

                    if (SUCCEEDED(hr)) {
                        hr = PropertyManager.AddProperty(WIA_IPS_PAGE_HEIGHT, WIA_IPS_PAGE_HEIGHT_STR, RN, lVerticalSize);
                    }

                    if (SUCCEEDED(hr)) {
                        CBasicDynamicArray<LONG> lOrientationArray;
                        lOrientationArray.Append(PORTRAIT);
                        lOrientationArray.Append(LANSCAPE);
                        lOrientationArray.Append(ROT180);
                        lOrientationArray.Append(ROT270);
                        hr = PropertyManager.AddProperty(WIA_IPS_ORIENTATION, WIA_IPS_ORIENTATION_STR, RWL, lOrientationArray[0], lOrientationArray[0], &lOrientationArray);
                    }
                }

                if(SUCCEEDED(hr))
                {
                    LONG lCurrentIntent             = WIA_INTENT_NONE;
                    LONG lCurrentIntentValidValues  = WIA_INTENT_IMAGE_TYPE_COLOR | WIA_INTENT_MINIMIZE_SIZE | WIA_INTENT_MAXIMIZE_QUALITY;
                    hr = PropertyManager.AddProperty(WIA_IPS_CUR_INTENT ,WIA_IPS_CUR_INTENT_STR ,RWF,lCurrentIntent,lCurrentIntentValidValues);
                }

                if(SUCCEEDED(hr))
                {
                    GUID guidItemCategory = WIA_CATEGORY_FLATBED;
                    switch(uiResourceID)
                    {
                    case IDB_FLATBED:
                        guidItemCategory = WIA_CATEGORY_FLATBED;
                        break;
                    case IDB_FEEDER:
                        guidItemCategory = WIA_CATEGORY_FEEDER;
                        break;
                    case IDB_FILM:
                        guidItemCategory = WIA_CATEGORY_FILM;
                        break;
                    default:
                        guidItemCategory = GUID_NULL;
                        break;
                    }

                    hr = PropertyManager.AddProperty(WIA_IPA_ITEM_CATEGORY,WIA_IPA_ITEM_CATEGORY_STR,RN,guidItemCategory);
                }

                if(SUCCEEDED(hr))
                {
                    CBasicDynamicArray<LONG> lXResolutionArray;
                    lXResolutionArray.Append(lXResolution);
                    hr = PropertyManager.AddProperty(WIA_IPS_XRES ,WIA_IPS_XRES_STR ,RWLC,lXResolutionArray[0],lXResolutionArray[0],&lXResolutionArray);
                }

                if(SUCCEEDED(hr))
                {
                    CBasicDynamicArray<LONG> lYResolutionArray;
                    lYResolutionArray.Append(lYResolution);
                    hr = PropertyManager.AddProperty(WIA_IPS_YRES ,WIA_IPS_YRES_STR ,RWLC,lYResolutionArray[0],lYResolutionArray[0],&lYResolutionArray);
                }

                if(SUCCEEDED(hr))
                {
                    hr = PropertyManager.AddProperty(WIA_IPS_XPOS, WIA_IPS_XPOS_STR, RWRC, lXPosition, lXPosition, 0, lPixWidth - 1, 1);
                }

                if(SUCCEEDED(hr))
                {
                    hr = PropertyManager.AddProperty(WIA_IPS_YPOS, WIA_IPS_YPOS_STR, RWRC, lYPosition, lYPosition, 0, lPixHeight -1, 1);
                }

                if(SUCCEEDED(hr))
                {
                    hr = PropertyManager.AddProperty(WIA_IPS_XEXTENT ,WIA_IPS_XEXTENT_STR ,RWRC, lXExtent, lXExtent, 1, lPixWidth - lXPosition, 1);
                }

                if(SUCCEEDED(hr))
                {
                    hr = PropertyManager.AddProperty(WIA_IPS_YEXTENT ,WIA_IPS_YEXTENT_STR ,RWRC, lYExtent, lYExtent, 1, lPixHeight - lYPosition, 1);
                }

                if(SUCCEEDED(hr))
                {
                    CBasicDynamicArray<LONG> lRotationArray;
                    lRotationArray.Append(PORTRAIT);
                    lRotationArray.Append(LANSCAPE);
                    lRotationArray.Append(ROT180);
                    lRotationArray.Append(ROT270);

                    hr = PropertyManager.AddProperty(WIA_IPS_ROTATION ,WIA_IPS_ROTATION_STR ,RWLC,lRotationArray[0],lRotationArray[0],&lRotationArray);
                }

                if(SUCCEEDED(hr))
                {
                    hr = PropertyManager.AddProperty(WIA_IPS_DESKEW_X ,WIA_IPS_DESKEW_X_STR ,RWRC,0,0,0,lXExtent,1);
                }

                if(SUCCEEDED(hr))
                {
                    hr = PropertyManager.AddProperty(WIA_IPS_DESKEW_Y ,WIA_IPS_DESKEW_Y_STR ,RWRC,0,0,0,lYExtent,1);
                }

                if(SUCCEEDED(hr))
                {
                    LONG lBrightness = 0;
                    hr = PropertyManager.AddProperty(WIA_IPS_BRIGHTNESS,WIA_IPS_BRIGHTNESS_STR,RWRC,lBrightness,lBrightness,-1000,1000,1);
                }

                if(SUCCEEDED(hr))
                {
                    LONG lContrast = 0;
                    hr = PropertyManager.AddProperty(WIA_IPS_CONTRAST ,WIA_IPS_CONTRAST_STR ,RWRC,lContrast,lContrast,-1000,1000,1);
                }

                if(SUCCEEDED(hr))
                {
                    LONG lErrorHandler             = ERROR_HANDLING_NONE;
                    LONG lErrorHandlerValidValues  = ERROR_HANDLING_WARMING_UP | ERROR_HANDLING_COVER_OPEN | ERROR_HANDLING_PRIVATE_ERROR | ERROR_HANDLING_UNHANDLED_STATUS | ERROR_HANDLING_UNHANDLED_ERROR;

                    hr = PropertyManager.AddProperty(MY_WIA_ERROR_HANDLING_PROP ,MY_WIA_ERROR_HANDLING_PROP_STR ,RWF,lErrorHandler,lErrorHandlerValidValues);
                }

                if(SUCCEEDED(hr))
                {
                    CBasicDynamicArray<LONG> lTestFilterArray;
                    lTestFilterArray.Append(0);
                    lTestFilterArray.Append(1);

                    hr = PropertyManager.AddProperty(MY_TEST_FILTER_PROP ,MY_TEST_FILTER_PROP_STR ,RWLC,lTestFilterArray[0],lTestFilterArray[0],&lTestFilterArray);
                }

                if(SUCCEEDED(hr))
                {
                    LONG lItemSize = 0;
                    hr = PropertyManager.AddProperty(WIA_IPA_ITEM_SIZE ,WIA_IPA_ITEM_SIZE_STR ,RN,lItemSize);
                }

                if(SUCCEEDED(hr))
                {
                    // TBD: This property is assuming that the source image is color.  Should be changed to be
                    //      more dynamic.
                    CBasicDynamicArray<LONG> lDataTypeArray;
                    lDataTypeArray.Append(WIA_DATA_COLOR);
                    hr = PropertyManager.AddProperty(WIA_IPA_DATATYPE ,WIA_IPA_DATATYPE_STR ,RWL,lDataTypeArray[0],lDataTypeArray[0],&lDataTypeArray);
                }

                if(SUCCEEDED(hr))
                {
                    // TBD: This property is assuming that the source image is 24-bit color.  Should be changed to be
                    //      more dynamic.
                    CBasicDynamicArray<LONG> lBitDepthArray;
                    lBitDepthArray.Append(24);
                    hr = PropertyManager.AddProperty(WIA_IPA_DEPTH ,WIA_IPA_DEPTH_STR ,RWLC,lBitDepthArray[0],lBitDepthArray[0],&lBitDepthArray);
                }

                if(SUCCEEDED(hr))
                {
                    GUID guidPreferredFormat = WiaImgFmt_BMP;
                    hr = PropertyManager.AddProperty(WIA_IPA_PREFERRED_FORMAT ,WIA_IPA_PREFERRED_FORMAT_STR ,RN,guidPreferredFormat);
                }

                if(SUCCEEDED(hr))
                {
                    CBasicDynamicArray<GUID> guidFormatArray;
                    guidFormatArray.Append(WiaImgFmt_BMP);
                    guidFormatArray.Append(WiaImgFmt_RAW);
                    hr = PropertyManager.AddProperty(WIA_IPA_FORMAT ,WIA_IPA_FORMAT_STR ,RWL,guidFormatArray[0],guidFormatArray[0],&guidFormatArray);
                }

                if(SUCCEEDED(hr))
                {
                    CBasicDynamicArray<LONG> lCompressionArray;
                    lCompressionArray.Append(WIA_COMPRESSION_NONE);
                    hr = PropertyManager.AddProperty(WIA_IPA_COMPRESSION ,WIA_IPA_COMPRESSION_STR ,RWL, lCompressionArray[0], lCompressionArray[0], &lCompressionArray);
                }

                if(SUCCEEDED(hr))
                {
                    CBasicDynamicArray<LONG> lTymedArray;
                    lTymedArray.Append(TYMED_FILE);
                    hr = PropertyManager.AddProperty(WIA_IPA_TYMED ,WIA_IPA_TYMED_STR ,RWL,lTymedArray[0],lTymedArray[0],&lTymedArray);
                }

                if(SUCCEEDED(hr))
                {
                    // TBD: This property is assuming that the source image is 24-bit color and has 3 channels.  Should be changed to be
                    //      more dynamic.
                    LONG lChannelsPerPixel = 3;
                    hr = PropertyManager.AddProperty(WIA_IPA_CHANNELS_PER_PIXEL ,WIA_IPA_CHANNELS_PER_PIXEL_STR ,RN,lChannelsPerPixel);
                }

                if(SUCCEEDED(hr))
                {
                    // TBD: This property is assuming that the source image is 24-bit color and has 8 bits per channel.  Should be changed to be
                    //      more dynamic.
                    LONG lBitsPerChannel = 8;
                    hr = PropertyManager.AddProperty(WIA_IPA_BITS_PER_CHANNEL ,WIA_IPA_BITS_PER_CHANNEL_STR ,RN,lBitsPerChannel);
                }

                if(SUCCEEDED(hr))
                {
                    //
                    // According with the limited type of input image data we use in this sample
                    // we'll initialize this property for 24-bit color / 3 channels RGB image data.
                    // (see also above the initialization of WIA_IPA_CHANNELS_PER_PIXEL and WIA_IPA_BITS_PER_CHANNEL)
                    // A real solution may need however to consider more than just this single format:
                    //
                    BYTE bBitsPerChannel[] = { 8, 8, 8 };
                    hr = PropertyManager.AddProperty(WIA_IPA_RAW_BITS_PER_CHANNEL, WIA_IPA_RAW_BITS_PER_CHANNEL_STR, RN, &bBitsPerChannel[0], 3);
                }

                if(SUCCEEDED(hr))
                {
                    //
                    // WIA_IPS_PHOTOMETRIC_INTERP is needed for the Raw transfer format:
                    // (It shall have WIA_PROP_LIST (with single valid value) | WIA_PROP_RW
                    //
                    CBasicDynamicArray<LONG> lPhotometricInterpArray;
                    lPhotometricInterpArray.Append(WIA_PHOTO_WHITE_1);
                    hr = PropertyManager.AddProperty(WIA_IPS_PHOTOMETRIC_INTERP, WIA_IPS_PHOTOMETRIC_INTERP_STR, RWL,
                        lPhotometricInterpArray[0], lPhotometricInterpArray[0], &lPhotometricInterpArray);
                }

                if(SUCCEEDED(hr))
                {
                    LONG lPlanar = WIA_PACKED_PIXEL;
                    hr = PropertyManager.AddProperty(WIA_IPA_PLANAR ,WIA_IPA_PLANAR_STR ,RN,lPlanar);
                }

                if(SUCCEEDED(hr))
                {
                    // TBD: A small buffer size was used here to allow slower transfers with more progress.  Real
                    //      drivers should use a higher value to increase performance.
                    LONG lBufferSize = DEFAULT_BUFFER_SIZE;
                    hr = PropertyManager.AddProperty(WIA_IPA_BUFFER_SIZE ,WIA_IPA_BUFFER_SIZE_STR ,RN,lBufferSize);
                }

                if(SUCCEEDED(hr))
                {
                    BSTR bstrFileExtension = SysAllocString(L"BMP");
                    if(bstrFileExtension)
                    {
                        hr = PropertyManager.AddProperty(WIA_IPA_FILENAME_EXTENSION ,WIA_IPA_FILENAME_EXTENSION_STR ,RN,bstrFileExtension);

                        SysFreeString(bstrFileExtension);
                        bstrFileExtension = NULL;
                    }
                    else
                    {
                        hr = E_OUTOFMEMORY;
                        WIAS_ERROR((g_hInst, "Could not allocate the file name extension property value, hr = 0x%lx.",hr));
                    }
                }

                /* Optional property
                if(SUCCEEDED(hr))
                {
                GUID guidStreamCompatID = GUID_NULL;
                hr = PropertyManager.AddProperty(WIA_IPA_PROP_STREAM_COMPAT_ID,WIA_IPA_PROP_STREAM_COMPAT_ID_STR,RN,guidStreamCompatID);
                }*/

                if(SUCCEEDED(hr))
                {
                    hr = PropertyManager.SetItemProperties(pWiasContext);
                    if(FAILED(hr))
                    {
                        WIAS_ERROR((g_hInst, "CWIAPropertyManager::SetItemProperties failed to set WIA flatbed item properties, hr = 0x%lx",hr));
                    }
                }
            }
            else
            {
                WIAS_ERROR((g_hInst, "Failed to obtain valid information from flatbed bitmap file resource to build a WIA property set"));
            }
        }
        else
        {
            WIAS_ERROR((g_hInst, "Failed to obtain driver item context data"));
        }
    }
    else
    {
        WIAS_ERROR((g_hInst, "Invalid parameters were passed"));
    }
    return hr;
}

/**
 * This function initializes child item properties
 * needed for this WIA driver's storage item.  The
 * WIA_DRIVER_ITEM_CONTEXT structure stored as the
 * the WIA driver item context will be used to
 * properly set the WIA_IPA_FILENAME_EXTENSION property
 * value.
 *
 * @param pWiasContext
 *                  Pointer to the WIA item context
 * @param bRootItem TRUE - Legacy WIA properties that belong on the root item
 *                  of the device will be added.
 *                  FALSE - Child item WIA properties will be added to the item.
 * @param bFolderItem
 *                  TRUE - storage folder (WIA_CATEGORY_FOLDER)
 *                  FALSE - finished file (WIA_CATEGORY_FINISHED_FILE)
 * @return
 */
HRESULT InitializeWIAStorageItemProperties(
    _In_    BYTE        *pWiasContext,
            BOOL        bRootItem,
            BOOL        bFolderItem)
{
    UNREFERENCED_PARAMETER(bRootItem);

    HRESULT hr = E_INVALIDARG;
    if(pWiasContext)
    {
        CWIAPropertyManager PropertyManager;
        LONG                lItemType = 0;
        hr = wiasGetItemType(pWiasContext,&lItemType);
        if(SUCCEEDED(hr))
        {
            GUID guidItemCategory = bFolderItem ? WIA_CATEGORY_FOLDER : WIA_CATEGORY_FINISHED_FILE;
            hr = PropertyManager.AddProperty(WIA_IPA_ITEM_CATEGORY,WIA_IPA_ITEM_CATEGORY_STR,RN,guidItemCategory);
            if(SUCCEEDED(hr))
            {
                if(!(lItemType & WiaItemTypeGenerated))
                {
                    WIA_DRIVER_ITEM_CONTEXT *pWiaDriverItemContext = NULL;
                    hr = wiasGetDriverItemPrivateContext(pWiasContext,(BYTE**)&pWiaDriverItemContext);
                    if(SUCCEEDED(hr))
                    {
                        if(lItemType & WiaItemTypeStorage)
                        {
                            //
                            // This is the parent storage item, update the number of items stored and
                            // proper access rights
                            //

                            LONG lAccessRights = WIA_ITEM_READ;
                            hr = PropertyManager.AddProperty(WIA_IPA_ACCESS_RIGHTS ,WIA_IPA_ACCESS_RIGHTS_STR ,RF, lAccessRights, lAccessRights);
                            if(SUCCEEDED(hr))
                            {
                                hr = PropertyManager.AddProperty(WIA_IPA_ITEMS_STORED,
                                                             WIA_IPA_ITEMS_STORED_STR,
                                                             RN,
                                                             pWiaDriverItemContext->lNumItemsStored);
                                if(FAILED(hr))
                                {
                                    WIAS_ERROR((g_hInst, "Failed to add WIA_IPA_ITEMS_STORED property to the property manager, hr = 0x%lx",hr));
                                }
                            }
                            else
                            {
                                WIAS_ERROR((g_hInst, "Failed to add WIA_IPA_ACCESS_RIGHTS property to the property manager, hr = 0x%lx",hr));
                            }

                            //
                            // Support creation of child items underneath the root storage item:
                            //

                            BOOL bChildItemCreation = TRUE;
                            hr = PropertyManager.AddProperty(WIA_IPS_SUPPORTS_CHILD_ITEM_CREATION, WIA_IPS_SUPPORTS_CHILD_ITEM_CREATION_STR, RN, bChildItemCreation);
                            if(FAILED(hr))
                            {
                                WIAS_ERROR((g_hInst, "Failed to add WIA_IPS_SUPPORTS_CHILD_ITEM_CREATION(TRUE) property to the property manager, hr = 0x%lx",hr));
                            }
                        }
                        else
                        {
                            //
                            // This must be a child item of some kind
                            //

                            if (SUCCEEDED(hr))
                            {
                                // Let enable delete for this item for better WIA testing
                                LONG lAccessRights = WIA_ITEM_READ | WIA_ITEM_CAN_BE_DELETED;
                                hr = PropertyManager.AddProperty(WIA_IPA_ACCESS_RIGHTS ,WIA_IPA_ACCESS_RIGHTS_STR ,RF, lAccessRights, lAccessRights);
                            }

                            if(SUCCEEDED(hr))
                            {
                                LONG lItemSize = 0;
                                hr = PropertyManager.AddProperty(WIA_IPA_ITEM_SIZE ,WIA_IPA_ITEM_SIZE_STR ,RN,lItemSize);
                            }

                            if(SUCCEEDED(hr))
                            {
                                GUID guidPreferredFormat = WiaImgFmt_UNDEFINED;
                                hr = PropertyManager.AddProperty(WIA_IPA_PREFERRED_FORMAT ,WIA_IPA_PREFERRED_FORMAT_STR ,RN,guidPreferredFormat);
                            }

                            if(SUCCEEDED(hr))
                            {
                                CBasicDynamicArray<GUID> guidFormatArray;
                                guidFormatArray.Append(WiaImgFmt_UNDEFINED);
                                hr = PropertyManager.AddProperty(WIA_IPA_FORMAT ,WIA_IPA_FORMAT_STR ,RWL,guidFormatArray[0],guidFormatArray[0],&guidFormatArray);
                            }

                            if(SUCCEEDED(hr))
                            {
                                CBasicDynamicArray<LONG> lTymedArray;
                                lTymedArray.Append(TYMED_FILE);
                                hr = PropertyManager.AddProperty(WIA_IPA_TYMED ,WIA_IPA_TYMED_STR ,RWL,lTymedArray[0],lTymedArray[0],&lTymedArray);
                            }

                            if(SUCCEEDED(hr))
                            {
                                // TBD: A small buffer size was used here to allow slower transfers with more progress.  Real
                                //      drivers should use a higher value to increase performance.
                                LONG lBufferSize = DEFAULT_BUFFER_SIZE;
                                hr = PropertyManager.AddProperty(WIA_IPA_BUFFER_SIZE ,WIA_IPA_BUFFER_SIZE_STR ,RN,lBufferSize);
                            }

                            if(SUCCEEDED(hr))
                            {
                                BSTR bstrFileExtension = NULL;
                                if(pWiaDriverItemContext->bstrStorageDataPath)
                                {
                                    hr = GetFileExtensionFromPath(pWiaDriverItemContext->bstrStorageDataPath, &bstrFileExtension);
                                }
                                else
                                {
                                    bstrFileExtension = SysAllocString(L"UNDEFINED");
                                    hr = S_OK;
                                }
                                if(SUCCEEDED(hr))
                                {
                                    if(bstrFileExtension)
                                    {
                                        hr = PropertyManager.AddProperty(WIA_IPA_FILENAME_EXTENSION ,WIA_IPA_FILENAME_EXTENSION_STR ,RN,bstrFileExtension);
                                        SysFreeString(bstrFileExtension);
                                        bstrFileExtension = NULL;
                                    }
                                    else
                                    {
                                        hr = E_OUTOFMEMORY;
                                        WIAS_ERROR((g_hInst, "Could not allocate the file name extension property value, hr = 0x%lx.",hr));
                                    }
                                }
                                else
                                {
                                    WIAS_ERROR((g_hInst, "Failed to extract file extension from path (%ws), hr = 0x%lx",pWiaDriverItemContext->bstrStorageDataPath,hr));
                                }
                            }

                            if (SUCCEEDED(hr))
                            {
                                //
                                // Support creation of child items underneath folder items:
                                //

                                if (bFolderItem)
                                {
                                    BOOL bChildItemCreation = TRUE;
                                    hr = PropertyManager.AddProperty(WIA_IPS_SUPPORTS_CHILD_ITEM_CREATION, WIA_IPS_SUPPORTS_CHILD_ITEM_CREATION_STR, RN, bChildItemCreation);
                                    if(FAILED(hr))
                                    {
                                        WIAS_ERROR((g_hInst, "Failed to add WIA_IPS_SUPPORTS_CHILD_ITEM_CREATION property to the property manager, hr = 0x%lx",hr));
                                    }
                                }
                                else
                                {
                                    //
                                    // This is a child item (image item). So we add some image only properties here.
                                    //
                                    // This sample driver supports only 24-bpp color data. A real driver would have to consider separate WIA_IPA_DEPTH
                                    // values for each supported WIA_IPA_DATATYPE and update the available and current WIA_IPA_DEPTH values
                                    // every time the current WIA_IPA_DATATYPE is changed.
                                    //

                                    CBasicDynamicArray<LONG> lDataTypeArray;
                                    lDataTypeArray.Append(WIA_DATA_COLOR);
                                    hr = PropertyManager.AddProperty(WIA_IPA_DATATYPE ,WIA_IPA_DATATYPE_STR ,RWL,lDataTypeArray[0],lDataTypeArray[0],&lDataTypeArray);
                                    if(FAILED(hr))
                                    {
                                        WIAS_ERROR((g_hInst, "Failed to add WIA_IPA_DATATYPE property to the property manager, hr = 0x%lx",hr));
                                    }

                                    CBasicDynamicArray<LONG> lBitDepthArray;
                                    lBitDepthArray.Append(24);
                                    hr = PropertyManager.AddProperty(WIA_IPA_DEPTH ,WIA_IPA_DEPTH_STR ,RWLC,lBitDepthArray[0],lBitDepthArray[0],&lBitDepthArray);
                                    if(FAILED(hr))
                                    {
                                        WIAS_ERROR((g_hInst, "Failed to add WIA_IPA_DEPTH property to the property manager, hr = 0x%lx",hr));
                                    }
                                }

                            }
                        }
                    }
                    else
                    {
                        WIAS_ERROR((g_hInst, "Failed to obtain the WIA_DRIVER_ITEM_CONTEXT structure from the WIA driver item, hr = 0x%lx",hr));
                    }
                }
                else
                {
                    WIAS_TRACE((g_hInst,"WIA item was created by application"));

                    //
                    // Support creation of child items underneath generated folder items:
                    //
                    if (bFolderItem)
                    {
                        BOOL bChildItemCreation = TRUE;
                        hr = PropertyManager.AddProperty(WIA_IPS_SUPPORTS_CHILD_ITEM_CREATION, WIA_IPS_SUPPORTS_CHILD_ITEM_CREATION_STR, RN, bChildItemCreation);
                        if(FAILED(hr))
                        {
                            WIAS_ERROR((g_hInst, "Failed to add WIA_IPS_SUPPORTS_CHILD_ITEM_CREATION property to the property manager, hr = 0x%lx",hr));
                        }
                    }
                }
            }
            else
            {
                WIAS_ERROR((g_hInst, "Failed to add WIA_IPA_ITEM_CATEGORY property to the property manager, hr = 0x%lx",hr));
            }
        }
        else
        {
            WIAS_ERROR((g_hInst, "Failed to get the WIA item type, hr = 0x%lx",hr));
        }

        if(SUCCEEDED(hr))
        {
            hr = PropertyManager.SetItemProperties(pWiasContext);
            if(FAILED(hr))
            {
                WIAS_ERROR((g_hInst, "CWIAPropertyManager::SetItemProperties failed to set WIA storage item properties, hr = 0x%lx",hr));
            }
        }
    }
    else
    {
        WIAS_ERROR((g_hInst, "Invalid parameters were passed"));
    }
    return hr;
}

/**
 * This function returns the WIA driver item context
 * data stored with the driver item.  NOT ALL DRIVER ITEMS
 * HAVE CONTEXTS STORED WITH THEM.  The context is initialized
 * and stored at WIA item creation.  See CreateWIAChildItem
 * function.
 *
 * @param pWiasContext
 *               Pointer to the WIA item context
 * @param ppWiaDriverItemContext
 *               Pointer to the WIA driver item context data
 * @return
 */
HRESULT wiasGetDriverItemPrivateContext(
    _In_    BYTE    *pWiasContext,
    _Out_   BYTE    **ppWiaDriverItemContext)
{
    HRESULT hr = E_INVALIDARG;
    if((pWiasContext)&&(ppWiaDriverItemContext))
    {
        IWiaDrvItem *pIWiaDrvItem   = NULL;
        hr = wiasGetDrvItem(pWiasContext, &pIWiaDrvItem);
        if(SUCCEEDED(hr))
        {
            hr = pIWiaDrvItem->GetDeviceSpecContext(ppWiaDriverItemContext);
            //
            // The caller will handle the failure case.  A failure, may mean that the
            // the WIA item does not have a private device specific context
            // stored.  This is OK, because is is not required.
            //
        }
        else
        {
            WIAS_ERROR((g_hInst, "Failed to get the WIA driver item from the application item, hr = 0x%lx",hr));
        }
    }
    else
    {
        WIAS_ERROR((g_hInst, "Invalid parameters were passed"));
    }
    return hr;
}

/**
 * This function returns the application item's
 * parent WIA item context.  The returned context
 * can be used to access the parent's WIA property
 * set.
 *
 * @param pWiasContext
 *               Pointer to the WIA item context
 * @param ppWiasContext
 *               Pointer to the parent WIA item context
 * @return
 */
HRESULT wiasGetAppItemParent(
    _In_    BYTE    *pWiasContext,
    _Out_   BYTE    **ppWiasContext)
{
    HRESULT hr = E_INVALIDARG;
    if((pWiasContext) && (ppWiasContext))
    {
        // FIX! This helper function is actually getting the backing driver
        //      item and returning it as the parent.  This will make the
        //      driver always associate the newly created child item with its
        //      proper backing driver item.  This function should be fixed to
        //      return the parent application item.
        //

        IWiaDrvItem *pIWiaDrvItemParent = NULL;
        hr = wiasGetDrvItem(pWiasContext,&pIWiaDrvItemParent);
        if(SUCCEEDED(hr))
        {
            BSTR bstrFullItemName = NULL;
            hr = pIWiaDrvItemParent->GetFullItemName(&bstrFullItemName);
            if(SUCCEEDED(hr))
            {
                hr = wiasGetContextFromName(pWiasContext,0,bstrFullItemName,ppWiasContext);
                if(FAILED(hr))
                {
                    WIAS_ERROR((g_hInst, "Failed to get the parent's application item from the item name (%ws), hr = 0x%lx",bstrFullItemName,hr));
                }
                SysFreeString(bstrFullItemName);
                bstrFullItemName = NULL;
            }
            else
            {
                WIAS_ERROR((g_hInst, "Failed to get full item name from parent IWiaDrvItem, hr = 0x%lx",hr));
            }
        }
        else
        {
            WIAS_ERROR((g_hInst, "Failed to get the WIA driver item from the application item, hr = 0x%lx",hr));
        }
    }
    else
    {
        WIAS_ERROR((g_hInst, "Invalid parameters were passed"));
    }
    return hr;
}

/**
 * This function converts a unit (in pixels) to
 * another unit (1/1000ths of an inch).
 *
 * @param lPixelLength
 *               Unit length in pixels
 * @param lResolution
 *               Resolution of Pixel unit length (in Dots Per Inch "DPI")
 * @return
 */
LONG ConvertTo1000thsOfAnInch(
    LONG lPixelLength,
    LONG lResolution)
{
    LONG lConvertedValue = 0;
    if((lPixelLength)&&(lResolution))
    {
        lConvertedValue = (LONG)((((lPixelLength * 1000) + lResolution - 1) / lResolution));
    }
    else
    {
        WIAS_ERROR((g_hInst, "Invalid parameters were passed"));
    }
    return lConvertedValue;
}

/**
 * This function populates a BITMAPINFOHEADER structure
 * using data contained in a Gdiplus::BitmapData object.
 * This function only works with 24-bit data.
 *
 * @param pGDIPlusBitmapData
 *               Pointer to a GDI+ BitmapData object
 * @param pBitmapInfoHeader
 *               Pointer to a BITMAPINFOHEADER structure
 * @return
 */
HRESULT GetBitmapHeaderFromBitmapData(
    _In_    Gdiplus::BitmapData     *pGDIPlusBitmapData,
    _Out_   BITMAPINFOHEADER        *pBitmapInfoHeader)
{
    HRESULT hr = E_INVALIDARG;
    if((pGDIPlusBitmapData) && (pBitmapInfoHeader) && (pGDIPlusBitmapData->PixelFormat == PixelFormat24bppRGB))
    {
        memset(pBitmapInfoHeader, 0, sizeof(BITMAPINFOHEADER));
        pBitmapInfoHeader->biSize       = sizeof(BITMAPINFOHEADER);
        pBitmapInfoHeader->biPlanes     = 1;
        pBitmapInfoHeader->biWidth      = pGDIPlusBitmapData->Width;
        pBitmapInfoHeader->biHeight     = pGDIPlusBitmapData->Height;

        // We cannot use the stride to calculate the size, because if there is no
        // format conversion, we might get the original bits...
        // We need to calculate the size based on the width
        pBitmapInfoHeader->biSizeImage  = ((((pGDIPlusBitmapData->Width * 3) + 3) & ~3) * pGDIPlusBitmapData->Height);

        pBitmapInfoHeader->biBitCount   = 24;
        hr = S_OK;
    }
    else
    {
        WIAS_ERROR((g_hInst, "Invalid parameters were passed"));
    }
    return hr;
}

/**
 * This function returns a Gdiplus::Rect structure
 * initialized with the current WIA extent setting
 * values.  This function assumes that the WIA item
 * passed in supports WIA_IPS_XPOS, WIA_IPS_YPOS,
 * WIA_IPS_XEXTENT and WIA_IPS_YEXTENT properties.
 *
 * @param pWiasContext
 *               Pointer to the WIA item context
 * @param pRect  Pointer to a Gdiplus::Rect object
 * @return
 */
HRESULT GetSelectionAreaRect(
    _In_    BYTE*           pWiasContext,
    _Out_   Gdiplus::Rect   *pRect)
{
    HRESULT hr = E_INVALIDARG;
    LONG    lXPos       = 0;
    LONG    lYPos       = 0;
    LONG    lXExtent    = 0;
    LONG    lYExtent    = 0;

    if((pWiasContext)&&(pRect))
    {
        hr = wiasReadPropLong(pWiasContext,WIA_IPS_XPOS,&lXPos,NULL,TRUE);
        if(SUCCEEDED(hr))
        {
            hr = wiasReadPropLong(pWiasContext,WIA_IPS_YPOS,&lYPos,NULL,TRUE);
            if(FAILED(hr))
            {
                WIAS_ERROR((g_hInst, "Failed to read the WIA_IPS_YPOS property, hr = 0x%lx",hr));
            }
        }
        else
        {
            WIAS_ERROR((g_hInst, "Failed to read the WIA_IPS_XPOS property, hr = 0x%lx",hr));
        }
        if(SUCCEEDED(hr))
        {
            hr = wiasReadPropLong(pWiasContext,WIA_IPS_YEXTENT,&lYExtent,NULL,TRUE);
            if(FAILED(hr))
            {
                WIAS_ERROR((g_hInst, "Failed to read the WIA_IPS_YEXTENT property, hr = 0x%lx",hr));
            }
        }
        if(SUCCEEDED(hr))
        {
            hr = wiasReadPropLong(pWiasContext,WIA_IPS_XEXTENT,&lXExtent,NULL,TRUE);
            if(FAILED(hr))
            {
                WIAS_ERROR((g_hInst, "Failed to read the WIA_IPS_XEXTENT property, hr = 0x%lx",hr));
            }
        }
        if(SUCCEEDED(hr))
        {
            Gdiplus::Rect rFrame((INT)lXPos,(INT)lYPos,(INT)lXExtent,(INT)lYExtent);
            *pRect = rFrame;
            hr = S_OK;
        }
    }
    else
    {
        WIAS_ERROR((g_hInst, "Invalid parameters were passed"));
    }
    return hr;
}

/**
 * This function locks down a portion of a bitmap using
 * the current WIA extent setting values.  This function assumes that the WIA item
 * passed in supports WIA_IPS_XPOS, WIA_IPS_YPOS,
 * WIA_IPS_XEXTENT and WIA_IPS_YEXTENT properties.
 *
 * @param pWiasContext
 *                Pointer to the WIA item context
 * @param pBitmap Pointer to a Gdiplus::Bitmap object containing the
 *                bitmap data.
 * @param pBitmapData
 *                Pointer to a Gdiplus::BitmapData object
 * @param pbmih  Pointer to a BITMAPINFOHEADER structure that will
 *               receive the information about the locked area of the
 *               bitmap.
 * @param ppBitmapBits
 *                Pointer to the first scan line of data of the locked
 *                portion of the bitmap
 * @return
 */
HRESULT LockSelectionAreaOnBitmap(
    _In_    BYTE                    *pWiasContext,
    _In_    Gdiplus::Bitmap         *pBitmap,
    _Out_   Gdiplus::BitmapData     *pBitmapData,
    _In_    BITMAPINFOHEADER        *pbmih,
    _Outptr_result_maybenull_
            BYTE                    **ppBitmapBits)
{
    HRESULT hr = E_INVALIDARG;

    if((pBitmapData)&&(pbmih)&&(ppBitmapBits))
    {
        Gdiplus::Rect rFrame(0,0,0,0);
        hr = GetSelectionAreaRect(pWiasContext,&rFrame);

        if(SUCCEEDED(hr))
        {
            if(pBitmap->LockBits(&rFrame,
                                 ImageLockModeRead,
                                 PixelFormat24bppRGB,
                                 pBitmapData) == Ok)
            {
                hr = GetBitmapHeaderFromBitmapData(pBitmapData,pbmih);
                if(SUCCEEDED(hr))
                {
                    *ppBitmapBits = (BYTE*)pBitmapData->Scan0;
                }
                else
                {
                    WIAS_ERROR((g_hInst, "Failed to get the BITMAPINFOHEADER information from the GDI+ bitmap data object, hr = 0x%lx",hr));
                }
            }
            else
            {
                hr = E_FAIL;
                WIAS_ERROR((g_hInst, "Failed to LockBits on GDI+ bitmap object, hr = 0x%lx",hr));
            }
        }
        else
        {
            WIAS_ERROR((g_hInst, "Failed to get selection area rect from WIA extent settings properties, hr = 0x%lx",hr));
        }
    }
    else
    {
        WIAS_ERROR((g_hInst, "Invalid parameters were passed"));
    }
    return  hr;
}

/**
 * This function unlocks a portion of a bitmap previously locked
 * by LockSelectionAreaOnBitmap function.
 *
 * @param pBitmap Pointer to a Gdiplus::Bitmap object containing the
 *                bitmap data.
 * @param pBitmapData
 *                Pointer to a Gdiplus::BitmapData object
 */
void UnlockSelectionAreaOnBitmap(
    _In_    Gdiplus::Bitmap         *pBitmap,
    _In_    Gdiplus::BitmapData     *pBitmapData)
{
    if((pBitmap)&&(pBitmapData))
    {
        if(pBitmap->UnlockBits(pBitmapData) != Ok)
        {
            WIAS_ERROR((g_hInst, "Failed to UnlockBits on GDI+ bitmap object"));
        }
    }
    else
    {
        WIAS_ERROR((g_hInst, "Invalid parameters were passed"));
    }
}

/**
 * This helper function attempts to grab a IWiaMiniDrvTransferCallback interface
 * from a PMINIDRV_TRANSFER_CONTEXT structure.
 *
 * If successful, caller must Release.
 *
 * @param pmdtc  The PMINIDRV_TRANSFER_CONTEXT handed in during drvAcquireItemData.
 * @param ppIWiaMiniDrvTransferCallback
 *               Address of a interface pointer which receives the callback.
 * @return HRESULT return value.
 */
HRESULT GetTransferCallback(
    _In_        PMINIDRV_TRANSFER_CONTEXT       pmdtc,
    __callback  IWiaMiniDrvTransferCallback     **ppIWiaMiniDrvTransferCallback)
{
    HRESULT hr = E_INVALIDARG;
    if (pmdtc && ppIWiaMiniDrvTransferCallback)
    {
        if (pmdtc->pIWiaMiniDrvCallBack)
        {
            hr = pmdtc->pIWiaMiniDrvCallBack->QueryInterface(IID_IWiaMiniDrvTransferCallback,
                                                             (void**) ppIWiaMiniDrvTransferCallback);
        }
        else
        {
            hr = E_UNEXPECTED;
            WIAS_ERROR((g_hInst, "A NULL pIWiaMiniDrvCallBack was passed in the MINIDRV_TRANSFER_CONTEXT structure, hr = 0x%lx",hr));
        }
    }
    else
    {
        WIAS_ERROR((g_hInst, "Invalid parameters were passed"));
    }
    return hr;
}

/**
 * This function allocates a buffer to be used during
 * data transfers.  FreeTransferBuffer should be called
 * to free the memory allocated by this function.
 *
 * @param pWiasContext
 *                 Pointer to the WIA item context
 * @param ppBuffer Pointer to the allocated buffer.  The caller should call
 *                 FreeTransferBuffer() when finished with this buffer.
 * @param pulBufferSize
 *                 Size of the buffer allocated.
 * @return
 */
HRESULT AllocateTransferBuffer(
    _In_    BYTE                    *pWiasContext,
    _Out_   BYTE                    **ppBuffer,
    _In_    ULONG                   *pulBufferSize)
{
    HRESULT hr = S_OK;

    if (pWiasContext && ppBuffer && pulBufferSize)
    {
        //
        // Set the buffer size to DEFAULT_BUFFER_SIZE
        //
        *pulBufferSize  = DEFAULT_BUFFER_SIZE;

        // Allocate the memory
        *ppBuffer = (BYTE*) CoTaskMemAlloc(*pulBufferSize);
        if (*ppBuffer)
        {
            hr = S_OK;
        }
        else
        {
            hr = E_OUTOFMEMORY;
            WIAS_ERROR((g_hInst, "Failed to allocate memory for transfer buffer, hr = 0x%lx",hr));
        }
    }
    else
    {
        WIAS_ERROR((g_hInst, "Invalid parameters were passed"));
        hr = E_INVALIDARG;
    }
    return hr;
}

/**
 * This function frees any memory allocated using AllocateTransferBuffer()
 * function.
 *
 * @param pBuffer Pointer to a buffer allocated with the AllocateTransferBuffer()
 *                function.
 */
void FreeTransferBuffer(
    _In_    BYTE                    *pBuffer)
{
    // Free the memory
    if (pBuffer)
    {
        CoTaskMemFree(pBuffer);
    }
    else
    {
        WIAS_ERROR((g_hInst, "Invalid parameters were passed. (Attempted to free NULL transfer buffer)"));
    }
}

/**
 * This function attempts to extract the file
 * extension from a file path.  It is assumed that
 * the passed in path contains an extension of some
 * type. (e.g. x:\xxxxx\xxx\xxxxxx.xxx)
 *
 * @param bstrFullPath
 *               Full path containing extension
 * @param pbstrExtension
 *               Extracted extension
 * @return
 */
HRESULT GetFileExtensionFromPath(
    _In_    BSTR    bstrFullPath,
    _Out_   BSTR    *pbstrExtension)
{
    HRESULT hr = E_INVALIDARG;
    if((bstrFullPath)&&(pbstrExtension))
    {
        CBasicStringWide cswPath = bstrFullPath;
        CBasicStringWide cswExtension;
        if(cswPath.Length())
        {
            size_t iEndIndex   = cswPath.Length();
            size_t iStartIndex = cswPath.ReverseFind(TEXT("."));
            if(iStartIndex > 0)
            {
                cswExtension = cswPath.SubStr((iStartIndex + 1),iEndIndex);
                cswExtension = cswExtension.ToUpper();
            }
        }
        *pbstrExtension = SysAllocString(cswExtension.String());
        if(*pbstrExtension)
        {
            hr = S_OK;
        }
        else
        {
            hr = E_OUTOFMEMORY;
            WIAS_ERROR((g_hInst, "Failed to allocate memory for BSTR file extension string, hr = 0x%lx",hr));
        }
    }
    else
    {
        WIAS_ERROR((g_hInst, "Invalid parameters were passed"));
    }
    return hr;
}

/**
 * This function returns TRUE is the WIA item passed
 * in contains the WIA item flag setting of
 * WiaItemTypeProgrammableDataSource.
 *
 * @param pWiasContext
 *                 Pointer to the WIA item context
 * @return
 */
bool IsProgrammableItem(
    _In_    BYTE    *pWiasContext)
{
    LONG    lItemType   = 0;
    HRESULT hr          = S_OK;
    hr = wiasGetItemType(pWiasContext,&lItemType);
    return ((lItemType & WiaItemTypeProgrammableDataSource) == WiaItemTypeProgrammableDataSource);
}

/**
 * This function queues a WIA event using the passed in
 * WIA item context.
 *
 * @param pWiasContext
 *               Pointer to the WIA item context
 * @param guidWIAEvent
 *               WIA event to queue
 */
void QueueWIAEvent(
    _In_    BYTE        *pWiasContext,
            const GUID  &guidWIAEvent)
{
    HRESULT hr                  = S_OK;
    BSTR    bstrDeviceID        = NULL;
    BSTR    bstrFullItemName    = NULL;
    BYTE    *pRootItemContext   = NULL;

    hr = wiasReadPropStr(pWiasContext, WIA_IPA_FULL_ITEM_NAME, &bstrFullItemName, NULL,TRUE);
    if(SUCCEEDED(hr))
    {
        hr = wiasGetRootItem(pWiasContext,&pRootItemContext);
        if(SUCCEEDED(hr))
        {
            hr = wiasReadPropStr(pRootItemContext, WIA_DIP_DEV_ID,&bstrDeviceID, NULL,TRUE);
            if(SUCCEEDED(hr))
            {
                hr = wiasQueueEvent(bstrDeviceID,&guidWIAEvent,bstrFullItemName);
                if(FAILED(hr))
                {
                    WIAS_ERROR((g_hInst, "Failed to queue WIA event, hr = 0x%lx",hr));
                }
            }
            else
            {
                WIAS_ERROR((g_hInst, "Failed to read the WIA_DIP_DEV_ID property, hr = 0x%lx",hr));
            }
        }
        else
        {
            WIAS_ERROR((g_hInst, "Failed to get the Root item from child item, using wiasGetRootItem, hr = 0x%lx",hr));
        }
    }
    else
    {
        WIAS_ERROR((g_hInst, "Failed to read WIA_IPA_FULL_ITEM_NAME property, hr = %lx",hr));
    }

    if(bstrFullItemName)
    {
        SysFreeString(bstrFullItemName);
        bstrFullItemName = NULL;
    }

    if(bstrDeviceID)
    {
        SysFreeString(bstrDeviceID);
        bstrDeviceID = NULL;
    }
}
