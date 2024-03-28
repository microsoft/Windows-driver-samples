//
// AecApoMFX.cpp -- Copyright (c) Microsoft Corporation. All rights reserved.
//
// Description:
//
//  Implementation of CAecApoMFX
//

#include <atlbase.h>
#include <atlcom.h>
#include <atlcoll.h>
#include <atlsync.h>
#include <mmreg.h>

#include <initguid.h>
#include <audioenginebaseapo.h>
#include <baseaudioprocessingobject.h>
#include <resource.h>

#include <float.h>

#include "AecApo.h"
#include <devicetopology.h>
#include <CustomPropKeys.h>

#define SUPPORTED_AEC_SAMPLINGRATE (16000)

// Static declaration of the APO_REG_PROPERTIES structure
// associated with this APO.  The number in <> brackets is the
// number of IIDs supported by this APO.  If more than one, then additional
// IIDs are added at the end
#pragma warning (disable : 4815)
const AVRT_DATA CRegAPOProperties<1> CAecApoMFX::sm_RegProperties(
    __uuidof(AecApoMFX),                           // clsid of this APO
    L"Sample MFX Aec APO",                         // friendly name of this APO
    L"Copyright (c) Microsoft Corporation",         // copyright info
    1,                                              // major version #
    0,                                              // minor version #
    __uuidof(IAudioProcessingObject),               // iid of primary interface
    (APO_FLAG) (APO_FLAG_BITSPERSAMPLE_MUST_MATCH | APO_FLAG_FRAMESPERSECOND_MUST_MATCH), // kak check this
    DEFAULT_APOREG_MININPUTCONNECTIONS,
    DEFAULT_APOREG_MAXINPUTCONNECTIONS,
    DEFAULT_APOREG_MINOUTPUTCONNECTIONS,
    DEFAULT_APOREG_MAXOUTPUTCONNECTIONS,
    DEFAULT_APOREG_MAXINSTANCES
    );


#pragma AVRT_CODE_BEGIN
//-------------------------------------------------------------------------
// Description:
//
//  Do the actual processing of data.
//
// Parameters:
//
//      u32NumInputConnections      - [in] number of input connections
//      ppInputConnections          - [in] pointer to list of input APO_CONNECTION_PROPERTY pointers
//      u32NumOutputConnections      - [in] number of output connections
//      ppOutputConnections         - [in] pointer to list of output APO_CONNECTION_PROPERTY pointers
//
// Return values:
//
//      void
//
// Remarks:
//
//  This function processes data in a manner dependent on the implementing
//  object.  This routine can not fail and can not block, or call any other
//  routine that blocks, or touch pagable memory.
//
STDMETHODIMP_(void) CAecApoMFX::APOProcess(
    UINT32 u32NumInputConnections,
    APO_CONNECTION_PROPERTY** ppInputConnections,
    UINT32 u32NumOutputConnections,
    APO_CONNECTION_PROPERTY** ppOutputConnections)
{
    UNREFERENCED_PARAMETER(u32NumInputConnections);
    UNREFERENCED_PARAMETER(u32NumOutputConnections);

    FLOAT32 *pf32InputFrames, *pf32OutputFrames;

    ATLASSERT(m_bIsLocked);

    // assert that the number of input and output connectins fits our registration properties
    ATLASSERT(m_pRegProperties->u32MinInputConnections <= u32NumInputConnections);
    ATLASSERT(m_pRegProperties->u32MaxInputConnections >= u32NumInputConnections);
    ATLASSERT(m_pRegProperties->u32MinOutputConnections <= u32NumOutputConnections);
    ATLASSERT(m_pRegProperties->u32MaxOutputConnections >= u32NumOutputConnections);

    ATLASSERT(ppInputConnections[0]->u32Signature == APO_CONNECTION_PROPERTY_V2_SIGNATURE);
    ATLASSERT(ppOutputConnections[0]->u32Signature == APO_CONNECTION_PROPERTY_V2_SIGNATURE);

    APO_CONNECTION_PROPERTY_V2* inConnection = reinterpret_cast<APO_CONNECTION_PROPERTY_V2*>(ppInputConnections[0]);
    APO_CONNECTION_PROPERTY_V2* outConnection = reinterpret_cast<APO_CONNECTION_PROPERTY_V2*>(ppOutputConnections[0]);

    // check APO_BUFFER_FLAGS.
    switch( ppInputConnections[0]->u32BufferFlags )
    {
        case BUFFER_INVALID:
        {
            ATLASSERT(false);  // invalid flag - should never occur.  don't do anything.
            break;
        }
        case BUFFER_VALID:
        case BUFFER_SILENT:
        {
            // get input pointer to connection buffer
            pf32InputFrames = reinterpret_cast<FLOAT32*>(ppInputConnections[0]->pBuffer);
            ATLASSERT( IS_VALID_TYPED_READ_POINTER(pf32InputFrames) );

            // get output pointer to connection buffer
            pf32OutputFrames = reinterpret_cast<FLOAT32*>(ppOutputConnections[0]->pBuffer);
            ATLASSERT( IS_VALID_TYPED_WRITE_POINTER(pf32OutputFrames) );

            //
            // Provide microphone buffer and timestamps to AEC algorithm
            //
            UNREFERENCED_PARAMETER(inConnection);
            UNREFERENCED_PARAMETER(outConnection);

            // Set the valid frame count.
            ppOutputConnections[0]->u32ValidFrameCount = ppInputConnections[0]->u32ValidFrameCount;
            ppOutputConnections[0]->u32BufferFlags = ppInputConnections[0]->u32BufferFlags;

            break;
        }
        default:
        {
            ATLASSERT(false);  // invalid flag - should never occur
            break;
        }
    } // switch

} // APOProcess
#pragma AVRT_CODE_END

