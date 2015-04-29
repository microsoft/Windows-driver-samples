/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   uiproperties.cpp

Abstract:

   This class encapsulates all handling of GPD and OEM private devmode settings
   used in the XPSDrv feature sample UI.

--*/

#include "precomp.h"
#include "debug.h"
#include "globals.h"
#include "xdstring.h"
#include "xdexcept.h"
#include "uiproperties.h"
#include "privatedefs.h"

PCSTR g_pszPSScaleWidth   = "PageScalingScaleWidth";
PCSTR g_pszPSScaleHeight  = "PageScalingScaleHeight";
PCSTR g_pszOffsetWidth    = "PageScalingOffsetWidth";
PCSTR g_pszOffsetHeight   = "PageScalingOffsetHeight";
PCSTR g_pszWMTransparency = "PageWatermarkTransparency";
PCSTR g_pszWMAngle        = "PageWatermarkTextAngle";
PCSTR g_pszWMOffsetWidth  = "PageWatermarkOriginWidth";
PCSTR g_pszWMOffsetHeight = "PageWatermarkOriginHeight";
PCSTR g_pszWMSizeWidth    = "PageWatermarkSizeWidth";
PCSTR g_pszWMSizeHeight   = "PageWatermarkSizeHeight";
PCSTR g_pszWMFontSize     = "PageWatermarkTextFontSize";
PCSTR g_pszWMFontColor    = "PageWatermarkTextColor";
PCSTR g_pszWMText         = "PageWatermarkTextText";

/*++

Routine Name:

    CUIProperties::CUIProperties

Routine Description:

    CUIProperties default class constructor.

Arguments:

    None.

Return Value:

    None
    Throws CXDException(HRESULT) on an error

--*/
CUIProperties::CUIProperties():
        m_pOEMDev(NULL)
{
    HRESULT hr = InitialiseMap();

    if (FAILED(hr))
    {
        throw CXDException(hr);
    }
}

/*++

Routine Name:

    CUIProperties::CUIProperties

Routine Description:

    CUIProperties class constructor.

Arguments:

    pOEMDM - Pointer to an OEMDEV structure.

Return Value:

    None
    Throws CXDException(HRESULT) on an error

--*/
CUIProperties::CUIProperties(
    POEMDEV pOEMDM):
    m_pOEMDev(pOEMDM)
{
    HRESULT hr = InitialiseMap();

    if (FAILED(hr))
    {
        throw CXDException(hr);
    }
}

/*++

Routine Name:

    CUIProperties::~CUIProperties

Routine Description:

    CUIProperties class destructor.

Arguments:

    None

Return Value:

    None

--*/
CUIProperties::~CUIProperties()
{
}

