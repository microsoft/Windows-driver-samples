/**************************************************************************
*
*  Copyright © Microsoft Corporation
*
*  File Name:   Validate.cpp
*
*  Description: This file contains code for WIA property validation
*               performed by the Production Scanning Driver Sample
*               for special categories of properties such as "format"
*               and "scan region/document size" properties.
*
***************************************************************************/

#include "stdafx.h"

/**************************************************************************\
*
* Validates new current values for the following dependent WIA properties:
*
* WIA_IPA_DATATYPE (*)
* WIA_IPS_CUR_INTENT (*)
* WIA_IPA_DEPTH (*)
* WIA_IPA_FORMAT
* WIA_IPA_TYMED (skipped since its value cannot be changed for this driver)
* WIA_IPA_COMPRESSION
*
* The current values for the following properties could be changed by the driver
* when one of the above mentioned properties is changed:
*
* WIA_IPA_DATATYPE (*)
* WIA_IPS_CUR_INTENT (*)
* WIA_IPA_DEPTH (*)
* WIA_IPA_CHANNELS_PER_PIXEL (*)
* WIA_IPA_BITS_PER_CHANNEL (*)
* WIA_IPA_FORMAT
* WIA_IPA_TYMED (skipped since its value cannot be changed for this driver)
* WIA_IPA_FILENAME_EXTENSION
* WIA_IPA_COMPRESSION
*
* (*) - These properties are available only on the the Flatbed and Feeder items
*
* Parameters:
*
*    pWiasContext            - pointer to the item context
*    pPropertyContext        - pointer to the property context which
*                              indicates which properties are being written
*    nDocumentHandlingSelect - FLAT or FEED (as defined in wiadef.h)
*
* Return Value:
*
*    S_OK if successful, an error HRESULT otherwise (E_INVALIDARG if
*    an invalid combination is attempted)
*
\**************************************************************************/

