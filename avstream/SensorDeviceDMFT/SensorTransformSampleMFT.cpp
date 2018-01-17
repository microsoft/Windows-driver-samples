//*@@@+++@@@@******************************************************************
//
// Copyright (C) Microsoft Corporation. All rights reserved.
//
//*@@@---@@@@******************************************************************
//


#include "stdafx.h"
#include "common.h"

//
// How to Enable Sensor Transforms. 
// We need to insert two Strings in the Device Interface key(s) of the devices to be a part of the sensor group under these categories
//       KSCATEGORY_SENSOR_CAMERA and KSCATEGORY_VIDEO_CAMERA
// 1)    "FSSensorGroupID" is the Registry String that should exist in the device interface entry of the 'n' devices that need to have a sensor transform
// 2)    "FSSensorTransformFactoryCLSID" is the Registry string which should have the GUID with which we register this dll. The pipeline will load this dll
//        using this GUID. It is defined in this sample as CLSID_SensorTransformDeviceMFT in common.h
//  


HRESULT CComDMFTCreateInstance(_In_ REFIID riid, _Outptr_ VOID** ppVoid)
{
    return CSensorTransformMFT::CreateInstance(riid, ppVoid);
}

HRESULT 
CSensorTransformMFT::CreateInstance( _In_ REFIID riid, _COM_Outptr_ PVOID* ppCSensorTransformMFT)
{
    HRESULT     hr = S_OK;
    ComPtr<CSensorTransformMFT>   spCSensorTransformMFT;
    SDMFTCHECKNULL_GOTO (ppCSensorTransformMFT, done, E_POINTER);

    *ppCSensorTransformMFT = nullptr;
    SDMFTCHECKHR_GOTO(ExceptionBoundary([&]() { 
        spCSensorTransformMFT = new CSensorTransformMFT();
    }), done); // This process may throw an Exception if an out of memory is experienced or we fail to load the sensor transform library

    SDMFTCHECKNULL_GOTO(spCSensorTransformMFT.Get(), done, E_OUTOFMEMORY);
    SDMFTCHECKHR_GOTO (spCSensorTransformMFT->InitializeDMFT(), done);
    SDMFTCHECKHR_GOTO(spCSensorTransformMFT->QueryInterface(riid, ppCSensorTransformMFT), done);
done:
    return hr;
}

// Static information for the factory. 
static LPCWSTR      s_FactoryVersion = L"1.0.0.1";
static SYSTEMTIME   s_FactoryDateTime   = { 2017, 3, 24, 12, 0, 0, 0 };
static GUID         s_guidOurSTId = { 0xF85E6949, 0xB3FA, 0x43EB, { 0xA3, 0xE4, 0x7D, 0x94, 0xA5, 0xCD, 0x88, 0x7D } }; 
static LPCWSTR      s_SensorTransformName = L"SensorDeviceMFT";

/////////////////////////////////////////////////////////////////////////////////////////////// 
///  
CSensorTransformMFT::CSensorTransformMFT(
    )
:   m_lRef(0)
,   m_fShutdown(false)
,   m_dwWorkQueueId(0)
,   m_lWorkItemBasePriority(0)
{
    
}

CSensorTransformMFT::~CSensorTransformMFT(
    )
{
    HRESULT hr = S_OK;
    SDMFTTRACE(SENSORDEVICEMFT_LEVEL_INFO,L" Exiting...");
    // This takes the lock again
    if (FAILED(hr = Shutdown()))
    {
        SDMFTTRACE(SENSORDEVICEMFT_LEVEL_INFO, L" Failed Shutdown... 0x%x",hr);
    }
    m_aryInputPins.clear();
    m_aryOutputPins.clear();
}

/// IUnknown
IFACEMETHODIMP_(ULONG)
CSensorTransformMFT::AddRef(
    )
{
    return InterlockedIncrement(&m_lRef);
}

IFACEMETHODIMP_(ULONG)
CSensorTransformMFT::Release(
    )
{
    LONG cRef = InterlockedDecrement(&m_lRef);

    if (0 == cRef)
    {
        delete this;
    }

    return(cRef);
}

IFACEMETHODIMP
CSensorTransformMFT::QueryInterface(
    _In_ REFIID iid,
    _Out_ LPVOID *ppv
    )
{
    HRESULT hr = S_OK;

    SDMFTCHECKNULL_GOTO (ppv, done, E_POINTER);
    *ppv = nullptr;

    if ((iid == __uuidof(IMFDeviceTransform)) || (iid == __uuidof(IUnknown)))
    {
        *ppv = static_cast< IMFDeviceTransform* >(this);
        AddRef();
    }
    else if ( iid == __uuidof( IMFMediaEventGenerator ) )
    {
        *ppv = static_cast< IMFMediaEventGenerator* >(this);
        AddRef();
    }
    else if ( iid == __uuidof( IMFShutdown ) )
    {
        *ppv = static_cast< IMFShutdown* >( this );
        AddRef();
    }
    else if ( iid == __uuidof( IKsControl ) )
    {
        *ppv = static_cast< IKsControl* >( this );
        AddRef();
    }
    else if ( iid == __uuidof( IMFRealTimeClientEx ) )
    {
        *ppv = static_cast< IMFRealTimeClientEx* >( this );
        AddRef();
    }
    else if ( iid == __uuidof( IMFAttributes ) )
    {
        *ppv = static_cast< IMFAttributes* >( this );
        AddRef();
    }
    else if ( iid == __uuidof( IMFSensorTransformFactory ) )
    {
        *ppv = static_cast< IMFSensorTransformFactory* >( this );
        AddRef();
    }
    else
    {
        hr = E_NOINTERFACE;
    }

done:
    return hr;
}


