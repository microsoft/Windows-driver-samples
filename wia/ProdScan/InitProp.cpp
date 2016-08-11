/**************************************************************************
*
*  Copyright © Microsoft Corporation
*
*  File Name:   InitProp.cpp
*
*  Description: This file contains code for WIA property initialization
*               for the Production Scanner Driver Sample
*
***************************************************************************/

#include "stdafx.h"

/**************************************************************************\
*
* Initializes the Root item properties
*
* Parameters:
*
*    pWiasContext - pointer to the item context
*
* Return Value:
*
*    S_OK if successful, an error HRESULT otherwise
*
\**************************************************************************/

HRESULT CWiaDriver::InitializeRootItemProperties(
    _In_ BYTE* pWiasContext)
{
    HRESULT hr = S_OK;

    WIAEX_TRACE_BEGIN;

    //
    // Validate input:
    //
    if (!pWiasContext)
    {
        hr = E_INVALIDARG;
        WIAEX_ERROR((g_hInst, "Invalid parameter, hr = 0x%08X", hr));
    }

    //
    // Initialize Root item properties:
    //
    if (SUCCEEDED(hr))
    {
        CWIAPropertyManager PropertyManager;

        //
        // WIA_IPA_ITEM_CATEGORY:
        //
        GUID guidItemCategory = WIA_CATEGORY_ROOT;
        hr = PropertyManager.AddProperty(WIA_IPA_ITEM_CATEGORY, WIA_IPA_ITEM_CATEGORY_STR, RN, guidItemCategory);
        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Failed to initialize WIA_IPA_ITEM_CATEGORY, hr = 0x%08X", hr));
        }

        //
        // WIA_IPA_ACCESS_RIGHTS
        //
        if (SUCCEEDED(hr))
        {
            LONG lAccessRights = WIA_ITEM_READ;

            hr = PropertyManager.AddProperty(WIA_IPA_ACCESS_RIGHTS, WIA_IPA_ACCESS_RIGHTS_STR, RF, lAccessRights, lAccessRights);
            if (FAILED(hr))
            {
                WIAEX_ERROR((g_hInst, "Failed to initialize WIA_IPA_ACCESS_RIGHTS, hr = 0x%08X", hr));
            }
        }

        //
        // WIA_DPS_DOCUMENT_HANDLING_CAPABILITIES and default WIA_DPS_DOCUMENT_HANDLING_STATUS:
        //

        LONG lDocumentHandlingCapabilities = AUTO_SOURCE | FLAT | FEED | DUP | IMPRINTER | ENDORSER |
            BARCODE_READER | PATCH_CODE_READER | MICR_READER;

        LONG lDocumentHandlingStatus = FLAT_READY |  FEED_READY | DUP_READY | IMPRINTER_READY | ENDORSER_READY |
            BARCODE_READER_READY | PATCH_CODE_READER_READY | MICR_READER_READY;

        LONG lValidDocumentHandlingStatus = FLAT_COVER_UP | FLAT_READY | FEED_READY | DUP_READY | IMPRINTER_READY |
            ENDORSER_READY | BARCODE_READER_READY | PATCH_CODE_READER_READY | MICR_READER_READY |
            PAPER_JAM | PATH_COVER_UP | MULTIPLE_FEED | DEVICE_ATTENTION | LAMP_ERR;

        if (SUCCEEDED(hr))
        {
            hr = PropertyManager.AddProperty(WIA_DPS_DOCUMENT_HANDLING_CAPABILITIES,
                WIA_DPS_DOCUMENT_HANDLING_CAPABILITIES_STR , RN, lDocumentHandlingCapabilities);

            if(FAILED(hr))
            {
                WIAEX_ERROR((g_hInst, "Failed to initialize WIA_DPS_DOCUMENT_HANDLING_CAPABILITIES, hr = 0x%08X", hr));
            }
        }

        //
        // WIA_DPS_DOCUMENT_HANDLING_STATUS:
        //
        if (SUCCEEDED(hr))
        {
            //
            // Initialize with default ready flag values:
            //
            hr = PropertyManager.AddProperty(WIA_DPS_DOCUMENT_HANDLING_STATUS,
                WIA_DPS_DOCUMENT_HANDLING_STATUS_STR, RN, lDocumentHandlingStatus, lValidDocumentHandlingStatus);

            if (FAILED(hr))
            {
                WIAEX_ERROR((g_hInst, "Failed to initialize WIA_DPS_DOCUMENT_HANDLING_STATUS, hr = 0x%08X", hr));
            }
        }

        //
        // WIA_DPA_CONNECT_STATUS
        //
        // The sample device is always available:
        //
        if (SUCCEEDED(hr))
        {
            LONG lDeviceConnected = 1;
            hr = PropertyManager.AddProperty(WIA_DPA_CONNECT_STATUS, WIA_DPA_CONNECT_STATUS_STR, RN, lDeviceConnected);
            if (FAILED(hr))
            {
                WIAEX_ERROR((g_hInst, "Failed to initialize WIA_DPA_CONNECT_STATUS, hr = 0x%08X", hr));
            }
        }

        //
        // WIA_DPS_SCAN_AVAILABLE_ITEM:
        //
        if (SUCCEEDED(hr))
        {
            //
            // If the global (per driver instance) m_bstrScanAvailableItem is not yet initialized
            // initialize it now to an empty string. Note that because we use here m_bstrScanAvailableItem
            // to initialize the property we do not need to execute UpdateScanAvailableItemProperty:
            //
            if (!m_bstrScanAvailableItem)
            {
                hr = UpdateScanAvailableItemName(NULL);
                if (FAILED(hr))
                {
                    WIAEX_ERROR((g_hInst, "Failed to initialize the scan available item name, hr = 0x%08X", hr));
                }
            }
            else
            {
                WIAS_TRACE((g_hInst, "Scan available from %ws", m_bstrScanAvailableItem));
            }

            if (SUCCEEDED(hr))
            {
                #pragma prefast(suppress:__WARNING_INVALID_PARAM_VALUE_1, "m_bstrScanAvailableItem is allocated to an empty string by the UpdateScanAvailableItemName call above"
                hr = PropertyManager.AddProperty(WIA_DPS_SCAN_AVAILABLE_ITEM, WIA_DPS_SCAN_AVAILABLE_ITEM_STR, RN, m_bstrScanAvailableItem);
                if (FAILED(hr))
                {
                    WIAEX_ERROR((g_hInst, "Failed to initialize WIA_DPS_SCAN_AVAILABLE_ITEM, hr = 0x%08X", hr));
                }
            }
        }

        //
        // Set the properties:
        //
        if (SUCCEEDED(hr))
        {
            hr = PropertyManager.SetItemProperties(pWiasContext);
            if (FAILED(hr))
            {
                WIAEX_ERROR((g_hInst, "CWIAPropertyManager::SetItemProperties failed to set WIA root item properties, hr = 0x%08X", hr));
            }
        }
        else
        {
            WIAEX_ERROR((g_hInst, "Failed to initialize WIA_IPA_ITEM_CATEGORY, hr = 0x%08X", hr));
        }
    }

    WIAEX_TRACE_FUNC_HR;

    return hr;
}

/**************************************************************************\
*
* Wrapper method to initializes the properties for the child items this
* sample driver creates.
*
* Parameters:
*
*    pWiasContext            - pointer to the item context
*    nDocumentHandlingSelect - a WIA_DPS_DOCUMENT_HANDLING_SELECT value (such
*                              as FLAT or FEED (defined in wiadef.h) identifying
*                              the item being initialized
*
* Return Value:
*
*    S_OK if successful, an error HRESULT otherwise
*
\**************************************************************************/

HRESULT CWiaDriver::InitializeChildItemProperties(
    _In_ BYTE*     pWiasContext,
    UINT           nDocumentHandlingSelect)

{
    HRESULT hr = S_OK;
    CWIAPropertyManager PropertyManager;

    WIAEX_TRACE_BEGIN;

    //
    // No need to validate parameters of trace failures here, the called functions
    // do the validation and output full error traces:
    //
    hr = InitializeCommonChildProperties(pWiasContext, nDocumentHandlingSelect);
    if (SUCCEEDED(hr))
    {
        if ((FLAT == nDocumentHandlingSelect) || (FEEDER == nDocumentHandlingSelect))
        {
            hr = InitializeFlatbedFeederProperties(pWiasContext, nDocumentHandlingSelect);
            if (SUCCEEDED(hr) && (FEEDER == nDocumentHandlingSelect))
            {
                hr = InitializeFeederSpecificProperties(pWiasContext);
            }
        }
        else if ((IMPRINTER == nDocumentHandlingSelect) || (ENDORSER == nDocumentHandlingSelect))
        {
            hr = InitializeImprinterEndorserProperties(pWiasContext, nDocumentHandlingSelect);
        }
        else if (BARCODE_READER == nDocumentHandlingSelect)
        {
            hr = InitializeBarcodeReaderProperties(pWiasContext);
        }
        else if (PATCH_CODE_READER == nDocumentHandlingSelect)
        {
            hr = InitializePatchCodeReaderProperties(pWiasContext);
        }
        else if (MICR_READER == nDocumentHandlingSelect)
        {
            hr = InitializeMicrReaderProperties(pWiasContext);
        }
    }

    WIAEX_TRACE_FUNC_HR;

    return hr;
}

/**************************************************************************\
*
* Initializes common child item properties (properties common to all
* data source items this driver creates: Flatbed, Feeder, Auto, Imprinter, etc.
*
* Parameters:
*
*    pWiasContext            - pointer to the item context
*    nDocumentHandlingSelect - a WIA_DPS_DOCUMENT_HANDLING_SELECT value (such
*                              as FLAT or FEED (defined in wiadef.h) identifying
*                              the item being initialized
*
* Return Value:
*
*    S_OK if successful, an error HRESULT otherwise
*
\**************************************************************************/

HRESULT CWiaDriver::InitializeCommonChildProperties(
    _In_ BYTE*     pWiasContext,
    UINT           nDocumentHandlingSelect)

{
    HRESULT hr = S_OK;

    CWIAPropertyManager PropertyManager;

    if (!pWiasContext)
    {
        hr = E_INVALIDARG;
        WIAEX_ERROR((g_hInst, "CWiaDriver::InitializeCommonChildProperties, invalid item context parameter, hr = 0x%08X", hr));
    }

    WIAEX_TRACE_BEGIN;

    //
    // WIA_IPA_ITEM_CATEGORY
    //
    if (SUCCEEDED(hr))
    {
        GUID guidItemCategory = WIA_CATEGORY_FLATBED;

        switch (nDocumentHandlingSelect)
        {
            case FLAT:
                guidItemCategory = WIA_CATEGORY_FLATBED;
                break;

            case FEED:
                guidItemCategory = WIA_CATEGORY_FEEDER;
                break;

            case AUTO_SOURCE:
                guidItemCategory = WIA_CATEGORY_AUTO;
                break;

            case IMPRINTER:
                guidItemCategory = WIA_CATEGORY_IMPRINTER;
                break;

            case ENDORSER:
                guidItemCategory = WIA_CATEGORY_ENDORSER;
                break;

            case BARCODE_READER:
                guidItemCategory = WIA_CATEGORY_BARCODE_READER;
                break;

            case PATCH_CODE_READER:
                guidItemCategory = WIA_CATEGORY_PATCH_CODE_READER;
                break;

            case MICR_READER:
                guidItemCategory = WIA_CATEGORY_MICR_READER;
                break;

            default:
                hr = E_INVALIDARG;
                WIAEX_ERROR((g_hInst, "CWiaDriver::InitializeCommonChildProperties, invalid item (%u) parameter, hr = 0x%08X",
                    nDocumentHandlingSelect, hr));
        }

        hr = PropertyManager.AddProperty(WIA_IPA_ITEM_CATEGORY, WIA_IPA_ITEM_CATEGORY_STR, RN, guidItemCategory);
        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Failed to initialize WIA_IPA_ITEM_CATEGORY for item %u, hr = 0x%08X", nDocumentHandlingSelect, hr));
        }
    }

    //
    // WIA_IPA_ACCESS_RIGHTS
    //
    if (SUCCEEDED(hr))
    {
        LONG lAccessRights = WIA_ITEM_READ;

        hr = PropertyManager.AddProperty(WIA_IPA_ACCESS_RIGHTS, WIA_IPA_ACCESS_RIGHTS_STR, RF, lAccessRights, lAccessRights);
        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Failed to initialize WIA_IPA_ACCESS_RIGHTS for item %u, hr = 0x%08X", nDocumentHandlingSelect, hr));
        }
    }

    //
    // WIA_IPA_FORMAT
    //
    // For image transfers, this sample driver supports the DIB (mandatory default), EXIF and Raw image file formats.
    //
    // The sample imprinter and endorser items support the CSV (mandatory default) and TXT for text transfers and
    // DIB (mandatory default) for graphics transfers. Default data transfer mode is text (required).
    //
    // The sample barcode, patch code and MICR reader items support the required XML and Raw metadata transfers.
    //
    if (SUCCEEDED(hr))
    {
        CBasicDynamicArray<GUID> guidFormatArray;

        if ((FLAT == nDocumentHandlingSelect) || (FEED == nDocumentHandlingSelect) ||
            (AUTO_SOURCE == nDocumentHandlingSelect))
        {
            guidFormatArray.Append(WiaImgFmt_BMP);
            guidFormatArray.Append(WiaImgFmt_EXIF);
            guidFormatArray.Append(WiaImgFmt_RAW);
        }
        else if ((IMPRINTER == nDocumentHandlingSelect) || (ENDORSER == nDocumentHandlingSelect))
        {
            guidFormatArray.Append(WiaImgFmt_CSV);
            guidFormatArray.Append(WiaImgFmt_TXT);
            guidFormatArray.Append(WiaImgFmt_BMP);
        }
        else if (BARCODE_READER == nDocumentHandlingSelect)
        {
            guidFormatArray.Append(WiaImgFmt_XMLBAR);
            guidFormatArray.Append(WiaImgFmt_RAWBAR);
        }
        else if (PATCH_CODE_READER == nDocumentHandlingSelect)
        {
            guidFormatArray.Append(WiaImgFmt_XMLPAT);
            guidFormatArray.Append(WiaImgFmt_RAWPAT);
        }
        else if (MICR_READER == nDocumentHandlingSelect)
        {
            guidFormatArray.Append(WiaImgFmt_XMLMIC);
            guidFormatArray.Append(WiaImgFmt_RAWMIC);
        }

        hr = PropertyManager.AddProperty(WIA_IPA_FORMAT, WIA_IPA_FORMAT_STR, RWL, guidFormatArray[0], guidFormatArray[0], &guidFormatArray);
        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Failed to initialize WIA_IPA_FORMAT for item %u, hr = 0x%08X", nDocumentHandlingSelect, hr));
        }
    }

    //
    // WIA_IPA_TYMED
    //
    // This sample driver supports only TYMED_FILE (single page files) for image as well as text and metadata data transfers
    //
    if (SUCCEEDED(hr))
    {
        CBasicDynamicArray<LONG> lTymedArray;
        lTymedArray.Append(TYMED_FILE);

        hr = PropertyManager.AddProperty(WIA_IPA_TYMED, WIA_IPA_TYMED_STR, RWL, lTymedArray[0], lTymedArray[0], &lTymedArray);
        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Failed to initialize WIA_IPA_TYMED for item %u, hr = 0x%08X", nDocumentHandlingSelect, hr));
        }
    }

    //
    // WIA_IPA_PREFERRED_FORMAT
    //
    // For image transfers, this sample driver reports EXIF as the preferred transfer file format.
    //
    // For printer/endorser transfers, this driver reports CSV as the preferred transfer file format.
    //
    // For barcode, patch code and MICR metadata transfers, this driver reports XML as the preferred transfer file format.
    //
    if (SUCCEEDED(hr))
    {
        GUID guidPreferredFormat = {};

        if ((FLAT == nDocumentHandlingSelect) || (FEED == nDocumentHandlingSelect) ||
            (AUTO_SOURCE == nDocumentHandlingSelect))
        {
            guidPreferredFormat = WiaImgFmt_EXIF;
        }
        else if ((IMPRINTER == nDocumentHandlingSelect) || (ENDORSER == nDocumentHandlingSelect))
        {
            guidPreferredFormat = WiaImgFmt_CSV;
        }
        else if (BARCODE_READER == nDocumentHandlingSelect)
        {
            guidPreferredFormat = WiaImgFmt_XMLBAR;
        }
        else if (PATCH_CODE_READER == nDocumentHandlingSelect)
        {
            guidPreferredFormat = WiaImgFmt_XMLPAT;
        }
        else if (MICR_READER == nDocumentHandlingSelect)
        {
            guidPreferredFormat = WiaImgFmt_XMLMIC;
        }

        hr = PropertyManager.AddProperty(WIA_IPA_PREFERRED_FORMAT, WIA_IPA_PREFERRED_FORMAT_STR, RN, guidPreferredFormat);
        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Failed to initialize WIA_IPA_PREFERRED_FORMAT for item %u, hr = 0x%08X", nDocumentHandlingSelect, hr));
        }
    }

    //
    // WIA_IPA_FILENAME_EXTENSION
    //
    if (SUCCEEDED(hr))
    {
        BSTR bstrFileExtension = NULL;

        //
        // Note that WIA_IPA_FILENAME_EXTENSION must match the WIA_IPA_FORMAT current value, not WIA_IPA_PREFERRED_FORMAT:
        //
        if ((FLAT == nDocumentHandlingSelect) || (FEED == nDocumentHandlingSelect) ||
            (AUTO_SOURCE == nDocumentHandlingSelect))
        {
            bstrFileExtension = SysAllocString(FILE_EXT_BMP);
        }
        else if ((IMPRINTER == nDocumentHandlingSelect) || (ENDORSER == nDocumentHandlingSelect))
        {
            bstrFileExtension = SysAllocString(FILE_EXT_CSV);
        }
        else if ((BARCODE_READER == nDocumentHandlingSelect) || (PATCH_CODE_READER == nDocumentHandlingSelect) ||
            (MICR_READER == nDocumentHandlingSelect))
        {
            bstrFileExtension = SysAllocString(FILE_EXT_XML);
        }

        if (bstrFileExtension)
        {
            hr = PropertyManager.AddProperty(WIA_IPA_FILENAME_EXTENSION, WIA_IPA_FILENAME_EXTENSION_STR, RN, bstrFileExtension);

            SysFreeString(bstrFileExtension);
            bstrFileExtension = NULL;
        }
        else
        {
            hr = E_OUTOFMEMORY;
            WIAEX_ERROR((g_hInst, "Could not allocate the file name extension property value, hr = 0x%08X", hr));
        }

        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Failed to initialize WIA_IPA_FILENAME_EXTENSION for item %u, hr = 0x%08X", nDocumentHandlingSelect, hr));
        }
    }

    //
    // WIA_IPA_COMPRESSION
    //
    // For image transfers, this sample driver supports no compression (mandatory default, for DIB and Raw transfers)
    // and JPEG (EEXIF transfers). The sample driver also pretends to support auto-compression (WIA_COMPRESSION_AUTO)
    // but in auto-compression mode JPEG compression is always selected.
    //
    // For all other metadata transfers, this sample driver supports no compression (mandatory default).
    //
    if (SUCCEEDED(hr))
    {
        CBasicDynamicArray<LONG> lCompressionArray;

        lCompressionArray.Append(WIA_COMPRESSION_NONE);
        if ((FLAT == nDocumentHandlingSelect) || (FEED == nDocumentHandlingSelect) || (AUTO_SOURCE == nDocumentHandlingSelect))
        {
            lCompressionArray.Append(WIA_COMPRESSION_JPEG);
            lCompressionArray.Append(WIA_COMPRESSION_AUTO);
        }

        hr = PropertyManager.AddProperty(WIA_IPA_COMPRESSION, WIA_IPA_COMPRESSION_STR, RWL, lCompressionArray[0], lCompressionArray[0], &lCompressionArray);
        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Failed to initialize WIA_IPA_COMPRESSION for item %u, hr = 0x%08X", nDocumentHandlingSelect, hr));
        }
    }

    //
    // Apply the property changes to the current session's Application Item Tree:
    //

    if (SUCCEEDED(hr))
    {
        hr = PropertyManager.SetItemProperties(pWiasContext);
        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "CWIAPropertyManager::SetItemProperties failed to set WIA item properties for item %u, hr = 0x%08X", nDocumentHandlingSelect, hr));
        }
    }

    WIAEX_TRACE_FUNC_HR;

    return hr;
}