HRESULT CWiaDriver::ValidateFormatProperties(
    _In_ BYTE*                    pWiasContext,
    _In_ WIA_PROPERTY_CONTEXT*    pPropertyContext,
    UINT                          nDocumentHandlingSelect)
{
    HRESULT hr = S_OK;

    LONG lDataType = WIA_DATA_COLOR;
    LONG lIntent = WIA_INTENT_NONE;
    LONG lDepth = 24;
    LONG lChannelsPerPixel = 3;
    LONG lBitsPerChannel = 8;
    GUID guidFormat = WiaImgFmt_UNDEFINED;
    LONG lCompression = WIA_COMPRESSION_NONE;
    BSTR bstrFileExtension = NULL;
    BYTE bRawBitsPerChannel[3] = {};

    BOOL bDataTypeChanged = FALSE;
    BOOL bIntentChanged = FALSE;
    BOOL bImageTypeIntentChanged = FALSE;
    BOOL bDepthChanged = FALSE;
    BOOL bFormatChanged = FALSE;
    BOOL bCompressionChanged = FALSE;

    PROPSPEC ps[3] = {};
    ULONG nPropSpec = 0;

    if ((!pWiasContext) || (!pPropertyContext))
    {
        hr = E_INVALIDARG;
        WIAEX_ERROR((g_hInst, "Invalid parameters, hr = 0x%08X", hr));
    }

    if (SUCCEEDED(hr))
    {
        //
        // Check which color properties have been changed (ignore failures).
        // Note that the Auto item supports only format, tymed and compression:
        //
        if ((FLAT == nDocumentHandlingSelect) || (FEED == nDocumentHandlingSelect))
        {
            wiasIsPropChanged(WIA_IPA_DATATYPE, pPropertyContext, &bDataTypeChanged);
            wiasIsPropChanged(WIA_IPS_CUR_INTENT, pPropertyContext, &bIntentChanged);
            wiasIsPropChanged(WIA_IPA_DEPTH, pPropertyContext, &bDepthChanged);
        }
        wiasIsPropChanged(WIA_IPA_FORMAT, pPropertyContext, &bFormatChanged);
        wiasIsPropChanged(WIA_IPA_COMPRESSION, pPropertyContext, &bCompressionChanged);

        if ((FLAT == nDocumentHandlingSelect) || (FEED == nDocumentHandlingSelect))
        {
            //
            // Read the current WIA_IPS_CUR_INTENT value:
            //
            if (SUCCEEDED(hr))
            {
                hr = wiasReadPropLong(pWiasContext, WIA_IPS_CUR_INTENT, &lIntent, NULL, TRUE);
                if (FAILED(hr))
                {
                    WIAEX_ERROR((g_hInst, "Error reading current WIA_IPS_CUR_INTENT, hr = 0x%08X", hr));
                }
            }

            //
            // When the intent is changed check if an image type intent flag is set:
            //
            if (SUCCEEDED(hr))
            {
                bImageTypeIntentChanged = (BOOL)(bIntentChanged && (WIA_INTENT_IMAGE_TYPE_MASK & lIntent));
            }
        }
    }

    //
    // Read the other current property values (no matter if each respective property was changed or not):
    //

    if (SUCCEEDED(hr) && (bDataTypeChanged || bIntentChanged || bDepthChanged) &&
        ((FLAT == nDocumentHandlingSelect) || (FEED == nDocumentHandlingSelect)))
    {
        hr = wiasReadPropLong(pWiasContext, WIA_IPA_DATATYPE, &lDataType, NULL, TRUE);
        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Error reading current WIA_IPA_DATATYPE, hr = 0x%08X", hr));
        }

        if (SUCCEEDED(hr))
        {
            hr = wiasReadPropLong(pWiasContext, WIA_IPA_DEPTH, &lDepth, NULL, TRUE);
            if (FAILED(hr))
            {
                WIAEX_ERROR((g_hInst, "Error reading current WIA_IPA_DEPTH, hr = 0x%08X", hr));
            }
        }
    }

    if (SUCCEEDED(hr) && (bFormatChanged || bCompressionChanged))
    {
        hr = wiasReadPropLong(pWiasContext, WIA_IPA_COMPRESSION, &lCompression, NULL, TRUE);
        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Error reading current WIA_IPA_COMPRESSION, hr = 0x%08X", hr));
        }

        if (SUCCEEDED(hr))
        {
            hr = wiasReadPropGuid(pWiasContext, WIA_IPA_FORMAT, &guidFormat, NULL, TRUE);
            if (FAILED(hr))
            {
                WIAEX_ERROR((g_hInst, "Error reading current WIA_IPA_FORMAT, hr = 0x%08X", hr));
            }
        }
    }

    //
    // If the application changed the image type intent the driver must consider
    // that WIA_IPA_DATA_TYPE was changed to the value apropriate for the new
    // intent no matter if the application set WIA_IPA_DATA_TYPE and to what value:
    //
    if (SUCCEEDED(hr) && bImageTypeIntentChanged)
    {
        //
        // If multiple color intents are set at the same time consider
        // just one and give the highest priority to highest bitdepth:
        //
        if (WIA_INTENT_IMAGE_TYPE_COLOR & lIntent)
        {
            bDataTypeChanged = TRUE;
            lDataType = WIA_DATA_COLOR;
        }
        else if (WIA_INTENT_IMAGE_TYPE_GRAYSCALE & lIntent)
        {
            bDataTypeChanged = TRUE;
            lDataType = WIA_DATA_GRAYSCALE;
        }
    }

    //
    // Validate the new current values against the total supported values for
    // each of the changed properties with write access in this category:
    //
    // WIA_IPA_DATATYPE
    // WIA_IPS_CUR_INTENT
    // WIA_IPA_DEPTH
    // WIA_IPA_FORMAT
    // WIA_IPA_TYMED (skipped here since its value cannot be changed for this driver)
    // WIA_IPA_COMPRESSION
    //
    // This sample driver can validate color and format properties separately
    // since none of its color modes (WIA_IPA_DATATYPE and WIA_IPA_DEPTH
    // combinations) are dependent on format changes (WIA_IPA_FORMAT,
    // WIA_IPA_TYMED -ignored here- and WIA_IPA_COMPRESSION combinations).
    // If the driver would also support WIA_DATA_BW, WIA_COMPRESSION_G4
    // and WiaImgFmt_TIFF the driver would need to validate all the color
    // and format properties together (for example WIA_DATA_BW and
    // WIA_COMPRESSION_G4 do neither work with WiaImgFmt_EXIF).
    //
    if (SUCCEEDED(hr) && ((FLAT == nDocumentHandlingSelect) || (FEED == nDocumentHandlingSelect)) &&
        (bDataTypeChanged || bIntentChanged || bDepthChanged))
    {
        nPropSpec = 0;

        if (bDataTypeChanged)
        {
            ps[nPropSpec].ulKind = PRSPEC_PROPID;
            ps[nPropSpec].propid = WIA_IPA_DATATYPE;
            nPropSpec++;
        }

        if (bIntentChanged)
        {
            ps[nPropSpec].ulKind = PRSPEC_PROPID;
            ps[nPropSpec].propid = WIA_IPS_CUR_INTENT;
            nPropSpec++;
        }

        if (bDepthChanged)
        {
            ps[nPropSpec].ulKind = PRSPEC_PROPID;
            ps[nPropSpec].propid = WIA_IPA_DEPTH;
            nPropSpec++;
        }

        hr = wiasValidateItemProperties(pWiasContext, nPropSpec, ps);
        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Invalid color property value(s) requested, hr = 0x%08X", hr));
        }
    }

    if (SUCCEEDED(hr) && (bFormatChanged || bCompressionChanged))
    {
        nPropSpec = 0;

        if (bFormatChanged)
        {
            ps[nPropSpec].ulKind = PRSPEC_PROPID;
            ps[nPropSpec].propid = WIA_IPA_FORMAT;
            nPropSpec++;
        }

        if (bCompressionChanged)
        {
            ps[nPropSpec].ulKind = PRSPEC_PROPID;
            ps[nPropSpec].propid = WIA_IPA_COMPRESSION;
            nPropSpec++;
        }

        hr = wiasValidateItemProperties(pWiasContext, nPropSpec, ps);
        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Invalid format property value(s) requested, hr = 0x%08X", hr));
        }
    }

    //
    // Additional validation for WIA_IPA_DATATYPE and WIA_IPA_DEPTH:
    //
    if (SUCCEEDED(hr) && ((FLAT == nDocumentHandlingSelect) || (FEED == nDocumentHandlingSelect)) &&
        (bDataTypeChanged || bDepthChanged))
    {
        if (bDataTypeChanged && bDepthChanged)
        {
            if (((WIA_DATA_COLOR == lDataType) && (8 == lDepth)) ||
                ((WIA_DATA_GRAYSCALE == lDataType) && (24 == lDepth)))
            {
                hr = E_INVALIDARG;
                WIAEX_ERROR((g_hInst, "Unsupported data type (%u) - depth (%u) combination requested, hr = 0x%08X",
                    lDataType, lDepth, hr));
            }
        }
        else if (bDataTypeChanged && (!bDepthChanged))
        {
            if (WIA_DATA_COLOR == lDataType)
            {
                lDepth = 24;
            }
            else if (WIA_DATA_GRAYSCALE == lDataType)
            {
                lDepth = 8;
            }
            else if (WIA_DATA_AUTO == lDataType)
            {
                lDepth = WIA_DEPTH_AUTO;
            }
        }
        else if ((!bDataTypeChanged) && bDepthChanged)
        {
            if (8 == lDepth)
            {
                lDataType = WIA_DATA_GRAYSCALE;
            }
            else if (24 == lDepth)
            {
                lDataType = WIA_DATA_COLOR;
            }
            else if (WIA_DEPTH_AUTO == lDepth)
            {
                lDataType = WIA_DATA_AUTO;
            }
        }
    }

    //
    // Additional validation for WIA_IPA_FORMAT and WIA_IPA_COMPRESSION (skipped for the
    // sample non-image sources since those do not support compresssed data transfers):
    //
    if (SUCCEEDED(hr) && ((FLAT == nDocumentHandlingSelect) || (FEED == nDocumentHandlingSelect) ||
        (AUTO_SOURCE == nDocumentHandlingSelect)) && (bFormatChanged || bCompressionChanged))
    {
        if (bFormatChanged && (!bCompressionChanged))
        {
            //
            // If WIA_IPA_FORMAT if changed alone, update WIA_IPA_COMPRESSION to match:
            //
            if (IsEqualGUID(guidFormat, WiaImgFmt_EXIF))
            {
                lCompression = WIA_COMPRESSION_JPEG;
            }
            else if (IsEqualGUID(guidFormat, WiaImgFmt_BMP) || IsEqualGUID(guidFormat, WiaImgFmt_RAW))
            {
                lCompression = WIA_COMPRESSION_NONE;
            }
        }
        else if ((!bFormatChanged) && bCompressionChanged)
        {
            //
            // If WIA_IPA_COMPRESSION if changed alone, update WIA_IPA_FORMAT to match:
            //
            if ((WIA_COMPRESSION_JPEG == lCompression) || (WIA_COMPRESSION_AUTO == lCompression))
            {
                guidFormat = WiaImgFmt_EXIF;
            }
            else if (WIA_COMPRESSION_NONE == lCompression)
            {
                guidFormat = WiaImgFmt_BMP;
            }
        }
        else if (bFormatChanged && bCompressionChanged)
        {
            //
            // If both WIA_IPA_FORMAT and WIA_IPA_COMPRESSION are changed, verify that their values work together:
            //
            if (((WIA_COMPRESSION_NONE == lCompression) && IsEqualGUID(guidFormat, WiaImgFmt_EXIF)) ||
                (((WIA_COMPRESSION_JPEG == lCompression) || (WIA_COMPRESSION_AUTO == lCompression)) &&
                    (IsEqualGUID(guidFormat, WiaImgFmt_BMP) || IsEqualGUID(guidFormat, WiaImgFmt_RAW))))
            {
                hr = E_INVALIDARG;
                WIAEX_ERROR((g_hInst, "Unsupported file format - compression mode combination, hr = 0x%08X", hr));
            }
        }
    }

    //
    // Update current values:
    //

    if (SUCCEEDED(hr) && ((FLAT == nDocumentHandlingSelect) || (FEED == nDocumentHandlingSelect)) &&
        (bDataTypeChanged || bIntentChanged || bDepthChanged))
    {
        //
        // WIA_IPA_DATATYPE:
        //
        hr = wiasWritePropLong(pWiasContext, WIA_IPA_DATATYPE, lDataType);
        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Failed to update WIA_IPA_DATATYPE, hr = 0x%08X", hr));
        }
        else
        {
            wiasSetPropChanged(WIA_IPA_DATATYPE, pPropertyContext, TRUE);
        }

        //
        // WIA_IPS_CUR_INTENT
        //
        // If an image type intent is set we must make sure it matches the current WIA_IPA_DATATYPE.
        // Don't do anything if the application changes any of the other intent flags.
        //
        if (SUCCEEDED(hr) && (lIntent & WIA_INTENT_IMAGE_TYPE_MASK))
        {
            //
            // Reset all current image type intent flags.
            //
            lIntent &= ~ WIA_INTENT_IMAGE_TYPE_MASK;

            //
            // .. and add just the one apropriate with the current WIA_IPA_DATATYPE value:
            //

            switch (lDataType)
            {
                case WIA_DATA_COLOR:
                    lIntent |= WIA_INTENT_IMAGE_TYPE_COLOR;
                    break;

                case WIA_DATA_GRAYSCALE:
                    lIntent |= WIA_INTENT_IMAGE_TYPE_GRAYSCALE;
            }

            hr = wiasWritePropLong(pWiasContext, WIA_IPS_CUR_INTENT, lIntent);
            if (FAILED(hr))
            {
                WIAEX_ERROR((g_hInst, "Failed to update WIA_IPS_CUR_INTENT, hr = 0x%08X", hr));
            }
            else
            {
                wiasSetPropChanged(WIA_IPS_CUR_INTENT, pPropertyContext, TRUE);
            }
        }

        //
        // WIA_IPA_DEPTH:
        //
        if (SUCCEEDED(hr))
        {
            hr = wiasWritePropLong(pWiasContext, WIA_IPA_DEPTH, lDepth);
            if (FAILED(hr))
            {
                WIAEX_ERROR((g_hInst, "Failed to update WIA_IPA_DEPTH, hr = 0x%08X", hr));
            }
            else
            {
                wiasSetPropChanged(WIA_IPA_DEPTH, pPropertyContext, TRUE);
            }
        }

        //
        // WIA_IPA_CHANNELS_PER_PIXEL:
        //
        if (SUCCEEDED(hr))
        {
            lChannelsPerPixel = (24 == lDepth) ? 3 : 1;

            hr = wiasWritePropLong(pWiasContext, WIA_IPA_CHANNELS_PER_PIXEL, lChannelsPerPixel);
            if (FAILED(hr))
            {
                WIAEX_ERROR((g_hInst, "Failed to update WIA_IPA_CHANNELS_PER_PIXEL, hr = 0x%08X", hr));
            }
            else
            {
                wiasSetPropChanged(WIA_IPA_CHANNELS_PER_PIXEL, pPropertyContext, TRUE);
            }
        }

        //
        // WIA_IPA_BITS_PER_CHANNEL:
        //
        if (SUCCEEDED(hr))
        {
            lBitsPerChannel = 8;

            hr = wiasWritePropLong(pWiasContext, WIA_IPA_BITS_PER_CHANNEL, lBitsPerChannel);
            if (FAILED(hr))
            {
                WIAEX_ERROR((g_hInst, "Failed to update WIA_IPA_BITS_PER_CHANNEL, hr = 0x%08X", hr));
            }
            else
            {
                wiasSetPropChanged(WIA_IPA_BITS_PER_CHANNEL, pPropertyContext, TRUE);
            }
        }

        //
        // WIA_IPA_RAW_BITS_PER_CHANNEL
        //
        if (SUCCEEDED(hr))
        {
            for (int i = 0; i < lChannelsPerPixel; i++)
            {
                bRawBitsPerChannel[i] = 8;
            }

            hr = wiasWritePropBin(pWiasContext, WIA_IPA_RAW_BITS_PER_CHANNEL, lChannelsPerPixel, bRawBitsPerChannel);
            if (FAILED(hr))
            {
                WIAEX_ERROR((g_hInst, "Failed to update WIA_IPA_RAW_BITS_PER_CHANNEL, hr = 0x%08X", hr));
            }
            else
            {
                wiasSetPropChanged(WIA_IPA_RAW_BITS_PER_CHANNEL, pPropertyContext, TRUE);
            }
        }
    }

    if (SUCCEEDED(hr) && (bFormatChanged || bCompressionChanged))
    {
        //
        // WIA_IPA_COMPRESSION:
        //
        if (SUCCEEDED(hr))
        {
            hr = wiasWritePropLong(pWiasContext, WIA_IPA_COMPRESSION, lCompression);
            if (FAILED(hr))
            {
                WIAEX_ERROR((g_hInst, "Failed to update WIA_IPA_COMPRESSION, hr = 0x%08X", hr));
            }
            else
            {
                wiasSetPropChanged(WIA_IPA_COMPRESSION, pPropertyContext, TRUE);
            }
        }

        //
        // WIA_IPA_FORMAT:
        //
        if (SUCCEEDED(hr))
        {
            hr = wiasWritePropGuid(pWiasContext, WIA_IPA_FORMAT, guidFormat);
            if (FAILED(hr))
            {
                WIAEX_ERROR((g_hInst, "Failed to update WIA_IPA_FORMAT, hr = 0x%08X", hr));
            }
            else
            {
                wiasSetPropChanged(WIA_IPA_COMPRESSION, pPropertyContext, TRUE);
            }
        }

        //
        // WIA_IPA_FILENAME_EXTENSION:
        //

        if (SUCCEEDED(hr))
        {
            if (IsEqualGUID(guidFormat, WiaImgFmt_BMP))
            {
                bstrFileExtension = SysAllocString(FILE_EXT_BMP);
            }
            else if (IsEqualGUID(guidFormat, WiaImgFmt_EXIF))
            {
                bstrFileExtension = SysAllocString(FILE_EXT_JPG);
            }
            else if (IsEqualGUID(guidFormat, WiaImgFmt_RAW) || IsEqualGUID(guidFormat, WiaImgFmt_RAWBAR) ||
                IsEqualGUID(guidFormat, WiaImgFmt_RAWPAT) || IsEqualGUID(guidFormat, WiaImgFmt_RAWMIC))
            {
                bstrFileExtension = SysAllocString(FILE_EXT_RAW);
            }
            else if (IsEqualGUID(guidFormat, WiaImgFmt_CSV))
            {
                bstrFileExtension = SysAllocString(FILE_EXT_CSV);
            }
            else if (IsEqualGUID(guidFormat, WiaImgFmt_TXT))
            {
                bstrFileExtension = SysAllocString(FILE_EXT_TXT);
            }
            else if (IsEqualGUID(guidFormat, WiaImgFmt_XMLBAR) || IsEqualGUID(guidFormat, WiaImgFmt_XMLPAT) ||
                IsEqualGUID(guidFormat, WiaImgFmt_XMLMIC))
            {
                bstrFileExtension = SysAllocString(FILE_EXT_XML);
            }
            else
            {
                hr = E_FAIL;
                WIAEX_ERROR((g_hInst, "Unsupported file format, hr = 0x%08X", hr));
            }
        }

        if (SUCCEEDED(hr) && (!bstrFileExtension))
        {
            hr = E_OUTOFMEMORY;
            WIAEX_ERROR((g_hInst, "Unable to allocate memory for new file extension, hr = 0x%08X", hr));
        }

        if (SUCCEEDED(hr))
        {
            hr = wiasWritePropStr(pWiasContext, WIA_IPA_FILENAME_EXTENSION, bstrFileExtension);
            if (FAILED(hr))
            {
                WIAEX_ERROR((g_hInst, "Failed to update WIA_IPA_FILENAME_EXTENSION, hr = 0x%08X", hr));
            }
        }
    }

    if (bstrFileExtension)
    {
        SysFreeString(bstrFileExtension);
    }

    return hr;
}