//-------------------------------------------------------------------------
// Description:
//
// Parameters:
//
//      pTime                       - [out] hundreds-of-nanoseconds
//
// Return values:
//
//      S_OK on success, a failure code on failure
STDMETHODIMP CAecApoMFX::GetLatency(HNSTIME* pTime)  
{  
    ASSERT_NONREALTIME();  
    HRESULT hr = S_OK;  
  
    IF_TRUE_ACTION_JUMP(NULL == pTime, hr = E_POINTER, Exit);  
  
    *pTime = 0;

Exit:  
    return hr;  
}


//-------------------------------------------------------------------------
// Description:
//
//  Verifies that the APO is ready to process and locks its state if so.
//
// Parameters:
//
//      u32NumInputConnections - [in] number of input connections attached to this APO
//      ppInputConnections - [in] connection descriptor of each input connection attached to this APO
//      u32NumOutputConnections - [in] number of output connections attached to this APO
//      ppOutputConnections - [in] connection descriptor of each output connection attached to this APO
//
// Return values:
//
//      S_OK                                Object is locked and ready to process.
//      E_POINTER                           Invalid pointer passed to function.
//      APOERR_INVALID_CONNECTION_FORMAT    Invalid connection format.
//      APOERR_NUM_CONNECTIONS_INVALID      Number of input or output connections is not valid on
//                                          this APO.
STDMETHODIMP CAecApoMFX::LockForProcess(UINT32 u32NumInputConnections,
    APO_CONNECTION_DESCRIPTOR** ppInputConnections,  
    UINT32 u32NumOutputConnections, APO_CONNECTION_DESCRIPTOR** ppOutputConnections)
{
    ASSERT_NONREALTIME();
    HRESULT hr = S_OK;

    UNCOMPRESSEDAUDIOFORMAT  uncompAudioFormat;

    // fill in the samples per frame for the output (since APO_FLAG_SAMPLESPERFRAME_MUST_MATCH is not selected)
    // There are two potentially different samples per frame values here. The input, which will be interleaved + primary. 
    // And the output, which is just the primary. Because this is used for clearing the zeroing the output buffer, we're going
    // to fill it in with the output samples per frame. ProcessBuffer has both.
    hr = ppOutputConnections[0]->pFormat->GetUncompressedAudioFormat(&uncompAudioFormat);
    IF_FAILED_JUMP(hr, Exit);

    m_u32SamplesPerFrame = uncompAudioFormat.dwSamplesPerFrame;

    hr = CBaseAudioProcessingObject::LockForProcess(u32NumInputConnections,
        ppInputConnections, u32NumOutputConnections, ppOutputConnections);
    IF_FAILED_JUMP(hr, Exit);
    
Exit:
    return hr;
}

