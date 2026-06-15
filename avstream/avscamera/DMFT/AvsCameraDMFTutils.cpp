//*@@@+++@@@@******************************************************************
//
// Microsoft Windows Media Foundation
// Copyright (C) Microsoft Corporation. All rights reserved.
//
//*@@@---@@@@******************************************************************
//

#include "stdafx.h"

#pragma comment(lib, "d2d1") 
#ifdef MF_WPP
#include "AvsCameraDMFTutils.tmh"    //--REF_ANALYZER_DONT_REMOVE--
#endif

// Critical sections

CCritSec::CCritSec()
{
    InitializeCriticalSection(&m_criticalSection);
}

CCritSec::~CCritSec()
{
    DeleteCriticalSection(&m_criticalSection);
}

_Requires_lock_not_held_(m_criticalSection) _Acquires_lock_(m_criticalSection)
void CCritSec::Lock()
{
    EnterCriticalSection(&m_criticalSection);
}

_Requires_lock_held_(m_criticalSection) _Releases_lock_(m_criticalSection)
void CCritSec::Unlock()
{
    LeaveCriticalSection(&m_criticalSection);
}


_Acquires_lock_(this->m_pCriticalSection->m_criticalSection)
CAutoLock::CAutoLock(CCritSec& crit)
{
    m_pCriticalSection = &crit;
    m_pCriticalSection->Lock();
}
_Acquires_lock_(this->m_pCriticalSection->m_criticalSection)
CAutoLock::CAutoLock(CCritSec* crit)
{
    m_pCriticalSection = crit;
    m_pCriticalSection->Lock();
}
_Releases_lock_(this->m_pCriticalSection->m_criticalSection)
CAutoLock::~CAutoLock()
{
    m_pCriticalSection->Unlock();
}

//
//Some utility functions..
/*++
    Description:
        Helper function to return back if the Pin is in stopped stateo or not
--*/
STDMETHODIMP_(BOOL) IsPinStateInActive( _In_ DeviceStreamState state)
{
    if ((state == DeviceStreamState_Disabled) ||
        (state == DeviceStreamState_Stop))
    {
        return TRUE;
    }
    return FALSE;
}


#ifndef IF_EQUAL_RETURN
#define IF_EQUAL_RETURN(param, val) if(val == param) return #val
#endif

#define checkAdjustBufferCap(a,len){\
    char* tStore = NULL; \
if (a && strlen(a) > ((len * 7) / 10)){\
    tStore = a; \
    len *= 2; \
    a = new (std::nothrow) char[len]; \
if (!a){\
goto done;}\
    a[0] = 0; \
    strcat_s(a, len, tStore); \
    delete(tStore); }\
}

//
// Queue implementation
//
CPinQueue::CPinQueue(_In_ DWORD dwPinId, _In_ IMFDeviceTransform* pParent) :
    m_dwInPinId(dwPinId), m_pTransform(pParent), m_cRef(1)

/*
Description
dwPinId is the input pin Id to which this queue corresponds
*/
{
    m_streamCategory = GUID_NULL;
}
CPinQueue::~CPinQueue()
{
}

/*++
Description:
    Insert sample into the list once we reach the open queue
--*/
STDMETHODIMP CPinQueue::Insert(_In_ IMFSample* pSample)
{
    HRESULT hr = ExceptionBoundary([&]() {
        m_sampleList.push_back(pSample);
    });

    if (SUCCEEDED(hr) && m_pTransform)
    {
        hr = reinterpret_cast<CMultipinMft*>(m_pTransform)->QueueEvent(METransformHaveOutput, GUID_NULL, S_OK, NULL);
    }

    if (FAILED(hr))
    {
        DMFTRACE(DMFT_GENERAL, TRACE_LEVEL_INFORMATION, "%!FUNC! exiting %x = %!HRESULT!", hr, hr);
        // There is a bug in the pipeline that doesn't release the sample fed from processinput. We have to explicitly release the sample here
        SAFE_RELEASE(pSample);
    }
    return hr;
}


/*++
Description:
    This extracts the first sample from the queue. called from Pin's ProcessOutput
--*/

