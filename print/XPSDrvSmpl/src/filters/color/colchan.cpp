/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   colchan.cpp

Abstract:

   Color channel class implementation. The color channel class is responsible for maintaining
   simple single color multiple channel data intialised from color references in the XPS markup.
   It provides methods for intialization, access and conversion of the data.

   Note regarding color formats: The CColorChannelData class is only used in the filter for
   processing color references in mark-up. As such we only ever see 8 bit per channel sRGB and
   floating point scRGB and n-channel colors and we only ever need convert to 16 bpc for
   down-level scRGB conversion. If the CColorChannelData is ever used when fixed point input
   or other output types are required then these formats need support added (see E_NOTIMPL
   return values).

--*/

#include "precomp.h"
#include "debug.h"
#include "globals.h"
#include "xdexcept.h"
#include "colchan.h"

#define MAX_CHANNEL_COUNT 9 // 1 alpha channel and 8 color channels
#define MAX_CHANNEL_SIZE  sizeof(FLOAT)

DWORD g_cbChannelType[] = {
    sizeof(BYTE),  // COLOR_BYTE
    sizeof(WORD),  // COLOR_WORD
    sizeof(FLOAT), // COLOR_FLOAT
    sizeof(WORD)   // COLOR_S2DOT13FIXED
};


/*++

Routine Name:

    CColorChannelData::CColorChannelData

Routine Description:

    CColorChannelData default constructor

Arguments:

    None

Return Value:

    None
    Throws an exception on failure.

--*/
CColorChannelData::CColorChannelData() :
    m_cChannels(0),
    m_channelType(COLOR_BYTE),
    m_pChannelData(NULL),
    m_cbChannelData(0),
    m_dataType(sRGB)
{
    if (FAILED(AllocateChannelBuffers(&m_cbChannelData, &m_pChannelData)))
    {
        throw CXDException(E_OUTOFMEMORY);
    }
}

/*++

Routine Name:

    CColorChannelData::~CColorChannelData

Routine Description:

    CColorChannelData destructor

Arguments:

    None

Return Value:

    None

--*/
CColorChannelData::~CColorChannelData()
{
    FreeChannelBuffers();
}

