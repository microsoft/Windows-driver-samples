/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   colconv.cpp

Abstract:

   Color conversion manager implementation. The CColorConverter class is responsible
   for coordinating the handling of the color conversion process.

   Note regarding transform caching: The color filter provides caching for the last
   transform. This aids performance as quite some time can be consumed creating a
   transform. The filter may however suffer if source content color profiles are rapidly
   switched (e.g. alternating sRGB and scRGB mark-up). This could be mitigated by increasing
   the number cached transforms beyond one.

--*/

#include "precomp.h"
#include "debug.h"
#include "globals.h"
#include "xdstring.h"
#include "xdexcept.h"
#include "colconv.h"
#include "cmimg.h"
#include "dictionary.h"

using XDPrintSchema::PageSourceColorProfile::EProfileOption;
using XDPrintSchema::PageSourceColorProfile::RGB;
using XDPrintSchema::PageSourceColorProfile::CMYK;

using XDPrintSchema::PageICMRenderingIntent::AbsoluteColorimetric;
using XDPrintSchema::PageICMRenderingIntent::RelativeColorimetric;
using XDPrintSchema::PageICMRenderingIntent::Photographs;
using XDPrintSchema::PageICMRenderingIntent::BusinessGraphics;

COLORTYPE g_nChannelMap[] = {
    COLOR_3_CHANNEL,
    COLOR_CMYK,
    COLOR_5_CHANNEL,
    COLOR_6_CHANNEL,
    COLOR_7_CHANNEL,
    COLOR_8_CHANNEL
};

/*++

Routine Name:

    CColorConverter::CColorConverter

Routine Description:

    Constructor for the base CColorConverter class. Provides common functionality
    between the bitmap and color ref converter classes

Arguments:

    pXpsConsumer - Pointer to the XPS consumer interface. Used to write out resources
    pFixedPage   - Pointer to the FixedPage interface. Resource cache uses this when writing
    pResCache    - Pointer to the resource cache.
    pProfManager - Pointer to a profile manager for supplying a suitable color profile

Return Value:

    None
    Throws CXDException(HRESULT) on an error

--*/
CColorConverter::CColorConverter(
    _In_ IXpsDocumentConsumer* pXpsConsumer,
    _In_ IFixedPage*           pFixedPage,
    _In_ CFileResourceCache*   pResCache,
    _In_ CProfileManager*      pProfManager,
    _In_ ResDeleteMap*         pResDel
    ) :
    m_pXpsConsumer(pXpsConsumer),
    m_pFixedPage(pFixedPage),
    m_pProfManager(pProfManager),
    m_pResCache(pResCache),
    m_pResDel(pResDel)
{
    HRESULT hr = S_OK;

    if (FAILED(hr = CHECK_POINTER(m_pXpsConsumer, E_POINTER)) ||
        FAILED(hr = CHECK_POINTER(m_pFixedPage, E_POINTER)) ||
        FAILED(hr = CHECK_POINTER(m_pProfManager, E_POINTER)) ||
        FAILED(hr = CHECK_POINTER(m_pResCache, E_POINTER)) ||
        FAILED(hr = CHECK_POINTER(m_pResDel, E_POINTER)))
    {
        throw CXDException(hr);
    }
}

/*++

Routine Name:

    CColorConverter::~CColorConverter

Routine Description:

    Default destructor for the CColorConverter class

Arguments:

    None

Return Value:

    None

--*/
CColorConverter::~CColorConverter()
{
}

/*++

Routine Name:

    CBitmapColorConverter::CBitmapColorConverter

Routine Description:

    Constructor for the CBitmapColorConverter class

Arguments:

    pXpsConsumer - Pointer to the XPS consumer interface. Used to write out resources
    pFixedPage   - Pointer to the FixedPage interface. Resource cache uses this when writing
    pResCache    - Pointer to the resource cache.
    pProfManager - Pointer to a profile manager for supplying a suitable color profile

Return Value:

    None
    Throws CXDException(HRESULT) on an error

--*/
CBitmapColorConverter::CBitmapColorConverter(
    _In_ IXpsDocumentConsumer* pXpsConsumer,
    _In_ IFixedPage*           pFixedPage,
    _In_ CFileResourceCache*   pResCache,
    _In_ CProfileManager*      pProfManager,
    _In_ ResDeleteMap*         pResDel
    ) :
    CColorConverter(pXpsConsumer, pFixedPage, pResCache, pProfManager, pResDel)
{
}

