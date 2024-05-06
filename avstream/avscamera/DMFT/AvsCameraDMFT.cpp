//
//    Copyright (C) Microsoft.  All rights reserved.
//

#include "stdafx.h"
#ifdef MF_WPP
#include "AvsCameraDMFT.tmh"    //--REF_ANALYZER_DONT_REMOVE--
#endif
//
// This DeviceMFT is a stripped down implementation of the device MFT Sample present in the sample Repo
// The original DMFT is present at https://github.com/microsoft/Windows-driver-samples/tree/main/avstream/sampledevicemft
//
CMultipinMft::CMultipinMft()
:   m_nRefCount( 0 ),
    m_InputPinCount( 0 ),
    m_OutputPinCount( 0 ),
    m_dwWorkQueueId ( MFASYNC_CALLBACK_QUEUE_MULTITHREADED ),
    m_lWorkQueuePriority ( 0 ),
    m_spAttributes( nullptr ),
    m_spSourceTransform( nullptr ),
    m_SymbolicLink(nullptr)

{
    HRESULT hr = S_OK;
    ComPtr<IMFAttributes> pAttributes = nullptr;
    MFCreateAttributes( &pAttributes, 0 );
    DMFTCHECKHR_GOTO(pAttributes->SetUINT32( MF_TRANSFORM_ASYNC, TRUE ),done);
    DMFTCHECKHR_GOTO(pAttributes->SetUINT32( MFT_SUPPORT_DYNAMIC_FORMAT_CHANGE, TRUE ),done);
    DMFTCHECKHR_GOTO(pAttributes->SetUINT32( MF_SA_D3D_AWARE, TRUE ), done);
    DMFTCHECKHR_GOTO(pAttributes->SetString( MFT_ENUM_HARDWARE_URL_Attribute, L"SampleMultiPinMft" ),done);
    m_spAttributes = pAttributes;
done:
    DMFTRACE(DMFT_GENERAL, TRACE_LEVEL_INFORMATION, "%!FUNC! exiting %x = %!HRESULT!", hr, hr);
}

CMultipinMft::~CMultipinMft( )
{
    m_OutPins.clear();
    SAFE_ARRAYDELETE(m_SymbolicLink);
    m_spSourceTransform = nullptr;

}

IFACEMETHODIMP_(ULONG) CMultipinMft::AddRef(
    void
    )
{
    return InterlockedIncrement(&m_nRefCount);
}

IFACEMETHODIMP_(ULONG) CMultipinMft::Release(
    void
    )
{
    ULONG uCount = InterlockedDecrement(&m_nRefCount);

    if ( uCount == 0 )
    {
        delete this;
    }
    return uCount;
}