/*++

Routine Name:

    CColorChannelData::AddChannelData

Routine Description:

    Template method for adding channel data. This allows any of the available color
    channel data types to be added to the channel data buffer

Arguments:

    channelValue - Value for the channel to be added

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
template <class _T>
HRESULT
CColorChannelData::AddChannelData(
    _In_ CONST _T& channelValue
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = ValidateDataSize<_T>()))
    {
        if (m_cChannels < MAX_CHANNEL_COUNT)
        {
            _T* pChannelData = reinterpret_cast<_T*>(m_pChannelData);
            pChannelData[m_cChannels] = channelValue;
            m_cChannels++;
        }
        else
        {
            hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CColorChannelData::GetChannelCount

Routine Description:

    Retrieves the current number of channels defining the color.

Arguments:

    pcChannels - Pointer to a variable that recieves the count of channels

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CColorChannelData::GetChannelCount(
    _Out_ DWORD* pcChannels
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pcChannels, E_POINTER)))
    {
        *pcChannels = m_cChannels;
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CColorChannelData::GetChannelCountNoAlpha

Routine Description:

    Retrieves the number of channels less the alpha channel if present

Arguments:

    pcChannels - Pointer to a variable that recieves the count of non-alpha channels

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CColorChannelData::GetChannelCountNoAlpha(
    _Out_ DWORD* pcChannels
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = GetChannelCount(pcChannels)))
    {
        if (HasAlpha())
        {
            (*pcChannels)--;
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CColorChannelData::GetChannelType

Routine Description:

    Retrieves the COLORDATATYPE for the channel data

Arguments:

    pChannelType - Pointer to a COLORDATATYPE variable that recieves the type

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CColorChannelData::GetChannelType(
    _Out_ COLORDATATYPE* pChannelType
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pChannelType, E_POINTER)))
    {
        *pChannelType = m_channelType;
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CColorChannelData::ResetChannelType

Routine Description:

    Resets the underlying channel data type to the requested type

Arguments:

    channelType - The channel data type to reset to

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CColorChannelData::ResetChannelType(
    _In_ CONST COLORDATATYPE& channelType
    )
{
    m_channelType = channelType;
    m_cChannels = 0;

    return S_OK;
}

/*++

Routine Name:

    CColorChannelData::GetChannelData

Routine Description:

    Retrieves the buffer and count of bytes of the channel data

Arguments:

    pcbDataSize - Pointer to variable that recieves the count of bytes in the buffer
    ppData      - Pointer to a BYTE pointer that recieves the buffer

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CColorChannelData::GetChannelData(
    _Out_                   DWORD* pcbDataSize,
    _Out_
    _When_(*pcbDataSize > 0, _At_(*ppData, _Post_ _Readable_bytes_(*pcbDataSize)))
    _When_(*pcbDataSize == 0, _At_(*ppData, _Post_ _Maybenull_))
                            PVOID* ppData
    )
{
    HRESULT hr = S_OK;

    DWORD cbDataTypeSize = 0;
    if (SUCCEEDED(hr = CHECK_POINTER(pcbDataSize, E_POINTER)) &&
        SUCCEEDED(hr = CHECK_POINTER(ppData, E_POINTER)) &&
        SUCCEEDED(hr = GetChannelSizeFromType(m_channelType, &cbDataTypeSize)))
    {
        *ppData = NULL;
        *pcbDataSize = 0;

        if (m_cbChannelData >= m_cChannels * cbDataTypeSize)
        {
            if (m_cChannels > 0)
            {
                *pcbDataSize = m_cChannels * cbDataTypeSize;
                *ppData = m_pChannelData;
            }
        }
        else
        {
            hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CColorChannelData::GetChannelDataNoAlpha

Routine Description:

    Retrieves the buffer and count of bytes of the channel data excluding the
    alpha channel if present

Arguments:

    pcbDataSize - Pointer to variable that recieves the count of bytes in the buffer
    ppData      - Pointer to a BYTE pointer that recieves the buffer

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CColorChannelData::GetChannelDataNoAlpha(
    _Out_                           DWORD* pcbDataSize,
    _Out_
    _When_(*pcbDataSize > 0, _At_(*ppData, _Post_ _Readable_bytes_(*pcbDataSize)))
    _When_(*pcbDataSize == 0, _At_(*ppData, _Post_ _Maybenull_))
                                    PVOID* ppData
    )
{
    HRESULT hr = S_OK;

    DWORD cbData = 0;
    PBYTE pData  = NULL;

    if (SUCCEEDED(hr = CHECK_POINTER(pcbDataSize, E_POINTER)) &&
        SUCCEEDED(hr = CHECK_POINTER(ppData, E_POINTER)) &&
        SUCCEEDED(hr = GetChannelData(&cbData, reinterpret_cast<PVOID*>(&pData))))
    {
        *pcbDataSize = cbData;
        *ppData = pData;

        if (HasAlpha())
        {
            DWORD cbChannelSize = 0;
            if (SUCCEEDED(hr = GetChannelSizeFromType(m_channelType, &cbChannelSize)))
            {
                if (cbData >= cbChannelSize)
                {
                    *pcbDataSize = cbData - cbChannelSize;
                    *ppData = pData + cbChannelSize;
                }
                else
                {
                    hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
                }
            }
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CColorChannelData::ClampChannelValues

Routine Description:

    Template method that recieves the minimum and maximum value for a channel
    and applies this to all channels defining the color

Arguments:

    min - Minimum channel value
    max - Maximum channel value

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
template <class _T>
HRESULT
CColorChannelData::ClampChannelValues(
    _In_ CONST _T& min,
    _In_ CONST _T& max
    )
{
    HRESULT hr = S_OK;

    //
    // Validate the data size against the current type
    //
    if (SUCCEEDED(hr = ValidateDataSize<_T>()))
    {
        _T* pData = reinterpret_cast<_T*>(m_pChannelData);
        for (DWORD cChannel = 0; cChannel < m_cChannels; cChannel++, pData++)
        {
            if (*pData < min)
            {
                *pData = min;
            }
            else if (*pData > max)
            {
                *pData = max;
            }
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CColorChannelData::InitializeChannelData

Routine Description:

    Initializes the channel data according to the type, count of channels and a default value

Arguments:

    channelType  - The required channel data format
    dataType     - The required data type
    cChannels    - The count of channels defining the color
    channelValue - The intial value for all channels

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
template <class _T>
HRESULT
CColorChannelData::InitializeChannelData(
    _In_ CONST COLORDATATYPE&  channelType,
    _In_ CONST EColorDataType& dataType,
    _In_ CONST DWORD&          cChannels,
    _In_ CONST _T&             channelValue
    )
{
    HRESULT hr = S_OK;

    //
    // Validate the data size against the current type
    //
    if (SUCCEEDED(hr = ResetChannelType(channelType)) &&
        SUCCEEDED(hr = ValidateDataSize<_T>()))
    {
        if (cChannels <= MAX_CHANNEL_COUNT)
        {
            m_dataType = dataType;
            m_cChannels = cChannels;
            _T* pChannelData = reinterpret_cast<_T*>(m_pChannelData);

            for (DWORD cChannel = 0; cChannel < m_cChannels; cChannel++, pChannelData++)
            {
                *pChannelData = channelValue;
            }
        }
        else
        {
            hr = E_INVALIDARG;
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CColorChannelData::SetColorDataType

Routine Description:

    Sets the color data type

Arguments:

    dataType - The color data type

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CColorChannelData::SetColorDataType(
    _In_ CONST EColorDataType& dataType
    )
{
    m_dataType = dataType;

    return S_OK;
}

/*++

Routine Name:

    CColorChannelData::GetColorDataType

Routine Description:

    Retrieves the current color data type

Arguments:

    pDataType - Pointer to a variable to recieve the color data type

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CColorChannelData::GetColorDataType(
    _Out_ EColorDataType* pDataType
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pDataType, E_POINTER)))
    {
        *pDataType = m_dataType;
    }

    return S_OK;
}

/*++

Routine Name:

    CColorChannelData::HasAlpha

Routine Description:

    Indicates if the color channel data includes an alpha channel

Arguments:

    None

Return Value:

    TRUE  - An alpha channel is present
    FALSE - There is no alpha channel

--*/
BOOL
CColorChannelData::HasAlpha(
    VOID
    )
{
    //
    // If the source or destination is n-channel, or if either sRGB or scRGB have 4 channels,
    // we have an alpha channel
    //
    return m_dataType == nChannel || m_cChannels == 4;
}