/*++

Routine Name:

    CBitmapColorConverter::~CBitmapColorConverter

Routine Description:

    Destructor for the CBitmapColorConverter class

Arguments:

    None

Return Value:

    None

--*/
CBitmapColorConverter::~CBitmapColorConverter()
{
}

/*++

Routine Name:

    CColorConverter::ConvertBitmap

Routine Description:

    Method which performs the color conversion process to a bitmap resource and sets
    the resource URI to contain the URI of the converted bitmap resource

Arguments:

    pbstrBmpURI - Pointer to a string containing the bitmap resource path and name

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CBitmapColorConverter::Convert(
    _Inout_ BSTR* pbstrBmpURI
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pbstrBmpURI, E_POINTER)))
    {
        try
        {
            //
            // Create a color managed image object
            //
            CColorManagedImage bmpColManaged(*pbstrBmpURI, m_pProfManager, m_pFixedPage, m_pResDel);

            //
            // Write out the cached bitmap and get a keyname for the written bitmap
            //
            CComBSTR bstrKey;
            if (SUCCEEDED(hr = m_pResCache->WriteResource<IPartImage>(m_pXpsConsumer, m_pFixedPage, &bmpColManaged)) &&
                SUCCEEDED(hr = bmpColManaged.GetKeyName(&bstrKey)))
            {
                hr = m_pResCache->GetURI(bstrKey, pbstrBmpURI);

                ASSERTMSG(SUCCEEDED(hr), "Failed to process image");
            }
        }
        catch (CXDException& e)
        {
            hr = e;
        }
    }
    else
    {
        hr = E_POINTER;
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CColorRefConverter::CColorRefConverter

Routine Description:

    Constructor for the base CColorRefConverter class which handles parsing and
    conversion of a color string within XPS mark-up

Arguments:

    pXpsConsumer - Pointer to the XPS consumer interface. Used to write out resources
    pFixedPage   - Pointer to the FixedPage interface. Resource cache uses this when writing
    pResCache    - Pointer to the resource cache.
    pProfManager - Pointer to a profile manager for supplying a suitable color profile

Return Value:

    None
    Throws CXDException(HRESULT) on an error

--*/
CColorRefConverter::CColorRefConverter(
    _In_ IXpsDocumentConsumer* pXpsConsumer,
    _In_ IFixedPage*           pFixedPage,
    _In_ CFileResourceCache*   pResCache,
    _In_ CProfileManager*      pProfManager,
    _In_ ResDeleteMap*         pResDel
    ) :
    CColorConverter(pXpsConsumer, pFixedPage, pResCache, pProfManager, pResDel)
{
}

/*++

Routine Name:

    CColorRefConverter::~CColorRefConverter

Routine Description:

    Destructor for the CColorRefConverter class

Arguments:

    None

Return Value:

    None

--*/
CColorRefConverter::~CColorRefConverter()
{
}