/**************************************************************************\
*
* Initializes the properties common to the Flatbed and Feeder items.
*
* Parameters:
*
*    pWiasContext            - pointer to the item context
*    nDocumentHandlingSelect - FLAT or FEED (defined in wiadef.h)
*
* Return Value:
*
*    S_OK if successful, an error HRESULT otherwise
*
\**************************************************************************/

HRESULT CWiaDriver::InitializeFlatbedFeederProperties(
    _In_ BYTE*     pWiasContext,
    UINT           nDocumentHandlingSelect)

{
    HRESULT hr = S_OK;
    CWIAPropertyManager PropertyManager;

    if ((!pWiasContext) || ((FLAT != nDocumentHandlingSelect) && (FEED != nDocumentHandlingSelect)))
    {
        hr = E_INVALIDARG;
        WIAEX_ERROR((g_hInst, "CWiaDriver::InitializeFlatbedFeederProperties, invalid parameter, hr = 0x%08X", hr));
    }

    WIAEX_TRACE_BEGIN;

    //
    // WIA_IPA_ITEM_SIZE
    //
    if (SUCCEEDED(hr))
    {
        LONG lItemSize = 0;

        hr = PropertyManager.AddProperty(WIA_IPA_ITEM_SIZE, WIA_IPA_ITEM_SIZE_STR, RN, lItemSize);
        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Failed to initialize WIA_IPA_ITEM_SIZE for item %u, hr = 0x%08X", nDocumentHandlingSelect, hr));
        }
    }

    //
    // WIA_IPA_PLANAR:
    //
    LONG lPlanar = WIA_PACKED_PIXEL;

    hr = PropertyManager.AddProperty(WIA_IPA_PLANAR, WIA_IPA_PLANAR_STR, RN, lPlanar);
    if (FAILED(hr))
    {
        WIAEX_ERROR((g_hInst, "Failed to initialize WIA_IPA_PLANAR for item %u, hr = 0x%08X", nDocumentHandlingSelect, hr));
    }

    //
    // WIA_IPA_DATATYPE
    //
    // This sample driver supports 24-bpp RGB color and 8-bpp Grayscale for the image transfers, as well as the auto color mode.
    // When WIA_DATA_AUTO is set the sample driver choses randomly between WIA_DATA_GRAYSCALE and WIA_DATA_COLOR.
    // A real driver should base this decision on the actual document that is scanned:
    //
    if (SUCCEEDED(hr))
    {
        CBasicDynamicArray<LONG> lDataTypeArray;
        lDataTypeArray.Append(WIA_DATA_GRAYSCALE);
        lDataTypeArray.Append(WIA_DATA_COLOR);
        lDataTypeArray.Append(WIA_DATA_AUTO);

        hr = PropertyManager.AddProperty(WIA_IPA_DATATYPE, WIA_IPA_DATATYPE_STR, RWL, lDataTypeArray[0], lDataTypeArray[0], &lDataTypeArray);
        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Failed to initialize WIA_IPA_DATATYPE for item %u, hr = 0x%08X", nDocumentHandlingSelect, hr));
        }
    }

    //
    // WIA_IPA_DEPTH
    //
    // This sample driver supports 24-bpp RGB color and 8-bpp Grayscale, as well as the auto value (WIA_DEPTH_AUTO or 0):
    //
    if (SUCCEEDED(hr))
    {
        CBasicDynamicArray<LONG> lDepthArray;
        lDepthArray.Append(8);
        lDepthArray.Append(24);
        lDepthArray.Append(WIA_DEPTH_AUTO);

        hr = PropertyManager.AddProperty(WIA_IPA_DEPTH , WIA_IPA_DEPTH_STR, RWLC, lDepthArray[0], lDepthArray[0], &lDepthArray);
        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Failed to initialize WIA_IPA_DEPTH for item %u, hr = 0x%08X", nDocumentHandlingSelect, hr));
        }
    }

    //
    // WIA_IPA_CHANNELS_PER_PIXEL
    //
    // This sample driver supports 1 and 3 channels (samples) per pixel
    //
    if (SUCCEEDED(hr))
    {
        LONG lChannelsPerPixel = 1; //default value that matches the default WIA_DATA_GRAYSCALE

        hr = PropertyManager.AddProperty(WIA_IPA_CHANNELS_PER_PIXEL, WIA_IPA_CHANNELS_PER_PIXEL_STR, RN, lChannelsPerPixel);
        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Failed to initialize WIA_IPA_CHANNELS_PER_PIXEL for item %u, hr = 0x%08X", nDocumentHandlingSelect, hr));
        }
    }

    //
    // WIA_IPA_BITS_PER_CHANNEL
    //
    // This sample driver supports only 8 bits per channel (sample)
    //
    if (SUCCEEDED(hr))
    {
        LONG lBitsPerChannel = 8;

        hr = PropertyManager.AddProperty(WIA_IPA_BITS_PER_CHANNEL, WIA_IPA_BITS_PER_CHANNEL_STR, RN, lBitsPerChannel);
        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Failed to initialize WIA_IPA_BITS_PER_CHANNEL for item %u, hr = 0x%08X", nDocumentHandlingSelect, hr));
        }
    }

    //
    // WIA_IPA_RAW_BITS_PER_CHANNEL
    //
    if (SUCCEEDED(hr))
    {
        BYTE bRawBitsPerChannel[] = { 8 }; //to match the default WIA_DATA_GRAYSCALE

        hr = PropertyManager.AddProperty(WIA_IPA_RAW_BITS_PER_CHANNEL, WIA_IPA_RAW_BITS_PER_CHANNEL_STR, RN, &bRawBitsPerChannel[0], 1);
        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Failed to initialize WIA_IPA_RAW_BITS_PER_CHANNEL for item %u, hr = 0x%08X", nDocumentHandlingSelect, hr));
        }
    }


    //
    // WIA_IPS_CUR_INTENT
    //
    if (SUCCEEDED(hr))
    {
        LONG lCurrentIntent = WIA_INTENT_NONE;
        LONG lValidIntents = WIA_INTENT_IMAGE_TYPE_COLOR | WIA_INTENT_IMAGE_TYPE_GRAYSCALE | WIA_INTENT_MAXIMIZE_QUALITY | WIA_INTENT_MINIMIZE_SIZE;

        hr = PropertyManager.AddProperty(WIA_IPS_CUR_INTENT, WIA_IPS_CUR_INTENT_STR, RWF, lCurrentIntent, lValidIntents);
        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Failed to initialize WIA_IPS_CUR_INTENT for item %u, hr = 0x%08X", nDocumentHandlingSelect, hr));
        }
    }

    //
    // WIA_IPS_OPTICAL_XRES and WIA_IPS_OPTICAL_YRES
    //
    // This sample driver reports OPTICAL_RESOLUTION DPI as optical resolution on both scan directions
    //
    if (SUCCEEDED(hr))
    {
        LONG lOpticalResolution = OPTICAL_RESOLUTION;

        hr = PropertyManager.AddProperty(WIA_IPS_OPTICAL_XRES, WIA_IPS_OPTICAL_XRES_STR, RN, lOpticalResolution);
        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Failed to initialize WIA_IPS_OPTICAL_XRES for item %u, hr = 0x%08X", nDocumentHandlingSelect, hr));
        }

        if (SUCCEEDED(hr))
        {
            hr = PropertyManager.AddProperty(WIA_IPS_OPTICAL_YRES, WIA_IPS_OPTICAL_YRES_STR, RN, lOpticalResolution);
            if (FAILED(hr))
            {
                WIAEX_ERROR((g_hInst, "Failed to initialize WIA_IPS_OPTICAL_YRES for item %u, hr = 0x%08X", nDocumentHandlingSelect, hr));
            }
        }
    }

    //
    // WIA_IPS_XRES and WIA_IPS_YRES
    //
    // This sample driver supports OPTICAL_RESOLUTION DPI as the only scan resolution for both scan directions
    //
    if (SUCCEEDED(hr))
    {
        CBasicDynamicArray<LONG> lResolutionArray;
        lResolutionArray.Append(OPTICAL_RESOLUTION);

        //
        // Add WIA_IPS_XRES and WIA_IPS_YRES as WIA_PROP_LIST:
        //
        hr = PropertyManager.AddProperty(WIA_IPS_XRES, WIA_IPS_XRES_STR, RWLC, lResolutionArray[0], lResolutionArray[0], &lResolutionArray);
        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Failed to initialize WIA_IPS_XRES for item %u, hr = 0x%08X", nDocumentHandlingSelect, hr));
        }

        if (SUCCEEDED(hr))
        {
            hr = PropertyManager.AddProperty(WIA_IPS_YRES, WIA_IPS_YRES_STR, RWLC, lResolutionArray[0], lResolutionArray[0], &lResolutionArray);
            if (FAILED(hr))
            {
                WIAEX_ERROR((g_hInst, "Failed to initialize WIA_IPS_YRES for item %u, hr = 0x%08X", nDocumentHandlingSelect, hr));
            }
        }
    }

    //
    // WIA_IPS_XSCALING and WIA_IPS_YSCALING
    //
    // This sample driover supports only 100% scaling (which means no actual scaling)
    //
    if (SUCCEEDED(hr))
    {
        LONG lScaling = 100;

        hr = PropertyManager.AddProperty(WIA_IPS_XSCALING, WIA_IPS_XSCALING_STR, RWRC, lScaling, lScaling, lScaling, lScaling, 0);
        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Failed to initialize WIA_IPS_XSCALING for item %u, hr = 0x%08X", nDocumentHandlingSelect, hr));
        }

        if (SUCCEEDED(hr))
        {
            hr = PropertyManager.AddProperty(WIA_IPS_YSCALING, WIA_IPS_YSCALING_STR, RWRC, lScaling, lScaling, lScaling, lScaling, 0);
            if (FAILED(hr))
            {
                WIAEX_ERROR((g_hInst, "Failed to initialize WIA_IPS_YSCALING for item %u, hr = 0x%08X", nDocumentHandlingSelect, hr));
            }
        }
    }

    //
    // WIA_IPS_MIN_HORIZONTAL_SIZE, WIA_IPS_MAX_HORIZONTAL_SIZE, WIA_IPS_MIN_VERTICAL_SIZE and WIA_IPS_MAX_VERTICAL_SIZE
    //
    // This sample driver supports for both flatbed and feeder the following:
    //
    // Minimum scan region is MIN_SCAN_AREA_WIDTH x MIN_SCAN_AREA_HEIGHT
    // Maximum scan region is MAX_SCAN_AREA_WIDTH x MAX_SCAN_AREA_HEIGHT
    //
    if (SUCCEEDED(hr))
    {
        LONG lMaximumWidth = MAX_SCAN_AREA_WIDTH;

        hr = PropertyManager.AddProperty(WIA_IPS_MAX_HORIZONTAL_SIZE, WIA_IPS_MAX_HORIZONTAL_SIZE_STR, RN, lMaximumWidth);
        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Failed to initialize WIA_IPS_MAX_HORIZONTAL_SIZE for item %u, hr = 0x%08X", nDocumentHandlingSelect, hr));
        }
    }

    if (SUCCEEDED(hr))
    {
        LONG lMaximumHeight = MAX_SCAN_AREA_HEIGHT;

        hr = PropertyManager.AddProperty(WIA_IPS_MAX_VERTICAL_SIZE, WIA_IPS_MAX_VERTICAL_SIZE_STR, RN, lMaximumHeight);
        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Failed to initialize WIA_IPS_MAX_VERTICAL_SIZE for item %u, hr = 0x%08X", nDocumentHandlingSelect, hr));
        }
    }

    if (SUCCEEDED(hr))
    {
        LONG lMinimumWidth = MIN_SCAN_AREA_WIDTH;

        hr = PropertyManager.AddProperty(WIA_IPS_MIN_HORIZONTAL_SIZE, WIA_IPS_MIN_HORIZONTAL_SIZE_STR, RN, lMinimumWidth);
        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Failed to initialize WIA_IPS_MIN_HORIZONTAL_SIZE for item %u, hr = 0x%08X", nDocumentHandlingSelect, hr));
        }
    }

    if (SUCCEEDED(hr))
    {
        LONG lMinimumHeight = MIN_SCAN_AREA_HEIGHT;

        hr = PropertyManager.AddProperty(WIA_IPS_MIN_VERTICAL_SIZE, WIA_IPS_MIN_VERTICAL_SIZE_STR, RN, lMinimumHeight);
        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Failed to initialize WIA_IPS_MIN_VERTICAL_SIZE for item %u, hr = 0x%08X", nDocumentHandlingSelect, hr));
        }
    }

    //
    // WIA_IPS_XPOS, WIA_IPS_YPOS, WIA_IPS_XEXTENT and WIA_IPS_YEXTENT
    //
    // In general, for the flatbed item, valid values are to be initialized from
    // (considering current WIA_IPS_X/YRES and WIA_IPS_X/YSCALING):
    //
    // WIA_IPS_MIN_HORIZONTAL_SIZE,
    // WIA_IPS_MIN_VERTICAL_SIZE,
    // WIA_IPS_MAX_HORIZONTAL_SIZE,
    // WIA_IPS_MAX_VERTICAL_SIZE
    //
    // For the feeder item valid values are to be initialized from
    // the size of the currently selected document size, considering
    // orientation and current WIA_IPS_X/YRES/SCALING:
    //
    // WIA_IPS_PAGE_SIZE
    // WIA_IPS_PAGE_WIDTH/HEIGHT (if WIA_IPS_PAGE_SIZE is set to CUSTOM, default)
    // WIA_IPS_ORIENTATION
    //
    // The default scan region should cover the entire available scan area.
    //

    LONG lMinXExtent = 1;
    LONG lMinYExtent = 1;
    LONG lMaxXExtent = 2;
    LONG lMaxYExtent = 2;

    LONG lXResolution = OPTICAL_RESOLUTION;
    LONG lYResolution = OPTICAL_RESOLUTION;

    if (SUCCEEDED(hr) && (AUTO_SOURCE != nDocumentHandlingSelect))
    {
        //
        // Convert back from 1/1000" values x pixels-per-inch:
        //
        lMinXExtent = (MIN_SCAN_AREA_WIDTH * lXResolution) / 1000;
        if (!lMinXExtent)
        {
            lMinXExtent = 1;
        }
        lMinYExtent = (MIN_SCAN_AREA_HEIGHT * lYResolution) / 1000;
        if (!lMinYExtent)
        {
            lMinYExtent = 1;
        }
        lMaxXExtent = (MAX_SCAN_AREA_WIDTH * lXResolution) / 1000;
        lMaxYExtent = (MAX_SCAN_AREA_HEIGHT * lYResolution) / 1000;

        //
        // IMPORTANT: do not round up!
        //
        // lMaxXExtent = (LONG)((((float)MAX_SCAN_AREA_WIDTH * (float)lXResolution) / 1000.0f) + 0.5f);
        // lMaxYExtent = (LONG)((((float)MAX_SCAN_AREA_HEIGHT * (float)lYResolution) / 1000.0f) + 0.5f);
        //

        if ((lMaxXExtent < 1) || (lMaxYExtent < 1) || (lMinXExtent > lMaxXExtent) || (lMinYExtent > lMaxYExtent))
        {
            hr = E_FAIL;
            WIAEX_ERROR((g_hInst, "Invalid resolution and-or minimum and-or maximum scan area size values, hr = 0x%08X", hr));
        }
    }

    if (SUCCEEDED(hr))
    {
        hr = PropertyManager.AddProperty(WIA_IPS_XPOS, WIA_IPS_XPOS_STR, RWRC, 0, 0, 0, lMaxXExtent - lMinXExtent, 1);
        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Failed to initialize WIA_IPS_XPOS for item %u, hr = 0x%08X", nDocumentHandlingSelect, hr));
        }

        if (SUCCEEDED(hr))
        {
            hr = PropertyManager.AddProperty(WIA_IPS_YPOS, WIA_IPS_YPOS_STR, RWRC, 0, 0, 0, lMaxYExtent - lMinYExtent, 1);
            if (FAILED(hr))
            {
                WIAEX_ERROR((g_hInst, "Failed to initialize WIA_IPS_YPOS for item %u, hr = 0x%08X", nDocumentHandlingSelect, hr));
            }
        }

        if (SUCCEEDED(hr))
        {
            hr = PropertyManager.AddProperty(WIA_IPS_XEXTENT, WIA_IPS_XEXTENT_STR, RWRC, lMaxXExtent, lMaxXExtent, lMinXExtent, lMaxXExtent, 1);
            if (FAILED(hr))
            {
                WIAEX_ERROR((g_hInst, "Failed to initialize WIA_IPS_XEXTENT for item %u, hr = 0x%08X", nDocumentHandlingSelect, hr));
            }
        }

        if (SUCCEEDED(hr))
        {
            hr = PropertyManager.AddProperty(WIA_IPS_YEXTENT, WIA_IPS_YEXTENT_STR, RWRC, lMaxYExtent, lMaxYExtent, lMinYExtent, lMaxYExtent, 1);
            if (FAILED(hr))
            {
                WIAEX_ERROR((g_hInst, "Failed to initialize WIA_IPS_YEXTENT for item %u, hr = 0x%08X", nDocumentHandlingSelect, hr));
            }
        }
    }

    //
    // WIA_IPS_BRIGHTNESS and WIA_IPS_CONTRAST
    //
    // This sample driver simulates brightness and contrast adjustment between
    // a standard range from -1000 to 1000, with a default value of 0.
    //
    if (SUCCEEDED(hr))
    {
        hr = PropertyManager.AddProperty(WIA_IPS_BRIGHTNESS, WIA_IPS_BRIGHTNESS_STR, RWRC, 0, 0, -1000, 1000, 1);
        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Failed to initialize WIA_IPS_BRIGHTNESS for item %u, hr = 0x%08X", nDocumentHandlingSelect, hr));
        }
    }

    if (SUCCEEDED(hr))
    {
        hr = PropertyManager.AddProperty(WIA_IPS_CONTRAST, WIA_IPS_CONTRAST_STR, RWRC, 0, 0, -1000, 1000, 1);
        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Failed to initialize WIA_IPS_CONTRAST for item %u, hr = 0x%08X", nDocumentHandlingSelect, hr));
        }
    }

    //
    // WIA_IPS_ROTATION
    //
    // This sample driver supports only 0 degrees rotation (no actual rotation)
    //
    if (SUCCEEDED(hr))
    {
        CBasicDynamicArray<LONG> lRotationArray;
        lRotationArray.Append(0);

        hr = PropertyManager.AddProperty(WIA_IPS_ROTATION, WIA_IPS_ROTATION_STR, RWLC, lRotationArray[0], lRotationArray[0], &lRotationArray);
        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Failed to initialize WIA_IPS_ROTATION for item %u, hr = 0x%08X", nDocumentHandlingSelect, hr));
        }
    }

    //
    // WIA_IPS_THRESHOLD
    //
    // This sample driver supports only the default value of 128
    //
    if (SUCCEEDED(hr))
    {
        hr = PropertyManager.AddProperty(WIA_IPS_THRESHOLD, WIA_IPS_THRESHOLD_STR, RWRC, 128, 128, 128, 128, 0);
        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Failed to initialize WIA_IPS_THRESHOLD for item %u, hr = 0x%08X", nDocumentHandlingSelect, hr));
        }
    }

    //
    // WIA_IPS_PREVIEW
    //
    if (SUCCEEDED(hr))
    {
        CBasicDynamicArray<LONG> lScanModeArray;
        lScanModeArray.Append(WIA_FINAL_SCAN);
        lScanModeArray.Append(WIA_PREVIEW_SCAN);

        hr = PropertyManager.AddProperty(WIA_IPS_PREVIEW, WIA_IPS_PREVIEW_STR, RWL, lScanModeArray[0], lScanModeArray[0], &lScanModeArray);
        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Failed to initialize WIA_IPS_PREVIEW for item %u, hr = 0x%08X", nDocumentHandlingSelect, hr));
        }
    }

    //
    // WIA_IPS_SHOW_PREVIEW_CONTROL
    //
    if (SUCCEEDED(hr))
    {
        //
        // There is the option to disable the preview control for Feeder but this sample driver is not using it:
        //
        // lShowPreviewControl = (FLAT == nDocumentHandlingSelect) ? WIA_SHOW_PREVIEW_CONTROL : WIA_DONT_SHOW_PREVIEW_CONTROL;
        //

        LONG lShowPreviewControl = WIA_SHOW_PREVIEW_CONTROL;

        hr = PropertyManager.AddProperty(WIA_IPS_SHOW_PREVIEW_CONTROL, WIA_IPS_SHOW_PREVIEW_CONTROL_STR, RN, lShowPreviewControl);
        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Failed to initialize WIA_IPS_SHOW_PREVIEW_CONTROL for item %u, hr = 0x%08X", nDocumentHandlingSelect, hr));
        }
    }

    //
    // WIA_IPS_SUPPORTS_CHILD_ITEM_CREATION
    //
    if (SUCCEEDED(hr))
    {
        BOOL lSupportsChildItem = FALSE;

        hr = PropertyManager.AddProperty(WIA_IPS_SUPPORTS_CHILD_ITEM_CREATION, WIA_IPS_SUPPORTS_CHILD_ITEM_CREATION_STR, RN, lSupportsChildItem);
        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Failed to initialize WIA_IPS_SUPPORTS_CHILD_ITEM_CREATION for item %u, hr = 0x%08X", nDocumentHandlingSelect, hr));
        }
    }

    //
    // WIA_IPS_PHOTOMETRIC_INTERP
    //
    // This sample driver supports only the default value of WIA_PHOTO_WHITE_1
    //
    if (SUCCEEDED(hr))
    {
        CBasicDynamicArray<LONG> lPhotoInterpArray;
        lPhotoInterpArray.Append(WIA_PHOTO_WHITE_1);

        hr = PropertyManager.AddProperty(WIA_IPS_PHOTOMETRIC_INTERP, WIA_IPS_PHOTOMETRIC_INTERP_STR, RWL, lPhotoInterpArray[0], lPhotoInterpArray[0], &lPhotoInterpArray);
        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Failed to initialize WIA_IPS_PHOTOMETRIC_INTERP for item %u, hr = 0x%08X", nDocumentHandlingSelect, hr));
        }
    }

    //
    // Even though deprecated the following "image information" properties are still required
    // for compatibility with existing legacy XP applications, including Scanner and Camera Wizard,
    // applications using the default WIA UI (including Paint) and the TWAIN applications using
    // the WIA driver though the TWAIN - WIA compatibility layer:
    //
    // WIA_IPA_PIXELS_PER_LINE - the image width, in pixels, for the final image
    // WIA_IPA_NUMBER_OF_LINES - the image length, in pixels, for the final image
    // WIA_IPA_BYTES_PER_LINE - line width in bytes matching WIA_IPA_PIXELS_PER_LINE and WIA_IPA_DEPTH
    //
    // All these values must match the exact dimensions of the final image to be transferred
    // to the application, a mismatch could cause unpredictable behaviour, including Divide by Zero
    // and Access Violation errors in the application attempting to receive data that doesn't exist.
    //

    if (SUCCEEDED(hr))
    {
        LONG lPixelsPerLine = lMaxXExtent;

        hr = PropertyManager.AddProperty(WIA_IPA_PIXELS_PER_LINE, WIA_IPA_PIXELS_PER_LINE_STR, RN, lPixelsPerLine);
        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Failed to initialize WIA_IPA_PIXELS_PER_LINE for item %u, hr = 0x%08X", nDocumentHandlingSelect, hr));
        }
    }

    if (SUCCEEDED(hr))
    {
        LONG lNumberOfLines = lMaxYExtent;

        hr = PropertyManager.AddProperty(WIA_IPA_NUMBER_OF_LINES, WIA_IPA_NUMBER_OF_LINES_STR, RN, lNumberOfLines);
        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Failed to initialize WIA_IPA_NUMBER_OF_LINES for item %u, hr = 0x%08X", nDocumentHandlingSelect, hr));
        }
    }

    if (SUCCEEDED(hr))
    {
        //
        // DIB (the default image file transfer format) lines must be DWORD aligned, meaning
        // that each line must be multiple by 4 bytes in length, padded if necessary at the end:
        //
        LONG lBytesPerLine = 2552;

        hr = PropertyManager.AddProperty(WIA_IPA_BYTES_PER_LINE, WIA_IPA_BYTES_PER_LINE_STR, RN, lBytesPerLine);
        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Failed to initialize WIA_IPA_BYTES_PER_LINE for item %u, hr = 0x%08X", nDocumentHandlingSelect, hr));
        }
    }

    //
    // WIA_IPA_BUFFER_SIZE must be supported in order to be able to increase (if needed) the default 64KB value set
    // by the WIA Compatibility Layer in the WIA Service when the driver is used with a legacy WIA 1.0 application:
    //

    if (SUCCEEDED(hr))
    {
        LONG lBufferSize = DEFAULT_BUFFER_SIZE;

        hr = PropertyManager.AddProperty(WIA_IPA_BUFFER_SIZE, WIA_IPA_BUFFER_SIZE_STR, RN, lBufferSize);
        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Failed to initialize WIA_IPA_BUFFER_SIZE for item %u, hr = 0x%08X", nDocumentHandlingSelect, hr));
        }
    }

    //
    // WIA_IPS_AUTO_CROP
    //
    // The sample driver implements WIA_AUTO_CROP_SINGLE  but does not support actual image cropping:
    //
    if (SUCCEEDED(hr))
    {
        CBasicDynamicArray<LONG> lAutoCropArray;
        lAutoCropArray.Append(WIA_AUTO_CROP_DISABLED);
        lAutoCropArray.Append(WIA_AUTO_CROP_SINGLE);

        hr = PropertyManager.AddProperty(WIA_IPS_AUTO_CROP, WIA_IPS_AUTO_CROP_STR, RWL, lAutoCropArray[0], lAutoCropArray[0], &lAutoCropArray);
        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Failed to initialize WIA_IPS_AUTO_CROP for item %u, hr = 0x%08X", nDocumentHandlingSelect, hr));
        }
    }

    //
    // WIA_IPS_OVER_SCAN
    //
    // The sample driver pretends to support overscanning on all directions, however the overscan settings are unfunctional:
    //
    if (SUCCEEDED(hr))
    {
        CBasicDynamicArray<LONG> lOverScanArray;
        lOverScanArray.Append(WIA_OVER_SCAN_DISABLED);
        lOverScanArray.Append(WIA_OVER_SCAN_TOP_BOTTOM);
        lOverScanArray.Append(WIA_OVER_SCAN_LEFT_RIGHT);
        lOverScanArray.Append(WIA_OVER_SCAN_ALL);

        hr = PropertyManager.AddProperty(WIA_IPS_OVER_SCAN, WIA_IPS_OVER_SCAN_STR, RWL, lOverScanArray[0], lOverScanArray[0], &lOverScanArray);
        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Failed to initialize WIA_IPS_OVER_SCAN for item %u, hr = 0x%08X", nDocumentHandlingSelect, hr));
        }
    }

    //
    // WIA_IPS_OVER_SCAN_LEFT, WIA_IPS_OVER_SCAN_RIGHT, WIA_IPS_OVER_SCAN_TOP and WIA_IPS_OVER_SCAN_BOTTOM
    //
    // The sample driver pretends to support overscanning from 0 to 1" on all document sides, in 0.001" increments:
    //

    if (SUCCEEDED(hr))
    {
        hr = PropertyManager.AddPropertyUL(WIA_IPS_OVER_SCAN_LEFT, WIA_IPS_OVER_SCAN_LEFT_STR, RWRC, 0, 0, 0, 1000, 1);
        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Failed to initialize WIA_IPS_OVER_SCAN_LEFT for item %u, hr = 0x%08X", nDocumentHandlingSelect, hr));
        }
    }

    if (SUCCEEDED(hr))
    {
        hr = PropertyManager.AddPropertyUL(WIA_IPS_OVER_SCAN_RIGHT, WIA_IPS_OVER_SCAN_RIGHT_STR, RWRC, 0, 0, 0, 1000, 1);
        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Failed to initialize WIA_IPS_OVER_SCAN_RIGHT for item %u, hr = 0x%08X", nDocumentHandlingSelect, hr));
        }
    }

    if (SUCCEEDED(hr))
    {
        hr = PropertyManager.AddPropertyUL(WIA_IPS_OVER_SCAN_TOP, WIA_IPS_OVER_SCAN_TOP_STR, RWRC, 0, 0, 0, 1000, 1);
        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Failed to initialize WIA_IPS_OVER_SCAN_TOP for item %u, hr = 0x%08X", nDocumentHandlingSelect, hr));
        }
    }

    if (SUCCEEDED(hr))
    {
        hr = PropertyManager.AddPropertyUL(WIA_IPS_OVER_SCAN_BOTTOM, WIA_IPS_OVER_SCAN_BOTTOM_STR, RWRC, 0, 0, 0, 1000, 1);
        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Failed to initialize WIA_IPS_OVER_SCAN_BOTTOM for item %u, hr = 0x%08X", nDocumentHandlingSelect, hr));
        }
    }

    //
    // WIA_IPS_COLOR_DROP
    //
    // The sample driver implements the color-drop properties, however it does not
    // execute any actual color filtering (drop) on the test image:
    //
    if (SUCCEEDED(hr))
    {
        CBasicDynamicArray<LONG> lColorDropArray;
        lColorDropArray.Append(WIA_COLOR_DROP_DISABLED);
        lColorDropArray.Append(WIA_COLOR_DROP_RED);
        lColorDropArray.Append(WIA_COLOR_DROP_GREEN);
        lColorDropArray.Append(WIA_COLOR_DROP_BLUE);
        lColorDropArray.Append(WIA_COLOR_DROP_RGB);

        hr = PropertyManager.AddProperty(WIA_IPS_COLOR_DROP, WIA_IPS_COLOR_DROP_STR, RWL, lColorDropArray[0], lColorDropArray[0], &lColorDropArray);
        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Failed to initialize WIA_IPS_COLOR_DROP for item %u, hr = 0x%08X", nDocumentHandlingSelect, hr));
        }
    }

    //
    // WIA_IPS_COLOR_DROP_MULTI:
    //
    if (SUCCEEDED(hr))
    {
        hr = PropertyManager.AddProperty(WIA_IPS_COLOR_DROP_MULTI, WIA_IPS_COLOR_DROP_MULTI_STR, RN, g_lMaxDropColors);
        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Failed to initialize WIA_IPS_COLOR_DROP_MULTI for item %u, hr = 0x%08X", nDocumentHandlingSelect, hr));
        }
    }

    //
    // WIA_IPS_COLOR_DROP_RED, WIA_IPS_COLOR_DROP_GREEN and WIA_IPS_COLOR_DROP_BLUE:
    //

    if (SUCCEEDED(hr))
    {
        hr = PropertyManager.AddProperty(WIA_IPS_COLOR_DROP_RED, WIA_IPS_COLOR_DROP_RED_STR, RW, g_lMaxDropColors, (LONG *)&g_lDefaultDropColors[0]);
        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Failed to initialize WIA_IPS_COLOR_DROP_RED for item %u, hr = 0x%08X", nDocumentHandlingSelect, hr));
        }
    }

    if (SUCCEEDED(hr))
    {
        hr = PropertyManager.AddProperty(WIA_IPS_COLOR_DROP_GREEN, WIA_IPS_COLOR_DROP_GREEN_STR, RW, g_lMaxDropColors, (LONG *)&g_lDefaultDropColors[0]);
        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Failed to initialize WIA_IPS_COLOR_DROP_GREEN for item %u, hr = 0x%08X", nDocumentHandlingSelect, hr));
        }
    }

    if (SUCCEEDED(hr))
    {
        hr = PropertyManager.AddProperty(WIA_IPS_COLOR_DROP_BLUE, WIA_IPS_COLOR_DROP_BLUE_STR, RW, g_lMaxDropColors, (LONG *)&g_lDefaultDropColors[0]);
        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Failed to initialize WIA_IPS_COLOR_DROP_BLUE for item %u, hr = 0x%08X", nDocumentHandlingSelect, hr));
        }
    }

    //
    // Apply the property changes to the current session's Application Item Tree:
    //

    if (SUCCEEDED(hr))
    {
        hr = PropertyManager.SetItemProperties(pWiasContext);
        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "CWIAPropertyManager::SetItemProperties failed to set WIA item properties for item %u, hr = 0x%08X", nDocumentHandlingSelect, hr));
        }
    }

    WIAEX_TRACE_FUNC_HR;

    return hr;
}