/*++

Routine Name:

    CColorChannelData::GetAlphaChannelSize

Routine Description:

    Retrieves the count of bytes of the alpha channel

Arguments:

    pcbAlphaChannel - pointer to a variable that recieves the count of bytes of the alpha channel

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CColorChannelData::GetAlphaChannelSize(
    _Out_ DWORD* pcbAlphaChan
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pcbAlphaChan, E_POINTER)))
    {
        *pcbAlphaChan = 0;
        if (HasAlpha())
        {
            hr = GetChannelSizeFromType(m_channelType, pcbAlphaChan);
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CColorChannelData::InitializeAlphaChannel

Routine Description:

    Initialize the alpha channel based off a source color channel data object

Arguments:

    pSrcChannelData - Pointer to a source color data channel object

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CColorChannelData::InitializeAlphaChannel(
    _In_ CColorChannelData* pSrcChannelData
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pSrcChannelData, E_POINTER)))
    {
        if (HasAlpha())
        {
            FLOAT alpha = 1.0f;
            if (SUCCEEDED(hr) &&
                pSrcChannelData->HasAlpha() &&
                SUCCEEDED(hr = pSrcChannelData->GetAlphaAsFloat(&alpha)))
            {
                //
                // Ensure alpha lies between 0.0 and 1.0
                //
                if (alpha < 0.0)
                {
                    alpha = 0.0;
                }
                else if (alpha > 1.0)
                {
                    alpha = 1.0;
                }
            }

            switch (m_channelType)
            {
                case COLOR_BYTE:
                {
                    *m_pChannelData = static_cast<BYTE>(alpha * kMaxByteAsFloat);
                }
                break;

                case COLOR_WORD:
                {
                    *reinterpret_cast<WORD*>(m_pChannelData) = static_cast<WORD>(alpha * kMaxWordAsFloat);
                }
                break;

                case COLOR_FLOAT:
                {
                    *reinterpret_cast<FLOAT*>(m_pChannelData) = alpha;
                }
                break;

                case COLOR_S2DOT13FIXED:
                {
                    hr = E_NOTIMPL;
                }
                break;

                default:
                {
                    RIP("Unrecognised channel data format.\n");
                    hr = E_FAIL;
                }
                break;
            }
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CColorChannelData::GetColor

Routine Description:

    Retrieves a COLOR object based of the channel data

Arguments:

    pColor - Pointer to a color structure to be filled in
    pType  - Pointer to storage to accept the color type

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CColorChannelData::GetColor(
    _Out_ PCOLOR     pColor,
    _Out_ COLORTYPE* pType
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pColor, E_POINTER)) &&
        SUCCEEDED(hr = CHECK_POINTER(pType, E_POINTER)))
    {
        if (m_cChannels == 0)
        {
            hr = E_PENDING;
        }
    }

    DWORD cChannels = 0;
    if (SUCCEEDED(hr) &&
        SUCCEEDED(hr = GetChannelCountNoAlpha(&cChannels)))
    {
        if (cChannels <= 4)
        {
            hr = ColorToWord(reinterpret_cast<PWORD>(pColor), static_cast<UINT>(sizeof(COLOR)));
        }
        else if (cChannels <= 8)
        {
            hr = ColorToByte(reinterpret_cast<PBYTE>(pColor), static_cast<UINT>(sizeof(COLOR)));
        }
        else
        {
            hr = E_FAIL;
        }

        if (SUCCEEDED(hr))
        {
            if (m_dataType == sRGB ||
                m_dataType == scRGB)
            {
                *pType = COLOR_RGB;
            }
            else if (m_dataType == nChannel)
            {
                *pType = static_cast<COLORTYPE>(cChannels + 3);
            }
            else
            {
                RIP("Unrecognised data type.\n");

                hr = E_FAIL;
            }
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CColorChannelData::SetColor

Routine Description:

    Sets the channel data based on a COLOR structure

Arguments:

    pColor - Pointer to a color structure to set the channel data from
    type   - The color type described by the color structure

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CColorChannelData::SetColor(
    _In_ COLOR*           pColor,
    _In_ CONST COLORTYPE& type
    )
{
    HRESULT hr = S_OK;

    DWORD cChannels = 0;
    DWORD cbData = 0;
    PBYTE pData = NULL;

    if (SUCCEEDED(hr = CHECK_POINTER(pColor, E_POINTER)) &&
        SUCCEEDED(hr = GetChannelCountNoAlpha(&cChannels)) &&
        SUCCEEDED(hr = GetChannelDataNoAlpha(&cbData, reinterpret_cast<PVOID*>(&pData))))
    {
        DWORD cSrcChannels = 0;

        switch (type)
        {
            case COLOR_RGB:
            {
                if (m_dataType == sRGB ||
                    m_dataType == scRGB)
                {
                    cSrcChannels = 3;
                }
                else
                {
                    hr = HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
                }
            }
            break;

            case COLOR_3_CHANNEL:
            case COLOR_CMYK:
            case COLOR_5_CHANNEL:
            case COLOR_6_CHANNEL:
            case COLOR_7_CHANNEL:
            case COLOR_8_CHANNEL:
            {
                if (m_dataType == nChannel)
                {
                    cSrcChannels = static_cast<DWORD>(type - 3);
                }
                else
                {
                    hr = HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
                }
            }
            break;

            case COLOR_GRAY:
            case COLOR_XYZ:
            case COLOR_Yxy:
            case COLOR_Lab:
            case COLOR_NAMED:
            {
                hr = E_NOTIMPL;
            }
            break;

            default:
            {
                RIP("Unrecognised color type\n");
                hr = E_FAIL;
            }
            break;
        }

        if (SUCCEEDED(hr))
        {
            if (cChannels == cSrcChannels)
            {
                if (cSrcChannels <= 4)
                {
                    hr = ColorFromWord(reinterpret_cast<PWORD>(pColor), sizeof(COLOR));
                }
                else if (cSrcChannels <= 8)
                {
                    hr = ColorFromByte(reinterpret_cast<PBYTE>(pColor), sizeof(COLOR));
                }
                else
                {
                    RIP("Invalid channel count\n");
                    hr = E_FAIL;
                }
            }
            else
            {
                RIP("Miss-match between source channel count and current channel count\n");
                hr = E_FAIL;
            }
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CColorChannelData::ColorToByte

Routine Description:

    Converts the channel data to a 8 bpc COLOR type

Arguments:

    pDstData  - Pointer to the data to be set
    cbDstData - Size of the destination buffer

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CColorChannelData::ColorToByte(
    _Out_writes_bytes_(cbDstData) PBYTE      pDstData,
    _In_                    CONST UINT cbDstData
    )
{
    HRESULT hr = S_OK;

    DWORD cChannels = 0;
    DWORD cbData = 0;
    PBYTE pData = NULL;

    if (SUCCEEDED(hr = CHECK_POINTER(pDstData, E_POINTER)) &&
        SUCCEEDED(hr = GetChannelCountNoAlpha(&cChannels)) &&
        SUCCEEDED(hr = GetChannelDataNoAlpha(&cbData, reinterpret_cast<PVOID*>(&pData))))
    {
        ZeroMemory(pDstData, cbDstData);

        if (cbDstData < cChannels)
        {
            hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
        }
    }

    if (SUCCEEDED(hr) &&
        cbData == 0 &&
        cChannels != 0)
    {
        hr = E_UNEXPECTED;
    }

    if (SUCCEEDED(hr))
    {
        if (cbDstData > 0)
        {
            switch (m_channelType)
            {
                case COLOR_BYTE:
                {
                    if (cbDstData >= cbData)
                    {
                        //
                        // Just copy all channels after alpha into the buffer
                        //
                        CopyMemory(pDstData, pData, cbData);
                    }
                    else
                    {
                        hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
                    }
                }
                break;

                case COLOR_WORD:
                {
                    if (cbDstData >= cChannels * sizeof(BYTE) &&
                        cbData    >= cChannels * sizeof(WORD))
                    {
                        PWORD pSrcData = reinterpret_cast<PWORD>(pData);

                        for (UINT cCurrChan = 0;
                             cCurrChan < cChannels;
                             cCurrChan++)
                        {
                            _Analysis_assume_(cCurrChan < cbDstData);

                            pDstData[cCurrChan] = static_cast<BYTE>(MulDiv(pSrcData[cCurrChan], 0xFF, 0xFFFF));
                        }
                    }
                    else
                    {
                        hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
                    }
                }
                break;

                case COLOR_FLOAT:
                {
                    //
                    // Make sure the float data runs between 0.0 and 1.0 for nChannel and sRGB and is
                    // truncated and scaled as follows for scRGB:
                    //
                    //    1. Truncate the input color to between -2.0 and +2.0
                    //    2. Offset the value by +2.0 to put it into the 0.0 to 4.0 range
                    //    3. Scale the value down by 4.0 to put it into the range 0.0 to 1.0
                    //    4. Set the 16 bit value according to the channel value from 0x00
                    //       for 0.0 to 0xFF for 1.0
                    //
                    if (cbDstData >= cChannels * sizeof(BYTE) &&
                        cbData    >= cChannels * sizeof(FLOAT))
                    {
                        PFLOAT pSrcData = reinterpret_cast<PFLOAT>(pData);

                        if (m_dataType == scRGB)
                        {
                            for (UINT cCurrChan = 0;
                                 cCurrChan < cChannels;
                                 cCurrChan++)
                            {
                                _Analysis_assume_(cCurrChan < cbDstData);

                                FLOAT channelValue = pSrcData[cCurrChan];
                                channelValue = channelValue < -2.0f ? -2.0f : channelValue;
                                channelValue = channelValue >  2.0f ?  2.0f : channelValue;
                                channelValue += 2.0f;
                                channelValue /= 4.0f;
                                pDstData[cCurrChan] = static_cast<BYTE>(channelValue * kMaxByteAsFloat);
                            }
                        }
                        else
                        {
                            if (SUCCEEDED(hr = ClampChannelValues(0.0f, 1.0f)))
                            {
                                for (UINT cCurrChan = 0;
                                     cCurrChan < cChannels;
                                     cCurrChan++)
                                {
                                    _Analysis_assume_(cCurrChan < cbDstData);

                                    pDstData[cCurrChan] = static_cast<BYTE>(pSrcData[cCurrChan] * kMaxByteAsFloat);
                                }
                            }
                        }
                    }
                    else
                    {
                        hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
                    }
                }
                break;

                case COLOR_S2DOT13FIXED:
                {
                    hr = E_NOTIMPL;
                }
                break;

                default:
                {
                    RIP("Unrecognised color type\n");
                    hr = E_FAIL;
                }
                break;
            }
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CColorChannelData::ColorToWord

Routine Description:

    Converts the channel data to a 16 bpc COLOR type

Arguments:

    pDstData  - Pointer to the data to be set
    cbDstData - Size of the destination buffer

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CColorChannelData::ColorToWord(
    _Out_writes_bytes_(cbDstData) PWORD      pDstData,
    _In_                    CONST UINT cbDstData
    )
{
    HRESULT hr = S_OK;

    DWORD cChannels = 0;
    DWORD cbData = 0;
    PBYTE pData = NULL;

    if (SUCCEEDED(hr = CHECK_POINTER(pDstData, E_POINTER)) &&
        SUCCEEDED(hr = GetChannelCountNoAlpha(&cChannels)) &&
        SUCCEEDED(hr = GetChannelDataNoAlpha(&cbData, reinterpret_cast<PVOID*>(&pData))))
    {
        ZeroMemory(pDstData, cbDstData);

        if (cbDstData / sizeof(WORD) < cChannels)
        {
            hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
        }
    }

    if (SUCCEEDED(hr) &&
        cbData == 0 &&
        cChannels != 0)
    {
        hr = E_UNEXPECTED;
    }

    if (SUCCEEDED(hr))
    {
        if (cbDstData > 0)
        {
            switch (m_channelType)
            {
                case COLOR_BYTE:
                {
                    if (cbDstData >= cChannels * sizeof(WORD) &&
                        cbData    >= cChannels * sizeof(BYTE))
                    {
                        for (UINT cCurrChan = 0;
                             cCurrChan < cChannels;
                             cCurrChan++)
                        {
                            _Analysis_assume_((cCurrChan + 1) * sizeof(WORD) <= cbDstData);

                            pDstData[cCurrChan] = static_cast<WORD>(MulDiv(pData[cCurrChan], 0xFFFF, 0xFF));
                        }
                    }
                    else
                    {
                        hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
                    }
                }
                break;

                case COLOR_WORD:
                {
                    if (cbDstData - cbData > 0)
                    {
                        //
                        // Just copy all channels after alpha into the COLOR structure
                        //
                        CopyMemory(pDstData, pData, cbData);
                    }
                    else
                    {
                        hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
                    }
                }
                break;

                case COLOR_FLOAT:
                {
                    //
                    // Make sure the float data runs between 0.0 and 1.0 for nChannel and sRGB and is
                    // truncated and scaled as follows for scRGB:
                    //
                    //    1. Truncate the input color to between -2.0 and +2.0
                    //    2. Offset the value by +2.0 to put it into the 0.0 to 4.0 range
                    //    3. Scale the value by 4.0 to put it into the range 0.0 to 1.0
                    //    4. Set the 16 bit value according to the channel value from 0x0000
                    //       for 0.0 to 0xFFFF for 1.0
                    //
                    if (cbDstData >= cChannels * sizeof(WORD) &&
                        cbData    >= cChannels * sizeof(FLOAT))
                    {
                        PFLOAT pSrcData = reinterpret_cast<PFLOAT>(pData);

                        if (m_dataType == scRGB)
                        {
                            for (UINT cCurrChan = 0;
                                 cCurrChan < cChannels;
                                 cCurrChan++)
                            {
                                _Analysis_assume_((cCurrChan + 1) * sizeof(WORD) <= cbDstData);

                                FLOAT channelValue = pSrcData[cCurrChan];
                                channelValue = channelValue < -2.0f ? -2.0f : channelValue;
                                channelValue = channelValue >  2.0f ?  2.0f : channelValue;
                                channelValue += 2.0f;
                                channelValue /= 4.0f;
                                pDstData[cCurrChan] = static_cast<WORD>(channelValue * kMaxWordAsFloat);
                            }
                        }
                        else
                        {
                            if (SUCCEEDED(hr = ClampChannelValues(0.0f, 1.0f)))
                            {
                                for (UINT cCurrChan = 0;
                                     cCurrChan < cChannels;
                                     cCurrChan++)
                                {
                                    _Analysis_assume_((cCurrChan + 1) * sizeof(WORD) <= cbDstData);

                                    pDstData[cCurrChan] = static_cast<WORD>(pSrcData[cCurrChan] * kMaxWordAsFloat);
                                }
                            }
                        }
                    }
                    else
                    {
                        hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
                    }
                }
                break;

                case COLOR_S2DOT13FIXED:
                {
                    hr = E_NOTIMPL;
                }
                break;

                default:
                {
                    RIP("Unrecognised color type\n");
                    hr = E_FAIL;
                }
                break;
            }
        }
    }

    ERR_ON_HR(hr);
    return hr;
}


/*++

Routine Name:

    CColorChannelData::ColorFromByte

Routine Description:

    Uses an 8 bpc COLOR structure to set the channel data

Arguments:

    pSrcData  - Pointer to the source COLOR data
    cbSrcData - Size of the source buffer in bytes

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CColorChannelData::ColorFromByte(
    _In_reads_bytes_(cbSrcData) PBYTE       pSrcData,
    _In_                   CONST UINT& cbSrcData
    )
{
    HRESULT hr = S_OK;

    DWORD cChannels = 0;
    DWORD cbData = 0;
    PBYTE pData = NULL;

    if (SUCCEEDED(hr = CHECK_POINTER(pSrcData, E_POINTER)) &&
        SUCCEEDED(hr = GetChannelCountNoAlpha(&cChannels)) &&
        SUCCEEDED(hr = GetChannelDataNoAlpha(&cbData, reinterpret_cast<PVOID*>(&pData))))
    {
        if (cbData == 0 &&
            cChannels != 0)
        {
            hr = E_UNEXPECTED;
        }
    }

    if (SUCCEEDED(hr))
    {
        ZeroMemory(pData, cbData);

        if (cbSrcData > 0)
        {
            switch (m_channelType)
            {
                case COLOR_BYTE:
                {
                    if (cbData    >= cChannels * sizeof(BYTE) &&
                        cbSrcData >= cChannels * sizeof(BYTE))
                    {
                        //
                        // Just copy src to dst
                        //
                        CopyMemory(pData, pSrcData, cChannels * sizeof(BYTE));
                    }
                    else
                    {
                        hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
                    }
                }
                break;

                case COLOR_WORD:
                {
                    if (cbData    >= cChannels * sizeof(WORD) &&
                        cbSrcData >= cChannels * sizeof(BYTE))
                    {
                        PWORD pDstData = reinterpret_cast<PWORD>(pData);

                        for (UINT cCurrChan = 0;
                             cCurrChan < cChannels;
                             cCurrChan++)
                        {
                            _Analysis_assume_(cCurrChan < cbSrcData);

                            pDstData[cCurrChan] = static_cast<WORD>(MulDiv(pSrcData[cCurrChan], 0xFFFF, 0xFF));
                        }
                    }
                    else
                    {
                        hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
                    }
                }
                break;

                case COLOR_FLOAT:
                {
                    //
                    // Make sure the float data is scaled to 0.0 and 1.0 for nChannel and sRGB and is
                    // scaled as follows for scRGB:
                    //
                    //    1. Convert the WORD value from 0x00 - 0xFF to 0.0f - 1.0f
                    //    1. Scale the value up by 4.0 to put it into the range 0.0 to 2.0
                    //    2. Offset the value by -2.0 to put it into the -2.0 to 2.0 range
                    //
                    if (cbData    >= cChannels * sizeof(FLOAT) &&
                        cbSrcData >= cChannels * sizeof(BYTE))
                    {
                        PFLOAT pDstData = reinterpret_cast<PFLOAT>(pData);

                        if (m_dataType == scRGB)
                        {
                            for (UINT cCurrChan = 0;
                                 cCurrChan < cChannels;
                                 cCurrChan++)
                            {
                                _Analysis_assume_(cCurrChan < cbSrcData);

                                pDstData[cCurrChan] = ((static_cast<FLOAT>(pSrcData[cCurrChan])*4.0f)/kMaxByteAsFloat) - 2.0f;
                            }
                        }
                        else
                        {
                            for (UINT cCurrChan = 0;
                                 cCurrChan < cChannels;
                                 cCurrChan++)
                            {
                                _Analysis_assume_(cCurrChan < cbSrcData);

                                pDstData[cCurrChan] = static_cast<FLOAT>(pSrcData[cCurrChan])/kMaxByteAsFloat;
                            }
                        }
                    }
                    else
                    {
                        hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
                    }
                }
                break;

                case COLOR_S2DOT13FIXED:
                {
                    hr = E_NOTIMPL;
                }
                break;

                default:
                {
                    RIP("Unrecognised color type\n");
                    hr = E_FAIL;
                }
                break;
            }
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CColorChannelData::ColorFromWord

Routine Description:

    Uses an 16 bpc COLOR structure to set the channel data

Arguments:

    pSrcData  - Pointer to the source COLOR data
    cbSrcData - Size of the source buffer in bytes

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CColorChannelData::ColorFromWord(
    _In_reads_bytes_(cbSrcData) PWORD       pSrcData,
    _In_                   CONST UINT& cbSrcData
    )
{
    HRESULT hr = S_OK;

    DWORD cChannels = 0;
    DWORD cbData = 0;
    PBYTE pData = NULL;

    if (SUCCEEDED(hr = CHECK_POINTER(pSrcData, E_POINTER)) &&
        SUCCEEDED(hr = GetChannelCountNoAlpha(&cChannels)) &&
        SUCCEEDED(hr = GetChannelDataNoAlpha(&cbData, reinterpret_cast<PVOID*>(&pData))))
    {
        if (cbData == 0 &&
            cChannels != 0)
        {
            hr = E_UNEXPECTED;
        }
    }

    if (SUCCEEDED(hr))
    {
        ZeroMemory(pData, cbData);

        if (cbSrcData > 0)
        {
            switch (m_channelType)
            {
                case COLOR_BYTE:
                {
                    if (cbData    >= cChannels * sizeof(BYTE) &&
                        cbSrcData >= cChannels * sizeof(WORD))
                    {
                        for (UINT cCurrChan = 0;
                             cCurrChan < cChannels;
                             cCurrChan++)
                        {
                            _Analysis_assume_((cCurrChan + 1) * sizeof(WORD) < cbSrcData);

                            pData[cCurrChan] = static_cast<BYTE>(MulDiv(pSrcData[cCurrChan], 0xFF, 0xFFFF));
                        }
                    }
                    else
                    {
                        hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
                    }
                }
                break;

                case COLOR_WORD:
                {
                    if (cbData    >= cChannels * sizeof(WORD) &&
                        cbSrcData >= cChannels * sizeof(WORD))
                    {
                        //
                        // Just copy src to dst
                        //
                        CopyMemory(pData, pSrcData, cChannels * sizeof(WORD));
                    }
                    else
                    {
                        hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
                    }
                }
                break;

                case COLOR_FLOAT:
                {
                    //
                    // Make sure the float data is scaled to 0.0 and 1.0 for nChannel and sRGB and is
                    // scaled as follows for scRGB:
                    //
                    //    1. Convert the WORD value from 0x0000 - 0xFFFF to 0.0f - 1.0f
                    //    1. Scale the value up by 4.0 to put it into the range 0.0 to 2.0
                    //    2. Offset the value by -2.0 to put it into the -2.0 to 2.0 range
                    //
                    if (cbData    >= cChannels * sizeof(FLOAT) &&
                        cbSrcData >= cChannels * sizeof(WORD))
                    {
                        PFLOAT pDstData = reinterpret_cast<PFLOAT>(pData);

                        if (m_dataType == scRGB)
                        {
                            for (UINT cCurrChan = 0;
                                 cCurrChan < cChannels;
                                 cCurrChan++)
                            {
                                _Analysis_assume_((cCurrChan + 1) * sizeof(WORD) < cbSrcData);

                                pDstData[cCurrChan] = ((static_cast<FLOAT>(pSrcData[cCurrChan])*4.0f)/kMaxWordAsFloat) - 2.0f;
                            }
                        }
                        else
                        {
                            for (UINT cCurrChan = 0;
                                 cCurrChan < cChannels;
                                 cCurrChan++)
                            {
                                _Analysis_assume_((cCurrChan + 1) * sizeof(WORD) < cbSrcData);

                                pDstData[cCurrChan] = static_cast<FLOAT>(pSrcData[cCurrChan])/kMaxWordAsFloat;
                            }
                        }
                    }
                    else
                    {
                        hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
                    }
                }
                break;

                case COLOR_S2DOT13FIXED:
                {
                    hr = E_NOTIMPL;
                }
                break;

                default:
                {
                    RIP("Unrecognised color type\n");
                    hr = E_FAIL;
                }
                break;
            }
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CColorChannelData::GetChannelSizeFromType

Routine Description:

    Retrieves the channel data size from the channel data type

Arguments:

    channelType    - The color channel data type
    pcbChannelSize - Pointer to storage to recieve the channel data size

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CColorChannelData::GetChannelSizeFromType(
    _In_  CONST COLORDATATYPE& channelType,
    _Out_ DWORD*               pcbChannelSize
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pcbChannelSize, E_POINTER)))
    {
        if (channelType < COLOR_BYTE ||
            channelType > COLOR_S2DOT13FIXED)
        {
            hr = E_INVALIDARG;
        }
    }

    if (SUCCEEDED(hr))
    {
        *pcbChannelSize = g_cbChannelType[channelType - 1];
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CColorChannelData::ValidateDataSize

Routine Description:

    Template method that validates the data size given the current channel data type

Arguments:

    None

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
template <class _T>
HRESULT
CColorChannelData::ValidateDataSize(
    VOID
    )
{
    HRESULT hr = S_OK;

    DWORD cbDataSize = 0;
    if (SUCCEEDED(hr = GetChannelSizeFromType(m_channelType, &cbDataSize)))
    {
        //
        // Make sure the data being added matches the data type in size
        //
        if (sizeof(_T) != cbDataSize)
        {
            hr = E_INVALIDARG;
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CColorChannelData::AllocateChannelBuffers

Routine Description:

    Allocates the channel data buffer

Arguments:

    pcbBuffer - Pointer to variable that recieves the allocated buffer size
    ppBuffer  - Pointer to a pointer that recieves the buffer address

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CColorChannelData::AllocateChannelBuffers(
    _Out_                          UINT*  pcbBuffer,
    _Outptr_result_bytebuffer_(*pcbBuffer) _At_buffer_(*ppBuffer, _Iter_, *pcbBuffer, _Post_invalid_)
                                    PBYTE* ppBuffer
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pcbBuffer, E_POINTER)) &&
        SUCCEEDED(hr = CHECK_POINTER(ppBuffer, E_POINTER)))
    {
        //
        // Allocate a buffer for the channel data MAX_CHANNEL_COUNT x MAX_CHANNEL_SIZE
        // This guarantees we have enough storage for the maximum n-channel support
        // in WCS
        //
        *pcbBuffer = MAX_CHANNEL_COUNT*MAX_CHANNEL_SIZE;
        *ppBuffer = new(std::nothrow) BYTE[*pcbBuffer];

        if (*ppBuffer == NULL)
        {
            *pcbBuffer = 0;
            hr = E_OUTOFMEMORY;
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CColorChannelData::FreeChannelBuffers

Routine Description:

    Releases channel buffer

Arguments:

    None

Return Value:

    None

--*/
VOID
CColorChannelData::FreeChannelBuffers(
    VOID
    )
{
    if (m_pChannelData != NULL)
    {
        delete[] m_pChannelData;
        m_pChannelData = NULL;
    }
    m_cbChannelData = 0;
}

/*++

Routine Name:

    CColorChannelData::GetAlphaAsFloat

Routine Description:

    Retrieves the alpha value of the channel data as a float

Arguments:

    pAlpha - Pointer to a float that recieves the alpha value

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CColorChannelData::GetAlphaAsFloat(
    _Out_ PFLOAT pAlpha
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pAlpha, E_POINTER)))
    {
        *pAlpha = 0.0f;

        if (HasAlpha())
        {
            switch (m_channelType)
            {
                case COLOR_BYTE:
                {
                    *pAlpha = static_cast<FLOAT>(*m_pChannelData)/kMaxByteAsFloat;
                }
                break;

                case COLOR_WORD:
                {
                    *pAlpha = static_cast<FLOAT>(*reinterpret_cast<WORD*>(m_pChannelData))/kMaxWordAsFloat;
                }
                break;

                case COLOR_FLOAT:
                {
                    *pAlpha = *reinterpret_cast<PFLOAT>(m_pChannelData);
                }
                break;

                case COLOR_S2DOT13FIXED:
                {
                    hr = E_NOTIMPL;
                }
                break;

                default:
                {
                    RIP("Unrecognised channel data format.\n");
                    hr = E_FAIL;
                }
                break;
            }
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