IFACEMETHODIMP CMultipinMft::QueryInterface(
    _In_ REFIID iid,
    _COM_Outptr_ void** ppv
    )
{

    HRESULT hr = S_OK;
    *ppv = NULL;
    
    if ((iid == __uuidof(IMFDeviceTransform)) || (iid == __uuidof(IUnknown)))
    {
        *ppv = static_cast< IMFDeviceTransform* >(this);
    }
    else if ( iid == __uuidof( IMFMediaEventGenerator ) )
    {
        *ppv = static_cast< IMFMediaEventGenerator* >(this);
    }
    else if ( iid == __uuidof( IMFShutdown ) )
    {
        *ppv = static_cast< IMFShutdown* >( this );
    }
    else if ( iid == __uuidof( IKsControl ) )
    {
        *ppv = static_cast< IKsControl* >( this );
    }
    else if ( iid == __uuidof( IMFRealTimeClientEx ) )
    {
        *ppv = static_cast< IMFRealTimeClientEx* >( this );
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

/*++
    Description:
    This function is the entry point of the transform
    The following things may be initialized here
    1) Query for MF_DEVICEMFT_CONNECTED_FILTER_KSCONTROL on the attributes supplied 
    2) From the IUnknown acquired get the IMFTransform interface.
    3) Get the stream count.. The output streams are of consequence to the tranform.
    The input streams should correspond to the output streams exposed by the source transform
    acquired from the Attributes supplied.
    4) Get the IKSControl which is used to send KSPROPERTIES, KSEVENTS and KSMETHODS to the driver for the filer level. Store it in your filter class
    5) Get the OutPutStreamAttributes for the output pins of the source transform.  This can further be used to QI and acquire
     the IKSControl related to the specific pin. This can be used to send PIN level KSPROPERTIES, EVENTS and METHODS to the pins
    6) Create the output pins

--*/

IFACEMETHODIMP CMultipinMft::InitializeTransform ( 
    _In_ IMFAttributes *pAttributes
    )
{
    HRESULT                 hr              = S_OK;
    ComPtr<IUnknown>        spFilterUnk     = nullptr;
    DWORD                   *pcInputStreams = NULL, *pcOutputStreams = NULL;
    DWORD                   inputStreams    = 0;
    DWORD                   outputStreams   = 0;
    GUID*                   outGuids        = NULL;
    GUID                    streamCategory  = GUID_NULL;
    ULONG                   ulOutPinIndex   = 0;
    UINT32                  uiSymLinkLen    = 0;
    DMFTCHECKNULL_GOTO( pAttributes, done, E_INVALIDARG );
    //
    // The attribute passed with MF_DEVICEMFT_CONNECTED_FILTER_KSCONTROL is the source transform. This generally represents a filter
    // This needs to be stored so that we know the device properties. We cache it. We query for the IKSControl which is used to send
    // controls to the driver.
    //
    DMFTCHECKHR_GOTO( pAttributes->GetUnknown( MF_DEVICEMFT_CONNECTED_FILTER_KSCONTROL,IID_PPV_ARGS( &spFilterUnk ) ),done );
    
    if (SUCCEEDED(pAttributes->GetStringLength(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_SYMBOLIC_LINK, &uiSymLinkLen))) // Not available prior to RS5
    {
        m_SymbolicLink = new (std::nothrow) WCHAR[++uiSymLinkLen];
        DMFTCHECKNULL_GOTO(m_SymbolicLink, done, E_OUTOFMEMORY);
        DMFTCHECKHR_GOTO(pAttributes->GetString(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_SYMBOLIC_LINK, m_SymbolicLink, uiSymLinkLen, &uiSymLinkLen), done);
    }

    DMFTCHECKHR_GOTO( spFilterUnk.As( &m_spSourceTransform ), done );
    
    DMFTCHECKHR_GOTO( m_spSourceTransform.As( &m_spIkscontrol ), done );
    
    DMFTCHECKHR_GOTO(m_spSourceTransform->GetStreamCount(&inputStreams, &outputStreams), done);

    spFilterUnk = nullptr;

    //
    //The number of input pins created by the device transform should match the pins exposed by
    //the source transform i.e. outputStreams from SourceTransform or DevProxy = Input pins of the Device MFT
    //
    
    if ( inputStreams > 0 || outputStreams > 0 )
    {
        pcInputStreams = new (std::nothrow) DWORD[ inputStreams ];
        DMFTCHECKNULL_GOTO( pcInputStreams, done, E_OUTOFMEMORY);

        pcOutputStreams = new (std::nothrow) DWORD[ outputStreams ];
        DMFTCHECKNULL_GOTO( pcOutputStreams, done, E_OUTOFMEMORY );
        
        DMFTCHECKHR_GOTO( m_spSourceTransform->GetStreamIDs( inputStreams, pcInputStreams,
            outputStreams,
            pcOutputStreams ),done );

        for ( ULONG ulIndex = 0; ulIndex < outputStreams; ulIndex++ )
        {           
            ComPtr<IMFAttributes>   spInAttributes;
            ComPtr<CInPin>          spInPin;

            DMFTCHECKHR_GOTO(m_spSourceTransform->GetOutputStreamAttributes(ulIndex, &spInAttributes),done);
            spInPin = new (std::nothrow) CInPin(spInAttributes.Get(), ulIndex, this);
            DMFTCHECKNULL_GOTO(spInPin.Get(), done, E_OUTOFMEMORY);

            hr = ExceptionBoundary([&]()
            {
                m_InPins.push_back(spInPin.Get());
            });
            DMFTCHECKHR_GOTO(hr, done);
            DMFTCHECKHR_GOTO(spInPin->Init(m_spSourceTransform.Get()), done);
        }
        
        //
        // Create one on one mapping
        //
        for (ULONG ulIndex = 0; ulIndex < m_InPins.size(); ulIndex++)
        {
            
            ComPtr<CInPin> spInPin = (CInPin*)m_InPins[ulIndex].Get();
            
            if (spInPin.Get())
            {
                ComPtr<COutPin> spOutPin;
                ComPtr<IKsControl> spKscontrol;
                GUID pinGuid = GUID_NULL;
                UINT32 uiFrameSourceType = 0;

                DMFTCHECKHR_GOTO(spInPin.As(&spKscontrol), done); // Grab the IKSControl off the input pin
                DMFTCHECKHR_GOTO(spInPin->GetGUID(MF_DEVICESTREAM_STREAM_CATEGORY, &pinGuid), done); // Get the Stream Category. Advertise on the output pin


                spOutPin = new (std::nothrow) COutPin(
                    ulIndex,
                    this,
                    spKscontrol.Get()); // Create the output pin
                DMFTCHECKNULL_GOTO(spOutPin.Get(), done, E_OUTOFMEMORY);

                DMFTCHECKHR_GOTO(spOutPin->SetGUID(MF_DEVICESTREAM_STREAM_CATEGORY, pinGuid), done); // Advertise the Stream category to the Pipeline
                DMFTCHECKHR_GOTO(spOutPin->SetUINT32(MF_DEVICESTREAM_STREAM_ID, ulIndex), done);
                if (SUCCEEDED(spInPin->GetUINT32(MF_DEVICESTREAM_ATTRIBUTE_FRAMESOURCE_TYPES, &uiFrameSourceType)))
                {
                    DMFTCHECKHR_GOTO(spOutPin->SetUINT32(MF_DEVICESTREAM_ATTRIBUTE_FRAMESOURCE_TYPES, uiFrameSourceType), done);
                }

                hr = BridgeInputPinOutputPin(spInPin.Get(), spOutPin.Get());
                if (SUCCEEDED(hr))
                {
                    DMFTCHECKHR_GOTO(ExceptionBoundary([&]()
                    {
                       m_OutPins.push_back(spOutPin.Get());
                    }), done);
                    ulOutPinIndex++;
                }
                DMFTCHECKHR_GOTO(hr, done);
            }
        }
 
    }
    
    m_InputPinCount =  ULONG ( m_InPins.size() );
    m_OutputPinCount = ULONG ( m_OutPins.size() );
    
done:
    DMFTRACE(DMFT_GENERAL, TRACE_LEVEL_INFORMATION, "%!FUNC! exiting %x = %!HRESULT!",hr,hr);
 
    if ( pcInputStreams )
    {
        delete[ ] ( pcInputStreams );
    }
    if ( pcOutputStreams )
    {
        delete[ ] ( pcOutputStreams );
    }
    if ( outGuids )
    {
        delete [] ( outGuids );
    }
    if ( FAILED( hr ) )
    {
        //Release the pins and the resources acquired
        m_InPins.clear();
        m_OutPins.clear();
        //
        // Simply clear the custom pins since the input pins must have deleted the pin
        //
        m_spSourceTransform = nullptr;
        m_spIkscontrol = nullptr;
    }
    return hr;
}


IFACEMETHODIMP CMultipinMft::SetWorkQueueEx(
    _In_  DWORD dwWorkQueueId,
    _In_ LONG lWorkItemBasePriority
    )
/*++
    Description:

    Implements IMFRealTimeClientEx::SetWorkQueueEx function

--*/
{
    CAutoLock   lock( m_critSec );
    //
    // Cache the WorkQueuId and WorkItemBasePriority. This is called once soon after the device MFT is initialized
    //
    m_dwWorkQueueId = dwWorkQueueId;
    m_lWorkQueuePriority = lWorkItemBasePriority;
    // Set it on the pins
    for (DWORD dwIndex = 0; dwIndex < (DWORD)m_InPins.size(); dwIndex++)
    {
        m_InPins[dwIndex]->SetWorkQueue(dwWorkQueueId);
    }
    for (DWORD dwIndex = 0; dwIndex < (DWORD)m_OutPins.size(); dwIndex++)
    {
        m_OutPins[dwIndex]->SetWorkQueue(dwWorkQueueId);
    }
    return S_OK;

}

//
// IMFDeviceTransform functions
//
IFACEMETHODIMP  CMultipinMft::GetStreamCount(
    _Inout_ DWORD  *pdwInputStreams,
    _Inout_ DWORD  *pdwOutputStreams
    )
/*++
    Description:    Implements IMFTransform::GetStreamCount function
--*/
{
    HRESULT hr = S_OK;
    CAutoLock   lock(m_critSec);
    DMFTCHECKNULL_GOTO(pdwInputStreams, done, E_INVALIDARG);
    DMFTCHECKNULL_GOTO(pdwOutputStreams, done, E_INVALIDARG);
    *pdwInputStreams     = m_InputPinCount;
    *pdwOutputStreams    = m_OutputPinCount;
    DMFTRACE( DMFT_GENERAL, TRACE_LEVEL_INFORMATION, "%!FUNC! exiting %x = %!HRESULT!", hr, hr );
done:
    return hr;
}

//
//Doesn't strictly conform to the GetStreamIDs on IMFTransform Interface!
//
IFACEMETHODIMP  CMultipinMft::GetStreamIDs(
    _In_                                    DWORD  dwInputIDArraySize,
    _When_(dwInputIDArraySize >= m_InputPinCount, _Out_writes_(dwInputIDArraySize))  DWORD* pdwInputIDs,
    _In_                                    DWORD  dwOutputIDArraySize,
    _When_(dwOutputIDArraySize >= m_OutputPinCount && (pdwInputIDs && (dwInputIDArraySize > 0)),
    _Out_writes_(dwOutputIDArraySize)) _On_failure_(_Valid_) DWORD* pdwOutputIDs
    )
/*++
    Description:
        Implements IMFTransform::GetStreamIDs function
--*/
{
    HRESULT hr = S_OK;
    CAutoLock   lock(m_critSec);
    if ( ( dwInputIDArraySize < m_InputPinCount ) && ( dwOutputIDArraySize < m_OutputPinCount ) )
    {
        hr = MF_E_BUFFERTOOSMALL;
        goto done;
    }

    if ( dwInputIDArraySize )
    {
        DMFTCHECKNULL_GOTO( pdwInputIDs, done, E_POINTER );
        for ( DWORD dwIndex = 0; dwIndex < ((dwInputIDArraySize > m_InputPinCount) ? m_InputPinCount:
            dwInputIDArraySize); dwIndex++ )
        {
            pdwInputIDs[ dwIndex ] = ( m_InPins[dwIndex] )->streamId();
        }
    }
    
    if ( dwOutputIDArraySize )
    {
        DMFTCHECKNULL_GOTO( pdwOutputIDs, done, E_POINTER );
        for ( DWORD dwIndex = 0; dwIndex < ((dwOutputIDArraySize >  m_OutputPinCount)? m_OutputPinCount:
            dwOutputIDArraySize); dwIndex++ )
        {
            pdwOutputIDs[ dwIndex ] = (m_OutPins[ dwIndex ])->streamId();
        }
    }
done:
    return hr;
}

/*++
Name: CMultipinMft::GetInputAvailableType
Description:
Implements IMFTransform::GetInputAvailableType function. This function
gets the media type supported by the specified stream based on the
index dwTypeIndex.
--*/
IFACEMETHODIMP  CMultipinMft::GetInputAvailableType(
    _In_        DWORD           dwInputStreamID,
    _In_        DWORD           dwTypeIndex,
    _Out_ IMFMediaType**        ppMediaType
    )
{
    HRESULT hr = S_OK;
        
    ComPtr<CInPin> spiPin = GetInPin( dwInputStreamID );
    DMFTCHECKNULL_GOTO(ppMediaType, done, E_INVALIDARG);
    DMFTCHECKNULL_GOTO( spiPin, done, MF_E_INVALIDSTREAMNUMBER );
    
    *ppMediaType = nullptr;

    hr = spiPin->GetOutputAvailableType( dwTypeIndex,ppMediaType );

    if (FAILED(hr))
    {
        DMFTRACE( DMFT_GENERAL, TRACE_LEVEL_INFORMATION, "%!FUNC! Pin: %d Index: %d exiting  %!HRESULT!",
            dwInputStreamID,
            dwTypeIndex,
            hr);
    }

done:
    return hr;
}

IFACEMETHODIMP CMultipinMft::GetOutputAvailableType(
    _In_         DWORD           dwOutputStreamID,
    _In_         DWORD           dwTypeIndex,
    _Out_        IMFMediaType**  ppMediaType
    )
/*++
    Description:

        Implements IMFTransform::GetOutputAvailableType function. This function
        gets the media type supported by the specified stream based on the
        index dwTypeIndex.

--*/
{
    HRESULT hr = S_OK;
    CAutoLock Lock(m_critSec);

    ComPtr<COutPin> spoPin = GetOutPin( dwOutputStreamID );

    DMFTCHECKNULL_GOTO( spoPin.Get(), done, MF_E_INVALIDSTREAMNUMBER );
    DMFTCHECKNULL_GOTO(ppMediaType, done, E_INVALIDARG);

    *ppMediaType = nullptr;

    hr = spoPin->GetOutputAvailableType( dwTypeIndex, ppMediaType );

    if ( FAILED( hr ) )
    {
        DMFTRACE( DMFT_GENERAL, TRACE_LEVEL_INFORMATION, "%!FUNC! Pin: %d Index: %d exiting  %!HRESULT!", 
            dwOutputStreamID,
            dwTypeIndex,
            hr );
    }

done:
    return hr;
}

IFACEMETHODIMP  CMultipinMft::GetInputCurrentType(
    _In_        DWORD           dwInputStreamID,
    _COM_Outptr_result_maybenull_ IMFMediaType**  ppMediaType
    )
/*++
    Description:
        Implements IMFTransform::GetInputCurrentType function. This function
        returns the current media type set on the specified stream.
--*/
{
    //
    // The input current types will not come to this transform.
    // The outputs of this transform matter. The DTM manages the
    // output of this transform and the inptuts of the source transform
    //
    UNREFERENCED_PARAMETER(dwInputStreamID);
    UNREFERENCED_PARAMETER(ppMediaType);
    return S_OK;
}

IFACEMETHODIMP  CMultipinMft::GetOutputCurrentType(
    _In_         DWORD           dwOutputStreamID,
    _Out_        IMFMediaType**  ppMediaType
    )
/*++
    Description:

        Implements IMFTransform::GetOutputCurrentType function. This function
        returns the current media type set on the specified stream.

--*/
{
    HRESULT hr = S_OK;
    ComPtr<COutPin> spoPin;
    CAutoLock lock( m_critSec );

    DMFTCHECKNULL_GOTO( ppMediaType, done, E_INVALIDARG );
    
    *ppMediaType = nullptr;

    spoPin = GetOutPin( dwOutputStreamID );

    DMFTCHECKNULL_GOTO(spoPin, done, MF_E_INVALIDSTREAMNUMBER );

    DMFTCHECKHR_GOTO(spoPin->getMediaType( ppMediaType ),done );
    
    DMFTCHECKNULL_GOTO( *ppMediaType, done, MF_E_TRANSFORM_TYPE_NOT_SET );

done:
    DMFTRACE( DMFT_GENERAL, TRACE_LEVEL_INFORMATION, "%!FUNC! exiting %x = %!HRESULT!", hr, hr );
    return hr;
}


IFACEMETHODIMP  CMultipinMft::ProcessEvent(
    _In_    DWORD           dwInputStreamID,
    _In_    IMFMediaEvent*  pEvent
    )
    /*++
    Description:

    Implements IMFTransform::ProcessEvent function. This function
    processes events that come to the MFT.

    --*/
{
    UNREFERENCED_PARAMETER(dwInputStreamID);
    UNREFERENCED_PARAMETER(pEvent);
    return S_OK;
}



IFACEMETHODIMP  CMultipinMft::ProcessMessage(
    _In_ MFT_MESSAGE_TYPE    eMessage,
    _In_ ULONG_PTR           ulParam
    )
/*++
    Description:

        Implements IMFTransform::ProcessMessage function. This function
        processes messages coming to the MFT.

--*/
{
    HRESULT hr = S_OK;

    UNREFERENCED_PARAMETER(ulParam);
    
    CAutoLock _lock( m_critSec );
    
    switch ( eMessage )
    {
    case MFT_MESSAGE_COMMAND_FLUSH:
        //
        // This is MFT wide flush.. Flush all output pins
        //
        (VOID)FlushAllStreams();
        break;
    case MFT_MESSAGE_COMMAND_DRAIN:
        //
        // There is no draining for Device MFT. Just kept here for reference
        //
        break;
    case MFT_MESSAGE_NOTIFY_START_OF_STREAM:
        //
        // No op for device MFTs
        //
        break;
    case MFT_MESSAGE_SET_D3D_MANAGER:
        {
           if ( ulParam )
           {
               ComPtr< IDirect3DDeviceManager9 >   spD3D9Manager;
               ComPtr< IMFDXGIDeviceManager >      spDXGIManager;

               hr = ( ( IUnknown* ) ulParam )->QueryInterface( IID_PPV_ARGS( &spD3D9Manager ) );
               if ( SUCCEEDED( hr ) )
               {
                   m_spDeviceManagerUnk = ( IUnknown* )ulParam;
                   DMFTRACE( DMFT_GENERAL, TRACE_LEVEL_INFORMATION, "%!FUNC! IDirect3DDeviceManager9 %p, is passed", spD3D9Manager.Get() );
               }
               else
               {
                   hr = ( ( IUnknown* ) ulParam )->QueryInterface( IID_PPV_ARGS( &spDXGIManager ) );
                   if ( SUCCEEDED(hr) )
                   {
                       m_spDeviceManagerUnk = (IUnknown*)ulParam;
                       DMFTRACE( DMFT_GENERAL, TRACE_LEVEL_INFORMATION, "%!FUNC! IMFDXGIDeviceManager %p, is passed", spDXGIManager.Get());
                  }
               }
           }
           else
           {
               m_spDeviceManagerUnk = nullptr;
               hr = S_OK;
               DMFTRACE( DMFT_GENERAL, TRACE_LEVEL_INFORMATION, "%!FUNC!IDirect3DDeviceManager9 was not passed in");
           }
           //
           // set it on the pins. Can happen anytime
           //
           for (DWORD dwIndex = 0; dwIndex < (DWORD)m_InPins.size(); dwIndex++)
           {
               m_InPins[dwIndex]->SetD3DManager(m_spDeviceManagerUnk.Get());
           }
           for (DWORD dwIndex = 0; dwIndex < (DWORD)m_OutPins.size(); dwIndex++)
           {
               m_OutPins[dwIndex]->SetD3DManager(m_spDeviceManagerUnk.Get());
           }
        }
        break;
    case MFT_MESSAGE_NOTIFY_BEGIN_STREAMING:
    {
        SetStreamingState( DeviceStreamState_Run );
    }
        break;
    case MFT_MESSAGE_NOTIFY_END_STREAMING:
    {
        SetStreamingState(DeviceStreamState_Stop);
    }
        break;
    case MFT_MESSAGE_NOTIFY_END_OF_STREAM:
    {
        SetStreamingState(DeviceStreamState_Stop);
    }
        break;
    default:
        ;
    }

    DMFTRACE( DMFT_GENERAL, TRACE_LEVEL_INFORMATION, "%!FUNC! exiting %x = %!HRESULT!", hr, hr );
    return hr;
}

IFACEMETHODIMP  CMultipinMft::ProcessInput(
    _In_ DWORD      dwInputStreamID,
    _In_ IMFSample* pSample,
    _In_ DWORD      dwFlags
    )
/*++
    Description:

        Implements IMFTransform::ProcessInput function.This function is called
        when the sourcetransform has input to feed. the pins will try to deliver the 
        samples to the active output pins conencted. if none are connected then just
        returns the sample back to the source transform

--*/
{
    HRESULT     hr = S_OK;
    UNREFERENCED_PARAMETER( dwFlags );
    CAutoLock lock(m_critSec);

    ComPtr<CInPin> spInPin = GetInPin( dwInputStreamID );
    DMFTCHECKNULL_GOTO(spInPin, done, MF_E_INVALIDSTREAMNUMBER);

    if ( !IsStreaming() )
    {
        goto done;
    }

    DMFTCHECKHR_GOTO(spInPin->SendSample( pSample ), done );
done:
    DMFTRACE( DMFT_GENERAL, TRACE_LEVEL_INFORMATION, "%!FUNC! exiting %x = %!HRESULT!", hr, hr );
    //
    //@@@@ README : There is a bug in the sample that the device transform manager which manages the
    // device MFT does not release the sample after passing it to Device MFT in processInput like it should. The
    // Device MFT therefore unfortunately has to make sure that the sample that leaves processoutput has a reference count of 1
    // 
    SAFE_RELEASE(pSample);
    return hr;

}

IFACEMETHODIMP  CMultipinMft::ProcessOutput(
    _In_    DWORD                       dwFlags,
    _In_    DWORD                       cOutputBufferCount,
    _Inout_updates_(cOutputBufferCount)  MFT_OUTPUT_DATA_BUFFER  *pOutputSamples,
    _Out_   DWORD                       *pdwStatus
)
/*++
Description:

Implements IMFTransform::ProcessOutput function. This is called by the DTM when
the DT indicates it has samples to give. The DTM will send enough MFT_OUTPUT_DATA_BUFFER
pointers to be filled up as is the number of output pins available. The DT should traverse its
output pins and populate the corresponding MFT_OUTPUT_DATA_BUFFER with the samples available

--*/
{
    HRESULT     hr      = S_OK;
    BOOL       gotOne   = false;
    ComPtr<COutPin> spOpin;
    UNREFERENCED_PARAMETER( dwFlags );

    if (cOutputBufferCount > m_OutputPinCount )
    {
        DMFTCHECKHR_GOTO( E_INVALIDARG, done );
    }
    *pdwStatus = 0;

    for ( DWORD i = 0; i < cOutputBufferCount; i++ )
    {
        DWORD dwStreamID = pOutputSamples[i].dwStreamID;
        {
            CAutoLock _lock(m_critSec);
            spOpin = nullptr;
            spOpin = GetOutPin(dwStreamID);
            GUID     pinGuid = GUID_NULL;
            DMFTCHECKNULL_GOTO(spOpin.Get(), done, E_INVALIDARG);
        }
        if ( SUCCEEDED(spOpin->ProcessOutput( dwFlags, &pOutputSamples[i],
            pdwStatus ) ) )
        {
            if (pOutputSamples[i].pSample)
            {
                ProcessMetadata(pOutputSamples[i].pSample);
            }
            gotOne = true;
        }
    }
    if (gotOne)
    {
        hr = S_OK;
    }
     
done:
    DMFTRACE(DMFT_GENERAL, TRACE_LEVEL_INFORMATION, "%!FUNC! exiting %x = %!HRESULT!", hr, hr);
    return hr;
}

IFACEMETHODIMP  CMultipinMft::GetInputStreamAttributes(
    _In_        DWORD           dwInputStreamID,
    _COM_Outptr_result_maybenull_ IMFAttributes** ppAttributes
    )
/*++
    Description:

        Implements IMFTransform::GetInputStreamAttributes function. This function
        gets the specified input stream's attributes.

--*/
{
    HRESULT hr = S_OK;
    ComPtr<CInPin> spIPin;
    CAutoLock Lock(m_critSec);

    DMFTCHECKNULL_GOTO( ppAttributes, done, E_INVALIDARG );
    *ppAttributes = nullptr;

    spIPin = GetInPin( dwInputStreamID );

    DMFTCHECKNULL_GOTO(spIPin, done, E_INVALIDARG );

    hr  = spIPin->getPinAttributes(ppAttributes);

done:
    DMFTRACE(DMFT_GENERAL, TRACE_LEVEL_INFORMATION, "%!FUNC! exiting %x = %!HRESULT!", hr, hr);
    return hr;
}

IFACEMETHODIMP  CMultipinMft::GetOutputStreamAttributes(
    _In_        DWORD           dwOutputStreamID,
    _Out_ IMFAttributes** ppAttributes
    )
/*++
    Description:

        Implements IMFTransform::GetOutputStreamAttributes function. This function
        gets the specified output stream's attributes.

--*/
{
    HRESULT hr = S_OK;
    ComPtr<COutPin> spoPin;
    CAutoLock Lock(m_critSec);

    DMFTCHECKNULL_GOTO(ppAttributes, done, E_INVALIDARG);

    *ppAttributes = nullptr;

    spoPin = GetOutPin(dwOutputStreamID);

    DMFTCHECKNULL_GOTO(spoPin, done, E_INVALIDARG );

    DMFTCHECKHR_GOTO(spoPin->getPinAttributes(ppAttributes), done );
done:
    DMFTRACE(DMFT_GENERAL, TRACE_LEVEL_INFORMATION, "%!FUNC! exiting %x = %!HRESULT!", hr, hr);
    return hr;
}

_Requires_no_locks_held_
IFACEMETHODIMP CMultipinMft::SetInputStreamState(
    _In_    DWORD               dwStreamID,
    _In_    IMFMediaType        *pMediaType,
    _In_    DeviceStreamState   value,
    _In_    DWORD               dwFlags
    )
    /*++
    Description:

    Implements IMFdeviceTransform::SetInputStreamState function. 
    Sets the input stream state. 

    The control lock is not taken here. The lock is taken for operations on
    output pins. This operation is a result of the DT notifying the DTM that
    output pin change has resulted in the need for the input to be changed. In
    this case the DTM sends a getpreferredinputstate and then this call

    --*/
{
    HRESULT hr = S_OK;
    ComPtr<CInPin> spiPin = GetInPin(dwStreamID);
    CAutoLock Lock(m_critSec);

    DMFTCHECKNULL_GOTO(spiPin, done, MF_E_INVALIDSTREAMNUMBER);
    
    DMFTCHECKHR_GOTO(spiPin->SetInputStreamState(pMediaType, value, dwFlags),done);
   
done:
    DMFTRACE(DMFT_GENERAL, TRACE_LEVEL_INFORMATION, "%!FUNC! exiting %x = %!HRESULT!", hr, hr);
    return hr;
}

IFACEMETHODIMP CMultipinMft::GetInputStreamState(
    _In_    DWORD               dwStreamID,
    _Out_   DeviceStreamState   *value
    )
{
    HRESULT hr = S_OK;
    ComPtr<CInPin> piPin = GetInPin(dwStreamID);

    DMFTCHECKNULL_GOTO(piPin, done, MF_E_INVALIDSTREAMNUMBER);
    
    *value =  piPin->GetState();

done:
    DMFTRACE(DMFT_GENERAL, TRACE_LEVEL_INFORMATION, "%!FUNC! exiting %x = %!HRESULT!", hr, hr);
    return hr;
}


IFACEMETHODIMP CMultipinMft::SetOutputStreamState(
    _In_ DWORD                  dwStreamID,
    _In_ IMFMediaType           *pMediaType,
    _In_ DeviceStreamState      state,
    _In_ DWORD                  dwFlags
    )
    /*++
    Description:

    Implements IMFdeviceTransform::SetOutputStreamState function.
    Sets the output stream state. This is called whenever the stream
    is selected or deslected i.e. started or stopped.

    The control lock taken here and this operation should be atomic.
    This function should check the input pins connected to the output pin
    switch off the state of the input pin. Check if any other Pin connected
    to the input pin is in a conflicting state with the state requested on this
    output pin. Accordinly it calculates the media type to be set on the input pin
    and the state to transition into. It then might recreate the other output pins
    connected to it
    --*/
{
    HRESULT hr = S_OK;
    UNREFERENCED_PARAMETER(dwFlags);
    CAutoLock Lock(m_critSec);

    DMFTCHECKHR_GOTO(ChangeMediaTypeEx(dwStreamID, pMediaType, state),done);
   
done:    
    DMFTRACE(DMFT_GENERAL, TRACE_LEVEL_INFORMATION, "%!FUNC! exiting %x = %!HRESULT!", hr, hr);
    return hr;
}

IFACEMETHODIMP CMultipinMft::GetOutputStreamState(
    _In_    DWORD               dwStreamID,
    _Out_   DeviceStreamState   *pState
    )
    /*++
    Description:

    Implements IMFdeviceTransform::GetOutputStreamState function.
    Gets the output stream state.
    Called by the DTM to checks states. Atomic operation. needs a lock
    --*/
{
    HRESULT hr = S_OK;
    CAutoLock lock(m_critSec);

    ComPtr<COutPin> spoPin = GetOutPin(dwStreamID);
    DMFTCHECKNULL_GOTO(pState, done, E_INVALIDARG);
    DMFTCHECKNULL_GOTO(spoPin, done, MF_E_INVALIDSTREAMNUMBER);
    *pState = spoPin->GetState();
done:
    DMFTRACE(DMFT_GENERAL, TRACE_LEVEL_INFORMATION, "%!FUNC! exiting %x = %!HRESULT!", hr, hr);
    return hr;
}

IFACEMETHODIMP CMultipinMft::GetInputStreamPreferredState(
    _In_              DWORD                             dwStreamID,
    _Inout_           DeviceStreamState                 *value,
    _Outptr_opt_result_maybenull_ IMFMediaType          **ppMediaType
    )
    /*++
    Description:

    Implements IMFdeviceTransform::GetInputStreamPreferredState function.
    Gets the preferred state and the media type to be set on the input pin.
    The lock is not held as this will always be called only when we notify
    DTM to call us. We notify DTM only from the context on operations 
    happening on the output pin
    --*/
{
    HRESULT hr = S_OK;
    CAutoLock lock(m_critSec);
    ComPtr<CInPin> spiPin = GetInPin(dwStreamID);
    DMFTCHECKNULL_GOTO(ppMediaType, done, E_INVALIDARG);
    DMFTCHECKNULL_GOTO(spiPin, done, MF_E_INVALIDSTREAMNUMBER);
    hr = spiPin->GetInputStreamPreferredState(value, ppMediaType);
done:
    DMFTRACE(DMFT_GENERAL, TRACE_LEVEL_INFORMATION, "%!FUNC! exiting %x = %!HRESULT!", hr, hr);
   return hr;
}

IFACEMETHODIMP CMultipinMft::FlushInputStream(
    _In_ DWORD dwStreamIndex,
    _In_ DWORD dwFlags
    )
    /*++
    Description:

    Implements IMFdeviceTransform::FlushInputStream function.
    --*/
{
    HRESULT hr = S_OK;
    UNREFERENCED_PARAMETER(dwStreamIndex);
    UNREFERENCED_PARAMETER(dwFlags);
    DMFTRACE(DMFT_GENERAL, TRACE_LEVEL_INFORMATION, "%!FUNC! exiting %x = %!HRESULT!", hr, hr);
    return hr;
}

IFACEMETHODIMP CMultipinMft::FlushOutputStream(
    _In_ DWORD                  dwStreamIndex,
    _In_ DWORD                  dwFlags
    )
    /*++
    Description:

    Implements IMFdeviceTransform::FlushOutputStream function.
    Called by the DTM to flush streams
    --*/
{
    
    HRESULT hr = S_OK;
    UNREFERENCED_PARAMETER(dwFlags);
    CAutoLock Lock(m_critSec);

    ComPtr<COutPin> spoPin = GetOutPin(dwStreamIndex);
    DMFTCHECKNULL_GOTO(spoPin, done, E_INVALIDARG);
    DeviceStreamState oldState = spoPin->SetState(DeviceStreamState_Disabled);
    DMFTCHECKHR_GOTO(spoPin->FlushQueues(),done);
    spoPin->SetState(oldState);
done:
    DMFTRACE(DMFT_GENERAL, TRACE_LEVEL_INFORMATION, "%!FUNC! exiting %x = %!HRESULT!", hr, hr);
    return hr;
}


/*++
    Description:

    Called when the Device Transform gets a MFT_MESSAGE_COMMAND_FLUSH. We drain all the queues. 
    This is called in device source when the source gets end of streaming.
    --*/
IFACEMETHODIMP_(VOID) CMultipinMft::FlushAllStreams(
    VOID
    )
{
    DeviceStreamState oldState;
    CAutoLock Lock(m_critSec);
    for ( DWORD dwIndex = 0, dwSize = (DWORD)m_OutPins.size(); dwIndex < dwSize; dwIndex++ )
    {
        ComPtr<COutPin> spoPin = (COutPin *)m_OutPins[dwIndex].Get();
        oldState = spoPin->SetState(DeviceStreamState_Disabled);
        spoPin->FlushQueues();
        //
        //Restore state
        //
        spoPin->SetState(oldState);
    }
}

//
// IKsControl interface functions
//
IFACEMETHODIMP CMultipinMft::KsProperty(
    _In_reads_bytes_(ulPropertyLength) PKSPROPERTY pProperty,
    _In_ ULONG ulPropertyLength,
    _Inout_updates_bytes_(ulDataLength) LPVOID pvPropertyData,
    _In_ ULONG ulDataLength,
    _Inout_ ULONG* pulBytesReturned
    )
    /*++
    Description:

    Implements IKSProperty::KsProperty function.
    used to pass control commands to the driver (generally)
    This can be used to intercepted the control to figure out
    if it needs to be propogated to the driver or not
    --*/
{
    HRESULT hr = S_OK;
    
    DMFTCHECKHR_GOTO(m_spIkscontrol->KsProperty(pProperty,
        ulPropertyLength,
        pvPropertyData,
        ulDataLength,
        pulBytesReturned),done);
done:
    return hr;
}

IFACEMETHODIMP CMultipinMft::KsMethod(
    _In_reads_bytes_(ulPropertyLength) PKSMETHOD   pMethod,
    _In_ ULONG ulPropertyLength,
    _Inout_updates_bytes_(ulDataLength) LPVOID pvPropertyData,
    _In_ ULONG ulDataLength,
    _Inout_ ULONG* pulBytesReturned
    )
    /*++
    Description:

    Implements IKSProperty::KsMethod function. We can trap ksmethod calls here.
    --*/
{
    HRESULT hr = S_OK;

    DMFTCHECKHR_GOTO(m_spIkscontrol->KsMethod(
        pMethod,
        ulPropertyLength,
        pvPropertyData,
        ulDataLength,
        pulBytesReturned
        ), done);
done:
    return hr;
}

IFACEMETHODIMP CMultipinMft::KsEvent(
    _In_reads_bytes_(ulEventLength) PKSEVENT pEvent,
    _In_ ULONG ulEventLength,
    _Inout_updates_bytes_opt_(ulDataLength) LPVOID pEventData,
    _In_ ULONG ulDataLength,
    _Inout_ ULONG* pBytesReturned
    )
    /*++
    Description:

    Implements IKSProperty::KsEvent function.
    --*/
{

    HRESULT hr = S_OK;
    // Handle the events here if you want, This sample passes the events to the driver
    DMFTCHECKHR_GOTO(m_spIkscontrol->KsEvent(pEvent,
        ulEventLength,
        pEventData,
        ulDataLength,
        pBytesReturned), done);
done:
    return hr;
}

//
// HELPER FUNCTIONS
//

//
// A lock here could mean a deadlock because this will be called when the lock is already held
// in another thread.
//
CInPin* CMultipinMft::GetInPin( 
    _In_ DWORD dwStreamId
)
{
    CInPin *inPin = NULL;
    for (DWORD dwIndex = 0, dwSize = (DWORD)m_InPins.size(); dwIndex < dwSize; dwIndex++)
    {
         inPin = (CInPin *)m_InPins[dwIndex].Get();
        if (dwStreamId == inPin->streamId())
        {
            break;
        }
        inPin = NULL;
    }
    return inPin;
}

COutPin* CMultipinMft::GetOutPin(
    _In_ DWORD dwStreamId
    )
{
    COutPin *outPin = NULL;
    for ( DWORD dwIndex = 0, dwSize = (DWORD) m_OutPins.size(); dwIndex < dwSize; dwIndex++ )
    {
        outPin = ( COutPin * )m_OutPins[ dwIndex ].Get();

        if ( dwStreamId == outPin->streamId() )
        {
            break;
        }

        outPin = NULL;
    }

    return outPin;
}
_Requires_lock_held_(m_Critsec)
HRESULT CMultipinMft::GetConnectedInpin(_In_ ULONG ulOutpin, _Out_ ULONG &ulInPin)
{
    HRESULT hr = S_OK;
    map<int, int>::iterator it = m_outputPinMap.find(ulOutpin);
    if (it != m_outputPinMap.end())
    {
        ulInPin = it->second;
    }
    else
    {
        hr = MF_E_INVALIDSTREAMNUMBER;
    }
    return hr;
}

//
// The Below function changes media type on the pins exposed by device MFT
//
__requires_lock_held(m_critSec)
HRESULT CMultipinMft::ChangeMediaTypeEx(
    _In_        ULONG              pinId,
    _In_opt_    IMFMediaType       *pMediaType,
    _In_        DeviceStreamState  reqState
)
{
    HRESULT hr = S_OK;
    ComPtr<COutPin> spoPin = GetOutPin(pinId);
    ComPtr<CInPin> spinPin;
    DeviceStreamState       oldOutPinState, oldInputStreamState, newOutStreamState, newRequestedInPinState;
    ComPtr<IMFMediaType>    pFullType, pInputMediaType;
    ULONG                   ulInPinId       = 0;
    DWORD                   dwFlags         = 0;
    

    DMFTCHECKNULL_GOTO(spoPin, done, E_INVALIDARG);
   
    if (pMediaType)
    {
        if (!spoPin->IsMediaTypeSupported(pMediaType, &pFullType))
        {
            DMFTCHECKHR_GOTO(MF_E_INVALIDMEDIATYPE, done);
        }
    }
 
    DMFTCHECKHR_GOTO(GetConnectedInpin(pinId, ulInPinId), done);
    spinPin = GetInPin(ulInPinId); // Get the input pin
  
    (VOID)spinPin->getMediaType(&pInputMediaType);
    oldInputStreamState = spinPin->SetState(DeviceStreamState_Disabled); // Disable input pin
    oldOutPinState      = spoPin->SetState(DeviceStreamState_Disabled);  // Disable output pin
    (void)spoPin->FlushQueues();  // Flush the output queues
    (void)spinPin->FlushQueues(); // Flush the input queues
    newOutStreamState = pinStateTransition[oldOutPinState][reqState];  // New state needed  
    
    // The Old input and the output pin states should be the same
    newRequestedInPinState = newOutStreamState;
   
    if ((newOutStreamState != oldOutPinState) /*State change*/
        ||((pFullType.Get() != nullptr) && (pInputMediaType.Get()!=nullptr) && (S_OK != (pFullType->IsEqual(pInputMediaType.Get(), &dwFlags)))) /*Media Types dont match*/
        ||((pFullType == nullptr)||(pInputMediaType == nullptr))/*Either one of the mediatypes is null*/
        )
    {
        //
        // State has change or media type has changed so we need to change the media type on the 
        // underlying kernel pin
        //
        DMFTRACE(DMFT_GENERAL, TRACE_LEVEL_INFORMATION, "Changing Mediatype on the input ");
        spinPin->setPreferredMediaType(pFullType.Get());
        spinPin->setPreferredStreamState(newRequestedInPinState);
        // Let the pipline know that the input needs to be changed. 
        SendEventToManager(METransformInputStreamStateChanged, GUID_NULL, spinPin->streamId());
        //
        //  The media type will be set on the input pin by the time we return from the wait
        //          
        m_critSec.Unlock();
        hr = spinPin->WaitForSetInputPinMediaChange();
        m_critSec.Lock();
        // Change the media type on the output..
        DMFTCHECKHR_GOTO(spoPin->ChangeMediaTypeFromInpin( pMediaType , reqState), done);
        //
        // Notify the pipeline that the output stream media type has changed
        //
        DMFTCHECKHR_GOTO(SendEventToManager(MEUnknown, MEDeviceStreamCreated, spoPin->streamId()), done);
        spoPin->SetFirstSample(TRUE);
    }
    else
    {
        // Restore back old states as we have nothing to do
        spinPin->SetState(oldInputStreamState);
        spoPin->SetState(oldOutPinState);
    }
    

done:
    return hr;
}

//
// The below function sends events to the pipeline.
//

HRESULT CMultipinMft::SendEventToManager(
        _In_ MediaEventType eventType,
        _In_ REFGUID        pGuid,
        _In_ UINT32         context
        )
        /*++
        Description:
        Used to send the event to DTM.
        --*/
    {
        HRESULT                 hr      = S_OK;
        ComPtr<IMFMediaEvent>  pEvent  = nullptr;    

        DMFTCHECKHR_GOTO(MFCreateMediaEvent(eventType, pGuid, S_OK, NULL, &pEvent ),done);   
        DMFTCHECKHR_GOTO(pEvent->SetUINT32(MF_EVENT_MFT_INPUT_STREAM_ID, (ULONG)context),done);
        DMFTCHECKHR_GOTO(QueueEvent(pEvent.Get()),done);
    done:

         return hr;
    }
/*++
Description:
This function connects the input and output pins.
Any media type filtering can  happen here
--*/
HRESULT CMultipinMft::BridgeInputPinOutputPin(
    _In_ CInPin* piPin,
    _In_ COutPin* poPin
    )
{
    HRESULT hr                      = S_OK;
    ULONG   ulIndex                 = 0;
    ULONG   ulAddedMediaTypeCount   = 0;
    ComPtr<IMFMediaType> spMediaType;

    DMFTCHECKNULL_GOTO( piPin, done, E_INVALIDARG );
    DMFTCHECKNULL_GOTO( poPin, done, E_INVALIDARG );

    while ( SUCCEEDED( hr = piPin->GetMediaTypeAt( ulIndex++, spMediaType.ReleaseAndGetAddressOf() )))
    {
        DMFTCHECKHR_GOTO(hr = poPin->AddMediaType(NULL, spMediaType.Get() ), done );
        ulAddedMediaTypeCount++;
    }

    if (ulAddedMediaTypeCount == 0)
    {
        DMFTRACE(DMFT_GENERAL, TRACE_LEVEL_INFORMATION, "%!FUNC! Make Sure Pin %d has one media type exposed ", piPin->streamId());
        DMFTCHECKHR_GOTO( MF_E_INVALID_STREAM_DATA, done ); 
    }
    //
    //Add the Input Pin to the output Pin
    //
    DMFTCHECKHR_GOTO(poPin->AddPin(piPin->streamId()), done);
    hr = ExceptionBoundary([&](){
        //
        // Add the output pin to the input pin. 
        // Create the pin map. So that we know which pin input pin is connected to which output pin
        //
        piPin->ConnectPin(poPin);
        m_outputPinMap.insert(std::pair< int, int >(poPin->streamId(), piPin->streamId()));
    });
done:
    //
    //Failed adding media types
    //
    if (FAILED(hr))
    {
        DMFTRACE(DMFT_GENERAL, TRACE_LEVEL_ERROR, "%!FUNC! exiting %x = %!HRESULT!", hr, hr);
    }
    return hr;
}

//
// IMFShutdown interface functions
//

/*++
Description:
Implements the Shutdown from IMFShutdown
--*/
IFACEMETHODIMP CMultipinMft::Shutdown(
    void
    )
{
    CAutoLock Lock(m_critSec);

    for (ULONG ulIndex = 0, ulSize = (ULONG)m_InPins.size(); ulIndex < ulSize; ulIndex++ )
    {
        CInPin *pInPin = static_cast<CInPin *>(m_InPins[ulIndex].Get());

        // Deref on the connected outpins to break reference loop
        (VOID)pInPin->ShutdownPin();
    }
    return ShutdownEventGenerator();
}

//
// Static method to create an instance of the MFT.
//
HRESULT CMultipinMft::CreateInstance(REFIID iid, void **ppMFT)
{
    HRESULT hr = S_OK;
    ComPtr<CMultipinMft> spMFT;
    DMFTCHECKNULL_GOTO(ppMFT, done, E_POINTER);
    spMFT = new (std::nothrow) CMultipinMft();
    DMFTCHECKNULL_GOTO(spMFT.Get(), done, E_OUTOFMEMORY);

    DMFTCHECKHR_GOTO(spMFT->QueryInterface(iid, ppMFT), done);
done:
    DMFTRACE(DMFT_GENERAL, TRACE_LEVEL_INFORMATION, "%!FUNC! exiting %x = %!HRESULT!", hr, hr);
    return hr;
}