// The method that this long comment refers to is "Initialize()"
//-------------------------------------------------------------------------
// Description:
//
//  Generic initialization routine for APOs.
//
// Parameters:
//
//     cbDataSize - [in] the size in bytes of the initialization data.
//     pbyData - [in] initialization data specific to this APO
//
// Return values:
//
//     S_OK                         Successful completion.
//     E_POINTER                    Invalid pointer passed to this function.
//     E_INVALIDARG                 Invalid argument
//     AEERR_ALREADY_INITIALIZED    APO is already initialized
//
// Remarks:
//
//  This method initializes the APO.  The data is variable length and
//  should have the form of:
//
//    struct MyAPOInitializationData
//    {
//        APOInitBaseStruct APOInit;
//        ... // add additional fields here
//    };
//
//  If the APO needs no initialization or needs no data to initialize
//  itself, it is valid to pass NULL as the pbyData parameter and 0 as
//  the cbDataSize parameter.
//
//  As part of designing an APO, decide which parameters should be
//  immutable (set once during initialization) and which mutable (changeable
//  during the lifetime of the APO instance).  Immutable parameters must
//  only be specifiable in the Initialize call; mutable parameters must be
//  settable via methods on whichever parameter control interface(s) your
//  APO provides. Mutable values should either be set in the initialize
//  method (if they are required for proper operation of the APO prior to
//  LockForProcess) or default to reasonable values upon initialize and not
//  be required to be set before LockForProcess.
//
//  Within the mutable parameters, you must also decide which can be changed
//  while the APO is locked for processing and which cannot.
//
//  All parameters should be considered immutable as a first choice, unless
//  there is a specific scenario which requires them to be mutable; similarly,
//  no mutable parameters should be changeable while the APO is locked, unless
//  a specific scenario requires them to be.  Following this guideline will
//  simplify the APO's state diagram and implementation and prevent certain
//  types of bug.
//
//  If a parameter changes the APOs latency or MaxXXXFrames values, it must be
//  immutable.
//
//  The default version of this function uses no initialization data, but does verify
//  the passed parameters and set the m_bIsInitialized member to true.
//
//  Note: This method may not be called from a real-time processing thread.
//

HRESULT CAecApoMFX::Initialize(UINT32 cbDataSize, BYTE* pbyData)
{
    HRESULT                     hr = S_OK;

    IF_TRUE_ACTION_JUMP( ((NULL == pbyData) && (0 != cbDataSize)), hr = E_INVALIDARG, Exit);
    IF_TRUE_ACTION_JUMP( ((NULL != pbyData) && (0 == cbDataSize)), hr = E_INVALIDARG, Exit);

    if (cbDataSize == sizeof(APOInitSystemEffects3))
    {
        //
        // pbyData contains APOInitSystemEffects3 structure describing the microphone endpoint
        //
        APOInitSystemEffects3* papoSysFxInit3 = (APOInitSystemEffects3*)pbyData;
        m_initializedForEffectsDiscovery = papoSysFxInit3->InitializeForDiscoveryOnly;

        // Support for COMMUNICATIONS mode only when streaming
        IF_TRUE_ACTION_JUMP(
            !m_initializedForEffectsDiscovery && (papoSysFxInit3->AudioProcessingMode != AUDIO_SIGNALPROCESSINGMODE_COMMUNICATIONS),
            hr = E_INVALIDARG,
            Exit);
        m_audioSignalProcessingMode = papoSysFxInit3->AudioProcessingMode;

        // Register for notification of endpoint volume change in GetApoNotificationRegistrationInfo      
        // Keep a reference to the device that will be registering for endpoint volume notifcations. 

        IF_TRUE_ACTION_JUMP(papoSysFxInit3->pDeviceCollection == nullptr, hr = E_INVALIDARG, Exit);
        // Get the endpoint on which this APO has been created. It is the last device in the device collection.
        UINT32 numDevices;
        hr = papoSysFxInit3->pDeviceCollection->GetCount(&numDevices);
        IF_FAILED_JUMP(hr, Exit);
        IF_TRUE_ACTION_JUMP(numDevices <= 0, hr = E_INVALIDARG, Exit);

        hr = papoSysFxInit3->pDeviceCollection->Item(numDevices - 1, &m_spCaptureDevice);
        IF_FAILED_JUMP(hr, Exit);

        m_bIsInitialized = true;

        // Try to get the logging service, but ignore errors as failure to do logging it is not fatal.
        if(SUCCEEDED(papoSysFxInit3->pServiceProvider->QueryService(SID_AudioProcessingObjectLoggingService, IID_PPV_ARGS(&m_apoLoggingService))))
        {
            m_apoLoggingService->ApoLog(APO_LOG_LEVEL_INFO, L"CAecApoMFX::Initialize called with APOInitSystemEffects3.");
        }        
    }
    else if (cbDataSize == sizeof(APOInitSystemEffects2))
    {
        //
        // pbyData contains APOInitSystemEffects2 structure describing the microphone endpoint
        //
        APOInitSystemEffects2* papoSysFxInit2 = (APOInitSystemEffects2*)pbyData;
        m_initializedForEffectsDiscovery = papoSysFxInit2->InitializeForDiscoveryOnly;

        // Support for COMMUNICATIONS mode only when streaming
        IF_TRUE_ACTION_JUMP(
            !m_initializedForEffectsDiscovery && (papoSysFxInit2->AudioProcessingMode != AUDIO_SIGNALPROCESSINGMODE_COMMUNICATIONS),
            hr = E_INVALIDARG,
            Exit);
        m_audioSignalProcessingMode = papoSysFxInit2->AudioProcessingMode;

        m_bIsInitialized = true;
    }
    else
    {
        hr = E_INVALIDARG;
    }

Exit:
    return hr;
}

