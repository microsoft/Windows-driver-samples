//
//    Copyright (C) Microsoft.  All rights reserved.
//
#include "stdafx.h"

#ifdef MF_WPP
#include "basepin.tmh"    //--REF_ANALYZER_DONT_REMOVE--
#endif
/*    -------> New STATE
      |
      |old State
DeviceStreamState_Stop DeviceStreamState_Pause DeviceStreamState_Run DeviceStreamState_Disabled
DeviceStreamState_Pause
DeviceStreamState_Run
DeviceStreamState_Disabled
*/

DeviceStreamState pinStateTransition[4][4] = {
    { DeviceStreamState_Stop, DeviceStreamState_Pause, DeviceStreamState_Run, DeviceStreamState_Disabled },
    { DeviceStreamState_Stop, DeviceStreamState_Pause, DeviceStreamState_Run, DeviceStreamState_Disabled },
    { DeviceStreamState_Stop, DeviceStreamState_Pause, DeviceStreamState_Run, DeviceStreamState_Disabled },
    { DeviceStreamState_Disabled, DeviceStreamState_Disabled, DeviceStreamState_Disabled, DeviceStreamState_Disabled }
};

CBasePin::CBasePin( _In_ ULONG id, _In_ CMultipinMft *parent) :
    m_StreamId(id)
    , m_Parent(parent)
    , m_setMediaType(nullptr)
    , m_nRefCount(0)
    , m_state(DeviceStreamState_Stop)
    , m_dwWorkQueueId(MFASYNC_CALLBACK_QUEUE_UNDEFINED)
{
    
}

CBasePin::~CBasePin()
{
    m_listOfMediaTypes.clear();
    m_spAttributes = nullptr;
}

IFACEMETHODIMP_(DeviceStreamState) CBasePin::GetState()
{
    return (DeviceStreamState) InterlockedCompareExchange((PLONG)&m_state, 0L,0L);
}

IFACEMETHODIMP_(DeviceStreamState) CBasePin::SetState(_In_ DeviceStreamState state)
{
    return (DeviceStreamState) InterlockedExchange((LONG*)&m_state, state);
}

HRESULT CBasePin::AddMediaType( _Inout_ DWORD *pos, _In_ IMFMediaType *pMediaType)
{
    HRESULT hr = S_OK;
    CAutoLock Lock(lock());

    DMFTCHECKNULL_GOTO(pMediaType, done, E_INVALIDARG);
    hr = ExceptionBoundary([&]()
    {
        m_listOfMediaTypes.push_back(pMediaType);
    });
    DMFTCHECKHR_GOTO(hr, done);
    if (pos)
    {
        *pos = (DWORD)(m_listOfMediaTypes.size() - 1);
    }

done:
    return hr;
}

HRESULT CBasePin::GetMediaTypeAt( _In_ DWORD pos, _Outptr_result_maybenull_ IMFMediaType **ppMediaType )
{
    HRESULT hr = S_OK;
    CAutoLock Lock(lock());
    ComPtr<IMFMediaType> spMediaType;
    DMFTCHECKNULL_GOTO(ppMediaType,done,E_INVALIDARG);
    *ppMediaType = nullptr;
    if (pos >= m_listOfMediaTypes.size())
    {
        DMFTCHECKHR_GOTO(MF_E_NO_MORE_TYPES,done);
    }
    spMediaType = m_listOfMediaTypes[pos];
    *ppMediaType = spMediaType.Detach();
done:
    return hr;
}

IFACEMETHODIMP_(BOOL) CBasePin::IsMediaTypeSupported
(
    _In_ IMFMediaType *pMediaType, 
    _When_(ppIMFMediaTypeFull != nullptr, _Outptr_result_maybenull_)
     IMFMediaType **ppIMFMediaTypeFull
)
{
    HRESULT hr = S_OK;
    BOOL bFound = FALSE;
    CAutoLock Lock(lock());
    DMFTCHECKNULL_GOTO(pMediaType,done,E_INVALIDARG);
    if (ppIMFMediaTypeFull)
    {
        *ppIMFMediaTypeFull = nullptr;
    }

    for (UINT uIIndex = 0, uISize = (UINT)m_listOfMediaTypes.size(); uIIndex < uISize ; uIIndex++ )
    {
        DWORD dwResult = 0;
        hr = m_listOfMediaTypes[ uIIndex ]->IsEqual( pMediaType, &dwResult );
        if (hr == S_FALSE)
        {

            if ((dwResult & MF_MEDIATYPE_EQUAL_MAJOR_TYPES) &&
                (dwResult& MF_MEDIATYPE_EQUAL_FORMAT_TYPES) &&
                (dwResult& MF_MEDIATYPE_EQUAL_FORMAT_DATA))
            {
                hr = S_OK;
            }
        }
        if (hr == S_OK)
        {
            bFound = TRUE;
            if (ppIMFMediaTypeFull) {
                DMFTCHECKHR_GOTO(m_listOfMediaTypes[uIIndex].CopyTo(ppIMFMediaTypeFull), done);
            }
            break;
        }
        else if (FAILED(hr))
        {
            DMFTCHECKHR_GOTO(hr,done);
        }
    }
done:
    return SUCCEEDED(hr) ? TRUE : FALSE;
}

