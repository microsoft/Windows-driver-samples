#include "stdafx.h"
#include "common.h"
#include "multipinmft.h"
#include "multipinmfthelpers.h"
#include "basepin.h"
#include "contosodevice.h"

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
    
    for ( ULONG ulIndex = 0, ulSize = (ULONG)m_listOfMediaTypes.size(); ulIndex < ulSize; ulIndex++ )
    {
        ComPtr<IMFMediaType> spMediaType;
        spMediaType.Attach(m_listOfMediaTypes[ulIndex]); // Releases the previously stored pointer
    }
    m_listOfMediaTypes.clear();
    m_spAttributes = nullptr;
}

STDMETHODIMP_(DeviceStreamState) CBasePin::GetState()
{
    return (DeviceStreamState) InterlockedCompareExchange((PLONG)&m_state, 0L,0L);
}

STDMETHODIMP_(DeviceStreamState) CBasePin::SetState(_In_ DeviceStreamState state)
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
    pMediaType->AddRef();
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

STDMETHODIMP_(BOOL) CBasePin::IsMediaTypeSupported
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
                *ppIMFMediaTypeFull = m_listOfMediaTypes[uIIndex];
                (*ppIMFMediaTypeFull)->AddRef();
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

STDMETHODIMP CBasePin::GetOutputAvailableType( 
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

STDMETHODIMP CInPin::Init( 
    _In_ IMFTransform* pTransform
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
        TEXT("MediaTypeWaiter")
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

        hr = m_spSourceTransform->MFTGetOutputAvailableType(uiStreamId, iMediaType, spMediaType.GetAddressOf());
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

STDMETHODIMP CInPin::SendSample(
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

STDMETHODIMP_(VOID) CInPin::ConnectPin( _In_ CBasePin * poPin )
{
    CAutoLock Lock(lock());
    if (poPin!=nullptr)
    {
        m_outpin = poPin;
    }
}

STDMETHODIMP CInPin::WaitForSetInputPinMediaChange()
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

STDMETHODIMP_(VOID) CInPin::ShutdownPin()
{
    m_spSourceTransform = nullptr;
    m_outpin = nullptr;
}
#if ((defined NTDDI_WIN10_VB) && (NTDDI_VERSION >= NTDDI_WIN10_VB))
//
//  ForwardSecureBuffer()
//
//  Parameters:
//      sample - The sample to send out for secure processing
//
//  Returns:
//      HRESULT
//
//  Notes:
//      This helper function calls back to the AVstream driver for post-processing of a 
//      secure buffer.  This is done here as an example for simplicity.  A more realistic
//      scenario might have you calling into an ISP driver that is not an AVstream driver.
//      To do that you would need to acquire a handle to that device and post an IOCTL.
//      That device would in turn need to have a secure companion driver (trustlet)
//      installed as part of the package to process the buffer.
//      
HRESULT CInPin::ForwardSecureBuffer(
    _In_    IMFSample *sample
)
{
    DWORD   bytesReturned = 0;
    CONTOSODEVICE_PROCESSBUFFER_PAYLOAD payload = { 0 };
    KSPROPERTY  property = { 0 };

    wil::com_ptr_nothrow<IMFMediaBuffer> mediaBuffer;
    wil::com_ptr_nothrow<IMFSecureBuffer> secureBuffer;

    //  Set up our private KSPROPERTY to some sample avstream driver.
    property.Set = PROPSETID_CONTOSODEVICE;
    property.Id = KSPROPERTY_CONTOSODEVICE_PROCESSBUFFER;
    property.Flags = KSPROPERTY_TYPE_SET;

    //  Set up the payload for that property.  Includes the secure buffer ID and buffer length.
    RETURN_IF_FAILED(sample->GetBufferByIndex(0, &mediaBuffer));
    //  Forward the buffer if it is secure.
    if (mediaBuffer.try_query_to(&secureBuffer))
    {
        RETURN_IF_FAILED(secureBuffer->GetIdentifier(&payload.identifier));
        RETURN_IF_FAILED(mediaBuffer->GetMaxLength(&payload.size));

        //  Log the buffer's secure ID and size for debugging.
        DMFTRACE(DMFT_GENERAL, TRACE_LEVEL_INFORMATION, "%!FUNC! identifier=%!GUID!, length=%d", &payload.identifier, payload.size);

        //  Send the KSPROPERTY synchronously to the driver.
        RETURN_IF_FAILED(KsProperty(&property, sizeof(property), &payload, sizeof(payload), &bytesReturned));
    }
    return S_OK;
}
#endif // ((defined NTDDI_WIN10_VB) && (NTDDI_VERSION >= NTDDI_WIN10_VB))
//
//Output Pin Implementation
//
COutPin::COutPin( 
    _In_     ULONG         ulPinId,
    _In_opt_ CMultipinMft *pparent,
    _In_     IKsControl*   pIksControl
#if ((defined NTDDI_WIN10_VB) && (NTDDI_VERSION >= NTDDI_WIN10_VB))
    , _In_     MFSampleAllocatorUsage allocatorUsage
#endif
    )
    : CBasePin(ulPinId, pparent)
    , m_firstSample(false)
    , m_queue(nullptr)
#if ((defined NTDDI_WIN10_VB) && (NTDDI_VERSION >= NTDDI_WIN10_VB))
    , m_allocatorUsage(allocatorUsage)
#endif
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
    if (m_queue)
    {
        SAFE_DELETE(m_queue);
    }
}

/*++
COutPin::AddPin
Description:
Called from AddSample if the Output Pin is in open state. This function looks for the queue
corresponding to the input pin and adds it in the queue.
--*/
STDMETHODIMP COutPin::AddPin(
    _In_ DWORD inputPinId
    )
{
    //
    //Add a new queue corresponding to the input pin
    //
    HRESULT hr = S_OK;
    CAutoLock Lock(lock());

    if (m_queue != NULL)
    {
        // This pin is alreaqdy connected.. This sample only supports a one on one pin mapping
        DMFTCHECKHR_GOTO(E_UNEXPECTED, done);

    }
#if defined MF_DEVICEMFT_ADD_GRAYSCALER_ // Take this out to remove the gray scaler
    m_queue = new (std::nothrow) CPinQueueWithGrayScale(inputPinId);
#else
    m_queue = new (std::nothrow) CPinQueue(inputPinId,Parent());
#endif
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

STDMETHODIMP COutPin::AddSample( 
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
        // @@@@ Readme : This is how to throw error to the pipeline and inform it that there is an Error.
        // This will destroy the pipeline and the App will need to recreate the source
        //
        DMFTCHECKHR_GOTO(Parent()->QueueEvent(MEError, GUID_NULL, hr, NULL), done);
    }
    return hr;
}

/*++
COutPin::SetState
Description:
State setter for the output pin
--*/
STDMETHODIMP_(VOID) COutPin::SetFirstSample(
    _In_ BOOL fisrtSample )
{
    m_firstSample = fisrtSample;
}

STDMETHODIMP_(VOID) COutPin::SetAllocator(
    _In_    IMFVideoSampleAllocator* pAllocator
)
{
    CAutoLock Lock(lock());
    m_spDefaultAllocator = pAllocator;
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
    _In_ IMFMediaType *pInMediatype,
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
#if ((defined NTDDI_WIN10_VB) && (NTDDI_VERSION >= NTDDI_WIN10_VB))
    hr = m_queue->RecreateTeeByAllocatorMode(pInMediatype, pOutMediaType, m_spDxgiManager.Get(), m_allocatorUsage, m_spDefaultAllocator.get());
#else
    hr = m_queue->RecreateTee(pInMediatype, pOutMediaType, m_spDxgiManager.Get());
#endif // ((defined NTDDI_WIN10_VB) && (NTDDI_VERSION >= NTDDI_WIN10_VB))
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

STDMETHODIMP COutPin::GetOutputStreamInfo(
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

STDMETHODIMP COutPin::ProcessOutput(_In_  DWORD dwFlags,
    _Inout_  MFT_OUTPUT_DATA_BUFFER  *pOutputSample,
    _Out_   DWORD                       *pdwStatus
    )
{
    HRESULT             hr          = S_OK;
    ComPtr<IMFSample>   spSample;
    MFTIME              llTime      = 0L;
    UNREFERENCED_PARAMETER(pdwStatus);
    UNREFERENCED_PARAMETER(dwFlags);
    CAutoLock lock(lock());
    
    if (FAILED(Active()))
    {
        goto done;
    }
    DMFTCHECKNULL_GOTO(m_queue, done, MF_E_INVALID_STREAM_STATE);
    pOutputSample->dwStatus = S_OK;

    DMFTCHECKHR_GOTO(m_queue->Remove(spSample.GetAddressOf()), done);
    
    if (FAILED(spSample->GetSampleTime(&llTime)))
    {
        llTime = MFGetSystemTime();
        spSample->SetSampleTime(llTime);
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
STDMETHODIMP COutPin::KsProperty(
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

//
// Asynchronous IO handling.
//

STDMETHODIMP CAsyncInPin::SendSample(_In_ IMFSample *pSample)
{
    HRESULT hr = S_OK;
    CAutoLock Lock(lock());
    if (SUCCEEDED(Active()))
    {
        DMFTCHECKHR_GOTO(MFPutWorkItem(m_dwWorkQueueId, static_cast<IMFAsyncCallback*>(m_asyncCallback.Get()), pSample), done);
    }
done:
    return hr;
}

STDMETHODIMP CAsyncInPin::Init()
{
    m_asyncCallback = new (std::nothrow) CDMFTAsyncCallback<CAsyncInPin, &CAsyncInPin::Invoke>(this, m_dwWorkQueueId);
    if (!m_asyncCallback)
        throw bad_alloc();
    return S_OK;
}

//  Pass a secure frame buffer to our AVstream driver.
HRESULT CAsyncInPin::Invoke( _In_ IMFAsyncResult* pResult )
{
    HRESULT hr = S_OK;
    ComPtr<IUnknown> spUnknown;
    ComPtr<IMFSample> spSample;

    DMFTCHECKHR_GOTO(Active(), done);
    DMFTCHECKNULL_GOTO(pResult, done, E_UNEXPECTED);
    DMFTCHECKHR_GOTO(pResult->GetState(&spUnknown), done);

    DMFTCHECKHR_GOTO(spUnknown->QueryInterface(__uuidof(IMFSample), reinterpret_cast<PVOID*>(spSample.GetAddressOf())), done);

    DMFTCHECKNULL_GOTO(spSample.Get(), done, E_INVALIDARG);

#if ((defined NTDDI_WIN10_VB) && (NTDDI_VERSION >= NTDDI_WIN10_VB))
    //
    //  Do secure buffer post-processing.
    //
    RETURN_IF_FAILED(ForwardSecureBuffer(spSample.Get()));
#endif // ((defined NTDDI_WIN10_VB) && (NTDDI_VERSION >= NTDDI_WIN10_VB))
    COutPin *poPin = static_cast<COutPin*>(m_outpin.Get());
    DMFTCHECKHR_GOTO(poPin->AddSample(spSample.Get(), this), done);
done:
    return hr;
}

STDMETHODIMP_(VOID) CAsyncInPin::ShutdownPin()
{
    CAutoLock Lock(lock());
    if (m_asyncCallback.Get())
    {
        (VOID)m_asyncCallback->Shutdown(); //Break reference with the parent
    }
    CInPin::ShutdownPin();
}

//
// Pins that need translation like from MJPEG to NV12 or H264 to NV12
// Return Value: S_OK: media type added, S_FALSE: mediatype skipped. Not an Error
// Failure: any other catastrophic failure
//
STDMETHODIMP CTranslateOutPin::AddMediaType(
    _Inout_ DWORD *pos,
    _In_ IMFMediaType *pMediaType)
{
    HRESULT hr = S_OK;
    ComPtr<IMFMediaType> pNewMediaType = nullptr;
    GUID   guidSubType = GUID_NULL;

    auto needTranslation = [&](IMFMediaType* pMediaType) {
        if (SUCCEEDED(pMediaType->GetGUID(MF_MT_SUBTYPE, &guidSubType)))
        {
            for (UINT32 uiIndex = 0; uiIndex < ARRAYSIZE(tranlateGUIDS); uiIndex++)
            {
                if (IsEqualGUID(guidSubType, tranlateGUIDS[uiIndex]))
                    return true;
            }
        }
        return false;
    };
    DMFTCHECKNULL_GOTO(pMediaType, done, E_INVALIDARG);
    DMFTCHECKHR_GOTO(pMediaType->GetGUID(MF_MT_SUBTYPE, &guidSubType), done);

    // @@@@ README the below lines show how to exclude mediatypes which we don't want
  /*  if ((guidSubType != MFVideoFormat_H264))
    {
        hr = S_FALSE;
        goto done;
    }*/

    if (needTranslation(pMediaType))
    {
        UINT32 uiWidth = 0, uiHeight = 0, uiImageSize = 0;
       
        DMFTCHECKHR_GOTO(MFGetAttributeRatio(pMediaType, MF_MT_FRAME_SIZE, &uiWidth, &uiHeight), done);
        DMFTCHECKHR_GOTO(MFCreateMediaType(pNewMediaType.GetAddressOf()), done);
        DMFTCHECKHR_GOTO(pMediaType->CopyAllItems(pNewMediaType.Get()), done);
        DMFTCHECKHR_GOTO(MFCalculateImageSize(translatedGUID, uiWidth, uiHeight, &uiImageSize), done);
        DMFTCHECKHR_GOTO(pNewMediaType->SetGUID(MF_MT_SUBTYPE, translatedGUID), done);
        DMFTCHECKHR_GOTO(pNewMediaType->SetUINT32(MF_MT_SAMPLE_SIZE, uiImageSize), done);
        hr = ExceptionBoundary([&]()
        {
            m_TranslatedMediaTypes.insert(std::pair<IMFMediaType*, IMFMediaType*>( pNewMediaType.Get(), pMediaType));
        });
        DMFTCHECKHR_GOTO(hr, done);
    }
    else
    {
        pNewMediaType = pMediaType;
    }
    // Set custom properties here
    //@@@@ README .. add these lines if you want this media types to be understood as a spherical media type
#if defined MF_DEVICEMFT_SET_SPHERICAL_ATTRIBUTES
    pNewMediaType->SetUINT32(MF_SD_VIDEO_SPHERICAL, TRUE);
    pNewMediaType->SetUINT32(MF_SD_VIDEO_SPHERICAL_FORMAT, MFVideoSphericalFormat_Equirectangular);
#endif
    // Add it to the pin media type
    hr = CBasePin::AddMediaType(pos, pNewMediaType.Get());
done:
    return hr;
}

STDMETHODIMP_(BOOL) CTranslateOutPin::IsMediaTypeSupported(
    _In_ IMFMediaType *pMediaType,
    _When_(ppIMFMediaTypeFull != nullptr, _Outptr_result_maybenull_)
    IMFMediaType **ppIMFMediaTypeFull)
{
    DWORD dwFlags = 0, dwMatchedFlags = (MF_MEDIATYPE_EQUAL_MAJOR_TYPES | MF_MEDIATYPE_EQUAL_FORMAT_TYPES | MF_MEDIATYPE_EQUAL_FORMAT_DATA);

    std::map<IMFMediaType*, IMFMediaType*>::iterator found = std::find_if(m_TranslatedMediaTypes.begin(), m_TranslatedMediaTypes.end(),
        [&](std::pair<IMFMediaType*, IMFMediaType*> p)
    {
        return (SUCCEEDED(pMediaType->IsEqual(p.first, &dwFlags))
            && ((dwFlags & dwMatchedFlags) == (dwMatchedFlags)));
    });

    if (found != m_TranslatedMediaTypes.end())
    {
        ComPtr<IMFMediaType> spMediaType = (*found).second;
        if (ppIMFMediaTypeFull)
        {
            *ppIMFMediaTypeFull = spMediaType.Detach();
        }
        return true;
    }
    else
    {
        // Check if we can find it in the Base list.. maybe the mediatype is originally an uncompressed media type
        return CBasePin::IsMediaTypeSupported(pMediaType,ppIMFMediaTypeFull);
    }
}

HRESULT CTranslateOutPin::ChangeMediaTypeFromInpin(
    _In_ IMFMediaType *pInMediatype,
    _In_ IMFMediaType* pOutMediaType,
    _In_ DeviceStreamState state)
{
    CAutoLock Lock(lock());
    //
    // Set the state to disabled and while going out we will reset the state back to the requested state
    // Flush so that we drop any samples we have in store!!
    //
    SetState(DeviceStreamState_Disabled);
    RETURN_IF_FAILED(FlushQueues());
    RETURN_HR_IF_NULL(E_UNEXPECTED, m_queue); // The queue should always be set
    RETURN_IF_FAILED(m_queue->RecreateTee(pInMediatype, pOutMediaType, m_spDxgiManager.Get()));

    (VOID)setMediaType(pOutMediaType);
    (VOID)SetState(state);

    return S_OK;
}
