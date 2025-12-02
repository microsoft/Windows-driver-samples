//
// FormatHelpers.h -- Copyright (c) Microsoft Corporation
//
// Author: zaneh
//
// Description:
//
//   Format Helpers for test formats used in APODeviceTest
//
#pragma once

#include <MMDeviceAPIp.h>
#include <DeviceTopologyp.h>
#include <mfapi.h>
#include <AVEndpointKeys.h>
#include <AudioCoreAPO32Params.h>
#include <ksmedia.h>

// Consts used for APO format creation
static const int cFrameRate             = 48000; // 48kHz
static const int cProcessingPeriod      = 10;    // milliseconds
static const int cMaxFrameCount         = cFrameRate * cProcessingPeriod / 1000;
static const int cFrameCountToProcess   = cMaxFrameCount / 2;

// Returns true if the two IAudioMediaType formats are equal.
// False otherwise
bool IsEqualFormat
(
    IAudioMediaType      *pIAMTFormat1,
    IAudioMediaType      *pIAMTFormat2
)
{
    UINT                        cbBuffer1, cbBuffer2;
    WAVEFORMATEX const         *pwfx1, *pwfx2;
    UNCOMPRESSEDAUDIOFORMAT     format1, format2;
    
    pwfx1     = pIAMTFormat1->GetAudioFormat();
    pwfx2     = pIAMTFormat2->GetAudioFormat();
    cbBuffer1 = ((WAVE_FORMAT_PCM == pwfx1->wFormatTag)?sizeof(PCMWAVEFORMAT):(sizeof(WAVEFORMATEX) + pwfx1->cbSize));
    cbBuffer2 = ((WAVE_FORMAT_PCM == pwfx2->wFormatTag)?sizeof(PCMWAVEFORMAT):(sizeof(WAVEFORMATEX) + pwfx2->cbSize));
    
    if (cbBuffer1 != cbBuffer2)
    {
        return false;
    }
    
    //  Two formats don't compare as WAVEFORMATEX...
    if (0 != memcmp(pwfx1, pwfx2, cbBuffer1))
    {
        return false;
    }
    
    SecureZeroMemory(&format1, sizeof(format1));
    SecureZeroMemory(&format2, sizeof(format2));
    
    if (S_OK != pIAMTFormat1->GetUncompressedAudioFormat(&format1))
    {
        return false;
    }
    
    if (S_OK != pIAMTFormat2->GetUncompressedAudioFormat(&format2))
    {
        return false;
    }
    
    //  Two formats don't compare as WAVEFORMATEX...
    if (0 != memcmp(&format1, &format2, sizeof(UNCOMPRESSEDAUDIOFORMAT)))
    {
        return false;
    }
    
    return true;
}

// Creates a KSData format from the given WaveFormatEx
// Returns a struct containing both the KSData format and WaveFormatEx
KSDATAFORMAT_WAVEFORMATEX *CreateKSDataFromWFX
(
    WAVEFORMATEX               *pwfx
)
{
    if (nullptr == pwfx)
    {
        return nullptr;
    }

    UINT cbAlloc;    
    cbAlloc = sizeof(KSDATAFORMAT_WAVEFORMATEX) + pwfx->cbSize;
    
    wil::unique_cotaskmem_ptr<KSDATAFORMAT_WAVEFORMATEX> pKsFormat((KSDATAFORMAT_WAVEFORMATEX*)(CoTaskMemAlloc(cbAlloc)));
    
    if (nullptr == pKsFormat)
    {
        return nullptr;
    }
    
    pKsFormat->DataFormat.FormatSize  = cbAlloc;
    pKsFormat->DataFormat.Flags       = 0;
    pKsFormat->DataFormat.Reserved    = 0;
    pKsFormat->DataFormat.MajorFormat = KSDATAFORMAT_TYPE_AUDIO;
    pKsFormat->DataFormat.Specifier   = KSDATAFORMAT_SPECIFIER_WAVEFORMATEX;
    
    CopyMemory(&(pKsFormat->WaveFormatEx), pwfx, sizeof(WAVEFORMATEX) + pwfx->cbSize);
    
    if (WAVE_FORMAT_EXTENSIBLE == pwfx->wFormatTag)
    {
        pKsFormat->DataFormat.SubFormat = ((WAVEFORMATEXTENSIBLE*)(pwfx))->SubFormat;
    }
    else
    {
        INIT_WAVEFORMATEX_GUID(&(pKsFormat->DataFormat.SubFormat), pwfx->wFormatTag);
    }
    
    return (pKsFormat.release());
}