/**************************************************************************\
*
* Helper for IWiaMinIDrv::drvValidateItemProperties.
*
* Updates the following "image information" properties:
*
* WIA_IPA_PIXELS_PER_LINE
* WIA_IPA_NUMBER_OF_LINES
* WIA_IPA_BYTES_PER_LINE
*
* Parameters:
*
*    pWiasContext            - pointer to the item context
*    pPropertyContext        - pointer to the property context which
*                              indicates which properties are being written
*    nDocumentHandlingSelect - FLAT or FEED (as defined in wiadef.h)
*
* Return Value:
*
*    S_OK if successful, an error HRESULT otherwise
*
\**************************************************************************/
HRESULT CWiaDriver::ValidateImageInfoProperties(
    _In_ BYTE*                    pWiasContext,
    _In_ WIA_PROPERTY_CONTEXT*    pPropertyContext,
    UINT                          nDocumentHandlingSelect)
{
    HRESULT hr = S_OK;
    LONG lXExtent = 0;
    LONG lYExtent = 0;
    LONG lPixelsPerLine = 0;
    LONG lNumberOfLines = 0;
    LONG lDepth = 0;
    LONG lBytesPerLine = 0;

    if ((!pWiasContext) || (!pPropertyContext) || (AUTO_SOURCE == nDocumentHandlingSelect))
    {
        hr = E_INVALIDARG;
        WIAEX_ERROR((g_hInst, "Invalid parameters, hr = 0x%08X", hr));
    }

    //
    // Read the current extent and bit depth values:
    //

    if (SUCCEEDED(hr))
    {
        hr = wiasReadPropLong(pWiasContext, WIA_IPA_DEPTH, &lDepth, NULL, TRUE);
        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Error reading current WIA_IPA_DEPTH, hr = 0x%08X", hr));
        }
    }

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

    //
    // WIA_IPA_PIXELS_PER_LINE - the image width, in pixels, for the final image
    // WIA_IPA_NUMBER_OF_LINES - the image length, in pixels, for the final image
    // WIA_IPA_BYTES_PER_LINE - line width in bytes matching WIA_IPA_PIXELS_PER_LINE and WIA_IPA_DEPTH
    //

    lPixelsPerLine = lXExtent;
    lNumberOfLines = lYExtent;
    lBytesPerLine = BytesPerLine(lPixelsPerLine, lDepth);

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

    return hr;
}

/**************************************************************************\
*
* Validates new current values for the following dependent WIA properties:
*
* WIA_IPS_PAGE_SIZE
* WIA_IPS_ORIENTATION
* WIA_IPS_PAGE_WIDTH
* WIA_IPS_PAGE_HEIGHT
* WIA_IPS_XPOS
* WIA_IPS_YPOS
* WIA_IPS_XEXTENT
* WIA_IPS_YEXTENT
* WIA_IPS_XRES
* WIA_IPS_YRES
* WIA_IPS_XSCALING
* WIA_IPS_YSCALING
* WIA_IPS_LONG_DOCUMENT
*
* If the application changes the document size and orientation the driver
* should update the current and valid origin and extent properties to match
* at the current resolution the current full document dimensions.
*
* Resolution changes should also cause updated origins and extents (valid and current).
*
* Orientation changes should result in updated valid page size lists and
* updated current and valid origin and extents values.
*
* The driver is going to consider the origins and extents to configure
* the scan area (crop region) and the current page size to configure
* the physical document size, if needed.
*
* Parameters:
*
*    pWiasContext            - pointer to the item context
*    pPropertyContext        - pointer to the property context which
*                              indicates which properties are being written
*    nDocumentHandlingSelect - FLAT or FEED (as defined in wiadef.h)
*
* Return Value:
*
*    S_OK if successful, an error HRESULT otherwise (E_INVALIDARG if
*    an invalid combination is attempted)
*
\**************************************************************************/