//-------------------------------------------------------------------------
// Description:
//
//
//
// Parameters:
//
//
//
// Return values:
//
//
//
// Remarks:
//
//
STDMETHODIMP CAecApoMFX::GetEffectsList(_Outptr_result_buffer_maybenull_(*pcEffects) LPGUID *ppEffectsIds, _Out_ UINT *pcEffects, _In_ HANDLE Event)
{
    UNREFERENCED_PARAMETER(Event);

    *ppEffectsIds = NULL;
    *pcEffects = 0;

    if (m_audioSignalProcessingMode == AUDIO_SIGNALPROCESSINGMODE_COMMUNICATIONS)
    {
        // Return the list of effects implemented by this APO for COMMUNICATIONS processing mode  
        static const GUID effectsList[] = { AUDIO_EFFECT_TYPE_ACOUSTIC_ECHO_CANCELLATION };

        *ppEffectsIds = (LPGUID)CoTaskMemAlloc(sizeof(effectsList));
        if (!*ppEffectsIds)
        {
            return E_OUTOFMEMORY;
        }
        *pcEffects = ARRAYSIZE(effectsList);
        CopyMemory(*ppEffectsIds, effectsList, sizeof(effectsList));
    }

    return S_OK;
}

STDMETHODIMP CAecApoMFX::GetControllableSystemEffectsList(_Outptr_result_buffer_maybenull_(*numEffects) AUDIO_SYSTEMEFFECT** effects, _Out_ UINT* numEffects, _In_opt_ HANDLE event)
{
    UNREFERENCED_PARAMETER(event);

    RETURN_HR_IF_NULL(E_POINTER, effects);
    RETURN_HR_IF_NULL(E_POINTER, numEffects);

    *effects = nullptr;
    *numEffects = 0;

    if (m_audioSignalProcessingMode == AUDIO_SIGNALPROCESSINGMODE_COMMUNICATIONS)
    {
        // Return the list of effects implemented by this APO for COMMUNICATIONS processing mode
        static const GUID effectsList[] = {AUDIO_EFFECT_TYPE_ACOUSTIC_ECHO_CANCELLATION};

        wil::unique_cotaskmem_array_ptr<AUDIO_SYSTEMEFFECT> audioEffects(
            static_cast<AUDIO_SYSTEMEFFECT*>(CoTaskMemAlloc(ARRAYSIZE(effectsList) * sizeof(AUDIO_SYSTEMEFFECT))), ARRAYSIZE(effectsList));
        RETURN_IF_NULL_ALLOC(audioEffects.get());

        for (UINT i = 0; i < ARRAYSIZE(effectsList); i++)
        {
            audioEffects[i].id = effectsList[i];
            audioEffects[i].state = AUDIO_SYSTEMEFFECT_STATE_ON;
            audioEffects[i].canSetState = FALSE;
        }

        *numEffects = (UINT)audioEffects.size();
        *effects = audioEffects.release();
    }

    return S_OK;
}