// Creates an IAudioMediaType given an uncompressed audio format
HRESULT AECreateAudioMediaTypeFromUncompressedAudioFormat(const UNCOMPRESSEDAUDIOFORMAT* pUncompressedAudioFormat,IAudioMediaType** ppIAudioMediaType)
{
    RETURN_IF_FAILED(CTestAudioMediaType::Create(pUncompressedAudioFormat, ppIAudioMediaType));

    return S_OK;
}

// Creates an IAudioMediaType given a WaveFormatEx
HRESULT AECreateAudioMediaType(const WAVEFORMATEX* pWfx, IAudioMediaType** ppIAudioMediaType)
{
    UNCOMPRESSEDAUDIOFORMAT   audioFormat;
   
    audioFormat.dwBytesPerSampleContainer = pWfx->nBlockAlign / pWfx->nChannels;
    audioFormat.dwSamplesPerFrame = pWfx->nChannels;
    if (WAVE_FORMAT_EXTENSIBLE == pWfx->wFormatTag)
    {
        audioFormat.dwValidBitsPerSample = ((WAVEFORMATEXTENSIBLE*)pWfx)->Samples.wValidBitsPerSample;
        audioFormat.fFramesPerSecond = (float)(((WAVEFORMATEXTENSIBLE*)pWfx)->Format.nSamplesPerSec);
        audioFormat.guidFormatType = ((WAVEFORMATEXTENSIBLE*)pWfx)->SubFormat;
    }
    else
    {
        audioFormat.dwValidBitsPerSample = pWfx->wBitsPerSample;
        audioFormat.fFramesPerSecond = (float)(pWfx->nSamplesPerSec);
        INIT_WAVEFORMATEX_GUID(&(audioFormat.guidFormatType), pWfx->wFormatTag);
    }

    RETURN_IF_FAILED(CTestAudioMediaType::Create(&audioFormat, ppIAudioMediaType));
    return S_OK;
}

// Fills an APO connection using the provided uncompressed format.
// If pBuffer is NULL, the connection will be created with an APO_CONNECTION_BUFFER_TYPE_ALLOCATED.
// If pBuffer is not null, APO_CONNECTION_BUFFER_TYPE_EXTERNAL will be used.
// Outptr pConnection returns an APO_CONNECTION_DESCRIPTOR
HRESULT FillConnection
(
    UNCOMPRESSEDAUDIOFORMAT* Format,
    UINT_PTR pBuffer,
    UINT32 /* u32ExtraFrameCount */,
    UINT32 u32MaxFrameCount,
    APO_CONNECTION_DESCRIPTOR* pConnection
)
{
    if (Format == nullptr)
    {
        LOG_ERROR(L"FillConnection recieved null format");
        return E_INVALIDARG;
    }
    if (pConnection == nullptr)
    {
        LOG_ERROR(L"FillConnection recieved null APO connection descriptor");
        return E_INVALIDARG;
    }
    
    IAudioMediaType *pFormat;

    // Attempt to create the AudioMediaType using the standard API
    HRESULT createFormatHr = CreateAudioMediaTypeFromUncompressedAudioFormat(
        Format,
        &pFormat);

    if (FAILED(createFormatHr))
    {
        LOG_OUTPUT(L"Standard AudioMediaType creation failed. Attempting creation using TestMediaType");
        RETURN_IF_FAILED(AECreateAudioMediaTypeFromUncompressedAudioFormat(Format, &pFormat));
    }

    // set connection type based on pBuffer
    // this will get re(set) when the actual allocation occurs
    if (NULL != pBuffer)
    {
        pConnection->Type = APO_CONNECTION_BUFFER_TYPE_EXTERNAL;
    }
    else
    {
        pConnection->Type = APO_CONNECTION_BUFFER_TYPE_ALLOCATED;
    }
    
    pConnection->pFormat = pFormat;
    pConnection->pBuffer = pBuffer;
    pConnection->u32MaxFrameCount = u32MaxFrameCount;
    pConnection->u32Signature = APO_CONNECTION_DESCRIPTOR_SIGNATURE;

    return createFormatHr;
}