//////////////////////////////////////////////////////////////////////////
// IMFDeviceTransform functions
//////////////////////////////////////////////////////////////////////////
IFACEMETHODIMP
CSensorTransformMFT::InitializeTransform(
    _In_ IMFAttributes *pAttributes 
    )
{
    HRESULT                 hr = S_OK;
    ComPtr<IMFTransform>    spSourceTransform;
    DWORD                   dwInputStreamCount = 0;
    DWORD*                  pdwInputStreamIDs = nullptr;
    DWORD                   dwOutputStreamCount = 0;
    DWORD*                  pdwOutputStreamIDs = nullptr;
    
    CAutoLock               lock( &m_lock );
    SDMFTCHECKHR_GOTO (CheckShutdown(), done);
    SDMFTCHECKNULL_GOTO (pAttributes, done, E_INVALIDARG);

    SDMFTTRACE(SENSORDEVICEMFT_LEVEL_INFO,L"Entering.. ");
    SDMFTCHECKHR_GOTO (pAttributes->GetUnknown(MF_DEVICEMFT_CONNECTED_FILTER_KSCONTROL, IID_PPV_ARGS(&spSourceTransform)), done);
    SDMFTCHECKHR_GOTO (spSourceTransform->GetStreamCount(&dwInputStreamCount, &dwOutputStreamCount), done);
    
    if (dwOutputStreamCount == 0)
    {
        SDMFTCHECKHR_GOTO (MF_E_INVALIDSTREAMNUMBER, done);
    }
    
    SDMFTTRACE(SENSORDEVICEMFT_LEVEL_INFO,L"0x%p Loading Sensor Transform with %d Input Pins and %d Output Pins", this, dwInputStreamCount, dwOutputStreamCount);
    pdwOutputStreamIDs = new DWORD[dwOutputStreamCount];
    SDMFTCHECKNULL_GOTO (pdwOutputStreamIDs, done, E_OUTOFMEMORY);
    if (dwInputStreamCount > 0)
    {
        pdwInputStreamIDs = new DWORD[dwInputStreamCount];
        SDMFTCHECKNULL_GOTO (pdwInputStreamIDs, done, E_OUTOFMEMORY);
    }
    SDMFTCHECKHR_GOTO (spSourceTransform->GetStreamIDs(dwInputStreamCount, pdwInputStreamIDs, dwOutputStreamCount, pdwOutputStreamIDs), done);

    for (DWORD dwIndex = 0; dwIndex < dwOutputStreamCount; dwIndex++)
    {
        ComPtr<CSensorTransformMFTPin>    spInputPin;
        SDMFTTRACE(SENSORDEVICEMFT_LEVEL_INFO,L"0x%p Loading Sensor Transform creating Input Pin %d ", this, pdwOutputStreamIDs[dwIndex]);
        SDMFTCHECKHR_GOTO (CSensorTransformMFTPin::CreateInstance(pdwOutputStreamIDs[dwIndex],this, spSourceTransform.Get(), spInputPin.GetAddressOf()), done);
        //
        // Catch the Exception which can be thrown by the STL data structure. Fail gracefully in case of Error and not crash the pipeline
        //
        SDMFTCHECKHR_GOTO(ExceptionBoundary([this, spInputPin]() {
            m_aryInputPins.push_back(spInputPin);}
        ),done);
        
        if (dwIndex == 0)
        {
            // Here we are creating just one output pin to the many input pins
            ComPtr<CSensorTransformMFTPin>        spOutPin;
            ComPtr<IMFAttributes>                 spAttributes;

            SDMFTCHECKHR_GOTO (CSensorTransformMFTPin::CreateInstance(pdwOutputStreamIDs[dwIndex], this, spSourceTransform.Get(), spOutPin.GetAddressOf()), done);
            SDMFTCHECKHR_GOTO (spOutPin->SetString(MF_DEVSOURCE_ATTRIBUTE_FRIENDLY_NAME, s_SensorTransformName), done);
            hr = ExceptionBoundary([this, spOutPin]() {m_aryOutputPins.push_back(spOutPin);});
            SDMFTCHECKHR_GOTO (hr, done);
            SDMFTCHECKHR_GOTO (spSourceTransform->GetOutputStreamAttributes(pdwOutputStreamIDs[dwIndex], spAttributes.GetAddressOf()), done);
            SDMFTCHECKHR_GOTO (spAttributes->GetUnknown(MF_DEVICESTREAM_FILTER_KSCONTROL, IID_PPV_ARGS(&m_spKsControl)), done);
        }
    }

done:
    SAFE_ARRAYDELETE(pdwInputStreamIDs);
    SAFE_ARRAYDELETE(pdwOutputStreamIDs);
    return hr;
}

IFACEMETHODIMP
CSensorTransformMFT::GetInputAvailableType(
    _In_ DWORD dwInputStreamID, 
    _In_ DWORD dwTypeIndex, 
    _Out_ IMFMediaType** ppType
    )
{
    CAutoLock lock( &m_lock );
    HRESULT     hr = S_OK;

    SDMFTCHECKHR_GOTO (CheckShutdown(), done);
    if (dwInputStreamID >= m_aryInputPins.size())
    {
        SDMFTCHECKHR_GOTO (MF_E_INVALIDSTREAMNUMBER, done);
    }

    SDMFTCHECKHR_GOTO (m_aryInputPins[dwInputStreamID]->GetAvailableMediaType(dwTypeIndex, ppType), done);

done:
    return hr;
}

IFACEMETHODIMP
CSensorTransformMFT::GetInputCurrentType(
    _In_ DWORD dwInputStreamID, 
    _COM_Outptr_result_maybenull_ IMFMediaType**  ppMediaType
    )
{
    CAutoLock lock( &m_lock );
    HRESULT     hr = S_OK;

    SDMFTCHECKHR_GOTO (CheckShutdown(), done);
    if (dwInputStreamID >= m_aryInputPins.size())
    {
        SDMFTCHECKHR_GOTO (MF_E_INVALIDSTREAMNUMBER, done);
    }

    SDMFTCHECKHR_GOTO (m_aryInputPins[dwInputStreamID]->GetCurrentMediaType(ppMediaType), done);

done:
    return hr;
}

IFACEMETHODIMP
CSensorTransformMFT::GetInputStreamAttributes(
    _In_ DWORD dwInputStreamID, 
    _COM_Outptr_result_maybenull_ IMFAttributes** ppAttributes
    )
{
    CAutoLock lock( &m_lock );
    HRESULT     hr = S_OK;

    SDMFTCHECKHR_GOTO (CheckShutdown(), done);
    SDMFTCHECKNULL_GOTO (ppAttributes, done, E_POINTER);
    *ppAttributes = nullptr;
    if (dwInputStreamID >= m_aryInputPins.size())
    {
        SDMFTCHECKHR_GOTO (MF_E_INVALIDSTREAMNUMBER, done);
    }

    SDMFTCHECKHR_GOTO (m_aryInputPins[dwInputStreamID]->GetStreamAttributes(ppAttributes), done);

done:
    return hr;
}