STDMETHODIMP CPinQueue::Remove(_Outptr_result_maybenull_ IMFSample** ppSample)
{
    HRESULT hr = S_OK;
    DMFTCHECKNULL_GOTO(ppSample, done, E_INVALIDARG);
    *ppSample = nullptr;

    if (!m_sampleList.empty())
    {
        *ppSample = m_sampleList.front().Detach();
    }

    DMFTCHECKNULL_GOTO(*ppSample, done, MF_E_TRANSFORM_NEED_MORE_INPUT);
    m_sampleList.erase(m_sampleList.begin());
done:
    return hr;
}

/*++
Description:
    Empties the Queue. used by the flush
--*/
VOID CPinQueue::Clear()
{
    while (!Empty())
    {
        ComPtr<IMFSample> spSample;
        Remove(spSample.GetAddressOf());
    }
}

// Handle Metadata

#if (NTDDI_VERSION >= NTDDI_WINBLUE)
/////////////////////////////////////////////////////////////////////
//
// Handle the Metadata with the buffer
//

HRESULT ParseMetadata_FaceDetection(
    _In_ PKSCAMERA_METADATA_ITEMHEADER pItem,
    _In_ IMFAttributes* pMetaDataAttributes
);

HRESULT ParseMetadata_PreviewAggregation(
    _In_ PKSCAMERA_METADATA_ITEMHEADER pItem,
    _In_ IMFAttributes* pMetaDataAttributes
);

HRESULT ParseMetadata_ImageAggregation(
    _In_ PKSCAMERA_METADATA_ITEMHEADER pItem,
    _In_ IMFAttributes* pMetaDataAttributes
);

HRESULT ParseMetadata_Histogram(
    _In_ PKSCAMERA_METADATA_ITEMHEADER pItem,
    _In_ IMFAttributes* pMetaDataAttributes
);

HRESULT ProcessMetadata(_In_ IMFSample* pSample)
{
    ComPtr<IMFAttributes>  spMetadata;
    ComPtr<IMFSample> spSample = pSample;
    HRESULT hr = spSample->GetUnknown(MFSampleExtension_CaptureMetadata, IID_PPV_ARGS(spMetadata.ReleaseAndGetAddressOf()));
    if (FAILED(hr))
    {
        return hr;
    }

    ComPtr<IMFMediaBuffer> spBuffer;
    hr = spMetadata->GetUnknown(MF_CAPTURE_METADATA_FRAME_RAWSTREAM, IID_PPV_ARGS(spBuffer.ReleaseAndGetAddressOf()));
    if (FAILED(hr))
    {
        return hr;
    }

    MediaBufferLock bufferLock(spBuffer.Get());
    BYTE* pData = NULL;
    DWORD dwLength = 0;
    hr = bufferLock.LockBuffer(&pData, NULL, &dwLength);
    if (FAILED(hr))
    {
        return hr;
    }

    // OEM put meta data passing logic here,
    if (FAILED(hr))
    {
        return hr;
    }

    LONG lBufferLeft = static_cast<LONG>(dwLength);
    if (lBufferLeft < sizeof(KSCAMERA_METADATA_ITEMHEADER))
    {
        return E_UNEXPECTED;
    }

    PKSCAMERA_METADATA_ITEMHEADER pItem = (PKSCAMERA_METADATA_ITEMHEADER)pData;

    while (lBufferLeft > 0)
    {
        switch (pItem->MetadataId)
        {
        case MetadataId_Custom_PreviewAggregation:
            hr = ParseMetadata_PreviewAggregation(pItem, spMetadata.Get());
            if (FAILED(hr))
            {
                return hr;
            }
            break;
        case MetadataId_Custom_ImageAggregation:
            hr = ParseMetadata_ImageAggregation(pItem, spMetadata.Get());
            if (FAILED(hr))
            {
                return hr;
            }
            break;
        case MetadataId_Custom_Histogram:
            hr = ParseMetadata_Histogram(pItem, spMetadata.Get());
            if (FAILED(hr))
            {
                return hr;
            }
            break;
        case MetadataId_Custom_FaceDetection:
            hr = ParseMetadata_FaceDetection(pItem, spMetadata.Get());
            if (FAILED(hr))
            {
                return hr;
            }
        }

        if (!pItem->Size)
        {
            // 0 size item will cause the loop to break and
            // we will report buffer malformated
            break;
        }
        lBufferLeft -= (LONG)pItem->Size;
        if (lBufferLeft < sizeof(KSCAMERA_METADATA_ITEMHEADER))
        {
            break;
        }
        pItem = reinterpret_cast<PKSCAMERA_METADATA_ITEMHEADER>
            (reinterpret_cast<PBYTE>(pItem) + pItem->Size);
    }

    if (lBufferLeft != 0)
    {
        //Check and log for malformated data
        return E_UNEXPECTED;
    }

    return S_OK;
}