IFACEMETHODIMP CBasePin::GetOutputAvailableType( 
    _In_ DWORD dwTypeIndex,
    _Out_opt_ IMFMediaType** ppType)
{
    return GetMediaTypeAt( dwTypeIndex, ppType );
}

HRESULT CBasePin::QueryInterface(
    _In_ REFIID iid,
    _Outptr_result_maybenull_ void** ppv
    )
{
    HRESULT hr = S_OK;

    DMFTCHECKNULL_GOTO(ppv, done, E_POINTER);
    *ppv = nullptr;
    if ( iid == __uuidof( IUnknown ) )
    {
        *ppv = static_cast<VOID*>(this);
    }
    else if ( iid == __uuidof( IMFAttributes ) )
    {
        *ppv = static_cast< IMFAttributes* >( this );
    }
    else if ( iid == __uuidof( IKsControl ) )
    {
        *ppv = static_cast< IKsControl* >( this );
    }
    else
    {
        hr = E_NOINTERFACE;
        goto done;
    }
    AddRef();
done:
    return hr;
}

VOID CBasePin::SetD3DManager(_In_opt_ IUnknown* pManager)
{
    //
    // Should release the old dxgi manager.. We will not invalidate the pins or allocators
    // We will recreate all allocator when the media types are set, so we should be fine
    // And the pipeline will not set the dxgimanager when the pipeline is already built
    //
    CAutoLock Lock(lock());
    m_spDxgiManager = pManager;
    DMFTRACE(DMFT_GENERAL, TRACE_LEVEL_INFORMATION, "%!FUNC! Setting D3DManager on pin %d", m_StreamId);
}
//
//Input Pin implementation
//
CInPin::CInPin(
    _In_opt_ IMFAttributes *pAttributes,
    _In_ ULONG ulPinId,
    _In_ CMultipinMft *pParent)
    :
    CBasePin(ulPinId, pParent),
    m_stStreamType(GUID_NULL),
    m_waitInputMediaTypeWaiter(NULL),
    m_preferredStreamState(DeviceStreamState_Stop)
{
    setAttributes(pAttributes);
}

CInPin::~CInPin()
{
    setAttributes( nullptr );
    m_spSourceTransform = nullptr;

    if (m_waitInputMediaTypeWaiter)
    {
        CloseHandle(m_waitInputMediaTypeWaiter);
    }
}

IFACEMETHODIMP CInPin::Init( 
    _In_ IMFDeviceTransform* pTransform
    )
{
    
    HRESULT hr = S_OK;
    
    DMFTCHECKNULL_GOTO( pTransform, done, E_INVALIDARG );

    m_spSourceTransform = pTransform;

    DMFTCHECKHR_GOTO( GetGUID( MF_DEVICESTREAM_STREAM_CATEGORY, &m_stStreamType ), done );

    //
    //Get the DevProxy IKSControl.. used to send the KSControls or the device control IOCTLS over to devproxy and finally on to the driver!!!!
    //
    DMFTCHECKHR_GOTO( m_spAttributes.As( &m_spIkscontrol ), done );

    m_waitInputMediaTypeWaiter = CreateEvent( NULL,
        FALSE,
        FALSE,
        nullptr
        );
    DMFTCHECKNULL_GOTO( m_waitInputMediaTypeWaiter, done, E_OUTOFMEMORY );

    DMFTCHECKHR_GOTO( GenerateMFMediaTypeListFromDevice(streamId()),done );

done:
    if ( FAILED(hr) )
    {
        m_spSourceTransform = nullptr;

        if ( m_waitInputMediaTypeWaiter )
        {
            CloseHandle( m_waitInputMediaTypeWaiter );
            m_waitInputMediaTypeWaiter = NULL;
        }

        m_stStreamType = GUID_NULL;
    }

    return hr;
}

