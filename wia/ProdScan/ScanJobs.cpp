/**************************************************************************
*
*  Copyright © Microsoft Corporation
*
*  Title: ScanJobs.cpp
*
*  Description: This file contains the implementation of the IWiaMiniDrv::
*               drvAcquireItemData method and its helper methods used to
*               execute scan job requests and transfer image and metadata
*               files from or to the Production Scanner Driver Sample.
*
***************************************************************************/

#include "stdafx.h"

/**************************************************************************\
*
* Implements IWiaMiniDrv::drvAcquireItemData. This method is called by the
* WIA service when the driver must transfer image data to the application.
* This driver implements the Stream based WIA 2.0 transfer model introduced
* in Windows Vista. This call correponds to one scan job executed at the device
* (scaner starts, scan documents and transfers data, scanner stops).
*
* Parameters:
*
*    pWiasContext - pointer to the item context
*    lFlags       - reserved (set to 0)
*    pmdtc        - pointer to a MINIDRV_TRANSFER_CONTEXT structure
*                   containing the device transfer context
*    plDevErrVal  - unused (drvGetDeviceErrorStr unsupported)
*
* Return Value:
*
*    S_OK if successful, S_FALSE if the transfer is canceled
*    or an error HRESULT if an error occurrs
*
\**************************************************************************/

HRESULT CWiaDriver::drvAcquireItemData(
    _In_ BYTE*                     pWiasContext,
    LONG                           lFlags,
    _In_ PMINIDRV_TRANSFER_CONTEXT pmdtc,
    _Out_ LONG*                    plDevErrVal)
{
    HRESULT hr = S_OK;

    GUID guidItemCategory = GUID_NULL;
    BSTR bstrItemName = NULL;
    BSTR bstrFullItemName = NULL;

    WIA_DRIVER_ITEM_CONTEXT *pWiaDriverItemContext = NULL;
    IWiaMiniDrvTransferCallback *pTransferCallback = NULL;
    WiaTransferParams *pCallbackTransferParams = NULL;

    BYTE *pTransferBuffer = NULL;
    ULONG ulBufferSize = 0;

    BOOL bImageTransfer = TRUE;

    WIAEX_TRACE_BEGIN;

    //
    // Validate parameters:
    //
    if ((!pWiasContext) || (!pmdtc) || (!plDevErrVal) || (!pmdtc->pIWiaMiniDrvCallBack))
    {
        hr = E_INVALIDARG;
        WIAEX_ERROR((g_hInst, "Invalid parameter, hr = 0x%08X", hr));
    }

    //
    // Identify the item to execute the acquisition for checking WIA_IPA_ITEM_CATEGORY:
    //
    if (SUCCEEDED(hr))
    {
        hr = wiasReadPropGuid(pWiasContext, WIA_IPA_ITEM_CATEGORY, &guidItemCategory, NULL, TRUE);
        if (SUCCEEDED(hr))
        {
            bImageTransfer = (IsEqualGUID(WIA_CATEGORY_FLATBED, guidItemCategory) ||
                IsEqualGUID(WIA_CATEGORY_FEEDER, guidItemCategory) || IsEqualGUID(WIA_CATEGORY_AUTO, guidItemCategory));
        }
        else
        {
            WIAEX_ERROR((g_hInst, "Failed to read WIA_IPA_ITEM_CATEGORY property, hr = 0x%08X", hr));
        }
    }

    //
    // Get the item names, we will need them when requesting a new WIA transfer stream:
    //
    if (SUCCEEDED(hr))
    {
        hr = wiasReadPropStr(pWiasContext, WIA_IPA_ITEM_NAME, &bstrItemName, NULL, TRUE);
        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Failed reading WIA_IPA_ITEM_NAME, hr = 0x%08X", hr));
        }
    }
    if (SUCCEEDED(hr))
    {
        hr = wiasReadPropStr(pWiasContext, WIA_IPA_FULL_ITEM_NAME, &bstrFullItemName, NULL, TRUE);
        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Failed reading WIA_IPA_FULL_ITEM_NAME, hr = 0x%08X", hr));
        }
    }

    //
    // Transfers from the Root item are not supported:
    //
    if (SUCCEEDED(hr) && (IsEqualGUID(WIA_CATEGORY_ROOT, guidItemCategory)))
    {
        hr = E_INVALIDARG;
        WIAEX_ERROR((g_hInst, "Acquisition not supported from the Root item, hr = 0x%08X", hr));
    }


    //
    // Get the private context data for this item. This sample driver stores in this data
    // an image  that the WIA application uploads in this session to the Imprinter or the
    // Endorser and that should be used in subsequent downloads from the respective item:
    //
    if (SUCCEEDED(hr))
    {
        hr = wiasGetDriverItemPrivateContext(pWiasContext, (BYTE**)&pWiaDriverItemContext);
        if (SUCCEEDED(hr) && (!pWiaDriverItemContext))
        {
            hr = E_POINTER;
        }
        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Failed to retrieve the driver item context data, hr = 0x%08X", hr));
        }
    }

    //
    // Check the requested direction for this data transfer. Download means from the driver to the
    // application (from the scanner device to the PC). Upload means from the application to the
    // driver (from the PC to the scanner device). For a stream upload the flag value is
    // WIA_MINIDRV_TRANSFER_UPLOAD while for a WIA 1.0 tymed-style transfer the flag value is 0:
    //
    if (SUCCEEDED(hr) && (bImageTransfer || IsEqualGUID(WIA_CATEGORY_BARCODE_READER, guidItemCategory) ||
        IsEqualGUID(WIA_CATEGORY_PATCH_CODE_READER, guidItemCategory) ||
        IsEqualGUID(WIA_CATEGORY_MICR_READER, guidItemCategory)) && (!(lFlags & WIA_MINIDRV_TRANSFER_DOWNLOAD)))
    {
        hr = E_INVALIDARG;
        WIAEX_ERROR((g_hInst,
            "Invalid transfer flag parameter requested, this driver supports only download transfer direction for image data, barcode, patch code and MICR metadata, hr = 0x%08X",
            hr));
    }

    if (SUCCEEDED(hr))
    {
        *plDevErrVal = 0;
    }

    //
    // Get the WIA transfer callback interface to use for these transfers:
    //
    if (SUCCEEDED(hr))
    {
        hr = pmdtc->pIWiaMiniDrvCallBack->QueryInterface(IID_IWiaMiniDrvTransferCallback, (void**)&pTransferCallback);
        if (SUCCEEDED(hr) && (!pTransferCallback))
        {
            hr = E_POINTER;
        }
        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Failed to retrieve IID_IWiaMiniDrvTransferCallback interface, scan failed, hr = 0x%08X", hr));
        }
    }

    //
    // Allocate memory for the WIA transfer parameters structure and the transfer buffer:
    //
    if (SUCCEEDED(hr))
    {
        pCallbackTransferParams = (WiaTransferParams*)CoTaskMemAlloc(sizeof(WiaTransferParams));
        if (!pCallbackTransferParams)
        {
            hr = E_OUTOFMEMORY;
            WIAEX_ERROR((g_hInst, "Memory allocation for transfer parameters failed, scan failed, hr = 0x%08X", hr));
        }
        else
        {
            memset(pCallbackTransferParams, 0, sizeof(WiaTransferParams));
        }
    }
    if (SUCCEEDED(hr))
    {
        hr = AllocateTransferBuffer(&pTransferBuffer, &ulBufferSize);
        if (!pTransferBuffer)
        {
            hr = E_OUTOFMEMORY;
            WIAEX_ERROR((g_hInst, "Memory allocation for transfer buffer failed, scan failed, hr = 0x%08X", hr));
        }
    }

    if (SUCCEEDED(hr))
    {
        //
        // The CWiaDriver::Download and CWiaDriver::Upload methods called below
        // fully trace their execution, no need to trace additionally here.
        // Also note that the pInputStrem stream is released by these functions:
        //
        if (lFlags & WIA_MINIDRV_TRANSFER_DOWNLOAD)
        {
            //
            // 'Download' transfer direction (driver to application)
            //
            hr = Download(pWiasContext, guidItemCategory, bstrItemName, bstrFullItemName, pmdtc, pTransferBuffer,
                ulBufferSize, pTransferCallback, pCallbackTransferParams,  pWiaDriverItemContext);
        }
        else if (lFlags & WIA_MINIDRV_TRANSFER_UPLOAD)
        {
            //
            // 'Upload' transfer direction (application to driver)
            //
            hr = Upload(pWiasContext, guidItemCategory, bstrItemName, bstrFullItemName, pTransferBuffer,
                ulBufferSize, pTransferCallback, pCallbackTransferParams, pWiaDriverItemContext);

        }
    }

    //
    // Note that WIA_TRANSFER_MSG_END_OF_STREAM and WIA_TRANSFER_MSG_END_OF_TRANSFER
    // are sent to the WIA application by the WIA service itself (first when the driver
    // asks for a new stream), the driver should not send these itself.
    //

    //
    // Clean-up for this scan job:
    //

    if (bstrItemName)
    {
        SysFreeString(bstrItemName);
    }

    if (bstrFullItemName)
    {
        SysFreeString(bstrFullItemName);
    }

    if (pTransferBuffer)
    {
        FreeTransferBuffer(pTransferBuffer);
    }

    if (pCallbackTransferParams)
    {
        CoTaskMemFree(pCallbackTransferParams);
    }

    if (pTransferCallback)
    {
        pTransferCallback->Release();
    }

    if (FAILED(hr))
    {
        m_hrLastEdviceError = hr;
    }

    WIAEX_TRACE((g_hInst, "IWiaMiniDrv::drvAcquireItemData 0x%08X", hr));

    return hr;
}

/**************************************************************************\
*
* Helper for CWiaDriver::drvAcquireItemData. Executes the download data
* transfer sequence for the current scan job,
*
* Parameters:
*
*    pWiasContext            - item context
*    guidItemCategory        - item category
*    bstrItemName            - item name
*    bstrFullItemName        - full item name
*    pmdc                    - driver transfer context data
*    pTransferBuffer         - pre-allocated transfer buffer
*    ulBufferSize            - size of the pre-allocated transfer buffer, in bytes
*    pTransferCallback       - IWiaMiniDrvTransferCallback* for WIA status
*    pCallbackTransferParams - WiaTransferParams* for WIA status callbacks
*    ulEstimatedFileSize     - estimated file size, in bytes (0 if unknown)
*
* Return Value:
*
*    S_OK if successful, S_FALSE if the transfer is canceled
*    or an error HRESULT if an error occurrs
*
\**************************************************************************/