HRESULT IsInputFormatSupportedForAec(IAudioMediaType* pMediaType, BOOL * pSupported)
{
    ASSERT_NONREALTIME();

    HRESULT hr = S_OK;
    UNCOMPRESSEDAUDIOFORMAT format;

    IF_TRUE_ACTION_JUMP((pMediaType == nullptr || pSupported == nullptr), hr = E_INVALIDARG, exit);
    hr = pMediaType->GetUncompressedAudioFormat(&format);
    IF_FAILED_JUMP(hr, exit);

    *pSupported = format.dwBytesPerSampleContainer == 4 &&
                  format.dwValidBitsPerSample == 32 &&
                  format.fFramesPerSecond == SUPPORTED_AEC_SAMPLINGRATE && // We only support one sampling rate at the input
                  format.dwSamplesPerFrame <= 16 && // We only support <= 16 channels at the input
                  format.guidFormatType == KSDATAFORMAT_SUBTYPE_IEEE_FLOAT;

exit:
    return hr;
}

HRESULT IsOutputFormatSupportedForAec(IAudioMediaType* pMediaType, BOOL * pSupported)
{
    ASSERT_NONREALTIME();

    HRESULT hr = S_OK;
    UNCOMPRESSEDAUDIOFORMAT format;

    IF_TRUE_ACTION_JUMP((pMediaType == nullptr || pSupported == nullptr), hr = E_INVALIDARG, exit);
    hr = pMediaType->GetUncompressedAudioFormat(&format);
    IF_FAILED_JUMP(hr, exit);

    *pSupported = format.dwBytesPerSampleContainer == 4 &&
                  format.dwValidBitsPerSample == 32 &&
                  format.fFramesPerSecond == SUPPORTED_AEC_SAMPLINGRATE &&
                  format.dwSamplesPerFrame == 1 && // We only mono output
                  format.guidFormatType == KSDATAFORMAT_SUBTYPE_IEEE_FLOAT;

exit:
    return hr;
}

HRESULT
CreatePreferredInputMediaType(IAudioMediaType** ppMediaType, UINT32 requestedInputChannelCount)
{
    ASSERT_NONREALTIME();

    // Default to mono format
    // We will adjust the channel count based on the requested format later on
    UNCOMPRESSEDAUDIOFORMAT format =
    {
        KSDATAFORMAT_SUBTYPE_IEEE_FLOAT,
        1, 4, 32, SUPPORTED_AEC_SAMPLINGRATE, KSAUDIO_SPEAKER_DIRECTOUT
    };

    // Match the channel count of the input if it is less than 16
    if (requestedInputChannelCount <= 16)
    {
        format.dwSamplesPerFrame = requestedInputChannelCount;
        format.dwChannelMask = KSAUDIO_SPEAKER_DIRECTOUT;
    }
    
    return CreateAudioMediaTypeFromUncompressedAudioFormat(&format, ppMediaType);
}

HRESULT
CreatePreferredOutputMediaType(IAudioMediaType** ppMediaType)
{
    ASSERT_NONREALTIME();

    // Output is mono @ the same sampling rate as the input
    UNCOMPRESSEDAUDIOFORMAT format =
    {
        KSDATAFORMAT_SUBTYPE_IEEE_FLOAT,
        1, 4, 32, SUPPORTED_AEC_SAMPLINGRATE, KSAUDIO_SPEAKER_DIRECTOUT
    };

    return CreateAudioMediaTypeFromUncompressedAudioFormat(&format, ppMediaType);
}