IFACEMETHODIMP
CSensorTransformMFT::GetOutputAvailableType(
    _In_ DWORD dwOutputStreamID, 
    _In_ DWORD dwTypeIndex, 
    _Out_ IMFMediaType**  ppMediaType
    )
{
    CAutoLock lock( &m_lock );
    HRESULT     hr = S_OK;

    SDMFTCHECKHR_GOTO (CheckShutdown(), done);
    SDMFTCHECKNULL_GOTO (ppMediaType, done, E_POINTER);
    *ppMediaType = nullptr;
    if (dwOutputStreamID >= m_aryOutputPins.size())
    {
        SDMFTCHECKHR_GOTO (MF_E_INVALIDSTREAMNUMBER, done);
    }

    SDMFTCHECKHR_GOTO (m_aryOutputPins[dwOutputStreamID]->GetAvailableMediaType(dwTypeIndex, ppMediaType), done);

done:
    return hr;
}

IFACEMETHODIMP
CSensorTransformMFT::GetOutputCurrentType(
    _In_ DWORD dwOutputStreamID, 
    _COM_Outptr_ IMFMediaType**  ppMediaType
    )
{
    CAutoLock lock( &m_lock );
    HRESULT     hr = S_OK;

    SDMFTCHECKHR_GOTO (CheckShutdown(), done);
    SDMFTCHECKNULL_GOTO (ppMediaType, done, E_POINTER);
    *ppMediaType = nullptr;
    if (dwOutputStreamID >= m_aryOutputPins.size())
    {
        SDMFTCHECKHR_GOTO (MF_E_INVALIDSTREAMNUMBER, done);
    }

    SDMFTCHECKHR_GOTO (m_aryOutputPins[dwOutputStreamID]->GetCurrentMediaType(ppMediaType), done);

done:
    return hr;
}

IFACEMETHODIMP
CSensorTransformMFT::GetOutputStreamAttributes(
    _In_ DWORD dwOutputStreamID, 
    _Out_ IMFAttributes** ppAttributes
    )
{
    CAutoLock lock( &m_lock );
    HRESULT     hr = S_OK;

    SDMFTCHECKHR_GOTO (CheckShutdown(), done);
    SDMFTCHECKNULL_GOTO (ppAttributes, done, E_POINTER);
    *ppAttributes = nullptr;
    if (dwOutputStreamID >= m_aryOutputPins.size())
    {
        SDMFTCHECKHR_GOTO (MF_E_INVALIDSTREAMNUMBER, done);
    }

    SDMFTCHECKHR_GOTO (m_aryOutputPins[dwOutputStreamID]->GetStreamAttributes(ppAttributes), done);

done:
    return hr;
}

IFACEMETHODIMP
CSensorTransformMFT::GetStreamCount(
    _Inout_ DWORD *pdwInputStreams, 
    _Inout_ DWORD *pdwOutputStreams)
{
    CAutoLock lock( &m_lock );
    HRESULT     hr = S_OK;

    SDMFTCHECKHR_GOTO (CheckShutdown(), done);
    SDMFTCHECKNULL_GOTO (pdwInputStreams, done, E_POINTER);
    SDMFTCHECKNULL_GOTO (pdwOutputStreams, done, E_POINTER);
    *pdwInputStreams = (DWORD)m_aryInputPins.size();
    *pdwOutputStreams = (DWORD)m_aryOutputPins.size();

done:
    return hr;
}

IFACEMETHODIMP
CSensorTransformMFT::GetStreamIDs(
    _In_ DWORD  dwInputIDArraySize, 
    _Out_writes_to_(dwInputIDArraySize, m_aryInputPins.size()) DWORD* pdwInputIDs,
    _In_ DWORD  dwOutputIDArraySize, 
    _Out_writes_to_(wOutputIDArraySize, m_aryOutputPins.size()) DWORD* pdwOutputIDs
    )
{
    CAutoLock lock( &m_lock );
    HRESULT     hr = S_OK;

    SDMFTCHECKHR_GOTO (CheckShutdown(), done);
    SDMFTCHECKNULL_GOTO (pdwInputIDs, done, E_POINTER);
    SDMFTCHECKNULL_GOTO (pdwOutputIDs, done, E_POINTER);
    if (dwInputIDArraySize < m_aryInputPins.size() ||
        dwOutputIDArraySize < m_aryOutputPins.size())
    {
        SDMFTCHECKHR_GOTO (MF_E_BUFFERTOOSMALL, done);
    }
    for (DWORD i = 0; i < m_aryInputPins.size(); i++)
    {
        pdwInputIDs[i] = m_aryInputPins[i]->GetStreamID();
    }
    for (DWORD i = 0; i < m_aryOutputPins.size(); i++)
    {
        pdwOutputIDs[i] = m_aryOutputPins[i]->GetStreamID();
    }

done:
    return hr;
}


IFACEMETHODIMP
CSensorTransformMFT::ProcessInput(
    _In_ DWORD dwInputStreamID,
    _In_ IMFSample* pSample,
    _In_ DWORD dwFlags
)
{
    CAutoLock lock(&m_lock);
    HRESULT     hr = S_OK;
    UNREFERENCED_PARAMETER(dwFlags);
    SDMFTCHECKHR_GOTO(CheckShutdown(), done);
    if (dwInputStreamID != 0)
    {
        // We only accept from the first stream. 
        // We have the first stream on the input connected to the first stream on the output
        //
        goto done;
    }

    // Our pins aren't going to be changing, so no need to lock.
    SDMFTCHECKHR_GOTO(m_aryOutputPins[0]->SendInput(pSample), done);

done:
    return hr;
}


IFACEMETHODIMP
CSensorTransformMFT::ProcessOutput(
    _In_ DWORD dwFlags,
    _In_ DWORD cOutputBufferCount,
    _Inout_updates_(cOutputBufferCount) MFT_OUTPUT_DATA_BUFFER *pOutputSamples,
    _Out_ DWORD *pdwStatus
)
{
    CAutoLock lock(&m_lock);
    HRESULT     hr = S_OK;
    DWORD       cOutSize = (DWORD)m_aryOutputPins.size();
    UNREFERENCED_PARAMETER(dwFlags);
    SDMFTCHECKHR_GOTO(CheckShutdown(), done);
    SDMFTCHECKNULL_GOTO(pdwStatus, done, E_POINTER);
    *pdwStatus = 0;
    if (cOutputBufferCount < 1 || nullptr == pOutputSamples)
    {
        SDMFTCHECKHR_GOTO(E_INVALIDARG, done);
    }

    for (DWORD dwIndex = 0; dwIndex < cOutputBufferCount; dwIndex++)
    {
        if (pOutputSamples[dwIndex].dwStreamID >= cOutSize)
        {
            SDMFTCHECKHR_GOTO(MF_E_INVALIDSTREAMNUMBER, done);
        }

        SDMFTCHECKHR_GOTO(m_aryOutputPins[pOutputSamples[dwIndex].dwStreamID]->GetOutput(&(pOutputSamples[dwIndex].pSample)), done);
    }

done:
    return hr;
}

