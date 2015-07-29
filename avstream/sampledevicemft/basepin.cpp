#include "stdafx.h"
#include "common.h"
#include "multipinmft.h"
#include "multipinmfthelpers.h"
#include "basepin.h"

#ifdef MF_WPP
#include "basepin.tmh"
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
    { DeviceStreamState_Stop, DeviceStreamState_Stop, DeviceStreamState_Run, DeviceStreamState_Disabled },
    { DeviceStreamState_Stop, DeviceStreamState_Pause, DeviceStreamState_Run, DeviceStreamState_Disabled },
    { DeviceStreamState_Stop, DeviceStreamState_Pause, DeviceStreamState_Run, DeviceStreamState_Disabled },
    { DeviceStreamState_Disabled, DeviceStreamState_Disabled, DeviceStreamState_Disabled, DeviceStreamState_Disabled }
};

CBasePin::CBasePin( _In_ ULONG id, _In_ CMultipinMft *parent) :
    m_StreamId(id)
    , m_Parent(parent)
    , m_setMediaType(nullptr)
    , m_Attributes(nullptr)
    , m_Ikscontrol(nullptr)
{
    
}

CBasePin::~CBasePin()
{
    IMFMediaType *pMediaType = nullptr;
    for ( ULONG ulIndex = 0, ulSize = (ULONG)m_listOfMediaTypes.size(); ulIndex < ulSize; ulIndex++ )
    {
        pMediaType = m_listOfMediaTypes[ulIndex];
        SAFE_RELEASE(pMediaType);
    }
    m_listOfMediaTypes.clear();
    m_Attributes = nullptr;
}

