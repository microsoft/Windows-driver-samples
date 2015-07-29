//*@@@+++@@@@******************************************************************
//
// Microsoft Windows Media Foundation
// Copyright (C) Microsoft Corporation. All rights reserved.
//
//*@@@---@@@@******************************************************************
//

#pragma once

#define GUID_STRING_LENGTH  36
#define GUID_BUFFER_SIZE    37
#define SLEEP_5MILLISEC 5
#define SLEEP_ONE_SECOND 1000 // One second in milli seconds.

#ifndef MF_WPP
#define DMFTRACE(...) 
#endif

//TP_NORMAL
//TP_LOWEST
//TP_LOW
//TP_HIGH
//TP_HIGHEST
//TP_ERROR
//TP_MUTED = 0x00070000



#define mf_assert(a) if(!a) DebugBreak()

#define WPP_CONTROL_GUIDS \
    WPP_DEFINE_CONTROL_GUID(CtlGUID_DMFTTrace, (CBCCA12E, 9472, 409D, A1B1, 753C98BF03C0), \
    WPP_DEFINE_BIT(DMFT_INIT) \
    WPP_DEFINE_BIT(DMFT_CONTROL) \
    WPP_DEFINE_BIT(DMFT_GENERAL) \
    )

#define WPP_LEVEL_FLAG_LOGGER(lvl,flags) WPP_LEVEL_LOGGER(flags)  
#define WPP_LEVEL_FLAG_ENABLED(lvl, flags) (WPP_LEVEL_ENABLED(flags) && WPP_CONTROL(WPP_BIT_ ## flags).Level >= lvl)  
#define WPP_FLAG_LEVEL_LOGGER(flags,lvl) WPP_LEVEL_LOGGER(flags)  
#define WPP_FLAG_LEVEL_ENABLED(flags, lvl) (WPP_LEVEL_ENABLED(flags) && WPP_CONTROL(WPP_BIT_ ## flags).Level >= lvl)  

#define SAFE_ADDREF(p)              if( NULL != p ) { ( p )->AddRef(); }
#define SAFE_DELETE(p)              delete p; p = NULL;
#define SAFE_SHUTDELETE(p)          if( NULL != p ) { ( p )->Shutdown(); delete p; p = NULL; }
#define SAFE_RELEASE(p)             if( NULL != p ) { ( p )->Release(); p = NULL; }
#define SAFE_SHUTRELEASE(p)         if( NULL != p ) { ( p )->Shutdown(); ( p )->Release(); p = NULL; }
#define SAFE_CLOSERELEASE(p)        if( NULL != p ) { ( p )->Close( TRUE ); ( p )->Release(); p = NULL; }
#define SAFE_COTASKMEMFREE(p)       CoTaskMemFree( p ); p = NULL;
#define SAFE_SYSFREESTRING(p)       SysFreeString( p ); p = NULL;
#define SAFE_ARRAYDELETE(p)         delete [] p; p = NULL;
#define SAFE_BYTEARRAYDELETE(p)     delete [] (BYTE*) p; p = NULL;



#define DMFTCHECKHR_GOTO(val,label) \
    hr = (val); \
if (FAILED(hr)) {\
    /*DMFTRACE(DMFT_GENERAL, TRACE_LEVEL_INFORMATION, "%!FUNC! exiting %x = %!HRESULT!", hr, hr); */\
    goto label; \
}


#define DMFTCHECKNULL_GOTO( val, label, err) \
if( (val) == NULL ) { \
    hr = (err); \
    /*DMFTRACE(DMFT_GENERAL, TRACE_LEVEL_INFORMATION, "%!FUNC! exiting %x = %!HRESULT!", hr, hr);*/\
    goto label; \
}

#define TP_SCOPE_TRACE TP_NORMAL
#define DH_THIS_FILE DH_DEVPROXY

#define SAFERELEASE(x) \
if (x) {\
    x->Release(); \
    x = NULL; \
}