/**************************************************************************\
*
* Initializes the properties specific to the Feeder item.
*
* Parameters:
*
*    pWiasContext - pointer to the item context
*
* Return Value:
*
*    S_OK if successful, an error HRESULT otherwise
*
\**************************************************************************/

HRESULT CWiaDriver::InitializeFeederSpecificProperties(
    _In_ BYTE* pWiasContext)

{
    HRESULT hr = S_OK;
    CWIAPropertyManager PropertyManager;

    if (!pWiasContext)
    {
        hr = E_INVALIDARG;
        WIAEX_ERROR((g_hInst, "CWiaDriver::InitializeFeederSpecificProperties, invalid parameter, hr = 0x%08X", hr));
    }

    WIAEX_TRACE_BEGIN;

    //
    // WIA_IPS_DOCUMENT_HANDLING_SELECT
    //
    LONG lDocumentHandlingSelect = FRONT_ONLY;
    LONG lDocumentHandlingSelectValidValues = FRONT_ONLY | DUPLEX;

    hr = PropertyManager.AddProperty(WIA_IPS_DOCUMENT_HANDLING_SELECT, WIA_IPS_DOCUMENT_HANDLING_SELECT_STR, RWF, lDocumentHandlingSelect, lDocumentHandlingSelectValidValues);
    if (FAILED(hr))
    {
        WIAEX_ERROR((g_hInst, "Failed to initialize WIA_IPS_DOCUMENT_HANDLING_SELECT, hr = 0x%08X", hr));
    }

    //
    // WIA_IPS_SHEET_FEEDER_REGISTRATION
    //
    if (SUCCEEDED(hr))
    {
        LONG lFeederRegistration = LEFT_JUSTIFIED;

        hr = PropertyManager.AddProperty(WIA_IPS_SHEET_FEEDER_REGISTRATION, WIA_IPS_SHEET_FEEDER_REGISTRATION_STR, RN, lFeederRegistration);
        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Failed to initialize WIA_IPS_SHEET_FEEDER_REGISTRATION, hr = 0x%08X", hr));
        }
    }

    //
    // WIA_IPS_PAGES
    //
    // Important: the default value for this property should be ALL_PAGES (0).
    // However legacy XP applications need a WIA_IPS_PAGES > 0 in order to work.
    // For this reason the default WIA_IPS_PAGES is changed to 1. Clients who need
    // to transfer all available images must set WIA_IPS_PAGES to ALL_PAGES.
    //
    if (SUCCEEDED(hr))
    {
        LONG lMaxPages     = 0x7FFFFFFF; //maximum pozitive value for a signed 32-bit integer
        LONG lMinPages     = 0; //ALL_PAGES
        LONG lDefaultPages = 1;

        hr = PropertyManager.AddProperty(WIA_IPS_PAGES, WIA_IPS_PAGES_STR,
            RWR, lDefaultPages, lDefaultPages, lMinPages, lMaxPages, 1);

        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Failed to initialize WIA_IPS_PAGES, hr = 0x%08X", hr));
        }
    }

    LONG lPageWidth = MAX_SCAN_AREA_WIDTH;
    LONG lPageHeight = MAX_SCAN_AREA_HEIGHT;

    //
    // WIA_IPS_PAGE_SIZE
    //
    // This sample driver supports Letter, custom and auto-detect document sizes
    //
    if (SUCCEEDED(hr))
    {
        LONG lDefaultPageSize = WIA_PAGE_CUSTOM;

        hr = PropertyManager.AddProperty(WIA_IPS_PAGE_SIZE, WIA_IPS_PAGE_SIZE_STR, RWL, lDefaultPageSize, lDefaultPageSize, &m_lPortraitSizesArray);
        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Failed to initialize WIA_IPS_PAGE_SIZE, hr = 0x%08X", hr));
        }
    }

    //
    // WIA_IPS_ORIENTATION
    //
    // This sample driver supports only Portrait orientation
    //
    if (SUCCEEDED(hr))
    {
        CBasicDynamicArray<LONG> lOrientationArray;
        lOrientationArray.Append(PORTRAIT);

        hr = PropertyManager.AddProperty(WIA_IPS_ORIENTATION, WIA_IPS_ORIENTATION_STR, RWL, lOrientationArray[0], lOrientationArray[0], &lOrientationArray);
        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Failed to initialize WIA_IPS_ORIENTATION, hr = 0x%08X", hr));
        }
    }

    //
    // WIA_IPS_PAGE_WIDTH
    //
    if (SUCCEEDED(hr))
    {
        hr = PropertyManager.AddProperty(WIA_IPS_PAGE_WIDTH, WIA_IPS_PAGE_WIDTH_STR, RN, lPageWidth);
        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Failed to initialize WIA_IPS_ORIENTATION, hr = 0x%08X", hr));
        }
    }

    //
    // WIA_IPS_PAGE_HEIGHT
    //
    if (SUCCEEDED(hr))
    {
        hr = PropertyManager.AddProperty(WIA_IPS_PAGE_HEIGHT, WIA_IPS_PAGE_HEIGHT_STR, RN, lPageHeight);
        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Failed to initialize WIA_IPS_ORIENTATION, hr = 0x%08X", hr));
        }
    }

    //
    // WIA_IPS_JOB_SEPARATORS
    //
    // When job separators are enabled, the sample driver simulates a job separator page every JOB_SEPARATOR_AT_PAGE pages scanned:
    //
    if (SUCCEEDED(hr))
    {
        CBasicDynamicArray<LONG> lJobSeparatorsArray;
        lJobSeparatorsArray.Append(WIA_SEPARATOR_DISABLED);
        lJobSeparatorsArray.Append(WIA_SEPARATOR_DETECT_SCAN_CONTINUE);
        lJobSeparatorsArray.Append(WIA_SEPARATOR_DETECT_SCAN_STOP);
        lJobSeparatorsArray.Append(WIA_SEPARATOR_DETECT_NOSCAN_CONTINUE);
        lJobSeparatorsArray.Append(WIA_SEPARATOR_DETECT_NOSCAN_STOP);

        hr = PropertyManager.AddProperty(WIA_IPS_JOB_SEPARATORS, WIA_IPS_JOB_SEPARATORS_STR, RWL, lJobSeparatorsArray[0], lJobSeparatorsArray[0], &lJobSeparatorsArray);
        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Failed to initialize WIA_IPS_JOB_SEPARATORS, hr = 0x%08X", hr));
        }
    }

    //
    // WIA_IPS_LONG_DOCUMENT
    //
    // The sample driver implemenmts the property but does not implement the actual functionality:
    //
    if (SUCCEEDED(hr))
    {
        CBasicDynamicArray<LONG> lLongDocArray;
        lLongDocArray.Append(WIA_LONG_DOCUMENT_DISABLED);
        lLongDocArray.Append(WIA_LONG_DOCUMENT_ENABLED);
        lLongDocArray.Append(WIA_LONG_DOCUMENT_SPLIT);

        hr = PropertyManager.AddProperty(WIA_IPS_LONG_DOCUMENT, WIA_IPS_LONG_DOCUMENT_STR, RWL, lLongDocArray[0], lLongDocArray[0], &lLongDocArray);
        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Failed to initialize WIA_IPS_LONG_DOCUMENT, hr = 0x%08X", hr));
        }
    }

    //
    // WIA_IPS_BLANK_PAGES
    //
    // The sample driver implements the property but does not implement actual blank page detection functionality:
    //
    if (SUCCEEDED(hr))
    {
        CBasicDynamicArray<LONG> lBlankPagesArray;
        lBlankPagesArray.Append(WIA_BLANK_PAGE_DETECTION_DISABLED);
        lBlankPagesArray.Append(WIA_BLANK_PAGE_DISCARD);
        lBlankPagesArray.Append(WIA_BLANK_PAGE_JOB_SEPARATOR);

        hr = PropertyManager.AddProperty(WIA_IPS_BLANK_PAGES, WIA_IPS_BLANK_PAGES_STR, RWL, lBlankPagesArray[0], lBlankPagesArray[0], &lBlankPagesArray);
        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Failed to initialize WIA_IPS_BLANK_PAGES, hr = 0x%08X", hr));
        }
    }

    //
    // WIA_IPS_BLANK_PAGES_SENSITIVITY
    //
    // This sample driver reports a range of supported values (which are not functional)
    // between 0 and 10 inclusive, with 5 being the default sensitivity:
    //
    if (SUCCEEDED(hr))
    {
        hr = PropertyManager.AddProperty(WIA_IPS_BLANK_PAGES_SENSITIVITY, WIA_IPS_BLANK_PAGES_SENSITIVITY_STR, RWR, 5, 5, 1, 10, 1);

        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Failed to initialize WIA_IPS_BLANK_PAGES_SENSITIVITY, hr = 0x%08X", hr));
        }
    }

    //
    // WIA_IPS_MULTI_FEED
    //
    // When multi-feed detection is enabled, the sample driver simulates a multi-feed condition every MULTI_FEED_AT_PAGE pages scanned:
    //
    if (SUCCEEDED(hr))
    {
        CBasicDynamicArray<LONG> lMultiFeedArray;
        lMultiFeedArray.Append(WIA_MULTI_FEED_DETECT_DISABLED);
        lMultiFeedArray.Append(WIA_MULTI_FEED_DETECT_STOP_ERROR);
        lMultiFeedArray.Append(WIA_MULTI_FEED_DETECT_STOP_SUCCESS);
        lMultiFeedArray.Append(WIA_MULTI_FEED_DETECT_CONTINUE);

        hr = PropertyManager.AddProperty(WIA_IPS_MULTI_FEED, WIA_IPS_MULTI_FEED_STR, RWL, lMultiFeedArray[0], lMultiFeedArray[0], &lMultiFeedArray);
        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Failed to initialize WIA_IPS_MULTI_FEED, hr = 0x%08X", hr));
        }
    }

    //
    // WIA_IPS_MULTI_FEED_SENSITIVITY
    //
    // This sample driver reports a range of supported values (which are not functional)
    // between 0 and 10 inclusive, with 5 being the default sensitivity:
    //
    if (SUCCEEDED(hr))
    {
        hr = PropertyManager.AddProperty(WIA_IPS_MULTI_FEED_SENSITIVITY, WIA_IPS_MULTI_FEED_SENSITIVITY_STR, RWR, 5, 5, 1, 10, 1);

        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Failed to initialize WIA_IPS_MULTI_FEED_SENSITIVITY, hr = 0x%08X", hr));
        }
    }

    //
    // WIA_IPS_ALARM
    //
    // This sample driver does pretend to support one kind of audible alarm (beep) to signal
    // when a multi-feed conditions is detected, not functional:
    //
    if (SUCCEEDED(hr))
    {
        CBasicDynamicArray<LONG> lAlarmArray;
        lAlarmArray.Append(WIA_ALARM_NONE);
        lAlarmArray.Append(WIA_ALARM_BEEP1);

        hr = PropertyManager.AddProperty(WIA_IPS_ALARM, WIA_IPS_ALARM_STR, RWL, lAlarmArray[0], lAlarmArray[0], &lAlarmArray);
        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Failed to initialize WIA_IPS_ALARM, hr = 0x%08X", hr));
        }
    }

    //
    // WIA_IPS_SCAN_AHEAD:
    //
    if (SUCCEEDED(hr))
    {
        CBasicDynamicArray<LONG> lScanAheadArray;
        lScanAheadArray.Append(WIA_SCAN_AHEAD_DISABLED);
        lScanAheadArray.Append(WIA_SCAN_AHEAD_ENABLED);

        hr = PropertyManager.AddProperty(WIA_IPS_SCAN_AHEAD, WIA_IPS_SCAN_AHEAD_STR, RWL, lScanAheadArray[0], lScanAheadArray[0], &lScanAheadArray);
        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Failed to initialize WIA_IPS_SCAN_AHEAD, hr = 0x%08X", hr));
        }
    }

    //
    // WIA_IPS_SCAN_AHEAD_CAPACITY:
    //
    if (SUCCEEDED(hr))
    {
        ULONG ulScanAheadCapacity = 0; //undefined

        hr = PropertyManager.AddPropertyUL(WIA_IPS_SCAN_AHEAD_CAPACITY, WIA_IPS_SCAN_AHEAD_CAPACITY_STR, RN, ulScanAheadCapacity);

        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Failed to initialize WIA_IPS_SCAN_AHEAD_CAPACITY, hr = 0x%08X", hr));
        }
    }

    //
    // WIA_IPS_FEEDER_CONTROL
    //
    // The sample driver pretents to support manual feeder motor control
    // (the WIA_COMMAND_START_FEEDER and the WIA_COMMAND_STOP_FEEDER commands):
    //
    if (SUCCEEDED(hr))
    {
        CBasicDynamicArray<LONG> lFeederControlArray;
        lFeederControlArray.Append(WIA_FEEDER_CONTROL_AUTO);
        lFeederControlArray.Append(WIA_FEEDER_CONTROL_MANUAL);

        hr = PropertyManager.AddProperty(WIA_IPS_FEEDER_CONTROL, WIA_IPS_FEEDER_CONTROL_STR, RWL, lFeederControlArray[0], lFeederControlArray[0], &lFeederControlArray);
        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Failed to initialize WIA_IPS_SCAN_AHEAD, hr = 0x%08X", hr));
        }
    }

    //
    // Apply the property changes to the current session's Application Item Tree:
    //
    if (SUCCEEDED(hr))
    {
        hr = PropertyManager.SetItemProperties(pWiasContext);
        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "CWIAPropertyManager::SetItemProperties failed to set WIA item properties for feeder, hr = 0x%08X", hr));
        }
    }

    WIAEX_TRACE_FUNC_HR;

    return hr;
}

