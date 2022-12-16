//*@@@+++@@@@******************************************************************
//
// Microsoft Windows Media Foundation
// Copyright (C) Microsoft Corporation. All rights reserved.
//
//*@@@---@@@@******************************************************************
//

#include "stdafx.h"
#include "multipinmft.h"
#ifdef MF_WPP
#include "multipinmft.tmh"    //--REF_ANALYZER_DONT_REMOVE--
#endif
//
// Note since MFT_UNIQUE_METHOD_NAMES is defined all the functions of IMFTransform have the Mft suffix..
//
extern const CLSID CLSID_HwMFTActivate;

CMultipinMft::CMultipinMft()
:   m_nRefCount( 0 ),
    m_InputPinCount( 0 ),
    m_OutputPinCount( 0 ),
    m_dwWorkQueueId ( MFASYNC_CALLBACK_QUEUE_MULTITHREADED ),
    m_lWorkQueuePriority ( 0 ),
    m_spAttributes( nullptr ),
    m_spSourceTransform( nullptr ),
    m_SymbolicLink(nullptr)
#if defined (MF_DEVICEMFT_PHTOTOCONFIRMATION)
   , m_spPhotoConfirmationCallback(nullptr)
   , m_firePhotoConfirmation(FALSE)
#endif
#if defined (MF_DEVICEMFT_WARMSTART_HANDLING)
    , m_dwWarmStartMask(0)
#endif

{
    HRESULT hr = S_OK;
    ComPtr<IMFAttributes> pAttributes = nullptr;
    MFCreateAttributes( &pAttributes, 0 );
    DMFTCHECKHR_GOTO(pAttributes->SetUINT32( MF_TRANSFORM_ASYNC, TRUE ),done);
    DMFTCHECKHR_GOTO(pAttributes->SetUINT32( MFT_SUPPORT_DYNAMIC_FORMAT_CHANGE, TRUE ),done);
    DMFTCHECKHR_GOTO(pAttributes->SetUINT32( MF_SA_D3D_AWARE, TRUE ), done);
    DMFTCHECKHR_GOTO(pAttributes->SetString( MFT_ENUM_HARDWARE_URL_Attribute, L"SampleMultiPinMft" ),done);
    m_spAttributes = pAttributes;
#if defined (MF_DEVICEMFT_PHTOTOCONFIRMATION)
    m_guidPhotoConfirmationSubtype = MFVideoFormat_NV12;
#endif
done:
    if (FAILED(hr))
    {

    }
}

CMultipinMft::~CMultipinMft( )
{
    m_InPins.clear();
    m_OutPins.clear();
    SAFE_ARRAYDELETE(m_SymbolicLink);
    m_spSourceTransform = nullptr;
}

STDMETHODIMP_(ULONG) CMultipinMft::AddRef(
    void
    )
{
    return InterlockedIncrement(&m_nRefCount);
}