HRESULT ParseMetadata_PreviewAggregation(
    _In_ PKSCAMERA_METADATA_ITEMHEADER pItem,
    _In_ IMFAttributes* pMetaDataAttributes
)
{
    HRESULT hr = S_OK;
    if (pItem->Size < sizeof(CAMERA_METADATA_PREVIEWAGGREGATION))
    {
        return E_UNEXPECTED;
    }

    PCAMERA_METADATA_PREVIEWAGGREGATION pFixedStruct =
        (PCAMERA_METADATA_PREVIEWAGGREGATION)pItem;

    if (pFixedStruct->Data.FocusState.Set)
    {
        hr = pMetaDataAttributes->SetUINT32(
            MF_CAPTURE_METADATA_FOCUSSTATE,
            pFixedStruct->Data.FocusState.Value);
        if (FAILED(hr))
        {
            return hr;
        }
    }

    if (pFixedStruct->Data.ExposureTime.Set)
    {
        hr = pMetaDataAttributes->SetUINT64(
            MF_CAPTURE_METADATA_EXPOSURE_TIME,
            pFixedStruct->Data.ExposureTime.Value);
        if (FAILED(hr))
        {
            return hr;
        }
    }

    if (pFixedStruct->Data.ISOSpeed.Set)
    {
        hr = pMetaDataAttributes->SetUINT32(
            MF_CAPTURE_METADATA_ISO_SPEED,
            pFixedStruct->Data.ISOSpeed.Value);
        if (FAILED(hr))
        {
            return hr;
        }
    }

    if (pFixedStruct->Data.LensPosition.Set)
    {
        hr = pMetaDataAttributes->SetUINT32(
            MF_CAPTURE_METADATA_LENS_POSITION,
            pFixedStruct->Data.LensPosition.Value);
        if (FAILED(hr))
        {
            return hr;
        }
    }

    if (pFixedStruct->Data.FlashOn.Set)
    {
        hr = pMetaDataAttributes->SetUINT32(
            MF_CAPTURE_METADATA_FLASH,
            pFixedStruct->Data.FlashOn.Value);
        if (FAILED(hr))
        {
            return hr;
        }
    }

    if (pFixedStruct->Data.WhiteBalanceMode.Set)
    {
        hr = pMetaDataAttributes->SetUINT32(
            MF_CAPTURE_METADATA_WHITEBALANCE,
            pFixedStruct->Data.WhiteBalanceMode.Value);
        if (FAILED(hr))
        {
            return hr;
        }
    }

    if (pFixedStruct->Data.EVCompensation.Set)
    {
        CapturedMetadataExposureCompensation EVCompensation;

        EVCompensation.Flags = pFixedStruct->Data.EVCompensation.Flags;
        EVCompensation.Value = pFixedStruct->Data.EVCompensation.Value;

        hr = pMetaDataAttributes->SetBlob(
            MF_CAPTURE_METADATA_EXPOSURE_COMPENSATION,
            (const UINT8*)&EVCompensation,
            sizeof(EVCompensation));
        if (FAILED(hr))
        {
            return hr;
        }
    }

    if (pFixedStruct->Data.SensorFrameRate.Set)
    {
        hr = pMetaDataAttributes->SetUINT64(
            MF_CAPTURE_METADATA_SENSORFRAMERATE,
            pFixedStruct->Data.SensorFrameRate.Value);
        if (FAILED(hr))
        {
            return hr;
        }
    }

    if (pFixedStruct->Data.IsoAnalogGain.Set ||
        pFixedStruct->Data.IsoDigitalGain.Set)
    {
        CapturedMetadataISOGains IsoGains;

        if (pFixedStruct->Data.IsoAnalogGain.Set)
        {
            IsoGains.AnalogGain =
                FLOAT(pFixedStruct->Data.IsoAnalogGain.Numerator) /
                FLOAT(pFixedStruct->Data.IsoAnalogGain.Denominator);
        }
        if (pFixedStruct->Data.IsoDigitalGain.Set)
        {
            IsoGains.DigitalGain =
                FLOAT(pFixedStruct->Data.IsoDigitalGain.Numerator) /
                FLOAT(pFixedStruct->Data.IsoDigitalGain.Denominator);
        }

        hr = pMetaDataAttributes->SetBlob(
            MF_CAPTURE_METADATA_ISO_GAINS,
            (const UINT8*)&IsoGains,
            sizeof(IsoGains));
        if (FAILED(hr))
        {
            return hr;
        }
    }

    if (pFixedStruct->Data.WhiteBalanceGain_R.Set ||
        pFixedStruct->Data.WhiteBalanceGain_G.Set ||
        pFixedStruct->Data.WhiteBalanceGain_B.Set)
    {
        CapturedMetadataWhiteBalanceGains WhiteBalanceGains;

        if (pFixedStruct->Data.WhiteBalanceGain_R.Set)
        {
            WhiteBalanceGains.R =
                FLOAT(pFixedStruct->Data.WhiteBalanceGain_R.Numerator) /
                FLOAT(pFixedStruct->Data.WhiteBalanceGain_R.Denominator);
        }
        if (pFixedStruct->Data.WhiteBalanceGain_G.Set)
        {
            WhiteBalanceGains.G =
                FLOAT(pFixedStruct->Data.WhiteBalanceGain_G.Numerator) /
                FLOAT(pFixedStruct->Data.WhiteBalanceGain_G.Denominator);
        }
        if (pFixedStruct->Data.WhiteBalanceGain_B.Set)
        {
            WhiteBalanceGains.B =
                FLOAT(pFixedStruct->Data.WhiteBalanceGain_B.Numerator) /
                FLOAT(pFixedStruct->Data.WhiteBalanceGain_B.Denominator);
        }

        hr = pMetaDataAttributes->SetBlob(
            MF_CAPTURE_METADATA_WHITEBALANCE_GAINS,
            (const UINT8*)&WhiteBalanceGains,
            sizeof(WhiteBalanceGains));
        if (FAILED(hr))
        {
            return hr;
        }
    }

    return S_OK;
}

