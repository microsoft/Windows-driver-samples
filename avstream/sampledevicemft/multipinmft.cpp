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
#include "multipinmft.tmh"
#endif
//
//Note since MFT_UNIQUE_METHOD_NAMES is defined all the functions of IMFTransform have the Mft suffix..
//
extern const CLSID CLSID_HwMFTActivate;

#if _NEED_MFTLOCKING_
#define MFTLOCKED() {\
    UINT32 punValue = FALSE; \
    hr = GetUINT32(MF_TRANSFORM_ASYNC_UNLOCK, &punValue);\
if (FAILED(hr) || punValue == FALSE){\
    return MF_E_TRANSFORM_ASYNC_LOCKED; \
}\
}
#else

#define MFTLOCKED() 

#endif

CMultipinMft::CMultipinMft()
:   m_nRefCount( 0 ),
    m_InputPinCount( 0 ),
    m_OutputPinCount( 0 ),
    m_dwWorkQueueId ( MFASYNC_CALLBACK_QUEUE_MULTITHREADED ),
    m_lWorkQueuePriority ( 0 ),
    m_attributes( nullptr ),
    m_pSourceTransform( nullptr ),
    m_PhotoTriggerSent(false),
    m_filterHasIndependentPin( false ),
    m_FilterInPhotoSequence( false ),
    m_AsyncPropEvent( nullptr ),
    m_filterInWarmStart(false)
#if defined (MF_DEVICEMFT_PHTOTOCONFIRMATION)
   , m_spPhotoConfirmationCallback(nullptr)
#endif
{
    ComPtr<IMFAttributes> pAttributes = nullptr;
    MFCreateAttributes( &pAttributes, 0 );
    pAttributes->SetUINT32( MF_TRANSFORM_ASYNC, TRUE );
    pAttributes->SetUINT32( MFT_SUPPORT_DYNAMIC_FORMAT_CHANGE, TRUE );
    pAttributes->SetUINT32( MF_SA_D3D_AWARE, TRUE );
    pAttributes->SetString( MFT_ENUM_HARDWARE_URL_Attribute, L"SampleMultiPinMft" );
    m_attributes = pAttributes;
}

CMultipinMft::~CMultipinMft( )
{
    CBasePin *pioPin = NULL;

    for ( ULONG ulIndex = 0, ulSize = (ULONG) m_InPins.size(); ulIndex < ulSize; ulIndex++ )
    {
        pioPin = m_InPins[ ulIndex ];
        SAFERELEASE( pioPin );
    }
    m_InPins.clear();

    for (ULONG ulIndex = 0, ulSize = (ULONG) m_OutPins.size(); ulIndex < ulSize; ulIndex++)
    {
        pioPin = m_OutPins[ ulIndex ];
        SAFERELEASE( pioPin );
    }
    m_OutPins.clear();
    m_pSourceTransform = nullptr;

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
        AddRef();
    }
    else
    if ( iid == __uuidof( IMFMediaEventGenerator ) )
    {
        *ppv = static_cast< IMFMediaEventGenerator* >(this);
        AddRef();
    }
    else
    if ( iid == __uuidof( IMFShutdown ) )
    {
        *ppv = static_cast< IMFShutdown* >( this );
        AddRef();
    }
#if defined (MF_DEVICEMFT_ALLOW_MFT0_LOAD) && defined (MFT_UNIQUE_METHOD_NAMES)
    else
    if (iid == __uuidof(IMFTransform))
    {
        *ppv = static_cast< IMFTransform* >(this);
        AddRef();
    }
#endif
    else
    if ( iid == __uuidof( IKsControl ) )
    {
        *ppv = static_cast< IKsControl* >( this );
        AddRef();
    }
    else
    if ( iid == __uuidof( IMFRealTimeClientEx ) )
    {
        *ppv = static_cast< IMFRealTimeClientEx* >( this );
        AddRef();
    }
#if defined (MF_DEVICEMFT_PHTOTOCONFIRMATION)
    else
    if (iid == __uuidof(IMFCapturePhotoConfirmation))
    {
        *ppv = static_cast< IMFCapturePhotoConfirmation* >(this);
        AddRef();
    }