/**************************************************************************\
*
* Initializes the properties specific to the Imprinter and Endorser items.
*
* Parameters:
*
*    pWiasContext            - pointer to the item context
*    nDocumentHandlingSelect - IMPRINTER or ENDORSER (defined in wiadef.h)
*
* Return Value:
*
*    S_OK if successful, an error HRESULT otherwise
*
\**************************************************************************/

HRESULT CWiaDriver::InitializeImprinterEndorserProperties(
    _In_ BYTE*     pWiasContext,
    UINT           nDocumentHandlingSelect)
{
    HRESULT hr = S_OK;
    CWIAPropertyManager PropertyManager;

    if ((!pWiasContext) || ((IMPRINTER != nDocumentHandlingSelect) && (ENDORSER != nDocumentHandlingSelect)))
    {
        hr = E_INVALIDARG;
        WIAEX_ERROR((g_hInst, "CWiaDriver::InitializeImprinterEndorserProperties, invalid parameter, hr = 0x%08X", hr));
    }

    WIAEX_TRACE_BEGIN;

    //
    // WIA_IPS_PRINTER_ENDORSER
    //
    // This sample driver pretends to have an imprinter on the front side of the feeder and an endorser on the back side
    //
    if (SUCCEEDED(hr))
    {
        CBasicDynamicArray<LONG> lPrinterEndorserArray;
        lPrinterEndorserArray.Append(WIA_PRINTER_ENDORSER_DISABLED);
        lPrinterEndorserArray.Append(WIA_PRINTER_ENDORSER_AUTO);
        lPrinterEndorserArray.Append((IMPRINTER == nDocumentHandlingSelect) ? WIA_PRINTER_ENDORSER_FEEDER_FRONT : WIA_PRINTER_ENDORSER_FEEDER_BACK);

        hr = PropertyManager.AddProperty(WIA_IPS_PRINTER_ENDORSER, WIA_IPS_PRINTER_ENDORSER_STR, RWL, lPrinterEndorserArray[0], lPrinterEndorserArray[0], &lPrinterEndorserArray);
        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Failed to initialize WIA_IPS_PRINTER_ENDORSER for item %u, hr = 0x%08X", nDocumentHandlingSelect, hr));
        }
    }

    //
    // WIA_IPS_PRINTER_ENDORSER_ORDER
    //
    // The sample imprinter operates after scan, the sample endorser operates before scan
    //
    if (SUCCEEDED(hr))
    {
        LONG lPrinterEndorserOrder = (IMPRINTER == nDocumentHandlingSelect) ? WIA_PRINTER_ENDORSER_AFTER_SCAN : WIA_PRINTER_ENDORSER_BEFORE_SCAN;

        hr = PropertyManager.AddProperty(WIA_IPS_PRINTER_ENDORSER_ORDER, WIA_IPS_PRINTER_ENDORSER_ORDER_STR, RN, lPrinterEndorserOrder);
        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Failed to initialize WIA_IPS_PRINTER_ENDORSER_ORDER for item %u, hr = 0x%08X", nDocumentHandlingSelect, hr));
        }
    }

    //
    // WIA_IPS_PRINTER_ENDORSER_COUNTER
    //
    if (SUCCEEDED(hr))
    {
        hr = PropertyManager.AddPropertyUL(WIA_IPS_PRINTER_ENDORSER_COUNTER , WIA_IPS_PRINTER_ENDORSER_COUNTER_STR, RWRC, 0, 0, 0, 0xFFFFFFFF, 1);
        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Failed to initialize WIA_IPS_PRINTER_ENDORSER_COUNTER for item %u, hr = 0x%08X", nDocumentHandlingSelect, hr));
        }

    }

    //
    // WIA_IPS_PRINTER_ENDORSER_STEP
    //
    if (SUCCEEDED(hr))
    {
        hr = PropertyManager.AddPropertyUL(WIA_IPS_PRINTER_ENDORSER_STEP, WIA_IPS_PRINTER_ENDORSER_STEP_STR, RWRC, 1, 1, 1, 0xFFFFFFFF, 1);
        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Failed to initialize WIA_IPS_PRINTER_ENDORSER_STEP for item %u, hr = 0x%08X", nDocumentHandlingSelect, hr));
        }

    }

    //
    // WIA_IPS_PRINTER_ENDORSER_XOFFSET and WIA_IPS_PRINTER_ENDORSER_YOFFSET
    //
    // This sample driver pretends to support from 0" to 3" inclusive imprinter and endorser offsets, in 0.001" step increments
    //
    if (SUCCEEDED(hr))
    {
        hr = PropertyManager.AddPropertyUL(WIA_IPS_PRINTER_ENDORSER_XOFFSET, WIA_IPS_PRINTER_ENDORSER_XOFFSET_STR, RWRC, 0, 0, 0, 3000, 1);
        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Failed to initialize WIA_IPS_PRINTER_ENDORSER_XOFFSET for item %u, hr = 0x%08X", nDocumentHandlingSelect, hr));
        }
        else
        {
            hr = PropertyManager.AddPropertyUL(WIA_IPS_PRINTER_ENDORSER_YOFFSET, WIA_IPS_PRINTER_ENDORSER_YOFFSET_STR, RWRC, 0, 0, 0, 3000, 1);
            if (FAILED(hr))
            {
                WIAEX_ERROR((g_hInst, "Failed to initialize WIA_IPS_PRINTER_ENDORSER_YOFFSET for item %u, hr = 0x%08X", nDocumentHandlingSelect, hr));
            }
        }
    }

    //
    // WIA_IPS_ROTATION
    //
    // This sample driver pretends to support all standard 90' rotation values for its imprinter and endorser units
    //
    if (SUCCEEDED(hr))
    {
        CBasicDynamicArray<LONG> lRotationArray;
        lRotationArray.Append(PORTRAIT);
        lRotationArray.Append(LANDSCAPE);
        lRotationArray.Append(ROT180);
        lRotationArray.Append(ROT270);

        hr = PropertyManager.AddProperty(WIA_IPS_ROTATION, WIA_IPS_ROTATION_STR, RWL, lRotationArray[0], lRotationArray[0], &lRotationArray);
        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Failed to initialize WIA_IPS_ROTATION for item %u, hr = 0x%08X", nDocumentHandlingSelect, hr));
        }
    }

    //
    // WIA_IPS_PRINTER_ENDORSER_NUM_LINES
    //
    // This sample driver pretends that supports only one line of text for the imprinter and for the endorser
    //
    if (SUCCEEDED(hr))
    {
        hr = PropertyManager.AddPropertyUL(WIA_IPS_PRINTER_ENDORSER_NUM_LINES, WIA_IPS_PRINTER_ENDORSER_NUM_LINES_STR, RN, 1);
        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Failed to initialize WIA_IPS_PRINTER_ENDORSER_NUM_LINES for item %u, hr = 0x%08X", nDocumentHandlingSelect, hr));
        }
    }

    //
    // WIA_IPS_PRINTER_ENDORSER_STRING
    //
    if (SUCCEEDED(hr))
    {
        BSTR bstrPrinterEndorser = SysAllocString((IMPRINTER == nDocumentHandlingSelect) ? L"Sample imprinter text" : L"Sample endorser text");
        if (bstrPrinterEndorser)
        {
            hr = PropertyManager.AddProperty(WIA_IPS_PRINTER_ENDORSER_STRING, WIA_IPS_PRINTER_ENDORSER_STRING_STR, RW, bstrPrinterEndorser);

            SysFreeString(bstrPrinterEndorser);
        }
        else
        {
            hr = E_OUTOFMEMORY;
            WIAEX_ERROR((g_hInst, "Could not allocate memory for the the WIA_IPS_PRINTER_ENDORSER_STRING property value, hr = 0x%08X", hr));
        }

        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Failed to initialize WIA_IPS_PRINTER_ENDORSER_STRING for item %u, hr = 0x%08X", nDocumentHandlingSelect, hr));
        }
    }

    //
    // WIA_IPS_PRINTER_ENDORSER_VALID_CHARACTERS
    //
    if (SUCCEEDED(hr))
    {
        BSTR bstrValidChars = SysAllocString((IMPRINTER == nDocumentHandlingSelect) ? SAMPLE_IMPRINTER_VALID_CHARS : SAMPLE_ENDORSER_VALID_CHARS);
        if (bstrValidChars)
        {
            hr = PropertyManager.AddProperty(WIA_IPS_PRINTER_ENDORSER_VALID_CHARACTERS, WIA_IPS_PRINTER_ENDORSER_VALID_CHARACTERS_STR, RN, bstrValidChars);

            SysFreeString(bstrValidChars);
            bstrValidChars = NULL;
        }
        else
        {
            hr = E_OUTOFMEMORY;
            WIAEX_ERROR((g_hInst, "Could not allocate memory for the the IA_IPS_PRINTER_ENDORSER_VALID_CHARACTERS property value, hr = 0x%08X", hr));
        }

        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Failed to initialize WIA_IPS_PRINTER_ENDORSER_VALID_CHARACTERS for item %u, hr = 0x%08X", nDocumentHandlingSelect, hr));
        }
    }

    //
    // WIA_IPS_PRINTER_ENDORSER_VALID_FORMAT_SPECIFIERS
    //
    // This sample driver implements this optional property only for the imprinter
    // where it implements a sub-set of all the possible standard values.
    //
    if (SUCCEEDED(hr) && (IMPRINTER == nDocumentHandlingSelect))
    {
        LONG lFormatSpecs[] = {
            WIA_PRINT_DATE,
            WIA_PRINT_YEAR,
            WIA_PRINT_MONTH,
            WIA_PRINT_DAY,
            WIA_PRINT_WEEK_DAY,
            WIA_PRINT_TIME_24H,
            WIA_PRINT_HOUR_24H,
            WIA_PRINT_MINUTE,
            WIA_PRINT_SECOND,
            WIA_PRINT_PAGE_COUNT};
        ULONG ulFormatSpecs = ARRAYSIZE(lFormatSpecs);

        hr = PropertyManager.AddProperty(WIA_IPS_PRINTER_ENDORSER_VALID_FORMAT_SPECIFIERS, WIA_IPS_PRINTER_ENDORSER_VALID_FORMAT_SPECIFIERS_STR, RN, ulFormatSpecs, lFormatSpecs);
        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Failed to initialize WIA_IPS_PRINTER_ENDORSER_VALID_FORMAT_SPECIFIERS for item %u, hr = 0x%08X", nDocumentHandlingSelect, hr));
        }
    }

    //
    // WIA_IPS_PRINTER_ENDORSER_PADDING
    //
    // This sample driver does pretend to support all imprinter/endorser padding values but does not really apply padding:
    //
    if (SUCCEEDED(hr))
    {
        CBasicDynamicArray<LONG> lPaddingArray;
        lPaddingArray.Append(WIA_PRINT_PADDING_NONE);
        lPaddingArray.Append(WIA_PRINT_PADDING_ZERO);
        lPaddingArray.Append(WIA_PRINT_PADDING_BLANK);

        hr = PropertyManager.AddProperty(WIA_IPS_PRINTER_ENDORSER_PADDING, WIA_IPS_PRINTER_ENDORSER_PADDING_STR, RWL, lPaddingArray[0], lPaddingArray[0], &lPaddingArray);
        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Failed to initialize WIA_IPS_PRINTER_ENDORSER_PADDING for item %u, hr = 0x%08X", nDocumentHandlingSelect, hr));
        }
    }

    //
    // WIA_IPS_PRINTER_ENDORSER_FONT_TYPE
    //
    // This sample driver does support all font type values but does not apply font type changes.
    //
    if (SUCCEEDED(hr))
    {
        CBasicDynamicArray<LONG> lFontTypeArray;
        lFontTypeArray.Append(WIA_PRINT_FONT_NORMAL);
        lFontTypeArray.Append(WIA_PRINT_FONT_BOLD);
        lFontTypeArray.Append(WIA_PRINT_FONT_EXTRA_BOLD);
        lFontTypeArray.Append(WIA_PRINT_FONT_ITALIC_BOLD);
        lFontTypeArray.Append(WIA_PRINT_FONT_ITALIC_EXTRA_BOLD);
        lFontTypeArray.Append(WIA_PRINT_FONT_ITALIC);
        lFontTypeArray.Append(WIA_PRINT_FONT_SMALL);
        lFontTypeArray.Append(WIA_PRINT_FONT_SMALL_BOLD);
        lFontTypeArray.Append(WIA_PRINT_FONT_SMALL_EXTRA_BOLD);
        lFontTypeArray.Append(WIA_PRINT_FONT_SMALL_ITALIC_BOLD);
        lFontTypeArray.Append(WIA_PRINT_FONT_SMALL_ITALIC_EXTRA_BOLD);
        lFontTypeArray.Append(WIA_PRINT_FONT_SMALL_ITALIC);
        lFontTypeArray.Append(WIA_PRINT_FONT_LARGE);
        lFontTypeArray.Append(WIA_PRINT_FONT_LARGE_BOLD);
        lFontTypeArray.Append(WIA_PRINT_FONT_LARGE_EXTRA_BOLD);
        lFontTypeArray.Append(WIA_PRINT_FONT_LARGE_ITALIC_BOLD);
        lFontTypeArray.Append(WIA_PRINT_FONT_LARGE_ITALIC_EXTRA_BOLD);
        lFontTypeArray.Append(WIA_PRINT_FONT_LARGE_ITALIC);

        hr = PropertyManager.AddProperty(WIA_IPS_PRINTER_ENDORSER_FONT_TYPE, WIA_IPS_PRINTER_ENDORSER_FONT_TYPE_STR, RWL, lFontTypeArray[0], lFontTypeArray[0], &lFontTypeArray);
        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Failed to initialize WIA_IPS_PRINTER_ENDORSER_FONT_TYPE for item %u, hr = 0x%08X", nDocumentHandlingSelect, hr));
        }
    }

    //
    // WIA_IPS_PRINTER_ENDORSER_CHARACTER_ROTATION
    //
    // This sample driver pretends to support all possible values, but does not actually apply any character rotation.
    //
    if (SUCCEEDED(hr))
    {
        CBasicDynamicArray<LONG> lCharRotatationArray;
        lCharRotatationArray.Append(PORTRAIT);
        lCharRotatationArray.Append(LANDSCAPE);
        lCharRotatationArray.Append(ROT180);
        lCharRotatationArray.Append(ROT270);

        hr = PropertyManager.AddProperty(WIA_IPS_PRINTER_ENDORSER_CHARACTER_ROTATION, WIA_IPS_PRINTER_ENDORSER_CHARACTER_ROTATION_STR,
            RWL, lCharRotatationArray[0], lCharRotatationArray[0], &lCharRotatationArray);
        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Failed to initialize WIA_IPS_PRINTER_ENDORSER_CHARACTER_ROTATION for item %u, hr = 0x%08X", nDocumentHandlingSelect, hr));
        }
    }


    //
    // WIA_IPS_PRINTER_ENDORSER_MAX_CHARACTERS
    //
    // This sample driver pretends to support a maximum number of characters of 0xFFFFFFFF (unrealistic) for the imprinter and endorser.
    //
    if (SUCCEEDED(hr))
    {
        hr = PropertyManager.AddPropertyUL(WIA_IPS_PRINTER_ENDORSER_MAX_CHARACTERS, WIA_IPS_PRINTER_ENDORSER_MAX_CHARACTERS_STR, RN, 0xFFFFFFFF);
        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Failed to initialize WIA_IPS_PRINTER_ENDORSER_MAX_CHARACTERS for item %u, hr = 0x%08X", nDocumentHandlingSelect, hr));
        }
    }

    //
    // WIA_IPS_PRINTER_ENDORSER_INK
    //
    // This sample driver hard-codes a value of 50% (half capacity remaining) for the imprinter and endorser ink.
    //
    if (SUCCEEDED(hr))
    {
        hr = PropertyManager.AddPropertyUL(WIA_IPS_PRINTER_ENDORSER_INK, WIA_IPS_PRINTER_ENDORSER_INK_STR, RN, 50);
        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Failed to initialize WIA_IPS_PRINTER_ENDORSER_INK for item %u, hr = 0x%08X", nDocumentHandlingSelect, hr));
        }
    }

    //
    // WIA_IPS_PRINTER_ENDORSER_TEXT_UPLOAD
    //
    // This sample driver reports to support imprinter/endorser text upload to show how an upload
    // transfer is to be executed, however the uploaded data is not retained/applied.
    //
    if (SUCCEEDED(hr))
    {
        hr = PropertyManager.AddProperty(WIA_IPS_PRINTER_ENDORSER_TEXT_UPLOAD, WIA_IPS_PRINTER_ENDORSER_TEXT_UPLOAD_STR, RN, 1);
        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Failed to initialize WIA_IPS_PRINTER_ENDORSER_TEXT_UPLOAD for item %u, hr = 0x%08X", nDocumentHandlingSelect, hr));
        }
    }

    //
    // WIA_IPS_PRINTER_ENDORSER_TEXT_DOWNLOAD
    //
    // This sample driver reports to support imprinter/endorser text download to show how a download
    // transfer is to be executed, however the downloaded data is always the same/fixed, and is not
    // modified to match WIA_IPS_PRINTER_ENDORSER_STRING.
    //
    if (SUCCEEDED(hr))
    {
        hr = PropertyManager.AddProperty(WIA_IPS_PRINTER_ENDORSER_TEXT_DOWNLOAD, WIA_IPS_PRINTER_ENDORSER_TEXT_DOWNLOAD_STR, RN, 1);
        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Failed to initialize WIA_IPS_PRINTER_ENDORSER_TEXT_DOWNLOAD for item %u, hr = 0x%08X", nDocumentHandlingSelect, hr));
        }
    }

    //
    // WIA_IPS_PRINTER_ENDORSER_GRAPHICS
    //
    if (SUCCEEDED(hr))
    {
        hr = PropertyManager.AddProperty(WIA_IPS_PRINTER_ENDORSER_GRAPHICS, WIA_IPS_PRINTER_ENDORSER_GRAPHICS_STR, RN, 1);
        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Failed to initialize WIA_IPS_PRINTER_ENDORSER_GRAPHICS for item %u, hr = 0x%08X", nDocumentHandlingSelect, hr));
        }
    }

    //
    // WIA_IPS_PRINTER_ENDORSER_GRAPHICS_POSITION
    //
    if (SUCCEEDED(hr))
    {
        CBasicDynamicArray<LONG> lPositionArray;
        lPositionArray.Append(WIA_PRINTER_ENDORSER_GRAPHICS_DEVICE_DEFAULT);
        lPositionArray.Append(WIA_PRINTER_ENDORSER_GRAPHICS_LEFT);
        lPositionArray.Append(WIA_PRINTER_ENDORSER_GRAPHICS_RIGHT);
        lPositionArray.Append(WIA_PRINTER_ENDORSER_GRAPHICS_TOP);
        lPositionArray.Append(WIA_PRINTER_ENDORSER_GRAPHICS_BOTTOM);
        lPositionArray.Append(WIA_PRINTER_ENDORSER_GRAPHICS_TOP_LEFT);
        lPositionArray.Append(WIA_PRINTER_ENDORSER_GRAPHICS_TOP_RIGHT);
        lPositionArray.Append(WIA_PRINTER_ENDORSER_GRAPHICS_BOTTOM_LEFT);
        lPositionArray.Append(WIA_PRINTER_ENDORSER_GRAPHICS_BOTTOM_RIGHT);
        lPositionArray.Append(WIA_PRINTER_ENDORSER_GRAPHICS_BACKGROUND);

        hr = PropertyManager.AddProperty(WIA_IPS_PRINTER_ENDORSER_GRAPHICS_POSITION, WIA_IPS_PRINTER_ENDORSER_GRAPHICS_POSITION_STR, RWL, lPositionArray[0], lPositionArray[0], &lPositionArray);
        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Failed to initialize WIA_IPS_ROTATION for item %u, hr = 0x%08X", nDocumentHandlingSelect, hr));
        }
    }

    //
    // WIA_IPS_PRINTER_ENDORSER_GRAPHICS_MIN_WIDTH
    // WIA_IPS_PRINTER_ENDORSER_GRAPHICS_MIN_HEIGHT
    // WIA_IPS_PRINTER_ENDORSER_GRAPHICS_MAX_WIDTH
    // WIA_IPS_PRINTER_ENDORSER_GRAPHICS_MAX_HEIGHT
    //
    // This sample driver supports a fixed/predefined graphics size that match the sample imprinter/endorser test image
    //
    if (SUCCEEDED(hr))
    {
        hr = PropertyManager.AddPropertyUL(WIA_IPS_PRINTER_ENDORSER_GRAPHICS_MIN_WIDTH, WIA_IPS_PRINTER_ENDORSER_GRAPHICS_MIN_WIDTH_STR, RN, IMPRINTER_MIN_WIDTH);
        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Failed to initialize WIA_IPS_PRINTER_ENDORSER_GRAPHICS_MIN_WIDTH for item %u, hr = 0x%08X", nDocumentHandlingSelect, hr));
        }

        if (SUCCEEDED(hr))
        {
            hr = PropertyManager.AddPropertyUL(WIA_IPS_PRINTER_ENDORSER_GRAPHICS_MAX_WIDTH, WIA_IPS_PRINTER_ENDORSER_GRAPHICS_MAX_WIDTH_STR, RN, IMPRINTER_MAX_WIDTH);
            if (FAILED(hr))
            {
                WIAEX_ERROR((g_hInst, "Failed to initialize WIA_IPS_PRINTER_ENDORSER_GRAPHICS_MIN_WIDTH for item %u, hr = 0x%08X", nDocumentHandlingSelect, hr));
            }
        }

        if (SUCCEEDED(hr))
        {
            hr = PropertyManager.AddPropertyUL(WIA_IPS_PRINTER_ENDORSER_GRAPHICS_MIN_HEIGHT, WIA_IPS_PRINTER_ENDORSER_GRAPHICS_MIN_HEIGHT_STR, RN, IMPRINTER_MIN_HEIGHT);
            if (FAILED(hr))
            {
                WIAEX_ERROR((g_hInst, "Failed to initialize WIA_IPS_PRINTER_ENDORSER_GRAPHICS_MIN_HEIGHT for item %u, hr = 0x%08X", nDocumentHandlingSelect, hr));
            }
        }

        if (SUCCEEDED(hr))
        {
            hr = PropertyManager.AddPropertyUL(WIA_IPS_PRINTER_ENDORSER_GRAPHICS_MAX_HEIGHT, WIA_IPS_PRINTER_ENDORSER_GRAPHICS_MAX_HEIGHT_STR, RN, IMPRINTER_MAX_HEIGHT);
            if (FAILED(hr))
            {
                WIAEX_ERROR((g_hInst, "Failed to initialize WIA_IPS_PRINTER_ENDORSER_GRAPHICS_MAX_HEIGHT for item %u, hr = 0x%08X", nDocumentHandlingSelect, hr));
            }
        }
    }

    //
    // WIA_IPS_PRINTER_ENDORSER_MAX_GRAPHICS
    //
    // This sample driver pretends to support a maximum number of graphics of 1 for its imprinter and endorser.
    //
    if (SUCCEEDED(hr))
    {
        hr = PropertyManager.AddPropertyUL(WIA_IPS_PRINTER_ENDORSER_MAX_GRAPHICS, WIA_IPS_PRINTER_ENDORSER_MAX_GRAPHICS_STR, RN, 1);
        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Failed to initialize WIA_IPS_PRINTER_ENDORSER_MAX_GRAPHICS for item %u, hr = 0x%08X", nDocumentHandlingSelect, hr));
        }
    }

    //
    // WIA_IPS_PRINTER_ENDORSER_GRAPHICS_UPLOAD
    //
    if (SUCCEEDED(hr))
    {
        hr = PropertyManager.AddProperty(WIA_IPS_PRINTER_ENDORSER_GRAPHICS_UPLOAD, WIA_IPS_PRINTER_ENDORSER_GRAPHICS_UPLOAD_STR, RN, 1);
        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Failed to initialize WIA_IPS_PRINTER_ENDORSER_GRAPHICS_UPLOAD for item %u, hr = 0x%08X", nDocumentHandlingSelect, hr));
        }
    }

    //
    // WIA_IPS_PRINTER_ENDORSER_GRAPHICS_DOWNLOAD
    //
    if (SUCCEEDED(hr))
    {
        hr = PropertyManager.AddProperty(WIA_IPS_PRINTER_ENDORSER_GRAPHICS_DOWNLOAD, WIA_IPS_PRINTER_ENDORSER_GRAPHICS_DOWNLOAD_STR, RN, 1);
        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Failed to initialize WIA_IPS_PRINTER_ENDORSER_GRAPHICS_DOWNLOAD for item %u, hr = 0x%08X", nDocumentHandlingSelect, hr));
        }
    }

    //
    // WIA_IPA_DATATYPE
    //
    // The sample driver supports 1-bpp BW graphics data for the imprinter and the endorser.
    //
    if (SUCCEEDED(hr))
    {
        CBasicDynamicArray<LONG> lDataTypeArray;
        lDataTypeArray.Append(WIA_DATA_DITHER);

        hr = PropertyManager.AddProperty(WIA_IPA_DATATYPE, WIA_IPA_DATATYPE_STR, RWL, lDataTypeArray[0], lDataTypeArray[0], &lDataTypeArray);
        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Failed to initialize WIA_IPA_DATATYPE for item %u, hr = 0x%08X", nDocumentHandlingSelect, hr));
        }
    }

    //
    // WIA_IPA_DEPTH
    //
    if (SUCCEEDED(hr))
    {
        CBasicDynamicArray<LONG> lDepthArray;
        lDepthArray.Append(1);

        hr = PropertyManager.AddProperty(WIA_IPA_DEPTH , WIA_IPA_DEPTH_STR, RWLC, lDepthArray[0], lDepthArray[0], &lDepthArray);
        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Failed to initialize WIA_IPA_DEPTH for item %u, hr = 0x%08X", nDocumentHandlingSelect, hr));
        }
    }

    //
    // WIA_IPA_CHANNELS_PER_PIXEL
    //
    if (SUCCEEDED(hr))
    {
        LONG lChannelsPerPixel = 1;

        hr = PropertyManager.AddProperty(WIA_IPA_CHANNELS_PER_PIXEL, WIA_IPA_CHANNELS_PER_PIXEL_STR, RN, lChannelsPerPixel);
        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Failed to initialize WIA_IPA_CHANNELS_PER_PIXEL for item %u, hr = 0x%08X", nDocumentHandlingSelect, hr));
        }
    }

    //
    // WIA_IPA_BITS_PER_CHANNEL
    //
    if (SUCCEEDED(hr))
    {
        LONG lBitsPerChannel = 1;

        hr = PropertyManager.AddProperty(WIA_IPA_BITS_PER_CHANNEL, WIA_IPA_BITS_PER_CHANNEL_STR, RN, lBitsPerChannel);
        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Failed to initialize WIA_IPA_BITS_PER_CHANNEL for item %u, hr = 0x%08X", nDocumentHandlingSelect, hr));
        }
    }

    //
    // Apply the property changes to the current session's Application Item Tree:
    //

    if (SUCCEEDED(hr))
    {
        hr = PropertyManager.SetItemProperties(pWiasContext);
        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "CWIAPropertyManager::SetItemProperties failed to set WIA item properties for item %u, hr = 0x%08X",
                nDocumentHandlingSelect, hr));
        }
    }

    WIAEX_TRACE_FUNC_HR;

    return hr;
}