IFACEMETHODIMP
CSensorTransformMFT::ProcessEvent(
    _In_ DWORD dwInputStreamID, 
    _In_ IMFMediaEvent *pEvent
    )
{
    CAutoLock lock( &m_lock );
    HRESULT     hr = S_OK;

    SDMFTCHECKHR_GOTO (CheckShutdown(), done);
    UNREFERENCED_PARAMETER(dwInputStreamID);
    UNREFERENCED_PARAMETER(pEvent);

done:
    return hr;
}


IFACEMETHODIMP
CSensorTransformMFT::ProcessMessage(
    _In_ MFT_MESSAGE_TYPE eMessage, 
    _In_ ULONG_PTR ulParam
    )
{
    CAutoLock lock( &m_lock );
    HRESULT     hr = S_OK;

    SDMFTCHECKHR_GOTO (CheckShutdown(), done);
    // For us, this is a no-op.  We're going to always be in a "streaming" mode,
    // as long as the individual streams are told to stream.
    UNREFERENCED_PARAMETER(eMessage);
    UNREFERENCED_PARAMETER(ulParam);

done:
    return hr;
}

///////////////////////////////////////////////////////////////////////////////////////////////// 
// State operations 
/////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////// 
// Do not take the lock on the input pin operations or change the synchronization mechanism here
// The input operations are mostly called when SetOutputStreamState is in progress
// The state operation is as follows
// SetOutputStreamState calls with State 'S' and MediaType 'M'                                                                                  -- Output Pin operation
//      Acquire Transform Master Lock                                                       
//          MFT sends operation METransformInputStreamStateChanged to the Transform Manager
//              Transform Manager asks for CSensorTransformMFT::GetInputStreamPreferredState from us                                            -- Input Pin operation
//              We reply back with State 'S' and MediaType 'M' or whichever is right according to the internal state design
//          Transform Manager calls back into SetInputStreamState with mediatype 'M' and state 'S' after transmitting changes to the pipeline   -- Input Pin operation
//      Release Transform's Master Lock
// Return from SetOutputStreamState
/////////////////////////////////////////////////////////////////////////////////////////////// 


// INPUT Pin operation
IFACEMETHODIMP
CSensorTransformMFT::GetInputStreamPreferredState(
    _In_ DWORD dwStreamID,
    _Inout_ DeviceStreamState *value,
    _Outptr_opt_result_maybenull_ IMFMediaType **ppMediaType
)
{
    HRESULT hr = S_OK;
    SDMFTCHECKHR_GOTO(CheckShutdown(), done);
    SDMFTCHECKNULL_GOTO(value, done, E_POINTER);
    SDMFTCHECKNULL_GOTO(ppMediaType, done, E_POINTER);
    //
    // Cannot Lock SetInputStreamState because Lock is held in SetOutputStreamState. 
    // All State change Operations come from the output side. 
    //
    if (dwStreamID >= m_aryInputPins.size())
    {
        SDMFTCHECKHR_GOTO(MF_E_INVALIDSTREAMNUMBER, done);
    }

    if (dwStreamID != 0)
    {
        // We don't care about the other streams.
        *value = DeviceStreamState_Stop;
        *ppMediaType = nullptr;
        goto done;
    }
    else
    {
        // get the required parameters from the state stored in the state transition object
        SDMFTCHECKHR_GOTO(m_pStateOp.GetMediaTypeForInput(dwStreamID, ppMediaType, *value), done);
    }

done:
    return hr;
}

// INPUT PIN operation

IFACEMETHODIMP
CSensorTransformMFT::SetInputStreamState(
    _In_ DWORD dwStreamID, 
    _In_ IMFMediaType *pMediaType, 
    _In_ DeviceStreamState value, 
    _In_ DWORD dwFlags 
    )
{
    HRESULT     hr = S_OK;

    UNREFERENCED_PARAMETER(dwFlags);
    SDMFTCHECKHR_GOTO (CheckShutdown(), done);
    if (dwStreamID >= m_aryInputPins.size())
    {
        SDMFTCHECKHR_GOTO (MF_E_INVALIDSTREAMNUMBER, done);
    }
    SDMFTCHECKHR_GOTO (m_aryInputPins[dwStreamID]->SetStreamState(value, pMediaType), done);
    SDMFTCHECKHR_GOTO(m_pStateOp.Finish(dwStreamID),done);
done:
    return hr;
}


// INPUT PIN operation
IFACEMETHODIMP
CSensorTransformMFT::GetInputStreamState(
    _In_ DWORD dwStreamID, 
    _Out_ DeviceStreamState *value 
    )
{
    HRESULT     hr = S_OK;

    SDMFTCHECKHR_GOTO (CheckShutdown(), done);
    SDMFTCHECKNULL_GOTO (value, done, E_POINTER);
    if (dwStreamID >= m_aryInputPins.size())
    {
        SDMFTCHECKHR_GOTO (MF_E_INVALIDSTREAMNUMBER, done);
    }

    *value = m_aryInputPins[dwStreamID]->GetStreamState();

done:
    return hr;
}


