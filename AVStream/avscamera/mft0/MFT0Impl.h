/**************************************************************************

    A/V Stream Camera Sample

    Copyright (c) 2013, Microsoft Corporation.

    File:

        Mft0Impl.h

    Abstract:

        The CSocMft0 class definition - an implementation of the iSocMft0
        interface found in Mft0.idl.

    History:

        created 4/26/2013

**************************************************************************/


#pragma once
#include "Mft0.h"
#include "SampleHelpers.h"
#include "Mft0clsid.h"

#include <wrl.h>

// CSocMft0
#define FaceDetectionDelayMax 2  //frames between emitting facedetection data

ULONG DllAddRef();
ULONG DllRelease();

using namespace Microsoft::WRL;

class CSocMft0:
    public Microsoft::WRL::RuntimeClass<Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::WinRtClassicComMix>,
    ISocMft0,
    IMFTransform,
    IInspectable>
{

public:

    static HRESULT CreateInstance(REFIID iid, void **ppMFT);

    CSocMft0():
        m_bEnableEffects(TRUE),
        m_bEnableVideoStabilization(FALSE),
        m_uiSourceStreamId(0),
        m_uiInternalState(0),
        m_uThumbnailScaleFactor(4)
    {
        InitializeCriticalSection(&m_critSec);
        m_stThumbnailFormat = MFVideoFormat_ARGB32;
        DllAddRef();

    }

    HRESULT FinalConstruct();

public:

    /*ISocMft0*/
    STDMETHOD(SetState)(UINT32 state);
    STDMETHOD(GetState)(UINT32 *pState);

    /*IInspectable*/
    STDMETHOD(GetIids)(
        _Out_ ULONG *iidCount,
        _Outptr_result_buffer_maybenull_(*iidCount)
        IID **iids
    );

    STDMETHOD(GetRuntimeClassName)(
        _Outptr_result_maybenull_ HSTRING *pClassName
    );

    STDMETHOD(GetTrustLevel)(
        _Out_ TrustLevel *trustLevel
    );

    /*IMFTransform*/
    STDMETHOD(GetStreamLimits)(
        _Out_ DWORD *pdwInputMinimum,
        _Out_ DWORD *pdwInputMaximum,
        _Out_ DWORD *pdwOutputMinimum,
        _Out_ DWORD *pdwOutputMaximum
    );

    STDMETHOD(GetStreamCount)(
        _Out_ DWORD *pcInputStreams,
        _Out_ DWORD *pcOutputStreams
    );

    STDMETHOD(GetStreamIDs)(
        DWORD dwInputIDArraySize,
        _Out_writes_(dwInputIDArraySize) DWORD *pdwInputIDs,
        DWORD dwOutputIDArraySize,
        _Out_writes_(dwOutputIDArraySize) DWORD *pdwOutputIDs
    );

    STDMETHOD(GetInputStreamInfo)(
        DWORD dwInputStreamID,
        _Out_ MFT_INPUT_STREAM_INFO *pStreamInfo
    );

    STDMETHOD(GetOutputStreamInfo)(
        DWORD dwOutputStreamID,
        _Out_ MFT_OUTPUT_STREAM_INFO *pStreamInfo
    );

    STDMETHOD(GetAttributes)(
        _Outptr_result_maybenull_ IMFAttributes **pAttributes
    );

    STDMETHOD(GetInputStreamAttributes)(
        DWORD dwInputStreamID,
        _Outptr_result_maybenull_ IMFAttributes **pAttributes
    );

    STDMETHOD(GetOutputStreamAttributes)(
        DWORD dwOutputStreamID,
        _Outptr_result_maybenull_
        IMFAttributes **pAttributes
    );

    STDMETHOD(DeleteInputStream)(
        DWORD dwStreamID
    );

    STDMETHOD(AddInputStreams)(
        DWORD cStreams,
        _In_ DWORD *adwStreamIDs
    );

    STDMETHOD(GetInputAvailableType)(
        DWORD dwInputStreamID,
        DWORD dwTypeIndex,
        _Outptr_result_maybenull_ IMFMediaType **ppType
    );

    STDMETHOD(GetOutputAvailableType)(
        DWORD dwOutputStreamID,
        DWORD dwTypeIndex,
        _Outptr_result_maybenull_ IMFMediaType **ppType
    );

    STDMETHOD(SetInputType)(
        DWORD dwInputStreamID,
        _In_opt_ IMFMediaType *pType,
        DWORD dwFlags);

    STDMETHOD(SetOutputType)(
        DWORD dwOutputStreamID,
        _In_opt_ IMFMediaType *pType,
        DWORD dwFlags);

    STDMETHOD(GetInputCurrentType)(
        DWORD dwInputStreamID,
        _Outptr_result_maybenull_ IMFMediaType **ppType
    );

    STDMETHOD(GetOutputCurrentType)(
        DWORD dwOutputStreamID,
        _Outptr_result_maybenull_ IMFMediaType **ppType
    );

    STDMETHOD(GetInputStatus)(
        DWORD dwInputStreamID,
        _Out_ DWORD *pdwFlags
    );

    STDMETHOD(GetOutputStatus)(
        _Out_ DWORD *pdwFlags
    );

    STDMETHOD(SetOutputBounds)(
        LONGLONG hnsLowerBound,
        LONGLONG hnsUpperBound
    );

    STDMETHOD(ProcessEvent)(
        DWORD dwInputStreamID,
        _In_opt_ IMFMediaEvent *pEvent
    );

    STDMETHOD(ProcessMessage)(
        MFT_MESSAGE_TYPE eMessage,
        ULONG_PTR ulParam
    );

    STDMETHOD(ProcessInput)(
        DWORD dwInputStreamID,
        IMFSample *pSample,
        DWORD dwFlags
    );

    STDMETHOD(ProcessOutput)(
        DWORD dwFlags,
        DWORD cOutputBufferCount,
        MFT_OUTPUT_DATA_BUFFER *pOutputSamples,
        DWORD *pdwStatus
    );

protected:
    virtual ~CSocMft0()
    {
        DeleteCriticalSection(&m_critSec);
        DllRelease();
    }

    STDMETHOD(GetMediaType)(
        _In_ DWORD  dwStreamID,
        _In_ DWORD dwTypeIndex,
        _Outptr_result_maybenull_ IMFMediaType **ppType
    );

    STDMETHOD(IsMediaTypeSupported)(
        _In_ UINT uiStreamId,
        _In_ IMFMediaType *pIMFMediaType,
        _Outptr_result_maybenull_ IMFMediaType **ppIMFMediaTypeFull = NULL
    );

    STDMETHOD(GenerateMFMediaTypeListFromDevice)();

    _Success_(return == 0)
    STDMETHOD(FindMediaIndex)(
        _In_ UINT uiStreamId,
        _In_ IMFMediaType *pIMFMediaType,
        _Out_ UINT *puiMediaIndex
    );

    // HasPendingOutput: Returns TRUE if the MFT is holding an input sample.
    BOOL HasPendingOutput() const
    {
        return m_spSample != NULL;
    }

    // IsValidInputStream: Returns TRUE if dwInputStreamID is a valid input stream identifier.
    BOOL IsValidInputStream(DWORD dwInputStreamID) const
    {
        return dwInputStreamID == 0;
    }

    // IsValidOutputStream: Returns TRUE if dwOutputStreamID is a valid output stream identifier.
    BOOL IsValidOutputStream(DWORD dwOutputStreamID) const
    {
        return dwOutputStreamID == 0;
    }

    HRESULT CreateOutputSample(
        _Outptr_result_maybenull_ IMFSample **ppSample
    );

    HRESULT GetPreviewMediaType(
        _Outptr_result_maybenull_ IMFMediaType **ppType
    );

    HRESULT ProcessMetadata();

    HRESULT ParseMetadata_PreviewAggregation(
        _In_ PKSCAMERA_METADATA_ITEMHEADER pItem,
        _In_ IMFAttributes *pMetaDataAttributes
    );

    HRESULT ParseMetadata_ImageAggregation(
        _In_ PKSCAMERA_METADATA_ITEMHEADER pItem,
        _In_ IMFAttributes *pMetaDataAttributes
    );

    HRESULT ParseMetadata_Histogram(
        _In_ PKSCAMERA_METADATA_ITEMHEADER pItem,
        _In_ IMFAttributes *pMetaDataAttributes
    );

    HRESULT ParseMetadata_FaceDetection(
        _In_ PKSCAMERA_METADATA_ITEMHEADER pItem,
        _In_ IMFAttributes *pMetaDataAttributes
    );

    HRESULT FillBufferLengthFromMediaType(
        _In_ IMFMediaType *pPreviewType,
        _Inout_ IMFMediaBuffer *pBuffer
    );

    STDMETHOD(OnFlush)();

    HRESULT GetVideoStabilizationEnabled();

    HRESULT GetThumbnailResolution();

    HRESULT OnProcessImage(
        _Outptr_result_maybenull_
        IMFSample **ppIMFOutputSample
    );

    CRITICAL_SECTION            m_critSec;

    ComPtr<IMFSample>          m_spSample;                 // Input sample.
    ComPtr<IMFMediaType>       m_spInputType;              // Input media type.
    ComPtr<IMFMediaType>       m_spOutputType;             // Output media type.

    // Image transform function. (Changes based on the media type.)
    ComPtr<IMFAttributes>      m_spInputAttributes;
    ComPtr<IMFAttributes>      m_spGlobalAttributes;
    GUID                        m_stStreamType;
    ComPtr<IMFTransform>       m_spSourceTransform;
    volatile BOOL               m_bEnableEffects;
    BOOL                        m_bEnableVideoStabilization;
    UINT                        m_uiSourceStreamId;
    UINT                        m_uiInternalState;
    BYTE                        m_uThumbnailScaleFactor;
    GUID                        m_stThumbnailFormat;

    std::vector<ComPtr<IMFMediaType>>
                                  m_listOfMediaTypes;

};

inline HRESULT MFT0CreateInstance(REFIID riid, void **ppv)
{
    return CSocMft0::CreateInstance(riid, ppv);
}

//CoCreateableClass(CSocMft0);