/*++

Routine Name:

    CUIProperties::InitialiseMap

Routine Description:

    Creates a map of GPD names and private OEM devmode properties that will be used by the driver.

Arguments:

    None

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CUIProperties::InitialiseMap(
    VOID
    )
{
    HRESULT hr = S_OK;

    try
    {
        //
        // Initialise the OEM devmode entries
        //
        m_PropertyMap[g_pszPSScaleWidth]   = UIPropertyPair(sizeof(DWORD), offsetof(OEMDEV, dwPgScaleX));
        m_PropertyMap[g_pszPSScaleHeight]  = UIPropertyPair(sizeof(DWORD), offsetof(OEMDEV, dwPgScaleY));
        m_PropertyMap[g_pszOffsetWidth]    = UIPropertyPair(sizeof(INT),   offsetof(OEMDEV, iPgOffsetX));
        m_PropertyMap[g_pszOffsetHeight]   = UIPropertyPair(sizeof(INT),   offsetof(OEMDEV, iPgOffsetY));
        m_PropertyMap[g_pszWMTransparency] = UIPropertyPair(sizeof(INT),   offsetof(OEMDEV, iWMTransparency));
        m_PropertyMap[g_pszWMAngle]        = UIPropertyPair(sizeof(INT),   offsetof(OEMDEV, iWMAngle));
        m_PropertyMap[g_pszWMFontSize]     = UIPropertyPair(sizeof(INT),   offsetof(OEMDEV, iWMFontSize));
        m_PropertyMap[g_pszWMFontColor]    = UIPropertyPair(sizeof(DWORD), offsetof(OEMDEV, dwColText));
        m_PropertyMap[g_pszWMOffsetWidth]  = UIPropertyPair(sizeof(INT),   offsetof(OEMDEV, iWMOffsetX));
        m_PropertyMap[g_pszWMOffsetHeight] = UIPropertyPair(sizeof(INT),   offsetof(OEMDEV, iWMOffsetY));
        m_PropertyMap[g_pszWMSizeWidth]    = UIPropertyPair(sizeof(INT),   offsetof(OEMDEV, iWMWidth));
        m_PropertyMap[g_pszWMSizeHeight]   = UIPropertyPair(sizeof(INT),   offsetof(OEMDEV, iWMHeight));
        m_PropertyMap[g_pszWMText]         = UIPropertyPair(sizeof(TCHAR) * MAX_WATERMARK_TEXT,   offsetof(OEMDEV, strWMText));

        //
        // Initialise the GPD OptItem entries.
        // NOTE: These options will be removed from the standard Unidrv UI Treeview.
        //
        m_OptItemList.push_back("JobBindAllDocuments");
        m_OptItemList.push_back("DocumentBinding");
        m_OptItemList.push_back("PageColorManagement");
        m_OptItemList.push_back("PageSourceColorProfile");
        m_OptItemList.push_back("PageICMRenderingIntent");
        m_OptItemList.push_back("PageScaling");
        m_OptItemList.push_back("ScaleOffsetAlignment");
        m_OptItemList.push_back("PageWatermarkType");
        m_OptItemList.push_back("PageWatermarkLayering");
        m_OptItemList.push_back("JobNUpAllDocumentsContiguously");
        m_OptItemList.push_back("JobNUpContiguouslyPresentationOrder");
        m_OptItemList.push_back("DocumentNUp");
        m_OptItemList.push_back("DocumentNUpPresentationOrder");
        m_OptItemList.push_back("PageBorderless");
        m_OptItemList.push_back("PagePhotoPrintingIntent");
        m_OptItemList.push_back("DocumentDuplex");
    }
    catch (exception& DBG_ONLY(e))
    {
        ERR(e.what());
        hr = E_FAIL;
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CUIProperties::SetItem

Routine Description:

    This method is used to set properties in the private OEM devmode.

Arguments:

    pFeatureName - Pointer to the property name.
    pUIProperty - Pointer to the property value.
    cbSize - Size of the buffer pointed to by pUIProperty.

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CUIProperties::SetItem(
    _In_                PCSTR             pFeatureName,
    _In_reads_bytes_(cbSize) CONST UIProperty* pUIProperty,
    _In_                SIZE_T            cbSize
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(m_pOEMDev, E_PENDING)))
    {
        try
        {
            UIPropertyMap::const_iterator iterItem = m_PropertyMap.find(pFeatureName);

            if (iterItem != m_PropertyMap.end())
            {
                if (iterItem->second.first >= cbSize)
                {
                    memcpy(reinterpret_cast<LPBYTE>(m_pOEMDev) + iterItem->second.second, pUIProperty, cbSize);
                }
                else
                {
                    hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
                }
            }
            else
            {
                hr = E_INVALIDARG;
            }
        }
        catch (exception& DBG_ONLY(e))
        {
            ERR(e.what());
            hr = E_FAIL;
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CUIProperties::GetItem


Routine Description:

    This method is used to get properties in the private OEM devmode.

Arguments:

    pFeatureName - Pointer to the property name.
    pUIProperty - Pointer to the property value.
    cbSize - Size of the buffer pointed to by pUIProperty.

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CUIProperties::GetItem(
    _In_                 PCSTR       pFeatureName,
    _Out_writes_bytes_(cbSize) UIProperty* pUIProperty,
    _In_                 SIZE_T      cbSize
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(m_pOEMDev, E_PENDING)))
    {
        try
        {
            UIPropertyMap::const_iterator iterItem = m_PropertyMap.find(pFeatureName);

            if (iterItem != m_PropertyMap.end())
            {
                if (iterItem->second.first == cbSize)
                {
                    memcpy(pUIProperty, reinterpret_cast<LPBYTE>(m_pOEMDev) + iterItem->second.second, cbSize);
                }
                else
                {
                    hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
                }
            }
            else
            {
                hr = E_INVALIDARG;
            }
        }
        catch (exception& DBG_ONLY(e))
        {
            ERR(e.what());
            hr = E_FAIL;
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CUIProperties::SetHeader

Routine Description:

    Initialises the Unidrv private header portion of the OEM devmode.

Arguments:

    None

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CUIProperties::SetHeader()
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(m_pOEMDev, E_PENDING)))
    {
        //
        //OEM_DMEXTRAHEADER Members
        //
        m_pOEMDev->dmOEMExtra.dwSize       = sizeof(OEMDEV);
        m_pOEMDev->dmOEMExtra.dwSignature  = OEM_SIGNATURE;
        m_pOEMDev->dmOEMExtra.dwVersion    = OEM_VERSION;
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CUIProperties::SetDefaults

Routine Description:

    Initialises the OEM devmode with default values.

Arguments:

    None

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CUIProperties::SetDefaults()
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(m_pOEMDev, E_PENDING)))
    {
        if (SUCCEEDED(hr = SetHeader()))
        {
            //
            //Private members
            //

            //
            // Page Scaling
            //
            m_pOEMDev->dwPgScaleX   = pgscParamDefIntegers[ePageScalingScaleWidth].default_value;
            m_pOEMDev->dwPgScaleY   = pgscParamDefIntegers[ePageScalingScaleHeight].default_value;
            m_pOEMDev->iPgOffsetX   = MICRON_TO_HUNDREDTH_OFINCH(pgscParamDefIntegers[ePageScalingOffsetWidth].default_value);
            m_pOEMDev->iPgOffsetY   = MICRON_TO_HUNDREDTH_OFINCH(pgscParamDefIntegers[ePageScalingOffsetHeight].default_value);

            //
            // Page Watermark
            //
            m_pOEMDev->iWMTransparency = wmParamDefIntegers[ePageWatermarkTransparency].default_value;
            m_pOEMDev->iWMAngle        = wmParamDefIntegers[ePageWatermarkAngle].default_value;
            m_pOEMDev->iWMOffsetX      = MICRON_TO_HUNDREDTH_OFINCH(wmParamDefIntegers[ePageWatermarkOriginWidth].default_value);
            m_pOEMDev->iWMOffsetY      = MICRON_TO_HUNDREDTH_OFINCH(wmParamDefIntegers[ePageWatermarkOriginHeight].default_value);

            m_pOEMDev->iWMFontSize     = wmParamDefIntegers[ePageWatermarkTextFontSize].default_value;
            m_pOEMDev->dwColText       = wmParamDefIntegers[ePageWatermarkTextColor].default_value;

            try
            {
                CStringXD cstrWMText(wmParamDefStrings[ePageWatermarkTextText].default_value);
                hr = StringCchCopyN(m_pOEMDev->strWMText, MAX_WATERMARK_TEXT, cstrWMText.GetBuffer(), cstrWMText.GetLength());
            }
            catch (CXDException& e)
            {
                hr = e;
            }

            m_pOEMDev->iWMWidth        = MICRON_TO_HUNDREDTH_OFINCH(wmParamDefIntegers[ePageWatermarkSizeWidth].default_value);
            m_pOEMDev->iWMHeight       = MICRON_TO_HUNDREDTH_OFINCH(wmParamDefIntegers[ePageWatermarkSizeHeight].default_value);
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CUIProperties::Convert

Routine Description:

    This method is used to convert the OEM private devmode portion of this IUIProperty interface
    given another interface as input.

Arguments:

    pUIProperties - Pointer to the source IUIProperty interface to be converted.

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CUIProperties::Convert(
    _In_ CUIProperties * pUIProperties
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pUIProperties, E_POINTER)) &&
        SUCCEEDED(hr = CHECK_POINTER(pUIProperties->m_pOEMDev, E_PENDING)) &&
        SUCCEEDED(hr = CHECK_POINTER(m_pOEMDev, E_PENDING)))
    {
        //
        // Check OEM Signature, if it doesn't match ours,
        // then just assume DMIn is bad and use defaults.
        //
        if (m_pOEMDev->dmOEMExtra.dwSignature == pUIProperties->m_pOEMDev->dmOEMExtra.dwSignature)
        {
            if (SUCCEEDED(hr = SetDefaults()))
            {
                // Copy the old structure in to the new using which ever size is the smaller.
                // Devmode maybe from newer Devmode (not likely since there is only one), or
                // Devmode maybe a newer Devmode, in which case it maybe larger,
                // but the first part of the structure should be the same.

                // DESIGN ASSUMPTION: the private DEVMODE structure only gets added to;
                // the fields that are in the DEVMODE never change only new fields get added to the end.

                DWORD dwSize = __min(m_pOEMDev->dmOEMExtra.dwSize, pUIProperties->m_pOEMDev->dmOEMExtra.dwSize);
                memcpy(m_pOEMDev, pUIProperties->m_pOEMDev, dwSize);

                // Re-fill in the size and version fields to indicated
                // that the DEVMODE is the current private DEVMODE version.
                hr = SetHeader();
            }
        }
        else
        {
            hr = SetDefaults();
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CUIProperties::Validate

Routine Description:

    This method is used to ensure that the OEM private devmode is validated.

Arguments:

    None

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CUIProperties::Validate()
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(m_pOEMDev, E_PENDING)))
    {
        //
        // ASSUMPTION: pOEMDevmode is large enough to contain OEMDEV structure.
        // Make sure that dmOEMExtra indicates the current OEMDEV structure.
        //
        if (SUCCEEDED(hr = SetHeader()))
        {
            //
            // Page Scaling Members
            //
            if (m_pOEMDev->dwPgScaleX < static_cast<DWORD>(pgscParamDefIntegers[ePageScalingScaleWidth].min_length) ||
                m_pOEMDev->dwPgScaleX > static_cast<DWORD>(pgscParamDefIntegers[ePageScalingScaleWidth].max_length))
            {
                m_pOEMDev->dwPgScaleX   = static_cast<DWORD>(pgscParamDefIntegers[ePageScalingScaleWidth].default_value);
            }

            if (m_pOEMDev->dwPgScaleY < static_cast<DWORD>(pgscParamDefIntegers[ePageScalingScaleHeight].min_length) ||
                m_pOEMDev->dwPgScaleY > static_cast<DWORD>(pgscParamDefIntegers[ePageScalingScaleHeight].max_length))
            {
                m_pOEMDev->dwPgScaleY   = static_cast<DWORD>(pgscParamDefIntegers[ePageScalingScaleHeight].default_value);
            }

            if (m_pOEMDev->iPgOffsetX < MICRON_TO_HUNDREDTH_OFINCH(pgscParamDefIntegers[ePageScalingOffsetWidth].min_length) ||
                m_pOEMDev->iPgOffsetX > MICRON_TO_HUNDREDTH_OFINCH(pgscParamDefIntegers[ePageScalingOffsetWidth].max_length))
            {
                m_pOEMDev->iPgOffsetX   = MICRON_TO_HUNDREDTH_OFINCH(pgscParamDefIntegers[ePageScalingOffsetWidth].default_value);
            }

            if (m_pOEMDev->iPgOffsetY < MICRON_TO_HUNDREDTH_OFINCH(pgscParamDefIntegers[ePageScalingOffsetHeight].min_length) ||
                m_pOEMDev->iPgOffsetY > MICRON_TO_HUNDREDTH_OFINCH(pgscParamDefIntegers[ePageScalingOffsetHeight].max_length))
            {
                m_pOEMDev->iPgOffsetY   = MICRON_TO_HUNDREDTH_OFINCH(pgscParamDefIntegers[ePageScalingOffsetHeight].default_value);
            }

            //
            // Watermark Members
            //
            if (m_pOEMDev->iWMTransparency < wmParamDefIntegers[ePageWatermarkTransparency].min_length ||
                m_pOEMDev->iWMTransparency > wmParamDefIntegers[ePageWatermarkTransparency].max_length)
            {
                m_pOEMDev->iWMTransparency = wmParamDefIntegers[ePageWatermarkTransparency].default_value;
            }

            if (m_pOEMDev->iWMAngle < wmParamDefIntegers[ePageWatermarkAngle].min_length ||
                m_pOEMDev->iWMAngle > wmParamDefIntegers[ePageWatermarkAngle].max_length)
            {
                m_pOEMDev->iWMAngle = wmParamDefIntegers[ePageWatermarkAngle].default_value;
            }

            if (m_pOEMDev->iWMOffsetX < MICRON_TO_HUNDREDTH_OFINCH(wmParamDefIntegers[ePageWatermarkOriginWidth].min_length) ||
                m_pOEMDev->iWMOffsetX > MICRON_TO_HUNDREDTH_OFINCH(wmParamDefIntegers[ePageWatermarkOriginWidth].max_length))
            {
                m_pOEMDev->iWMOffsetX = MICRON_TO_HUNDREDTH_OFINCH(wmParamDefIntegers[ePageWatermarkOriginWidth].default_value);
            }

            if (m_pOEMDev->iWMOffsetY < MICRON_TO_HUNDREDTH_OFINCH(wmParamDefIntegers[ePageWatermarkOriginHeight].min_length) ||
                m_pOEMDev->iWMOffsetY > MICRON_TO_HUNDREDTH_OFINCH(wmParamDefIntegers[ePageWatermarkOriginHeight].max_length))
            {
                m_pOEMDev->iWMOffsetY = MICRON_TO_HUNDREDTH_OFINCH(wmParamDefIntegers[ePageWatermarkOriginHeight].default_value);
            }

            if (m_pOEMDev->iWMFontSize < wmParamDefIntegers[ePageWatermarkTextFontSize].min_length ||
                m_pOEMDev->iWMFontSize > wmParamDefIntegers[ePageWatermarkTextFontSize].max_length)
            {
                m_pOEMDev->iWMFontSize = wmParamDefIntegers[ePageWatermarkTextFontSize].default_value;
            }

            if (m_pOEMDev->dwColText < static_cast<DWORD>(wmParamDefIntegers[ePageWatermarkTextColor].min_length) ||
                m_pOEMDev->dwColText > static_cast<DWORD>(wmParamDefIntegers[ePageWatermarkTextColor].max_length))
            {
                m_pOEMDev->dwColText = wmParamDefIntegers[ePageWatermarkTextColor].default_value;
            }

            if (m_pOEMDev->iWMWidth < MICRON_TO_HUNDREDTH_OFINCH(wmParamDefIntegers[ePageWatermarkSizeWidth].min_length) ||
                m_pOEMDev->iWMWidth > MICRON_TO_HUNDREDTH_OFINCH(wmParamDefIntegers[ePageWatermarkSizeWidth].max_length))
            {
                m_pOEMDev->iWMWidth = MICRON_TO_HUNDREDTH_OFINCH(wmParamDefIntegers[ePageWatermarkSizeWidth].default_value);
            }

            if (m_pOEMDev->iWMHeight < MICRON_TO_HUNDREDTH_OFINCH(wmParamDefIntegers[ePageWatermarkSizeHeight].min_length) ||
                m_pOEMDev->iWMHeight > MICRON_TO_HUNDREDTH_OFINCH(wmParamDefIntegers[ePageWatermarkSizeHeight].max_length))
            {
                m_pOEMDev->iWMHeight = MICRON_TO_HUNDREDTH_OFINCH(wmParamDefIntegers[ePageWatermarkSizeHeight].default_value);
            }

            //
            // Valid text is a NULL terminated string of length < MAX_WATERMARK_TEXT
            //
            size_t cch = 0;
            if (FAILED(StringCchLength(m_pOEMDev->strWMText, MAX_WATERMARK_TEXT, &cch)))
            {
                try
                {
                    CStringXD cstrWMText(wmParamDefStrings[ePageWatermarkTextText].default_value);
                    hr = StringCchCopyN(m_pOEMDev->strWMText, MAX_WATERMARK_TEXT, cstrWMText.GetBuffer(), cstrWMText.GetLength());
                }
                catch (CXDException& e)
                {
                    hr = e;
                }
            }
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CUIProperties::GetOptItem

Routine Description:

    This method is used to get properties in the GPD.

Arguments:

    pOemCUIPParam - Pointer to the POEMCUIPPARAM function.
    pFeatureName - Pointer to a GPD property name.
    ppOptItem - Address of a pointer that will be filled out with an OPTITEM structure.

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CUIProperties::GetOptItem(
    _In_            POEMCUIPPARAM pOemCUIPParam,
    _In_            PCSTR         pFeatureName,
    _Outptr_result_maybenull_ POPTITEM*     ppOptItem
    )
{
    HRESULT hr         = S_OK;
    POPTITEM pOIResult = NULL;

    if (SUCCEEDED(hr = CHECK_POINTER(pOemCUIPParam, E_POINTER)) &&
        SUCCEEDED(hr = CHECK_POINTER(pOemCUIPParam->pDrvOptItems, E_PENDING)) &&
        SUCCEEDED(hr = CHECK_POINTER(pFeatureName,  E_POINTER)) &&
        SUCCEEDED(hr = CHECK_POINTER(ppOptItem,     E_POINTER)))
    {
        *ppOptItem = NULL;

        for (DWORD indexOptItem = 0; indexOptItem < pOemCUIPParam->cDrvOptItems; indexOptItem++)
        {
            pOIResult = &(pOemCUIPParam->pDrvOptItems[indexOptItem]);

            if (pOIResult->UserData != NULL)
            {
                PUSERDATA pUserData = reinterpret_cast<PUSERDATA>(pOIResult->UserData);
                if (SUCCEEDED(hr = CHECK_POINTER(pUserData->pKeyWordName, E_FAIL)))
                {
                    if (strncmp(pUserData->pKeyWordName, pFeatureName, strlen(pFeatureName)) == 0)
                    {
                        hr = S_OK;
                        *ppOptItem = pOIResult;
                        break;
                    }
                    else
                    {
                        hr = E_ELEMENT_NOT_FOUND;
                    }
                }
            }
            else
            {
                hr = E_FAIL;
            }
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CUIProperties::HideOptItems

Routine Description:

    Ensures that all GPD settings managed by the UI Plug-in are removed from the main Unidrv driver UI.

Arguments:

    pOemCUIPParam - Pointer to the POEMCUIPPARAM function.

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CUIProperties::HideOptItems(
    _In_ POEMCUIPPARAM         pOemCUIPParam
    )
{
    HRESULT hr         = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pOemCUIPParam, E_POINTER)) &&
        SUCCEEDED(hr = CHECK_POINTER(pOemCUIPParam->pDrvOptItems, E_PENDING)))
    {
        POPTITEM pOptItem = NULL;

        try
        {
            UIOptItemList::const_iterator iterOptItem = m_OptItemList.begin();

            for (; iterOptItem != m_OptItemList.end() && SUCCEEDED(hr); iterOptItem++)
            {
                if (SUCCEEDED(hr = GetOptItem(pOemCUIPParam, *iterOptItem, &pOptItem)) &&
                    SUCCEEDED(hr = CHECK_POINTER(pOptItem, E_FAIL)))
                {
                    pOptItem->Flags |= OPTIF_HIDE;
                }
            }
        }
        catch (exception& DBG_ONLY(e))
        {
            ERR(e.what());
            hr = E_FAIL;
        }
    }

    ERR_ON_HR(hr);
    return hr;
}


