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

//
// The below guid is used to register the GUID as the Device Transform. This should be adeed to the 
// HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\DeviceClasses\ and under the 
// GLOBAL#\Device Parameters key, add a CameraDeviceMFTCLSID value, and set its value to
// {0E313280-3169-4F41-A329-9E854169634F} for the Pipeline to pick up the Transform.
//
DEFINE_GUID(CLSID_MultiPinMFT,
    0xe313280, 0x3169, 0x4f41, 0xa3, 0x29, 0x9e, 0x85, 0x41, 0x69, 0x63, 0x4f);


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



//TP_NORMAL
//TP_LOWEST
//TP_LOW
//TP_HIGH
//TP_HIGHEST
//TP_ERROR
//TP_MUTED = 0x00070000
#ifdef DBG
#define mf_assert(a) if(!a) DebugBreak()
#else
#define mf_assert(a)
#endif

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

#define WPP_CHECK_LEVEL_ENABLED(flags, level) 1

//begin_wpp config
//
// USEPREFIX (DMFTCHECKNULL_GOTO,"%!STDPREFIX!");
// FUNC DMFTCHECKNULL_GOTO(CHECKNULLGOTO_EXP,LABEL,HR,...);
// USESUFFIX (DMFTCHECKNULL_GOTO," failed %!HRESULT!\n", hr);
//
//end_wpp

//begin_wpp config
//
// USEPREFIX (DMFTCHECKHR_GOTO,"%!STDPREFIX!");
// FUNC DMFTCHECKHR_GOTO(CHECKHRGOTO_EXP,LABEL,...);
// USESUFFIX (DMFTCHECKHR_GOTO," failed %!HRESULT!\n", hr);
//

//end_wpp

#define WPP_CHECKNULLGOTO_EXP_LABEL_HR_PRE(pointer,label,HR) if( pointer == NULL ) { hr = HR;
#define WPP_CHECKNULLGOTO_EXP_LABEL_HR_POST(pointer,label,HR) ; goto label; }

#define WPP_CHECKNULLGOTO_EXP_LABEL_HR_LOGGER(pointer,label,HR) WPP_LEVEL_LOGGER( DMFT_INIT )
#define WPP_CHECKNULLGOTO_EXP_LABEL_HR_ENABLED(pointer,label,HR) WPP_CHECK_LEVEL_ENABLED( DMFT_INIT, TP_ERROR )

#define WPP_CHECKHRGOTO_EXP_LABEL_PRE(HR,label) if( FAILED( hr = HR ) ) {
#define WPP_CHECKHRGOTO_EXP_LABEL_POST(HR,label) ; goto label; }

#define WPP_CHECKHRGOTO_EXP_LABEL_LOGGER(HR, label) WPP_LEVEL_LOGGER( DMFT_INIT )
#define WPP_CHECKHRGOTO_EXP_LABEL_ENABLED(HR, label) WPP_CHECK_LEVEL_ENABLED( DMFT_INIT, TP_ERROR )


// Give it checkhr and checknull definitions if some future generations of visual studio remove the wpp processor support

//#if !defined DMFTCHECKHR_GOTO
//#define DMFTCHECKHR_GOTO(a,b) {hr=(a); if(FAILED(hr)){goto b;}} 
//#endif
//
//#if !defined DMFTCHECKNULL_GOTO
//#define DMFTCHECKNULL_GOTO(a,b,c) {if(!a) {hr = c; goto b;}} 
//#endif

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
#define SAFE_CLOSEHANDLE(h)         { if(INVALID_HANDLE_VALUE != (h)) { ::CloseHandle(h); (h) = INVALID_HANDLE_VALUE; } }


#define SAFERELEASE(x) \
if (x) {\
    x->Release(); \
    x = NULL; \
}

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




typedef enum _DMFT_conversion_type{
    DeviceMftTransformTypeXVP,              // Need an XVP
    DeviceMftTransformTypeDecoder,          // Need a Decoder
    DeviceMftTransformTypeEqual,            // No op.. The media types are equal    
    DeviceMftTransformTypeIllegal           // We cannot satisfy the input and output combination
}DMFT_conversion_type,*PDMFT_conversion_type;