#define CHK_LOG_BRK(exp) \
if (FAILED(hr = (exp)))  {\
    wprintf(L"HR=%08x File: %S Ln:  %d\n", hr, __FILE__, __LINE__); \
    /*MFWMITRACE(DH_THIS_FILE, TP_NORMAL, __FUNCTION__ " : MULTIPINERROR File: %s Ln:%d",__FILE__,__LINE__);*/\
    break; \
}


#define CHK_NULL_BRK(exp) \
if ((exp) == NULL) {\
hr = E_OUTOFMEMORY; \
wprintf(L"HR=%08x File: %S Ln:  %d\n", hr, __FILE__, __LINE__); \
break; \
}

#define CHK_NULL_PTR_BRK(exp) \
if ((exp) == NULL) {\
hr = E_INVALIDARG; \
wprintf(L"HR=%08x File: %S Ln:  %d\n", hr, __FILE__, __LINE__); \
break; \
}

#define CHK_BOOL_BRK(exp) \
if (!exp) {\
    hr = E_FAIL; \
    wprintf(L"HR=%08x File: %S Ln:  %d\n", hr, __FILE__, __LINE__); \
    break; \
}


//
// The Below type definitions are added for this iteration. It will taken out in the
// next one when we will have the windows headers modified to include the interfaces 
// externally.
//


#if !defined(_IKsControl_)
#define _IKsControl_
interface DECLSPEC_UUID("28F54685-06FD-11D2-B27A-00A0C9223196") IKsControl;
#undef INTERFACE
#define INTERFACE IKsControl
DECLARE_INTERFACE_(IKsControl, IUnknown)
{
    STDMETHOD(KsProperty)(
        THIS_
        IN PKSPROPERTY Property,
        IN ULONG PropertyLength,
        IN OUT LPVOID PropertyData,
        IN ULONG DataLength,
        OUT ULONG* BytesReturned
        ) PURE;
    STDMETHOD(KsMethod)(
        THIS_
        IN PKSMETHOD Method,
        IN ULONG MethodLength,
        IN OUT LPVOID MethodData,
        IN ULONG DataLength,
        OUT ULONG* BytesReturned
        ) PURE;
    STDMETHOD(KsEvent)(
        THIS_
        IN PKSEVENT Event OPTIONAL,
        IN ULONG EventLength,
        IN OUT LPVOID EventData,
        IN ULONG DataLength,
        OUT ULONG* BytesReturned
        ) PURE;
};
#endif //!defined(_IKsControl_)



#if !defined(__IMFDeviceTransform_INTERFACE_DEFINED__)
//
//These definitions will have to be taken out when the
//internal defines are made public
//

EXTERN_GUID(MEDeviceStreamCreated, 0x0252a1cf, 0x3540, 0x43b4, 0x91, 0x64, 0xd7, 0x2e, 0xb4, 0x05, 0xfa, 0x40);

typedef
enum _DeviceStreamState
{
    DeviceStreamState_Stop = 0,
    DeviceStreamState_Pause = (DeviceStreamState_Stop + 1),
    DeviceStreamState_Run = (DeviceStreamState_Pause + 1),
    DeviceStreamState_Disabled = (DeviceStreamState_Run + 1)
} 	DeviceStreamState;

/* size is 4, align is 4*/
typedef enum _DeviceStreamState *PDeviceStreamState;