//-------------------------------------------------------------------------
// Description:
//
//
//
// Parameters:
//
//
//
// Return values:
//
//
//
// Remarks:
//
//
STDMETHODIMP CAecApoMFX::IsInputFormatSupported(IAudioMediaType *pOutputFormat, IAudioMediaType *pRequestedInputFormat, IAudioMediaType **ppSupportedInputFormat)
{
    ASSERT_NONREALTIME();
    HRESULT hResult;

    IF_TRUE_ACTION_JUMP((NULL == pRequestedInputFormat) || (NULL == ppSupportedInputFormat), hResult = E_POINTER, Exit);
    *ppSupportedInputFormat = NULL;

    // This method here is called in the context of the MIC endpoint
    // There are 2 supported scenarios
    // - The AEC APO can handle any mic format
    // - The AEC APO can support exactly 1 input format
    //
    // For the purposes of this sample AEC APO, we pretend that the AEC APO supports exactly 1 format 
    // - a sampling rate of SUPPORTED_AEC_SAMPLINGRATE. The APO can accept upto 16 channels at the input
    //   and will output mono audio.
    //

    if (pOutputFormat)
    {
        // Is this a valid format that we support at the output?
        BOOL bSupportedOut = FALSE;
        hResult = IsOutputFormatSupportedForAec(pOutputFormat, &bSupportedOut);
        IF_FAILED_JUMP(hResult, Exit);
        if (!bSupportedOut)
        {
            return APOERR_FORMAT_NOT_SUPPORTED;
        }
    } 
    
    BOOL bSupported = FALSE;
    hResult = IsInputFormatSupportedForAec(pRequestedInputFormat, &bSupported);
    IF_FAILED_JUMP(hResult, Exit);

    if (!bSupported)
    {
        hResult = CreatePreferredInputMediaType(ppSupportedInputFormat, pRequestedInputFormat->GetAudioFormat()->nChannels);
        IF_FAILED_JUMP(hResult, Exit);
        return S_FALSE;
    }
   
    pRequestedInputFormat->AddRef();
    *ppSupportedInputFormat = pRequestedInputFormat;

Exit:

    return hResult;
}

//-------------------------------------------------------------------------
// Description:
//
//
//
// Parameters:
//
//
//
// Return values:
//
//
//
// Remarks:
//
//
STDMETHODIMP CAecApoMFX::IsOutputFormatSupported(IAudioMediaType *pInputFormat, IAudioMediaType *pRequestedOutputFormat, IAudioMediaType **ppSupportedOutputFormat)
{
    ASSERT_NONREALTIME();
    HRESULT hResult;

    IF_TRUE_ACTION_JUMP((NULL == pRequestedOutputFormat) || (NULL == ppSupportedOutputFormat), hResult = E_POINTER, Exit);
    *ppSupportedOutputFormat = NULL;

    if (pInputFormat != nullptr)
    {
        BOOL bSupportedIn = FALSE;
        hResult = IsInputFormatSupportedForAec(pInputFormat, &bSupportedIn);
        IF_FAILED_JUMP(hResult, Exit);
        if (!bSupportedIn)
        {
            return APOERR_FORMAT_NOT_SUPPORTED;
        }
    }

    BOOL bSupported = FALSE;
    hResult = IsOutputFormatSupportedForAec(pRequestedOutputFormat, &bSupported);
    IF_FAILED_JUMP(hResult, Exit);

    if (!bSupported)
    {
        hResult = CreatePreferredOutputMediaType(ppSupportedOutputFormat);
        IF_FAILED_JUMP(hResult, Exit);
        return S_FALSE;
    }

    pRequestedOutputFormat->AddRef();
    *ppSupportedOutputFormat = pRequestedOutputFormat;

Exit:

    return hResult;
}