HRESULT CInPin::GenerateMFMediaTypeListFromDevice(
    _In_ UINT uiStreamId
    )
{
    HRESULT hr = S_OK;
    GUID stSubType = { 0 };
    //This is only called in the begining when the input pin is constructed
    DMFTCHECKNULL_GOTO( m_spSourceTransform, done, MF_E_TRANSFORM_TYPE_NOT_SET );
    for (UINT iMediaType = 0; SUCCEEDED(hr) ; iMediaType++)
    {
        ComPtr<IMFMediaType> spMediaType;
        DWORD pos = 0;

        hr = m_spSourceTransform->GetOutputAvailableType(uiStreamId, iMediaType, spMediaType.GetAddressOf());
        if (hr != S_OK)
            break;
     
        DMFTCHECKHR_GOTO(AddMediaType(&pos, spMediaType.Get()), done);
    }
done:
    if (hr == MF_E_NO_MORE_TYPES) {
        hr = S_OK;
    }
    return hr;
}

IFACEMETHODIMP CInPin::SendSample(
    _In_ IMFSample *pSample
    )
{
    HRESULT hr = S_OK;
    CAutoLock Lock(lock());
    if (FAILED(Active()))
    {
        goto done;
    }
    COutPin *poPin = static_cast<COutPin*>(m_outpin.Get());
    DMFTCHECKNULL_GOTO(pSample, done, S_OK);
    DMFTCHECKHR_GOTO(poPin->AddSample(pSample, this), done);
    
 done: 
    return hr;
}

IFACEMETHODIMP_(VOID) CInPin::ConnectPin( _In_ CBasePin * poPin )
{
    CAutoLock Lock(lock());
    if (poPin!=nullptr)
    {
        m_outpin = poPin;
    }
}

IFACEMETHODIMP CInPin::WaitForSetInputPinMediaChange()
{
    DWORD   dwWait  = 0;
    HRESULT hr      = S_OK;

    dwWait = WaitForSingleObject( m_waitInputMediaTypeWaiter, INFINITE );
    
    if ( dwWait != WAIT_OBJECT_0 )
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto done;
    }
done:
    DMFTRACE(DMFT_GENERAL, TRACE_LEVEL_INFORMATION, "%!FUNC! exiting %x = %!HRESULT!", hr, hr);
    return hr;
}

HRESULT CInPin::GetInputStreamPreferredState(
    _Inout_               DeviceStreamState*  value,
    _Outptr_opt_result_maybenull_  IMFMediaType**      ppMediaType
    )
{
    HRESULT hr = S_OK;
    CAutoLock Lock(lock());

    if (value!=nullptr)
    {
        *value = m_preferredStreamState;
    }

    if (ppMediaType )
    {
        *ppMediaType = nullptr;
        if (m_spPrefferedMediaType != nullptr )
        {
            m_spPrefferedMediaType.CopyTo(ppMediaType);
        }
    }

    DMFTRACE(DMFT_GENERAL, TRACE_LEVEL_INFORMATION, "%!FUNC! exiting %x = %!HRESULT!", hr, hr);
    return hr;
}

HRESULT CInPin::SetInputStreamState(
    _In_ IMFMediaType*      pMediaType,
    _In_ DeviceStreamState  value,
    _In_ DWORD              dwFlags
    )
{
    HRESULT hr = S_OK;
    UNREFERENCED_PARAMETER(dwFlags);

    CAutoLock Lock(lock());
    //
    //Set the media type
    //
    setMediaType(pMediaType);
    SetState(value);
    //
    //Set the event. This event is being waited by an output media/state change operation
    //
    m_spPrefferedMediaType = nullptr;
    SetEvent(m_waitInputMediaTypeWaiter);
    DMFTRACE(DMFT_GENERAL, TRACE_LEVEL_INFORMATION, "%!FUNC! exiting %x = %!HRESULT!", hr, hr);
    return hr;
}