HRESULT ParseMetadata_ImageAggregation(
    _In_ PKSCAMERA_METADATA_ITEMHEADER pItem,
    _In_ IMFAttributes* pMetaDataAttributes
)
{
    HRESULT hr = S_OK;
    if (pItem->Size < sizeof(CAMERA_METADATA_IMAGEAGGREGATION))
    {
        return E_UNEXPECTED;
    }

    PCAMERA_METADATA_IMAGEAGGREGATION pFixedStruct =
        (PCAMERA_METADATA_IMAGEAGGREGATION)pItem;

    if (pFixedStruct->Data.FrameId.Set)
    {
        hr = pMetaDataAttributes->SetUINT32(
            MF_CAPTURE_METADATA_REQUESTED_FRAME_SETTING_ID,
            pFixedStruct->Data.FrameId.Value);
        if (FAILED(hr))
        {
            return hr;
        }
    }

    if (pFixedStruct->Data.ExposureTime.Set)
    {
        hr = pMetaDataAttributes->SetUINT64(
            MF_CAPTURE_METADATA_EXPOSURE_TIME,
            pFixedStruct->Data.ExposureTime.Value);
        if (FAILED(hr))
        {
            return hr;
        }
    }

    if (pFixedStruct->Data.ISOSpeed.Set)
    {
        hr = pMetaDataAttributes->SetUINT32(
            MF_CAPTURE_METADATA_ISO_SPEED,
            pFixedStruct->Data.ISOSpeed.Value);
        if (FAILED(hr))
        {
            return hr;
        }
    }

    if (pFixedStruct->Data.LensPosition.Set)
    {
        hr = pMetaDataAttributes->SetUINT32(
            MF_CAPTURE_METADATA_LENS_POSITION,
            pFixedStruct->Data.LensPosition.Value);
        if (FAILED(hr))
        {
            return hr;
        }
    }

    if (pFixedStruct->Data.SceneMode.Set)
    {
        hr = pMetaDataAttributes->SetUINT64(
            MF_CAPTURE_METADATA_SCENE_MODE,
            pFixedStruct->Data.SceneMode.Value);
        if (FAILED(hr))
        {
            return hr;
        }
    }

    if (pFixedStruct->Data.FlashOn.Set)
    {
        hr = pMetaDataAttributes->SetUINT32(
            MF_CAPTURE_METADATA_FLASH,
            pFixedStruct->Data.FlashOn.Value);
        if (FAILED(hr))
        {
            return hr;
        }
    }

    if (pFixedStruct->Data.FlashPower.Set)
    {
        hr = pMetaDataAttributes->SetUINT32(
            MF_CAPTURE_METADATA_FLASH_POWER,
            pFixedStruct->Data.FlashPower.Value);
        if (FAILED(hr))
        {
            return hr;
        }
    }

    if (pFixedStruct->Data.WhiteBalanceMode.Set)
    {
        hr = pMetaDataAttributes->SetUINT32(
            MF_CAPTURE_METADATA_WHITEBALANCE,
            pFixedStruct->Data.WhiteBalanceMode.Value);
        if (FAILED(hr))
        {
            return hr;
        }
    }

    if (pFixedStruct->Data.ZoomFactor.Set)
    {
        hr = pMetaDataAttributes->SetUINT32(
            MF_CAPTURE_METADATA_ZOOMFACTOR,
            pFixedStruct->Data.ZoomFactor.Value);
        if (FAILED(hr))
        {
            return hr;
        }
    }

    if (pFixedStruct->Data.EVCompensation.Set)
    {
        CapturedMetadataExposureCompensation EVCompensation;

        EVCompensation.Flags = pFixedStruct->Data.EVCompensation.Flags;
        EVCompensation.Value = pFixedStruct->Data.EVCompensation.Value;

        hr = pMetaDataAttributes->SetBlob(
            MF_CAPTURE_METADATA_EXPOSURE_COMPENSATION,
            (const UINT8*)&EVCompensation,
            sizeof(EVCompensation));
        if (FAILED(hr))
        {
            return hr;
        }
    }

    if (pFixedStruct->Data.FocusState.Set)
    {
        hr = pMetaDataAttributes->SetUINT32(
            MF_CAPTURE_METADATA_FOCUSSTATE,
            pFixedStruct->Data.FocusState.Value);
        if (FAILED(hr))
        {
            return hr;
        }
    }
    return S_OK;
}
#endif // (NTDDI_VERSION >= NTDDI_WINBLUE)
struct HistogramData
{
    HistogramDataHeader     Header;
    ULONG                   Color[256];