#endif
    else
    {
        hr = E_NOINTERFACE;
    }
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
    
    DMFTCHECKNULL_GOTO( pAttributes, done, E_INVALIDARG );
    //
    //The attribute passed with MF_DEVICEMFT_CONNECTED_FILTER_KSCONTROL is the source transform. This generally represents a filter
    //This needs to be stored so that we know the device properties. We cache it. We query for the IKSControl which is used to send
    //controls to the driver.
    //
    DMFTCHECKHR_GOTO( pAttributes->GetUnknown( MF_DEVICEMFT_CONNECTED_FILTER_KSCONTROL,IID_PPV_ARGS( &spFilterUnk ) ),done );
    
    DMFTCHECKHR_GOTO( spFilterUnk.As( &m_pSourceTransform ), done );
    
    DMFTCHECKHR_GOTO( m_pSourceTransform.As( &m_ikscontrol ), done );
    
    DMFTCHECKHR_GOTO( m_pSourceTransform->MFTGetStreamCount( &inputStreams, &outputStreams ), done );
    
    spFilterUnk = nullptr;

    //
    //The number of input pins created by the device transform should match the pins exposed by
    //the source transform i.e. outputStreams from SourceTransform or DevProxy = Input pins of the Device MFT
    //
    
    if ( inputStreams > 0 || outputStreams > 0 )
    {
        pcInputStreams = new DWORD[ inputStreams ];
        DMFTCHECKNULL_GOTO( pcInputStreams, done, E_OUTOFMEMORY);

        pcOutputStreams = new DWORD[ outputStreams ];
        DMFTCHECKNULL_GOTO( pcOutputStreams, done, E_OUTOFMEMORY );
        
        DMFTCHECKHR_GOTO( m_pSourceTransform->MFTGetStreamIDs( inputStreams, pcInputStreams,
            outputStreams,
            pcOutputStreams ),done );

        //
        // Output pins from DevProxy = Input pins of device MFT.. We are the first transform in the pipeline before MFT0
        //
        
        for ( ULONG ulIndex = 0; ulIndex < outputStreams; ulIndex++ )
        {           
            ComPtr<IMFAttributes> pInAttributes = nullptr;
            
            DMFTCHECKHR_GOTO( m_pSourceTransform->GetOutputStreamAttributes(pcOutputStreams[ulIndex], pInAttributes.GetAddressOf()), done);
            
            DMFTCHECKHR_GOTO( pInAttributes->GetGUID(MF_DEVICESTREAM_STREAM_CATEGORY, &streamCategory), done);
            
            if ( IsEqualCLSID( streamCategory, PINNAME_IMAGE ) )
            {
                //We have independent pins..
                m_filterHasIndependentPin = true;
            }
            CInPin *pInPin = nullptr;
            if (IsEqualCLSID(streamCategory, AVSTREAM_CUSTOM_PIN_IMAGE))
            {
                pInPin = new CCustomPin( pInAttributes.Get(), pcOutputStreams[ulIndex], this);
                DMFTCHECKNULL_GOTO( pInPin, done, E_OUTOFMEMORY);
                //
                // Since the custom pin is an input pin too we will push it
                // in the input pins list. This is however not being connected
                // to the output in this sample.
                m_CustomPinCount++;
            }
            else
            {
                pInPin = new CInPin( pInAttributes.Get(), pcOutputStreams[ulIndex], this);
                DMFTCHECKNULL_GOTO(pInPin, done, E_OUTOFMEMORY);
            }
            m_InPins.push_back( pInPin );
            
            DMFTCHECKHR_GOTO( pInPin->Init(m_pSourceTransform.Get() ), done);
            
            
            pInPin->AddRef();

        }
        //
        // If we have just one output stream exposed off the source transform create a one to three pin
        //
        if ( ( outputStreams == 1 ) && IsEqualGUID( streamCategory, PINNAME_VIDEO_CAPTURE ) )
        {
            outputStreams = 3;
            outGuids =  new GUID [ outputStreams ];
            DMFTCHECKNULL_GOTO(outGuids, done, E_OUTOFMEMORY);
            
            m_OutPins.resize(outputStreams, nullptr);

            CInPin  *piPin = (CInPin *)m_InPins[0];

            *outGuids           = PINNAME_VIDEO_CAPTURE;
            *( outGuids + 1 )   = PINNAME_VIDEO_PREVIEW;
            *(outGuids + 2)     = PINNAME_IMAGE;    //PINNAME_IMAGE for independent pin and PINNAME_VIDEO_STILL for dependent pin!!

            for ( ULONG ulIndex = 0; ulIndex < outputStreams; ulIndex++ )
            {
                ComPtr<IKsControl> pKsControl = NULL;
                
                DMFTCHECKHR_GOTO( piPin->QueryInterface(IID_PPV_ARGS( &pKsControl ) ), done);

                COutPin *poPin  = new COutPin( ulIndex, this, pKsControl.Get() ); 
                DMFTCHECKNULL_GOTO( poPin,done, E_OUTOFMEMORY );

                DMFTCHECKHR_GOTO( BridgeInputPinOutputPin( piPin, poPin ),done );

                DMFTCHECKHR_GOTO( poPin->SetGUID( MF_DEVICESTREAM_STREAM_CATEGORY, outGuids[ ulIndex ] ),done );
                
                DMFTCHECKHR_GOTO( poPin->SetUINT32( MF_DEVICESTREAM_STREAM_ID, ulIndex ),done );
                
                m_OutPins[ ulIndex ] = poPin;
                poPin->AddRef();

            }
        }
        else
        {
            //
            //Create one on one mapping
            //
            
            for (ULONG ulIndex = 0; ulIndex < m_InPins.size(); ulIndex++)
            {
                ULONG  ulPinIndex               = 0;
                GUID   pinGuid                  = GUID_NULL;
                ComPtr< IKsControl > pKsControl = nullptr;

                CInPin  *piPin = ( CInPin * )m_InPins[ ulIndex ];

                if (piPin)
                {
                    BOOL isCustom = false;
                    if ( SUCCEEDED( CheckCustomPin( piPin, &isCustom )) && ( isCustom ) )
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
                    ulPinIndex = piPin->streamId();

                    DMFTCHECKHR_GOTO(piPin->GetGUID(MF_DEVICESTREAM_STREAM_CATEGORY, &pinGuid), done);

                    DMFTCHECKHR_GOTO(piPin->QueryInterface(IID_PPV_ARGS(&pKsControl)), done);

                    COutPin *poPin = new COutPin(ulPinIndex, this, pKsControl.Get());

                    DMFTCHECKNULL_GOTO(poPin, done, E_OUTOFMEMORY);

                    DMFTCHECKHR_GOTO(poPin->SetGUID(MF_DEVICESTREAM_STREAM_CATEGORY, pinGuid), done);

                    DMFTCHECKHR_GOTO(poPin->SetUINT32(MF_DEVICESTREAM_STREAM_ID, ulPinIndex), done);

#if defined (MF_DEVICEMFT_ALLOW_MFT0_LOAD) && defined (MFT_UNIQUE_METHOD_NAMES)
                    //
                    // If we wish to load MFT0 as well as Device MFT then we should be doing the following
                    // Copy over the GUID attribute MF_DEVICESTREAM_EXTENSION_PLUGIN_CLSID from the input
                    // pin to the output pin. This is because Device MFT is the new face of the filter now
                    // and MFT0 will now get loaded for the output pins exposed from Device MFT rather than
                    // DevProxy!
                    //

                    GUID        guidMFT0 = GUID_NULL;

                    hr = piPin->GetGUID(MF_DEVICESTREAM_EXTENSION_PLUGIN_CLSID, &guidMFT0);

                    if (SUCCEEDED(hr))
                    {
                        //
                        // This stream has an MFT0 .. Attach the GUID to the Outpin pin attribute
                        // The downstream will query this attribute  on the pins exposed from device MFT
                        //
                        DMFTRACE(DMFT_GENERAL, TRACE_LEVEL_INFORMATION, "%!FUNC! setting Mft0 guid on pin %d", ulIndex);

                        DMFTCHECKHR_GOTO(poPin->SetGUID(MF_DEVICESTREAM_EXTENSION_PLUGIN_CLSID, guidMFT0), done);

                        DMFTCHECKHR_GOTO(poPin->SetUnknown(MF_DEVICESTREAM_EXTENSION_PLUGIN_CONNECTION_POINT,
                            static_cast< IUnknown* >(static_cast < IKsControl * >(this))), done);

                    }
                    else
                    {
                        // Reset Error.. MFT0 absence should not be an error
                        hr = S_OK;
                    }
#endif
                    DMFTCHECKHR_GOTO(BridgeInputPinOutputPin(piPin, poPin), done);

                    m_OutPins.push_back(poPin);
                }
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
        while ( m_InPins.size() > 0 )
        {
            CInPin *pInPin = nullptr;
            pInPin = ( CInPin* )m_InPins.back();
            m_InPins.pop_back();
            SAFERELEASE( pInPin );
        }
        while ( m_OutPins.size() > 0 )
        {
            COutPin *pin = nullptr;
            pin = ( COutPin* )m_OutPins.back();
            m_OutPins.pop_back();
            SAFERELEASE( pin );
        }
        //
        // Simply clear the custom pins since the input pins must have deleted the pin
        //
        m_pSourceTransform = nullptr;
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
    // Cache the WorkQueuId and WorkItemBasePriority
    //
    m_dwWorkQueueId = dwWorkQueueId;
    m_lWorkQueuePriority = lWorkItemBasePriority;
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
    
  
    *pdwInputStreams     = m_InputPinCount;
    *pdwOutputStreams    = m_OutputPinCount;

    DMFTRACE( DMFT_GENERAL, TRACE_LEVEL_INFORMATION, "%!FUNC! exiting %x = %!HRESULT!", hr, hr );
    return hr;
}

//
//Doesn't striclt conform to GetStreamIDs on IMFTransform Interface!
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
    
    MFTLOCKED();
    
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
    _Out_ IMFMediaType**  ppMediaType
    )
{
    HRESULT hr = S_OK;
    MFTLOCKED();

    
    CInPin *piPin = ( CInPin* )GetInPin( dwInputStreamID );

    DMFTCHECKNULL_GOTO( piPin, done, MF_E_INVALIDSTREAMNUMBER );
    
    *ppMediaType = nullptr;

    hr = piPin->GetOutputAvailableType( dwTypeIndex,ppMediaType );

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
    MFTLOCKED();
    
    DMFTCHECKNULL_GOTO(ppMediaType, done, E_INVALIDARG);

    COutPin*poPin = ( COutPin* )GetOutPin( dwOutputStreamID );
    
    DMFTCHECKNULL_GOTO( poPin, done, MF_E_INVALIDSTREAMNUMBER );

    *ppMediaType = nullptr;

    hr = poPin->GetOutputAvailableType( dwTypeIndex, ppMediaType );

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
    //The input current types will not come to this transform.
    //The outputs of this transform matter. The DTM manages the
    //output of this transform and the inptuts of the source transform
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
    MFTLOCKED();
    CAutoLock lock( m_critSec );

    DMFTCHECKNULL_GOTO( ppMediaType, done, E_INVALIDARG );
    
    *ppMediaType = nullptr;

    COutPin *poPin = ( COutPin* )GetOutPin( dwOutputStreamID );

    DMFTCHECKNULL_GOTO( poPin, done, MF_E_INVALIDSTREAMNUMBER );

    DMFTCHECKHR_GOTO( poPin->getMediaType( ppMediaType ),done );
    
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
    MFTLOCKED();

    UNREFERENCED_PARAMETER(ulParam);
    
    CAutoLock _lock( m_critSec );
    
    printMessageEvent( eMessage );

    switch ( eMessage )
    {
    case MFT_MESSAGE_COMMAND_FLUSH:
        //
        //This is MFT wide flush.. Flush all output pins
        //
        (VOID)FlushAllStreams();
        break;
    case MFT_MESSAGE_COMMAND_DRAIN:
        //
        //There is no draining for Device MFT. Just kept here for reference
        //
        break;
    case MFT_MESSAGE_NOTIFY_START_OF_STREAM:
        //
        //No op for device MFTs
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

    MFTLOCKED();

    CInPin *inPin = ( CInPin* )GetInPin( dwInputStreamID );
    DMFTCHECKNULL_GOTO( inPin, done, E_INVALIDARG );

    if ( !IsStreaming() )
    {
        goto done;
    }

    DMFTCHECKHR_GOTO( inPin->SendSample( pSample ), done );

    QueueEvent( METransformHaveOutput, GUID_NULL, S_OK, NULL );
   
done:
    SAFERELEASE( pSample );
    DMFTRACE( DMFT_GENERAL, TRACE_LEVEL_INFORMATION, "%!FUNC! exiting %x = %!HRESULT!", hr, hr );
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
    MFTLOCKED();
    UNREFERENCED_PARAMETER( dwFlags );

    if (cOutputBufferCount > m_OutputPinCount )
    {
        DMFTCHECKHR_GOTO( E_INVALIDARG, done );
    }
    *pdwStatus = 0;

    for ( DWORD i = 0; i < cOutputBufferCount; i++ )
    {
        DWORD dwStreamID = pOutputSamples[i].dwStreamID;

        COutPin *poPin = ( COutPin * )GetOutPin( dwStreamID );
        DMFTCHECKNULL_GOTO( poPin, done, E_INVALIDARG );

        if ( SUCCEEDED( poPin->ProcessOutput( dwFlags, &pOutputSamples[i],
            pdwStatus ) ) )
        {
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
    MFTLOCKED();

    DMFTCHECKNULL_GOTO( ppAttributes, done, E_INVALIDARG );
    *ppAttributes = nullptr;

    CInPin *piPin = static_cast<CInPin*>(GetInPin( dwInputStreamID ));

    DMFTCHECKNULL_GOTO( piPin, done, E_INVALIDARG );

    hr  = piPin->getPinAttributes(ppAttributes);

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
    MFTLOCKED(); 

    DMFTCHECKNULL_GOTO(ppAttributes, done, E_INVALIDARG);
    *ppAttributes = nullptr;

    COutPin *poPin = (COutPin *)GetOutPin(dwOutputStreamID);

    DMFTCHECKNULL_GOTO( poPin, done, E_INVALIDARG );

    DMFTCHECKHR_GOTO( poPin->getPinAttributes(ppAttributes), done );
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
    CInPin *piPin = (CInPin*)GetInPin(dwStreamID);
    DMFTCHECKNULL_GOTO(piPin, done, MF_E_INVALIDSTREAMNUMBER);
    
    DMFTCHECKHR_GOTO(piPin->SetInputStreamState(pMediaType, value, dwFlags),done);
   
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
    CInPin *piPin = (CInPin*)GetInPin(dwStreamID);

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
    _Out_   DeviceStreamState   *value
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

    COutPin *poPin = (COutPin *)GetOutPin(dwStreamID);
    DMFTCHECKNULL_GOTO(poPin, done, MF_E_INVALIDSTREAMNUMBER);
    
    *value = poPin->GetState();
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
    CInPin *piPin = (CInPin*)GetInPin(dwStreamID);
    DMFTCHECKNULL_GOTO(piPin, done, MF_E_INVALIDSTREAMNUMBER);

    piPin->GetInputStreamPreferredState(value, ppMediaType);
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

    COutPin *poPin = (COutPin*)GetOutPin(dwStreamIndex);
    DMFTCHECKNULL_GOTO(poPin, done, E_INVALIDARG);
    
    DeviceStreamState oldState = poPin->SetState(DeviceStreamState_Disabled);
    
    hr = poPin->FlushQueues();
    
    if (FAILED(hr))
    {
        DMFTRACE(DMFT_GENERAL, TRACE_LEVEL_INFORMATION, "%!FUNC! failed %x = %!HRESULT!", hr, hr);
    }
    //
    //Restore state
    //
    poPin->SetState(oldState);
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
    for ( DWORD dwIndex = 0, dwSize = (DWORD)m_OutPins.size(); dwIndex < dwSize; dwIndex++ )
    {
        COutPin *poPin = (COutPin *)m_OutPins[dwIndex];
        oldState = poPin->SetState(DeviceStreamState_Disabled);
        poPin->FlushQueues();
        //
        //Restore state
        //
        poPin->SetState(oldState);
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
    if (IsEqualCLSID(pProperty->Set, KSPROPERTYSETID_ExtendedCameraControl)
        && (pProperty->Id == KSPROPERTY_CAMERACONTROL_EXTENDED_PHOTOTHUMBNAIL))
    {
        DMFTRACE(DMFT_GENERAL, TRACE_LEVEL_INFORMATION, "%!FUNC! Thumbnail sent %d",pProperty->Flags);
    }
    if (IsEqualCLSID(pProperty->Set, KSPROPERTYSETID_ExtendedCameraControl)
        &&(!filterHasIndependentPin()))
    {
        DMFTRACE(DMFT_GENERAL, TRACE_LEVEL_INFORMATION, "%!FUNC! Extended Control %d Passed ",pProperty->Id);
        switch (pProperty->Id)
        {
        case KSPROPERTY_CAMERACONTROL_EXTENDED_PHOTOMODE:
            hr = ExtendedPhotoModeHandler(pProperty,
                ulPropertyLength, pvPropertyData, ulDataLength, pulBytesReturned);
            goto done;
            break;
        case KSPROPERTY_CAMERACONTROL_EXTENDED_PHOTOMAXFRAMERATE:
            hr = ExtendedPhotoMaxFrameRate(pProperty,
                ulPropertyLength, pvPropertyData, ulDataLength, pulBytesReturned);
            goto done;
            break;
        case KSPROPERTY_CAMERACONTROL_EXTENDED_MAXVIDFPS_PHOTORES:
            hr = MaxVidFPS_PhotoResHandler(pProperty,
                ulPropertyLength, pvPropertyData, ulDataLength, pulBytesReturned);
            goto done;
            break;
        case KSPROPERTY_CAMERACONTROL_EXTENDED_PHOTOTRIGGERTIME:
            hr = QPCTimeHandler(pProperty,
                ulPropertyLength, pvPropertyData, ulDataLength, pulBytesReturned);
            goto done;
            break;
        case KSPROPERTY_CAMERACONTROL_EXTENDED_PHOTOFRAMERATE:
            hr = PhotoFrameRateHandler(pProperty,
                ulPropertyLength, pvPropertyData, ulDataLength, pulBytesReturned);
            goto done;
            break;
        case KSPROPERTY_CAMERACONTROL_EXTENDED_WARMSTART:
            hr = WarmStartHandler(pProperty,
                ulPropertyLength, pvPropertyData, ulDataLength, pulBytesReturned);
            goto done;
            break;
        }
    }
    else
    if ((IsEqualCLSID(pProperty->Set, PROPSETID_VIDCAP_VIDEOCONTROL)) &&
        (pProperty->Id == KSPROPERTY_VIDEOCONTROL_MODE))
    {
        //
        //A photo trigger was sent!!!
        //We need to set the event haveoutput
        //
        PKSPROPERTY_VIDEOCONTROL_MODE_S VideoControl = NULL;
        if (sizeof(KSPROPERTY_VIDEOCONTROL_MODE_S) == ulDataLength)
        {
            VideoControl = (PKSPROPERTY_VIDEOCONTROL_MODE_S)pvPropertyData;
            m_PhotoModeIsPhotoSequence = false;

            if (VideoControl->Mode == KS_VideoControlFlag_StartPhotoSequenceCapture)
            {
                //
                //Signalling start of photo sequence
                //
                DMFTRACE(DMFT_GENERAL, TRACE_LEVEL_INFORMATION, "%!FUNC! Starting PhotoSequence Trigger");
                m_PhotoModeIsPhotoSequence = true;
                setPhotoTriggerSent(true);
            }
            else
            if (VideoControl->Mode == KS_VideoControlFlag_StopPhotoSequenceCapture)
            {
                DMFTRACE(DMFT_GENERAL, TRACE_LEVEL_INFORMATION, "%!FUNC! Stopping PhotoSequence Trigger");
                m_PhotoModeIsPhotoSequence = false;
                setPhotoTriggerSent(false);
            }
            else
            {
                //
                //Normal trigger sent for single photo acquisition!
                //
                DMFTRACE(DMFT_GENERAL, TRACE_LEVEL_INFORMATION, "%!FUNC! Take Single Photo Trigger");
                setPhotoTriggerSent(true);
            }
            
        }
        
        if (!filterHasIndependentPin())
        {
            goto done;
        }
    }
    hr = m_ikscontrol->KsProperty(pProperty,
        ulPropertyLength,
        pvPropertyData,
        ulDataLength,
        pulBytesReturned);
 
    
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

    Implements IKSProperty::KsMethod function.
    --*/
{
    return m_ikscontrol->KsMethod(
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
    if ((pEvent && (pEvent->Set == KSEVENTSETID_ExtendedCameraControl))
        && (!filterHasIndependentPin()))
    {
        //
        //Store only for emualted controls..To do!!!
        //
        switch (pEvent->Id)
        {
        case KSPROPERTY_CAMERACONTROL_EXTENDED_PHOTOMODE:
        case KSPROPERTY_CAMERACONTROL_EXTENDED_PHOTOMAXFRAMERATE:
        case KSPROPERTY_CAMERACONTROL_EXTENDED_MAXVIDFPS_PHOTORES:
        case KSPROPERTY_CAMERACONTROL_EXTENDED_PHOTOTRIGGERTIME:
        case KSPROPERTY_CAMERACONTROL_EXTENDED_PHOTOFRAMERATE:
        case KSPROPERTY_CAMERACONTROL_EXTENDED_WARMSTART:
        {
             DMFTRACE(DMFT_GENERAL, TRACE_LEVEL_INFORMATION, "%!FUNC! Acquiring Event for Async Extended Control");
             KSEVENTDATA* ksEventData = (KSEVENTDATA*)pEventData;
             m_AsyncPropEvent = ksEventData->EventHandle.Event;
        }
            return S_OK;
        }
    }
    return m_ikscontrol->KsEvent(pEvent,
        ulEventLength,
        pEventData,
        ulDataLength,
        pBytesReturned);
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
    m_guidPhotoConfirmationSubtype = subtype;
    return S_OK;
}
STDMETHODIMP   CMultipinMft::GetPixelFormat(
    _Out_ GUID* subtype
    )
{
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

    DMFTCHECKNULL_GOTO(ppAttributes, done, E_INVALIDARG);
    
    *ppAttributes = nullptr;

    if (m_attributes != nullptr)
    {
        m_attributes.CopyTo(ppAttributes);
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

done:
    return hr;
}
#endif




//
//HELPER FUNCTIONS
//

STDMETHODIMP_(CBasePin*) CMultipinMft::GetInPin( 
    _In_ DWORD dwStreamId
    )
{
    CInPin *inPin = NULL;
    for (DWORD dwIndex = 0, dwSize = (DWORD)m_InPins.size(); dwIndex < dwSize; dwIndex++)
    {
         inPin = (CInPin *)m_InPins[dwIndex];
        if (dwStreamId == inPin->streamId())
        {
            break;
        }
        inPin = NULL;
    }
    return inPin;
}

STDMETHODIMP_(CBasePin*) CMultipinMft::GetOutPin(
    _In_ DWORD dwStreamId
    )
{
    COutPin *outPin = NULL;

    for ( DWORD dwIndex = 0, dwSize = (DWORD) m_OutPins.size(); dwIndex < dwSize; dwIndex++ )
    {
        outPin = ( COutPin * )m_OutPins[ dwIndex ];

        if ( dwStreamId == outPin->streamId() )
        {
            break;
        }

        outPin = NULL;
    }

    return outPin;
}

/*++
Description:
This is a critical function which changes the state on an output pin
This should be called under the control lock of the DT.
Here the 
--*/
STDMETHODIMP CMultipinMft::ChangeMediaTypeEx(
    _In_ ULONG pinId,
    _In_opt_ IMFMediaType *pMediaType,
    _In_ DeviceStreamState reqState
    )
{
    HRESULT                 hr                  = S_OK;
    DeviceStreamState       oldOutPinState;
    DeviceStreamState       newOutStreamState;
    ComPtr<IMFMediaType>   pFullType           = nullptr;

    COutPin *poPin = static_cast<COutPin*>( GetOutPin(pinId) );
    
    MMFTMMAPITERATOR inputPinPos;

    DMFTCHECKNULL_GOTO( poPin, done, E_INVALIDARG );
    //
    //Check if the media type requested is a supported type
    //
    if ( pMediaType )
    {
        if ( !poPin->IsMediaTypeSupported( pMediaType, &pFullType ) )
        {
            DMFTCHECKHR_GOTO(MF_E_INVALIDMEDIATYPE, done);
        }
    }
    //
    //First step disable the output pin. store the old state
    //
    oldOutPinState = poPin->SetState( DeviceStreamState_Disabled );
    
    (void)poPin->FlushQueues();

    newOutStreamState = pinStateTransition[oldOutPinState][reqState];  //New state needed
    
    //
    //Go through the output pins' maps that has the conencted input pin to the output pin
    //
    inputPinPos = m_outputPinMap.equal_range( pinId );
    
	for (std::multimap<int, int>::iterator piterator = inputPinPos.first; piterator != inputPinPos.second;piterator++)
    {
        //
        //Get the output pins connected to the input pin. The input pin map consists of the output pins connected 
        //
        ULONG connectedInputPin             = (*piterator).second;
        BOOL isAnyConnectedOutPinActive     = false;
        BOOL isAnyConnectedOutPinPaused     = false;
        BOOL isAnyConnectedOutPinRunning    = false;
        BOOL doWeWaitForSetInput            = false;
        DeviceStreamState oldInputStreamState;
        MF_TRANSFORM_XVP_OPERATION operation = DeviceMftTransformXVPIllegal;
        ComPtr<IMFMediaType> pInputMediaType = nullptr;
        CInPin *pconnectedInPin = (CInPin*)GetInPin( connectedInputPin );
        oldInputStreamState = pconnectedInPin->SetState( DeviceStreamState_Disabled );
        DeviceStreamState newRequestedInPinState = pinStateTransition[oldInputStreamState][newOutStreamState];
        
        if (pFullType)
        {
            pconnectedInPin->getMediaType( &pInputMediaType );
        }
        
        //
        // Check if we need an  XVP to be inserted between this input and output pins.
        //
        CompareMediaTypesForXVP( pInputMediaType.Get(), pFullType.Get(), &operation );
        DMFTCHECKHR_GOTO( GetConnectOutPinStatus( connectedInputPin,
            pinId,                          // pinId = stream which should be excluded from the search. This function searches if 
            &isAnyConnectedOutPinPaused,    // there are any other output pins connected other than the ouput pin (on which the change media type is requested)
            &isAnyConnectedOutPinRunning,   //, which is streaming, paused. This way if the output pin is requested to go to Pause, Stop and if any of the
            &isAnyConnectedOutPinActive ),done ); //other connected output pins are active, then input it not deactivated

        //
        //Check if we are asked to go to an active state
        //Check for the new media type. if 
        //
        if ( !IsPinStateInActive( newRequestedInPinState ) )
        {
            //
            //We are being told to go active
            //We will request a change in input stream state.
            //
            DMFTRACE( DMFT_GENERAL, TRACE_LEVEL_INFORMATION, "%!FUNC! %p Going active IN: %d OUT: %d ", this, connectedInputPin, pinId );
            doWeWaitForSetInput = true;
            if ( !pFullType ) 
            {
                DMFTCHECKHR_GOTO(MF_E_INVALIDMEDIATYPE, done);
            }
            if ( (! pInputMediaType ) || ( operation == DeviceMftTransformXVPDisruptiveIn ) )
            {
                pInputMediaType = pFullType;
            }
            if ( newRequestedInPinState == DeviceStreamState_Pause && isAnyConnectedOutPinRunning )
            {
                newRequestedInPinState = DeviceStreamState_Run;
            }
        }
        else
        {
            //
            //We have been requested to go inactive
            //
            DMFTRACE( DMFT_GENERAL, TRACE_LEVEL_INFORMATION, "%!FUNC! %p Going Inactive IN: %d OUT: %d ", this, connectedInputPin, pinId );

            if ( operation == DeviceMftTransformXVPDisruptiveIn )
            {
                //
                //Only where we recieve a disruptive media type change and a media type along with it, which is unlikely!!
                //
                pInputMediaType = pFullType;
                doWeWaitForSetInput = true;
            }
            if ( isAnyConnectedOutPinActive )
            {
                newRequestedInPinState = ( isAnyConnectedOutPinRunning ) ? DeviceStreamState_Run : DeviceStreamState_Pause;
            }
            else
            {
                //Switch over to the new media type.. 
                pInputMediaType = pFullType;
                if ( !IsPinStateInActive( oldInputStreamState ) )
                {
                    //It was originall active.. now going down
                    doWeWaitForSetInput = true;
                }
            }
        }

        //
        // This will happen if we need a change in Input media type, We will send an event
        // METransformInputStreamStateChanged to the Device transform manager. This will result
        // in the Device Transform manager calling us back in getprefferedinputstate where
        // we will give it back the state of the input and the media type to be set. 
        // All this happens when we are holding a lock here in Change output media type. Hence during the
        // input media type change we don't hold a lock. THE DTM takes care not to send you any more media
        // type change operations that can cause a deadlock.
        //

        if ( doWeWaitForSetInput )
        {
            pconnectedInPin->setPreferredMediaType( pInputMediaType.Get() );
            pconnectedInPin->setPreferredStreamState( newRequestedInPinState );
            SendEventToManager( METransformInputStreamStateChanged, GUID_NULL, pconnectedInPin->streamId() );
            //
            //The media type will be set on the input pin by the time we return from the wait
            //          
            DMFTCHECKHR_GOTO( pconnectedInPin->WaitForSetInputPinMediaChange(), done );
        }
        else
        {
            pconnectedInPin->SetState( oldInputStreamState );
            pconnectedInPin->setMediaType( pInputMediaType.Get() );
        }
        
        //Now the input type is all set..
        pInputMediaType = nullptr;
        (VOID)pconnectedInPin->getMediaType( &pInputMediaType );
        //
        //Now propogate the media type change to the output pins
        //
        MMFTMMAPITERATOR outputPinPos = m_inputPinMap.equal_range(pconnectedInPin->streamId());
        for ( std::multimap<int, int>::iterator poutPinsIterator = outputPinPos.first;
            poutPinsIterator != outputPinPos.second;
            poutPinsIterator++ )
        {
            ComPtr<IMFMediaType> pOutMediatype = nullptr;
            DeviceStreamState     outPinState;
            ULONG connectedoutPin = (*poutPinsIterator).second;
            COutPin* pIoPin = static_cast<COutPin*>( GetOutPin ( connectedoutPin ) );

            (VOID)pIoPin->getMediaType( &pOutMediatype );
            outPinState = pIoPin->GetState();

            if ( pIoPin->streamId() != pinId)
            {
                //
                //This is the pin other than the output pin where the original change media type was recieved.
                //

                if ( pInputMediaType != nullptr && pOutMediatype != nullptr )
                {
                    DMFTCHECKHR_GOTO( pIoPin->ChangeMediaTypeFromInpin(pconnectedInPin, pInputMediaType.Get(),
                        pOutMediatype.Get(),
                        outPinState ),done);
                }
            }
            else
            {
                //
                //Change the media type of the requested output pin
                //
                DMFTCHECKHR_GOTO( pIoPin->ChangeMediaTypeFromInpin(pconnectedInPin, pInputMediaType.Get(), pFullType.Get(), newOutStreamState), done);
                //Also signal to the manager that a stream state change has happened
                SendEventToManager( MEUnknown, MEDeviceStreamCreated, pIoPin->streamId());
                //
                //Set the First Sample Flag. this will get reset when the first sample comes in. We will signal the discontinuity 
                //when we get a Processoutput from the Device Transform Manager
                //
                pIoPin->SetFirstSample(TRUE);

            }
            pOutMediatype = nullptr;
        }

    }
    
done:
    DMFTRACE(DMFT_GENERAL, TRACE_LEVEL_INFORMATION, "%!FUNC! exiting %x = %!HRESULT!", hr, hr);
    return hr;
}

STDMETHODIMP CMultipinMft::SendEventToManager(
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


STDMETHODIMP CMultipinMft::GetConnectOutPinStatus(
    _In_ ULONG              ulPinId,
    _In_ ULONG              ulOutPinId,
    _Inout_ PBOOL          pAnyInPauseState,
    _Inout_ PBOOL          pAnyInRunState,
    _Inout_ PBOOL          pAnyActive
    )
    /*++
    Description:

    This function gets the statuses of the output pins other than the one specified
    The arguments are as follows
    ulPinId: The input pins which are connected to the ouput pin ulOutPinId
    ulOutPinId: The pin to be excluded from the check i.e. the input pin from which the request is usually sent.
    --*/
{
    HRESULT hr = S_OK;
    MMFTMMAPITERATOR outputPinPos;
    CInPin *pconnectedInPin = (CInPin*)GetInPin(ulPinId);
    DMFTCHECKNULL_GOTO( pconnectedInPin, done, E_INVALIDARG );

    outputPinPos = m_inputPinMap.equal_range( pconnectedInPin->streamId() );
    DMFTCHECKNULL_GOTO( pconnectedInPin, done, E_FAIL );

    *pAnyActive = *pAnyInPauseState = *pAnyInRunState =  false;
    
    for ( std::multimap<int, int>::iterator poutPosIterator = outputPinPos.first;
        poutPosIterator != outputPinPos.second;
        poutPosIterator++ )
    {
        ULONG    connectedoutPin = (*poutPosIterator).second;
        COutPin* pconnectedOutPin   = ( COutPin * )GetOutPin( connectedoutPin );

        if (pconnectedOutPin->streamId() != ulOutPinId)
        {
            //This is excluding the outpin which requested pin state change
            *pAnyActive         |= !IsPinStateInActive( pconnectedOutPin->GetState() );
            *pAnyInPauseState   |= ( pconnectedOutPin->GetState() == DeviceStreamState_Pause );
            *pAnyInRunState     |= ( pconnectedOutPin->GetState() == DeviceStreamState_Run );
        }

    }
    done:
    return S_OK;
}


/*++
Description:
This function connects the input and output pins.
Any media type filtering can  happen here
--*/
STDMETHODIMP CMultipinMft::BridgeInputPinOutputPin(
    _In_ CInPin* piPin,
    _In_ COutPin* poPin
    )
{
    HRESULT hr               = S_OK;
    ULONG   ulIndex          = 0;
    ComPtr<IMFMediaType> pMediaType = nullptr;

    DMFTCHECKNULL_GOTO( piPin, done, E_INVALIDARG );
    DMFTCHECKNULL_GOTO( poPin, done, E_INVALIDARG );
    //
    //Copy over the media types from input pin to output pin. Since there is no
    //decoder support, only the uncompressed media types are inserted
    //
    while ( SUCCEEDED( hr = piPin->GetMediaTypeAt( ulIndex++, &pMediaType )))
    {
        GUID subType = GUID_NULL;
        DMFTCHECKHR_GOTO( pMediaType->GetGUID(MF_MT_SUBTYPE,&subType), done );
        
        if ( IsKnownUncompressedVideoType( subType ) )
        {
            DMFTCHECKHR_GOTO( poPin->AddMediaType(NULL, pMediaType.Get() ), done );
        }

        pMediaType = nullptr;
    }
    //
    //Add the Input Pin to the output Pin
    //
    DMFTCHECKHR_GOTO( poPin->AddPin(piPin->streamId()), done );
    //
    //Add the output pin to the input pin. 
    //
    piPin->ConnectPin( poPin );

    //
    //Create the map. This will be useful when we have to decide the state transitions of the pins
    //
    m_inputPinMap.insert  ( std::pair< int,int >( piPin->streamId(), poPin->streamId())  );
    m_outputPinMap.insert ( std::pair< int, int >( poPin->streamId(), piPin->streamId()) );
done:
    //
    //Failed adding media types
    //
    return hr;
}


//
//The below routines are used to implement the extended controls needed to implement the photo sequence
//The photo sequence is enabled for cameras with no independent image pins
//The extended controls needed to enable photo sequence are discussed in detail in the photo sequence document
//

/*++
Extended Photo Mode handler is the extended property handler dealing with the photosequence and single mode capabilities of the camera
--*/
STDMETHODIMP CMultipinMft::ExtendedPhotoModeHandler(
_In_       PKSPROPERTY Property,
_In_       ULONG       ulPropertyLength,
_In_       LPVOID      pData,
_In_       ULONG       ulOutputBufferLength,
_Inout_    PULONG      pulBytesReturned
)

{
    HRESULT              hr = S_OK;
    UNREFERENCED_PARAMETER( ulPropertyLength );

    if ( Property->Flags & KSPROPERTY_TYPE_SET )
    {
        if ( ulOutputBufferLength == 0 )
        {
            *pulBytesReturned = sizeof( KSCAMERA_EXTENDEDPROP_HEADER )+sizeof( KSCAMERA_EXTENDEDPROP_PHOTOMODE );
            DMFTCHECKHR_GOTO( HRESULT_FROM_WIN32( ERROR_MORE_DATA ), done );
        }
        else if ( ulOutputBufferLength < sizeof( KSCAMERA_EXTENDEDPROP_HEADER )+sizeof( KSCAMERA_EXTENDEDPROP_PHOTOMODE ) )
        {
            DMFTCHECKHR_GOTO( HRESULT_FROM_WIN32( ERROR_MORE_DATA ), done );
        }
        else if ( pData && ulOutputBufferLength >= sizeof( KSCAMERA_EXTENDEDPROP_HEADER )+sizeof( KSCAMERA_EXTENDEDPROP_PHOTOMODE ) )
        {
            PBYTE pPayload = ( PBYTE )pData;
            PKSCAMERA_EXTENDEDPROP_HEADER pExtendedHeader = ( PKSCAMERA_EXTENDEDPROP_HEADER )( pPayload );
            //
            //Use the below structure to make changes to the Property and thus affect the configuration
            //PKSCAMERA_EXTENDEDPROP_PHOTOMODE pExtendedValue = (PKSCAMERA_EXTENDEDPROP_PHOTOMODE)(pPayload + sizeof(KSCAMERA_EXTENDEDPROP_HEADER));
            //
            m_FilterInPhotoSequence = pExtendedHeader->Flags & KSCAMERA_EXTENDEDPROP_PHOTOMODE_SEQUENCE;
            SetEvent( m_AsyncPropEvent );
        }
        else
        {
            DMFTCHECKHR_GOTO(E_INVALIDARG, done);
        }
    }
    else if ( Property->Flags & KSPROPERTY_TYPE_GET )
    {
        if ( ulOutputBufferLength == 0 )
        {
            *pulBytesReturned = sizeof( KSCAMERA_EXTENDEDPROP_HEADER )+sizeof( KSCAMERA_EXTENDEDPROP_PHOTOMODE );
            hr = HRESULT_FROM_WIN32(ERROR_MORE_DATA);
        }
        else if ( pData && ulOutputBufferLength >= sizeof( KSCAMERA_EXTENDEDPROP_HEADER )+sizeof( KSCAMERA_EXTENDEDPROP_PHOTOMODE ) )
        {
            PBYTE pPayload = (PBYTE)pData;
            PKSCAMERA_EXTENDEDPROP_HEADER pExtendedHeader   = ( PKSCAMERA_EXTENDEDPROP_HEADER )( pPayload );
            PKSCAMERA_EXTENDEDPROP_PHOTOMODE pExtendedValue = ( PKSCAMERA_EXTENDEDPROP_PHOTOMODE )( pPayload + sizeof( KSCAMERA_EXTENDEDPROP_HEADER ) );

            pExtendedHeader->Capability     = ( KSCAMERA_EXTENDEDPROP_CAPS_ASYNCCONTROL | KSCAMERA_EXTENDEDPROP_PHOTOMODE_SEQUENCE );
            pExtendedHeader->Flags          = ( m_FilterInPhotoSequence ) ? KSCAMERA_EXTENDEDPROP_PHOTOMODE_SEQUENCE : KSCAMERA_EXTENDEDPROP_PHOTOMODE_NORMAL;
            pExtendedHeader->Result         = 0;
            pExtendedHeader->Size           = sizeof( KSCAMERA_EXTENDEDPROP_HEADER )+sizeof( KSCAMERA_EXTENDEDPROP_PHOTOMODE );
            pExtendedHeader->Version        = 1;

            pExtendedValue->MaxHistoryFrames        = 10;
            pExtendedValue->RequestedHistoryFrames  = 0 ;
            pExtendedValue->SubMode                 = 0 ;
            *pulBytesReturned = sizeof( KSCAMERA_EXTENDEDPROP_HEADER )+sizeof( KSCAMERA_EXTENDEDPROP_PHOTOMODE );
        }
        else
        {
            DMFTCHECKHR_GOTO( E_INVALIDARG, done );
        }
    }

done:
    DMFTRACE( DMFT_GENERAL, TRACE_LEVEL_INFORMATION, "%!FUNC! exiting %x = %!HRESULT!", hr, hr);

    return hr;
}

/*++
The Extended Max Frame rate is self explanatory
--*/
STDMETHODIMP CMultipinMft::ExtendedPhotoMaxFrameRate(
    _In_       PKSPROPERTY Property,
    _In_       ULONG       ulPropertyLength,
    _In_       LPVOID      pData,
    _In_       ULONG       ulOutputBufferLength,
    _Inout_    PULONG      pulBytesReturned
    )

{
    HRESULT hr = S_OK;
    UNREFERENCED_PARAMETER( ulPropertyLength );

    if ( Property->Flags & KSPROPERTY_TYPE_SET )
    {
        if ( ulOutputBufferLength == 0 )
        {
            *pulBytesReturned = sizeof( KSCAMERA_EXTENDEDPROP_HEADER )+sizeof( KSCAMERA_EXTENDEDPROP_VALUE );
            DMFTCHECKHR_GOTO(HRESULT_FROM_WIN32(ERROR_MORE_DATA), done);
        }
        else if (ulOutputBufferLength < sizeof(KSCAMERA_EXTENDEDPROP_HEADER)+sizeof(KSCAMERA_EXTENDEDPROP_VALUE))
        {
            DMFTCHECKHR_GOTO(HRESULT_FROM_WIN32(ERROR_MORE_DATA), done);
        }
        else if (pData && ulOutputBufferLength >= sizeof(KSCAMERA_EXTENDEDPROP_HEADER)+sizeof(KSCAMERA_EXTENDEDPROP_VALUE))
        {
            //
            //This is for setting the Max frame rate..
            //
            //PBYTE pPayload = (PBYTE)pData;
            //PKSCAMERA_EXTENDEDPROP_HEADER pExtendedHeader = (PKSCAMERA_EXTENDEDPROP_HEADER)(pPayload);
            //PKSCAMERA_EXTENDEDPROP_VALUE pExtendedValue = (PKSCAMERA_EXTENDEDPROP_VALUE)(pPayload + sizeof(KSCAMERA_EXTENDEDPROP_HEADER));
            //
            SetEvent(m_AsyncPropEvent);
        }
        else
        {
            DMFTCHECKHR_GOTO(E_INVALIDARG, done);
        }
    }
    else if ( Property->Flags & KSPROPERTY_TYPE_GET )
    {
        if ( ulOutputBufferLength == 0 )
        {
            *pulBytesReturned = sizeof( KSCAMERA_EXTENDEDPROP_HEADER )+sizeof( KSCAMERA_EXTENDEDPROP_VALUE );
            hr = HRESULT_FROM_WIN32( ERROR_MORE_DATA );
        }
        else if ( pData && ulOutputBufferLength >= sizeof( KSCAMERA_EXTENDEDPROP_HEADER )+sizeof( KSCAMERA_EXTENDEDPROP_VALUE ))
        {
            PBYTE pPayload = ( PBYTE )pData;
            PKSCAMERA_EXTENDEDPROP_HEADER pExtendedHeader   = ( PKSCAMERA_EXTENDEDPROP_HEADER )( pPayload );
            PKSCAMERA_EXTENDEDPROP_VALUE pExtendedValue     = ( PKSCAMERA_EXTENDEDPROP_VALUE )( pPayload + sizeof( KSCAMERA_EXTENDEDPROP_HEADER ) );
            pExtendedHeader->Capability = KSCAMERA_EXTENDEDPROP_CAPS_ASYNCCONTROL;
            pExtendedHeader->Flags      = 0;
            pExtendedHeader->Result     = 0;
            pExtendedHeader->Size       = sizeof(KSCAMERA_EXTENDEDPROP_HEADER)+sizeof(KSCAMERA_EXTENDEDPROP_VALUE);
            pExtendedHeader->Version    = 1;
            pExtendedValue->Value.ratio.HighPart    = 30;
            pExtendedValue->Value.ratio.LowPart     = 1;
            *pulBytesReturned = sizeof(KSCAMERA_EXTENDEDPROP_HEADER)+sizeof(KSCAMERA_EXTENDEDPROP_VALUE);
        }
        else
        {
            DMFTCHECKHR_GOTO(E_INVALIDARG, done);
        }
    }

done:
    DMFTRACE(DMFT_GENERAL, TRACE_LEVEL_INFORMATION, "%!FUNC! exiting %x = %!HRESULT!", hr, hr);

    return hr;
}


STDMETHODIMP CMultipinMft::MaxVidFPS_PhotoResHandler(
    _In_       PKSPROPERTY Property,
    _In_       ULONG       ulPropertyLength,
    _In_       LPVOID      pData,
    _In_       ULONG       ulOutputBufferLength,
    _Inout_    PULONG      pulBytesReturned
    )
{
    HRESULT hr = S_OK;
    UNREFERENCED_PARAMETER( ulPropertyLength );
    if ( Property->Flags & KSPROPERTY_TYPE_SET )
    {
        if ( ulOutputBufferLength == 0 )
        {
            *pulBytesReturned = sizeof( KSCAMERA_EXTENDEDPROP_HEADER )+ sizeof( KSCAMERA_MAXVIDEOFPS_FORPHOTORES );
            DMFTCHECKHR_GOTO(HRESULT_FROM_WIN32(ERROR_MORE_DATA), done);
        }
        else if ( ulOutputBufferLength < sizeof( KSCAMERA_EXTENDEDPROP_HEADER )+ sizeof( KSCAMERA_MAXVIDEOFPS_FORPHOTORES ) )
        {
            DMFTCHECKHR_GOTO( HRESULT_FROM_WIN32( ERROR_MORE_DATA ), done );
        }
        else if (pData && ulOutputBufferLength >= sizeof( KSCAMERA_EXTENDEDPROP_HEADER )+sizeof( KSCAMERA_MAXVIDEOFPS_FORPHOTORES ))
        {
            PBYTE pPayload = (PBYTE)pData;
            PKSCAMERA_EXTENDEDPROP_HEADER pExtendedHeader = ( PKSCAMERA_EXTENDEDPROP_HEADER )( pPayload );
            //
            //Use the extended value to make changes to the property.. refer documentation
            //PKSCAMERA_MAXVIDEOFPS_FORPHOTORES pExtendedValue = (PKSCAMERA_MAXVIDEOFPS_FORPHOTORES)(pPayload + sizeof(KSCAMERA_EXTENDEDPROP_HEADER));
            //
            pExtendedHeader->Capability = 0;
            pExtendedHeader->Flags = 0;
            pExtendedHeader->Result = 0;
            pExtendedHeader->Size = sizeof( KSCAMERA_EXTENDEDPROP_HEADER )+sizeof( KSCAMERA_EXTENDEDPROP_VALUE );
            pExtendedHeader->Version = 1;

            *pulBytesReturned = sizeof( KSCAMERA_EXTENDEDPROP_HEADER )+sizeof( KSCAMERA_MAXVIDEOFPS_FORPHOTORES );
        }

        else
        {
            hr = E_INVALIDARG;
        }
    }
    else if (Property->Flags & KSPROPERTY_TYPE_GET)
    {
        if (ulOutputBufferLength == 0)
        {
            *pulBytesReturned = sizeof(KSCAMERA_EXTENDEDPROP_HEADER)+sizeof(KSCAMERA_MAXVIDEOFPS_FORPHOTORES);
            DMFTCHECKHR_GOTO(HRESULT_FROM_WIN32(ERROR_MORE_DATA), done);
        }
        else if (ulOutputBufferLength < sizeof(KSCAMERA_EXTENDEDPROP_HEADER)+sizeof(KSCAMERA_MAXVIDEOFPS_FORPHOTORES))
        {
            DMFTCHECKHR_GOTO(HRESULT_FROM_WIN32(ERROR_MORE_DATA), done);
        }
        else if (pData && ulOutputBufferLength >= sizeof(KSCAMERA_EXTENDEDPROP_HEADER)+sizeof(KSCAMERA_MAXVIDEOFPS_FORPHOTORES))
        {
            PBYTE pPayload = (PBYTE)pData;
            PKSCAMERA_EXTENDEDPROP_HEADER pExtendedHeader = ( PKSCAMERA_EXTENDEDPROP_HEADER )(pPayload );
            PKSCAMERA_MAXVIDEOFPS_FORPHOTORES pExtendedValue = (PKSCAMERA_MAXVIDEOFPS_FORPHOTORES)(pPayload + sizeof(KSCAMERA_EXTENDEDPROP_HEADER));
            pExtendedHeader->Capability = 0;
            pExtendedHeader->Flags = 0;
            pExtendedHeader->Result = 0;
            pExtendedHeader->Size = sizeof(KSCAMERA_EXTENDEDPROP_HEADER) + sizeof(PKSCAMERA_MAXVIDEOFPS_FORPHOTORES);
            pExtendedHeader->Version = 1;

            pExtendedValue->PreviewFPSNum = 30;
            pExtendedValue->PreviewFPSDenom = 1;
            pExtendedValue->CaptureFPSNum = 30;
            pExtendedValue->CaptureFPSDenom = 1;
            pExtendedValue->PhotoResHeight  =   240;
            pExtendedValue->PhotoResWidth   =   320;

            *pulBytesReturned = sizeof(KSCAMERA_EXTENDEDPROP_HEADER) + sizeof(KSCAMERA_MAXVIDEOFPS_FORPHOTORES);
        }
        else
        {
            hr = E_INVALIDARG;
        }
    }
done:
    DMFTRACE(DMFT_GENERAL, TRACE_LEVEL_INFORMATION, "%!FUNC! exiting %x = %!HRESULT!", hr, hr);
    return hr;
}


STDMETHODIMP CMultipinMft::QPCTimeHandler(
    _In_       PKSPROPERTY Property,
    _In_       ULONG       ulPropertyLength,
    _In_       LPVOID      pData,
    _In_       ULONG       ulOutputBufferLength,
    _Inout_    PULONG      pulBytesReturned
    )
{
    HRESULT hr = S_OK;
    UNREFERENCED_PARAMETER( ulPropertyLength );

    if ( Property->Flags & KSPROPERTY_TYPE_SET )
    {
        if ( ulOutputBufferLength == 0 )
        {
            *pulBytesReturned = sizeof( KSCAMERA_EXTENDEDPROP_HEADER )+sizeof( KSCAMERA_EXTENDEDPROP_VALUE );
            DMFTCHECKHR_GOTO( HRESULT_FROM_WIN32( ERROR_MORE_DATA ), done );
        }
        else if ( ulOutputBufferLength < sizeof( KSCAMERA_EXTENDEDPROP_HEADER )+sizeof( KSCAMERA_EXTENDEDPROP_VALUE ) )
        {
            DMFTCHECKHR_GOTO(HRESULT_FROM_WIN32(ERROR_MORE_DATA), done);
        }
        else if ( pData && ulOutputBufferLength >= sizeof( KSCAMERA_EXTENDEDPROP_HEADER )+sizeof( KSCAMERA_EXTENDEDPROP_VALUE ) )
        {
            //PBYTE pPayload = (PBYTE)pData;
            //
            //If the payload is to be used.. use the below structures
            //
            //PKSCAMERA_EXTENDEDPROP_HEADER pExtendedHeader = (PKSCAMERA_EXTENDEDPROP_HEADER)(pPayload);
            //Use the extended value to make changes to the property.. refer documentation
            //PKSCAMERA_EXTENDEDPROP_VALUE pExtendedValue = (PKSCAMERA_EXTENDEDPROP_VALUE)(pPayload +sizeof(KSCAMERA_EXTENDEDPROP_HEADER));
            //
            *pulBytesReturned = sizeof( PKSCAMERA_EXTENDEDPROP_HEADER )+sizeof( KSCAMERA_EXTENDEDPROP_VALUE );
        }
        else
        {
            hr = E_INVALIDARG;
        }
    }
    else if ( Property->Flags & KSPROPERTY_TYPE_GET )
    {
        if ( ulOutputBufferLength == 0 )
        {
            *pulBytesReturned = sizeof( KSCAMERA_EXTENDEDPROP_HEADER )+sizeof( KSCAMERA_EXTENDEDPROP_VALUE );
            DMFTCHECKHR_GOTO(HRESULT_FROM_WIN32(ERROR_MORE_DATA), done);
        }
        else if ( ulOutputBufferLength < sizeof( KSCAMERA_EXTENDEDPROP_HEADER ) + sizeof( KSCAMERA_EXTENDEDPROP_VALUE ) )
        {
            DMFTCHECKHR_GOTO( HRESULT_FROM_WIN32( ERROR_MORE_DATA ), done );
        }
        else if ( pData && ulOutputBufferLength >= sizeof( KSCAMERA_EXTENDEDPROP_HEADER ) + sizeof( KSCAMERA_EXTENDEDPROP_VALUE ) )
        {
            PBYTE pPayload = (PBYTE)pData;
            PKSCAMERA_EXTENDEDPROP_HEADER pExtendedHeader = ( PKSCAMERA_EXTENDEDPROP_HEADER )( pPayload );
            PKSCAMERA_EXTENDEDPROP_VALUE pExtendedValue = ( PKSCAMERA_EXTENDEDPROP_VALUE )( pPayload + sizeof( KSCAMERA_EXTENDEDPROP_HEADER ) );
            pExtendedHeader->Capability = 0;
            pExtendedHeader->Flags = KSPROPERTY_CAMERA_PHOTOTRIGGERTIME_SET;
            pExtendedHeader->Result = 0;
            pExtendedHeader->Size = sizeof( KSCAMERA_EXTENDEDPROP_HEADER ) + sizeof( KSCAMERA_EXTENDEDPROP_VALUE );
            pExtendedHeader->Version = 1;
            pExtendedValue->Value.ull = 0;
            *pulBytesReturned = sizeof( KSCAMERA_EXTENDEDPROP_HEADER )+ sizeof( KSCAMERA_EXTENDEDPROP_VALUE );
        }
        else
        {
            hr = E_INVALIDARG;
        }
    }
done:
    DMFTRACE( DMFT_GENERAL, TRACE_LEVEL_INFORMATION, "%!FUNC! exiting %x = %!HRESULT!", hr, hr );
    return hr;
}

STDMETHODIMP CMultipinMft::PhotoFrameRateHandler(
    _In_       PKSPROPERTY Property,
    _In_       ULONG       ulPropertyLength,
    _In_       LPVOID      pData,
    _In_       ULONG       ulOutputBufferLength,
    _Inout_   PULONG       pulBytesReturned
    )
{
    HRESULT hr = S_OK;
   
    UNREFERENCED_PARAMETER( ulPropertyLength );
    if ( Property->Flags & KSPROPERTY_TYPE_SET )
    {
        //
        //This is a read only property!!!
        //
        hr = E_INVALIDARG;
    }
    else if ( Property->Flags & KSPROPERTY_TYPE_GET )
    {
        if ( ulOutputBufferLength < sizeof( KSCAMERA_EXTENDEDPROP_HEADER )+sizeof( KSCAMERA_EXTENDEDPROP_VALUE ))
        {
            *pulBytesReturned = sizeof( KSCAMERA_EXTENDEDPROP_HEADER )+sizeof( KSCAMERA_EXTENDEDPROP_VALUE );
            DMFTCHECKHR_GOTO( HRESULT_FROM_WIN32( ERROR_MORE_DATA ), done );
        }
        PBYTE pPayload = (PBYTE)pData;
        PKSCAMERA_EXTENDEDPROP_HEADER pExtendedHeader = ( PKSCAMERA_EXTENDEDPROP_HEADER )( pPayload );
        PKSCAMERA_EXTENDEDPROP_VALUE pExtendedValue = ( PKSCAMERA_EXTENDEDPROP_VALUE )( pPayload + sizeof( KSCAMERA_EXTENDEDPROP_HEADER ) );
        pExtendedHeader->Capability = 0;
        pExtendedHeader->Flags = 0;
        pExtendedHeader->Result = 0;
        pExtendedHeader->Size = sizeof( KSCAMERA_EXTENDEDPROP_HEADER )+sizeof( KSCAMERA_EXTENDEDPROP_VALUE );
        pExtendedHeader->Version = 1;
        pExtendedValue->Value.ratio.HighPart = 30;
        pExtendedValue->Value.ratio.LowPart = 1;
        *pulBytesReturned = sizeof( KSCAMERA_EXTENDEDPROP_HEADER )+sizeof( KSCAMERA_EXTENDEDPROP_VALUE );
        
    }
done:
    DMFTRACE( DMFT_GENERAL, TRACE_LEVEL_INFORMATION, "%!FUNC! exiting %x = %!HRESULT!", hr, hr );
    return hr;
}


STDMETHODIMP CMultipinMft::WarmStartHandler(
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
            if ( pExtendedHeader->Flags & KSCAMERA_EXTENDEDPROP_WARMSTART_MODE_ENABLED )
            {
                m_filterInWarmStart = true;
            }
            else
            {
                m_filterInWarmStart = false;
            }

            *pulBytesReturned = sizeof( PKSCAMERA_EXTENDEDPROP_HEADER )+sizeof( KSCAMERA_EXTENDEDPROP_VALUE );

            SetEvent( m_AsyncPropEvent );
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

            if (m_filterInWarmStart)
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

    DMFTCHECKHR_GOTO(pSample->SetUnknown(MFSourceReader_SampleAttribute_MediaType, spMediaType.Get()), done);

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

STDMETHODIMP CMultipinMft::SetStreamingStateCustomPins(
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
            CInPin* pInPin = static_cast<CInPin*>(m_InPins[ulIndex]);

            if ( SUCCEEDED( CheckCustomPin(pInPin, &isCustom) )
                && ( isCustom ) )
            {
                pInPin->SetState(State);
            }
        }
    }
    DMFTRACE(DMFT_GENERAL, TRACE_LEVEL_INFORMATION, "%!FUNC! exiting %x = %!HRESULT!", hr, hr);
    return hr;
}