HRESULT
CWiaDriver::Download(
    _In_                      BYTE                        *pWiasContext,
                              GUID                         guidItemCategory,
    _In_                      BSTR                         bstrItemName,
    _In_                      BSTR                         bstrFullItemName,
    _In_                      PMINIDRV_TRANSFER_CONTEXT    pmdtc,
    _In_reads_bytes_(ulBufferSize) BYTE                        *pTransferBuffer,
                              ULONG                        ulBufferSize,
    _In_                      IWiaMiniDrvTransferCallback *pTransferCallback,
    _In_                      WiaTransferParams           *pCallbackTransferParams,
    _In_                      WIA_DRIVER_ITEM_CONTEXT     *pWiaDriverItemContext)
{
    HRESULT hr = S_OK;

    IStream *pInputStream = NULL;
    IStream *pOutputStream = NULL;

    LONG lPagesToScan = 1;
    LONG lJobSeparators = WIA_SEPARATOR_DISABLED;
    LONG lMultiFeed = WIA_MULTI_FEED_DETECT_DISABLED;
    LONG lDataType = WIA_DATA_GRAYSCALE;
    LONG lCompression = WIA_COMPRESSION_NONE;

    LONG lPixelsPerLine = 0;
    LONG lNumberOfLines = 0;
    LONG lBytesPerLine = 0;

    LONG lFilesToProcess = 1;
    LONG lFilesProcessed = 0;
    ULONG ulEstimatedFileSize = 0;
    ULONG ulFileSize = 0;

    ULONG_PTR pGDIPlusToken = NULL;
    LONG lGDIPlus_PixelsPerLine = 0;
    LONG lGDIPlus_NumberOfLines = 0;
    LONG lGDIPlus_BytesPerLine = 0;

    DWORD dwJobStart = 0;

    IStream *pWiaStream = NULL;
    BOOL bCancelTransfer = FALSE;
    BOOL bSkipTransfer = FALSE;

    BOOL bImageTransfer = (IsEqualGUID(WIA_CATEGORY_FLATBED, guidItemCategory) ||
        IsEqualGUID(WIA_CATEGORY_FEEDER, guidItemCategory) || IsEqualGUID(WIA_CATEGORY_AUTO, guidItemCategory));


    WIAEX_TRACE_BEGIN;

    if ((!pWiasContext) || (!bstrItemName) || (!bstrFullItemName) || (!pmdtc) || (!pTransferBuffer) || (!pTransferCallback) || (!pCallbackTransferParams))
    {
        hr = E_INVALIDARG;
        WIAEX_ERROR((g_hInst, "Invalid parameter, hr = 0x%08X", hr));
    }

    //
    // Reset the recorded scan available input source, if any, ignoring failures:
    //
    if (SUCCEEDED(hr) && bImageTransfer)
    {
        UpdateScanAvailableItemName(NULL);
    }

    //
    // For image transfers from feeder, use WIA_IPS_PAGES to know the number of images expected to transfer in this job:
    //
    if (SUCCEEDED(hr))
    {
        if (IsEqualGUID(guidItemCategory, WIA_CATEGORY_FEEDER))
        {
            //
            // When WIA_IPS_PAGES is set to 0 (ALL_PAGES) meaning "scan as
            // many documents as there may be loaded into the feeder" this
            // sample driver will transfer up to MAX_SCAN_PAGES:
            //
            hr = wiasReadPropLong(pWiasContext, WIA_IPS_PAGES, &lPagesToScan, NULL, TRUE);
            if (SUCCEEDED(hr))
            {
                if (ALL_PAGES == lPagesToScan)
                {
                    lPagesToScan = MAX_SCAN_PAGES;
                }
            }
            else
            {
                lPagesToScan = 1;
                WIAEX_ERROR((g_hInst, "Failed reading the WIA_IPS_PAGES property, hr = 0x%08X", hr));
            }
        }
        else if (IsEqualGUID(WIA_CATEGORY_AUTO, guidItemCategory))
        {
            //
            // In full automatic mode scan as many pages as the device decides to allow:
            //
            lPagesToScan = MAX_SCAN_PAGES;
        }
        else
        {
            //
            // One single document page to scan from the flatbed or one single metadata file:
            //
            lPagesToScan = 1;
        }
    }

    if (SUCCEEDED(hr) && bImageTransfer)
    {
        WIAS_TRACE((g_hInst, "Pages to scan: %u (0 means all)", lPagesToScan));
    }

    //
    // For feeder acquisitions, this sample driver simulates a job separator every
    // JOB_SEPARATOR_AT_PAGE page and a multi-feed every MULTI_FEED_AT_PAGE pages:
    //
    if (SUCCEEDED(hr) && IsEqualGUID(guidItemCategory, WIA_CATEGORY_FEEDER))
    {
        hr = wiasReadPropLong(pWiasContext, WIA_IPS_JOB_SEPARATORS, &lJobSeparators, NULL, TRUE);
        if (SUCCEEDED(hr))
        {
            hr = wiasReadPropLong(pWiasContext, WIA_IPS_MULTI_FEED, &lMultiFeed, NULL, TRUE);
            if (FAILED(hr))
            {
                WIAEX_ERROR((g_hInst, "Failed reading the WIA_IPS_MULTI_FEED property, hr = 0x%08X", hr));
            }
        }
        else
        {
            WIAEX_ERROR((g_hInst, "Failed reading the WIA_IPS_JOB_SEPARATORS property, hr = 0x%08X", hr));
        }
    }

    //
    // For feeder acquisitions, if WIA_IPS_FEEDER_CONTROL is set to WIA_FEEDER_CONTROL_MANUAL
    // and the feeder is not running, start the feeder and reset the feeder control to automatic
    // mode, updating WIA_IPS_FEEDER_CONTROL to WIA_FEEDER_CONTROL_AUTO:
    //
    if (SUCCEEDED(hr) && IsEqualGUID(guidItemCategory, WIA_CATEGORY_FEEDER))
    {
        LONG lFeederControl = WIA_FEEDER_CONTROL_AUTO;

        hr = wiasReadPropLong(pWiasContext, WIA_IPS_FEEDER_CONTROL, &lFeederControl, NULL, TRUE);
        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Failed reading the WIA_IPS_FEEDER_CONTROL property, hr = 0x%08X", hr));
        }

        if (SUCCEEDED(hr) && (WIA_FEEDER_CONTROL_MANUAL == lFeederControl) && (!m_bFeederStarted))
        {
            hr = StartFeeder();
            if (SUCCEEDED(hr))
            {
                lFeederControl = WIA_FEEDER_CONTROL_AUTO;

                hr = wiasWritePropLong(pWiasContext, WIA_IPS_FEEDER_CONTROL, lFeederControl);
                if (SUCCEEDED(hr))
                {
                    WIAS_TRACE((g_hInst, "Reverted to WIA_FEEDER_CONTROL_AUTO"));
                }
                else
                {
                    WIAEX_ERROR((g_hInst, "Failed to reset the WIA_IPS_FEEDER_CONTROL property, hr = 0x%08X", hr));
                }
            }
            else
            {
                WIAEX_ERROR((g_hInst, "Cannot start the feeder, hr = 0x%08X", hr));
            }
        }
    }

    //
    // Read the current data type configured for this image source:
    //
    if (SUCCEEDED(hr) && bImageTransfer && (!IsEqualGUID(WIA_CATEGORY_AUTO, guidItemCategory)))
    {
        hr = wiasReadPropLong(pWiasContext, WIA_IPA_DATATYPE, &lDataType, NULL, TRUE);
        if (SUCCEEDED(hr))
        {
            if (WIA_DATA_AUTO == lDataType)
            {
                //
                // When WIA_DATA_AUTO is set the sample driver choses randomly between WIA_DATA_GRAYSCALE
                // and WIA_DATA_COLOR. A real driver should base this decision on the actual scan document,
                // each document page preferrably. This sample driver uses the same test image to transfer
                // all single-file page scans in a job so this initialization is only performed once, for
                // the entire job:
                //
                lDataType = (rand() % 2) ? WIA_DATA_GRAYSCALE : WIA_DATA_COLOR;
                WIAS_TRACE((g_hInst, "Detected data type in automatic mode: %ws",
                    (WIA_DATA_GRAYSCALE == lDataType) ? L"8-bpp grayscale" : L"24-bpp RGB color"));

                //
                // Updated dependent image information properties:
                //
                hr = UpdateImageInfoProperties(pWiasContext, lDataType);
                if (FAILED(hr))
                {
                    WIAEX_ERROR((g_hInst, "Failed to update dependent image information properties, hr = 0x%08X", hr));
                }
            }
        }
        else
        {
            WIAEX_ERROR((g_hInst, "Failed to read WIA_IPA_DATATYPE property, hr = 0x%08X", hr));
        }
    }

    //
    // How many files and transfer streams the driver needs for this drvAcquireItemData call:
    //
    // a) For transfers from Feeder WIA_IPS_PAGES indicates the number of files.
    // b) For transfers from Flatbed there is always just one file.
    // c) For multi-page transfers (which this sample driver doesn't support) there would be just one file.
    //
    if (SUCCEEDED(hr))
    {
        if ((IsEqualGUID(guidItemCategory, WIA_CATEGORY_FEEDER)) || (IsEqualGUID(WIA_CATEGORY_AUTO, guidItemCategory)))
        {
            lFilesToProcess = lPagesToScan;
        }
        else
        {
            lFilesToProcess = 1;
        }

        WIAS_TRACE((g_hInst, "Files to transfer: %u", lFilesToProcess));
    }

    if (SUCCEEDED(hr))
    {
        //
        // Check first if we have a (previously uploaded in this session) imprinter/endorser image to use:
        //
        if (pWiaDriverItemContext->m_pUploadedImage)
        {
            pInputStream = pWiaDriverItemContext->m_pUploadedImage;
        }
        else
        {
            //
            // Create a new global memory stream to store the transfer data file:
            //
            hr = CreateStreamOnHGlobal(NULL, TRUE, &pInputStream);
            if (SUCCEEDED(hr) && (!pInputStream))
            {
                hr = E_FAIL;
            }
            if (FAILED(hr))
            {
                WIAEX_ERROR((g_hInst, "CreateStreamOnHGlobal failed, hr = 0x%08X", hr));
            }

            //
            // Read from resources or generate the test data file (image or metadata) to be downloaded
            // to the application, storing this data into the new memory stream:
            //
            if (SUCCEEDED(hr))
            {
                ULONG ulResource = 0;

                if (bImageTransfer)
                {
                    ulResource = (WIA_DATA_COLOR == lDataType) ? IDB_TESTIMAGE_COLOR : IDB_TESTIMAGE_GRAY;
                }
                else if ((IsEqualGUID(WIA_CATEGORY_IMPRINTER, guidItemCategory) || IsEqualGUID(WIA_CATEGORY_ENDORSER, guidItemCategory))
                    && IsEqualGUID(WiaImgFmt_BMP, pmdtc->guidFormatID))
                {
                    //
                    // The sample driver will generate itself the WiaImgFmt_CSV and WiaImgFmt_TXT files:
                    //
                    ulResource = IDB_TEST_IMPRINTER_IMAGE;
                }
                else if (IsEqualGUID(WIA_CATEGORY_BARCODE_READER, guidItemCategory) && IsEqualGUID(WiaImgFmt_XMLBAR, pmdtc->guidFormatID))
                {
                    //
                    // The sample driver hard-codes the sample WiaImgFmt_RAWBAR file:
                    //
                    ulResource = IDB_BARCODE_SAMPLE;
                }
                else if (IsEqualGUID(WIA_CATEGORY_PATCH_CODE_READER, guidItemCategory) && IsEqualGUID(WiaImgFmt_XMLPAT, pmdtc->guidFormatID))
                {
                    //
                    // The sample driver hard-codes the sample WiaImgFmt_RAWPAT file:
                    //
                    ulResource = IDB_PATCH_CODE_SAMPLE;
                }
                else if (IsEqualGUID(WIA_CATEGORY_MICR_READER, guidItemCategory) && IsEqualGUID(WiaImgFmt_XMLMIC, pmdtc->guidFormatID))
                {
                    //
                    // The sample driver hard-codes the sample WiaImgFmt_RAWMIC file:
                    //
                    ulResource = IDB_MICR_SAMPLE;
                }

                if (ulResource)
                {
                    hr = LoadTestDataResourceToStream(ulResource, pInputStream, &ulFileSize);
                    if (FAILED(hr))
                    {
                        WIAEX_ERROR((g_hInst, "Failed to load the test data file from resource %u, hr = 0x%08X", ulResource, hr));
                    }
                }
                else
                {
                    hr = LoadTestDataToStream(pWiasContext, guidItemCategory, pmdtc->guidFormatID, pInputStream, &ulFileSize);
                    if (FAILED(hr))
                    {
                        WIAEX_ERROR((g_hInst, "Failed to load the test data file from resource %u, hr = 0x%08X", ulResource, hr));
                    }
                }
            }
        }
    }

    //
    // For image transfers, if the application asks for DIB (which the driver must always support) or Raw
    // this sample driver converts the image from the attached EXIF test image to obtain the uncompressed
    // image before the first image transfer takes place. For this the driver first runs the test image
    // through the GDI+ DIB encoder to generate the DIB image to transfer to the WIA application.
    //
    if (S_OK == hr)
    {
        if (bImageTransfer && IsEqualGUID(pmdtc->guidFormatID, WiaImgFmt_BMP) || IsEqualGUID(pmdtc->guidFormatID, WiaImgFmt_RAW))
        {
            //
            // Initialize GDI+ if the driver must translate scanned images to the DIB format:
            //
            if (S_OK == hr)
            {
                WIAS_TRACE((g_hInst, "Initialize GDI++.."));
                hr = InitializeGDIPlus(&pGDIPlusToken);
                if (S_OK != hr)
                {
                    WIAEX_ERROR((g_hInst, "Failed to initialize GDI+, hr = 0x%08X", hr));
                }
            }

            //
            // Load the image to a GDI+ Image object (GDI+ cannot convert the image directly from the stream):
            //
            Image *pInputImage = NULL;
            if (S_OK == hr)
            {
                pInputImage = Image::FromStream(pInputStream);
                if (!pInputImage)
                {
                    hr = E_FAIL;
                    WIAEX_ERROR((g_hInst, "Image::FromStream failed, hr = 0x%08X", hr));
                }
            }

            //
            // Release the input stream now, no longer needed, the same image is held by the Image object:
            //
            pInputStream->Release();
            pInputStream = NULL;

            //
            // Hand the Image object to GDI+ to generate the DIB image:
            //
            if (S_OK == hr)
            {
                hr = ConvertImageToDIB(pInputImage, &pOutputStream,
                    &lGDIPlus_PixelsPerLine, &lGDIPlus_NumberOfLines, &lGDIPlus_BytesPerLine);

                if (S_OK != hr)
                {
                    WIAEX_ERROR((g_hInst, "Failed to convert scanned image file to DIB, hr = 0x%08X", hr));
                }
            }

            //
            // Free the GDI+ Image copy:
            //
            if (pInputImage)
            {
                delete pInputImage;
            }

            //
            // Done with file format conversions, if any. Shutdown GDI+, no longer needed:
            //
            if (pGDIPlusToken)
            {
                WIAS_TRACE((g_hInst, "Shutdown GDI++.."));
                ShutdownGDIPlus(pGDIPlusToken);
                pGDIPlusToken = NULL;
            }

            //
            // For a Raw image transfer, further convert the DIB to an uncompressed Raw image file:
            //
            if ((S_OK == hr) && IsEqualGUID(pmdtc->guidFormatID, WiaImgFmt_RAW))
            {
                IStream *pTempOutputStream = NULL;

                hr = ConvertDibToRaw(pOutputStream, &pTempOutputStream);
                if (S_OK == hr)
                {
                    //
                    // Release pOutputStream then reset its pointer to the new stream:
                    //
                    pOutputStream->Release();
                    pOutputStream = pTempOutputStream;
                    pTempOutputStream = NULL;
                }
            }

            //
            // If not in auto-config mode check the actual image dimensions reported by GDI+.
            // In both programmed and auto-config mode update the estimated file size from
            // the image dimensions and depth reported by GDI+ during the conversion process:
            //
            if (S_OK == hr)
            {
                if (!IsEqualGUID(WIA_CATEGORY_AUTO, guidItemCategory))
                {
                    HRESULT hrTemp = S_OK;

                    //
                    // Update WIA_IPA_PIXELS_PER_LINE, WIA_IPA_NUMBER_OF_LINES and WIA_IPA_BYTES_PER_LINE:
                    //
                    hrTemp = wiasWritePropLong(pWiasContext, WIA_IPA_PIXELS_PER_LINE, lGDIPlus_PixelsPerLine);
                    if (FAILED(hrTemp))
                    {
                        WIAEX_ERROR((g_hInst, "Failed to update WIA_IPA_PIXELS_PER_LINE to %u, hr = 0x%08X",
                            lGDIPlus_PixelsPerLine, hrTemp));
                    }
                    if (SUCCEEDED(hrTemp))
                    {
                        hrTemp = wiasWritePropLong(pWiasContext, WIA_IPA_NUMBER_OF_LINES, lGDIPlus_NumberOfLines);
                        if (FAILED(hrTemp))
                        {
                            WIAEX_ERROR((g_hInst, "Failed to update WIA_IPA_NUMBER_OF_LINES to %u, hr = 0x%08X",
                                lGDIPlus_NumberOfLines, hrTemp));
                        }
                    }
                    if (SUCCEEDED(hrTemp))
                    {
                        hrTemp = wiasWritePropLong(pWiasContext, WIA_IPA_BYTES_PER_LINE, lGDIPlus_BytesPerLine);
                        if (FAILED(hrTemp))
                        {
                            WIAEX_ERROR((g_hInst, "Failed to update WIA_IPA_BYTES_PER_LINE to %u, hr = 0x%08X",
                                lGDIPlus_BytesPerLine, hrTemp));
                        }
                    }
                }
            }
        }
    }

    //
    // Estimate the size of uncompressed data we need to transfer for each file, assuming
    // all images in the current acquisition sequence will be scanned using the same scan
    // parameters. The estimate is relatively accurate for uncompressed data. Cannot use
    // this kind of estimate for compressed data because of the unknown compression ratio:
    //
    if ((S_OK == hr)  && bImageTransfer && (!IsEqualGUID(WIA_CATEGORY_AUTO, guidItemCategory)))
    {
        //
        // Read the image information properties:
        //
        hr = wiasReadPropLong(pWiasContext, WIA_IPA_NUMBER_OF_LINES, &lNumberOfLines, NULL, TRUE);
        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Failed reading WIA_IPA_NUMBER_OF_LINES, hr = 0x%08X", hr));
        }

        if (S_OK == hr)
        {
            hr = wiasReadPropLong(pWiasContext, WIA_IPA_BYTES_PER_LINE, &lBytesPerLine, NULL, TRUE);
            if (FAILED(hr))
            {
                WIAEX_ERROR((g_hInst, "Failed reading WIA_IPA_BYTES_PER_LINE, hr = 0x%08X", hr));
            }
        }

        if (S_OK == hr)
        {
            hr = wiasReadPropLong(pWiasContext, WIA_IPA_PIXELS_PER_LINE, &lPixelsPerLine, NULL, TRUE);
            if (FAILED(hr))
            {
                WIAEX_ERROR((g_hInst, "Failed reading WIA_IPA_PIXELS_PER_LINE, hr = 0x%08X", hr));
            }
        }

        if (S_OK == hr)
        {
            hr = wiasReadPropLong(pWiasContext, WIA_IPA_COMPRESSION, &lCompression, NULL, TRUE);
            if (FAILED(hr))
            {
                WIAEX_ERROR((g_hInst, "Failed reading WIA_IPA_COMPRESSION, hr = 0x%08X", hr));
            }
        }

        if ((S_OK == hr) && (lBytesPerLine > 0) && (WIA_COMPRESSION_NONE == lCompression))
        {
            ulEstimatedFileSize = lBytesPerLine * lNumberOfLines;

            if (IsEqualGUID(pmdtc->guidFormatID, WiaImgFmt_BMP))
            {
                ulEstimatedFileSize += sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
            }
            else if (IsEqualGUID(pmdtc->guidFormatID, WiaImgFmt_RAW))
            {
                ulEstimatedFileSize += sizeof(WIA_RAW_HEADER);
            }

            WIAS_TRACE((g_hInst, "Transfer file size for uncompressed image data: %02.2f KB (%u BPL, %u lines)",
                ulEstimatedFileSize / 1024.0f, lBytesPerLine, lNumberOfLines));
        }
        else
        {
            ulEstimatedFileSize = ulFileSize;
            WIAS_TRACE((g_hInst, "Transfer file size for compressed image data: %02.2f KB (%u uncompreessed BPL, %u lines)",
                ulEstimatedFileSize / 1024.0f, lBytesPerLine, lNumberOfLines));
        }
    }

    if ((S_OK == hr)  && (!bImageTransfer))
    {
        ulEstimatedFileSize = ulFileSize;
        WIAS_TRACE((g_hInst, "Transfer file size for metadata: %02.2f bytes", ulEstimatedFileSize));
    }

    //
    // Begin the actual data transfer procedure to the WIA application:
    //
    //
    #pragma prefast(suppress:__WARNING_USE_OTHER_FUNCTION, "A sample scan job duration does not exceed 49 days. If there is risk to exceed, use GetTickCount64 instead:"
    dwJobStart = GetTickCount();
    lFilesProcessed = 0;
    while (S_OK == hr)
    {
        bSkipTransfer = FALSE;

        //
        // Request a new WIA stream from the WIA client application:
        //
        if (S_OK == hr)
        {
            _Analysis_assume_nullterminated_(bstrItemName);
            hr = pTransferCallback->GetNextStream(0, bstrItemName, bstrFullItemName, &pWiaStream);
            if (S_FALSE == hr)
            {
                bCancelTransfer = TRUE;
                WIAS_TRACE((g_hInst, "IWiaMiniDrvTransferCallback::GetNextStream returned S_FALSE (0x%08X), transfer must be canceled", hr));
            }
            else if (WIA_STATUS_SKIP_ITEM == hr)
            {
                bSkipTransfer = TRUE;
                WIAS_TRACE((g_hInst, "IWiaMiniDrvTransferCallback::GetNextStream returned WIA_STATUS_SKIP_ITEM (0x%08X), transfer must be skipped", hr));
                hr = S_OK;
            }
            else if (SUCCEEDED(hr) && (S_OK != hr))
            {
                WIAEX_ERROR((g_hInst, "IWiaMiniDrvTransferCallback::GetNextStream returned an unknown success value, hr = 0x%08X", hr));
                hr = E_UNEXPECTED;
            }
            else if (FAILED(hr))
            {
                WIAEX_ERROR((g_hInst, "IWiaMiniDrvTransferCallback::GetNextStream failed, hr = 0x%08X", hr));
            }
        }

        //
        // Signal start of data transfer to the WIA application and check
        // if the application asks already for the transfers to be cancelled:
        //
        if ((S_OK == hr)  && (!bSkipTransfer))
        {
            pCallbackTransferParams->lMessage = WIA_TRANSFER_MSG_STATUS;
            pCallbackTransferParams->hrErrorStatus = 0;
            pCallbackTransferParams->lPercentComplete = 0;
            pCallbackTransferParams->ulTransferredBytes = 0;

            WIAS_TRACE((g_hInst, "Transfer callback: WIA_TRANSFER_MSG_STATUS, 0 bytes"));
            hr = pTransferCallback->SendMessage(0, pCallbackTransferParams);
            if (FAILED(hr))
            {
                WIAEX_ERROR((g_hInst, "IWiaMiniDrvTransferCallback::SendMessage failed, hr = 0x%08X", hr));
            }
            else if (S_FALSE == hr)
            {
                bCancelTransfer = TRUE;
            }
            else if (S_OK != hr)
            {
                WIAEX_ERROR((g_hInst, "IWiaMiniDrvTransferCallback::SendMessage returned unknown success value, hr = 0x%08X", hr));
                bCancelTransfer = TRUE;
                hr = S_FALSE;
            }
        }

        if (S_OK == hr)
        {
            if (bSkipTransfer)
            {
                WIAS_TRACE((g_hInst, "Discarding data to skip the current file transfer (%u)..", lFilesProcessed + 1));
            }
            else
            {
                //
                // Read the test file and transfer it to the WIA application:
                //
                if ((S_OK == hr) && (!bCancelTransfer))
                {
                    hr = TransferFile(pOutputStream ? pOutputStream : pInputStream, pWiaStream, pTransferBuffer, ulBufferSize,
                        pTransferCallback, pCallbackTransferParams, ulEstimatedFileSize, &bCancelTransfer);
                    if (FAILED(hr))
                    {
                        WIAEX_ERROR((g_hInst, "File transfer to WIA stream failed, hr = 0x%08X", hr));
                    }
                }
           }
        }

        //
        // The transfer of one file was successfully completed, reset WIA_STATUS_END_OF_MEDIA:
        //
        if (WIA_STATUS_END_OF_MEDIA == hr)
        {
            WIAS_TRACE((g_hInst, "File transfer complete (WIA_STATUS_END_OF_MEDIA)"));
            hr = S_OK;
        }

        //
        // Signal 100% transfer complete to the WIA  client application (pCallbackTransferParams->ulTransferredBytes
        // contains the total number of bytes transferred to this stream):
        //
        if ((S_OK == hr) && (!bSkipTransfer))
        {
            pCallbackTransferParams->lMessage = WIA_TRANSFER_MSG_STATUS;
            pCallbackTransferParams->hrErrorStatus = 0;
            pCallbackTransferParams->lPercentComplete = 100;

            WIAS_TRACE((g_hInst, "Transfer callback: WIA_TRANSFER_MSG_STATUS, transfer complete, %02.2f KB total (%u)",
                pCallbackTransferParams->ulTransferredBytes / 1024.0f, pCallbackTransferParams->lPercentComplete));

            hr = pTransferCallback->SendMessage(0, pCallbackTransferParams);
            if (FAILED(hr))
            {
                WIAEX_ERROR((g_hInst, "IWiaMiniDrvTransferCallback::SendMessage(WIA_TRANSFER_MSG_STATUS, transfer complete) failed, hr = 0x%08X", hr));
            }
            else if (S_FALSE == hr)
            {
                bCancelTransfer = TRUE;
            }
            else if (S_OK != hr)
            {
                WIAEX_ERROR((g_hInst, "IWiaMiniDrvTransferCallback::SendMessage(WIA_TRANSFER_MSG_STATUS, transfer complete) returned unknown success value, hr = 0x%08X", hr));
                bCancelTransfer = TRUE;
                hr = S_FALSE;
            }
        }

        //
        // If the transfer was canceled from the client application send a CancelJob
        // to the scanner device to ensure scanning is stopped and all scanned image
        // data (not read yet) is discarded from the scanner internal buffer:
        //
        if (bCancelTransfer)
        {
            WIAS_TRACE((g_hInst, "Transfer cancelled from WIA application.."));

            //
            // Make sure hr is left set to S_FALSE (cancelled):
            //
            hr = S_FALSE;
        }
        //
        // The current job must be canceled at the device if there is an error during the data
        // transfer phase, error which may result in leaving the current job in a processing
        // state and risk to block the scanner from accepting new jobs until this job times out:
        //
        else if (FAILED(hr))
        {
            WIAS_TRACE((g_hInst, "Transfer failed (hr = 0x%08X), abort job..", hr));
        }

        //
        // Clean-up after finishing each individual WIA file transfer:
        //

        pCallbackTransferParams->lPercentComplete = 0;
        pCallbackTransferParams->ulTransferredBytes = 0;

        //
        // Release the current WIA transfer stream:
        //
        if (pWiaStream)
        {
            pWiaStream->Release();
            pWiaStream = NULL;
        }

        //
        // Note the loop ends when hr != S_OK.
        //
        // Increment the number of streams (files) transferred
        // and check if we must end the transfers here:
        //
        if (S_OK == hr)
        {
            //
            // If a finite number of files must be processed (transferred and/or skipped):
            //
            lFilesProcessed++;

            WIAS_TRACE((g_hInst, "Successfully transferred %u file(s)", lFilesProcessed));

            if (lFilesToProcess && (lFilesProcessed >= lFilesToProcess))
            {
                //
                // Note that in general for the Feeder item the driver must signal
                // end of paper, not S_OK (as documented by MSDN), to ensure that the
                // application does not request another drvAcquireItemData. In this
                // case however we transfer multiple files in a single call so we
                // won't do this:
                //
                // if (IsEqualGUID(WIA_CATEGORY_FEEDER, guidItemCategory))
                // {
                //     hr = WIA_ERROR_PAPER_EMPTY;
                // }
                //

                //
                // Do not transfer another file for this call:
                //
                hr = WIA_STATUS_END_OF_MEDIA;
            }
            //
            // For feeder acquisitions, if job separators and/or multi-feed detection is enabled,
            // check if the sample driver needs to perform an action here (such as stopping the scan):
            //
            else if ((!(lFilesProcessed % JOB_SEPARATOR_AT_PAGE)) && (WIA_SEPARATOR_DISABLED != lJobSeparators))
            {
                WIAS_TRACE((g_hInst, "Job separator detected"));
                if ((WIA_SEPARATOR_DETECT_SCAN_STOP == lJobSeparators) || (WIA_SEPARATOR_DETECT_NOSCAN_STOP == lJobSeparators))
                {
                    WIAS_TRACE((g_hInst, "End of scan job due to job separator"));
                    hr = WIA_STATUS_END_OF_MEDIA;
                }
            }
            else if ((!(lFilesProcessed % MULTI_FEED_AT_PAGE)) && (WIA_MULTI_FEED_DETECT_DISABLED != lMultiFeed))
            {
                WIAS_TRACE((g_hInst, "Multi-feed detected"));
                if (WIA_MULTI_FEED_DETECT_STOP_SUCCESS == lMultiFeed)
                {
                    WIAS_TRACE((g_hInst, "End of scan job due to multi-feed"));
                    hr = WIA_STATUS_END_OF_MEDIA;
                }
                else if (WIA_MULTI_FEED_DETECT_STOP_ERROR == lMultiFeed)
                {
                    WIAS_TRACE((g_hInst, "Failed scan job due to multi-feed"));
                    hr = WIA_ERROR_MULTI_FEED;
                }
            }

            //
            // For image transfers, if lFileToTransfer is 0 (ALL_PAGES) the transfers end (with S_OK) when either:
            //
            // 1) an error occurrs;
            // 2) a file transfer is canceled (either from the app or the scanner);
            // 3) when RetrieveImage would return WIA_ERROR_PAPER_EMPTY.
            //
        }
    }

    //
    // Compute the number of pages per minute (PPM) transferred for this job:
    //
    if ((lFilesProcessed > 0) && bImageTransfer)
    {
        #pragma prefast(suppress:__WARNING_USE_OTHER_FUNCTION, "A sample scan job duration does not exceed 49 days. If there is risk to exceed, use GetTickCount64 instead:"
        DWORD dwJobEnd = GetTickCount();

        if (dwJobEnd > dwJobStart)
        {
            float fSecondsElapsed = (dwJobEnd - dwJobStart) / 1000.0f;

            if (lFilesProcessed > 1)
            {
                DWORD dwPagesPerMinute = (DWORD)((60 * lFilesProcessed) / fSecondsElapsed);
                WIAS_TRACE((g_hInst, "Measured scan performance for this job is %u PPM (%u single-page image files transferred in %0.2f seconds)",
                    dwPagesPerMinute, lFilesProcessed, fSecondsElapsed));
            }
            else
            {
                WIAS_TRACE((g_hInst, "1 image file transferred in %0.2f seconds", fSecondsElapsed));
            }
        }
    }

    //
    // Keep the image in the item context, if there is one:
    //
    if (pWiaDriverItemContext->m_pUploadedImage)
    {
        pInputStream = NULL;
    }

    //
    // Clean-up for memory streams (set to automatically free memory on release)
    // used for the test image:
    //
    if (pInputStream)
    {
        pInputStream->Release();
    }
    if (pOutputStream)
    {
        pOutputStream->Release();
    }

    //
    // The transfers (of all files) were successfully completed, reset WIA_STATUS_END_OF_MEDIA:
    //
    if (WIA_STATUS_END_OF_MEDIA == hr)
    {
        WIAS_TRACE((g_hInst, "All files successfully transferred"));

        hr = S_OK;
    }

    //
    // If this job call is about to return WIA_ERROR_PAPER_EMPTY we have to consider
    // first the following conditions:
    //
    // - if an undefined number of transfers had to be performed and at least
    //   one file was transferred the return code must be changed to S_OK;
    //
    // - if a finite number of transfers had to be performed and all these
    //   transfers have been done the return code must be changed to S_OK;
    //
    // - if a finite number of transfers had to be performed and at least one
    //   of these transfers - but not all of them -  has been done the return
    //   code must be changed to WIA_STATUS_END_OF_MEDIA;
    //
    // - WIA_ERROR_PAPER_EMPTY must be returned only when no transfers could
    //   be completed in the current drvAcquireItemData call.
    //
    if (WIA_ERROR_PAPER_EMPTY == hr)
    {
        if (((!lFilesToProcess) && (lFilesProcessed >= 1)) ||
            ((lFilesToProcess > 0) && (lFilesProcessed >= lFilesToProcess)))
        {
            WIAS_TRACE((g_hInst, "Running out of paper, returning S_OK for drvAcquireItemData.."));
            hr = S_OK;
        }
        else if ((lFilesToProcess > 0) && (lFilesProcessed < lFilesToProcess) && (lFilesProcessed >= 1))
        {
            WIAS_TRACE((g_hInst, "Running out of paper, returning WIA_STATUS_END_OF_MEDIA for drvAcquireItemData.."));
            hr = WIA_STATUS_END_OF_MEDIA;
        }
        else
        {
            WIAS_TRACE((g_hInst, "Running out of paper, returning WIA_ERROR_PAPER_EMPTY for drvAcquireItemData.."));
        }
    }
    else if (WIA_ERROR_MULTI_FEED == hr)
    {
        WIAS_TRACE((g_hInst, "Multi-feed error, returning WIA_ERROR_MULTI_FEED for drvAcquireItemData.."));
    }

    WIAEX_TRACE_FUNC_HR;

    return hr;
}