/**************************************************************************\
*
* Initializes the properties specific to the Barcode Reader item.
*
* Parameters:
*
*    pWiasContext - pointer to the item context
*
* Return Value:
*
*    S_OK if successful, an error HRESULT otherwise
*
\**************************************************************************/

HRESULT CWiaDriver::InitializeBarcodeReaderProperties(
    _In_ BYTE* pWiasContext)
{
    HRESULT hr = S_OK;
    CWIAPropertyManager PropertyManager;

    if (!pWiasContext)
    {
        hr = E_INVALIDARG;
        WIAEX_ERROR((g_hInst, "CWiaDriver::InitializeBarcodeReaderProperties, invalid parameter, hr = 0x%08X", hr));
    }

    WIAEX_TRACE_BEGIN;

    //
    // WIA_IPS_BARCODE_READER
    //
    // This sample driver pretends to support a barcode reader device installed on the front feeder side
    //
    if (SUCCEEDED(hr))
    {
        CBasicDynamicArray<LONG> lBarcodeReaderArray;
        lBarcodeReaderArray.Append(WIA_BARCODE_READER_DISABLED);
        lBarcodeReaderArray.Append(WIA_BARCODE_READER_AUTO);
        lBarcodeReaderArray.Append(WIA_BARCODE_READER_FEEDER_FRONT);

        hr = PropertyManager.AddProperty(WIA_IPS_BARCODE_READER, WIA_IPS_BARCODE_READER_STR, RWL, lBarcodeReaderArray[0], lBarcodeReaderArray[0], &lBarcodeReaderArray);
        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Failed to initialize WIA_IPS_BARCODE_READER, hr = 0x%08X", hr));
        }
    }

    //
    // WIA_IPS_MAXIMUM_BARCODES_PER_PAGE
    //
    if (SUCCEEDED(hr))
    {
        hr = PropertyManager.AddPropertyUL(WIA_IPS_MAXIMUM_BARCODES_PER_PAGE, WIA_IPS_MAXIMUM_BARCODES_PER_PAGE_STR, RWR, 0, 0, 0, 10, 1);
        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Failed to initialize WIA_IPS_MAXIMUM_BARCODES_PER_PAGE, hr = 0x%08X", hr));
        }
    }

    //
    // WIA_IPS_BARCODE_SEARCH_DIRECTION
    //
    if (SUCCEEDED(hr))
    {
        CBasicDynamicArray<LONG> lBarcodeSearchArray;
        lBarcodeSearchArray.Append(WIA_BARCODE_AUTO_SEARCH);
        lBarcodeSearchArray.Append(WIA_BARCODE_HORIZONTAL_SEARCH);
        lBarcodeSearchArray.Append(WIA_BARCODE_VERTICAL_SEARCH);
        lBarcodeSearchArray.Append(WIA_BARCODE_HORIZONTAL_VERTICAL_SEARCH);
        lBarcodeSearchArray.Append(WIA_BARCODE_VERTICAL_HORIZONTAL_SEARCH);

        hr = PropertyManager.AddProperty(WIA_IPS_BARCODE_SEARCH_DIRECTION, WIA_IPS_BARCODE_SEARCH_DIRECTION_STR, RWL, lBarcodeSearchArray[0], lBarcodeSearchArray[0], &lBarcodeSearchArray);
        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Failed to initialize WIA_IPS_BARCODE_SEARCH_DIRECTION, hr = 0x%08X", hr));
        }
    }

    //
    // WIA_IPS_MAXIMUM_BARCODE_SEARCH_RETRIES
    //
    // This sample driver pretends to support no retries (range containing only the value 0)
    //
    if (SUCCEEDED(hr))
    {
        hr = PropertyManager.AddPropertyUL(WIA_IPS_MAXIMUM_BARCODE_SEARCH_RETRIES, WIA_IPS_MAXIMUM_BARCODE_SEARCH_RETRIES_STR, RWR, 0, 0, 0, 0, 0);
        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Failed to initialize WIA_IPS_MAXIMUM_BARCODE_SEARCH_RETRIES, hr = 0x%08X", hr));
        }
    }

    //
    // WIA_IPS_BARCODE_SEARCH_TIMEOUT
    //
    if (SUCCEEDED(hr))
    {
        hr = PropertyManager.AddPropertyUL(WIA_IPS_BARCODE_SEARCH_TIMEOUT, WIA_IPS_BARCODE_SEARCH_TIMEOUT_STR, RWR, 0, 0, 0, 100, 10);
        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Failed to initialize WIA_IPS_BARCODE_SEARCH_TIMEOUT, hr = 0x%08X", hr));
        }
    }

    //
    // WIA_IPS_SUPPORTED_BARCODE_TYPES
    //
    // This sample driver pretends to support a multitude of barcode types. This driver
    // ignores this setting when delivering its hard-coded sample barcodes but uses
    // WIA_IPS_SUPPORTED_BARCODE_TYPES to validate WIA_IPS_ENABLED_BARCODE_TYPES
    //
    if (SUCCEEDED(hr))
    {
        ULONG ulBarcodeTypes = ARRAYSIZE(g_lSupportedBarcodeTypes);

        hr = PropertyManager.AddProperty(WIA_IPS_SUPPORTED_BARCODE_TYPES, WIA_IPS_SUPPORTED_BARCODE_TYPES_STR, RN, ulBarcodeTypes, (LONG *)&g_lSupportedBarcodeTypes[0]);
        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Failed to initialize WIA_IPS_SUPPORTED_BARCODE_TYPES, hr = 0x%08X", hr));
        }
    }

    //
    // WIA_IPS_ENABLED_BARCODE_TYPES
    //
    // By default when barcode detection is enabled there are 3 barcodes enabled, which
    // happen to match the sample, hard-coded, barcode metadata for this driver
    //
    if (SUCCEEDED(hr))
    {
        LONG lDefaultEnabledBarcodeTypes[] = { WIA_BARCODE_UPCA, WIA_BARCODE_CODABAR, WIA_BARCODE_CODE39_FULLASCII };
        ULONG ulBarcodeTypes = ARRAYSIZE(lDefaultEnabledBarcodeTypes);

        hr = PropertyManager.AddProperty(WIA_IPS_ENABLED_BARCODE_TYPES, WIA_IPS_ENABLED_BARCODE_TYPES_STR, RW, ulBarcodeTypes, &lDefaultEnabledBarcodeTypes[0]);
        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Failed to initialize WIA_IPS_ENABLED_BARCODE_TYPES, hr = 0x%08X", hr));
        }
    }

    //
    // Apply the property changes to the current session's Application Item Tree:
    //

    if (SUCCEEDED(hr))
    {
        hr = PropertyManager.SetItemProperties(pWiasContext);
        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "CWIAPropertyManager::SetItemProperties failed to set WIA item properties for the barcode reader item, hr = 0x%08X", hr));
        }
    }

    WIAEX_TRACE_FUNC_HR;

    return hr;
}