HRESULT CWiaDriver::ValidateRegionProperties(
    _In_ BYTE*                    pWiasContext,
    _In_ WIA_PROPERTY_CONTEXT*    pPropertyContext,
    UINT                          nDocumentHandlingSelect)
{
    HRESULT hr = S_OK;

    if ((!pWiasContext) || (!pPropertyContext) || (AUTO_SOURCE == nDocumentHandlingSelect))
    {
        hr = E_INVALIDARG;
        WIAEX_ERROR((g_hInst, "Invalid parameters, hr = 0x%08X", hr));
    }

    LONG lXRes = 0;
    LONG lYRes = 0;
    LONG lMinXExtent = (MIN_SCAN_AREA_WIDTH * OPTICAL_RESOLUTION) / 1000;
    LONG lMinYExtent = (MIN_SCAN_AREA_HEIGHT * OPTICAL_RESOLUTION) / 1000;
    LONG lMaxXExtent = (MAX_SCAN_AREA_WIDTH * OPTICAL_RESOLUTION) / 1000;
    LONG lMaxYExtent = (MAX_SCAN_AREA_HEIGHT * OPTICAL_RESOLUTION) / 1000;

    LONG lXPos = 0;
    LONG lYPos = 0;
    LONG lXExtent = lMaxXExtent;
    LONG lYExtent = lMaxYExtent;

    LONG lPageWidth = MAX_SCAN_AREA_WIDTH;
    LONG lPageHeight = MAX_SCAN_AREA_HEIGHT;
    LONG lPageSize = WIA_PAGE_LETTER;
    LONG lOrientation = PORTRAIT;
    LONG lMaxWidth = MAX_SCAN_AREA_WIDTH;
    LONG lMaxHeight = MAX_SCAN_AREA_HEIGHT;
    LONG lLongDocument = WIA_LONG_DOCUMENT_DISABLED;

    BOOL bPageSizeChanged = FALSE;
    BOOL bOrientationChanged = FALSE;
    BOOL bLongDocumentChanged = FALSE;

    //
    // Read the current property values (no matter if they were changed or not):
    //

    if (SUCCEEDED(hr))
    {
        hr = wiasReadPropLong(pWiasContext, WIA_IPS_XPOS, &lXPos, NULL, TRUE);
        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Error reading current WIA_IPS_XPOS, hr = 0x%08X", hr));
        }
    }

    if (SUCCEEDED(hr))
    {
        hr = wiasReadPropLong(pWiasContext, WIA_IPS_YPOS, &lYPos, NULL, TRUE);
        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Error reading current WIA_IPS_YPOS, hr = 0x%08X", hr));
        }
    }

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
        hr = wiasReadPropLong(pWiasContext, WIA_IPS_XRES, &lXRes, NULL, TRUE);
        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Error reading current WIA_IPS_XRES, hr = 0x%08X", hr));
        }
    }

    if (SUCCEEDED(hr))
    {
        hr = wiasReadPropLong(pWiasContext, WIA_IPS_YRES, &lYRes, NULL, TRUE);
        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Error reading current WIA_IPS_YRES, hr = 0x%08X", hr));
        }
    }

    //
    // In order to validate scan frame or resolution changes we must know
    // the maximum scan area size. For Flatbed this is the total bed size.
    // For Feeder this is the size of the currently selected document size.
    //
    if (SUCCEEDED(hr) &&(FEED == nDocumentHandlingSelect))
    {
        wiasIsPropChanged(WIA_IPS_PAGE_SIZE, pPropertyContext, &bPageSizeChanged);
        wiasIsPropChanged(WIA_IPS_ORIENTATION, pPropertyContext, &bOrientationChanged);
        wiasIsPropChanged(WIA_IPS_LONG_DOCUMENT, pPropertyContext, &bLongDocumentChanged);

        if (SUCCEEDED(hr))
        {
            hr = wiasReadPropLong(pWiasContext, WIA_IPS_ORIENTATION, &lOrientation, NULL, TRUE);
            if (FAILED(hr))
            {
                WIAEX_ERROR((g_hInst, "Error reading current WIA_IPS_ORIENTATION, hr = 0x%08X", hr));
            }
        }

        if (SUCCEEDED(hr))
        {
            hr = wiasReadPropLong(pWiasContext, WIA_IPS_PAGE_SIZE, &lPageSize, NULL, TRUE);
            if (FAILED(hr))
            {
                WIAEX_ERROR((g_hInst, "Error reading current WIA_IPS_PAGE_SIZE, hr = 0x%08X", hr));
            }
        }

        if (SUCCEEDED(hr))
        {
            hr = wiasReadPropLong(pWiasContext, WIA_IPS_LONG_DOCUMENT, &lLongDocument, NULL, TRUE);
            if (FAILED(hr))
            {
                WIAEX_ERROR((g_hInst, "Error reading current WIA_IPS_LONG_DOCUMENT, hr = 0x%08X", hr));
            }
        }

        //
        // When WIA_LONG_DOCUMENT_ENABLED is set, if the application is not doing this update,
        // the driver must update itself the current WIA_IPS_PAGE_SIZE property value to WIA_PAGE_AUTO.
        // If WIA_LONG_DOCUMENT_ENABLED is set  and the application changes WIA_IPS_PAGE_SIZE to
        // another value than WIA_PAGE_AUTO, the driver must self-update the WIA_IPS_LONG_DOCUMENT
        // property to WIA_LONG_DOCUMENT_DISABLED.
        //
        if (SUCCEEDED(hr) && (bPageSizeChanged || bLongDocumentChanged))
        {
            if ((WIA_PAGE_AUTO != lPageSize) && (WIA_LONG_DOCUMENT_ENABLED == lLongDocument))
            {
                if (bLongDocumentChanged && bPageSizeChanged)
                {
                    hr = E_INVALIDARG;
                    WIAEX_ERROR((g_hInst, "Invalid WIA_IPS_PAGE_SIZE value for WIA_LONG_DOCUMENT_ENABLED, hr = 0x%08X", hr));
                }
                else if (bLongDocumentChanged && (!bPageSizeChanged))
                {
                    lPageSize = WIA_PAGE_AUTO;
                    bPageSizeChanged = TRUE;
                }
                else if ((!bLongDocumentChanged) && bPageSizeChanged)
                {
                    lLongDocument = WIA_LONG_DOCUMENT_DISABLED;
                    bLongDocumentChanged = TRUE;
                }
            }
        }

        //
        // Validate the current WIA_IPS_PAGE_SIZE against the valid values
        // apropriate with the current WIA_IPS_ORIENTATION:
        //
        if (SUCCEEDED(hr) && (bPageSizeChanged || bOrientationChanged))
        {
            CBasicDynamicArray<LONG> &lPageSizesArray = (PORTRAIT == lOrientation) ? m_lPortraitSizesArray : m_lLandscapeSizesArray;

            if (-1 == lPageSizesArray.Find(lPageSize))
            {
                if (bPageSizeChanged)
                {
                    hr = E_INVALIDARG;
                    WIAEX_ERROR((g_hInst, "Invalid WIA_IPS_PAGE_SIZE value, hr = 0x%08X", hr));
                }
                else if (WIA_PAGE_AUTO != lPageSize)
                {
                    //
                    // This means that the application did not request the page size
                    // to be changed, just changed the orientation and the current
                    // page size is not supported in the new orientation so we should
                    // quietly select WIA_PAGE_CUSTOM:
                    //
                    lPageSize = WIA_PAGE_CUSTOM;
                    lPageWidth = lMaxWidth;
                    lPageHeight = lMaxHeight;
                    bPageSizeChanged = TRUE;
                }
            }
            else
            {
                //
                // For WIA_PAGE_CUSTOM the page dimensions are the maximum dimensions of the scan area,
                // same for WIA_PAGE_AUTO (real page dimensions being detected here only after the
                // document has been scanned and reflected in the final image layout and dimensions):
                //
                if ((WIA_PAGE_CUSTOM == lPageSize) || (WIA_PAGE_AUTO == lPageSize))
                {
                    lPageWidth = lMaxWidth;
                    lPageHeight = lMaxHeight;
                }
                else
                {
                    //
                    // The page sizes from the current array are guaranteed to fit in
                    // the total scan acquisition area limits, minimum and maximum:
                    //
                    hr = GetPageDimensions(lPageSize, (BOOL)(PORTRAIT == lOrientation), lPageWidth, lPageHeight);
                    if (FAILED(hr))
                    {
                        WIAEX_ERROR((g_hInst, "Unable to retrieve dimensions for the current document size, hr = 0x%08X", hr));
                    }
                }
            }

            //
            // For Feeder the actual maximum dimensions for the scan area
            // are dictated by the current document size selected:
            //
            if (SUCCEEDED(hr))
            {
                lMaxWidth = lPageWidth;
                lMaxHeight = lPageHeight;
            }
        }
    }

    //
    // Use wiasUpdateScanRect to validate the current extents and
    // resolutions and update all their dependent properties:
    //
    if (SUCCEEDED(hr))
    {
        hr = wiasUpdateScanRect(pWiasContext, pPropertyContext, lMaxWidth, lMaxHeight);
        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "wiasUpdateScanRect(max width = %u, max height = %u) failed, hr = 0x%08X", lMaxWidth, lMaxHeight, hr));
        }
    }

    //
    // For feeder special validation must be performed for WIA_IPS_ORIENTATION and WIA_IPS_PAGE_SIZE changes:
    //
    if (FEED == nDocumentHandlingSelect)
    {
        //
        // Update valid list of values for WIA_IPS_PAGE_SIZE according with the new set WIA_IPS_ORIENTATION:
        //
        if (SUCCEEDED(hr) && bOrientationChanged)
        {
            LONG lNumPageSizes = 0;
            CBasicDynamicArray<LONG> &lPageSizesArray = (PORTRAIT == lOrientation) ? m_lPortraitSizesArray : m_lLandscapeSizesArray;
            lNumPageSizes = lPageSizesArray.Size();

            hr = wiasSetValidListLong(pWiasContext, WIA_IPS_PAGE_SIZE, (ULONG)lNumPageSizes,
                lPageSize, (LONG *)lPageSizesArray.Array());
            if (FAILED(hr))
            {
                WIAEX_ERROR((g_hInst, "Failed to update valid WIA_IPS_PAGE_SIZE values, hr = 0x%08X", hr));
            }
        }

        //
        // If the current WIA_IPS_PAGE_SIZE is changed updated all the dependent properties:
        //
        if (SUCCEEDED(hr) && bPageSizeChanged)
        {
            //
            // Update current WIA_IPS_PAGE_SIZE value:
            //
            if (SUCCEEDED(hr))
            {
                hr = wiasWritePropLong(pWiasContext, WIA_IPS_PAGE_SIZE, lPageSize);
                if (FAILED(hr))
                {
                    WIAEX_ERROR((g_hInst, "Failed to update WIA_IPS_PAGE_SIZE, hr = 0x%08X", hr));
                }
            }

            //
            // Update current WIA_IPS_PAGE_WIDTH:
            //
            if (SUCCEEDED(hr))
            {
                hr = wiasWritePropLong(pWiasContext, WIA_IPS_PAGE_WIDTH, lPageWidth);
                if (FAILED(hr))
                {
                    WIAEX_ERROR((g_hInst, "Failed to update WIA_IPS_PAGE_WIDTH, hr = 0x%08X", hr));
                }
            }

            //
            // Update current WIA_IPS_LONG_DOCUMENT:
            //
            if (SUCCEEDED(hr))
            {
                hr = wiasWritePropLong(pWiasContext, WIA_IPS_LONG_DOCUMENT, lLongDocument);
                if (FAILED(hr))
                {
                    WIAEX_ERROR((g_hInst, "Failed to update WIA_IPS_PAGE_HEIGHT, hr = 0x%08X", hr));
                }
            }

            //
            // Update current WIA_IPS_PAGE_HEIGHT:
            //
            if (SUCCEEDED(hr))
            {
                hr = wiasWritePropLong(pWiasContext, WIA_IPS_PAGE_HEIGHT, lPageHeight);
                if (FAILED(hr))
                {
                    WIAEX_ERROR((g_hInst, "Failed to update WIA_IPS_PAGE_HEIGHT, hr = 0x%08X", hr));
                }
            }

            //
            // If the current page size is changed always update the current and valid
            // WIA_IPS_XPOS, WIA_IPS_YPOS, WIA_IPS_XEXTENT and WIA_IPS_YEXTENT to match
            // the entire area of the currently selected document size, overwriting any
            // direct change for any of these properties requested at the same time with
            // the change for WIA_IPS_ORIENTATION or WIA_IPS_PAGESIZE.
            //

            //
            // Read current WIA_IPS_XRES:
            //
            if (SUCCEEDED(hr))
            {
                hr = wiasReadPropLong(pWiasContext, WIA_IPS_XRES, &lXRes, NULL, TRUE);
                if (FAILED(hr))
                {
                    WIAEX_ERROR((g_hInst, "Error reading current WIA_IPS_XRES, hr = 0x%08X", hr));
                }
            }

            //
            // Read current WIA_IPS_YRES:
            //
            if (SUCCEEDED(hr))
            {
                hr = wiasReadPropLong(pWiasContext, WIA_IPS_YRES, &lYRes, NULL, TRUE);
                if (FAILED(hr))
                {
                    WIAEX_ERROR((g_hInst, "Error reading current WIA_IPS_YRES, hr = 0x%08X", hr));
                }
            }

            //
            // Compute the new extent limits:
            //
            if (SUCCEEDED(hr))
            {
                lMinXExtent = (MIN_SCAN_AREA_WIDTH * lXRes) / 1000;
                if (!lMinXExtent)
                {
                    lMinXExtent = 1;
                }
                lMinYExtent = (MIN_SCAN_AREA_HEIGHT * lYRes) / 1000;
                if (!lMinYExtent)
                {
                    lMinYExtent = 1;
                }
                lMaxXExtent = (lPageWidth * lXRes) / 1000;
                lMaxYExtent = (lPageHeight * lYRes) / 1000;

                if ((lMaxXExtent < 1) || (lMaxYExtent < 1) || (lMinXExtent > lMaxXExtent) || (lMinYExtent > lMaxYExtent))
                {
                    hr = E_FAIL;
                    WIAEX_ERROR((g_hInst, "Unable to update the extent limits to match the new document size, hr = 0x%08X", hr));
                }
            }

            if (SUCCEEDED(hr))
            {
                //
                // The new current scan region, covering the entire document size:
                //
                lXPos = 0;
                lYPos = 0;
                lXExtent = lMaxXExtent;
                lYExtent = lMaxYExtent;

                //
                // Set new valid values for WIA_IPS_XPOS:
                //
                hr = wiasSetValidRangeLong(pWiasContext, WIA_IPS_XPOS, 0, lXPos, lMaxXExtent - lMinXExtent, 1);
                if (FAILED(hr))
                {
                    WIAEX_ERROR((g_hInst, "Failed to update valid WIA_IPS_XPOS, hr = 0x%08X", hr));
                }

                //
                // Set new valid values for WIA_IPS_YPOS:
                //
                if (SUCCEEDED(hr))
                {
                    hr = wiasSetValidRangeLong(pWiasContext, WIA_IPS_YPOS, 0, lYPos, lMaxYExtent - lMinYExtent, 1);
                    if (FAILED(hr))
                    {
                        WIAEX_ERROR((g_hInst, "Failed to update valid WIA_IPS_YPOS, hr = 0x%08X", hr));
                    }
                }

                //
                // Set new valid values for WIA_IPS_XEXTENT:
                //
                if (SUCCEEDED(hr))
                {
                    hr = wiasSetValidRangeLong(pWiasContext, WIA_IPS_XEXTENT, lMinXExtent, lXExtent, lMaxXExtent - lXPos, 1);
                    if (FAILED(hr))
                    {
                        WIAEX_ERROR((g_hInst, "Failed to update valid WIA_IPS_XEXTENT, hr = 0x%08X", hr));
                    }
                }

                //
                // Set new valid values for WIA_IPS_XEXTENT:
                //
                if (SUCCEEDED(hr))
                {
                    hr = wiasSetValidRangeLong(pWiasContext, WIA_IPS_YEXTENT, lMinYExtent, lYExtent, lMaxYExtent - lYPos, 1);
                    if (FAILED(hr))
                    {
                        WIAEX_ERROR((g_hInst, "Failed to update valid WIA_IPS_YEXTENT, hr = 0x%08X", hr));
                    }
                }

                //
                // Set new current value for WIA_IPS_XPOS:
                //
                if (SUCCEEDED(hr))
                {
                    hr = wiasWritePropLong(pWiasContext, WIA_IPS_XPOS, lXPos);
                    if (FAILED(hr))
                    {
                        WIAEX_ERROR((g_hInst, "Failed to update WIA_IPS_XPOS, hr = 0x%08X", hr));
                    }
                }

                //
                // Set new current value for WIA_IPS_YPOS:
                //
                if (SUCCEEDED(hr))
                {
                    hr = wiasWritePropLong(pWiasContext, WIA_IPS_YPOS, lYPos);
                    if (FAILED(hr))
                    {
                        WIAEX_ERROR((g_hInst, "Failed to update WIA_IPS_YPOS, hr = 0x%08X", hr));
                    }
                }

                //
                // Set new current value for WIA_IPS_XEXTENT:
                //
                if (SUCCEEDED(hr))
                {
                    hr = wiasWritePropLong(pWiasContext, WIA_IPS_XEXTENT, lXExtent);
                    if (FAILED(hr))
                    {
                        WIAEX_ERROR((g_hInst, "Failed to update WIA_IPS_XEXTENT, hr = 0x%08X", hr));
                    }
                }

                //
                // Set new current value for WIA_IPS_YEXTENT:
                //
                if (SUCCEEDED(hr))
                {
                    hr = wiasWritePropLong(pWiasContext, WIA_IPS_YEXTENT, lYExtent);
                    if (FAILED(hr))
                    {
                        WIAEX_ERROR((g_hInst, "Failed to update WIA_IPS_YEXTENT, hr = 0x%08X", hr));
                    }
                }
            }
        }
    }

    return hr;
}