EXTERN_C const IID IID_IMFDeviceTransform;
MIDL_INTERFACE("D818FBD8-FC46-42F2-87AC-1EA2D1F9BF32")
IMFDeviceTransform : public IUnknown
{
public:
    virtual HRESULT STDMETHODCALLTYPE InitializeTransform(
        /* [annotation][in] */
        _In_  IMFAttributes *pAttributes) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetInputAvailableType(
        /* [annotation][in] */
        _In_  DWORD dwInputStreamID,
        /* [annotation][in] */
        _In_  DWORD dwTypeIndex,
        /* [annotation][out] */
        _COM_Outptr_  IMFMediaType **pMediaType) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetInputCurrentType(
        /* [annotation][in] */
        _In_  DWORD dwInputStreamID,
        /* [annotation][out] */
        _COM_Outptr_  IMFMediaType **pMediaType) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetInputStreamAttributes(
        /* [annotation][in] */
        _In_  DWORD dwInputStreamID,
        /* [annotation][out] */
        _COM_Outptr_  IMFAttributes **ppAttributes) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetOutputAvailableType(
        /* [annotation][in] */
        _In_  DWORD dwOutputStreamID,
        /* [annotation][in] */
        _In_  DWORD dwTypeIndex,
        /* [annotation][out] */
        _COM_Outptr_  IMFMediaType **pMediaType) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetOutputCurrentType(
        /* [annotation][in] */
        _In_  DWORD dwOutputStreamID,
        /* [annotation][out] */
        _COM_Outptr_  IMFMediaType **pMediaType) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetOutputStreamAttributes(
        /* [annotation][in] */
        _In_  DWORD dwOutputStreamID,
        /* [annotation][out] */
        _COM_Outptr_  IMFAttributes **ppAttributes) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetStreamCount(
        /* [annotation][out] */
        _Out_  DWORD *pcInputStreams,
        /* [annotation][out] */
        _Out_  DWORD *pcOutputStreams) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetStreamIDs(
        /* [annotation][in] */
        _In_  DWORD dwInputIDArraySize,
        /* [annotation][out] */
        _Out_  DWORD *pdwInputStreamIds,
        /* [annotation][in] */
        _In_  DWORD dwOutputIDArraySize,
        /* [annotation][out] */
        _Out_  DWORD *pdwOutputStreamIds) = 0;

    virtual HRESULT STDMETHODCALLTYPE ProcessEvent(
        /* [annotation][in] */
        _In_  DWORD dwInputStreamID,
        /* [annotation][in] */
        _In_  IMFMediaEvent *pEvent) = 0;

    virtual HRESULT STDMETHODCALLTYPE ProcessInput(
        /* [annotation][in] */
        _In_  DWORD dwInputStreamID,
        /* [annotation][in] */
        _In_  IMFSample *pSample,
        /* [annotation][in] */
        _In_  DWORD dwFlags) = 0;

    virtual HRESULT STDMETHODCALLTYPE ProcessMessage(
        /* [annotation][in] */
        _In_  MFT_MESSAGE_TYPE eMessage,
        /* [annotation][in] */
        _In_  ULONG_PTR ulParam) = 0;

    virtual HRESULT STDMETHODCALLTYPE ProcessOutput(
        /* [annotation][in] */
        _In_  DWORD dwFlags,
        /* [annotation][in] */
        _In_  DWORD cOutputBufferCount,
        /* [size_is][annotation][out][in] */
        _Inout_  MFT_OUTPUT_DATA_BUFFER *pOutputSample,
        /* [annotation][out] */
        _Out_  DWORD *pdwStatus) = 0;

    virtual HRESULT STDMETHODCALLTYPE SetInputStreamState(
        /* [annotation][in] */
        _In_  DWORD dwStreamID,
        /* [annotation][in] */
        _In_  IMFMediaType *pMediaType,
        /* [annotation][in] */
        _In_  DeviceStreamState value,
        /* [annotation][in] */
        _In_  DWORD dwFlags) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetInputStreamState(
        /* [annotation][in] */
        _In_  DWORD dwStreamID,
        /* [annotation][out] */
        _Out_  DeviceStreamState *value) = 0;

    virtual HRESULT STDMETHODCALLTYPE SetOutputStreamState(
        /* [annotation][in] */
        _In_  DWORD dwStreamID,
        /* [annotation][in] */
        _In_  IMFMediaType *pMediaType,
        /* [annotation][in] */
        _In_  DeviceStreamState value,
        /* [annotation][in] */
        _In_  DWORD dwFlags) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetOutputStreamState(
        /* [annotation][in] */
        _In_  DWORD dwStreamID,
        /* [annotation][out] */
        _Out_  DeviceStreamState *value) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetInputStreamPreferredState(
        /* [annotation][in] */
        _In_  DWORD dwStreamID,
        /* [annotation][out] */
        _Out_  DeviceStreamState *value,
        /* [annotation][out] */
        _COM_Outptr_  IMFMediaType **ppMediaType) = 0;

    virtual HRESULT STDMETHODCALLTYPE FlushInputStream(
        /* [annotation][in] */
        _In_  DWORD dwStreamIndex,
        /* [annotation][in] */
        _In_  DWORD dwFlags) = 0;

    virtual HRESULT STDMETHODCALLTYPE FlushOutputStream(
        /* [annotation][in] */
        _In_  DWORD dwStreamIndex,
        /* [annotation][in] */
        _In_  DWORD dwFlags) = 0;

};