// OUTPUT PIN operation.. 
IFACEMETHODIMP
CSensorTransformMFT::SetOutputStreamState(
    _In_ DWORD dwStreamID, 
    _In_ IMFMediaType *pMediaType, 
    _In_ DeviceStreamState state, 
    _In_ DWORD dwFlags 
    )
{
    CAutoLock             lock( &m_lock );
    HRESULT                 hr = S_OK;
    ComPtr<IMFMediaEvent>  spEvent;

    UNREFERENCED_PARAMETER(dwFlags);
    SDMFTCHECKHR_GOTO (CheckShutdown(), done);
    if (dwStreamID >= m_aryOutputPins.size())
    {
        SDMFTCHECKHR_GOTO (MF_E_INVALIDSTREAMNUMBER, done);
    }

    //
    // Send an event to the Sensor Transform manager informing it that the stream has changed
    // This code will make sure the operation on input pin is completed before we move on the 
    // input pin
    //
    SDMFTCHECKHR_GOTO (m_pStateOp.Start(0, dwStreamID, state, pMediaType), done);
    SDMFTCHECKHR_GOTO (m_aryOutputPins[dwStreamID]->SetStreamState(state, pMediaType), done); // Check if the Pin can accept the media type?
    SDMFTCHECKHR_GOTO(MFCreateMediaEvent(METransformInputStreamStateChanged, GUID_NULL, S_OK, nullptr, &spEvent), done);
    SDMFTCHECKHR_GOTO(spEvent->SetUINT32(MF_EVENT_MFT_INPUT_STREAM_ID, (UINT32)dwStreamID), done);
    SDMFTCHECKHR_GOTO(QueueEvent(spEvent.Get()), done);
    // Wait for the operations to finish on the input pin and then leave the Pin after setting the type on the Pin
    SDMFTCHECKHR_GOTO(m_pStateOp.Wait(), done);
done:
    return hr;
}
// OUTPUT PIN operation
IFACEMETHODIMP
CSensorTransformMFT::GetOutputStreamState(
    _In_ DWORD dwStreamID, 
    _Out_ DeviceStreamState *value 
    )
{
    CAutoLock lock( &m_lock );
    HRESULT     hr = S_OK;

    SDMFTCHECKHR_GOTO (CheckShutdown(), done);
    SDMFTCHECKNULL_GOTO (value, done, E_POINTER);
    if (dwStreamID >= m_aryOutputPins.size())
    {
        SDMFTCHECKHR_GOTO (MF_E_INVALIDSTREAMNUMBER, done);
    }

    *value = m_aryOutputPins[dwStreamID]->GetStreamState();

done:
    return hr;
}


///////////////////////////////////////////////////////////////////////////////////////////////// 
// End of State operations 
/////////////////////////////////////////////////////////////////////////////////////////////////

IFACEMETHODIMP
CSensorTransformMFT::FlushInputStream(
    _In_ DWORD dwStreamIndex, 
    _In_ DWORD dwFlags 
    )
{
    CAutoLock lock( &m_lock );
    HRESULT hr = S_OK;

    SDMFTCHECKHR_GOTO (CheckShutdown(), done);

    // Nothing to flush for us.
    UNREFERENCED_PARAMETER(dwStreamIndex);
    UNREFERENCED_PARAMETER(dwFlags);

done:
    return hr;
}

IFACEMETHODIMP
CSensorTransformMFT::FlushOutputStream(
    _In_ DWORD dwStreamIndex, 
    _In_ DWORD dwFlags
    )
{
    CAutoLock lock( &m_lock );
    HRESULT hr = S_OK;
    UNREFERENCED_PARAMETER(dwFlags);
    SDMFTCHECKHR_GOTO (CheckShutdown(), done);
    if (dwStreamIndex >= m_aryOutputPins.size())
    {
        SDMFTCHECKHR_GOTO (MF_E_INVALIDSTREAMNUMBER, done);
    }

    m_aryOutputPins[dwStreamIndex]->Flush();

done:
    return hr;
}


//////////////////////////////////////////////////////////////////////////
// IMFRealTimeClientEx
//////////////////////////////////////////////////////////////////////////
IFACEMETHODIMP
CSensorTransformMFT::RegisterThreadsEx(
    _Inout_ DWORD* pdwTaskIndex, 
    _In_ LPCWSTR wszClassName, 
    _In_ LONG lBasePriority 
    )
{
    UNREFERENCED_PARAMETER(pdwTaskIndex);
    UNREFERENCED_PARAMETER(wszClassName);
    UNREFERENCED_PARAMETER(lBasePriority);
    return S_OK;
}

IFACEMETHODIMP
CSensorTransformMFT::UnregisterThreads(
    )
{
    return S_OK;
}

IFACEMETHODIMP
CSensorTransformMFT::SetWorkQueueEx(
    _In_ DWORD dwWorkQueueId, 
    _In_ LONG lWorkItemBasePriority
    )
{
    m_dwWorkQueueId = dwWorkQueueId;
    m_lWorkItemBasePriority = lWorkItemBasePriority;
    return S_OK;
}


//////////////////////////////////////////////////////////////////////////
// IMFShutdown Implementation
//////////////////////////////////////////////////////////////////////////
IFACEMETHODIMP
CSensorTransformMFT::Shutdown(
    )
{
    //
    // First set the state operation object to go into shutdown. Maybe there is a state operation in progress
    // which will get unblocked when the shutdown is called. Then hold the lock so no other operations 
    // sneak through while shutdown is in progress
    //
    HRESULT     hr = S_OK;
    DWORD       cCount = 0;
   
    if (SUCCEEDED(hr = m_pStateOp.ShutDown()))
    {
        CAutoLock lock(&m_lock);
        if (m_fShutdown)
        {
            SDMFTCHECKHR_GOTO(MF_E_SHUTDOWN, done);
        }
        m_fShutdown = true;
        cCount = (DWORD)m_aryInputPins.size();
        for (DWORD i = 0; i < cCount; i++)
        {
            m_aryInputPins[i]->Shutdown();
        }

        cCount = (DWORD)m_aryOutputPins.size();
        for (DWORD i = 0; i < cCount; i++)
        {
            m_aryOutputPins[i]->Shutdown();
        }
        SDMFTCHECKHR_GOTO(ShutdownEventGenerator(), done);
    }
done:
    return hr;
}

IFACEMETHODIMP
CSensorTransformMFT::GetShutdownStatus(
    _Out_ MFSHUTDOWN_STATUS *pStatus
    )
{
    CAutoLock lock( &m_lock );
    HRESULT     hr = S_OK;

    SDMFTCHECKNULL_GOTO (pStatus, done, E_POINTER);
    if (m_fShutdown)
    {
        //
        // Shutdown and GetShutdownStatus are in a critical section. We are either
        // Shutdwon completed or return MF_E_INVALIDREQUEST
        //
        *pStatus = MFSHUTDOWN_COMPLETED;
    }
    else
    {
        hr = MF_E_INVALIDREQUEST;
    }
done:
    return hr;
}


///
/// IKSControl Inferface function declarations
IFACEMETHODIMP
CSensorTransformMFT::KsEvent(
    _In_reads_bytes_(ulEventLength) PKSEVENT pEvent, 
    _In_ ULONG ulEventLength, 
    _Inout_updates_bytes_opt_(ulDataLength) LPVOID pEventData, 
    _In_ ULONG ulDataLength, 
    _Inout_ ULONG* pBytesReturned
    )
{
    CAutoLock lock( &m_lock );
    HRESULT     hr = S_OK;

    SDMFTCHECKHR_GOTO (CheckShutdown(), done);
    SDMFTCHECKHR_GOTO (m_spKsControl->KsEvent(pEvent, ulEventLength, pEventData, ulDataLength, pBytesReturned), done);

done:
    return hr;
}