/*++

Routine Name:

    CColorRefConverter::ConvertColor

Routine Description:

    Method which performs the color conversion process to an XPS color ref element

Arguments:

    pbstrColorRef - Pointer to a string containing the color data to be converted

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CColorRefConverter::Convert(
    _Inout_ BSTR* pbstrColorRef
    )
{
    HRESULT hr = S_OK;

    BOOL bIsResourceReference = FALSE;
    if (SUCCEEDED(hr = CHECK_POINTER(pbstrColorRef, E_POINTER)) &&
        SUCCEEDED(hr = ParseColorString(*pbstrColorRef, &m_srcData, &bIsResourceReference)) &&
        !bIsResourceReference)
    {
        //
        // If the source and destination types are the same (e.g. scRGB in and scRGB out)
        // we need go no further - simply return the existing string. This does not apply
        // to nChannel as ContextColors specify their own profile and we cannot prove that
        // this matches our output. The color space for sRGB and scRGB are implicit however.
        //
        EColorDataType srcType = sRGB;
        EColorDataType dstType = sRGB;
        if (SUCCEEDED(hr = InitDstChannels(&m_srcData, &m_dstData)) &&
            SUCCEEDED(hr = m_srcData.GetColorDataType(&srcType)) &&
            SUCCEEDED(hr = m_dstData.GetColorDataType(&dstType)))
        {
            if (dstType == nChannel ||
                dstType != srcType)
            {
                if (SUCCEEDED(hr = TransformColor(&m_srcData, &m_dstData)))
                {
                    hr = CreateColorString(&m_dstData, pbstrColorRef);
                }
            }
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CColorRefConverter::InitDstChannels

Routine Description:

    Initialise the destination color channel data based on the destination color
    profile

Arguments:

    pChannelDataDst - Pointer to a color channel data object to be intialised

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CColorRefConverter::InitDstChannels(
    _In_    CColorChannelData* pChannelDataSrc,
    _Inout_ CColorChannelData* pChannelDataDst
    )
{
    HRESULT hr = S_OK;

    EProfileOption profileType = RGB;
    if (SUCCEEDED(hr = CHECK_POINTER(pChannelDataSrc, E_POINTER)) &&
        SUCCEEDED(hr = CHECK_POINTER(pChannelDataDst, E_POINTER)) &&
        SUCCEEDED(hr = CHECK_POINTER(m_pProfManager, E_FAIL)) &&
        SUCCEEDED(hr = m_pProfManager->GetDstProfileType(&profileType)))
    {
        //
        // Initialize the channel data based on the destination color profile
        // settings:
        //    RGB  = 3 channel with no src alpha
        //           4 channel with src alpha
        //    CMYK = 5 channel (nChannel always has an alpha channel)
        //
        // The color type is based off the OS. XP does not have access to WCS
        // and therefore floating point formats are converted to 16 bits per channel
        //
        // The conversion process for  downlevel handling depends on the source format.
        // If we are color matching for scRGB we convert the floating point value as
        // follows:
        //
        //    1. Truncate the input color to between -2.0 and +2.0
        //    2. Offset the value by +2.0 to put it into the 0.0 to 4.0 range
        //    3. Scale the value by 4.0 to put it into the range 0.0 to 1.0
        //    4. Set the 16 bit value according to the channel value from 0x0000
        //       for 0.0 to 0xFFFF for 1.0
        //    5. Call TranslateColors passing the 16 bit value
        //    6. Reverse steps 1 - 4 to retrieve the floating point value
        //
        //  If we are color matching for context colors we apply the following conversion
        //
        //    1. Truncate the input color to between 0.0 and +1.0
        //    2. Set the 16 bit value according to the channel value from 0x0000
        //       for 0.0 to 0xFFFF for 1.0
        //    3. Call TranslateColors passing the 16 bit value
        //    4. Reverse steps 1 - 2 to retrieve the floating point value
        //
        //  Note: We do not modify the gamma during the scRGB conversion process as this will
        //        be applied by WCS/ICM as the gamma is encapsulated by the relevant ICC
        //        profile. i.e. The gamma is maintained linear as that is what the transform
        //        is expecting.
        //
        COLORDATATYPE colorDataType = COLOR_FLOAT;
        EColorDataType dataType = sRGB;

        DWORD cChannels = 0;
        if (profileType == RGB)
        {
            dataType = scRGB;
            cChannels = 3;
            if (pChannelDataSrc->HasAlpha())
            {
                cChannels++;
            }
        }
        else if (profileType == CMYK)
        {
            dataType = nChannel;
            cChannels = 5;
        }
        else
        {
            RIP("Unsupported color channel format\n");
            hr = E_FAIL;
        }

        if (SUCCEEDED(hr) &&
            SUCCEEDED(hr = pChannelDataDst->InitializeChannelData<FLOAT>(colorDataType, dataType, cChannels, 0.0f)))
        {
            hr = pChannelDataDst->InitializeAlphaChannel(pChannelDataSrc);
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CColorRefConverter::GetFloatChannelData

Routine Description:

    Method which converts a comma delimited set of floating point values
    defining color channels into floating point values in a list

Arguments:

    szColorRef   - The srting containing the comma seperated color values
    pChannelData - Pointer to the color channels object

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CColorRefConverter::GetFloatChannelData(
    _In_    LPCWSTR            szColorRef,
    _Inout_ CColorChannelData* pChannelData
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pChannelData, E_POINTER)) &&
        SUCCEEDED(hr = CHECK_POINTER(szColorRef, E_POINTER)) &&
        SUCCEEDED(hr = pChannelData->ResetChannelType(COLOR_FLOAT)))
    {
        try
        {
            CStringXDW cstrColorRef(szColorRef);
            cstrColorRef.Trim();
            cstrColorRef.MakeLower();

            INT cTokenIndex = 0;
            INT cChars = cstrColorRef.GetLength();
            while (SUCCEEDED(hr) &&
                   cTokenIndex < cChars &&
                   cTokenIndex != -1)
            {
                CStringXDW cstrChannel(cstrColorRef.Tokenize(L",", cTokenIndex));
                hr = pChannelData->AddChannelData(static_cast<FLOAT>(_wtof(cstrChannel)));
            }
        }
        catch (CXDException& e)
        {
            hr = e;
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CColorRefConverter::SetFloatChannelData

Routine Description:

    Method which converts channel data object into a comma delimited set of
    floating point values

Arguments:

    pChannelData      - Pointer to the color channel data
    pcstrChannelData  - Pointer to the color string to be populated

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CColorRefConverter::SetFloatChannelData(
    _In_    CColorChannelData* pChannelData,
    _Inout_ CStringXDW*        pcstrChannelData
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pChannelData, E_POINTER)) &&
        SUCCEEDED(hr = CHECK_POINTER(pcstrChannelData, E_POINTER)))
    {
        try
        {
            //
            // Over all channels, write out comma seperated float values
            //
            DWORD  cbData = 0;
            PFLOAT pData = NULL;
            DWORD cChanCount = 0;
            if (SUCCEEDED(hr = pChannelData->GetChannelData(&cbData, reinterpret_cast<PVOID*>(&pData))) &&
                SUCCEEDED(hr = pChannelData->GetChannelCount(&cChanCount)))
            {
                if (cChanCount == 0)
                {
                    hr = E_FAIL;
                }

                if (SUCCEEDED(hr) &&
                    cbData >= cChanCount * sizeof(FLOAT))
                {
                    CStringXDW cstrChannel;
                    cstrChannel.Format(L"%f", pData[0]);
                    pcstrChannelData->Append(cstrChannel);
                    for (DWORD cChan = 1; cChan < cChanCount; cChan++)
                    {
                        cstrChannel.Format(L",%f", pData[cChan]);
                        pcstrChannelData->Append(cstrChannel);
                    }
                }
                else
                {
                    hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
                }
            }
        }
        catch (CXDException& e)
        {
            hr = e;
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CColorRefConverter::ParseColorString

Routine Description:

    Method which converts a XML formatted color string into useable color data

Arguments:

    bstrColorRef - The color ref string to be parsed
    pChannelData - Pointer to the color channel data object to be populated

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CColorRefConverter::ParseColorString(
    _In_    BSTR               bstrColorRef,
    _Inout_ CColorChannelData* pChannelData,
    _Out_   BOOL*              pbIsResourceReference
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pChannelData, E_POINTER)) &&
        SUCCEEDED(hr = CHECK_POINTER(pbIsResourceReference, E_POINTER)))
    {
        *pbIsResourceReference = FALSE;

        if (SysStringLen(bstrColorRef) <= 0)
        {
            hr = E_INVALIDARG;
        }
    }

    if (SUCCEEDED(hr))
    {
        try
        {
            CStringXDW cstrColRef(bstrColorRef);
            cstrColRef.Trim();

            if (0 == cstrColRef.Find(L"sc#"))
            {
                //
                // Strip the "sc#" prefix ready to parse the floating point channels
                //
                cstrColRef.Delete(0, 3);

                //
                // Set the src profile to xdwscRGB.icc - this is an ICC profile with
                // the system wcsRGB profile embedded so should work down-level
                // Set the color data type and retrieve the channel data
                //
                if (SUCCEEDED(hr = m_pProfManager->SetSrcProfileFromColDir(L"xdwscRGB.icc")) &&
                    SUCCEEDED(hr = pChannelData->ResetChannelType(COLOR_FLOAT)) &&
                    SUCCEEDED(hr = pChannelData->SetColorDataType(scRGB)))
                {
                    hr = GetFloatChannelData(cstrColRef, pChannelData);
                }
            }
            else if (0 == cstrColRef.Find(L"ContextColor"))
            {
                //
                // Context colors always have an associated profile - retrieve
                // this and set it in the profile manager
                //
                cstrColRef.Delete(0, countof(L"ContextColor "));
                cstrColRef.Trim();
                CStringXDW cstrProfile(cstrColRef.Left(cstrColRef.Find(L" ")));
                cstrColRef.Delete(0, cstrProfile.GetLength());

                //
                // Set the source profile ready to create the transform, retrieve the channel data
                // and clamp between 0.0 and 1.0.
                //
                if (SUCCEEDED(hr = m_pProfManager->SetSrcProfileFromContainer(cstrProfile.GetBuffer())) &&
                    SUCCEEDED(hr = pChannelData->ResetChannelType(COLOR_FLOAT)) &&
                    SUCCEEDED(hr = pChannelData->SetColorDataType(nChannel)) &&
                    SUCCEEDED(hr = GetFloatChannelData(cstrColRef, pChannelData)) &&
                    SUCCEEDED(hr = pChannelData->ClampChannelValues(0.0f, 1.0f)))
                {
                    //
                    // Mark the color profile for deletion
                    //
                    (*m_pResDel)[cstrProfile] = TRUE;
                }
            }
            else if (0 == cstrColRef.Find(L"#"))
            {
                //
                // Delete the # symbol so we are just left with the RGB values
                //
                cstrColRef.Delete(0, 1);

                //
                // Set the source profile to sRGB rather than assume the default is set to
                // sRGB.
                //
                if (SUCCEEDED(hr = m_pProfManager->SetSrcProfileFromColDir(L"sRGB Color Space Profile.icm")) &&
                    SUCCEEDED(hr = pChannelData->ResetChannelType(COLOR_BYTE)) &&
                    SUCCEEDED(hr = pChannelData->SetColorDataType(sRGB)))
                {
                    while (SUCCEEDED(hr) &&
                           cstrColRef.GetLength() > 0)
                    {
                        //
                        // Add the channel data
                        //
                        hr = pChannelData->AddChannelData(static_cast<BYTE>(wcstol(cstrColRef.Left(2), NULL, 16)));

                        //
                        // Delete the channel from the color ref string
                        //
                        cstrColRef.Delete(0, 2);
                    }
                }
            }
            else if (0 == cstrColRef.Find(L"{StaticResource"))
            {
                *pbIsResourceReference = TRUE;
            }
            else
            {
                //
                // Unrecognised type
                //
                RIP("Unrecognised color syntax.");
                hr = E_FAIL;
            }
        }
        catch (CXDException& e)
        {
            hr = e;
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

    CColorRefConverter::TransformColor

Routine Description:

    Method which applies a color transform to a set of color data

Arguments:

    pSrcData - Pointer to the source color channel data
    pDstData - Pointer to the destination color channel data

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CColorRefConverter::TransformColor(
    _In_    CColorChannelData* pSrcData,
    _Inout_ CColorChannelData* pDstData
    )
{
    HRESULT hr = S_OK;

    HTRANSFORM hColorTrans = NULL;

    BOOL bUseWCS = FALSE;

    if (SUCCEEDED(hr = CHECK_POINTER(pSrcData, E_POINTER)) &&
        SUCCEEDED(hr = CHECK_POINTER(pDstData, E_POINTER)) &&
        SUCCEEDED(hr = m_pProfManager->GetColorTransform(&hColorTrans, &bUseWCS)))
    {
        if (bUseWCS)
        {
            DWORD cSrcChan = 0;
            DWORD cDstChan = 0;

            COLORDATATYPE srcDataType = COLOR_FLOAT;
            COLORDATATYPE dstDataType = COLOR_FLOAT;

            DWORD cbSrcChan = 0;
            DWORD cbDstChan = 0;

            PBYTE pSrcBuff = NULL;
            PBYTE pDstBuff = NULL;

            if (SUCCEEDED(hr = CHECK_POINTER(hColorTrans, E_HANDLE)) &&
                SUCCEEDED(hr = pSrcData->GetChannelCountNoAlpha(&cSrcChan)) &&
                SUCCEEDED(hr = pSrcData->GetChannelType(&srcDataType)) &&
                SUCCEEDED(hr = pSrcData->GetChannelDataNoAlpha(&cbSrcChan, reinterpret_cast<PVOID*>(&pSrcBuff))) &&
                SUCCEEDED(hr = pDstData->GetChannelCountNoAlpha(&cDstChan)) &&
                SUCCEEDED(hr = pDstData->GetChannelType(&dstDataType)) &&
                SUCCEEDED(hr = pDstData->GetChannelDataNoAlpha(&cbDstChan, reinterpret_cast<PVOID*>(&pDstBuff))))
            {
                if (cbDstChan > 0)
                {
                    if (!WcsTranslateColorsXD(hColorTrans,
                                              1,
                                              cSrcChan,
                                              srcDataType,
                                              cbSrcChan,
                                              pSrcBuff,
                                              cDstChan,
                                              dstDataType,
                                              cbDstChan,
                                              pDstBuff))
                    {
                        hr = GetLastErrorAsHResult();
                    }
                }
                else
                {
                    hr = E_FAIL;
                }
            }
        }
        else
        {
            COLOR srcColor;
            COLOR dstColor;

            COLORTYPE srcType;
            COLORTYPE dstType;

            if (SUCCEEDED(hr = pSrcData->GetColor(&srcColor, &srcType)) &&
                SUCCEEDED(hr = pDstData->GetColor(&dstColor, &dstType)))
            {
                if (TranslateColors(hColorTrans,
                                    &srcColor,
                                    1,
                                    srcType,
                                    &dstColor,
                                    dstType))
                {
                    hr = pDstData->SetColor(&dstColor, dstType);
                }
                else
                {
                    hr = GetLastErrorAsHResult();
                }
            }
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CColorRefConverter::CreateColorString

Routine Description:

    Method which creates a XML formatted string descripting a transformed set of colors

Arguments:

    pDstData      - Pointer to destination color channel data
    pbstrColorRef - Pointer to a string to contain the XML formatted color data

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CColorRefConverter::CreateColorString(
    _In_    CColorChannelData* pDstData,
    _Inout_ BSTR*              pbstrColorRef
    )
{
    CComBSTR bstrTmpHolder;

    HRESULT hr = S_OK;

    EColorDataType colDataType = sRGB;
    if (SUCCEEDED(hr = CHECK_POINTER(pDstData,  E_POINTER)) &&
        SUCCEEDED(hr = CHECK_POINTER(pbstrColorRef, E_POINTER)) &&
        SUCCEEDED(hr = pDstData->GetColorDataType(&colDataType)))
    {
        try
        {
            CStringXDW cstrChannelData;

            if (colDataType == scRGB)
            {
                //
                // scRGB
                //
                cstrChannelData += L"sc#";
                hr = SetFloatChannelData(pDstData, &cstrChannelData);
            }
            else if (colDataType == nChannel)
            {
                //
                // CMYK
                //
                cstrChannelData += L"ContextColor ";

                //
                // Add the ICC profile and retrieve the URI
                //
                CComBSTR bstrKey;
                CComBSTR bstrICCURI;
                if (SUCCEEDED(hr = m_pResCache->WriteResource<IPartColorProfile>(m_pXpsConsumer, m_pFixedPage, m_pProfManager)) &&
                    SUCCEEDED(hr = m_pProfManager->GetKeyName(&bstrKey)) &&
                    SUCCEEDED(hr = m_pResCache->GetURI(bstrKey, &bstrICCURI)))
                {
                    //
                    // Append the URI and a space
                    //
                    cstrChannelData += bstrICCURI;
                    cstrChannelData += L" ";

                    hr = SetFloatChannelData(pDstData, &cstrChannelData);
                }
            }
            else if (colDataType == sRGB)
            {
                //
                // sRGB
                //
                cstrChannelData += L"#";

                //
                // Over all channels, write out hex byte values
                //
                DWORD cbData = 0;
                PBYTE pData = NULL;
                DWORD cChanCount = 0;
                if (SUCCEEDED(hr = pDstData->GetChannelData(&cbData, reinterpret_cast<PVOID*>(&pData))) &&
                    SUCCEEDED(hr = pDstData->GetChannelCount(&cChanCount)))
                {
                    if (cbData >= cChanCount * sizeof(BYTE))
                    {
                        for (DWORD cChan = 0; cChan < cChanCount; cChan++)
                        {
                            _Analysis_assume_(cChan < cbData);

                            CStringXDW cstrChannel;
                            cstrChannel.Format(L"%02x", pData[cChan]);
                            cstrChannelData.Append(cstrChannel);
                        }
                    }
                    else
                    {
                        hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
                    }
                }
            }
            else
            {
                RIP("Invalid color type\n");
                hr = E_FAIL;
            }

            if (SUCCEEDED(hr))
            {
                SysFreeString(*pbstrColorRef);
                *pbstrColorRef = cstrChannelData.AllocSysString();
            }
        }
        catch (CXDException& e)
        {
            hr = e;
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CResourceDictionaryConverter::CResourceDictionaryConverter

Routine Description:

    CResourceDictionaryConverter constructir

Arguments:

    pXpsConsumer - Pointer to the XPS consumer interface
    pFixedPage   - Pointer to the fixed page interface
    pResCache    - Pointer to the resource cache
    pProfManager - Pointer to the color profile manager
    pResDel      - Pointer to the list of resources to delete from the page
    pBmpConv     - Pointer to the bitmap conversion class
    pRefConv     - Pointer to the color reference conversion class

Return Value:

    None
    Throws an exception on error.

--*/
CResourceDictionaryConverter::CResourceDictionaryConverter(
    _In_ IXpsDocumentConsumer*  pXpsConsumer,
    _In_ IFixedPage*            pFixedPage,
    _In_ CFileResourceCache*    pResCache,
    _In_ CProfileManager*       pProfManager,
    _In_ ResDeleteMap*          pResDel,
    _In_ CBitmapColorConverter* pBmpConv,
    _In_ CColorRefConverter*    pRefConv
    ) :
    CColorConverter(pXpsConsumer, pFixedPage, pResCache, pProfManager, pResDel),
    m_pBmpConv(pBmpConv),
    m_pRefConv(pRefConv)
{
    if (m_pBmpConv == NULL ||
        m_pRefConv == NULL)
    {
        throw CXDException(E_POINTER);
    }
}