#endif 


interface IDirect3DDeviceManager9;
//Forward defintion
class CBasePin;
//////////////////////////////////////////////////////////////////////////
//  CCritSec
//  Description: Wraps a critical section.
//////////////////////////////////////////////////////////////////////////

class CCritSec
{
private:
    CRITICAL_SECTION m_criticalSection;
public:
    CCritSec();
    ~CCritSec();
    _Requires_lock_not_held_(m_criticalSection) _Acquires_lock_(m_criticalSection)
    void Lock();
    _Requires_lock_held_(m_criticalSection) _Releases_lock_(m_criticalSection)
    void Unlock();
};


//////////////////////////////////////////////////////////////////////////
//  CAutoLock
//  Description: Provides automatic locking and unlocking of a 
//               of a critical section.
//////////////////////////////////////////////////////////////////////////

class CAutoLock
{
protected:
    CCritSec *m_pCriticalSection;
public:
    _Acquires_lock_(this->m_pCriticalSection->m_criticalSection)
    CAutoLock(CCritSec& crit);
    _Acquires_lock_(this->m_pCriticalSection->m_criticalSection)
    CAutoLock(CCritSec* crit);
    _Releases_lock_(this->m_pCriticalSection->m_criticalSection)
    ~CAutoLock();
};

//////////////////////////////////////////////////////////////////////////
//  CMediaTypePrinter
//  Description: Rudimentary class for printing media type!
//////////////////////////////////////////////////////////////////////////

#define MEDIAPRINTER_STARTLEN  (512)

class CMediaTypePrinter{
private:
    IMFMediaType        *pMediaType;
    PCHAR                m_pBuffer;
    ULONG                buffLen;
public:
    CMediaTypePrinter( _In_ IMFMediaType *_pMediaType );
    ~CMediaTypePrinter( );
    STDMETHODIMP_(PCHAR) ToCompleteString( );
    STDMETHODIMP_(PCHAR) ToString();
};


//
//The below guid is used to register the GUID as the Device Transform. This should be adeed to the 
//HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\DeviceClasses\ and under the 
//GLOBAL#\Device Parameters key, add a CameraPostProcessingPluginCLSID value, and set its value to
// {0E313280-3169-4F41-A329-9E854169634F} for the Pipeline to pick up the Transform. MFT0 and Device
// Transforms are exclusive and the presence of this key will lead the MFT0 to not load.
//
DEFINE_GUID(CLSID_MultiPinMFT,
    0xe313280, 0x3169, 0x4f41, 0xa3, 0x29, 0x9e, 0x85, 0x41, 0x69, 0x63, 0x4f);


typedef enum _MF_TRANSFORM_XVP_OPERATION{
    DeviceMftTransformXVPDisruptiveIn,   //Break the XVP tranform and go to the new Media Type
    DeviceMftTransformXVPDisruptiveOut,  //Keep the old transform
    DeviceMftTransformXVPCurrent,        //Don't need an XVP
    DeviceMftTransformXVPIllegal         //Either of the media types NULL, Major types don't match
}MF_TRANSFORM_XVP_OPERATION,*PMF_TRANSFORM_XVP_OPERATION;

typedef  std::vector< IMFMediaType *> IMFMediaTypeArray;
typedef  std::vector< CBasePin *>     CBasePinArray;
typedef  std::vector< IMFSample *>    IMFSampleList;
typedef  std::pair< std::multimap<int, int>::iterator, std::multimap<int, int>::iterator > MMFTMMAPITERATOR;