    HistogramData()
    {
        Header.Size = sizeof(*this);
        Header.ChannelMask = 0;
        Header.Linear = 1;
        RtlZeroMemory(Color, sizeof(Color));
    }
};

//  This is the blob we pass back every time.
template <ULONG I>
struct Histogram
{
    HistogramBlobHeader     Blob;
    //  RGB or YUV Histograms.
    HistogramHeader         Header;
    HistogramData           Data[I];

    Histogram(
        _In_    ULONG   Width = 0,
        _In_    ULONG   Height = 0
    )
    {
        Blob.Histograms = 1;
        Blob.Size = sizeof(*this);

        Header.Size = sizeof(Header) + sizeof(Data);
        Header.Bins = 256;
        Header.Grid.Height = Height;
        Header.Grid.Width = Width;
        Header.Grid.Region.top = 0;
        Header.Grid.Region.left = 0;
        Header.Grid.Region.bottom = Height - 1;
        Header.Grid.Region.right = Width - 1;
    }
};

#if (NTDDI_VERSION >= NTDDI_WINBLUE)
#define MF_HISTOGRAM_RGB    (MF_HISTOGRAM_CHANNEL_R | MF_HISTOGRAM_CHANNEL_G  | MF_HISTOGRAM_CHANNEL_B )
#define MF_HISTOGRAM_YCrCb  (MF_HISTOGRAM_CHANNEL_Y | MF_HISTOGRAM_CHANNEL_Cr | MF_HISTOGRAM_CHANNEL_Cb)