typedef  std::vector <ComPtr<IMFMediaType>>  IMFMediaTypeArray;
typedef  std::vector <ComPtr<CBasePin>>      CBasePinArray;
typedef  std::vector <ComPtr<IMFSample>>     IMFSampleList;
typedef  std::pair< std::multimap<int, int>::iterator, std::multimap<int, int>::iterator > MMFTMMAPITERATOR;



STDMETHODIMP   IsOptimizedPlanarVideoInputImageOutputPair(
    _In_        IMFMediaType *inMediaType,
    _In_        IMFMediaType *outMediaType,
    _Out_       bool *optimized,
    _Out_       bool *optimizedxvpneeded
    );

STDMETHODIMP CompareMediaTypesForConverter(_In_opt_ IMFMediaType *inMediaType,
    _In_        IMFMediaType                *newMediaType,
    _Inout_     PDMFT_conversion_type       operation
    );

HRESULT IsInputDxSample(
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
        return E_UNEXPECTED;
    }
}

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

//
// Object LifeTime manager. The Class has a global variable which
// maintains a reference count of the number of objects in the
// system managed by the DLL. 
//
class CDMFTModuleLifeTimeManager{
public:
    CDMFTModuleLifeTimeManager()
    {
        InterlockedIncrement(&s_lObjectCount);
    }
    ~CDMFTModuleLifeTimeManager()
    {
        InterlockedDecrement(&s_lObjectCount);
    }
    static long GetDMFTObjCount()
    {
        return s_lObjectCount;
    }
private:
    static volatile long s_lObjectCount;
};
#define DECLARE_TransformImageFunctions(a) void TransformImage_##a(\
const RECT &rcDest,\
_Inout_updates_(_Inexpressible_(lDestStride * dwHeightInPixels)) BYTE *pDest,\
_In_ LONG lDestStride,\
_In_reads_(_Inexpressible_(lSrcStride * dwHeightInPixels)) const BYTE *pSrc,\
_In_ LONG lSrcStride,\
_In_ DWORD dwWidthInPixels,\
_In_ DWORD dwHeightInPixels)

DECLARE_TransformImageFunctions(YUY2);
DECLARE_TransformImageFunctions(UYVY);
DECLARE_TransformImageFunctions(NV12);
DECLARE_TransformImageFunctions(RGB32);

class VideoBufferLock
{
public:
    VideoBufferLock(IMFMediaBuffer *pBuffer) 
    {
        m_spBuffer = pBuffer;
        // Query for the 2-D buffer interface. OK if this fails.
        if (FAILED(m_spBuffer->QueryInterface(IID_PPV_ARGS(m_sp2DBuffer2.GetAddressOf()))))
        {
            m_spBuffer->QueryInterface(IID_PPV_ARGS(m_sp2DBuffer.GetAddressOf()));
        } 
    }
    ~VideoBufferLock()
    {
        UnlockBuffer();
    }
    // LockBuffer:
    // Locks the buffer. Returns a pointer to scan line 0 and returns the stride.

    // The caller must provide the default stride as an input parameter, in case
    // the buffer does not expose IMF2DBuffer. You can calculate the default stride
    // from the media type.