/**************************************************************************\
*
* Validates new current values for the following dependent WIA properties:
*
* WIA_IPS_DOCUMENT_HANDLING_SELECT: either DUPLEX or FRONT_ONLY can be set
*
* WIA_IPS_PAGES: the valid range and current value are updated to match
* the current WIA_IPS_DOCUMENT_HANDLING_SELECT
*
* Parameters:
*
*    pWiasContext            - pointer to the item context
*    pPropertyContext        - pointer to the property context which
*                              indicates which properties are being written
*
* Return Value:
*
*    S_OK if successful, an error HRESULT otherwise (E_INVALIDARG if
*    an invalid combination is attempted)
*
\**************************************************************************/

HRESULT CWiaDriver::ValidateFeedProperties(
    _In_ BYTE*                    pWiasContext,
    _In_ WIA_PROPERTY_CONTEXT*    pPropertyContext)
{
    HRESULT hr = S_OK;
    BOOL bHandlingSelectChanged = TRUE;
    LONG lFeederHandlingSelect = FRONT_ONLY;
    BOOL bPagesChanged = TRUE;
    LONG lPages = 1;
    LONG lMaxPages = 0x7FFFFFFF; //maximum value for a signed 32-bit integer
    LONG lMinPages = 0; //ALL_PAGES
    LONG lStepPages = 1;
    WIAS_CHANGED_VALUE_INFO wiasValInfo = {};

    if ((!pWiasContext) || (!pPropertyContext))
    {
        hr = E_INVALIDARG;
        WIAEX_ERROR((g_hInst, "Invalid parameters, hr = 0x%08X", hr));
    }

    //
    // Check if WIA_IPS_DOCUMENT_HANDLING_SELECT was changed:
    //
    if (SUCCEEDED(hr))
    {
        if (SUCCEEDED(wiasGetChangedValueLong(pWiasContext, pPropertyContext, TRUE, WIA_IPS_DOCUMENT_HANDLING_SELECT, &wiasValInfo)))
        {
            lFeederHandlingSelect = wiasValInfo.Current.lVal;
            bHandlingSelectChanged = wiasValInfo.bChanged;
        }
        else
        {
            bHandlingSelectChanged = FALSE;
        }
    }

    //
    // Check if WIA_IPS_PAGES was changed and read its current value:
    //
    if (SUCCEEDED(hr))
    {
        if (SUCCEEDED(wiasGetChangedValueLong(pWiasContext, pPropertyContext, TRUE, WIA_IPS_PAGES, &wiasValInfo)))
        {
            lPages = wiasValInfo.Current.lVal;
            bPagesChanged = wiasValInfo.bChanged;
        }
        else
        {
            bPagesChanged = FALSE;

            if (bHandlingSelectChanged)
            {
                hr = wiasReadPropLong(pWiasContext, WIA_IPS_PAGES, &lPages, NULL, TRUE);
                if (FAILED(hr))
                {
                    WIAEX_ERROR((g_hInst, "Error reading current WIA_IPS_PAGES on the Feeder item, hr = 0x%08X", hr));
                }
            }
        }
    }

    //
    // If WIA_IPS_DOCUMENT_HANDLING_SELECT is changed verify that only FRONT_ONLY
    // or DUPLEX is requested to be set, only one at a time:
    //
    if (SUCCEEDED(hr) && bHandlingSelectChanged)
    {
        if ((DUPLEX != lFeederHandlingSelect) && (FRONT_ONLY != lFeederHandlingSelect))
        {
            hr = E_INVALIDARG;
            WIAEX_ERROR((g_hInst,
                "WIA_IPS_DOCUMENT_HANDLING_SELECT validation failed, only FRONT_ONLY and DUPLEX are valid, only one at a time, hr = 0x%08X", hr));
        }
    }

    //
    // Read the current WIA_IPS_DOCUMENT_HANDLING_SELECT value if not changed and WIA_IPS_PAGES is changed:
    //
    if (SUCCEEDED(hr) && (!bHandlingSelectChanged) && bPagesChanged)
    {
        hr = wiasReadPropLong(pWiasContext, WIA_IPS_DOCUMENT_HANDLING_SELECT, &lFeederHandlingSelect, NULL, TRUE);
        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Error reading current WIA_IPS_DOCUMENT_HANDLING_SELECT on the Feeder item, hr = 0x%08X", hr));
        }
    }

    //
    // If WIA_IPS_DOCUMENT_HANDLING_SELECT is changed and WIA_IPS_PAGES is not, update the valid and,
    // if needed, current values for WIA_IPS_PAGES. Note that the step WIA_IPS_PAGES value must remain
    // 1 for duplex in order to allow a legacy WIA 1.0 application to indirectly disable duplex by
    // settings WIA_DPS_PAGES to 1:
    //
    if (SUCCEEDED(hr) && bHandlingSelectChanged && (!bPagesChanged))
    {
        if (DUPLEX == lFeederHandlingSelect)
        {
            lMaxPages = 0x7FFFFFFE; //maximum even value for a signed 32-bit integer
            lMinPages = 0; //ALL_PAGES
            lStepPages = 1; //not 2

            //
            // Round up the current WIA_IPS_PAGES value to the nearest even number:
            //
            if ((lPages > 0) && (lPages % 2))
            {
                if (lPages <= (lMaxPages - 1))
                {
                    lPages += 1;
                }
                else if (lPages >= (lMinPages + 2))
                {
                    lPages -= 1;
                }
                else
                {
                    lPages = 2;
                }
            }
        }
        else
        {
            lMaxPages = 0x7FFFFFFF; //maximum value for a signed 32-bit integer
            lMinPages = 0; //ALL_PAGES
            lStepPages = 1;
        }

        //
        // Update the range of valid WIA_IPS_PAGES values:
        //
        hr = wiasSetValidRangeLong(pWiasContext, WIA_IPS_PAGES, lMinPages, lPages, lMaxPages, lStepPages);
        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Failed to update valid WIA_IPS_PAGES, hr = 0x%08X", hr));
        }

        //
        // Update the current WIA_IPS_PAGES value:
        //
        if (SUCCEEDED(hr))
        {
            hr = wiasWritePropLong(pWiasContext, WIA_IPS_PAGES, lPages);
            if (FAILED(hr))
            {
                WIAEX_ERROR((g_hInst, "Failed to update current WIA_IPS_PAGES, hr = 0x%08X", hr));
            }
        }
    }

    //
    // If WIA_IPS_PAGES is changed and WIA_IPS_DOCUMENT_HANDLING_SELECT is not, check if
    // we need to disable duplex as result of a WIA_IPS_PAGES current value change to 1.
    // We'll leave duplex enabled if the application changes WIA_IPS_PAGE to another
    // odd value, the application being responsible in this case to decide itself if
    // these pages are to be scanned duplex (and the last side discarded) or simplex:
    //
    if (SUCCEEDED(hr) && (!bHandlingSelectChanged) && bPagesChanged)
    {
        if ((DUPLEX == lFeederHandlingSelect) && (1 == lPages))
        {
            lFeederHandlingSelect = FRONT_ONLY;

            hr = wiasWritePropLong(pWiasContext, WIA_IPS_DOCUMENT_HANDLING_SELECT, lFeederHandlingSelect);
            if (FAILED(hr))
            {
                WIAEX_ERROR((g_hInst, "Failed to update current WIA_IPS_DOCUMENT_HANDLING_SELECT, hr = 0x%08X", hr));
            }
        }
    }

    //
    // If both WIA_IPS_PAGES and WIA_IPS_DOCUMENT_HANDLING_SELECT are changed at the
    // same time, check if DUPLEX and an odd WIA_IPS_PAGES value are set. We'll allow
    // odd values other than 1 while DUPLEX is set but not 1:
    //
    if (SUCCEEDED(hr) && bHandlingSelectChanged && bPagesChanged)
    {
        if ((DUPLEX == lFeederHandlingSelect) && (1 == lPages))
        {
            hr = E_INVALIDARG;
            WIAEX_ERROR((g_hInst, "WIA_IPS_PAGES cannot be set to 1 while setting WIA_IPS_DOCUMENT_HADLING_SELECT to DUPLEX, hr = 0x%08X", hr));
        }
    }

    return hr;
}