// Creates a WaveFormatEx format
void FillFormat
(
    WORD FormatType,
    DWORD dwSamplesPerFrame,
    DWORD /* dwBytesPerSampleContainer */,
    DWORD dwValidBitsPerSample,
    FLOAT32 fFramesPerSecond,
    WAVEFORMATEX* pWfx
)
{
    if (pWfx == nullptr)
    {
        LOG_ERROR(L"WaveformatEx is null");
    }

    if (dwSamplesPerFrame >= 0xffff)
    {
        LOG_ERROR(L"SamplesPerFrame too great");
    }

    if (dwValidBitsPerSample >= 0xffff)
    {
        LOG_ERROR(L"ValidBitsPerSample too great");
    }

    pWfx->wFormatTag = FormatType;
    pWfx->nChannels = (WORD)(dwSamplesPerFrame & 0xffff);
    pWfx->wBitsPerSample = (WORD)dwValidBitsPerSample;
    pWfx->nBlockAlign = pWfx->nChannels * (pWfx->wBitsPerSample / 8);
    pWfx->nSamplesPerSec = (DWORD)fFramesPerSecond;
    pWfx->nAvgBytesPerSec = pWfx->nSamplesPerSec * pWfx->nBlockAlign;
    pWfx->cbSize = 0;
}

// Creates an UncompressedAudioFormat 
void FillFormat
(
    GUID FormatType,
    DWORD dwSamplesPerFrame,
    DWORD dwBytesPerSampleContainer,
    DWORD dwValidBitsPerSample,
    FLOAT32 fFramesPerSecond,
    DWORD dwChannelMask,
    UNCOMPRESSEDAUDIOFORMAT* pFormat
)
{
    if (pFormat == nullptr)
    {
        LOG_ERROR(L"WaveformatEx is null");
    }

    if (dwSamplesPerFrame >= 0xffff)
    {
        LOG_ERROR(L"SamplesPerFrame too great");
    }

    if (dwValidBitsPerSample >= 0xffff)
    {
        LOG_ERROR(L"ValidBitsPerSample too great");
    }

    pFormat->guidFormatType = FormatType;
    pFormat->dwSamplesPerFrame = dwSamplesPerFrame;
    pFormat->dwBytesPerSampleContainer = dwBytesPerSampleContainer;
    pFormat->dwValidBitsPerSample = dwValidBitsPerSample;
    pFormat->fFramesPerSecond = fFramesPerSecond;
    pFormat->dwChannelMask = dwChannelMask;
}

HRESULT SetupConnection
(
    APO_CONNECTION_DESCRIPTOR* pInputConnDesc,
    APO_CONNECTION_DESCRIPTOR* pOutputConnDesc,
    APO_CONNECTION_PROPERTY* pInputConnProp,
    APO_CONNECTION_PROPERTY* pOutputConnProp,
    UNCOMPRESSEDAUDIOFORMAT* pDefaultFormat
)
{
    if (pInputConnDesc == nullptr)
    {
        LOG_ERROR(L"SetupConnection recieved null APO connection descriptor for input");
        return E_INVALIDARG;
    }

    if (pOutputConnDesc == nullptr)
    {
        LOG_ERROR(L"SetupConnection recieved null APO connection descriptor for output");
        return E_INVALIDARG;
    }

    if (pInputConnProp == nullptr)
    {
        LOG_ERROR(L"SetupConnection recieved null APO connection property for input");
        return E_INVALIDARG;
    }

    if (pOutputConnProp == nullptr)
    {
        LOG_ERROR(L"SetupConnection recieved null APO connection property for output");
        return E_INVALIDARG;
    }
    
    if (pDefaultFormat == nullptr)
    {
        LOG_ERROR(L"SetupConnection received null format");
        return E_INVALIDARG;
    }

    LOG_RETURN_IF_FAILED(FillConnection(pDefaultFormat, NULL, 0, cMaxFrameCount, pInputConnDesc),
                        L"\tFillConnection for input descriptor failed");

    LOG_RETURN_IF_FAILED(CreateConnection(pInputConnDesc, pInputConnProp),
                        L"\tCreateConnection for input descriptor failed");

    pInputConnProp->u32ValidFrameCount = cFrameCountToProcess;
    pInputConnProp->u32BufferFlags = BUFFER_VALID;

    LOG_RETURN_IF_FAILED(FillConnection(pDefaultFormat, NULL, 0, cMaxFrameCount, pOutputConnDesc),
                        L"\tFillConnection for output descriptor failed");

    LOG_RETURN_IF_FAILED(CreateConnection(pOutputConnDesc, pOutputConnProp),
                        L"\tCreateConnection for output descriptor failed");

    return S_OK;
}