STDMETHODIMP_(ULONG) CMultipinMft::Release(
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

STDMETHODIMP CMultipinMft::QueryInterface(
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
#if defined (MF_DEVICEMFT_ALLOW_MFT0_LOAD) && defined (MFT_UNIQUE_METHOD_NAMES)
    else if (iid == __uuidof(IMFTransform))
    {
        *ppv = static_cast< IMFTransform* >(this);
    }
#endif
    else if ( iid == __uuidof( IKsControl ) )
    {
        *ppv = static_cast< IKsControl* >( this );
    }
    else if ( iid == __uuidof( IMFRealTimeClientEx ) )
    {
        *ppv = static_cast< IMFRealTimeClientEx* >( this );
    }
#if defined (MF_DEVICEMFT_PHTOTOCONFIRMATION)
    else if (iid == __uuidof(IMFCapturePhotoConfirmation))
    {
        *ppv = static_cast< IMFCapturePhotoConfirmation* >(this);
    }
    else if (iid == __uuidof(IMFGetService))
    {
        *ppv = static_cast< IMFGetService* >(this);
    }
#endif
#if ((defined NTDDI_WIN10_VB) && (NTDDI_VERSION >= NTDDI_WIN10_VB))
    else if (iid == __uuidof(IMFSampleAllocatorControl))
    {
        *ppv = static_cast<IMFSampleAllocatorControl*>(this);
    }
#endif
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

// This sample will create a grayscale for known media types. Please remove MF_DEVICEMFT_ADD_GRAYSCALER_ to remove the grayscaler
// This sample also has photo confirmation enabled remove DMF_DEVICEMFT_PHTOTOCONFIRMATION to remove photo confirmation
// Please search for the @@@@ README tag for critical sections in code and it's documentation
//
STDMETHODIMP CMultipinMft::InitializeTransform ( 
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
    CPinCreationFactory*    pPinFactory = new (std::nothrow) CPinCreationFactory(this);
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
    
    DMFTCHECKHR_GOTO( m_spSourceTransform->GetStreamCount( &inputStreams, &outputStreams ), done );

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

        //
        // Output pins from DevProxy = Input pins of device MFT.. We are the first transform in the pipeline before MFT0
        //
        
        for ( ULONG ulIndex = 0; ulIndex < outputStreams; ulIndex++ )
        {           
            ComPtr<IMFAttributes>   pInAttributes   = nullptr;
            BOOL                    bCustom         = FALSE;
            ComPtr<CInPin>          spInPin;

            DMFTCHECKHR_GOTO(pPinFactory->CreatePin(
                pcOutputStreams[ulIndex], /*Input Pin ID as advertised by the pipeline*/
                0, /*This is not needed for Input Pin*/
                CPinCreationFactory::DMFT_PIN_INPUT, /*Input Pin*/
                (CBasePin**)spInPin.GetAddressOf(),
                bCustom), done);
            if (bCustom)
            {
                m_CustomPinCount++;
            }
            hr = ExceptionBoundary([&]()
            {
                m_InPins.push_back(spInPin.Get());
            });
            DMFTCHECKHR_GOTO(hr, done);
            DMFTCHECKHR_GOTO( spInPin->Init(m_spSourceTransform.Get() ), done);
        }
        
        //
        // Create one on one mapping
        //
        for (ULONG ulIndex = 0; ulIndex < m_InPins.size(); ulIndex++)
        {
            
            ComPtr<COutPin> spoPin;
            BOOL     bCustom = FALSE;
            ComPtr<CInPin> spiPin = ( CInPin * )m_InPins[ ulIndex ].Get();
            
            if (spiPin.Get())
            {
                BOOL isCustom = false;
                if (SUCCEEDED(CheckCustomPin(spiPin.Get(), &isCustom)) && (isCustom))
                {
                    //
                    // In this sample we are not connecting the custom pin to the output
                    // This is because we really have no way of testing the custom pin with the
                    // pipeline. 
                    // This however can be changed if the custom media type is converted here in
                    // the device MFT and later exposed to the pipeline..
                    //
                    continue;
                }

                DMFTCHECKHR_GOTO(pPinFactory->CreatePin(spiPin->streamId(), /*Input Pin connected to the Output Pin*/
                    ulOutPinIndex, /*Output pin Id*/
                    CPinCreationFactory::DMFT_PIN_OUTPUT, /*Output pin */
                    (CBasePin**)spoPin.ReleaseAndGetAddressOf(),
                    bCustom), done);
                hr = BridgeInputPinOutputPin(spiPin.Get(), spoPin.Get());
                if (SUCCEEDED(hr))
                {
                    DMFTCHECKHR_GOTO(ExceptionBoundary([&]()
                    {
                        m_OutPins.push_back(spoPin.Get());
                    }), done);
                    ulOutPinIndex++;
                    hr = S_OK;
                }
                if (hr == MF_E_INVALID_STREAM_DATA)
                {
                    // Skip the pin which doesn't have any mediatypes exposed
                    hr = S_OK;
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
    SAFE_DELETE(pPinFactory);
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


STDMETHODIMP CMultipinMft::SetWorkQueueEx(
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
STDMETHODIMP  CMultipinMft::GetStreamCount(
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
STDMETHODIMP  CMultipinMft::GetStreamIDs(
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
STDMETHODIMP  CMultipinMft::GetInputAvailableType(
    _In_        DWORD           dwInputStreamID,
    _In_        DWORD           dwTypeIndex,
    _Out_ IMFMediaType**        ppMediaType
    )
{
    HRESULT hr = S_OK;
    CAutoLock   lock(m_critSec);
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

STDMETHODIMP CMultipinMft::GetOutputAvailableType(
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

STDMETHODIMP  CMultipinMft::GetInputCurrentType(
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

STDMETHODIMP  CMultipinMft::GetOutputCurrentType(
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


STDMETHODIMP  CMultipinMft::ProcessEvent(
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



STDMETHODIMP  CMultipinMft::ProcessMessage(
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
    
    printMessageEvent( eMessage );

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
        //
        // Start Streaming custom pins if the device transfrom has any
        //
        SetStreamingStateCustomPins( DeviceStreamState_Run );
    }
        break;
    case MFT_MESSAGE_NOTIFY_END_STREAMING:
    {
        SetStreamingState(DeviceStreamState_Stop);
        //
        // Stop streaming custom pins if the device transform has any
        //
        SetStreamingStateCustomPins( DeviceStreamState_Stop );
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

STDMETHODIMP  CMultipinMft::ProcessInput(
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
    CAutoLock   lock(m_critSec);
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

STDMETHODIMP  CMultipinMft::ProcessOutput(
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
    CAutoLock _lock(m_critSec);
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
            spOpin = nullptr;
            spOpin = GetOutPin(dwStreamID);
            GUID     pinGuid = GUID_NULL;
            DMFTCHECKNULL_GOTO(spOpin.Get(), done, E_INVALIDARG);
        }
        if ( SUCCEEDED(spOpin->ProcessOutput( dwFlags, &pOutputSamples[i],
            pdwStatus ) ) )
        {
            gotOne = true;
            // Do photo confirmation if enabled from the preview stream only
#if defined (MF_DEVICEMFT_PHTOTOCONFIRMATION)
            BOOL pIsPreviewPin = FALSE;
            if (pOutputSamples[i].pSample &&
                IsPhotoConfirmationEnabled() &&
                ((SUCCEEDED(CheckPreviewPin(static_cast<IMFAttributes*>(spOpin.Get()), &pIsPreviewPin)) && pIsPreviewPin) &&
                    InterlockedCompareExchange(reinterpret_cast<PLONG>(&m_firePhotoConfirmation), FALSE, TRUE)))
            {
                // Please note photo confirmation should always be fired from the preview stream.
                ComPtr<IMFMediaType> spMediaType;
                if (SUCCEEDED(spOpin->getMediaType(spMediaType.GetAddressOf())))
                {
                    // Do Photo confirmation
                    ProcessCapturePhotoConfirmationCallBack(spMediaType.Get(), pOutputSamples[i].pSample);
                    m_firePhotoConfirmation = FALSE;
                }
            }
#endif
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

STDMETHODIMP  CMultipinMft::GetInputStreamAttributes(
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

STDMETHODIMP  CMultipinMft::GetOutputStreamAttributes(
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
STDMETHODIMP CMultipinMft::SetInputStreamState(
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
    CAutoLock Lock(m_critSec);
    ComPtr<CInPin> spiPin = GetInPin(dwStreamID);
    DMFTCHECKNULL_GOTO(spiPin, done, MF_E_INVALIDSTREAMNUMBER);
    
    DMFTCHECKHR_GOTO(spiPin->SetInputStreamState(pMediaType, value, dwFlags),done);
   
done:
    DMFTRACE(DMFT_GENERAL, TRACE_LEVEL_INFORMATION, "%!FUNC! exiting %x = %!HRESULT!", hr, hr);
    return hr;
}

STDMETHODIMP CMultipinMft::GetInputStreamState(
    _In_    DWORD               dwStreamID,
    _Out_   DeviceStreamState   *value
    )
{
    HRESULT hr = S_OK;
    CAutoLock Lock(m_critSec);
    ComPtr<CInPin> piPin = GetInPin(dwStreamID);

    DMFTCHECKNULL_GOTO(piPin, done, MF_E_INVALIDSTREAMNUMBER);
    
    *value =  piPin->GetState();

done:
    DMFTRACE(DMFT_GENERAL, TRACE_LEVEL_INFORMATION, "%!FUNC! exiting %x = %!HRESULT!", hr, hr);
    return hr;
}


STDMETHODIMP CMultipinMft::SetOutputStreamState(
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

STDMETHODIMP CMultipinMft::GetOutputStreamState(
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

STDMETHODIMP CMultipinMft::GetInputStreamPreferredState(
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

STDMETHODIMP CMultipinMft::FlushInputStream(
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

STDMETHODIMP CMultipinMft::FlushOutputStream(
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
STDMETHODIMP_(VOID) CMultipinMft::FlushAllStreams(
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
STDMETHODIMP CMultipinMft::KsProperty(
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
    UNREFERENCED_PARAMETER(pulBytesReturned);
    DMFTCHECKNULL_GOTO(pProperty, done, E_INVALIDARG);
    DMFTCHECKNULL_GOTO(pulBytesReturned, done, E_INVALIDARG);
    
    //
    // Enable Warm Start on All filters for the sample. Please comment out this
    // section if this is not needed
    //
    if (IsEqualCLSID(pProperty->Set, KSPROPERTYSETID_ExtendedCameraControl)
        && (pProperty->Id == KSPROPERTY_CAMERACONTROL_EXTENDED_WARMSTART))
    {
#if MF_DEVICEMFT_WARMSTART_HANDLING
        DMFTCHECKHR_GOTO(WarmStartHandler(pProperty,
            ulPropertyLength, pvPropertyData, ulDataLength, pulBytesReturned),done);
        goto done;
#endif
        DMFTRACE(DMFT_GENERAL, TRACE_LEVEL_INFORMATION, "%!FUNC! Warm Start Control %d Passed ", pProperty->Id);
    }

    if (IsEqualCLSID(pProperty->Set, KSPROPERTYSETID_ExtendedCameraControl))
    {
        DMFTRACE(DMFT_GENERAL, TRACE_LEVEL_INFORMATION, "%!FUNC! Extended Control %d Passed ",pProperty->Id);
    }
    else if ((IsEqualCLSID(pProperty->Set, PROPSETID_VIDCAP_VIDEOCONTROL)) && (pProperty->Id == KSPROPERTY_VIDEOCONTROL_MODE))
    {
        // A function illustrating how we can capture and service photos from the device MFT. This block shows how we can
        // intercept Photo triggers going down to the pipeline
        
        if (sizeof(KSPROPERTY_VIDEOCONTROL_MODE_S) == ulDataLength)
        {
            PKSPROPERTY_VIDEOCONTROL_MODE_S VideoControl = (PKSPROPERTY_VIDEOCONTROL_MODE_S)pvPropertyData;
            m_PhotoModeIsPhotoSequence = false;
            if (VideoControl->Mode == KS_VideoControlFlag_StartPhotoSequenceCapture)
            {
                DMFTRACE(DMFT_GENERAL, TRACE_LEVEL_INFORMATION, "%!FUNC! Starting PhotoSequence Trigger");
                m_PhotoModeIsPhotoSequence = true;
            }
            else if (VideoControl->Mode == KS_VideoControlFlag_StopPhotoSequenceCapture)
            {
                DMFTRACE(DMFT_GENERAL, TRACE_LEVEL_INFORMATION, "%!FUNC! Stopping PhotoSequence Trigger");
                m_PhotoModeIsPhotoSequence = false;
            }
            else
            {
                DMFTRACE(DMFT_GENERAL, TRACE_LEVEL_INFORMATION, "%!FUNC! Take Single Photo Trigger");
#if defined (MF_DEVICEMFT_PHTOTOCONFIRMATION)
                InterlockedExchange(reinterpret_cast<PLONG>(&m_firePhotoConfirmation),TRUE);
#endif
            }
        }
    }
    DMFTCHECKHR_GOTO(m_spIkscontrol->KsProperty(pProperty,
        ulPropertyLength,
        pvPropertyData,
        ulDataLength,
        pulBytesReturned),done);
done:
    LPSTR guidStr = DumpGUIDA(pProperty->Set);
    DMFTRACE(DMFT_GENERAL, TRACE_LEVEL_INFORMATION, "%!FUNC! g:%s p:%d exiting %x = %!HRESULT!", guidStr, pProperty->Id, hr, hr);
    delete(guidStr);
    return hr;
}

STDMETHODIMP CMultipinMft::KsMethod(
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
    return m_spIkscontrol->KsMethod(
        pMethod,
        ulPropertyLength,
        pvPropertyData,
        ulDataLength,
        pulBytesReturned
        );
}

STDMETHODIMP CMultipinMft::KsEvent(
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
#if MF_DEVICEMFT_WARMSTART_HANDLING
    if (pEvent && (pEvent->Set == KSEVENTSETID_ExtendedCameraControl) &&
        (pEvent->Id == KSPROPERTY_CAMERACONTROL_EXTENDED_WARMSTART))
    {
        DMFTRACE(DMFT_GENERAL, TRACE_LEVEL_INFORMATION, "%!FUNC! Acquiring Event for Async Extended Control");
        //
        // For the Sample the warmstate handlers are supported by Device MFT and not passed to driver
        //
       hr = m_eventHandler.KSEvent(pEvent,
            ulEventLength,
            pEventData,
            ulDataLength,
            pBytesReturned
            );
    }
    else
#endif
    {

        //
        // All the Events we don't handle should be sent to the driver!
        //
        hr = m_spIkscontrol->KsEvent(pEvent,
            ulEventLength,
            pEventData,
            ulDataLength,
            pBytesReturned);
    }
    DMFTRACE(DMFT_GENERAL, TRACE_LEVEL_INFORMATION, "%!FUNC! exiting %x = %!HRESULT!", hr, hr);
    return hr;
}


#if defined (MF_DEVICEMFT_PHTOTOCONFIRMATION)
//
//IMFGetService functions
//

STDMETHODIMP  CMultipinMft::GetService(
    __in REFGUID guidService,
    __in REFIID riid,
    __deref_out LPVOID* ppvObject
    )
{
    //
    //This doesn't necessarily need to be a GetService function, but this is just so that the
    //pipeline implementation and the Device transform implementation is consistent.
    //In this sample the photoconfirmation interface is implementated by the same class
    //we can delegate it later to any of the pins if needed
    //
    UNREFERENCED_PARAMETER(guidService);
    if (riid == __uuidof(IMFCapturePhotoConfirmation))
    {
        return QueryInterface(riid, ppvObject);
    }
    else
        return MF_E_UNSUPPORTED_SERVICE;


}

//
//IMFCapturePhotoConfirmation functions implemented
//

STDMETHODIMP  CMultipinMft::SetPhotoConfirmationCallback(
    _In_ IMFAsyncCallback* pNotificationCallback
    )
{
    CAutoLock Lock(m_critSec);
    DMFTRACE(DMFT_GENERAL, TRACE_LEVEL_INFORMATION, "%!FUNC! Setting PhotoConfirmation %p, is passed", pNotificationCallback);

    m_spPhotoConfirmationCallback = pNotificationCallback;
    return S_OK;
}
STDMETHODIMP  CMultipinMft::SetPixelFormat(
    _In_ GUID subtype
    )
{
    CAutoLock Lock(m_critSec);
    m_guidPhotoConfirmationSubtype = subtype;
    return S_OK;
}
STDMETHODIMP   CMultipinMft::GetPixelFormat(
    _Out_ GUID* subtype
    )
{
    CAutoLock Lock(m_critSec);
    *subtype = m_guidPhotoConfirmationSubtype;
    return S_OK;
}

#endif



#if defined (MF_DEVICEMFT_ALLOW_MFT0_LOAD) && defined (MFT_UNIQUE_METHOD_NAMES)

//
// IMFTransform function(s).
//

//
// Note: This is the only IMFTransform function which is not a redirector to the 
// DeviceTransform functions. The rest of IMFTransform functions are in the file common.h
// This function returns the IMFAttribute created for Device MFT. If DMFT is 
// not loaded (usually )MFT0's call to GetAttributes will get the Attribute store of DevProxy.
// A device MFT loaded will not pass through the devproxy attribute store, but it will pass 
// the device MFT attributes. This should be similar to the singular DevProxy attribute
// which the MFT0 providers can use to synchronize across various MFT0's
//

STDMETHODIMP CMultipinMft::GetAttributes(
    _COM_Outptr_opt_result_maybenull_ IMFAttributes** ppAttributes
    )
{
    HRESULT hr = S_OK;
    CAutoLock Lock(m_critSec);
    DMFTCHECKNULL_GOTO(ppAttributes, done, E_INVALIDARG);
    
    *ppAttributes = nullptr;

    if (m_spAttributes != nullptr)
    {
        m_spAttributes.CopyTo(ppAttributes);
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

done:
    return hr;
}
#endif

#if ((defined NTDDI_WIN10_VB) && (NTDDI_VERSION >= NTDDI_WIN10_VB))
//
// IMFSampleAllocatorControl Inferface function declarations
//

STDMETHODIMP CMultipinMft::SetDefaultAllocator(
    _In_ DWORD dwOutputStreamID,
    _In_ IUnknown *pAllocator
)
{
    CAutoLock Lock(m_critSec);
    
    // SetAllocator will be called on the streamId that returns MFSampleAllocatorMode_Default
    wil::com_ptr_nothrow<COutPin> outPin = GetOutPin(dwOutputStreamID);
    RETURN_HR_IF_NULL(E_INVALIDARG, outPin);
    RETURN_HR_IF_NULL(E_INVALIDARG, pAllocator);

    wil::com_ptr_nothrow<IMFVideoSampleAllocator> defaultAllocator;
    RETURN_IF_FAILED(pAllocator->QueryInterface(&defaultAllocator));
    outPin->SetAllocator(defaultAllocator.get());

    return S_OK;
}

STDMETHODIMP CMultipinMft::GetAllocatorUsage(
    _In_ DWORD dwOutputStreamID,
    _Out_  DWORD* pdwInputStreamID,
    _Out_ MFSampleAllocatorUsage* peUsage
)
{
    CAutoLock Lock(m_critSec);

    RETURN_HR_IF_NULL(E_INVALIDARG, peUsage);

    wil::com_ptr_nothrow<COutPin> outPin = GetOutPin(dwOutputStreamID);
    RETURN_HR_IF_NULL(MF_E_INVALIDSTREAMNUMBER, outPin);
    *peUsage = outPin->GetSampleAllocatorUsage();

    if (*peUsage == MFSampleAllocatorUsage_DoesNotAllocate)
    {
        RETURN_HR_IF_NULL(E_INVALIDARG, pdwInputStreamID);
        RETURN_IF_FAILED(GetConnectedInpin((ULONG)dwOutputStreamID, *(ULONG*)pdwInputStreamID));
    }

    return S_OK;
}
#endif // ((defined NTDDI_WIN10_VB) && (NTDDI_VERSION >= NTDDI_WIN10_VB))

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
    {
        //
        // dump the media types to the logs
        //
       ComPtr<IMFMediaType> spOldMediaType;
       (VOID)spoPin->getMediaType(spOldMediaType.GetAddressOf());
       CMediaTypePrinter newType(pMediaType);
       CMediaTypePrinter oldType(spOldMediaType.Get());
       if (WPP_LEVEL_ENABLED(DMFT_GENERAL))
       {
           DMFTRACE(DMFT_GENERAL, TRACE_LEVEL_INFORMATION, " Pin:%d old MT:[%s] St:%d", pinId, oldType.ToString(), reqState);
           DMFTRACE(DMFT_GENERAL, TRACE_LEVEL_INFORMATION, " Pin:%d new MT:[%s] St:%d", pinId, newType.ToString(), reqState);
       }
    }

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
        DMFTCHECKHR_GOTO(hr, done);
        // Change the media type on the output..
        DMFTCHECKHR_GOTO(spoPin->ChangeMediaTypeFromInpin(pFullType.Get(), pMediaType , reqState), done);
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
    //
    // Copy over the media types from input pin to output pin. Since there is no
    // decoder support, only the uncompressed media types are inserted. Please make
    // sure any pin advertised supports at least one media type. The pipeline doesn't
    // like pins with no media types
    //
    while ( SUCCEEDED( hr = piPin->GetMediaTypeAt( ulIndex++, spMediaType.ReleaseAndGetAddressOf() )))
    {
        GUID subType = GUID_NULL;
        DMFTCHECKHR_GOTO( spMediaType->GetGUID(MF_MT_SUBTYPE,&subType), done );
        {
            DMFTCHECKHR_GOTO(hr = poPin->AddMediaType(NULL, spMediaType.Get() ), done );
            if (hr == S_OK)
            {
                ulAddedMediaTypeCount++;
            }
        }
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
// Look at the below code only if we need to handle an extended property in Device MFT
//
#if defined (MF_DEVICEMFT_WARMSTART_HANDLING)
HRESULT CMultipinMft::WarmStartHandler(
    _In_       PKSPROPERTY Property,
    _In_       ULONG       ulPropertyLength,
    _In_       LPVOID      pData,
    _In_       ULONG       ulOutputBufferLength,
    _Inout_    PULONG      pulBytesReturned
    )
{
    
    HRESULT hr = S_OK;
    UNREFERENCED_PARAMETER( ulPropertyLength );
    *pulBytesReturned = 0;

    if ( Property->Flags & KSPROPERTY_TYPE_SET )
    {
        if ( ulOutputBufferLength == 0 )
        {
            *pulBytesReturned = sizeof( KSCAMERA_EXTENDEDPROP_HEADER )+sizeof( KSCAMERA_EXTENDEDPROP_VALUE );
            DMFTCHECKHR_GOTO( HRESULT_FROM_WIN32( ERROR_MORE_DATA ), done);
        }
        else if (ulOutputBufferLength < sizeof( KSCAMERA_EXTENDEDPROP_HEADER )+sizeof( KSCAMERA_EXTENDEDPROP_VALUE ))
        {
            DMFTCHECKHR_GOTO( HRESULT_FROM_WIN32( ERROR_MORE_DATA ), done);
        }
        else if ( pData && ulOutputBufferLength >= sizeof( KSCAMERA_EXTENDEDPROP_HEADER )+sizeof( KSCAMERA_EXTENDEDPROP_VALUE ))
        {
            PBYTE pPayload = ( PBYTE )pData;
            PKSCAMERA_EXTENDEDPROP_HEADER pExtendedHeader = ( PKSCAMERA_EXTENDEDPROP_HEADER )pPayload;
            //
            //Use the extended value to make changes to the property.. refer documentation
            //PKSCAMERA_EXTENDEDPROP_VALUE pExtendedValue = (PKSCAMERA_EXTENDEDPROP_VALUE)(pPayload + sizeof(KSCAMERA_EXTENDEDPROP_HEADER));
            //
            SetWarmStart(pExtendedHeader->PinId, (pExtendedHeader->Flags & KSCAMERA_EXTENDEDPROP_WARMSTART_MODE_ENABLED));

            *pulBytesReturned = sizeof( PKSCAMERA_EXTENDEDPROP_HEADER )+sizeof( KSCAMERA_EXTENDEDPROP_VALUE );
            m_eventHandler.SetOneShot(KSPROPERTY_CAMERACONTROL_EXTENDED_WARMSTART);
        }
        else
        {
            hr = S_OK;
        }
    }
    else if (Property->Flags & KSPROPERTY_TYPE_GET)
    {
        if (ulOutputBufferLength == 0)
        {
            *pulBytesReturned = sizeof( KSCAMERA_EXTENDEDPROP_HEADER )+sizeof( KSCAMERA_EXTENDEDPROP_VALUE );
            DMFTCHECKHR_GOTO(HRESULT_FROM_WIN32(ERROR_MORE_DATA), done);
        }
        else if (ulOutputBufferLength < sizeof( KSCAMERA_EXTENDEDPROP_HEADER )+sizeof( KSCAMERA_EXTENDEDPROP_VALUE ))
        {
            DMFTCHECKHR_GOTO( HRESULT_FROM_WIN32( ERROR_MORE_DATA ), done );
        }
        else if (pData && ulOutputBufferLength >= sizeof( KSCAMERA_EXTENDEDPROP_HEADER )+sizeof( KSCAMERA_EXTENDEDPROP_VALUE ))
        {
            PBYTE pPayload =  ( PBYTE )pData;
            PKSCAMERA_EXTENDEDPROP_HEADER pExtendedHeader = ( PKSCAMERA_EXTENDEDPROP_HEADER )( pPayload );
            //
            //Use the extended value to make changes to the property.. refer documentation
            //PKSCAMERA_EXTENDEDPROP_VALUE pExtendedValue = (PKSCAMERA_EXTENDEDPROP_VALUE)(pPayload  +sizeof(KSCAMERA_EXTENDEDPROP_HEADER));
            //
            pExtendedHeader->Capability = KSCAMERA_EXTENDEDPROP_CAPS_ASYNCCONTROL | KSCAMERA_EXTENDEDPROP_WARMSTART_MODE_ENABLED;
            pExtendedHeader->Flags = 0;

            if (GetWarmStart(pExtendedHeader->PinId))
y
            {
                pExtendedHeader->Flags |= KSCAMERA_EXTENDEDPROP_WARMSTART_MODE_ENABLED;
            }

            pExtendedHeader->Result = 0;
            pExtendedHeader->Size = sizeof( KSCAMERA_EXTENDEDPROP_HEADER )+sizeof( KSCAMERA_EXTENDEDPROP_VALUE );
            pExtendedHeader->Version = 1;
            *pulBytesReturned = sizeof( KSCAMERA_EXTENDEDPROP_HEADER )+sizeof( KSCAMERA_EXTENDEDPROP_VALUE );
            hr = S_OK;
        }
        else
        {
            hr = S_OK;
        }
    }
done:
    DMFTRACE( DMFT_GENERAL, TRACE_LEVEL_INFORMATION, "%!FUNC! exiting %x = %!HRESULT!", hr, hr );
    return hr;
}
#endif

//
// IMFShutdown interface functions
//

/*++
Description:
Implements the Shutdown from IMFShutdown
--*/
STDMETHODIMP CMultipinMft::Shutdown(
    void
    )
{
    CAutoLock Lock(m_critSec);
    (VOID) m_eventHandler.Clear();

    for (ULONG ulIndex = 0, ulSize = (ULONG)m_InPins.size(); ulIndex < ulSize; ulIndex++ )
    {
        // Deref on the connected outpins to break reference loop
        (VOID)((CInPin*)m_InPins[ulIndex].Get())->ShutdownPin();
    }
#if defined (MF_DEVICEMFT_ALLOW_MFT0_LOAD) && defined (MFT_UNIQUE_METHOD_NAMES)
    for (ULONG ulIndex = 0, ulSize = (ULONG)m_OutPins.size(); ulIndex < ulSize; ulIndex++)
    {
        (VOID) m_OutPins[ulIndex]->DeleteItem(MF_DEVICESTREAM_EXTENSION_PLUGIN_CONNECTION_POINT);
    }
#endif
    return ShutdownEventGenerator();
}

//
// Static method to create an instance of the MFT.
//
HRESULT CMultipinMft::CreateInstance(REFIID iid, void **ppMFT)
{
    HRESULT hr = S_OK;
    CMultipinMft *pMFT = NULL;
    DMFTCHECKNULL_GOTO(ppMFT, done, E_POINTER);
    pMFT = new (std::nothrow) CMultipinMft();
    DMFTCHECKNULL_GOTO(pMFT, done, E_OUTOFMEMORY);

    DMFTCHECKHR_GOTO(pMFT->QueryInterface(iid, ppMFT), done);

done:
    if (FAILED(hr))
    {
        SAFERELEASE(pMFT);
    }
    DMFTRACE(DMFT_GENERAL, TRACE_LEVEL_INFORMATION, "%!FUNC! exiting %x = %!HRESULT!", hr, hr);
    return hr;
}





#if defined (MF_DEVICEMFT_PHTOTOCONFIRMATION)
/*
Desciption:
This function will be called by the preview pin to execute the photo confirmation stored with the
MFT.
*/

STDMETHODIMP CMultipinMft::ProcessCapturePhotoConfirmationCallBack(
    _In_ IMFMediaType* pMediaType,
    _In_ IMFSample*    pSample
    )
{
    //
    //PhotoConfirmation Implementation
    //Note this function doesn't scan the metadata as the pipeline does to find out which buffer is the photoconfirmation buffer
    //The pipeline treats the preview buffer as the photo confirmation buffer and the driver marks the metadata on the buffer as being so.
    //This example treats every buffer coming on the pin as the confirmation buffer.
    //

    HRESULT hr = S_OK;
    ComPtr<IMFMediaType> spMediaType = nullptr;
    LONGLONG timeStamp = 0;

    DMFTCHECKHR_GOTO(MFCreateMediaType(&spMediaType), done);
    DMFTCHECKHR_GOTO(pMediaType->CopyAllItems(spMediaType.Get()), done);

    DMFTCHECKHR_GOTO(pSample->SetUnknown(MFSourceReader_SampleAttribute_MediaType_priv, spMediaType.Get()), done);

    DMFTCHECKHR_GOTO(pSample->GetSampleTime(&timeStamp), done);
    DMFTCHECKHR_GOTO(pSample->SetUINT64(MFSampleExtension_DeviceReferenceSystemTime, timeStamp), done);

    if (m_spPhotoConfirmationCallback)
    {

        //
        //We are directly sending the photo sample over to the consumers of the photoconfirmation interface.
        //
        ComPtr<IMFAsyncResult> spResult;
        DMFTCHECKHR_GOTO(MFCreateAsyncResult(pSample, m_spPhotoConfirmationCallback.Get(), NULL, &spResult), done);
        DMFTCHECKHR_GOTO(MFInvokeCallback(spResult.Get()), done);
    }
done:
    DMFTRACE(DMFT_GENERAL, TRACE_LEVEL_INFORMATION, "%!FUNC! exiting %x = %!HRESULT!", hr, hr);
    return hr;
}

#endif

//
// Only worry about this if you have customs pins defined in the driver
//

HRESULT CMultipinMft::SetStreamingStateCustomPins(
    DeviceStreamState State
    )
{
    HRESULT hr = S_OK;
    
    if ( m_CustomPinCount > 0 )
    {
        DMFTRACE(DMFT_GENERAL, TRACE_LEVEL_INFORMATION, "%!FUNC! Custom Pin State changing to %d", State);

        for (ULONG ulIndex = 0; ulIndex < m_InPins.size(); ulIndex++)
        {
            BOOL isCustom = false;
            CInPin* pInPin = static_cast<CInPin*>(m_InPins[ulIndex].Get());

            if ( SUCCEEDED( CheckCustomPin(pInPin, &isCustom) )
                && ( isCustom ) )
            {
                // Start the custom stream here. This will involve sending an event to the pipeline about the stream needing a state change
                ComPtr<IMFMediaType> spMediaType;
                if ( State == DeviceStreamState_Run )
                {
                    // Only get the media type if we are going into run state
                    DMFTCHECKHR_GOTO(pInPin->GetMediaTypeAt(0, spMediaType.GetAddressOf()), done);
                }
                pInPin->SetState(DeviceStreamState_Disabled);
                pInPin->setPreferredMediaType(spMediaType.Get());
                pInPin->setPreferredStreamState(State);
                //
                // Let the pipline know that the input needs to be changed on the custom pin
                //
                SendEventToManager(METransformInputStreamStateChanged, GUID_NULL, pInPin->streamId());
                //
                // The media type will be set on the input pin by the time we return from the wait.
                // For a custom pin, which is often the stats pin we can skip the wait as this will 
                // simply make the pipeline wait
                //
                DMFTCHECKHR_GOTO(pInPin->WaitForSetInputPinMediaChange(), done);
              }
        }
    }
done:
    DMFTRACE(DMFT_GENERAL, TRACE_LEVEL_INFORMATION, "%!FUNC! exiting %x = %!HRESULT!", hr, hr);
    return hr;
}