HRESULT CreateCodec(
    _In_opt_    IMFMediaType    * inMediaType,
    _In_opt_    IMFMediaType    *outMediaType,
    _In_        BOOL            operation /*True = Encode, False = Decode*/,
    _Out_       IMFTransform          **pTransform
    );

STDMETHODIMP   IsOptimizedPlanarVideoInputImageOutputPair(
    _In_        IMFMediaType *inMediaType,
    _In_        IMFMediaType *outMediaType,
    _Out_       bool *optimized,
    _Out_       bool *optimizedxvpneeded
    );

STDMETHODIMP CompareMediaTypesForXVP(_In_opt_ IMFMediaType *inMediaType,
    _In_        IMFMediaType                *newMediaType,
    _Inout_     MF_TRANSFORM_XVP_OPERATION  *operation
    );

STDMETHODIMP RandomnizeMediaTypes(
    _In_ IMFMediaTypeArray &pMediaTypeArray
    );

STDMETHODIMP IsInputDxSample(
    _In_ IMFSample* pSample,
    _Inout_ BOOL *isDxSample
    );

STDMETHODIMP_(BOOL) IsPinStateInActive(
    _In_ DeviceStreamState state
    );

STDMETHODIMP_(BOOL) IsKnownUncompressedVideoType(
    _In_ GUID guidSubType
    );
LPSTR DumpGUIDA(
    _In_ REFGUID guid
    );


void printMessageEvent(MFT_MESSAGE_TYPE msg);


template <typename Lambda>
HRESULT ExceptionBoundary(Lambda&& lambda)
{
    try
    {
        lambda();
        return S_OK;
    }
    catch (const _com_error& e)
    {
        return e.Error();
    }
    catch (const std::bad_alloc&)
    {
        return E_OUTOFMEMORY;
    }
    catch (const std::out_of_range&)
    {
        return MF_E_INVALIDINDEX;
    }
    catch (...)
    {
        return E_FAIL;
    }
}


//
// Custom Pin GUID. The avstream driver should define a PIN_DESCRIPTOR with this GUID
// defined in cateogory. This will lead to devproxy creating this pin. This pin however
// cannot be used by the pipeline as it may share a custom media type and pipeline could
// fail. Moreover pipeline can currently (when this sample is written) only deal with
// the known pins (PREVIEW/CAPTURE/IMAGE). This code snippet creates a custom pin 
// and 
// 
DEFINE_GUID(AVSTREAM_CUSTOM_PIN_IMAGE,
    0x888c4105, 0xb328, 0x4ed6, 0xa3, 0xca, 0x2f, 0xf4, 0xc0, 0x3a, 0x9f, 0x33);


//
// The Below redirections are to support MFT0. Implement this only if you need to load 
// MFT0 and DeviceMft simultaneously. 
// 