/**************************************************************************\
*
* Helper for CWiaDriver::drvAcquireItemData. Executes the upload data
* transfer sequence for the current scan job,
*
* Parameters:
*
*    pWiasContext            - item context
*    guidItemCategory        - item category
*    bstrItemName            - item name
*    bstrFullItemName        - full item name
*    pmdc                    - driver transfer context data
*    pTransferBuffer         - pre-allocated transfer buffer
*    ulBufferSize            - size of the pre-allocated transfer buffer, in bytes
*    pTransferCallback       - IWiaMiniDrvTransferCallback* for WIA status
*    pCallbackTransferParams - WiaTransferParams* for WIA status callbacks
*    ulEstimatedFileSize     - estimated file size, in bytes (0 if unknown)
*
* Return Value:
*
*    S_OK if successful, S_FALSE if the transfer is canceled
*    or an error HRESULT if an error occurrs
*
\**************************************************************************/

HRESULT
CWiaDriver::Upload(
    _In_                      BYTE                        *pWiasContext,
                              GUID                         guidItemCategory,
    _In_                      BSTR                         bstrItemName,
    _In_                      BSTR                         bstrFullItemName,
    _In_reads_bytes_(ulBufferSize) BYTE                        *pTransferBuffer,
                              ULONG                        ulBufferSize,
    _In_                      IWiaMiniDrvTransferCallback *pTransferCallback,
    _In_                      WiaTransferParams           *pCallbackTransferParams,
    _In_                      WIA_DRIVER_ITEM_CONTEXT     *pWiaDriverItemContext)
{
    HRESULT hr = S_OK;

    IStream *pInputStream = NULL;
    IStream *pWiaStream = NULL;
    BOOL bCancelTransfer = FALSE;
    BOOL bSkipTransfer = FALSE;
    GUID guidUploadFormat = GUID_NULL;

    WIAEX_TRACE_BEGIN;

    if ((!pWiasContext) || (!bstrItemName) || (!bstrFullItemName) || (!pTransferBuffer) || (!pTransferCallback) || (!pCallbackTransferParams))
    {
        hr = E_INVALIDARG;
        WIAEX_ERROR((g_hInst, "Invalid parameter, hr = 0x%08X", hr));
    }

    if ((!IsEqualGUID(WIA_CATEGORY_IMPRINTER, guidItemCategory)) && (!IsEqualGUID(WIA_CATEGORY_ENDORSER, guidItemCategory)))
    {
        hr = E_INVALIDARG;
        WIAEX_ERROR((g_hInst, "This driver does not support upload transfer direction for the requested item, hr = 0x%08X", hr));
    }

    //
    // Create a new global memory stream to store the transfer data file:
    //
    if (SUCCEEDED(hr))
    {
        hr = CreateStreamOnHGlobal(NULL, TRUE, &pInputStream);
        if (SUCCEEDED(hr) && (!pInputStream))
        {
            hr = E_FAIL;
        }
        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "CreateStreamOnHGlobal failed, hr = 0x%08X", hr));
        }
    }

    //
    // This sample driver executes a very simple data transfer upload for the imprinter/endorser data.
    //
    // For WiaImgFmt_BMP transfers the driver validates that the image dimensions match
    // WIA_IPS_PRINTER_ENDORSER_GRAPHICS_MIN/MAX_WIDTH/HEIGHT dimensions.
    //
    // For WiaImgFmt_TXT and WiaImgFmt_CSV transfers the driver loads the transferred text and
    // attempts to update the current WIA_IPS_PRINTER_ENDORSER_STRING value with it, performing
    // validation as if the application would directly set WIA_IPS_PRINTER_ENDORSER_STRING.
    //
    // It was already validated that WIA_MINIDRV_TRANSFER_UPLOAD works only for
    // WIA_CATEGORY_IMPRINTER and WIA_CATEGORY_ENDORSER.
    //

    //
    // Request the WIA transfer stream from the WIA client application:
    //
    if (SUCCEEDED(hr))
    {
        _Analysis_assume_nullterminated_(bstrItemName);
        hr = pTransferCallback->GetNextStream(0, bstrItemName, bstrFullItemName, &pWiaStream);
        if (S_FALSE == hr)
        {
            bCancelTransfer = TRUE;
            WIAS_TRACE((g_hInst, "IWiaMiniDrvTransferCallback::GetNextStream returned S_FALSE (0x%08X), transfer must be canceled", hr));
        }
        else if (WIA_STATUS_SKIP_ITEM == hr)
        {
            bSkipTransfer = TRUE;
            WIAS_TRACE((g_hInst, "IWiaMiniDrvTransferCallback::GetNextStream returned WIA_STATUS_SKIP_ITEM (0x%08X), transfer must be skipped", hr));
            hr = S_OK;
        }
        else if (SUCCEEDED(hr) && (S_OK != hr))
        {
            WIAEX_ERROR((g_hInst, "IWiaMiniDrvTransferCallback::GetNextStream returned an unknown success value, hr = 0x%08X", hr));
            hr = E_UNEXPECTED;
        }
        else if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "IWiaMiniDrvTransferCallback::GetNextStream failed, hr = 0x%08X", hr));
        }
    }

    //
    // Signal start of data transfer from the WIA application and check if the application asks for the transfer to be cancelled:
    //
    if ((S_OK == hr)  && (!bSkipTransfer))
    {
        pCallbackTransferParams->lMessage = WIA_TRANSFER_MSG_STATUS;
        pCallbackTransferParams->hrErrorStatus = 0;
        pCallbackTransferParams->lPercentComplete = 0;
        pCallbackTransferParams->ulTransferredBytes = 0;

        WIAS_TRACE((g_hInst, "Transfer callback: WIA_TRANSFER_MSG_STATUS, 0 bytes"));
        hr = pTransferCallback->SendMessage(0, pCallbackTransferParams);
        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "IWiaMiniDrvTransferCallback::SendMessage failed, hr = 0x%08X", hr));
        }
        else if (S_FALSE == hr)
        {
            bCancelTransfer = TRUE;
        }
        else if (S_OK != hr)
        {
            WIAEX_ERROR((g_hInst, "IWiaMiniDrvTransferCallback::SendMessage returned unknown success value, hr = 0x%08X", hr));
            bCancelTransfer = TRUE;
            hr = S_FALSE;
        }
    }

    if ((S_OK == hr) && (!bSkipTransfer))
    {
        //
        // Transfer the file from the WIA application:
        //
        if ((S_OK == hr) && (!bCancelTransfer))
        {
            hr = TransferFile(pWiaStream, pInputStream, pTransferBuffer, ulBufferSize,
                pTransferCallback, pCallbackTransferParams, 0, &bCancelTransfer);
            if (FAILED(hr))
            {
                WIAEX_ERROR((g_hInst, "File transfer from the WIA stream failed, hr = 0x%08X", hr));
            }
        }
    }

    //
    // The transfer of one file was successfully completed, reset WIA_STATUS_END_OF_MEDIA:
    //
    if (WIA_STATUS_END_OF_MEDIA == hr)
    {
        WIAS_TRACE((g_hInst, "File transfer complete (WIA_STATUS_END_OF_MEDIA)"));
        hr = S_OK;
    }

    //
    // Signal 100% transfer complete to the WIA  client application (pCallbackTransferParams->ulTransferredBytes
    // contains the total number of bytes transferred to this stream):
    //
    if ((S_OK == hr) && (!bSkipTransfer))
    {
        pCallbackTransferParams->lMessage = WIA_TRANSFER_MSG_STATUS;
        pCallbackTransferParams->hrErrorStatus = 0;
        pCallbackTransferParams->lPercentComplete = 100;

        WIAS_TRACE((g_hInst, "Transfer callback: WIA_TRANSFER_MSG_STATUS, transfer complete, %02.2f KB total (%u)",
            pCallbackTransferParams->ulTransferredBytes / 1024.0f, pCallbackTransferParams->lPercentComplete));

        hr = pTransferCallback->SendMessage(0, pCallbackTransferParams);
        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "IWiaMiniDrvTransferCallback::SendMessage(WIA_TRANSFER_MSG_STATUS, transfer complete) failed, hr = 0x%08X", hr));
        }
    }


    //
    // Clean-up after finishing each individual WIA file transfer:
    //

    pCallbackTransferParams->lPercentComplete = 0;
    pCallbackTransferParams->ulTransferredBytes = 0;

    //
    // Release the current WIA transfer stream:
    //
    if (pWiaStream)
    {
        pWiaStream->Release();
        pWiaStream = NULL;
    }

    //
    // For upload transfers we need to check the current WIA_IPA_FORMAT to see what
    // format is the file that was uploaded by the application, the MINIDRV_TRANSFER_CONTEXT
    // structure not being initialized with the right file format in this case.
    // Remember that the WIA application needs to set WIA_IPA_FORMAT before calling
    // IWiaTransfer::Upload:
    //
    if (S_OK == hr)
    {
        hr = wiasReadPropGuid(pWiasContext, WIA_IPA_FORMAT, &guidUploadFormat, NULL, TRUE);
        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Failed to read WIA_IPA_FORMAT, hr = 0x%08X", hr));
        }
    }

    if (S_OK == hr)
    {
        //
        // At this point pInputStream contains the data uploaded by the application
        //
        if (IsEqualGUID(WiaImgFmt_BMP, guidUploadFormat))
        {
            if (SUCCEEDED(IsDibValid(pInputStream, 1, IMPRINTER_MAX_WIDTH, IMPRINTER_MAX_HEIGHT)))
            {
                if (pWiaDriverItemContext->m_pUploadedImage)
                {
                    IStream *pTemp = pWiaDriverItemContext->m_pUploadedImage;
                    pWiaDriverItemContext->m_pUploadedImage = NULL;
                    pTemp->Release();
                }
                pWiaDriverItemContext->m_pUploadedImage = pInputStream;
                pInputStream = NULL;
            }
            else
            {
                hr = E_INVALIDARG;
                WIAEX_ERROR((g_hInst,
                    "The uploaded WiaImgFmt_BMP (DIB) image is invalid. This item accepts only a 1-bpp %ux%u pixels image, bottom to top line order, hr = 0x%08X",
                    IMPRINTER_MAX_WIDTH, IMPRINTER_MAX_HEIGHT, hr));
            }
        }
        else if (IsEqualGUID(WiaImgFmt_TXT, guidUploadFormat) || IsEqualGUID(WiaImgFmt_CSV, guidUploadFormat))
        {
            if (FAILED(IsImprinterEndorserTextValid(pWiasContext, pInputStream, IsEqualGUID(WIA_CATEGORY_IMPRINTER, guidItemCategory) ? IMPRINTER : ENDORSER)))
            {
                hr = E_INVALIDARG;
                WIAEX_ERROR((g_hInst, "The uploaded text data is invalid, hr = 0x%08X", hr));
            }
        }
        else
        {
            USHORT *pUUID = NULL;
            hr = E_INVALIDARG;

            if (RPC_S_OK == UuidToStringW(&guidUploadFormat, &pUUID))
            {
                WIAEX_ERROR((g_hInst, "Unsupported upload transfer file format (%ws), hr = 0x%08X", pUUID, hr));
                RpcStringFreeW(&pUUID);
            }
            else
            {
                WIAEX_ERROR((g_hInst, "Unsupported upload transfer file format, hr = 0x%08X", hr));
            }
        }
    }

    if (pInputStream)
    {
        pInputStream->Release();
    }


    WIAEX_TRACE_FUNC_HR;

    return hr;
}

