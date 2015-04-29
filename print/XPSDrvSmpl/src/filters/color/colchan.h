/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   colchan.h

Abstract:

   Color channel class definition. The color channel class is responsible for maintaining
   simple single color multiple channel data intialised from color references in the XPS markup.
   It provides methods for intialization, access and conversion of the data.

--*/

#pragma once

enum EColorDataType
{
    sRGB = 0,
    scRGB,
    nChannel
};

class CColorChannelData
{
public:
    CColorChannelData();

    ~CColorChannelData();

    template <class _T>
    HRESULT
    AddChannelData(
        _In_ CONST _T& channelValue
        );

    HRESULT
    GetChannelCount(
        _Out_ DWORD* pcChannels
        );

    HRESULT
    GetChannelCountNoAlpha(
        _Out_ DWORD* pcChannels
        );

    HRESULT
    GetChannelType(
        _Out_ COLORDATATYPE* pChannelType
        );

    HRESULT
    ResetChannelType(
        _In_ CONST COLORDATATYPE& channelType
        );

    HRESULT
    GetChannelData(
        _Out_           DWORD* pcbDataSize,
        _Out_
    _When_(*pcbDataSize > 0, _At_(*ppData, _Post_ _Readable_bytes_(*pcbDataSize)))
    _When_(*pcbDataSize == 0, _At_(*ppData, _Post_ _Maybenull_))
                        PVOID* ppData
        );

    HRESULT
    GetChannelDataNoAlpha(
        _Out_           DWORD* pcbDataSize,
        _Out_
        _When_(*pcbDataSize > 0, _At_(*ppData, _Post_ _Readable_bytes_(*pcbDataSize)))
        _When_(*pcbDataSize == 0, _At_(*ppData, _Post_ _Maybenull_))
                        PVOID* ppData
        );

    template <class _T>
    HRESULT
    ClampChannelValues(
        _In_ CONST _T& min,
        _In_ CONST _T& max
        );

    template <class _T>
    HRESULT
    InitializeChannelData(
        _In_ CONST COLORDATATYPE& channelType,
        _In_ CONST EColorDataType& dataType,
        _In_ CONST DWORD&         cChannels,
        _In_ CONST _T&            channelValue
        );

    HRESULT
    SetColorDataType(
        _In_ CONST EColorDataType& dataType
        );

    HRESULT
    GetColorDataType(
        _Out_ EColorDataType* pDataType
        );

    BOOL
    HasAlpha(
        VOID
        );

    HRESULT
    InitializeAlphaChannel(
        _In_ CColorChannelData* pSrcChannelData
        );

    HRESULT
    GetColor(
        _Out_ PCOLOR     pColor,
        _Out_ COLORTYPE* pType
        );

    HRESULT
    SetColor(
        _In_  COLOR*           pColor,
        _In_  CONST COLORTYPE& type
        );

private:
    HRESULT
    GetChannelSizeFromType(
        _In_  CONST COLORDATATYPE& channelType,
        _Out_ DWORD*               pcbChannelSize
        );

    template <class _T>
    HRESULT
    ValidateDataSize(
        VOID
        );

    HRESULT
    AllocateChannelBuffers(
        _Out_                          UINT*  pcbBuffer,
        _Outptr_result_bytebuffer_(*pcbBuffer) _At_buffer_(*ppBuffer, _Iter_, *pcbBuffer, _Post_invalid_)
                                       PBYTE* ppBuffer
        );

    VOID
    FreeChannelBuffers(
        VOID
        );

    HRESULT
    GetAlphaAsFloat(
        _Out_ PFLOAT pAlpha
        );

    HRESULT
    ColorToByte(
        _Out_writes_bytes_(cbDstData) PBYTE      pDstData,
        _In_                    CONST UINT cbDstData
        );

    HRESULT
    ColorToWord(
        _Out_writes_bytes_(cbDstData) PWORD      pDstData,
        _In_                    CONST UINT cbDstData
        );

    HRESULT
    ColorFromByte(
        _In_reads_bytes_(cbSrcData) PBYTE       pSrcData,
        _In_                   CONST UINT& cbSrcData
        );

    HRESULT
    ColorFromWord(
        _In_reads_bytes_(cbSrcData) PWORD       pSrcData,
        _In_                   CONST UINT& cbSrcData
        );

    HRESULT
    GetAlphaChannelSize(
        _Out_ DWORD* pcbAlphaChan
        );

private:
    DWORD          m_cChannels;

    COLORDATATYPE  m_channelType;

    PBYTE          m_pChannelData;

    UINT           m_cbChannelData;

    EColorDataType m_dataType;
};

//
// Explicitly instantiate the template functions for BYTE, WORD and FLOAT
//
template
HRESULT
CColorChannelData::AddChannelData<BYTE>(
    _In_ CONST BYTE& channelValue
    );

template
HRESULT
CColorChannelData::ClampChannelValues<BYTE>(
    _In_ CONST BYTE& min,
    _In_ CONST BYTE& max
    );

template
HRESULT
CColorChannelData::InitializeChannelData<BYTE>(
    _In_ CONST COLORDATATYPE& channelType,
    _In_ CONST EColorDataType& dataType,
    _In_ CONST DWORD&         cChannels,
    _In_ CONST BYTE&          channelValue
    );

template
HRESULT
CColorChannelData::ValidateDataSize<BYTE>(
    VOID
    );

template
HRESULT
CColorChannelData::AddChannelData<WORD>(
    _In_ CONST WORD& channelValue
    );

template
HRESULT
CColorChannelData::ClampChannelValues<WORD>(
    _In_ CONST WORD& min,
    _In_ CONST WORD& max
    );

template
HRESULT
CColorChannelData::InitializeChannelData<WORD>(
    _In_ CONST COLORDATATYPE& channelType,
    _In_ CONST EColorDataType& dataType,
    _In_ CONST DWORD&         cChannels,
    _In_ CONST WORD&          channelValue
    );

template
HRESULT
CColorChannelData::ValidateDataSize<WORD>(
    VOID
    );

template
HRESULT
CColorChannelData::AddChannelData<FLOAT>(
    _In_ CONST FLOAT& channelValue
    );

template
HRESULT
CColorChannelData::ClampChannelValues<FLOAT>(
    _In_ CONST FLOAT& min,
    _In_ CONST FLOAT& max
    );

template
HRESULT
CColorChannelData::InitializeChannelData<FLOAT>(
    _In_ CONST COLORDATATYPE& channelType,
    _In_ CONST EColorDataType& dataType,
    _In_ CONST DWORD&         cChannels,
    _In_ CONST FLOAT&         channelValue
    );

template
HRESULT
CColorChannelData::ValidateDataSize<FLOAT>(
    VOID
    );