/**************************************************************************\
*
* Initializes the properties specific to the Patch Code Reader item.
*
* Parameters:
*
*    pWiasContext - pointer to the item context
*
* Return Value:
*
*    S_OK if successful, an error HRESULT otherwise
*
\**************************************************************************/

HRESULT CWiaDriver::InitializePatchCodeReaderProperties(
    _In_ BYTE* pWiasContext)
{
    HRESULT hr = S_OK;
    CWIAPropertyManager PropertyManager;

    if (!pWiasContext)
    {
        hr = E_INVALIDARG;
        WIAEX_ERROR((g_hInst, "CWiaDriver::InitializePatchCodeReaderProperties, invalid parameter, hr = 0x%08X", hr));
    }

    WIAEX_TRACE_BEGIN;

    //
    // WIA_IPS_PATCH_CODE_READER
    //
    // This sample driver pretends to support a patch code reader device installed
    //
    if (SUCCEEDED(hr))
    {
        CBasicDynamicArray<LONG> lPatchCodeReaderArray;
        lPatchCodeReaderArray.Append(WIA_PATCH_CODE_READER_DISABLED);
        lPatchCodeReaderArray.Append(WIA_PATCH_CODE_READER_AUTO);

        hr = PropertyManager.AddProperty(WIA_IPS_PATCH_CODE_READER, WIA_IPS_PATCH_CODE_READER_STR, RWL, lPatchCodeReaderArray[0], lPatchCodeReaderArray[0], &lPatchCodeReaderArray);
        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Failed to initialize WIA_IPS_PATCH_CODE_READER, hr = 0x%08X", hr));
        }
    }

    //
    // WIA_IPS_SUPPORTED_PATCH_CODE_TYPES
    //
    // This sample driver pretends to support a multitude of barcode types. This driver
    // ignores this setting when delivering its hard-coded sample barcodes but uses
    // WIA_IPS_SUPPORTED_PATCH_CODE_TYPES to validate WIA_IPS_ENABLED_PATCH_CODE_TYPES
    //
    if (SUCCEEDED(hr))
    {
        ULONG ulPatchCodeTypes = ARRAYSIZE(g_lSupportedPatchCodeTypes);

        hr = PropertyManager.AddProperty(WIA_IPS_SUPPORTED_PATCH_CODE_TYPES, WIA_IPS_SUPPORTED_PATCH_CODE_TYPES_STR, RN, ulPatchCodeTypes, (LONG *)&g_lSupportedPatchCodeTypes[0]);
        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Failed to initialize WIA_IPS_SUPPORTED_PATCH_CODE_TYPES, hr = 0x%08X", hr));
        }
    }

    //
    // WIA_IPS_ENABLED_PATCH_CODE_TYPES
    //
    // By default when barcode detection is enabled there are 2 patch codes enabled, which
    // happen to match the sample, hard-coded, patch code metadata for this driver
    //
    if (SUCCEEDED(hr))
    {
        LONG lDefaultEnabledPatchCodeTypes[] = { WIA_PATCH_CODE_2, WIA_PATCH_CODE_3 };
        ULONG ulPatchCodeTypes = ARRAYSIZE(lDefaultEnabledPatchCodeTypes);

        hr = PropertyManager.AddProperty(WIA_IPS_ENABLED_PATCH_CODE_TYPES, WIA_IPS_ENABLED_PATCH_CODE_TYPES_STR, RW, ulPatchCodeTypes, &lDefaultEnabledPatchCodeTypes[0]);
        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Failed to initialize WIA_IPS_ENABLED_PATCH_CODE_TYPES, hr = 0x%08X", hr));
        }
    }

    //
    // WIA_IPS_ALARM
    //
    // This sample driver does pretend to support one kind of audible alarm (beep) to signal
    // when a path code is successfully detected
    //
    if (SUCCEEDED(hr))
    {
        CBasicDynamicArray<LONG> lAlarmArray;
        lAlarmArray.Append(WIA_ALARM_NONE);
        lAlarmArray.Append(WIA_ALARM_BEEP1);

        hr = PropertyManager.AddProperty(WIA_IPS_ALARM, WIA_IPS_ALARM_STR, RWL, lAlarmArray[0], lAlarmArray[0], &lAlarmArray);
        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Failed to initialize WIA_IPS_ALARM, hr = 0x%08X", hr));
        }
    }

    //
    // Apply the property changes to the current session's Application Item Tree:
    //

    if (SUCCEEDED(hr))
    {
        hr = PropertyManager.SetItemProperties(pWiasContext);
        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "CWIAPropertyManager::SetItemProperties failed to set WIA item properties for the patch code reader item, hr = 0x%08X", hr));
        }
    }

    WIAEX_TRACE_FUNC_HR;

    return hr;
}