/**************************************************************************\
*
* Helper for CWiaDriver::drvAcquireItemData. Transfers one file (image or
* metadata) to the WIA transfer stream provided by the aplication. Note that
* for image file transfers it is not a good solution to let GDI+ to write the
* image directly to the WIA transfer stream as the WIA stream has special
* requirements such as predetermined buffer size, reposition of the write
* cursor at the beginning of the stream between writes and WIA notifications.
*
* Parameters:
*
*    pInputStream            - IStream object to read the file from
*    pDestinationStream      - IStream object to write the file to
*    pTransferBuffer         - pre-allocated transfer buffer
*    ulBufferSize            - size of the pre-allocated transfer buffer, in bytes
*    pTransferCallback       - optional IWiaMiniDrvTransferCallback* for WIA status
*    pCallbackTransferParams - optional WiaTransferParams* for WIA status callbacks
*    ulEstimatedFileSize     - estimated file size, in bytes (0 if unknown)
*    pbCancelTransfer        - on return indicates if the WIA client canceled
*                              the operation
*
* Remarks:
*
*    The caller may specify a non zero pCallbackTransferParams->lPercentComplete
*    value (between 0% and 49%) to have this function resume incrementing the
*    percent complete starting from this value. An invalid value is set to 0%
*    and does not cause the function to fail (still, the caller must not do this).
*
* Return Value:
*
*    WIA_STATUS_END_OF_MEDIA if successful, S_FALSE if the transfer
*    is canceled or an error HRESULT if an error occurrs
*
\**************************************************************************/