HRESULT ParseMetadata_Histogram(
    _In_ PKSCAMERA_METADATA_ITEMHEADER pItem,
    _In_ IMFAttributes* pMetaDataAttributes
)
{
    if (pItem->Size < sizeof(CAMERA_METADATA_HISTOGRAM))
    {
        return E_UNEXPECTED;
    }

    PCAMERA_METADATA_HISTOGRAM pHistogram = (PCAMERA_METADATA_HISTOGRAM)pItem;

    if ((pHistogram->Data.ChannelMask & MF_HISTOGRAM_RGB) == MF_HISTOGRAM_RGB)
    {
        Histogram<4>    Blob(pHistogram->Data.Width, pHistogram->Data.Height);

        Blob.Header.FourCC = pHistogram->Data.FourCC;
        Blob.Header.ChannelMasks = pHistogram->Data.ChannelMask;

        //  For a RGB Histogram, we fake the Y channel by copying the G channel.
        Blob.Data[0].Header.ChannelMask = MF_HISTOGRAM_CHANNEL_Y;
        RtlCopyMemory(Blob.Data[0].Color, pHistogram->Data.P1Data, sizeof(Blob.Data[0].Color));

        //  Now just copy the RGB channels normally.
        Blob.Data[1].Header.ChannelMask = MF_HISTOGRAM_CHANNEL_R;
        RtlCopyMemory(Blob.Data[1].Color, pHistogram->Data.P0Data, sizeof(Blob.Data[1].Color));
        Blob.Data[2].Header.ChannelMask = MF_HISTOGRAM_CHANNEL_G;
        RtlCopyMemory(Blob.Data[2].Color, pHistogram->Data.P1Data, sizeof(Blob.Data[2].Color));
        Blob.Data[3].Header.ChannelMask = MF_HISTOGRAM_CHANNEL_B;
        RtlCopyMemory(Blob.Data[3].Color, pHistogram->Data.P2Data, sizeof(Blob.Data[3].Color));

        return pMetaDataAttributes->SetBlob(
            MF_CAPTURE_METADATA_HISTOGRAM,
            (PBYTE)&Blob,
            sizeof(Blob)
        );
    }
    else if ((pHistogram->Data.ChannelMask & MF_HISTOGRAM_YCrCb) == MF_HISTOGRAM_YCrCb)
    {
        Histogram<3>    Blob(pHistogram->Data.Width, pHistogram->Data.Height);

        Blob.Header.FourCC = pHistogram->Data.FourCC;
        Blob.Header.ChannelMasks = pHistogram->Data.ChannelMask;

        Blob.Data[0].Header.ChannelMask = MF_HISTOGRAM_CHANNEL_Y;
        RtlCopyMemory(Blob.Data[0].Color, pHistogram->Data.P0Data, sizeof(Blob.Data[0].Color));
        Blob.Data[1].Header.ChannelMask = MF_HISTOGRAM_CHANNEL_Cr;
        RtlCopyMemory(Blob.Data[1].Color, pHistogram->Data.P1Data, sizeof(Blob.Data[1].Color));
        Blob.Data[2].Header.ChannelMask = MF_HISTOGRAM_CHANNEL_Cb;
        RtlCopyMemory(Blob.Data[2].Color, pHistogram->Data.P2Data, sizeof(Blob.Data[2].Color));

        //TODO:
        return pMetaDataAttributes->SetBlob(
            MF_CAPTURE_METADATA_HISTOGRAM,
            (PBYTE)&Blob,
            sizeof(Blob)
        );
    }
    return E_UNEXPECTED;
}