/**************************************************************************\
*
* Initializes the properties specific to the MICR Reader item.
*
* Parameters:
*
*    pWiasContext - pointer to the item context
*
* Return Value:
*
*    S_OK if successful, an error HRESULT otherwise
*
\**************************************************************************/

HRESULT CWiaDriver::InitializeMicrReaderProperties(
    _In_ BYTE* pWiasContext)
{
    HRESULT hr = S_OK;
    CWIAPropertyManager PropertyManager;

    if (!pWiasContext)
    {
        hr = E_INVALIDARG;
        WIAEX_ERROR((g_hInst, "CWiaDriver::InitializeMicrReaderProperties, invalid parameter, hr = 0x%08X", hr));
    }

    WIAEX_TRACE_BEGIN;

   //
    // WIA_IPS_MICR_READER
    //
    // This sample driver pretends to support a MICR reader device installed on the front feeder side
    //
    if (SUCCEEDED(hr))
    {
        CBasicDynamicArray<LONG> lMICRReaderArray;
        lMICRReaderArray.Append(WIA_MICR_READER_DISABLED);
        lMICRReaderArray.Append(WIA_MICR_READER_AUTO);
        lMICRReaderArray.Append(WIA_MICR_READER_FEEDER_FRONT);

        hr = PropertyManager.AddProperty(WIA_IPS_MICR_READER, WIA_IPS_MICR_READER_STR, RWL, lMICRReaderArray[0], lMICRReaderArray[0], &lMICRReaderArray);
        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Failed to initialize WIA_IPS_MICR_READER, hr = 0x%08X", hr));
        }
    }

    //
    // Apply the property changes to the current session's Application Item Tree:
    //

    if (SUCCEEDED(hr))
    {
        hr = PropertyManager.SetItemProperties(pWiasContext);
        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "CWIAPropertyManager::SetItemProperties failed to set WIA item properties for the MICR reader item, hr = 0x%08X", hr));
        }
    }

    WIAEX_TRACE_FUNC_HR;

    return hr;
}

/**************************************************************************\
*
* Initializes WIA_FORMAT_INFO arrays needed for IWiaMiniDrv::drvGetWiaFormatInfo
*
* Parameters:
*
*    None
*
* Return Value:
*
*    S_OK if successful, an error HRESULT otherwise
*
\**************************************************************************/

HRESULT CWiaDriver::InitializeFormatInfoArrays()
{
    HRESULT hr = S_OK;
    WIA_FORMAT_INFO tFormatInfo = {};

    //
    // This sample driver supports the same WIA_FORMAT_INFO array for Root, Flatbed, Feeder and Auto items:
    //
    // { WiaImgFmt_BMP, TYMED_FILE }
    // { WiaImgFmt_EXIF, TYMED_FILE }
    // { WiaImgFmt_RAW, TYMED_FILE }
    //

    m_tFormatInfo.Destroy();

    tFormatInfo.guidFormatID = WiaImgFmt_BMP;
    tFormatInfo.lTymed = TYMED_FILE;
    m_tFormatInfo.Append(tFormatInfo);

    tFormatInfo.guidFormatID = WiaImgFmt_EXIF;
    tFormatInfo.lTymed = TYMED_FILE;
    m_tFormatInfo.Append(tFormatInfo);

    tFormatInfo.guidFormatID = WiaImgFmt_RAW;
    tFormatInfo.lTymed = TYMED_FILE;
    m_tFormatInfo.Append(tFormatInfo);

    //
    // The Imprinter and Endorser items support:
    //
    // { WiaImgFmt_CSV, TYMED_FILE }
    // { WiaImgFmt_TXT, TYMED_FILE }
    // { WiaImgFmt_BMP, TYMED_FILE }
    //

    m_tFormatInfoImprinterEndorser.Destroy();

    tFormatInfo.guidFormatID = WiaImgFmt_CSV;
    tFormatInfo.lTymed = TYMED_FILE;
    m_tFormatInfoImprinterEndorser.Append(tFormatInfo);

    tFormatInfo.guidFormatID = WiaImgFmt_TXT;
    tFormatInfo.lTymed = TYMED_FILE;
    m_tFormatInfoImprinterEndorser.Append(tFormatInfo);

    tFormatInfo.guidFormatID = WiaImgFmt_BMP;
    tFormatInfo.lTymed = TYMED_FILE;
    m_tFormatInfoImprinterEndorser.Append(tFormatInfo);

    //
    // The Barcode Reader item supports:
    //
    // { WiaImgFmt_XMLBAR, TYMED_FILE }
    // { WiaImgFmt_RAWBAR, TYMED_FILE }
    //

    m_tFormatInfoBarcodeReader.Destroy();

    tFormatInfo.guidFormatID = WiaImgFmt_XMLBAR;
    tFormatInfo.lTymed = TYMED_FILE;
    m_tFormatInfoBarcodeReader.Append(tFormatInfo);

    tFormatInfo.guidFormatID = WiaImgFmt_RAWBAR;
    tFormatInfo.lTymed = TYMED_FILE;
    m_tFormatInfoBarcodeReader.Append(tFormatInfo);

    //
    // The Patch Code Reader item supports:
    //
    // { WiaImgFmt_XMLPAT, TYMED_FILE }
    // { WiaImgFmt_RAWPAT, TYMED_FILE }
    //

    m_tFormatInfoPatchCodeReader.Destroy();

    tFormatInfo.guidFormatID = WiaImgFmt_XMLPAT;
    tFormatInfo.lTymed = TYMED_FILE;
    m_tFormatInfoPatchCodeReader.Append(tFormatInfo);

    tFormatInfo.guidFormatID = WiaImgFmt_RAWPAT;
    tFormatInfo.lTymed = TYMED_FILE;
    m_tFormatInfoPatchCodeReader.Append(tFormatInfo);

    //
    // The MICR Reader item supports:
    //
    // { WiaImgFmt_XMLMIC, TYMED_FILE }
    // { WiaImgFmt_RAWMIC, TYMED_FILE }
    //

    m_tFormatInfoMicrReader.Destroy();

    tFormatInfo.guidFormatID = WiaImgFmt_XMLMIC;
    tFormatInfo.lTymed = TYMED_FILE;
    m_tFormatInfoMicrReader.Append(tFormatInfo);

    tFormatInfo.guidFormatID = WiaImgFmt_RAWMIC;
    tFormatInfo.lTymed = TYMED_FILE;
    m_tFormatInfoMicrReader.Append(tFormatInfo);

    return hr;
}