STDMETHODIMP
CAecApoMFX::AddAuxiliaryInput(
    DWORD dwInputId,
    UINT32 cbDataSize,
    BYTE *pbyData,
    APO_CONNECTION_DESCRIPTOR * pInputConnection
)
{
    HRESULT hResult = S_OK;

    CComPtr<IAudioMediaType> spSupportedType;
    ASSERT_NONREALTIME();

    IF_TRUE_ACTION_JUMP(m_bIsLocked, hResult = APOERR_APO_LOCKED, Exit);
    IF_TRUE_ACTION_JUMP(!m_bIsInitialized, hResult = APOERR_NOT_INITIALIZED, Exit);

    BOOL bSupported = FALSE;
    hResult = IsInputFormatSupportedForAec(pInputConnection->pFormat, &bSupported);
    IF_FAILED_JUMP(hResult, Exit);
    IF_TRUE_ACTION_JUMP(!bSupported, hResult = APOERR_FORMAT_NOT_SUPPORTED, Exit);

    // This APO can only handle 1 auxiliary input
    IF_TRUE_ACTION_JUMP(m_auxiliaryInputId != 0, hResult = APOERR_NUM_CONNECTIONS_INVALID, Exit);

    m_auxiliaryInputId = dwInputId;

    IF_TRUE_ACTION_JUMP( ((NULL == pbyData) && (0 != cbDataSize)), hResult = E_INVALIDARG, Exit);
    IF_TRUE_ACTION_JUMP( ((NULL != pbyData) && (0 == cbDataSize)), hResult = E_INVALIDARG, Exit);
    if (cbDataSize == sizeof(APOInitSystemEffects3))
    {
        /*
        //
        // pbyData contains APOInitSystemEffects3 structure describing the loopback endpoint
        //
        APOInitSystemEffects3* papoSysFxInit3 = (APOInitSystemEffects3*)pbyData;

        // Register for notification in GetApoNotificationRegistrationInfo

        // Keep a reference to the loopback device that will be registering for endpoint volume notifcations.

        IF_TRUE_ACTION_JUMP(papoSysFxInit3->pDeviceCollection == nullptr, hResult = E_INVALIDARG, Exit);
        UINT32 numDevices;
        hResult = papoSysFxInit3->pDeviceCollection->GetCount(&numDevices);
        IF_FAILED_JUMP(hResult, Exit);
        IF_TRUE_ACTION_JUMP(numDevices <= 0, hResult = E_INVALIDARG, Exit);

        hResult = papoSysFxInit3->pDeviceCollection->Item(numDevices - 1, &m_spLoopbackDevice);
        IF_FAILED_JUMP(hResult, Exit);
        */
    }
    else
    {
        //
        // pbyData contains APOInitSystemEffects2 structure describing the render endpoint
        //
    }

    // Signal to AEC algorithm that there is a reference audio stream

Exit:
    return hResult;
}

STDMETHODIMP
CAecApoMFX::RemoveAuxiliaryInput(DWORD dwInputId)
{
    HRESULT hResult = S_OK;
    ASSERT_NONREALTIME();

    IF_TRUE_ACTION_JUMP(m_bIsLocked, hResult = APOERR_APO_LOCKED, Exit);
    IF_TRUE_ACTION_JUMP(!m_bIsInitialized, hResult = APOERR_NOT_INITIALIZED, Exit);

    // This APO can only handle 1 auxiliary input
    IF_TRUE_ACTION_JUMP(m_auxiliaryInputId != dwInputId, hResult = APOERR_INVALID_INPUTID, Exit);

    m_auxiliaryInputId = 0;

    // Signal to AEC algorithm that there is no longer any reference audio stream

Exit:
    return hResult;
}

STDMETHODIMP
CAecApoMFX::IsInputFormatSupported(IAudioMediaType* pRequestedInputFormat,
                                   IAudioMediaType** ppSupportedInputFormat)
{
    ASSERT_NONREALTIME();
    HRESULT hResult = S_OK;

    IF_TRUE_ACTION_JUMP((NULL == pRequestedInputFormat) || (NULL == ppSupportedInputFormat), hResult = E_POINTER, Exit);

    BOOL bSupported = FALSE;
    hResult = IsInputFormatSupportedForAec(pRequestedInputFormat, &bSupported);
    IF_FAILED_JUMP(hResult, Exit);

    if (!bSupported)
    {
        hResult = CreatePreferredInputMediaType(ppSupportedInputFormat, pRequestedInputFormat->GetAudioFormat()->nChannels);
        IF_FAILED_JUMP(hResult, Exit);
        return S_FALSE;
    }

    pRequestedInputFormat->AddRef();
    *ppSupportedInputFormat = pRequestedInputFormat;

Exit:
    return hResult;
}

// IAPOAuxiliaryInputRT
STDMETHODIMP_(void)
CAecApoMFX::AcceptInput(DWORD dwInputId,
                        const APO_CONNECTION_PROPERTY * pInputConnection)
{
    ASSERT_REALTIME();
    ATLASSERT(m_bIsInitialized);
    ATLASSERT(m_bIsLocked);

    ATLASSERT(pInputConnection->u32Signature == APO_CONNECTION_PROPERTY_V2_SIGNATURE);
    ATLASSERT(inputId == m_auxiliaryInputId);
    UNREFERENCED_PARAMETER(dwInputId);

    const APO_CONNECTION_PROPERTY_V2* connectionV2 = reinterpret_cast<const APO_CONNECTION_PROPERTY_V2*>(pInputConnection);

    // Check connectionV2->property.u32BufferFlags to see whether loopback buffer is silent
    // Provide loopback buffer and timestamp to AEC algorithm
    UNREFERENCED_PARAMETER(connectionV2);
}