HRESULT ParseMetadata_FaceDetection(
    _In_ PKSCAMERA_METADATA_ITEMHEADER pItem,
    _In_ IMFAttributes* pMetaDataAttributes
)
{
    HRESULT hr = S_OK;

    if (pItem->Size < sizeof(CAMERA_METADATA_FACEHEADER))
    {
        return E_UNEXPECTED;
    }

    PCAMERA_METADATA_FACEHEADER pFaceHeader = (PCAMERA_METADATA_FACEHEADER)pItem;

    if (pItem->Size < (sizeof(CAMERA_METADATA_FACEHEADER) + (sizeof(METADATA_FACEDATA) * pFaceHeader->Count)))
    {
        return E_UNEXPECTED;
    }
    PMETADATA_FACEDATA  pFaceData = (PMETADATA_FACEDATA)(pFaceHeader + 1);
    UINT32 cbRectSize = sizeof(FaceRectInfoBlobHeader) + (sizeof(FaceRectInfo) * (pFaceHeader->Count));
    BYTE* pRectBuf = new (std::nothrow) BYTE[cbRectSize];
    if (pRectBuf == NULL)
    {
        return E_OUTOFMEMORY;
    }

    UINT32 cbCharSize = sizeof(FaceCharacterizationBlobHeader) + (sizeof(FaceCharacterization) * (pFaceHeader->Count));
    BYTE* pCharBuf = new (std::nothrow) BYTE[cbCharSize];
    if (pCharBuf == NULL)
    {
        delete[] pRectBuf;
        return E_OUTOFMEMORY;
    }

    FaceRectInfoBlobHeader* pFaceRectHeader = (FaceRectInfoBlobHeader*)pRectBuf;
    pFaceRectHeader->Size = cbRectSize;
    pFaceRectHeader->Count = pFaceHeader->Count;

    FaceCharacterizationBlobHeader* pFaceCharHeader = (FaceCharacterizationBlobHeader*)pCharBuf;
    pFaceCharHeader->Size = cbCharSize;
    pFaceCharHeader->Count = pFaceHeader->Count;

    FaceRectInfo* FaceRegions = (FaceRectInfo*)(pFaceRectHeader + 1);
    FaceCharacterization* FaceChars = (FaceCharacterization*)(pFaceCharHeader + 1);

    for (UINT i = 0; i < pFaceHeader->Count; i++)
    {
        FaceRegions[i].Region = pFaceData[i].Region;
        FaceRegions[i].confidenceLevel = pFaceData[i].confidenceLevel;

        FaceChars[i].BlinkScoreLeft = pFaceData[i].BlinkScoreLeft;
        FaceChars[i].BlinkScoreRight = pFaceData[i].BlinkScoreRight;
        FaceChars[i].FacialExpression = (pFaceData[i].FacialExpression == EXPRESSION_SMILE) ? MF_METADATAFACIALEXPRESSION_SMILE : 0;
        FaceChars[i].FacialExpressionScore = pFaceData[i].FacialExpressionScore;
    }

    hr = pMetaDataAttributes->SetBlob(MF_CAPTURE_METADATA_FACEROIS, pRectBuf, cbRectSize);
    if (FAILED(hr))
    {
        goto done;
    }

    MetadataTimeStamps timestamp;
    timestamp.Flags = MF_METADATATIMESTAMPS_DEVICE;
    timestamp.Device = pFaceHeader->Timestamp;

    hr = pMetaDataAttributes->SetBlob(MF_CAPTURE_METADATA_FACEROITIMESTAMPS, (const UINT8*)&timestamp, sizeof(MetadataTimeStamps));
    if (FAILED(hr))
    {
        goto done;
    }

#if (NTDDI_VERSION >= NTDDI_WINTHRESHOLD)
    //  Include characterization data if any of the associated bits were set.
    if (pFaceHeader->Flags & KSCAMERA_EXTENDEDPROP_FACEDETECTION_ADVANCED_MASK)
    {
        hr = pMetaDataAttributes->SetBlob(MF_CAPTURE_METADATA_FACEROICHARACTERIZATIONS, pCharBuf, cbCharSize);
    }
#endif // (NTDDI_VERSION >= NTDDI_WINTHRESHOLD)

done:
    delete[] pRectBuf;
    delete[] pCharBuf;

    return hr;
}
#endif