HRESULT
CWiaDriver::TransferFile(
    _In_                      IStream                     *pInputStream,
    _In_                      IStream                     *pDestinationStream,
    _In_reads_bytes_(ulBufferSize)
                              BYTE                        *pTransferBuffer,
                              ULONG                        ulBufferSize,
    _In_opt_                  IWiaMiniDrvTransferCallback *pTransferCallback,
    _In_opt_                  WiaTransferParams           *pCallbackTransferParams,
                              ULONG                        ulEstimatedFileSize,
    _Out_                     BOOL                        *pbCancelTransfer)
{
    HRESULT hr = S_OK;
    HRESULT hrTemp = S_OK;

    ULONG ulBytesToRead = 0;
    ULONG ulBytesRead = 0;
    ULONG ulBytesToWrite = 0;
    ULONG ulBytesWritten = 0;
    ULONG ulFileBytesWritten = 0;
    ULONG ulPercentComplete = 0;
    ULONG ulStartProgressFrom = 0;

    const LARGE_INTEGER liZeroOffset = {};

    WIAEX_TRACE_BEGIN;

    if ((!pInputStream) || (!pDestinationStream) || (!pTransferBuffer) || (!pbCancelTransfer))
    {
        hr = E_INVALIDARG;
        WIAEX_ERROR((g_hInst, "Invalid parameter, hr = 0x%08X", hr));
    }
    else
    {
        *pbCancelTransfer = FALSE;
    }

    if ((S_OK == hr) && pTransferCallback && pCallbackTransferParams && pCallbackTransferParams->lPercentComplete)
    {
        if ((pCallbackTransferParams->lPercentComplete < 0) || (pCallbackTransferParams->lPercentComplete > 49))
        {
            WIAEX_ERROR((g_hInst, "Invalid WiaTransferParams::lPercentComplete parameter (%d), reset to 0",
                pCallbackTransferParams->lPercentComplete));
            ulStartProgressFrom = 0;
        }
        else
        {
            ulStartProgressFrom = (ULONG)pCallbackTransferParams->lPercentComplete;
            WIAS_TRACE((g_hInst, "Resuming progress indicator from %u", ulStartProgressFrom));
        }
    }

    if (S_OK == hr)
    {
        //
        // Make sure the input stream pointer is at the beginning of the stream before reading from it:
        //
        hr = pInputStream->Seek(liZeroOffset, STREAM_SEEK_SET, NULL);
        if (S_OK != hr)
        {
            WIAEX_ERROR((g_hInst, "IStream::Seek(0, STREAM_SEEK_SET, NULL) failed, hr = 0x%08X", hr));
        }
    }


    //
    // Resume from current pCallbackTransferParams->lPercentComplete,
    // do not reset to 0 pCallbackTransferParams->lPercentComplete:
    //
    if ((S_OK == hr) && pCallbackTransferParams)
    {
        pCallbackTransferParams->ulTransferredBytes = 0;
    }

    while (S_OK == hr)
    {
        ulBytesRead = 0;
        ulBytesToRead = ulBufferSize;
        ulBytesWritten = 0;
        ulBytesToWrite = 0;
        *pbCancelTransfer = FALSE;

        //
        // Read one buffer of data from the input stream. Note the source IStream
        // may return S_OK and no data when the transfer is complete:
        //
        hr = pInputStream->Read(pTransferBuffer, ulBytesToRead, &ulBytesRead);
        if ((S_FALSE == hr) || ((S_OK == hr) && (!ulBytesRead)))
        {
            //
            // End of file, transfer of this file should be complete:
            //
            hr = WIA_STATUS_END_OF_MEDIA;
            WIAS_TRACE((g_hInst, "IStream::Read: end of media, data transfer complete"));
        }
        else if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "IStream::Read failed, hr = 0x%08X", hr));
        }
        else if (ulBytesRead > ulBytesToRead)
        {
            hr = E_UNEXPECTED;
            WIAEX_ERROR((g_hInst, "IStream::Read caused a possible buffer overflow (%u bytes over %u limit), hr = 0x%08X",
                ulBytesRead - ulBytesToRead, ulBytesToRead, hr));
        }

        //
        // Write the the read buffer content to the destination stream:
        //
        if ((S_OK == hr) || ((WIA_STATUS_END_OF_MEDIA == hr) && (ulBytesRead > 0)))
        {
            //
            // Make sure the write stream pointer is at the end of the stream before writing to it:
            //
            hrTemp = pDestinationStream->Seek(liZeroOffset, STREAM_SEEK_END, NULL);
            if (FAILED(hrTemp))
            {
                WIAEX_ERROR((g_hInst, "IStream::Seek(0, STREAM_SEEK_END, NULL) failed, hr = 0x%08X", hrTemp));
            }

            //
            // Write the buffer to the destination stream:
            //
            if (S_OK == hrTemp)
            {
                ulBytesToWrite = ulBytesRead;

                hrTemp = pDestinationStream->Write(pTransferBuffer, ulBytesToWrite, &ulBytesWritten);
                if (FAILED(hrTemp))
                {
                    WIAEX_ERROR((g_hInst, "IStream::Write(%u bytes) failed, hr = 0x%08X", ulBytesToWrite, hrTemp));
                }
            }

            //
            // Make progress callback to the WIA client application and check for a cancel transfer request:
            //
            if (S_OK == hrTemp)
            {
                if (pTransferCallback && pCallbackTransferParams)
                {
                    ulFileBytesWritten += ulBytesWritten;

                    //
                    // Indicate to ComputeTransferProgress a direct/full transfer (0% .. 99%):
                    //
                    ComputeTransferProgress(&ulPercentComplete, ulEstimatedFileSize, ulFileBytesWritten, TRUE, ulStartProgressFrom);

                    pCallbackTransferParams->lMessage = WIA_TRANSFER_MSG_STATUS;
                    pCallbackTransferParams->ulTransferredBytes = ulFileBytesWritten;
                    pCallbackTransferParams->lPercentComplete = ulPercentComplete;

                    WIAS_TRACE((g_hInst, "Transfer callback: WIA_TRANSFER_MSG_STATUS, %u bytes, %02.2f KB total (%u)",
                        ulBytesWritten, pCallbackTransferParams->ulTransferredBytes / 1024.0f, ulPercentComplete));

                    hrTemp = pTransferCallback->SendMessage(0, pCallbackTransferParams);
                    if (FAILED(hrTemp))
                    {
                        WIAEX_ERROR((g_hInst, "IWiaMiniDrvTransferCallback::SendMessage(%u bytes written) failed, hr = 0x%08X",
                            ulBytesWritten, hrTemp));
                    }
                    else if (S_FALSE == hrTemp)
                    {
                        *pbCancelTransfer = TRUE;
                    }
                    else if (S_OK != hrTemp)
                    {
                        WIAEX_ERROR((g_hInst, "IWiaMiniDrvTransferCallback::SendMessage(%u bytes written) returned unknown success value, hr = 0x%08X",
                            ulBytesWritten, hrTemp));
                        *pbCancelTransfer = TRUE;
                        hrTemp = S_FALSE;
                    }
                }
                else
                {
                    WIAS_TRACE((g_hInst, "Transferred %u bytes, %02.2f KB total (%u)", ulBytesWritten,
                        ulFileBytesWritten / 1024.0f, ulPercentComplete));
                }
            }

            //
            // Preserve WIA_STATUS_END_OF_MEDIA to allow this loop to normally end, unless a failure was encountered:
            //
            if ((S_OK != hrTemp) || (WIA_STATUS_END_OF_MEDIA != hr))
            {
                hr = hrTemp;
            }
        }
    }

    WIAEX_TRACE_FUNC_HR;

    return hr;
}