STDMETHODIMP CAecApoMFX::GetApoNotificationRegistrationInfo(_Out_writes_(*count) APO_NOTIFICATION_DESCRIPTOR** apoNotifications, _Out_ DWORD* count)
{
    *apoNotifications = nullptr;
    *count = 0;

    /*
    RETURN_HR_IF_NULL(E_FAIL, m_spCaptureDevice);
    RETURN_HR_IF_NULL(E_FAIL, m_spLoopbackDevice);

    // Let the OS know what notifications we are interested in by returning an array of
    // APO_NOTIFICATION_DESCRIPTORs.
    constexpr DWORD numDescriptors = 2;
    wil::unique_cotaskmem_ptr<APO_NOTIFICATION_DESCRIPTOR[]> apoNotificationDescriptors;

    apoNotificationDescriptors.reset(
        static_cast<APO_NOTIFICATION_DESCRIPTOR*>(CoTaskMemAlloc(sizeof(APO_NOTIFICATION_DESCRIPTOR) * numDescriptors)));
    RETURN_IF_NULL_ALLOC(apoNotificationDescriptors);

    // Our APO wants to get notified when a endpoint volume changes on the capture endpoint.
    apoNotificationDescriptors[0].type = APO_NOTIFICATION_TYPE_ENDPOINT_VOLUME;
    (void)m_spCaptureDevice->QueryInterface(&apoNotificationDescriptors[0].audioEndpointVolume.device);

    // Our APO wants to get notified when a endpoint volume changes on the auxiliary input endpoint.
    apoNotificationDescriptors[1].type = APO_NOTIFICATION_TYPE_ENDPOINT_VOLUME;
    (void)m_spLoopbackDevice->QueryInterface(&apoNotificationDescriptors[1].audioEndpointVolume.device);

    *apoNotifications = apoNotificationDescriptors.release();
    *count = numDescriptors;
    */

    return S_OK;
}

static bool IsSameEndpointId(IMMDevice* device1, IMMDevice* device2)
{
    bool isSameEndpointId = false;

    wil::unique_cotaskmem_string deviceId1;
    if (SUCCEEDED(device1->GetId(&deviceId1)))
    {
        wil::unique_cotaskmem_string deviceId2;
        if (SUCCEEDED(device2->GetId(&deviceId2)))
        {
            isSameEndpointId = (CompareStringOrdinal(deviceId1.get(), -1, deviceId2.get(), -1, TRUE) == CSTR_EQUAL);
        }
    }
    return isSameEndpointId;
}

// HandleNotification is called whenever there is a change that matches any of the
// APO_NOTIFICATION_DESCRIPTOR elements in the array that was returned by GetApoNotificationRegistrationInfo.
// Note that the APO will have to query each property once to get its initial value because this method is
// only invoked when any of the properties have changed.
STDMETHODIMP_(void) CAecApoMFX::HandleNotification(_In_ APO_NOTIFICATION* /* apoNotification */)
{
    // Handle endpoint volume change
    /* 
    if (apoNotification->type == APO_NOTIFICATION_TYPE_ENDPOINT_PROPERTY_CHANGE)
    {
        if (IsSameEndpointId(apoNotification->audioEndpointVolumeChange.endpoint, m_spCaptureDevice))
        {            
            m_captureEndpointMasterVolume = apoNotification->audioEndpointVolumeChange.volume->fMasterVolume;
        }
        else if (IsSameEndpointId(apoNotification->audioEndpointVolumeChange.endpoint, m_spLoopbackDevice))
        {
            m_loopbackEndpointMasterVolume = apoNotification->audioEndpointVolumeChange.volume->fMasterVolume;
        }
    }
    else if (apoNotification->type == APO_NOTIFICATION_TYPE_ENDPOINT_PROPERTY_CHANGE)
    {

    }
    else if(apoNotification->type == APO_NOTIFICATION_TYPE_SYSTEM_EFFECTS_PROPERTY_CHANGE)
    {

    } 
    */
}