HRESULT CBasePin::AddMediaType( _Inout_ DWORD *pos, _In_ IMFMediaType *pMediaType)
{
    HRESULT hr = S_OK;
    
    DMFTCHECKNULL_GOTO(pMediaType, done, E_INVALIDARG);

    m_listOfMediaTypes.push_back( pMediaType );
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

    DMFTCHECKNULL_GOTO(ppMediaType,done,E_INVALIDARG);
    
    if (pos >= m_listOfMediaTypes.size())
    {
        DMFTCHECKHR_GOTO(MF_E_NO_MORE_TYPES,done);
    }
    *ppMediaType = m_listOfMediaTypes[pos];
    if (*ppMediaType)
    {
        (*ppMediaType)->AddRef();
    }
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


STDMETHODIMP CBasePin::GetOutputAvailableType(_In_ DWORD dwTypeIndex, _Out_opt_ IMFMediaType** ppType)
{
    return GetMediaTypeAt( dwTypeIndex, ppType );
}


HRESULT CBasePin::QueryInterface(
    _In_ REFIID iid,
    _Outptr_result_maybenull_ void** ppv
    )
{
    HRESULT hr = S_OK;

    DMFTCHECKNULL_GOTO(ppv, out, E_POINTER);

    *ppv = nullptr;

    if ( iid == __uuidof( IUnknown ) )
    {
        *ppv = static_cast<VOID*>(this);
        AddRef();
    }
    else
    if ( iid == __uuidof( IMFAttributes ) )
    {
        *ppv = static_cast< IMFAttributes* >( this );
        AddRef();
    }
    else
    if ( iid == __uuidof( IKsControl ) )
    {
        *ppv = static_cast< IKsControl* >( this );
        AddRef();
    }
    else
    {
        hr = E_NOINTERFACE;
    }
out:
    return hr;
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
    m_pSourceTransform(nullptr),  
    m_stStreamType(GUID_NULL),
    m_activeStreamCount(0),
    m_state(DeviceStreamState_Stop),
    m_prefferedMediaType(nullptr),
    m_waitInputMediaTypeWaiter(NULL),
    m_preferredStreamState(DeviceStreamState_Stop)
{
    setAttributes(pAttributes);
}



CInPin::~CInPin()
{
    setAttributes( nullptr );
    m_pSourceTransform = nullptr;

    for (ULONG ulIndex = 0, ulSize = (ULONG)m_outpins.size();
        ulIndex < ulSize;
        ulIndex++)
    {
        CBasePin *pPin = m_outpins[ulIndex];
        SAFERELEASE(pPin);
    }
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

    m_pSourceTransform = pTransform;

    DMFTCHECKHR_GOTO( GetGUID( MF_DEVICESTREAM_STREAM_CATEGORY, &m_stStreamType ), done );

    //
    //Get the DevProxy IKSControl.. used to send the KSControls or the device control IOCTLS over to devproxy and finally on to the driver!!!!
    //
    DMFTCHECKHR_GOTO( m_pSourceTransform.As( &m_Ikscontrol ), done );

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
        m_pSourceTransform = nullptr;

        if ( m_waitInputMediaTypeWaiter )
        {
            CloseHandle( m_waitInputMediaTypeWaiter );
            m_waitInputMediaTypeWaiter = NULL;
        }

        m_stStreamType = GUID_NULL;
    }

    return hr;
}


STDMETHODIMP CInPin::GenerateMFMediaTypeListFromDevice(
    _In_ UINT uiStreamId
    )
{
    HRESULT hr = S_OK;
    GUID stSubType = { 0 };
    //This is only called in the begining when the input pin is constructed
    DMFTCHECKNULL_GOTO( m_pSourceTransform, done, MF_E_TRANSFORM_TYPE_NOT_SET );
    for (UINT iMediaType = 0; SUCCEEDED(hr) ; iMediaType++)
    {
        ComPtr<IMFMediaType> pMediaType = nullptr;
        hr = m_pSourceTransform->MFTGetOutputAvailableType( uiStreamId, iMediaType, pMediaType.GetAddressOf() );
        if (hr != S_OK)
            break;
        DMFTCHECKHR_GOTO(pMediaType->GetGUID(MF_MT_SUBTYPE, &stSubType),done);

            if (((stSubType == MFVideoFormat_RGB8) || (stSubType == MFVideoFormat_RGB555) ||
                (stSubType == MFVideoFormat_RGB565) || (stSubType == MFVideoFormat_RGB24) ||
                (stSubType == MFVideoFormat_RGB32) || (stSubType == MFVideoFormat_ARGB32) ||
                (stSubType == MFVideoFormat_AI44) || (stSubType == MFVideoFormat_AYUV) ||
                (stSubType == MFVideoFormat_I420) || (stSubType == MFVideoFormat_IYUV) ||
                (stSubType == MFVideoFormat_NV11) || (stSubType == MFVideoFormat_NV12) ||
                (stSubType == MFVideoFormat_UYVY) || (stSubType == MFVideoFormat_Y41P) ||
                (stSubType == MFVideoFormat_Y41T) || (stSubType == MFVideoFormat_Y42T) ||
                (stSubType == MFVideoFormat_YUY2) || (stSubType == MFVideoFormat_YV12) ||
                (stSubType == MFVideoFormat_P010) || (stSubType == MFVideoFormat_P016) ||
                (stSubType == MFVideoFormat_P210) || (stSubType == MFVideoFormat_P216) ||
                (stSubType == MFVideoFormat_v210) || (stSubType == MFVideoFormat_v216) ||
                (stSubType == MFVideoFormat_v410) || (stSubType == MFVideoFormat_Y210) ||
                (stSubType == MFVideoFormat_Y216) || (stSubType == MFVideoFormat_Y410) ||
                (stSubType == MFVideoFormat_Y416)) )
            {
                DWORD pos = 0;
                AddMediaType(&pos, pMediaType.Get());
            }
            pMediaType = nullptr;
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
    BOOL    sentOne = TRUE;
    DMFTCHECKNULL_GOTO(pSample, done, S_OK);

    for ( ULONG ulIndex = 0, ulSize = (ULONG) m_outpins.size(); ulIndex < ulSize; ulIndex++ )
    {
        COutPin *poPin = (COutPin *)m_outpins[ ulIndex ];

        pSample->AddRef();
        
        if (FAILED(hr = poPin->AddSample(pSample, this)))
        {
            pSample->Release();
        }
        sentOne = (sentOne || SUCCEEDED(hr));
    }
done: 
    return sentOne ? S_OK : E_FAIL;
}

STDMETHODIMP_(VOID) CInPin::ConnectPin( _In_ CBasePin * poPin )
{
    if (poPin!=nullptr)
    {
        m_outpins.push_back(poPin);
        poPin->AddRef();
    }
}

STDMETHODIMP_(DeviceStreamState) CInPin::GetState()
{
    return m_state;
}

STDMETHODIMP_(DeviceStreamState) CInPin::SetState( _In_ DeviceStreamState state )
{
    return (DeviceStreamState)InterlockedExchange((LONG*)&m_state, state );
}

STDMETHODIMP CInPin::WaitForSetInputPinMediaChange()
{
    DWORD   dwWait  = 0;
    HRESULT hr      = S_OK;

    dwWait = WaitForSingleObject( m_waitInputMediaTypeWaiter, INFINITE );
    
    if ( dwWait != WAIT_OBJECT_0 )
    {
        hr = E_FAIL;
        goto done;
    }
done:
    DMFTRACE(DMFT_GENERAL, TRACE_LEVEL_INFORMATION, "%!FUNC! exiting %x = %!HRESULT!", hr, hr);
    return hr;
}

STDMETHODIMP CInPin::GetInputStreamPreferredState(
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
        if ( m_prefferedMediaType != nullptr )
        {
            m_prefferedMediaType.CopyTo(ppMediaType);
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

    m_prefferedMediaType = nullptr;
    SetEvent(m_waitInputMediaTypeWaiter);
    DMFTRACE(DMFT_GENERAL, TRACE_LEVEL_INFORMATION, "%!FUNC! exiting %x = %!HRESULT!", hr, hr);
    return hr;
}


//
//Output Pin Implementation
//
COutPin::COutPin( 
    _In_     ULONG         ulPinId,
    _In_opt_ CMultipinMft *pparent,
    _In_     IKsControl*   pIksControl  
    )
    : CBasePin( ulPinId, pparent ),
    m_firstSample( false )
{
    HRESULT         hr              = S_OK;
    CPinState*      pState          = NULL;
    IMFAttributes   *pAttributes    = nullptr;

    for ( ULONG ulIndex = 0; ulIndex <= DeviceStreamState_Disabled; ulIndex++ )
    {
        switch ( ulIndex )
        {

        case DeviceStreamState_Run:
            pState = ( CPinState* )new CPinOpenState(this);
            DMFTCHECKNULL_GOTO(pState, done, E_OUTOFMEMORY);
            break;
        case DeviceStreamState_Pause:
        case DeviceStreamState_Stop:
        case DeviceStreamState_Disabled:
            //Currently both the closed and the drain state behave similarly
            pState = ( CPinState* )new CPinClosedState();
            DMFTCHECKNULL_GOTO(pState, done, E_OUTOFMEMORY);
            break;
        }
        
        m_states.push_back( pState );
    
        m_state = m_states[ DeviceStreamState_Stop ];
    }
    //
    //Get the input pin IKS control.. the pin IKS control talks to sourcetransform's IKS control
    //
    m_Ikscontrol = pIksControl;

    MFCreateAttributes( &pAttributes, 3 ); //Create the space for the attribute store!!
    setAttributes( pAttributes );
    DMFTCHECKHR_GOTO( SetUINT32( MFT_SUPPORT_DYNAMIC_FORMAT_CHANGE, TRUE ), done );
    DMFTCHECKHR_GOTO( SetString( MFT_ENUM_HARDWARE_URL_Attribute, L"Sample_CameraExtensionMft" ),done );
    DMFTCHECKHR_GOTO( SetUINT32( MF_TRANSFORM_ASYNC, TRUE ),done );
    //Set the pin attribute

    SAFE_RELEASE( pAttributes );
done:
    ;
    DMFTRACE(DMFT_GENERAL, TRACE_LEVEL_INFORMATION, "%!FUNC! exiting %x = %!HRESULT!", hr, hr);
}

COutPin::~COutPin()
{
    CPinQueue *que = NULL;
    m_Attributes = nullptr;

    for ( ULONG ulIndex = 0, ulSize = (ULONG)m_queues.size(); ulIndex < ulSize; ulIndex++ )
    {
        que = m_queues[ ulIndex ];
        delete(que);
        que = NULL;
    }
    CPinState *pinState = NULL;
    for (ULONG ulIndex = 0, ulSize = (ULONG)m_states.size(); ulIndex < ulSize; ulIndex++)
    {
        pinState = m_states[ ulIndex ];
        delete( pinState );
        pinState = NULL;
    }
    m_states.clear();
    m_queues.clear();
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

    CPinQueue *que = new (std::nothrow) CPinQueue(inputPinId);
    DMFTCHECKNULL_GOTO( que, done, E_OUTOFMEMORY );

    (void)m_queues.push_back( que );
    //
    //Just ramdonmize media types for odd numbered pins
    //
    if ( streamId() != 0 && streamId() % 2 != 0 )
    {
        RandomnizeMediaTypes(m_listOfMediaTypes);
    }

done:

    DMFTRACE(DMFT_GENERAL, TRACE_LEVEL_INFORMATION, "%!FUNC! exiting %x = %!HRESULT!", hr, hr);
    return S_OK;
}

/*++
COutPin::AddSampleInternal
Description:
Called from AddSample if the Output Pin is in open state. This function looks for the queue 
corresponding to the input pin and adds it in the queue. 
--*/

STDMETHODIMP COutPin::AddSampleInternal( _In_ IMFSample *pSample, _In_ CBasePin *pPin )
{
    BOOL    res = true;
    CAutoLock Lock( lock() );

    for ( DWORD dwIndex = 0, dwSize = (DWORD)m_queues.size(); dwIndex < dwSize; dwIndex++ )
    {
        //
        //This output pin maybe connected to multiple input pins
        //Only insert into the corresponding queue to the input pin on which the
        //sample is received
        //
        if (m_queues[dwIndex]->pinStreamId() == pPin->streamId())
        {
            if (! m_queues[ dwIndex ]->Insert( pSample ) )
            {
                res |= false;
            }
             
        }
    }
    return res ? S_OK : E_FAIL;
}

/*++
COutPin::AddSample
Description:
Called when ProcessInput is called on the Device Transform. The Input Pin puts the samples
in the pins connected. If the Output pins are in open state the sample lands in the queues
--*/

STDMETHODIMP COutPin::AddSample( _In_ IMFSample *pSample, _In_ CBasePin *pPin)
{
    HRESULT hr = S_OK;
    CAutoLock lock( lock() );
    
    DMFTCHECKHR_GOTO( m_state->Open(), done );
    
    DMFTCHECKHR_GOTO( AddSampleInternal( pSample, pPin ),done );

done:
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

/*++
COutPin::GetState
Description:
State getter for the output pin
--*/
DeviceStreamState COutPin::GetState()
{
    return m_state->State();
}

/*++
COutPin::SetState
Description:
State setter for the output pin
--*/

DeviceStreamState COutPin::SetState(DeviceStreamState state)
{
    CAutoLock Lock(lock());
    DeviceStreamState oldState = m_state->State();
    m_state = m_states[state];
    return oldState;
        
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
    
    for ( DWORD dwIndex = 0, dwSize = (DWORD) m_queues.size(); dwIndex < dwSize; dwIndex++ )
    {
        CPinQueue *que = m_queues[ dwIndex ];
        que->Clear();
    }

     DMFTRACE(DMFT_GENERAL, TRACE_LEVEL_INFORMATION, "%!FUNC! exiting %x = %!HRESULT!", hr, hr);
    return hr;
}
/*++
COutPin::ChangeMediaTypeFromInpin
Description:
called from the Device Transfrom When the input media type is changed. This will result in 
the xvp being possibly installed in the queue if the media types set on the input
and the output dont match
--*/
HRESULT COutPin::ChangeMediaTypeFromInpin(
    _In_ CInPin* inPin, 
    _In_ IMFMediaType *pInMediatype,
    _In_ IMFMediaType* pOutMediaType,
    _In_ DeviceStreamState state)
{
    HRESULT hr = S_OK;
    CPinQueue *que = NULL;
    CAutoLock Lock(lock());
    //
    //Set the state to disabled and while going out we will reset the state back to the requested state
    //Flush so that we drop any samples we have in store!!
    //
    SetState(DeviceStreamState_Disabled); 
    FlushQueues();  

    for (DWORD dwIndex = 0, dwSize = (DWORD) m_queues.size(); dwIndex < dwSize; dwIndex++)
    {
        que = m_queues[ dwIndex ];
        if (inPin->streamId() == que->pinStreamId())
        {
            break;
        }
        que = NULL;
    }

    if ( que )
    {
        //
        //recreate the tee, pass the D3D Manager to the Tee which will use DX if D3D manager is present
        //
        IUnknown *pD3DManagerUnk = NULL;

        (VOID)Parent()->GetD3DDeviceManager( &pD3DManagerUnk );
        hr = que->RecreateTee( pInMediatype, pOutMediaType, pD3DManagerUnk );
        if ( SUCCEEDED( hr ) )
        {
            (VOID)setMediaType( pOutMediaType );
            (VOID)SetState( state );
        }
        SAFE_RELEASE(pD3DManagerUnk);
    }
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
 called from the Device Transfrom when the transform manager demands output samples..
 If we have samples we forward it.
 If we are a photo pin then we forward only if trigger is sent. We ask the devicetransform if we have recieved the transform or not.
 If we have recieved the sample and we are passing out a sample we should reset the trigger set on the Device Transform
--*/

STDMETHODIMP COutPin::ProcessOutput(_In_  DWORD dwFlags,
    _Inout_  MFT_OUTPUT_DATA_BUFFER  *pOutputSample,
    _Out_   DWORD                       *pdwStatus
    )
{
    HRESULT         hr          = S_OK;
    IMFSample*      pSample     = nullptr;
    GUID            pinClsid    = GUID_NULL;
    BOOL            IsImagePin  =  FALSE;
    BOOL            IsSkipSample = FALSE;
    UNREFERENCED_PARAMETER(pdwStatus);
    UNREFERENCED_PARAMETER(dwFlags);
    CAutoLock lock(lock());
    DMFTCHECKHR_GOTO(m_state->Open(), done);
   
    //
    //Check if we are an image photo pin. The process output in that case should only proceed if trigger has been sent
    //Candidate for subclass!
    //
    
    if (SUCCEEDED(GetGUID(MF_DEVICESTREAM_STREAM_CATEGORY, &pinClsid))
        && ((IsEqualCLSID(pinClsid, PINNAME_IMAGE)) || IsEqualCLSID(pinClsid, PINNAME_VIDEO_STILL)))
    {
        IsImagePin = TRUE;
    }

    if (IsImagePin && !Parent()->isPhotoTriggerSent())
    {
        IsSkipSample = TRUE;
    }

   

    for ( DWORD dwIndex = 0, dwSize = (DWORD) m_queues.size(); dwIndex < dwSize; dwIndex++ )
    {
        CPinQueue *que = m_queues[dwIndex];

        pOutputSample->dwStatus = S_OK;

        if (!que->Remove(&pSample))
        {
            break;
        }

        MFTIME llTime = 0L;
        
        if (FAILED(pSample->GetSampleTime(&llTime)))
        {
            llTime = MFGetSystemTime();
            pSample->SetSampleTime(llTime);
        }

        if (!IsSkipSample)
        {
            if (m_firstSample)
            {
                pSample->SetUINT32(MFSampleExtension_Discontinuity,TRUE);
                SetFirstSample(FALSE);
            }

            //
            // Any processing before we pass the sample to further in the pipeline should be done here
            // PROCESSSAMPLE(pSample);
            //

            pOutputSample->pSample = pSample;
            pOutputSample->dwStatus = S_OK;
        }
        else
        {
            SAFERELEASE(pSample);
        }
    }
    if (!IsSkipSample && IsImagePin && pSample)
    {
        //
        //A sample has been sent over so image pin should exit
        //
#if defined (MF_DEVICEMFT_PHTOTOCONFIRMATION)
        if (Parent()->IsPhotoConfirmationEnabled())
        {
            //
            // Photo confirmation is enabled i.e. the pipeline has set up photo confirmation
            // Service photo confirmation.
            //
            ComPtr<IMFMediaType> spMediaType = nullptr;

            DMFTCHECKHR_GOTO(getMediaType(spMediaType.GetAddressOf()), done);

            DMFTRACE(DMFT_GENERAL, TRACE_LEVEL_INFORMATION, "%!FUNC! Calling PhotoConfirmation %p, is passed", pSample);

            DMFTCHECKHR_GOTO(Parent()->ProcessCapturePhotoConfirmationCallBack(spMediaType.Get(), pSample), done);
        }
#endif
        if (!Parent()->isPhotoModePhotoSequence())
        {
            //
            //A sample has been sent over so image pin should exit
            //
            Parent()->setPhotoTriggerSent(FALSE);
        }
    }
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
    return m_Ikscontrol->KsProperty(pProperty,
        ulPropertyLength,
        pPropertyData,
        ulDataLength,
        pBytesReturned);
}