/**************************************************************************\
*
* Helper for IWiaMinIDrv::drvValidateItemProperties.
*
* Executes additional validation for the imprinter/endorser specific properties.
* The only imprinter/enorser property that this sample driver needs to validate
* here is WIA_IPS_PRINTER_ENDORSER_STRING - the characters submitted by the
* application must match WIA_IPS_PRINTER_ENDORSER_VALID_CHARACTERS. Unsupported
* WIA_IPS_PRINTER_ENDORSER_VALID_FORMAT_SPECIFIERS are quietly ignored by the
* driver (pretended to be printed/endorsed as-is).
*
* Parameters:
*
*    pWiasContext            - pointer to the item context
*    pPropertyContext        - pointer to the property context which
*                              indicates which properties are being written
*    nDocumentHandlingSelect - IMPRINTER or ENDORSER (defined in wiadef.h)
*
* Return Value:
*
*    S_OK if successful, an error HRESULT otherwise
*
\**************************************************************************/
HRESULT CWiaDriver::ValidateImprinterEndorserProperties(
    _In_ BYTE*                    pWiasContext,
    _In_ WIA_PROPERTY_CONTEXT*    pPropertyContext,
    UINT                          nDocumentHandlingSelect)
{
    HRESULT hr = S_OK;
    BOOL bStringChanged = TRUE;
    BSTR bstrNewString = NULL;
    BSTR bstrOldString = NULL;
    WIAS_CHANGED_VALUE_INFO wiasValInfo = {};

    if ((!pWiasContext) || (!pPropertyContext) ||
        ((IMPRINTER != nDocumentHandlingSelect) && (ENDORSER != nDocumentHandlingSelect)))
    {
        hr = E_INVALIDARG;
        WIAEX_ERROR((g_hInst, "CWiaDriver::ValidateImprinterEndorserProperties failed, invalid parameter, hr = 0x%08X", hr));
    }

    //
    // Check if WIA_IPS_PRINTER_ENDORSER_STRING is changed by the application:
    //
    if (SUCCEEDED(hr))
    {
        if (SUCCEEDED(wiasGetChangedValueStr(pWiasContext, pPropertyContext, TRUE, WIA_IPS_PRINTER_ENDORSER_STRING, &wiasValInfo)))
        {
            //
            // wiasGetChangedValueStr allocates both Current and Old BSTRs, read them both
            // even if not using the Old value, we'll need to free both of them when done:
            //
            bstrNewString = wiasValInfo.Current.bstrVal;
            bstrOldString = wiasValInfo.Old.bstrVal;
            bStringChanged = wiasValInfo.bChanged;
        }
        else
        {
            bStringChanged = FALSE;
        }
    }

    //
    // If WIA_IPS_PRINTER_ENDORSER_STRING is changed, check if all characters are valid:
    //
    if (SUCCEEDED(hr) && bStringChanged && bstrNewString)
    {
        PWCHAR szValidChars = (IMPRINTER == nDocumentHandlingSelect) ? SAMPLE_IMPRINTER_VALID_CHARS : SAMPLE_ENDORSER_VALID_CHARS;
        ULONG ulValidChars = ((IMPRINTER == nDocumentHandlingSelect) ? ARRAYSIZE(SAMPLE_IMPRINTER_VALID_CHARS) : ARRAYSIZE(SAMPLE_ENDORSER_VALID_CHARS)) - 1;
        ULONG i = 0;
        BOOL bFound = FALSE;

        while (bstrNewString[i] != NULL)
        {
            bFound = FALSE;

            for (ULONG j = 0; j < ulValidChars; j++)
            {
                if (bstrNewString[i] == szValidChars[j])
                {
                    bFound = TRUE;
                    i++;
                    break;
                }
            }

            if (!bFound)
            {
                hr = E_INVALIDARG;
                WIAEX_ERROR((g_hInst, "Invalid character for WIA_IPS_PRINTER_ENDORSER_STRING: 0x%X (%wc), hr = 0x%08X",
                    bstrNewString[i], bstrNewString[i], hr));
                break;
            }
        }
    }

    //
    // This sample driver supports only one line of text for its Imprinter/Endorser.
    // Check that the new WIA_IPS_PRINTER_ENDORSER_STRING value does not contain any
    // special '$N$'sequences ('new line'). This sample driver will ignore other
    // formatting sequences that may be contained by the new string:
    //
    if (SUCCEEDED(hr) && bStringChanged && bstrNewString)
    {
        WCHAR szNewLine[] = L"$N$";

        if (wcsstr(bstrNewString, szNewLine))
        {
            hr = E_INVALIDARG;
            WIAEX_ERROR((g_hInst, "Invalid new line ($N$) format sequence for WIA_IPS_PRINTER_ENDORSER_STRING, only one line of text supported, submitted value: %ws, hr = 0x%08X",
                bstrNewString, hr));
        }
    }

    //
    // Important: both the WIAS_CHANGED_VALUE_INFO::Current.bstrVal and
    // WIAS_CHANGED_VALUE_INFO::Old.bstrVal BSTRs must be freed.
    //
    if (bstrNewString)
    {
        SysFreeString(bstrNewString);
    }
    if (bstrOldString)
    {
        SysFreeString(bstrOldString);
    }

    return hr;
}