/**************************************************************************\
*
* Helper for IWiaMiniDrv::drvAcquireItemData. Executes a simple validation
* for the WiaImgFmt_BMP (DIB) image uploaded by the application and stored
* in the specified stream.
*
* Parameters:
*
*    pStream   - stream source for the image to be validated
*    lBitDepth - expected pixel bit depth (e.g. 24)
*    lWidth    - expected image width, in pixels
*    lHeight   - expected image height, in pixels; also describes
*                the DIB line order (if negative the image data is
*                top to bottom, positive means bottom to top)
*
* Return Value:
*
*    S_OK if successful and the image is valid, E_INVALIDARG if the image is
*    invalid or another error HRESULT if another error occurrs and the image
*    cannot be validated
*
\**************************************************************************/

HRESULT
CWiaDriver::IsDibValid(
    _In_ IStream* pStream,
         LONG     lBitDepth,
         LONG     lWidth,
         LONG     lHeight)
{
    HRESULT hr = S_OK;
    LARGE_INTEGER lStart = {};
    BITMAPINFO bi = {};

    WIAEX_TRACE_BEGIN;

    if (!pStream)
    {
        hr = E_INVALIDARG;
        WIAEX_ERROR((g_hInst, "Invalid parameter, hr = 0x%08X", hr));
    }

    if (SUCCEEDED(hr))
    {
        lStart.LowPart = sizeof(BITMAPFILEHEADER);

        hr = pStream->Seek(lStart, STREAM_SEEK_SET, NULL);
        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Failed to reset the input stream, hr = 0x%08X", hr));
        }
    }

    if (S_OK == hr)
    {
        hr = pStream->Read(&bi, sizeof(bi), NULL);
        if (S_OK != hr)
        {
            WIAEX_ERROR((g_hInst, "Failed to read the DIB header from the image, hr = 0x%08X", hr));
            if (SUCCEEDED(hr))
            {
                hr = E_INVALIDARG;
            }
        }
    }

    if (SUCCEEDED(hr) && (bi.bmiHeader.biBitCount != lBitDepth))
    {
        hr = E_INVALIDARG;
        WIAEX_ERROR((g_hInst, "Invalid DIB pixel bit depth, got %u bits, expected %u bit(s), hr = 0x%08X",
            bi.bmiHeader.biBitCount, lBitDepth, hr));
    }

    if (SUCCEEDED(hr) && (bi.bmiHeader.biWidth != lWidth))
    {
        hr = E_INVALIDARG;
        WIAEX_ERROR((g_hInst, "Invalid DIB width, got %u pixels, expected %u pixels, hr = 0x%08X",
            bi.bmiHeader.biWidth, lWidth, hr));
    }

    if (SUCCEEDED(hr) && (bi.bmiHeader.biHeight != lHeight))
    {
        hr = E_INVALIDARG;
        WIAEX_ERROR((g_hInst, "Invalid DIB height and/or line order, got %l pixels, expected %l pixels, hr = 0x%08X",
            bi.bmiHeader.biHeight, lHeight, hr));
    }

    if (S_OK == hr)
    {
        lStart.LowPart = 0;

        hr = pStream->Seek(lStart, STREAM_SEEK_SET, NULL);
        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Failed to reset the input stream, hr = 0x%08X", hr));
        }
    }

    WIAEX_TRACE_FUNC_HR;

    return hr;

}

/**************************************************************************\
*
* Helper for IWiaMiniDrv::drvAcquireItemData. Executes a simple validation
* for the WiaImgFmt_TXT and WiaImgFmt_CSV imprinter/endorser text uploaded
* by the application and stored in the specified stream and if the text is
* valid it updates WIA_IPS_PRINTER_ENDORSER_STRING.
*
* Parameters:
*
*    pWiasContext            - item context
*    pStream                 - stream source for the image to be validated
*    nDocumentHandlingSelect - IMPRINTER or ENDORSER (defined in wiadef.h)
*
* Return Value:
*
*    S_OK if successful and the image is valid, E_INVALIDARG or another
*    error HRESULT if validation fails or if it cannot be performed
*
\**************************************************************************/