/**************************************************************************\
*
* Updates the following image information properties in auto-detect color mode.
* Because this sample driver does not support cropping (scan region changes)
* and supports only a single/fixed scan resolution this function is not needed
* to be executed in other situations:
*
* WIA_IPA_PIXELS_PER_LINE
* WIA_IPA_NUMBER_OF_LINES
* WIA_IPA_BYTES_PER_LINE
* WIA_IPA_CHANNELS_PER_PIXEL
* WIA_IPA_RAW_CHANNELS_PER_PIXEL
*
* Note that these properties are not implemented/available on the Auto item.
*
* Parameters:
*
*    pWiasContext - pointer to the item context
*    lDataType    - data type; if set to WIA_DATA_AUTO the
*                   function reads the current WIA_IPA_DATATYPE.
*
* Return Value:
*
*    S_OK if successful, an error HRESULT otherwise
*
\**************************************************************************/
HRESULT CWiaDriver::UpdateImageInfoProperties(
    _In_ BYTE *pWiasContext,
    LONG lDataType)
{
    HRESULT hr = S_OK;
    GUID guidItemCategory = WIA_CATEGORY_ROOT;
    LONG lDepth = 8;
    LONG lChannelsPerPixel = 1;
    LONG lCompression = WIA_COMPRESSION_NONE;
    GUID guidFormat = WiaImgFmt_UNDEFINED;
    LONG lXExtent = 0;
    LONG lYExtent = 0;
    LONG lPixelsPerLine = 0;
    LONG lNumberOfLines = 0;
    LONG lBytesPerLine = 0;

    WIAEX_TRACE_BEGIN;

    if (!pWiasContext)
    {
        hr = E_INVALIDARG;
        WIAEX_ERROR((g_hInst, "CWiaDriver::UpdateImageInfoProperties, invalid parameter, hr = 0x%08X", hr));
    }

    //
    // Read WIA_IPA_ITEM_CATEGORY to identify the current item:
    //
    if (SUCCEEDED(hr))
    {
        hr = wiasReadPropGuid(pWiasContext, WIA_IPA_ITEM_CATEGORY, &guidItemCategory, NULL, TRUE);
        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Error reading current WIA_IPA_ITEM_CATEGORY, hr = 0x%08X", hr));
        }
    }

    //
    // Cannot validate image information properties on the Root or Auto items:
    //
    if (SUCCEEDED(hr) && ((IsEqualGUID(WIA_CATEGORY_ROOT, guidItemCategory)) ||
        (IsEqualGUID(WIA_CATEGORY_AUTO, guidItemCategory))))
    {
        hr = E_INVALIDARG;
        WIAEX_ERROR((g_hInst, "Functionality not supported on this item, hr = 0x%08X", hr));
    }

    //
    // If a data type is not specified, read WIA_IPA_DATATYPE:
    //
    if (SUCCEEDED(hr))
    {
        if (WIA_DATA_AUTO == lDataType)
        {
            LONG lActualDataType = WIA_DATA_GRAYSCALE;

            hr = wiasReadPropLong(pWiasContext, WIA_IPA_DATATYPE, &lActualDataType, NULL, TRUE);
            if (SUCCEEDED(hr))
            {
                if (WIA_DATA_AUTO == lActualDataType)
                {
                    WIAEX_ERROR((g_hInst, "Unspecified data type! Considering 8-bpp grayscale (default) and trying to continue"));
                    lDepth = 8;
                }
                else
                {
                    lDepth = (WIA_DATA_COLOR == lActualDataType) ? 24 : 8;
                }
            }
            else
            {
                WIAEX_ERROR((g_hInst, "Error reading current WIA_IPA_DATA_TYPE, hr = 0x%08X", hr));
            }
        }
        else
        {
            lDepth = (WIA_DATA_COLOR == lDataType) ? 24 : 8;
        }
    }

    //
    // Check the current WIA_IPA_COMPRESSION:
    //
    if (SUCCEEDED(hr))
    {
        hr = wiasReadPropLong(pWiasContext, WIA_IPA_COMPRESSION, &lCompression, NULL, TRUE);
        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Error reading current WIA_IPA_COMPRESSION, hr = 0x%08X", hr));
        }
    }

    //
    // Check the current WIA_IPA_FORMAT:
    //
    if (SUCCEEDED(hr))
    {
        hr = wiasReadPropGuid(pWiasContext, WIA_IPA_FORMAT, &guidFormat, NULL, TRUE);
        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Error reading current WIA_IPA_FORMAT, hr = 0x%08X", hr));
        }
    }

    //
    // Check the current WIA_IPS_XEXTENT and WIA_IPS_YTEXTENT:
    //
    if (SUCCEEDED(hr))
    {
        hr = wiasReadPropLong(pWiasContext, WIA_IPS_XEXTENT, &lXExtent, NULL, TRUE);
        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Error reading current WIA_IPS_XEXTENT, hr = 0x%08X", hr));
        }
    }

    if (SUCCEEDED(hr))
    {
        hr = wiasReadPropLong(pWiasContext, WIA_IPS_YEXTENT, &lYExtent, NULL, TRUE);
        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Error reading current WIA_IPS_YEXTENT, hr = 0x%08X", hr));
        }
    }

    if (SUCCEEDED(hr))
    {
        //
        // The number of bytes per line include the padding necessary to make each uncompressed
        // line (DIB or Raw) DWORD aligned. When data is compressed the number of bytes per line
        // calculated here reflects the original uncompressed image:
        //
        lPixelsPerLine = lXExtent;
        lNumberOfLines = lYExtent;
        lBytesPerLine = BytesPerLine(lPixelsPerLine, lDepth);

        WIAS_TRACE((g_hInst, "Image information: %u PPL, %u lines, %u BPL (compression: %u)",
            lPixelsPerLine, lNumberOfLines, lBytesPerLine, lCompression));
    }

    //
    // Update the following properties, supported for backwards compatibility (with WIA 1.0
    // and TWAIN) on the Flatbed and Feeder item but not on the new Auto item:
    //
    // WIA_IPA_PIXELS_PER_LINE - the image width, in pixels, for the final image
    // WIA_IPA_NUMBER_OF_LINES - the image length, in pixels, for the final image
    // WIA_IPA_BYTES_PER_LINE  - line width in bytes that must match WIA_IPA_PIXELS_PER_LINE and WIA_IPA_DEPTH
    //
    if (SUCCEEDED(hr))
    {
        hr = wiasWritePropLong(pWiasContext, WIA_IPA_PIXELS_PER_LINE, lPixelsPerLine);
        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Failed to update WIA_IPA_PIXELS_PER_LINE, hr = 0x%08X", hr));
        }
    }

    if (SUCCEEDED(hr))
    {
        hr = wiasWritePropLong(pWiasContext, WIA_IPA_NUMBER_OF_LINES, lNumberOfLines);
        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Failed to update WIA_IPA_NUMBER_OF_LINES, hr = 0x%08X", hr));
        }
    }

    if (SUCCEEDED(hr))
    {
        hr = wiasWritePropLong(pWiasContext, WIA_IPA_BYTES_PER_LINE, lBytesPerLine);
        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Failed to update WIA_IPA_BYTES_PER_LINE, hr = 0x%08X", hr));
        }
    }

    //
    // Update WIA_IPA_CHANNELS_PER_PIXEL and WIA_IPA_RAW_BITS_PER_CHANNEL top match the bit depth.
    // Note that this saple driver does not need to update WIA_IPA_BITS_PER_CHANNEL, and that
    // WIA_IPA_DEPTH and WIA_IPA_DATA_TYPE are updated during validation (see ValidateFormatProperties):
    //

    if (SUCCEEDED(hr))
    {
        lChannelsPerPixel = (24 == lDepth) ? 3 : 1;
        hr = wiasWritePropLong(pWiasContext, WIA_IPA_CHANNELS_PER_PIXEL, lChannelsPerPixel);
        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Failed to update WIA_IPA_CHANNELS_PER_PIXEL, hr = 0x%08X", hr));
        }
    }

    if (SUCCEEDED(hr))
    {
        BYTE bRawBitsPerChannel[3] = {};

        for (int i = 0; i < lChannelsPerPixel; i++)
        {
            bRawBitsPerChannel[i] = 8;
        }

        hr = wiasWritePropBin(pWiasContext, WIA_IPA_RAW_BITS_PER_CHANNEL, lChannelsPerPixel, bRawBitsPerChannel);
        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Failed to update WIA_IPA_RAW_BITS_PER_CHANNEL, hr = 0x%08X", hr));
        }
    }


    WIAEX_TRACE_FUNC_HR;

    return hr;
}

/**************************************************************************\
*
* Updates the globally stored (per driver instance) scan available item
* name indicating that this item is marked for data transfer due to a
* device initiated scan operation, and/or the scanner is not in a scan
* available state.
*
* Parameters:
*
*    wszInputSource - a WIA item name as the name of the input source,
*                     NULL to reset the value to an empty string.
*
* Return Value:
*
*    S_OK if successful, an error HRESULT otherwise
*
\**************************************************************************/
HRESULT CWiaDriver::UpdateScanAvailableItemName(
    _In_opt_ LPCWSTR wszInputSource)
{
    HRESULT hr = S_OK;

    //
    // Clear the current name:
    //
    if (m_bstrScanAvailableItem)
    {
        SysFreeString(m_bstrScanAvailableItem);
        m_bstrScanAvailableItem = NULL;
    }

    //
    // Identify the input source and prepare the value containing the apropriate item name:
    //
    if (wszInputSource)
    {
        if (!wcscmp(WIA_DRIVER_FEEDER_NAME, wszInputSource))
        {
            WIAS_TRACE((g_hInst, "Scan available from feeder (%ws)", wszInputSource));
            m_bstrScanAvailableItem = SysAllocString(WIA_DRIVER_FEEDER_NAME);
        }
        else if (!wcscmp(WIA_DRIVER_FLATBED_NAME, wszInputSource))
        {
            WIAS_TRACE((g_hInst, "Scan available from flatbed (%ws)", wszInputSource));
            m_bstrScanAvailableItem = SysAllocString(WIA_DRIVER_FLATBED_NAME);
        }
        else
        {
            WIAS_TRACE((g_hInst, "Scan available from unknown input source (%ws) - information not recorded",
                wszInputSource));
            m_bstrScanAvailableItem = SysAllocString(L"");
        }
    }
    else
    {
        //
        // A trace message is avoided here on purpose - the following message, if enabled,
        // would be visible both when initializing a new AIT Root with no scan available
        // input source recorded -and- when resetting the scan available soure information
        // from within IWiaMiniDrv::drvAcquireItemData before executing a new scan job.
        //
        m_bstrScanAvailableItem = SysAllocString(L"");
    }

    if (!m_bstrScanAvailableItem)
    {
        hr = E_OUTOFMEMORY;
        WIAEX_ERROR((g_hInst, "Failed to update the scan available item name, out of memory, hr = 0x%08X", hr));
    }

    return hr;
}

/**************************************************************************\
*
* Updates the WIA_DPS_SCAN_AVAILABLE_ITEM property at real-time when an
* application attempts to read a property from its AIT Root item. We cannot
* make this update immediately following a notification that a device initiated
* scan is available because the event comes globally, outside of any application
* session context (possibly at a time when no application session exists).
*
* WARNING: must not be called on other items than Root (WiaItemTypeRoot).
*
* Parameters:
*
*    pWiasContext   - pointer to the item context
*
* Return Value:
*
*    S_OK if successful, an error HRESULT otherwise
*
\**************************************************************************/
HRESULT CWiaDriver::UpdateScanAvailableItemProperty(
    _In_ BYTE *pWiasContext)
{
    HRESULT hr = S_OK;

    if (!pWiasContext)
    {
        hr = E_INVALIDARG;
        WIAEX_ERROR((g_hInst, "Invalid parameter, hr = 0x%08X", hr));
    }

    //
    // If no value is initialized yet, initialize it now to an
    // empty string meaning "no scan available source recorded":
    //
    if (SUCCEEDED(hr) && (!m_bstrScanAvailableItem))
    {
        m_bstrScanAvailableItem = SysAllocString(L"");
        if (!m_bstrScanAvailableItem)
        {
            hr = E_OUTOFMEMORY;
            WIAEX_ERROR((g_hInst, "Failed to initialize an empty string in lieu of a scan available item name, hr = 0x%08X", hr));
        }
    }

    //
    // Update the WIA_DPS_SCAN_AVAILABLE_ITEM property:
    //
    if (SUCCEEDED(hr))
    {
        hr = wiasWritePropStr(pWiasContext, WIA_DPS_SCAN_AVAILABLE_ITEM, m_bstrScanAvailableItem);
        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "wiasWritePropStr(WIA_DPS_SCAN_AVAILABLE_ITEM, '%ws') failed, hr = 0x%08X", m_bstrScanAvailableItem, hr));
        }
    }

    return hr;
}

/**************************************************************************\
*
* Retrieves standard WIA page sizes that fit in the specified scan area
* dimensions considering the specified orientation.
*
* Parameters:
*
*    lMaxWidth               - maximum width of the total scan area, in 1/1000"
*    lMaxHeight              - maximum height of the total scan area, in 1/1000"
*    lMinWidth               - minimum width of the total scan area, in 1/1000"
*    lMinHeight              - minimum height of the total scan area, in 1/1000"
*    bPortrait               - TRUE for portrait, FALSE for landscape orientation
*    arrayPageSizes          - reference for array where to return page sizes
*
* Return Value:
*
*    The number of page sizes found
*
\**************************************************************************/

LONG CWiaDriver::GetValidPageSizes(
    LONG lMaxWidth,
    LONG lMaxHeight,
    LONG lMinWidth,
    LONG lMinHeight,
    BOOL bPortrait,
    CBasicDynamicArray<LONG>& arrayPageSizes)
{
    LONG lNumPageSizesFound = 0;
    LONG lNumKnownPageSizes = sizeof(g_DefinedPageSizeCombinations) / sizeof(g_DefinedPageSizeCombinations[0]);

    //
    // Do not erase page sizes already added to the array,
    // append new values that are not yet in the array:
    //
    // arrayPageSizes.Destroy();
    //

    for (LONG i = 0; i < lNumKnownPageSizes; i++)
    {
        if (bPortrait)
        {
            if ((g_DefinedPageSizeCombinations[i].m_lPageWidth <= lMaxWidth) &&
                (g_DefinedPageSizeCombinations[i].m_lPageHeight <= lMaxHeight) &&
                (g_DefinedPageSizeCombinations[i].m_lPageWidth >= lMinWidth) &&
                (g_DefinedPageSizeCombinations[i].m_lPageHeight >= lMinHeight))
            {
                if (-1 == arrayPageSizes.Find(g_DefinedPageSizeCombinations[i].m_lPageSize))
                {
                    arrayPageSizes.Append(g_DefinedPageSizeCombinations[i].m_lPageSize);
                }
            }
        }
        else
        {
            if ((g_DefinedPageSizeCombinations[i].m_lPageWidth <= lMaxHeight) &&
                (g_DefinedPageSizeCombinations[i].m_lPageHeight <= lMaxWidth) &&
                (g_DefinedPageSizeCombinations[i].m_lPageWidth >= lMinHeight) &&
                (g_DefinedPageSizeCombinations[i].m_lPageHeight >= lMinWidth))
            {
                if (-1 == arrayPageSizes.Find(g_DefinedPageSizeCombinations[i].m_lPageSize))
                {
                    arrayPageSizes.Append(g_DefinedPageSizeCombinations[i].m_lPageSize);
                }
            }
        }
    }

    lNumPageSizesFound = arrayPageSizes.Size();

    return lNumPageSizesFound;
}

/**************************************************************************\
*
* Returns the width and height for the specified standard page size,
* in 1/1000", according with the indicated orientation
*
* Parameters:
*
*    lPageSize   - the page size to return the dimensions for
*    bPortrait   - TRUE for portait orientation, FALSE for landscape
*    lPageWidth  - reference for variable to receive the page width
*    lPageHeight - reference for variable to receive the page height
* Return Value:
*
*    S_OK if successful, E_INVALIDARG if no such standard page size is found
*
\**************************************************************************/

HRESULT CWiaDriver::GetPageDimensions(
    LONG lPageSize,
    BOOL bPortrait,
    LONG& lPageWidth,
    LONG& lPageHeight)
{
    HRESULT hr = E_INVALIDARG;
    LONG lNumKnownPageSizes = sizeof(g_DefinedPageSizeCombinations) / sizeof(g_DefinedPageSizeCombinations[0]);

    for (LONG i = 0; i < lNumKnownPageSizes; i++)
    {
        if (g_DefinedPageSizeCombinations[i].m_lPageSize == lPageSize)
        {
            if (bPortrait)
            {
                lPageWidth = g_DefinedPageSizeCombinations[i].m_lPageWidth;
                lPageHeight = g_DefinedPageSizeCombinations[i].m_lPageHeight;
            }
            else
            {
                lPageWidth = g_DefinedPageSizeCombinations[i].m_lPageHeight;
                lPageHeight = g_DefinedPageSizeCombinations[i].m_lPageWidth;
            }

            hr = S_OK;
            break;
        }
    }

    return hr;
}