/**************************************************************************\
*
* Helper for IWiaMinIDrv::drvValidateItemProperties.
*
* Validates WIA_IPS_ENABLED_BARCODE_TYPES
*
* Parameters:
*
*    pWiasContext            - pointer to the item context
*    pPropertyContext        - pointer to the property context which
*                              indicates which properties are being written
*
* Return Value:
*
*    S_OK if successful, an error HRESULT otherwise
*
\**************************************************************************/
HRESULT CWiaDriver::ValidateBarcodeReaderProperties(
    _In_ BYTE*                    pWiasContext,
    _In_ WIA_PROPERTY_CONTEXT*    pPropertyContext)
{
    HRESULT hr = S_OK;
    BOOL bBarcodeTypesChanged = TRUE;

    if ((!pWiasContext) || (!pPropertyContext))
    {
        hr = E_INVALIDARG;
        WIAEX_ERROR((g_hInst, "CWiaDriver::ValidateImprinterEndorserProperties failed, invalid parameter, hr = 0x%08X", hr));
    }

    //
    // Check if WIA_IPS_ENABLED_BARCODE_TYPES is changed by the application:
    //
    if (SUCCEEDED(hr))
    {
        //
        // wiasIsPropChanged fails if the property is not changed, do not fail ValidateBarcodeReaderProperties:
        //
        wiasIsPropChanged(WIA_IPS_ENABLED_BARCODE_TYPES, pPropertyContext, &bBarcodeTypesChanged);
    }

    //
    // If WIA_IPS_ENABLED_BARCODE_TYPES is changed, read the new (vector) array of values and validate against the valid values:
    //
    if (SUCCEEDED(hr) && bBarcodeTypesChanged)
    {
        PROPSPEC ps = {};
        PROPVARIANT pv = {};

        ps.ulKind = PRSPEC_PROPID;
        ps.propid = WIA_IPS_ENABLED_BARCODE_TYPES;

        PropVariantInit(&pv);

        hr = wiasReadMultiple(pWiasContext, 1, &ps, &pv, NULL);
        if (SUCCEEDED(hr))
        {
            if ((VT_VECTOR | VT_I4) == pv.vt)
            {
                ULONG ulValidValues = ARRAYSIZE(g_lSupportedBarcodeTypes);
                BOOL bFound = FALSE;

                for (ULONG i = 0; i < pv.cal.cElems; i++)
                {
                    bFound = FALSE;

                    for (ULONG j = 0; j < ulValidValues; j++)
                    {
                        if (g_lSupportedBarcodeTypes[j] == pv.cal.pElems[i])
                        {
                            bFound = TRUE;
                            break;
                        }
                    }

                    if (!bFound)
                    {
                        hr = E_INVALIDARG;
                        WIAEX_ERROR((g_hInst, "Unsupported barcode type: %u, hr = 0x%08X", pv.cal.pElems[i], hr));
                        break;
                    }
                }

            }
            else
            {
                hr = E_INVALIDARG;
                WIAEX_ERROR((g_hInst, "Invalid value type for WIA_IPS_ENABLED_BARCODE_TYPES, expected VT_VECTOR | VT_I4 (%u), got %u, hr = 0x%08X",
                    VT_VECTOR | VT_I4, pv.vt, hr));
            }
        }
        else
        {
            WIAEX_ERROR((g_hInst, "wiasReadMultiple(WIA_IPS_ENABLED_BARCODE_TYPES) failed, hr = 0x%08X", hr));
        }

        PropVariantClear(&pv);
    }

    return hr;
}

/**************************************************************************\
*
* Helper for IWiaMinIDrv::drvValidateItemProperties.
*
* Validates WIA_IPS_ENABLED_PATCH_CODE_TYPES
*
* Parameters:
*
*    pWiasContext            - pointer to the item context
*    pPropertyContext        - pointer to the property context which
*                              indicates which properties are being written
*
* Return Value:
*
*    S_OK if successful, an error HRESULT otherwise
*
\**************************************************************************/
HRESULT CWiaDriver::ValidatePatchCodeReaderProperties(
    _In_ BYTE*                    pWiasContext,
    _In_ WIA_PROPERTY_CONTEXT*    pPropertyContext)
{
    HRESULT hr = S_OK;
    BOOL bPatchCodeTypesChanged = TRUE;

    if ((!pWiasContext) || (!pPropertyContext))
    {
        hr = E_INVALIDARG;
        WIAEX_ERROR((g_hInst, "CWiaDriver::ValidateImprinterEndorserProperties failed, invalid parameter, hr = 0x%08X", hr));
    }

    //
    // Check if WIA_IPS_ENABLED_PATCH_CODE_TYPES is changed by the application:
    //
    if (SUCCEEDED(hr))
    {
        //
        // wiasIsPropChanged fails if the property is not changed, do not fail ValidatePatchCodeReaderProperties:
        //
        wiasIsPropChanged(WIA_IPS_ENABLED_PATCH_CODE_TYPES, pPropertyContext, &bPatchCodeTypesChanged);
    }

    //
    // If WIA_IPS_ENABLED_PATCH_CODE_TYPES is changed, read the new (vector) array of values and validate against the valid values:
    //
    if (SUCCEEDED(hr) && bPatchCodeTypesChanged)
    {
        PROPSPEC ps = {};
        PROPVARIANT pv = {};

        ps.ulKind = PRSPEC_PROPID;
        ps.propid = WIA_IPS_ENABLED_PATCH_CODE_TYPES;

        PropVariantInit(&pv);

        hr = wiasReadMultiple(pWiasContext, 1, &ps, &pv, NULL);
        if (SUCCEEDED(hr))
        {
            if ((VT_VECTOR | VT_I4) == pv.vt)
            {
                ULONG ulValidValues = ARRAYSIZE(g_lSupportedPatchCodeTypes);
                BOOL bFound = FALSE;

                for (ULONG i = 0; i < pv.cal.cElems; i++)
                {
                    bFound = FALSE;

                    for (ULONG j = 0; j < ulValidValues; j++)
                    {
                        if (g_lSupportedPatchCodeTypes[j] == pv.cal.pElems[i])
                        {
                            bFound = TRUE;
                            break;
                        }
                    }

                    if (!bFound)
                    {
                        hr = E_INVALIDARG;
                        WIAEX_ERROR((g_hInst, "Unsupported patch code type: %u, hr = 0x%08X", pv.cal.pElems[i], hr));
                        break;
                    }
                }

            }
            else
            {
                hr = E_INVALIDARG;
                WIAEX_ERROR((g_hInst, "Invalid value type for WIA_IPS_ENABLED_PATCH_CODE_TYPES, expected VT_VECTOR | VT_I4 (%u), got %u, hr = 0x%08X",
                    VT_VECTOR | VT_I4, pv.vt, hr));
            }
        }
        else
        {
            WIAEX_ERROR((g_hInst, "wiasReadMultiple(WIA_IPS_ENABLED_PATCH_CODE_TYPES) failed, hr = 0x%08X", hr));
        }

        PropVariantClear(&pv);
    }

    return hr;
}