    HRESULT LockBuffer(
        _In_        LONG  lDefaultStride,    // Minimum stride (with no padding).
        _In_        DWORD dwHeightInPixels,  // Height of the image, in pixels.
        _Out_       BYTE  **ppbScanLine0,    // Receives a pointer to the start of scan line 0.
        _Inout_     LONG  *plStride,         // Receives the actual stride.
        _Out_       DWORD *pcbBuffer,        // Length of the Buffer
        _In_        BOOL   Read = TRUE       // Whether Read    
    )
    {
        HRESULT hr = S_OK;
        // Use the 2-D version if available.
        if (m_sp2DBuffer2)
        {
            BYTE *pbBufferStart = NULL;
            MF2DBuffer_LockFlags Flags = (Read == TRUE) ? MF2DBuffer_LockFlags_Read : MF2DBuffer_LockFlags_Write;
            hr = m_sp2DBuffer2->Lock2DSize(
                Flags,
                ppbScanLine0,
                plStride,
                &pbBufferStart,
                pcbBuffer);
            if (dwHeightInPixels* abs(*plStride) > *pcbBuffer)
            {
                hr = E_UNEXPECTED;
            }
        } else if (m_sp2DBuffer)
        {
            hr = m_sp2DBuffer->Lock2D(ppbScanLine0, plStride);
        }
        else
        {
            // Use non-2D version.
            BYTE *pData = NULL;
            hr = m_spBuffer->Lock(&pData, NULL, NULL);
            if (SUCCEEDED(hr))
            {
                *plStride = lDefaultStride;
                if (lDefaultStride < 0)
                {
                    // Bottom-up orientation. Return a pointer to the start of the
                    // last row *in memory* which is the top row of the image.
                    *ppbScanLine0 = pData + abs(lDefaultStride) * (dwHeightInPixels - 1);
                }
                else
                {
                    // Top-down orientation. Return a pointer to the start of the
                    // buffer.
                    *ppbScanLine0 = pData;
                }
            }
        }
        return hr;
    }
    HRESULT UnlockBuffer()
    {
        if (m_sp2DBuffer2.Get())
        {
            return m_sp2DBuffer2->Unlock2D();
        }
        else if (m_sp2DBuffer.Get())
        {
            return m_sp2DBuffer->Unlock2D();
        }
        else
        {
            return m_spBuffer->Unlock();
        }
    }
private:
    ComPtr<IMFMediaBuffer>  m_spBuffer;
    ComPtr<IMF2DBuffer>     m_sp2DBuffer;
    ComPtr<IMF2DBuffer2>    m_sp2DBuffer2;
};

// Decoding and Allocator specific
HRESULT CreateDecoderHW(    _In_ IMFDXGIDeviceManager* pManager,
                            _In_ IMFMediaType* inType,
                            _In_ IMFMediaType* outType,
                            _Outptr_ IMFTransform** ppTransform,
                            _Inout_ BOOL&);

HRESULT EnumSWDecoder(      _Outptr_ IMFTransform** ppTransform,
                            _In_ GUID subType);

HRESULT SetDX11BindFlags(   _In_  IUnknown *pUnkManager,
                            _In_ GUID guidPinCategory,
                            _Inout_ DWORD &dwBindFlags);

HRESULT IsDXFormatSupported(
    _In_ IMFDXGIDeviceManager* pDeviceManager,
    _In_ GUID subType,
    _Outptr_opt_ ID3D11Device** ppDevice,
    _In_opt_ PUINT32 pSupportedFormat);

HRESULT ConfigureAllocator(
    _In_ IMFMediaType* pOutputMediaType,
    _In_ GUID streamCategory,
    _In_ IUnknown* pDeviceManagerUnk,
    _In_ BOOL &isDxAllocator,
    _In_ IMFVideoSampleAllocator* pAllocator);

HRESULT CreateAllocator( _In_ IMFMediaType* pOutputMediaType,
    _In_ GUID streamCategory,
    _In_ IUnknown* pDeviceManagerUnk,
    _In_ BOOL &isDxAllocator,      
    _Outptr_ IMFVideoSampleAllocatorEx** ppAllocator);

HRESULT GetDXGIAdapterLuid( _In_ IMFDXGIDeviceManager *pDXManager,
    _Out_ LUID *pAdapterLuid);

HRESULT MergeSampleAttributes(_In_ IMFSample* pInSample, _Inout_ IMFSample* pOutSample);

HRESULT CheckPassthroughMediaType( _In_ IMFMediaType *pMediaType1,
    _In_ IMFMediaType *pMediaType2,
    _Out_ BOOL& pfPassThrough);
//
// Internal attribute
//
EXTERN_GUID(MF_SA_D3D11_SHARED_WITH_NTHANDLE_PRIVATE, 0xb18f1ad, 0xe8c6, 0x4804, 0xb7, 0x70, 0x15, 0x69, 0xdd, 0xc4, 0xbf, 0xe8);