IFACEMETHODIMP
CSensorTransformMFT::KsProperty(
    _In_reads_bytes_(ulPropertyLength) PKSPROPERTY pProperty, 
    _In_ ULONG ulPropertyLength, 
    _Inout_updates_bytes_(ulDataLength) LPVOID pPropertyData, 
    _In_ ULONG ulDataLength, 
    _Inout_ ULONG* pBytesReturned)
{
    CAutoLock lock( &m_lock );
    HRESULT     hr = S_OK;

    SDMFTCHECKHR_GOTO (CheckShutdown(), done);
    SDMFTCHECKHR_GOTO (m_spKsControl->KsProperty(pProperty, ulPropertyLength, pPropertyData, ulDataLength, pBytesReturned), done)

done:
    return hr;
}

IFACEMETHODIMP
CSensorTransformMFT::KsMethod(
    _In_reads_bytes_(ulPropertyLength) PKSMETHOD pProperty, 
    _In_ ULONG ulPropertyLength, 
    _Inout_updates_bytes_(ulDataLength) LPVOID pPropertyData, 
    _In_ ULONG ulDataLength, 
    _Inout_ ULONG* pBytesReturned
    )
{
    CAutoLock lock( &m_lock );
    HRESULT     hr = S_OK;

    SDMFTCHECKHR_GOTO (CheckShutdown(), done);
    SDMFTCHECKHR_GOTO (m_spKsControl->KsMethod(pProperty, ulPropertyLength, pPropertyData, ulDataLength, pBytesReturned), done);

done:
    return hr;
}
//////////////////////////////////////////////////////////////////////////
// IMFAttributes implementation
//////////////////////////////////////////////////////////////////////////
IFACEMETHODIMP
CSensorTransformMFT::GetItem(
    _In_ REFGUID guidKey,
    _Inout_opt_  PROPVARIANT* pValue
    )
{
    return m_spDMFTAttributes->GetItem(guidKey, pValue);
}

IFACEMETHODIMP
CSensorTransformMFT::GetItemType(
    _In_ REFGUID guidKey,
    _Out_ MF_ATTRIBUTE_TYPE* pType
    )
{
    return m_spDMFTAttributes->GetItemType(guidKey, pType);
}

IFACEMETHODIMP
CSensorTransformMFT::CompareItem(
    _In_ REFGUID guidKey,
    _In_ REFPROPVARIANT Value,
    _Out_ BOOL* pbResult
    )
{
    return m_spDMFTAttributes->CompareItem(guidKey, Value, pbResult);
}

IFACEMETHODIMP
CSensorTransformMFT::Compare(
    _In_ IMFAttributes* pTheirs,
    _In_ MF_ATTRIBUTES_MATCH_TYPE MatchType,
    _Out_ BOOL* pbResult
    )
{
    return m_spDMFTAttributes->Compare(pTheirs, MatchType, pbResult);
}

IFACEMETHODIMP
CSensorTransformMFT::GetUINT32(
    _In_ REFGUID guidKey,
    _Out_ UINT32* punValue
    )
{
    return m_spDMFTAttributes->GetUINT32(guidKey, punValue);
}

IFACEMETHODIMP
CSensorTransformMFT::GetUINT64(
    _In_ REFGUID guidKey,
    _Out_ UINT64* punValue
    )
{
    return m_spDMFTAttributes->GetUINT64(guidKey, punValue);
}

IFACEMETHODIMP
CSensorTransformMFT::GetDouble(
    _In_ REFGUID guidKey,
    _Out_ double* pfValue
    )
{
    return m_spDMFTAttributes->GetDouble(guidKey, pfValue);
}

IFACEMETHODIMP
CSensorTransformMFT::GetGUID(
    _In_ REFGUID guidKey,
    _Out_ GUID* pguidValue
    )
{
    return m_spDMFTAttributes->GetGUID(guidKey, pguidValue);
}

IFACEMETHODIMP
CSensorTransformMFT::GetStringLength(
    _In_ REFGUID guidKey,
    _Out_ UINT32* pcchLength
    )
{
    return m_spDMFTAttributes->GetStringLength(guidKey, pcchLength);
}

IFACEMETHODIMP
CSensorTransformMFT::GetString(
    _In_ REFGUID guidKey,
    _Out_writes_(cchBufSize) LPWSTR pwszValue,
    _In_ UINT32 cchBufSize,
    _Inout_opt_ UINT32* pcchLength
    )
{
    return m_spDMFTAttributes->GetString(guidKey, pwszValue, cchBufSize, pcchLength);
}

IFACEMETHODIMP
CSensorTransformMFT::GetAllocatedString(
    _In_ REFGUID guidKey,
    _Out_writes_(*pcchLength + 1) LPWSTR* ppwszValue,
    _Inout_  UINT32* pcchLength
    )
{
    return m_spDMFTAttributes->GetAllocatedString(guidKey, ppwszValue, pcchLength);
}

IFACEMETHODIMP
CSensorTransformMFT::GetBlobSize(
    _In_ REFGUID guidKey,
    _Out_ UINT32* pcbBlobSize
    )
{
    return m_spDMFTAttributes->GetBlobSize(guidKey, pcbBlobSize);
}

IFACEMETHODIMP
CSensorTransformMFT::GetBlob(
    _In_                    REFGUID  guidKey,
    _Out_writes_(cbBufSize) UINT8* pBuf,
    UINT32 cbBufSize,
    _Inout_  UINT32* pcbBlobSize
    )
{
    return m_spDMFTAttributes->GetBlob(guidKey, pBuf, cbBufSize, pcbBlobSize);
}

IFACEMETHODIMP
CSensorTransformMFT::GetAllocatedBlob(
    __RPC__in REFGUID guidKey,
    __RPC__deref_out_ecount_full_opt(*pcbSize) UINT8** ppBuf,
    __RPC__out UINT32* pcbSize
    )
{
    return m_spDMFTAttributes->GetAllocatedBlob(guidKey, ppBuf, pcbSize);
}

IFACEMETHODIMP
CSensorTransformMFT::GetUnknown(
    __RPC__in REFGUID guidKey,
    __RPC__in REFIID riid,
    __RPC__deref_out_opt LPVOID *ppv
    )
{
    return m_spDMFTAttributes->GetUnknown(guidKey, riid, ppv);
}

