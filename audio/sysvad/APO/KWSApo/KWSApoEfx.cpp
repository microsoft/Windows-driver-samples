//
// KWSApoEFX.cpp -- Copyright (c) Microsoft Corporation. All rights reserved.
//
// Description:
//
//  Implementation of CKWSApoEFX
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

#include "KWSApo.h"
#include <devicetopology.h>
#include <CustomPropKeys.h>
#include <wil\result.h>

// Static declaration of the APO_REG_PROPERTIES structure
// associated with this APO.  The number in <> brackets is the
// number of IIDs supported by this APO.  If more than one, then additional
// IIDs are added at the end
#pragma warning (disable : 4815)
const AVRT_DATA CRegAPOProperties<1> CKWSApoEFX::sm_RegProperties(
    __uuidof(KWSApoEFX),                           // clsid of this APO
    L"CKWSApoEFX",                                 // friendly name of this APO
    L"Copyright (c) Microsoft Corporation",         // copyright info
    1,                                              // major version #
    0,                                              // minor version #
    __uuidof(IKWSApoEFX),                          // iid of primary interface
    (APO_FLAG) (APO_FLAG_BITSPERSAMPLE_MUST_MATCH | APO_FLAG_FRAMESPERSECOND_MUST_MATCH),
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
STDMETHODIMP_(void) CKWSApoEFX::APOProcess(
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

            if (BUFFER_SILENT == ppInputConnections[0]->u32BufferFlags)
            {
                WriteSilence( pf32OutputFrames,
                              ppInputConnections[0]->u32ValidFrameCount,
                              GetSamplesPerFrame() );
            }
            else
            {
                ProcessBuffer(pf32OutputFrames, pf32InputFrames,
                             ppInputConnections[0]->u32ValidFrameCount,
                             &m_FormatInfo);

                // we don't try to remember silence
                ppOutputConnections[0]->u32BufferFlags = BUFFER_VALID;
            }

            // Set the valid frame count.
            ppOutputConnections[0]->u32ValidFrameCount = ppInputConnections[0]->u32ValidFrameCount;

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
STDMETHODIMP CKWSApoEFX::GetLatency(HNSTIME* pTime)  
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
STDMETHODIMP CKWSApoEFX::LockForProcess(UINT32 u32NumInputConnections,
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

HRESULT CKWSApoEFX::Initialize(UINT32 cbDataSize, BYTE* pbyData)
{
    HRESULT                     hr = S_OK;
    CComPtr<IMMDevice>	        spMyDevice;
    CComPtr<IDeviceTopology>    spMyDeviceTopology;
    CComPtr<IConnector>         spMyConnector;
    CComPtr<IPart>              spPart;
    UINT                        myPartId;
    CComPtr<IKsControl>         spKsControl;
    ULONG                       cbReturned = 0;
    UINT                        nSoftwareIoConnectorIndex;

    IF_TRUE_ACTION_JUMP( ((NULL == pbyData) && (0 != cbDataSize)), hr = E_INVALIDARG, Exit);
    IF_TRUE_ACTION_JUMP( ((NULL != pbyData) && (0 == cbDataSize)), hr = E_INVALIDARG, Exit);

    if (cbDataSize == sizeof(APOInitSystemEffects3))
    {
        //
        // Initialize for mode-specific signal processing
        //
        APOInitSystemEffects3* papoSysFxInit3 = (APOInitSystemEffects3*)pbyData;

        nSoftwareIoConnectorIndex = papoSysFxInit3->nSoftwareIoConnectorIndex;
             
        // Windows should pass a valid collection.
        ATLASSERT(papoSysFxInit3->pDeviceCollection != nullptr);
        IF_TRUE_ACTION_JUMP(papoSysFxInit3->pDeviceCollection == nullptr, hr = E_INVALIDARG, Exit);

        // Get the IDeviceTopology and IConnector interfaces to communicate with this
        // APO's counterpart audio driver. This can be used for any proprietary
        // communication.
        hr = papoSysFxInit3->pDeviceCollection->Item(papoSysFxInit3->nSoftwareIoDeviceInCollection, &spMyDevice);
        IF_FAILED_JUMP(hr, Exit);       
        
        // Try to get the logging service, but ignore errors as failure to do logging it is not fatal.
        if(SUCCEEDED(papoSysFxInit3->pServiceProvider->QueryService(SID_AudioProcessingObjectLoggingService, IID_PPV_ARGS(&m_apoLoggingService))))
        {
            m_apoLoggingService->ApoLog(APO_LOG_LEVEL_INFO, L"CKWSApoEFX::Initialize called with APOInitSystemEffects3.");
        }        
    }
    else if (cbDataSize == sizeof(APOInitSystemEffects2))
    {
        //
        // Initialize for mode-specific signal processing
        //
        APOInitSystemEffects2* papoSysFxInit2 = (APOInitSystemEffects2*)pbyData;

        nSoftwareIoConnectorIndex = papoSysFxInit2->nSoftwareIoConnectorIndex;
       
        // Save reference to the effects property store. This saves effects settings
        // and is the communication medium between this APO and any associated UI.
        m_spAPOSystemEffectsProperties = papoSysFxInit2->pAPOSystemEffectsProperties;

        // Windows should pass a valid collection.
        ATLASSERT(papoSysFxInit2->pDeviceCollection != nullptr);
        IF_TRUE_ACTION_JUMP(papoSysFxInit2->pDeviceCollection == nullptr, hr = E_INVALIDARG, Exit);

        // Get the IDeviceTopology and IConnector interfaces to communicate with this
        // APO's counterpart audio driver. This can be used for any proprietary
        // communication.
        hr = papoSysFxInit2->pDeviceCollection->Item(papoSysFxInit2->nSoftwareIoDeviceInCollection, &spMyDevice);
        IF_FAILED_JUMP(hr, Exit);

    }
    else
    {
        // Invalid initialization size
        hr = E_INVALIDARG;
        goto Exit;
    }

    hr = spMyDevice->Activate(__uuidof(IKsControl), CLSCTX_ALL, NULL, (void**)&spKsControl);
    IF_FAILED_JUMP(hr, Exit);

    hr = spMyDevice->Activate(__uuidof(IDeviceTopology), CLSCTX_ALL, NULL, (void**)&spMyDeviceTopology);
    IF_FAILED_JUMP(hr, Exit);

    hr = spMyDeviceTopology->GetConnector(nSoftwareIoConnectorIndex, &spMyConnector);
    IF_FAILED_JUMP(hr, Exit);

    spPart = spMyConnector;

    hr = spPart->GetLocalId(&myPartId);
    IF_FAILED_JUMP(hr, Exit);

    KSP_PIN ksPinProperty;
    ::ZeroMemory(&ksPinProperty, sizeof(ksPinProperty));
    ksPinProperty.Property.Set = KSPROPSETID_InterleavedAudio;
    ksPinProperty.Property.Id = KSPROPERTY_INTERLEAVEDAUDIO_FORMATINFORMATION;
    ksPinProperty.Property.Flags = KSPROPERTY_TYPE_GET;
    ksPinProperty.PinId = myPartId & 0x0000ffff;

    ::ZeroMemory(&m_FormatInfo, sizeof(m_FormatInfo));

    hr = spKsControl->KsProperty(&(ksPinProperty.Property), sizeof(ksPinProperty), &m_FormatInfo, sizeof(m_FormatInfo), &cbReturned);
    IF_FAILED_JUMP(hr, Exit);

    IF_TRUE_ACTION_JUMP(m_FormatInfo.Size != sizeof(m_FormatInfo), hr = E_INVALIDARG, Exit);

    if (cbDataSize == sizeof(APOInitSystemEffects3))
    {
        // Register for notification of registry updates in GetApoNotificationRegistrationInfo
    }
    else if (cbDataSize == sizeof(APOInitSystemEffects2))
    {
        //
        //  Register for notification of registry updates
        //
        hr = m_spEnumerator.CoCreateInstance(__uuidof(MMDeviceEnumerator));
        IF_FAILED_JUMP(hr, Exit);

        hr = m_spEnumerator->RegisterEndpointNotificationCallback(this);
        IF_FAILED_JUMP(hr, Exit);

        m_bRegisteredEndpointNotificationCallback = TRUE;
    }

    m_bIsInitialized = true;

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
STDMETHODIMP CKWSApoEFX::GetEffectsList(_Outptr_result_buffer_maybenull_(*pcEffects) LPGUID *ppEffectsIds, _Out_ UINT *pcEffects, _In_ HANDLE)
{
    RETURN_HR_IF(E_POINTER, ppEffectsIds == nullptr);
    RETURN_HR_IF(E_POINTER, pcEffects == nullptr);

    *ppEffectsIds = nullptr;
    *pcEffects = 0;

    // Return the list of effects implemented by this APO
    static const GUID effectsList[] = {AUDIO_EFFECT_TYPE_ACOUSTIC_ECHO_CANCELLATION};
    *ppEffectsIds = (LPGUID)CoTaskMemAlloc(sizeof(effectsList));
    RETURN_IF_NULL_ALLOC(*ppEffectsIds);
    *pcEffects = ARRAYSIZE(effectsList);
    CopyMemory(*ppEffectsIds, effectsList, sizeof(effectsList));

    return S_OK;
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
STDMETHODIMP CKWSApoEFX::IsInputFormatSupported(IAudioMediaType *pOutputFormat, IAudioMediaType *pRequestedInputFormat, IAudioMediaType **ppSupportedInputFormat)
{
    ASSERT_NONREALTIME();
    bool formatChanged = false;
    HRESULT hResult;
    UNCOMPRESSEDAUDIOFORMAT uncompInputFormat;
    IAudioMediaType *recommendedFormat = NULL;
    UINT totalChannelCount = (m_FormatInfo.PrimaryChannelCount + m_FormatInfo.InterleavedChannelCount);

    IF_TRUE_ACTION_JUMP((NULL == pRequestedInputFormat) || (NULL == ppSupportedInputFormat), hResult = E_POINTER, Exit);
    *ppSupportedInputFormat = NULL;

    // Initial comparison to make sure the requested format is valid and consistent with the output
    // format. Because of the APO flags specified during creation, the samples per frame value will
    // not be validated.
    hResult = IsFormatTypeSupported( pOutputFormat, pRequestedInputFormat, &recommendedFormat, true );
    IF_FAILED_JUMP(hResult,  Exit);

    // If the input format is changed, make sure we track it for our return code.
    if (S_FALSE == hResult)
    {
        formatChanged = true;
    }

    // now retrieve the format that IsFormatTypeSupported decided on, building upon that by adding
    // our channel count constraint.
    hResult = recommendedFormat->GetUncompressedAudioFormat(&uncompInputFormat);
    IF_FAILED_JUMP(hResult, Exit);

    // the expected input channel count, for interleaved audio, is the total number of channels
    // reported in the interleaved format information. Fail any request for a format that doesn't
    // meet that requirement.
    if (uncompInputFormat.dwSamplesPerFrame != totalChannelCount)
    {
        hResult = APOERR_FORMAT_NOT_SUPPORTED;
        goto Exit;
    }

    // If the requested format exactly matched our requirements,
    // just return it.
    if(!formatChanged)
    {
        *ppSupportedInputFormat = pRequestedInputFormat;
        (*ppSupportedInputFormat)->AddRef();
        hResult = S_OK;
    }
    else  // we're proposing something different than the input, copy it and return S_FALSE;
    {
        hResult = CreateAudioMediaTypeFromUncompressedAudioFormat(&uncompInputFormat, ppSupportedInputFormat);
        IF_FAILED_JUMP(hResult, Exit);

        hResult = S_FALSE;
    }

Exit:

    if (recommendedFormat)
    {
        recommendedFormat->Release();
    }

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
STDMETHODIMP CKWSApoEFX::IsOutputFormatSupported(IAudioMediaType *pInputFormat, IAudioMediaType *pRequestedOutputFormat, IAudioMediaType **ppSupportedOutputFormat)
{
    ASSERT_NONREALTIME();
    bool formatChanged = false;
    HRESULT hResult;
    UNCOMPRESSEDAUDIOFORMAT uncompOutputFormat;
    IAudioMediaType *recommendedFormat = NULL;

    IF_TRUE_ACTION_JUMP((NULL == pRequestedOutputFormat) || (NULL == ppSupportedOutputFormat), hResult = E_POINTER, Exit);
    *ppSupportedOutputFormat = NULL;

    // Initial comparison to make sure the requested format is valid and consistent with the input
    // format. Because of the APO flags specified during creation, the samples per frame value will
    // not be validated.
    hResult = IsFormatTypeSupported( pInputFormat, pRequestedOutputFormat, &recommendedFormat, true );
    IF_FAILED_JUMP(hResult,  Exit);

    // If the output format is changed, make sure we track it for our return code.
    if (S_FALSE == hResult)
    {
        formatChanged = true;
    }

    // now retrieve the format that IsFormatTypeSupported decided on, building upon that by adding
    // our channel count constraint.
    hResult = recommendedFormat->GetUncompressedAudioFormat(&uncompOutputFormat);
    IF_FAILED_JUMP(hResult, Exit);

    // The expected output channel count is the number of primary channels in the interleaved data.
    // We're removing the interleaved data.
    if (uncompOutputFormat.dwSamplesPerFrame != m_FormatInfo.PrimaryChannelCount)
    {
        uncompOutputFormat.dwSamplesPerFrame = m_FormatInfo.PrimaryChannelCount;
        uncompOutputFormat.dwChannelMask = m_FormatInfo.PrimaryChannelMask;
        formatChanged = true;
    }

    // If the requested format exactly matched our requirements,
    // just return it.
    if(!formatChanged)
    {
        *ppSupportedOutputFormat = pRequestedOutputFormat;
        (*ppSupportedOutputFormat)->AddRef();
        hResult = S_OK;
    }
    else  // we're proposing something different, copy it and return S_FALSE;
    {
        hResult = CreateAudioMediaTypeFromUncompressedAudioFormat(&uncompOutputFormat, ppSupportedOutputFormat);
        IF_FAILED_JUMP(hResult, Exit);
        hResult = S_FALSE;
    }

Exit:

    if (recommendedFormat)
    {
        recommendedFormat->Release();
    }

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
STDMETHODIMP CKWSApoEFX::GetInputChannelCount(UINT32 *pu32ChannelCount)
{
    ASSERT_NONREALTIME();
    HRESULT hResult = S_OK;

    IF_TRUE_ACTION_JUMP(!m_bIsInitialized, hResult = APOERR_NOT_INITIALIZED, Exit);
    IF_TRUE_ACTION_JUMP(NULL == pu32ChannelCount, hResult = E_POINTER, Exit);

    // the input channel count is always the sum of the primary and interleaved
    *pu32ChannelCount = (m_FormatInfo.PrimaryChannelCount + m_FormatInfo.InterleavedChannelCount);

Exit:
    return hResult;
} // GetChannelCount

//-------------------------------------------------------------------------
// Description:
//
//  Destructor.
//
// Parameters:
//
//     void
//
// Return values:
//
//      void
//
// Remarks:
//
//      This method deletes whatever was allocated.
//
//      This method may not be called from a real-time processing thread.
//
CKWSApoEFX::~CKWSApoEFX(void)
{
    //
    // unregister for callbacks
    //
    if (m_bRegisteredEndpointNotificationCallback)
    {
        m_spEnumerator->UnregisterEndpointNotificationCallback(this);
    }
} // ~CKWSApoEFX

STDMETHODIMP CKWSApoEFX::GetControllableSystemEffectsList(_Outptr_result_buffer_maybenull_(*numEffects) AUDIO_SYSTEMEFFECT** effects, _Out_ UINT* numEffects, _In_opt_ HANDLE)
{
    RETURN_HR_IF(E_POINTER, effects == nullptr);
    RETURN_HR_IF(E_POINTER, numEffects == nullptr);
 
    *effects = nullptr;
    *numEffects = 0;

    // Return the list of effects implemented by this APO
    static const AUDIO_SYSTEMEFFECT effectsList[] = {{AUDIO_EFFECT_TYPE_ACOUSTIC_ECHO_CANCELLATION, FALSE, AUDIO_SYSTEMEFFECT_STATE_ON}};
    *effects = (AUDIO_SYSTEMEFFECT*)CoTaskMemAlloc(sizeof(effectsList));
    RETURN_IF_NULL_ALLOC(*effects);
    *numEffects = ARRAYSIZE(effectsList);
    CopyMemory(*effects, effectsList, sizeof(effectsList));

    return S_OK;
}