IFACEMETHODIMP_(VOID) CInPin::ShutdownPin()
{
    m_spSourceTransform = nullptr;
    m_outpin = nullptr;
}
//
//Output Pin Implementation
//
COutPin::COutPin( 
    _In_     ULONG         ulPinId,
    _In_opt_ CMultipinMft *pparent,
    _In_     IKsControl*   pIksControl
    )
    : CBasePin(ulPinId, pparent)
    , m_firstSample(false)
    , m_queue(nullptr)
{
    HRESULT                 hr              = S_OK;
    ComPtr<IMFAttributes>   spAttributes;

    //
    //Get the input pin IKS control.. the pin IKS control talks to sourcetransform's IKS control
    //
    m_spIkscontrol = pIksControl;

    MFCreateAttributes( &spAttributes, 3 ); //Create the space for the attribute store!!
    setAttributes( spAttributes.Get());
    DMFTCHECKHR_GOTO( SetUINT32( MFT_SUPPORT_DYNAMIC_FORMAT_CHANGE, TRUE ), done );
    DMFTCHECKHR_GOTO( SetString( MFT_ENUM_HARDWARE_URL_Attribute, L"Sample_CameraExtensionMft" ),done );
    DMFTCHECKHR_GOTO( SetUINT32( MF_TRANSFORM_ASYNC, TRUE ),done );
    
done:
    DMFTRACE(DMFT_GENERAL, TRACE_LEVEL_INFORMATION, "%!FUNC! exiting %x = %!HRESULT!", hr, hr);
}

COutPin::~COutPin()
{
    m_spAttributes = nullptr;
    SAFE_DELETE(m_queue);
}

/*++
COutPin::AddPin
Description:
Called from AddSample if the Output Pin is in open state. This function looks for the queue
corresponding to the input pin and adds it in the queue.
--*/
IFACEMETHODIMP COutPin::AddPin(
    _In_ DWORD inputPinId
    )
{
    //
    //Add a new queue corresponding to the input pin
    //
    HRESULT hr = S_OK;
    CAutoLock Lock(lock());

    m_queue = new (std::nothrow) CPinQueue(inputPinId,Parent());
    DMFTCHECKNULL_GOTO(m_queue, done, E_OUTOFMEMORY );
done:

    DMFTRACE(DMFT_GENERAL, TRACE_LEVEL_INFORMATION, "%!FUNC! exiting %x = %!HRESULT!", hr, hr);
    return S_OK;
}
/*++
COutPin::AddSample
Description:
Called when ProcessInput is called on the Device Transform. The Input Pin puts the samples
in the pins connected. If the Output pins are in open state the sample lands in the queues
--*/

IFACEMETHODIMP COutPin::AddSample( 
    _In_ IMFSample *pSample,
    _In_ CBasePin *pPin)
{
    HRESULT hr = S_OK;
    UNREFERENCED_PARAMETER(pPin);

    CAutoLock Lock(lock()); // Serialize

    DMFTCHECKNULL_GOTO(pSample, done, E_INVALIDARG);
    if (FAILED(Active()))
    {
        goto done;
    }
    DMFTCHECKHR_GOTO(m_queue->Insert(pSample), done);
done:
    if (FAILED(hr))
    {
        // Throw an Error to the pipeline
        DMFTCHECKHR_GOTO(Parent()->QueueEvent(MEError, GUID_NULL, hr, NULL), done);
    }
    return hr;
}

/*++
COutPin::SetState
Description:
State setter for the output pin
--*/
IFACEMETHODIMP_(VOID) COutPin::SetFirstSample(
    _In_ BOOL fisrtSample )
{
    m_firstSample = fisrtSample;
}

/*++
COutPin::FlushQueues
Description:
Called from the device Transform when the output queues have to be flushed

--*/
HRESULT COutPin::FlushQueues()
{
    HRESULT hr = S_OK;
    CAutoLock Lock( lock() );
    (VOID)m_queue->Clear();
    DMFTRACE(DMFT_GENERAL, TRACE_LEVEL_INFORMATION, "%!FUNC! exiting %x = %!HRESULT!", hr, hr);
    return hr;
}
/*++
COutPin::ChangeMediaTypeFromInpin
Description:
called from the Device Transform when the input media type is changed. This will result in 
the xvp being possibly installed in the queue if the media types set on the input
and the output dont match
--*/
HRESULT COutPin::ChangeMediaTypeFromInpin(
    _In_ IMFMediaType* pOutMediaType,
    _In_ DeviceStreamState state)
{
    HRESULT hr = S_OK;
    CAutoLock Lock(lock());
    //
    //Set the state to disabled and while going out we will reset the state back to the requested state
    //Flush so that we drop any samples we have in store!!
    //
    SetState(DeviceStreamState_Disabled); 
    DMFTCHECKHR_GOTO(FlushQueues(),done);  
    DMFTCHECKNULL_GOTO(m_queue,done, E_UNEXPECTED); // The queue should alwaye be set
    if ( SUCCEEDED( hr ) )
    {
        (VOID)setMediaType( pOutMediaType );
        (VOID)SetState( state );
    }
done:
    return hr;
}