IFACEMETHODIMP
CSensorTransformMFT::SetItem(
    _In_ REFGUID guidKey,
    _In_ REFPROPVARIANT Value
    )
{
    return m_spDMFTAttributes->SetItem(guidKey, Value);
}

IFACEMETHODIMP
CSensorTransformMFT::DeleteItem(
    _In_ REFGUID guidKey
    )
{
    return m_spDMFTAttributes->DeleteItem(guidKey);
}

IFACEMETHODIMP
CSensorTransformMFT::DeleteAllItems()
{
    return m_spDMFTAttributes->DeleteAllItems();
}

IFACEMETHODIMP
CSensorTransformMFT::SetUINT32(
    _In_ REFGUID guidKey,
    _In_ UINT32  unValue
    )
{
    return m_spDMFTAttributes->SetUINT32(guidKey, unValue);
}

IFACEMETHODIMP
CSensorTransformMFT::SetUINT64(
    _In_ REFGUID guidKey,
    _In_ UINT64  unValue
    )
{
    return m_spDMFTAttributes->SetUINT64(guidKey, unValue);
}

IFACEMETHODIMP
CSensorTransformMFT::SetDouble(
    _In_ REFGUID guidKey,
    _In_ double  fValue
    )
{
    return m_spDMFTAttributes->SetDouble(guidKey, fValue);
}

IFACEMETHODIMP
CSensorTransformMFT::SetGUID(
    _In_ REFGUID guidKey,
    _In_ REFGUID guidValue
    )
{
    return m_spDMFTAttributes->SetGUID(guidKey, guidValue);
}

IFACEMETHODIMP
CSensorTransformMFT::SetString(
    _In_ REFGUID guidKey,
    _In_ LPCWSTR wszValue
    )
{
    return m_spDMFTAttributes->SetString(guidKey, wszValue);
}

IFACEMETHODIMP
CSensorTransformMFT::SetBlob(
    _In_ REFGUID guidKey,
    _In_reads_(cbBufSize) const UINT8* pBuf,
    UINT32 cbBufSize
    )
{
    return m_spDMFTAttributes->SetBlob(guidKey, pBuf, cbBufSize);
}

IFACEMETHODIMP
CSensorTransformMFT::SetUnknown(
    _In_ REFGUID guidKey,
    _In_ IUnknown* pUnknown
    )
{
    return m_spDMFTAttributes->SetUnknown(guidKey, pUnknown);
}

IFACEMETHODIMP
CSensorTransformMFT::LockStore()
{
    return m_spDMFTAttributes->LockStore();
}

IFACEMETHODIMP
CSensorTransformMFT::UnlockStore()
{
    return m_spDMFTAttributes->UnlockStore();
}

IFACEMETHODIMP
CSensorTransformMFT::GetCount(
    _Out_ UINT32* pcItems
    )
{
    return m_spDMFTAttributes->GetCount(pcItems);
}

IFACEMETHODIMP
CSensorTransformMFT::GetItemByIndex(
    UINT32 unIndex,
    _Out_ GUID* pguidKey,
    _Inout_ PROPVARIANT* pValue
    )
{
    return m_spDMFTAttributes->GetItemByIndex(unIndex, pguidKey, pValue);
}

IFACEMETHODIMP
CSensorTransformMFT::CopyAllItems(
    _In_ IMFAttributes* pDest
    )
{
    return m_spDMFTAttributes->CopyAllItems(pDest);
}

//////////////////////////////////////////////////////////////////////////
/// IMFSensorTransformFactory Implementation
//////////////////////////////////////////////////////////////////////////
//
// Please note, This should usually be a part of another Class. According to
// the guid passed out of GetTransformInformation, We should return the Transform
// that corresponds to the GUID.
// For example if a Factory supports two transforms A and B then A and B should preferably have thier own subclasses 
// and the Factory Interface should have it's own. The Factory should be able to hand out whatever Transform is requested
// from CreateTransform method.
//
IFACEMETHODIMP
CSensorTransformMFT::GetFactoryAttributes(
    _COM_Outptr_ IMFAttributes** ppAttributes
    )
{
    HRESULT hr = S_OK;

    SDMFTCHECKNULL_GOTO (ppAttributes, done, E_POINTER);
    *ppAttributes = nullptr;

    SDMFTCHECKNULL_GOTO (m_spDMFTAttributes, done, MF_E_NOT_INITIALIZED);
    SDMFTCHECKHR_GOTO (m_spDMFTAttributes->QueryInterface(IID_PPV_ARGS(ppAttributes)), done);

done:
    return hr;
}

IFACEMETHODIMP
CSensorTransformMFT::InitializeFactory(
    _In_ DWORD dwMaxTransformCount,
    _In_ IMFCollection* pSensorDevices, 
    _In_opt_ IMFAttributes* pAttributes
    )
{
    HRESULT hr = S_OK;
    DWORD   cElementCount = 0;
    DWORD   StreamIndex = 0;
    UNREFERENCED_PARAMETER(pAttributes); //Unused..
    // We're going to provide 1 ST, so the max allowed has to be at least 1.
    if (dwMaxTransformCount < 1)
    {
        SDMFTCHECKHR_GOTO (E_INVALIDARG, done);
    }
    SDMFTCHECKNULL_GOTO (pSensorDevices, done, E_INVALIDARG);

    SDMFTCHECKHR_GOTO (pSensorDevices->GetElementCount(&cElementCount), done);
    for (DWORD dwSrcIndex = 0; dwSrcIndex < cElementCount; dwSrcIndex++)
    {
        ComPtr<IUnknown> spUnknown;
        ComPtr<IMFSensorDevice> spSensorDevice;
        DWORD dwStreamCount = 0;

        SDMFTCHECKHR_GOTO (pSensorDevices->GetElement(dwSrcIndex, &spUnknown), done);
        SDMFTCHECKHR_GOTO (spUnknown->QueryInterface(IID_PPV_ARGS(&spSensorDevice)), done);
        SDMFTCHECKHR_GOTO (spSensorDevice->GetStreamAttributesCount(MFSensorStreamType_Output, 
                                                               &dwStreamCount), done);
        for (DWORD dwStreamIndex = 0; dwStreamIndex < dwStreamCount; dwStreamIndex++)
        {
            ComPtr<CSensorTransformMFTPin>  spInputPin;
            ComPtr<IMFAttributes>      spAttributes;
            ComPtr<IMFSensorStream>    spDeviceStream;

            ComPtr<IMFSensorStream>    spSTStream;
            ComPtr<IMFCollection> spMTCollection;
            UINT32 StreamId = 0;

            SDMFTCHECKHR_GOTO (spSensorDevice->GetStreamAttributes(MFSensorStreamType_Output, dwStreamIndex, &spAttributes), done);
            SDMFTCHECKHR_GOTO (spAttributes->GetUINT32(MF_DEVICESTREAM_STREAM_ID, &StreamId), done);
            SDMFTCHECKHR_GOTO (spAttributes->QueryInterface(IID_PPV_ARGS(&spDeviceStream)), done);

            SDMFTCHECKHR_GOTO (CSensorTransformMFTPin::CreateInstance(StreamIndex, spDeviceStream.Get(), spInputPin.GetAddressOf()), done);
            hr = ExceptionBoundary([this, spInputPin]() {m_aryInputPins.push_back(spInputPin);});
            SDMFTCHECKHR_GOTO (hr, done);
            if (StreamIndex == 0)
            {
                ComPtr<CSensorTransformMFTPin>        spOutPin;

                // Create our output pin, which should just be the 0th Input pin.
                SDMFTCHECKHR_GOTO (CSensorTransformMFTPin::CreateInstance(0, spDeviceStream.Get(), spOutPin.GetAddressOf()), done);
                SDMFTCHECKHR_GOTO (spOutPin->SetString(MF_DEVSOURCE_ATTRIBUTE_FRIENDLY_NAME, s_SensorTransformName), done);
                hr = ExceptionBoundary([this, spOutPin]() {m_aryOutputPins.push_back(spOutPin);});
                SDMFTCHECKHR_GOTO (hr, done);
            }
            StreamIndex++;
        }
    }

done:
    return hr;
}