HRESULT
CWiaDriver::IsImprinterEndorserTextValid(
    _In_ BYTE*    pWiasContext,
    _In_ IStream* pStream,
         LONG     nDocumentHandlingSelect)
{
    HRESULT hr = S_OK;
    LARGE_INTEGER liZero = {};
    ULARGE_INTEGER ui = {};
    ULONG ulNumChars = 0;
    ULONG ulDataSize = 0;
    PWCHAR pwData = NULL;
    BSTR bstrData = NULL;

    WIAEX_TRACE_BEGIN;

    if (!pStream)
    {
        hr = E_INVALIDARG;
        WIAEX_ERROR((g_hInst, "Invalid parameter, hr = 0x%08X", hr));
    }

    if (SUCCEEDED(hr))
    {
        hr = IStream_Size(pStream, &ui);
        if (SUCCEEDED(hr))
        {
            WIAS_TRACE((g_hInst, "Stream size, low: %u bytes, high: %u bytes",
                ui.LowPart, ui.HighPart));

            if (ui.HighPart > 0)
            {
                WIAS_TRACE((g_hInst, "Stream size exceeeds maximum accepted by this item, last %u bytes will be ignored", ui.HighPart));
            }

            //
            // Data must contain the two bytes BOM and at least one double-byte character:
            //
            if (ui.LowPart >= (2 * sizeof(WCHAR)))
            {
                ulDataSize = ui.LowPart;
            }
            else
            {
                hr = E_INVALIDARG;
                WIAEX_ERROR((g_hInst, "Insufficient data (less than one actual character), hr = 0x%08X", hr));
            }
        }
        else
        {
            WIAEX_ERROR((g_hInst, "IStream_Size failed, hr = 0x%08X", hr));
        }
    }

    if (SUCCEEDED(hr))
    {
        hr = pStream->Seek(liZero, STREAM_SEEK_SET, NULL);
        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Failed to reset memory stream, hr = 0x%08X", hr));
        }
    }

    //
    // Read first the BOM bytes and validate:
    //
    if (S_OK == hr)
    {
        BYTE bBOM[2] = {};

        hr = pStream->Read(bBOM, 2, NULL);
        if (S_OK == hr)
        {
            if ((0xFF != bBOM[0]) || (0xFE != bBOM[1]))
            {
                hr = E_INVALIDARG;
                WIAEX_ERROR((g_hInst, "Invalid BOM. Expected 0xFF 0xFE, received 0x%X 0x%X, hr = 0x%08X", bBOM[0], bBOM[1], hr));
            }
        }
        else
        {
            WIAEX_ERROR((g_hInst, "Failed to read the BOM from the stream, hr = 0x%08X", hr));
            if (SUCCEEDED(hr))
            {
                hr = E_INVALIDARG;
            }
        }
    }

    //
    // Alocate memory for the NULL terminated WCHAR string. The NULL string terminator is not
    // expected in the stream, however the stream must contain the BOM "character" instead,
    // thus we need to allocate space for the same number of whole double byte characters
    // that are in the stream:
    //
    if (SUCCEEDED(hr))
    {
        ulNumChars = ulDataSize / sizeof(WCHAR);
        pwData = new WCHAR[ulNumChars];
        if (pwData)
        {
            //
            // NULL terminate the string:
            //
            pwData[ulNumChars - 1] = 0;
        }
        else
        {
            hr = E_OUTOFMEMORY;
            WIAEX_ERROR((g_hInst, "Out of memory when trying to allocate %u bytes, hr = 0x%08X", ulDataSize, hr));
        }
    }

    //
    // Read the actual imprinter/endorser characters:
    //
    if (S_OK == hr)
    {
        //
        // ulNumChars includes the length of the NULL terminator:
        //
        ulNumChars -= 1;

        hr = pStream->Read((PBYTE)pwData, ulNumChars * sizeof(WCHAR), NULL);
        if (S_OK != hr)
        {
            WIAEX_ERROR((g_hInst, "Failed to read from the stream, hr = 0x%08X", hr));
            if (SUCCEEDED(hr))
            {
                hr = E_INVALIDARG;
            }
        }
    }

    //
    // Validate each character against the set of valid characters for the source:
    //
    if (SUCCEEDED(hr))
    {
        PWCHAR szValidChars = (IMPRINTER == nDocumentHandlingSelect) ? SAMPLE_IMPRINTER_VALID_CHARS : SAMPLE_ENDORSER_VALID_CHARS;
        ULONG ulValidChars = ((IMPRINTER == nDocumentHandlingSelect) ? ARRAYSIZE(SAMPLE_IMPRINTER_VALID_CHARS) : ARRAYSIZE(SAMPLE_ENDORSER_VALID_CHARS)) - 1;
        BOOL bFound = FALSE;

        for (ULONG i = 0; i < ulNumChars; i++)
        {
            bFound = FALSE;

            for (ULONG j = 0; j < ulValidChars; j++)
            {
                if (pwData[i] == szValidChars[j])
                {
                    bFound = TRUE;
                    i++;
                    break;
                }
            }

            if (!bFound)
            {
                hr = E_INVALIDARG;
                WIAEX_ERROR((g_hInst, "Invalid character for WIA_IPS_PRINTER_ENDORSER_STRING: 0x%X (%wc), hr = 0x%08X", pwData[i], pwData[i], hr));
                break;
            }
        }
    }

    if (SUCCEEDED(hr))
    {
        bstrData = SysAllocString(pwData);
        if (!bstrData)
        {
            hr = E_OUTOFMEMORY;
            WIAEX_ERROR((g_hInst, "Out of memory when trying to allocate a BSTR of %u characters in length, hr = 0x%08X", ulNumChars, hr));
        }
    }

    if (SUCCEEDED(hr))
    {
        hr = wiasWritePropStr(pWiasContext, WIA_IPS_PRINTER_ENDORSER_STRING, bstrData);
        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Failed to set WIA_IPS_PRINTER_ENDORSER_STRING, hr = 0x%08X", hr));
        }
    }

    if (bstrData)
    {
        SysFreeString(bstrData);
    }

    if (pwData)
    {
        delete[] pwData;
    }

    WIAEX_TRACE_FUNC_HR;

    return hr;

}

/**************************************************************************\
*
* Helper for IWiaMiniDrv::drvAcquireItemData. Loads a test data file (image
* or metadata) from resources. A test image is expected to be in EXIF format
* and match the dimensions described by the MIN/MAX_TEST_SCAN_WIDTH/HEIGHT.
* The function does not attempt to validate the contents of data loaded from
* resources, it only copies the whole data file to the specified stream.
*
* Parameters:
*
*    ulResourceId - resource identifier
*    pStream      - stream destination for the test image
*    pulDataSize  - returns the size in bytes of the data loaded from
*                   resources and written to the destination stream
*
* Return Value:
*
*    S_OK if successful or an error HRESULT if an error occurrs
*
\**************************************************************************/

HRESULT
CWiaDriver::LoadTestDataResourceToStream(
          ULONG     ulResourceId,
    _In_  IStream  *pStream,
    _Out_ ULONG    *pulDataSize)
{
    HRESULT hr = S_OK;

    WIAEX_TRACE_BEGIN;

    if ((!pStream) && (!pulDataSize))
    {
        hr = E_INVALIDARG;
        WIAEX_ERROR((g_hInst, "Invalid parameter, hr = 0x%08X", hr));
    }

    if (SUCCEEDED(hr))
    {
        *pulDataSize = 0;

        HRSRC hResourceInfo = FindResource(g_hInst, MAKEINTRESOURCE(ulResourceId), RT_RCDATA);
        if (hResourceInfo)
        {
            ULONG ulWrittenData = 0;

            ULONG ulDataSize = (ULONG)SizeofResource(g_hInst, hResourceInfo);
            if (ulDataSize > 0)
            {
                HGLOBAL hTestData = LoadResource(g_hInst, hResourceInfo);
                if (hTestData)
                {
                    PBYTE pTestData = (PBYTE)LockResource(hTestData);
                    if (pTestData)
                    {
                        LARGE_INTEGER lZero = {};

                        hr = pStream->Seek(lZero, STREAM_SEEK_SET, NULL);
                        if (S_OK == hr)
                        {
                            hr = pStream->Write(pTestData, ulDataSize, &ulWrittenData);
                            if (S_OK == hr)
                            {
                                if  (ulWrittenData != ulDataSize)
                                {
                                    hr = E_FAIL;
                                    WIAEX_ERROR((g_hInst, "Failed to load data (%u) from resources, expected %u bytes, got %u bytes, hr = 0x%08X",
                                        ulResourceId, ulDataSize, ulWrittenData, hr));
                                }
                            }
                            else
                            {
                                WIAEX_ERROR((g_hInst, "Failed to load data (%u) from resources, hr = 0x%08X", ulResourceId, hr));
                            }

                            HRESULT hrTemp = pStream->Seek(lZero, STREAM_SEEK_SET, NULL);
                            if (S_OK != hrTemp)
                            {
                                if (S_OK == hr)
                                {
                                    hr = hrTemp;
                                }

                                WIAEX_ERROR((g_hInst, "Failed to reset memory stream, hr = 0x%08X", hrTemp));
                            }
                        }
                        else
                        {
                            WIAEX_ERROR((g_hInst, "Failed to reset memory stream, hr = 0x%08X", hr));
                        }

                    }
                    DeleteObject(hTestData);
                }

                if (S_OK == hr)
                {
                    *pulDataSize = ulDataSize;
                }
            }
            else
            {
                hr = HRESULT_FROM_WIN32(::GetLastError());
                WIAEX_ERROR((g_hInst, "Bad resource (%u), hr = 0x%08X", ulResourceId, hr));
            }
        }
        else
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());
            WIAEX_ERROR((g_hInst, "Cannot find resource (%u), hr = 0x%08X", ulResourceId, hr));
        }
    }

    WIAEX_TRACE_FUNC_HR;

    return hr;
}

/**************************************************************************\
*
* Helper for IWiaMiniDrv::drvAcquireItemData. Writes generated or hard-coded
* sample metadata to the specified download stream. This data is to be
* transferred from this sample driver to the WIA application client as an
* imprinter/endorser text/CSV file, or raw barcode/patch code/MICR metadata.
*
* Parameters:
*
*    pWiasContext     - item context
*    guidItemCategory - item category (WIA_CATEGORY_ value)
*    guidFormat       - trasfer file format (WiaImgFmt_ value)
*    pStream          - stream destination for the test image
*    pulDataSize      - returns the size in bytes of the data loaded from
*                       resources and written to the destination stream
*
* Return Value:
*
*    S_OK if successful or an error HRESULT if an error occurrs
*
\**************************************************************************/