/*++
Description:
 called from the IMFdeviceTransform's 
--*/

IFACEMETHODIMP COutPin::GetOutputStreamInfo(
    _Out_ MFT_OUTPUT_STREAM_INFO *pStreamInfo
    )
{
    HRESULT hr = S_OK;
    IMFMediaType* pMediatype = nullptr;
    getMediaType( &pMediatype );

    if (SUCCEEDED(hr) && !pMediatype) {
        pMediatype->Release();
        pStreamInfo->cbAlignment = 0;
        pStreamInfo->cbSize = 0;
        pStreamInfo->dwFlags = MFT_OUTPUT_STREAM_WHOLE_SAMPLES | MFT_OUTPUT_STREAM_SINGLE_SAMPLE_PER_BUFFER | MFT_OUTPUT_STREAM_FIXED_SAMPLE_SIZE;
        pStreamInfo->dwFlags |= MFT_OUTPUT_STREAM_PROVIDES_SAMPLES;
        //We provide our samples..
    }
    else {
        hr = MF_E_TRANSFORM_TYPE_NOT_SET;
    }
    return hr;
}


/*++
COutPin::ProcessOutput
Description:
 called from the Device Transform when the transform manager demands output samples..
 If we have samples we forward it.
 If we are a photo pin then we forward only if trigger is sent. We ask the devicetransform if we have received the transform or not.
 If we have received the sample and we are passing out a sample we should reset the trigger set on the Device Transform
--*/

IFACEMETHODIMP COutPin::ProcessOutput(_In_  DWORD dwFlags,
    _Inout_  MFT_OUTPUT_DATA_BUFFER  *pOutputSample,
    _Out_   DWORD                       *pdwStatus
    )
{
    HRESULT             hr          = S_OK;
    ComPtr<IMFSample>   spSample;
    UNREFERENCED_PARAMETER(pdwStatus);
    UNREFERENCED_PARAMETER(dwFlags);
    CAutoLock lock(lock());
    MFTIME llTime = 0;
    if (FAILED(Active()))
    {
        goto done;
    }
    DMFTCHECKNULL_GOTO(m_queue, done, MF_E_INVALID_STREAM_STATE);
    pOutputSample->dwStatus = S_OK;

    DMFTCHECKHR_GOTO(m_queue->Remove(spSample.GetAddressOf()), done);
    
    if (FAILED(spSample->GetSampleTime(&llTime)))
    {
        spSample->SetSampleTime(MFGetSystemTime());
    }
    if (m_firstSample)
    {
        spSample->SetUINT32(MFSampleExtension_Discontinuity,TRUE);
        SetFirstSample(FALSE);
    }
    //
    // Any processing before we pass the sample to further in the pipeline should be done here
    // PROCESSSAMPLE(pSample); There is a bug in the pipeline and to circumvent that we have to
    // keep a reference on the sample. The pipeline is not releasing a reference when the sample
    // is fed in ProcessInput. We are explicitly releasing it for the pipeline.
    //
    pOutputSample->pSample = spSample.Detach();
    pOutputSample->dwStatus = S_OK;
done:
    return hr;
}

/*++
    COutPin::KsProperty
Description:
The KsProperty for the Pin.. this is to reroute all pin kscontrols to the input pin
--*/
IFACEMETHODIMP COutPin::KsProperty(
    _In_reads_bytes_(ulPropertyLength) PKSPROPERTY pProperty,
    _In_ ULONG ulPropertyLength,
    _Inout_updates_bytes_(ulDataLength) LPVOID pPropertyData,
    _In_ ULONG ulDataLength,
    _Out_opt_ ULONG* pBytesReturned
    )
{
    //
    //Route it to input pin
    //
    return m_spIkscontrol->KsProperty(pProperty,
        ulPropertyLength,
        pPropertyData,
        ulDataLength,
        pBytesReturned);
}