/*++

Routine Name:

    CResourceDictionaryConverter::~CResourceDictionaryConverter

Routine Description:

    CResourceDictionaryConverter destructor

Arguments:

    None

Return Value:

    None

--*/
CResourceDictionaryConverter::~CResourceDictionaryConverter()
{
}

/*++

Routine Name:

    CResourceDictionaryConverter::Convert

Routine Description:

    Applies color conversion on a remote resource dictionary. This is achieved
    by creating a color SAX parser passing a stream based off the resource

Arguments:

    pbstrDictionaryURI - The remote dictionary URI

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CResourceDictionaryConverter::Convert(
    _Inout_ BSTR* pbstrDictionaryURI
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pbstrDictionaryURI, E_POINTER)))
    {
        try
        {
            CStringXDW cstrOriginalURI(*pbstrDictionaryURI);

            //
            // Create a color managed dictionary object and pass to the cache manager
            //
            CRemoteDictionary newDictionary(m_pFixedPage, m_pBmpConv, m_pRefConv, *pbstrDictionaryURI);

            CComBSTR bstrKey;
            if (SUCCEEDED(hr = m_pResCache->WriteResource<IPartResourceDictionary>(m_pXpsConsumer, m_pFixedPage, &newDictionary)) &&
                SUCCEEDED(hr = newDictionary.GetKeyName(&bstrKey)) &&
                SUCCEEDED(hr = m_pResCache->GetURI(bstrKey, pbstrDictionaryURI)))
            {
                (*m_pResDel)[cstrOriginalURI] = TRUE;
            }
        }
        catch (CXDException& e)
        {
            hr = e;
        }
        catch (exception& DBG_ONLY(e))
        {
            ERR(e.what());
            hr = E_FAIL;
        }
    }

    return hr;
}