HRESULT
CWiaDriver::LoadTestDataToStream(
    _In_  BYTE    *pWiasContext,
          GUID     guidItemCategory,
          GUID     guidFormat,
    _In_  IStream *pStream,
    _Out_ ULONG    *pulDataSize)
{
    HRESULT hr = S_OK;
    ULONG ulDataSize = 0;

    WIAEX_TRACE_BEGIN;

    if ((!pWiasContext) || (!pStream) && (!pulDataSize))
    {
        hr = E_INVALIDARG;
        WIAEX_ERROR((g_hInst, "Invalid parameter, hr = 0x%08X", hr));
    }

    if (SUCCEEDED(hr))
    {
        if ((IsEqualGUID(WIA_CATEGORY_IMPRINTER, guidItemCategory) || IsEqualGUID(WIA_CATEGORY_ENDORSER, guidItemCategory)) &&
            (!IsEqualGUID(WiaImgFmt_BMP, guidFormat)))
        {
            if (IsEqualGUID(WiaImgFmt_TXT, guidFormat) || IsEqualGUID(WiaImgFmt_CSV, guidFormat))
            {
                BSTR bstrData = NULL;

                //
                // Read the current WIA_IPS_PRINTER_ENDORSER_STRING character string value
                // and prepare to write it to the destination stream, for simplicity without
                // expanding any special formatting sequences. Note the NULL string terminator
                // is not to be written to the stream (which for this sample it will end up
                // as a TXT or CSV file):
                //
                hr = wiasReadPropStr(pWiasContext, WIA_IPS_PRINTER_ENDORSER_STRING, &bstrData, NULL, TRUE);
                if (SUCCEEDED(hr))
                {
                    hr = pStream->Write(g_bBOM, sizeof(g_bBOM), NULL);
                    if (SUCCEEDED(hr))
                    {
                        ulDataSize = (ULONG)(wcslen((PWCHAR)bstrData) * sizeof(WCHAR));

                        hr = pStream->Write((PBYTE)bstrData, ulDataSize, NULL);
                        if (FAILED(hr))
                        {
                            WIAEX_ERROR((g_hInst, "IStream::Write(data) failed, hr = 0x%08X", hr));
                        }
                    }
                    else
                    {
                        WIAEX_ERROR((g_hInst, "IStream::Write(prefix) failed, hr = 0x%08X", hr));
                    }
                }
                else
                {
                    WIAEX_ERROR((g_hInst, "Failed to read the current WIA_IPS_PRINTER_ENDORSER_STRING value, hr = 0x%08X", hr));
                }

                if (bstrData)
                {
                    SysFreeString(bstrData);
                }
            }
            else
            {
                hr = E_INVALIDARG;
            }
        }
        else if (IsEqualGUID(WIA_CATEGORY_BARCODE_READER, guidItemCategory) && IsEqualGUID(WiaImgFmt_RAWBAR, guidFormat))
        {
            WIA_BARCODES bc = {};
            WIA_BARCODE_INFO bi = {};

            //
            // 3 hard-coded sample barcodes to report. These sample barcodes match the XML sample metadata (Barcodes.xml):
            //

            WCHAR szBarcodeOne[] = L"036000291452";
            WCHAR szBarcodeTwo[] = L"3117013206375";
            WCHAR szBarcodeThree[] = L"This is a Full ASCII Code 39 example";

            //
            // When computing the data size take into account the following important details:
            //
            // WIA_BARCODES includes a WIA_BARCODE_INFO structure element as a placeholder.
            // WIA_BARCODE_INFO includes a WCHAR pleceholder for the text.
            // Make sure the size of these fields are not counted twice.
            //
            // Do not include the length of the NULL string terminators for the character sequences,
            // they and not NULL terminated.
            //
            // Do not compute the fixed size (excluding the text) of a WIA_BARCODE_INFO structure
            // doing sizeof(WIA_BARCODE_INFO) - sizeof(WCHAR), this will incorrectly add compiler
            // padding for the data structure (2 additional bytes by default). Instead compute
            // the length of the fixed WIA_BARCODE_INFO size as 8 * sizeof(DWORD).
            //
            ULONG ulFixedSize = 4 * sizeof(DWORD); //not: sizeof(WIA_BARCODES - sizeof(WIA_BARCODES_INFO)
            ULONG ulFixedInfoSize = 8 * sizeof(DWORD); //not: sizeof(WIA_BARCODES_INFO) - sizeof(WCHAR)
            ulDataSize = ulFixedSize + (3 * ulFixedInfoSize) + sizeof(szBarcodeOne) + sizeof(szBarcodeTwo) + sizeof(szBarcodeThree) - (3 * sizeof(WCHAR));

            const char szSignature[] = "WBAR";
            memcpy(&bc.Tag, szSignature, sizeof(DWORD));

            bc.Version = 0x00010000;
            bc.Size = ulDataSize;
            bc.Count = 3;

            hr = pStream->Write(&bc, ulFixedSize, NULL);
            if (SUCCEEDED(hr))
            {
                bi.Size = ulFixedInfoSize + sizeof(szBarcodeOne) - sizeof(WCHAR);
                bi.Type = 0;
                bi.Page = 0;
                bi.Confidence = 5;
                bi.XOffset = 0;
                bi.YOffset = 0;
                bi.Rotation = 90;
                bi.Length = (DWORD)wcslen(szBarcodeOne);

                hr = pStream->Write(&bi, ulFixedInfoSize, NULL);
                if (SUCCEEDED(hr))
                {
                    hr = pStream->Write(szBarcodeOne, sizeof(szBarcodeOne) - sizeof(WCHAR), NULL);
                }
            }

            if (SUCCEEDED(hr))
            {
                bi.Size = ulFixedInfoSize + sizeof(szBarcodeTwo) - sizeof(WCHAR);
                bi.Type = 2;
                bi.Page = 0;
                bi.Confidence = 9;
                bi.XOffset = 2;
                bi.YOffset = 1000;
                bi.Rotation = 0;
                bi.Length = (DWORD)wcslen(szBarcodeTwo);

                hr = pStream->Write(&bi, ulFixedInfoSize, NULL);
                if (SUCCEEDED(hr))
                {
                    hr = pStream->Write(szBarcodeTwo, sizeof(szBarcodeTwo) - sizeof(WCHAR), NULL);
                }
            }

            if (SUCCEEDED(hr))
            {
                bi.Size = ulFixedInfoSize + sizeof(szBarcodeThree) - sizeof(WCHAR);
                bi.Type = 7;
                bi.Page = 0;
                bi.Confidence = 10;
                bi.XOffset = 0;
                bi.YOffset = 2000;
                bi.Rotation = 0;
                bi.Length = (DWORD)wcslen(szBarcodeThree);

                hr = pStream->Write(&bi, ulFixedInfoSize, NULL);
                if (SUCCEEDED(hr))
                {
                    hr = pStream->Write(szBarcodeThree, sizeof(szBarcodeThree) - sizeof(WCHAR), NULL);
                }
            }
        }
        else if (IsEqualGUID(WIA_CATEGORY_PATCH_CODE_READER, guidItemCategory) && IsEqualGUID(WiaImgFmt_RAWPAT, guidFormat))
        {
            WIA_PATCH_CODES pc = {};
            WIA_PATCH_CODE_INFO pi = {};

            //
            // 2 hard-coded sample patch codes to report. These sample codes match the XML sample metadata (PatchCod.xml):
            //

            ULONG ulFixedSize = 4 * sizeof(DWORD);
            ULONG ulFixedInfoSize = sizeof(DWORD);
            ulDataSize = ulFixedSize + (2 * ulFixedInfoSize);

            const char szSignature[] = "WPAT";
            memcpy(&pc.Tag, szSignature, sizeof(DWORD));

            pc.Version = 0x00010000;
            pc.Size = ulDataSize;
            pc.Count = 2;

            hr = pStream->Write(&pc, ulFixedSize, NULL);
            if (SUCCEEDED(hr))
            {
                pi.Type = 2;

                hr = pStream->Write(&pi, ulFixedInfoSize, NULL);
            }

            if (SUCCEEDED(hr))
            {
                pi.Type = 1;

                hr = pStream->Write(&pi, ulFixedInfoSize, NULL);
            }
        }
        else if (IsEqualGUID(WIA_CATEGORY_MICR_READER, guidItemCategory) && IsEqualGUID(WiaImgFmt_RAWMIC, guidFormat))
        {
            WIA_MICR micr = {};
            WIA_MICR_INFO mi = {};

            //
            // 2 hard-coded sample MICR codes to report. These sample codes match the XML sample metadata (Micr.xml):
            //

            WCHAR szMicrOne[] = L"1234567890";
            WCHAR szMicrTwo[] = L"987?543?10";

            //
            // When computing the data size take into account the following important details:
            //
            // WIA_MICR includes a WIA_MICR_INFO structure element as a placeholder.
            // WIA_MICR_INFO includes a WCHAR pleceholder for the text.
            // Make sure the size of these fields are not counted twice.
            //
            // Do not include the length of the NULL string terminators for the character sequences,
            // they and not NULL terminated.
            //
            // Do not compute the fixed size (excluding the text) of a WIA_MICR_INFO structure
            // doing sizeof(WIA_MICR_INFO) - sizeof(WCHAR), this may add compiler padding for
            // the data structure. Instead compute the length of the fixed WIA_MICR_INFO size
            // as 3 * sizeof(DWORD).
            //
            ULONG ulFixedSize = 5 * sizeof(DWORD); //not: sizeof(WIA_MICR - sizeof(WIA_MICR_INFO)
            ULONG ulFixedInfoSize = 3 * sizeof(DWORD); //not: sizeof(WIA_MICR_INFO) - sizeof(WCHAR)
            ulDataSize = ulFixedSize + (2 * ulFixedInfoSize) + sizeof(szMicrOne) + sizeof(szMicrTwo) - (2 * sizeof(WCHAR));

            const char szSignature[] = "WMIC";
            memcpy(&micr.Tag, szSignature, sizeof(DWORD));

            micr.Version = 0x00010000;
            micr.Size = ulDataSize;
            micr.Count = 2;
            micr.Placeholder = L'?';

            hr = pStream->Write(&micr, ulFixedSize, NULL);
            if (SUCCEEDED(hr))
            {
                mi.Size = ulFixedInfoSize + sizeof(szMicrOne) - sizeof(WCHAR);
                mi.Page = 0;
                mi.Length = (DWORD)wcslen(szMicrOne);

                hr = pStream->Write(&mi, ulFixedInfoSize, NULL);
                if (SUCCEEDED(hr))
                {
                    hr = pStream->Write(szMicrOne, sizeof(szMicrOne) - sizeof(WCHAR), NULL);
                }
            }

            if (SUCCEEDED(hr))
            {
                mi.Size = ulFixedInfoSize + sizeof(szMicrTwo) - sizeof(WCHAR);
                mi.Page = 1;
                mi.Length = (DWORD)wcslen(szMicrTwo);

                hr = pStream->Write(&mi, ulFixedInfoSize, NULL);
                if (SUCCEEDED(hr))
                {
                    hr = pStream->Write(szMicrTwo, sizeof(szMicrTwo) - sizeof(WCHAR), NULL);
                }
            }
        }
        else
        {
            hr = E_INVALIDARG;
            WIAEX_ERROR((g_hInst, "Invalid item - transfer format combination, hr = 0x%08X", hr));
        }
    }

    if (SUCCEEDED(hr))
    {
        *pulDataSize = ulDataSize;
    }

    WIAEX_TRACE_FUNC_HR;

    return hr;
}

/**************************************************************************\
*
* Helper for TransferFile. Computes the transfer completion relative to the
* specified size of data written and the estimated target file size.
*
* Parameters:
*
*    pulPercentComplete      - percent complete value to be updated
*    ulEstimatedFileSize     - estimated file size, in bytes (0 if unknown)
*    ulFileBytesWritten      - number of bytes of data written
*    bDirectWiaTransfer      - TRUE if this is a full WIA transfer when the
*                              percent complete must be incremented from 0%
*                              to 99%, FALSE if this is a transfer of a file
*                              already read from the scanner when the percent
*                              complete is set from ulResumeFrom to 99% -or-
*                              if this is a transfer from scanner to the driver
*                              when the percent complete is set from 0% to 49%.
*    ulResumeFrom            - if bDirectWiaTransfer is FALSE this parameter
*                              indicates a start value between 0% and 49% for
*                              a transfer from the driver to the WIA app -or-
*                              0 to indicate a transfer from the scanner to driver
* Return Value:
*
*    None
*
\**************************************************************************/

void
CWiaDriver::ComputeTransferProgress(
    _Inout_ ULONG *pulPercentComplete,
            ULONG  ulEstimatedFileSize,
            ULONG  ulFileBytesWritten,
            BOOL   bDirectWIATransfer,
            ULONG  ulResumeFrom)
{
    ULONG ulStartPos = 0;
    double dMax = 100.0;
    ULONG ulLimit = 99;

    if (!bDirectWIATransfer)
    {
        if (!ulResumeFrom)
        {
            //
            // If this is an indirect (translated) transfer and the indicated
            // start position is 0 we will compute the transfer complete on a
            // 0% to 50% scale up to a maximum value of 49%:
            //
            ulStartPos = 0;
            dMax = 50.0;
            ulLimit = 49;

        }
        else
        {
            //
            // If a non-zero value is indicated to start the progress indicator
            // from we will resume incrementing the transfer complete from here:
            //
            if (ulResumeFrom < 50)
            {
                ulStartPos = ulResumeFrom + 1;
            }
            else
            {
                ulStartPos = 50;
            }

            dMax = 100.0 - (double)ulStartPos;
            ulLimit = 99 - ulStartPos;
        }
    }

    if (pulPercentComplete)
    {
        //
        // Note that we do not know in advance the exact size of the file to be transferred.
        // In order to allow the WIA client application to see activity (other than the
        // amount of data that is being transferred with each write) we will use the estimated
        // uncompressed data size (if available) or simply increment the percent complete until
        // 99 or end of file transfer (and then set it to 100%):
        //
        if (ulEstimatedFileSize > 0)
        {
            *pulPercentComplete = (ULONG)(dMax * (((double)ulFileBytesWritten) / ((double)ulEstimatedFileSize)));
            if ((*pulPercentComplete) > ulLimit)
            {
                *pulPercentComplete = ulLimit;
            }
        }
        else
        {
            if ((*pulPercentComplete) < ulLimit)
            {
                *pulPercentComplete += 1;
            }
        }

        if (!bDirectWIATransfer)
        {
            *pulPercentComplete += ulStartPos;
        }
    }
}