/**************************************************************************\
*
* Helper for IWiaMinIDrv::drvValidateItemProperties.
*
* Parameters:
*
*    pWiasContext            - pointer to the item context
*    pPropertyContext        - pointer to the property context which
*                              indicates which properties are being written
*
* Return Value:
*
*    S_OK if successful, an error HRESULT otherwise
*
\**************************************************************************/
HRESULT CWiaDriver::ValidateMicrReaderProperties(
    _In_ BYTE*                    pWiasContext,
    _In_ WIA_PROPERTY_CONTEXT*    pPropertyContext)
{
    HRESULT hr = S_OK;

    if ((!pWiasContext) || (!pPropertyContext))
    {
        hr = E_INVALIDARG;
        WIAEX_ERROR((g_hInst, "CWiaDriver::ValidateMicrReaderProperties failed, invalid parameter, hr = 0x%08X", hr));
    }

    //
    // Nothing special to validate here
    //

    return hr;
}

/**************************************************************************\
*
* Helper for CWiaDriver::ValidateColorDropProperties.
*
* Validates a RGB color drop property WIA_IPS_COLOR_DROP_RED, WIA_IPS_COLOR_DROP_GREEN and
* WIA_IPS_COLOR_DROP_BLUE
*
* Parameters:
*
*    pWiasContext            - pointer to the item context
*    pPropertyContext        - pointer to the property context which
*                              indicates which properties are being written
*    nChannel                - WIA_COLOR_DROP_RED, WIA_COLOR_DROP_GREEN or
*                              WIA_COLOR_DROP_BLUE (as defined in wiadef.h)
*
* Return Value:
*
*    S_OK if successful, an error HRESULT otherwise
*
\**************************************************************************/
HRESULT CWiaDriver::ValidateColorDropProperty(
    _In_ BYTE*                     pWiasContext,
    _In_ WIA_PROPERTY_CONTEXT*     pPropertyContext,
    UINT                           nChannel)
{
    HRESULT hr = S_OK;
    BOOL bChanged = TRUE;
    UINT nProp = 0;

    if ((!pWiasContext) || (!pPropertyContext) ||
        ((WIA_COLOR_DROP_RED != nChannel) && (WIA_COLOR_DROP_GREEN != nChannel) && (WIA_COLOR_DROP_BLUE != nChannel)))
    {
        hr = E_INVALIDARG;
        WIAEX_ERROR((g_hInst, "CWiaDriver::ValidateColorDropProperty failed, invalid parameter, hr = 0x%08X", hr));
    }

    switch (nChannel)
    {
        case WIA_COLOR_DROP_RED:
            nProp = WIA_IPS_COLOR_DROP_RED;
            break;

        case WIA_COLOR_DROP_GREEN:
            nProp = WIA_IPS_COLOR_DROP_GREEN;
            break;

        case WIA_COLOR_DROP_BLUE:
            nProp = WIA_IPS_COLOR_DROP_BLUE;
    }

    //
    // Check if this WIA_IPS_COLOR_DROP_* property is changed by the application:
    //
    if (SUCCEEDED(hr))
    {
        //
        // wiasIsPropChanged fails if the property is not changed, do not fail validation because of this:
        //
        wiasIsPropChanged(nProp, pPropertyContext, &bChanged);
    }

    //
    // If this WIA_IPS_COLOR_DROP_* property is changed, read the new (vector) array of values and validate
    // both against WIA_IPS_COLOR_DROP_MULTI and the valid range of values (from 0 and 100, inclusive):
    //
    if (SUCCEEDED(hr) && bChanged)
    {
        PROPSPEC ps = {};
        PROPVARIANT pv = {};

        ps.ulKind = PRSPEC_PROPID;
        ps.propid = nProp;

        PropVariantInit(&pv);

        hr = wiasReadMultiple(pWiasContext, 1, &ps, &pv, NULL);
        if (SUCCEEDED(hr))
        {
            if ((VT_VECTOR | VT_I4) == pv.vt)
            {
                if (pv.cal.cElems > g_lMaxDropColors)
                {
                    hr = E_INVALIDARG;
                    WIAEX_ERROR((g_hInst, "Unsupported number of drop out entries for property %u: %u, supported up to: %u, hr = 0x%08X",
                        nProp, pv.cal.cElems, g_lMaxDropColors, hr));
                }
                else
                {
                    for (ULONG i = 0; i < pv.cal.cElems; i++)
                    {
                        if ((pv.cal.pElems[i] < 0) || (pv.cal.pElems[i] > 100))
                        {
                            hr = E_INVALIDARG;
                            WIAEX_ERROR((g_hInst,
                                "Unsupported color drop value for property %u at vector position %u: %u, valid range is from 0 to 100 inclusive, hr = 0x%08X",
                                nProp, i, pv.cal.pElems[i], hr));
                            break;
                        }
                     }
                }
            }
            else
            {
                hr = E_INVALIDARG;
                WIAEX_ERROR((g_hInst, "Invalid value type for property %u, expected VT_VECTOR | VT_I4 (%u), got %u, hr = 0x%08X",
                    nProp, VT_VECTOR | VT_I4, pv.vt, hr));
            }
        }
        else
        {
            WIAEX_ERROR((g_hInst, "wiasReadMultiple(property %u) failed, hr = 0x%08X", nProp, hr));
        }

        PropVariantClear(&pv);
    }

    return hr;
}

/**************************************************************************\
*
* Helper for IWiaMinIDrv::drvValidateItemProperties.
*
* Validates WIA_IPS_COLOR_DROP_RED, WIA_IPS_COLOR_DROP_GREEN and
* WIA_IPS_COLOR_DROP_BLUE
*
* Parameters:
*
*    pWiasContext            - pointer to the item context
*    pPropertyContext        - pointer to the property context which
*                              indicates which properties are being written
*    nDocumentHandlingSelect - FLAT or FEED (as defined in wiadef.h)
*
* Return Value:
*
*    S_OK if successful, an error HRESULT otherwise
*
\**************************************************************************/
HRESULT CWiaDriver::ValidateColorDropProperties(
        _In_ BYTE*                 pWiasContext,
        _In_ WIA_PROPERTY_CONTEXT* pPropertyContext,
        UINT                       nDocumentHandlingSelect)
{
    HRESULT hr = S_OK;

    if ((!pWiasContext) || (!pPropertyContext) ||
        ((FLAT != nDocumentHandlingSelect) && (FEED != nDocumentHandlingSelect)))
    {
        hr = E_INVALIDARG;
        WIAEX_ERROR((g_hInst, "CWiaDriver::ValidateColorDropProperties failed, invalid parameter, hr = 0x%08X", hr));
    }

    //
    // Validate WIA_IPS_COLOR_DROP_RED (error logging is covered by ValidateColorDropProperty):
    //
    if (SUCCEEDED(hr))
    {
        hr = ValidateColorDropProperty(pWiasContext, pPropertyContext, WIA_COLOR_DROP_RED);
    }

    //
    // Validate WIA_IPS_COLOR_DROP_GREEN (error logging is covered by ValidateColorDropProperty):
    //
    if (SUCCEEDED(hr))
    {
        hr = ValidateColorDropProperty(pWiasContext, pPropertyContext, WIA_COLOR_DROP_GREEN);
    }

    //
    // Validate WIA_IPS_COLOR_DROP_BLUE (error logging is covered by ValidateColorDropProperty):
    //
    if (SUCCEEDED(hr))
    {
        hr = ValidateColorDropProperty(pWiasContext, pPropertyContext, WIA_COLOR_DROP_BLUE);
    }

    return hr;
}