IFACEMETHODIMP
CSensorTransformMFT::GetTransformCount(
    _Out_ DWORD* pdwCount
    )
{
    HRESULT hr = S_OK;

    SDMFTCHECKNULL_GOTO (pdwCount, done, E_POINTER);
    *pdwCount = 1; // One Transform exposed as of now
done:
    return hr;
}

IFACEMETHODIMP
CSensorTransformMFT::GetTransformInformation(
    _In_ DWORD TransformIndex, 
    _Out_ GUID* pguidTransformId, 
    _COM_Outptr_result_maybenull_ IMFAttributes** ppAttributes, 
    _COM_Outptr_ IMFCollection** ppOutputStreams
    )
{
    HRESULT                 hr = S_OK;
    ComPtr<IMFCollection>  spOutputStreams;
    ComPtr<IUnknown>       spUnknown;
    UNREFERENCED_PARAMETER(TransformIndex);
    SDMFTCHECKNULL_GOTO (ppOutputStreams, done, E_POINTER);
    *ppOutputStreams = nullptr;
    SDMFTCHECKNULL_GOTO (pguidTransformId, done, E_POINTER);
    *pguidTransformId = s_guidOurSTId;
    if (ppAttributes != nullptr)
    {
        // No optional ST attributes to report.  But this is how we tell the
        // Sensor Group if there's some special transform handling logic needed
        // here or not (like maybe we need to activate the Location services,
        // in which case, we report that we need access to Location services
        // for some of our streams--which means the activation code path will
        // have to check if the client app touching the stream has the neccessary
        // rights to use the location services).
        *ppAttributes = nullptr;
    }

    SDMFTCHECKHR_GOTO (MFCreateCollection(&spOutputStreams), done);
    for (DWORD i = 0; i < m_aryOutputPins.size(); i++)
    {
        ComPtr<IMFAttributes>  spAttributes;
        ComPtr<IUnknown>       spUnknown;

        SDMFTCHECKHR_GOTO (m_aryOutputPins[i]->GetStreamAttributes(&spAttributes), done);
        SDMFTCHECKHR_GOTO (spAttributes->QueryInterface(IID_PPV_ARGS(&spUnknown)), done);
        SDMFTCHECKHR_GOTO (spOutputStreams->AddElement(spUnknown.Get()), done);
    }
    *ppOutputStreams = spOutputStreams.Detach();

done:
    return hr;
}

IFACEMETHODIMP
CSensorTransformMFT::CreateTransform(
    _In_ REFGUID TransformID, 
    _In_opt_ IMFAttributes* pAttributes, 
    _COM_Outptr_ IMFDeviceTransform** ppDeviceMFT
    )
{
    HRESULT hr = S_OK;

    // Validate input parameters.
    UNREFERENCED_PARAMETER(pAttributes);
    UNREFERENCED_PARAMETER(TransformID); 
    SDMFTCHECKNULL_GOTO (ppDeviceMFT, done, E_POINTER);
    *ppDeviceMFT = nullptr;
    if (!IsEqualCLSID(TransformID, s_guidOurSTId))
    {
        // We have an incorrect GUID here.. But we expose. Log it and create the default transform
        SDMFTTRACE(SENSORDEVICEMFT_LEVEL_INFO, L" Incorrect GUID requested Passing default transform ");
    }
    // Create out device transform
    SDMFTCHECKHR_GOTO (QueryInterface(IID_PPV_ARGS(ppDeviceMFT)), done);

done:
    return hr;
}

/////////////////////////////////////////////////////////////////////////////////////////////// 
///  
HRESULT
CSensorTransformMFT::InitializeDMFT(
    )
{
    HRESULT         hr = S_OK;
    ULARGE_INTEGER  Timestamp;

    SDMFTCHECKHR_GOTO (MFCreateAttributes(&m_spDMFTAttributes, 4), done);
    // Friendly Name is used by the pipeline to display as the camera name to the user
    SDMFTCHECKHR_GOTO (m_spDMFTAttributes->SetString(MF_DEVSOURCE_ATTRIBUTE_FRIENDLY_NAME, s_SensorTransformName), done);
    // Set this device as a sensor transform. 
    SDMFTCHECKHR_GOTO (m_spDMFTAttributes->SetUINT32(MF_DEVSOURCE_ATTRIBUTE_DEVICETYPE_priv, (UINT32)MFSensorDeviceType_SensorTransform), done);
    SDMFTCHECKHR_GOTO (m_spDMFTAttributes->SetString(MF_STF_VERSION_INFO, s_FactoryVersion), done);
    if (!SystemTimeToFileTime(&s_FactoryDateTime, (FILETIME*)&Timestamp))
    {
        SDMFTCHECKHR_GOTO (HRESULT_FROM_WIN32(GetLastError()), done);
    }
    SDMFTCHECKHR_GOTO (m_spDMFTAttributes->SetUINT64(MF_STF_VERSION_DATE, (UINT64)Timestamp.QuadPart), done);

done:
    return hr;
}