#define _DEFINE_DEVICEMFT_MFT0HELPER_IMPL__    \
    STDMETHOD(MFTGetStreamLimits)(             \
    _Inout_  DWORD   *pdwInputMinimum,         \
    _Inout_  DWORD   *pdwInputMaximum,         \
    _Inout_  DWORD   *pdwOutputMinimum,        \
    _Inout_  DWORD   *pdwOutputMaximum         \
    )                                          \
{                                              \
    UNREFERENCED_PARAMETER(pdwInputMinimum);   \
    UNREFERENCED_PARAMETER(pdwInputMaximum);   \
    UNREFERENCED_PARAMETER(pdwOutputMinimum);  \
    UNREFERENCED_PARAMETER(pdwOutputMaximum);  \
    return E_NOTIMPL;                          \
}                                              \
    STDMETHOD(MFTGetStreamCount)(              \
    _Inout_  DWORD*    pdwInputStreams,        \
    _Inout_  DWORD*    pdwOutputStreams        \
    )                                          \
{                                              \
    return GetStreamCount(pdwInputStreams, pdwOutputStreams);  \
}                                                              \
    STDMETHOD(MFTGetStreamIDs)(                                         \
    _In_                                    DWORD  dwInputIDArraySize,  \
    _Inout_updates_(dwInputIDArraySize)    DWORD* pdwInputIDs,          \
    _In_                                    DWORD  dwOutputIDArraySize, \
    _Inout_updates_(dwOutputIDArraySize)   DWORD* pdwOutputIDs          \
    )                                                                   \
{                                                                       \
    return GetStreamIDs(dwInputIDArraySize, \
    pdwInputIDs,                            \
    dwOutputIDArraySize,                    \
    pdwOutputIDs);                          \
}                                           \
    STDMETHOD(MFTGetInputStreamInfo)(                \
    _In_  DWORD                     dwInputStreamID, \
    _Inout_  MFT_INPUT_STREAM_INFO*    pStreamInfo   \
    )                                                \
{                                                    \
    UNREFERENCED_PARAMETER(dwInputStreamID);         \
    UNREFERENCED_PARAMETER(pStreamInfo);             \
    return E_NOTIMPL;                                \
}                                                    \
    STDMETHOD(MFTGetOutputStreamInfo)(                 \
    _In_    DWORD                   dwOutputStreamID,  \
    _Inout_    MFT_OUTPUT_STREAM_INFO* pStreamInfo     \
    )                                                  \
{                                              \
    UNREFERENCED_PARAMETER(dwOutputStreamID);  \
    UNREFERENCED_PARAMETER(pStreamInfo);       \
    return E_NOTIMPL;                          \
}                                              \
    STDMETHOD(MFTGetInputStreamAttributes)(                   \
    _In_        DWORD           dwInputStreamID,              \
    _Out_       IMFAttributes** ppAttributes                  \
    )                                                         \
{                                                             \
    return GetInputStreamAttributes(                          \
    dwInputStreamID,                                          \
    ppAttributes);                                            \
}                                                             \
    STDMETHOD(MFTGetOutputStreamAttributes)(      \
    _In_        DWORD           dwOutputStreamID, \
    _Out_       IMFAttributes** ppAttributes      \
    )                                             \
{                                                 \
    return GetOutputStreamAttributes(             \
    dwOutputStreamID, ppAttributes);              \
}                                                 \
    STDMETHOD(MFTDeleteInputStream)(  \
    _In_ DWORD dwStreamID             \
    )                                 \
{                                     \
    UNREFERENCED_PARAMETER(dwStreamID); \
    return E_NOTIMPL;                   \
}                                       \
    STDMETHOD(MFTAddInputStreams)(        \
    _In_    DWORD   cStreams,             \
    _In_    DWORD*  adwStreamIDs          \
    )                                     \
{                                         \
    UNREFERENCED_PARAMETER(cStreams);     \
    UNREFERENCED_PARAMETER(adwStreamIDs); \
    return E_NOTIMPL;                     \
}                                         \
    STDMETHOD(MFTGetInputAvailableType)(                      \
    _In_        DWORD           dwInputStreamID,              \
    _In_        DWORD           dwTypeIndex,                  \
    _Out_       IMFMediaType**  ppType                        \
    )                                                         \
{                                                             \
    return GetInputAvailableType(                             \
    dwInputStreamID, dwTypeIndex, ppType);                    \
}                                                             \
    STDMETHOD(MFTGetOutputAvailableType)(                     \
    _In_        DWORD           dwOutputStreamID,             \
    _In_        DWORD           dwTypeIndex,                  \
    _Out_       IMFMediaType**  ppMediaType                   \
    )                                             \
{                                                 \
    return GetOutputAvailableType(                \
    dwOutputStreamID, dwTypeIndex, ppMediaType);  \
}                                                 \
    STDMETHOD(MFTSetInputType)(                   \
    _In_        DWORD           dwInputStreamID,  \
    _In_    IMFMediaType*   pMediaType,           \
    _In_        DWORD           dwFlags           \
    )                                             \
{\
    UNREFERENCED_PARAMETER(dwInputStreamID); \
    UNREFERENCED_PARAMETER(pMediaType);      \
    UNREFERENCED_PARAMETER(dwFlags);         \
    return E_NOTIMPL;                        \
}                                            \
    STDMETHOD(MFTSetOutputType)(                  \
    _In_        DWORD           dwOutputStreamID, \
    _In_    IMFMediaType*   pMediaType,           \
    _In_        DWORD           dwFlags           \
    )                                             \
{                                                 \
    UNREFERENCED_PARAMETER(dwOutputStreamID);     \
    UNREFERENCED_PARAMETER(pMediaType);           \
    UNREFERENCED_PARAMETER(dwFlags);              \
    return E_NOTIMPL;                             \
}                                                 \
    STDMETHOD(MFTGetInputCurrentType)(                             \
    _In_        DWORD           dwInputStreamID,                   \
    _Out_       IMFMediaType**  ppMediaType                        \
    )                                                              \
{\
    UNREFERENCED_PARAMETER(dwInputStreamID); \
    UNREFERENCED_PARAMETER(ppMediaType);     \
    return E_NOTIMPL;                        \
}                                            \
    STDMETHOD(MFTGetOutputCurrentType)(                         \
    _In_        DWORD           dwOutputStreamID,               \
    _Out_       IMFMediaType**  ppMediaType                     \
    )                                                           \
{                                                               \
    return GetOutputCurrentType(dwOutputStreamID, ppMediaType); \
}                                                               \
    STDMETHOD(MFTGetInputStatus)(    \
    _In_    DWORD   dwInputStreamID, \
    _Inout_   DWORD*  pdwFlags       \
    )\
{\
    UNREFERENCED_PARAMETER(dwInputStreamID); \
    UNREFERENCED_PARAMETER(pdwFlags);        \
    return E_NOTIMPL;                        \
}\
    STDMETHOD(MFTGetOutputStatus)(     \
    _Inout_   DWORD *pdwFlags          \
    )                                  \
{                                      \
    UNREFERENCED_PARAMETER(pdwFlags);  \
    return E_NOTIMPL;                  \
}                                      \
    STDMETHOD(MFTSetOutputBounds)(     \
    _In_    LONGLONG    hnsLowerBound, \
    _In_    LONGLONG    hnsUpperBound  \
    )                                  \
{\
    UNREFERENCED_PARAMETER(hnsLowerBound); \
    UNREFERENCED_PARAMETER(hnsUpperBound); \
    return E_NOTIMPL;                      \
}\
    STDMETHOD(MFTProcessMessage)(         \
    _In_    MFT_MESSAGE_TYPE    eMessage, \
    _In_    ULONG_PTR           ulParam   \
    )                                     \
{                                         \
    UNREFERENCED_PARAMETER(eMessage);     \
    UNREFERENCED_PARAMETER(ulParam);      \
    return E_NOTIMPL;                     \
}                                         \
    STDMETHOD(MFTProcessInput)(           \
    _In_    DWORD       dwInputStreamID,  \
    _In_    IMFSample*  pSample,          \
    _In_    DWORD       dwFlags           \
    )                                     \
{                                             \
    UNREFERENCED_PARAMETER(dwInputStreamID);  \
    UNREFERENCED_PARAMETER(pSample);          \
    UNREFERENCED_PARAMETER(dwFlags);          \
    return E_NOTIMPL;                         \
}                                             \
    STDMETHOD(MFTProcessOutput)(              \
    _In_    DWORD                       dwFlags,                                   \
    _In_    DWORD                       cOutputBufferCount,                        \
    _Inout_updates_(cOutputBufferCount)  MFT_OUTPUT_DATA_BUFFER  *pOutputSamples,  \
    _Inout_ DWORD                       *pdwStatus                                 \
    )                                                                              \
{                                                  \
    UNREFERENCED_PARAMETER(dwFlags);               \
    UNREFERENCED_PARAMETER(cOutputBufferCount);    \
    UNREFERENCED_PARAMETER(pOutputSamples);        \
    UNREFERENCED_PARAMETER(pdwStatus);             \
    return E_NOTIMPL;                              \
}


