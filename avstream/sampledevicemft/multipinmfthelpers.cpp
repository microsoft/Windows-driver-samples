#include "stdafx.h"
#include "common.h"
#include "multipinmfthelpers.h"
#include "multipinmft.h"
#include "basepin.h"
#include <wincodec.h>
#include <Codecapi.h>

#ifdef MF_WPP
#include "multipinmfthelpers.tmh"    //--REF_ANALYZER_DONT_REMOVE--
#endif

class CMediaTypePrinter;
//
//Queue implementation
//
CPinQueue::CPinQueue(   _In_ DWORD dwPinId ,
                        _In_ IMFDeviceTransform* pParent)
    :m_dwInPinId(dwPinId),
    m_pTransform(pParent),
    m_cRef(1)
    
    /*
    Description
    dwPinId is the input pin Id to which this queue corresponds
    */
{
    m_streamCategory = GUID_NULL;
}
CPinQueue::~CPinQueue( )
{
    Ctee::ReleaseTee(m_spTeer);
}

/*++
Description:
    Insert sample into the list once we reach the open queue
--*/
STDMETHODIMP_(VOID) CPinQueue::InsertInternal( _In_ IMFSample *pSample )
{
    HRESULT hr = ExceptionBoundary([&]()
    {
        m_sampleList.push_back(pSample);
    });

    if (SUCCEEDED(hr) && m_pTransform)
    {
        hr = reinterpret_cast<CMultipinMft*>(m_pTransform)->QueueEvent(METransformHaveOutput, GUID_NULL, S_OK, NULL);
    }

    if (FAILED(hr))
    {
        DMFTRACE(DMFT_GENERAL, TRACE_LEVEL_INFORMATION, "%!FUNC! exiting %x = %!HRESULT!", hr, hr);
    }
}

STDMETHODIMP CPinQueue::Insert( _In_ IMFSample *pSample )
//
//m_spTeer is the wraptee.. this could be a null tee which is a passthrough, an xvp tee which inserts an xvp into the queue etc
//
{
    return m_spTeer->PassThrough( pSample );
}

/*++
Description:
    This extracts the first sample from the queue. called from Pin's ProcessOutput 
--*/

STDMETHODIMP CPinQueue::Remove( _Outptr_result_maybenull_ IMFSample **ppSample)
{
    HRESULT hr = S_OK;
    DMFTCHECKNULL_GOTO( ppSample, done,E_INVALIDARG );
    *ppSample = nullptr;

    if ( !m_sampleList.empty() )
    {
        *ppSample = m_sampleList.front().Detach();
    }

    DMFTCHECKNULL_GOTO( *ppSample, done, MF_E_TRANSFORM_NEED_MORE_INPUT );
    m_sampleList.erase( m_sampleList.begin() );
done:
    return hr;
}

/*++
Description:
    Empties the Queue. used by the flush
--*/
VOID CPinQueue::Clear( )
{
    //
    // Stop the tees
    // Execute Flush
    //
    if (m_spTeer)
    {
        m_spTeer->Stop();
    }

    while ( !Empty() )
    {
        ComPtr<IMFSample> spSample;
        Remove( spSample.GetAddressOf());
    }
}

/*++
Description:
    RecreateTee creates the underlying Tees in the queue. It accepts the input media type
    which is the media type set on the input pin and the output mediatype, which (duh) is the
    media type on the output pin
    It also takes an IUnknown which is the D3D Manager. if it is valid we might use it if we
    have an xvp in the path i.e. inputMediatype =! outputMediatype
--*/

STDMETHODIMP CPinQueue::RecreateTee(_In_  IMFMediaType *inMediatype,
    _In_ IMFMediaType *outMediatype,
    _In_opt_ IUnknown* punkManager)
{
    HRESULT hr = S_OK;
    DMFT_conversion_type operation = DeviceMftTransformTypeIllegal;

    Ctee::ReleaseTee(m_spTeer);

    ComPtr<CNullTee> spNulltee = new (std::nothrow) CNullTee(this);
    DMFTCHECKNULL_GOTO(spNulltee.Get(), done, E_OUTOFMEMORY);

    DMFTCHECKHR_GOTO(CompareMediaTypesForConverter(inMediatype, outMediatype, &operation), done);

    if (operation == DeviceMftTransformTypeDecoder)
    {
        // Decoder needed
        CDecoderTee* pDecTee = new (std::nothrow) CDecoderTee(spNulltee.Get(),
            static_cast<CMultipinMft*>(m_pTransform)->GetQueueId(),
            pinCategory());
        DMFTCHECKNULL_GOTO(pDecTee, done, E_OUTOFMEMORY);
        (void)pDecTee->SetD3DManager(punkManager);
        DMFTCHECKHR_GOTO(pDecTee->SetMediaTypes(inMediatype, outMediatype), done);
        m_spTeer = pDecTee;
    }
    else if (operation == DeviceMftTransformTypeXVP)
    {
        CXvptee* pXvptee = new (std::nothrow) CXvptee(spNulltee.Get(), pinCategory());
        DMFTCHECKNULL_GOTO(pXvptee, done, E_OUTOFMEMORY);
        (void)pXvptee->SetD3DManager(punkManager);
        DMFTCHECKHR_GOTO(pXvptee->SetMediaTypes(inMediatype, outMediatype), done);
        m_spTeer = pXvptee;
    }
    else
    {
        m_spTeer = spNulltee.Get(); /*A simple passthrough*/
    }

done:
    DMFTRACE(DMFT_GENERAL, TRACE_LEVEL_INFORMATION, "%!FUNC! exiting %x = %!HRESULT!", hr, hr);

    if (FAILED(hr))
    {
        Ctee::ReleaseTee(m_spTeer);
    }
    return hr;
}
#if ((defined NTDDI_WIN10_VB) && (NTDDI_VERSION >= NTDDI_WIN10_VB))
STDMETHODIMP CPinQueue::RecreateTeeByAllocatorMode(
    _In_  IMFMediaType* inMediatype,
    _In_ IMFMediaType* outMediatype,
    _In_opt_ IUnknown* punkManager,
    _In_ MFSampleAllocatorUsage allocatorUsage,
    _In_opt_ IMFVideoSampleAllocator* pAllocator)
{
    HRESULT hr = S_OK;

    Ctee::ReleaseTee(m_spTeer);// Should release the reference

    ComPtr<CNullTee> spNulltee = new (std::nothrow) CNullTee(this);
    DMFTCHECKNULL_GOTO(spNulltee.Get(), done, E_OUTOFMEMORY);

    if (allocatorUsage == MFSampleAllocatorUsage_DoesNotAllocate)
    {
        m_spTeer = spNulltee.Get();
    }
    else
    {
        ComPtr<CSampleCopytee> spSampleCopytee;
        if (allocatorUsage == MFSampleAllocatorUsage_UsesProvidedAllocator)
        {
            RETURN_HR_IF_NULL(E_INVALIDARG, pAllocator);
            spSampleCopytee = new (std::nothrow) CSampleCopytee(spNulltee.Get(), pinCategory(), pAllocator);
        }
        else
        {
            spSampleCopytee = new (std::nothrow) CSampleCopytee(spNulltee.Get(), pinCategory(), nullptr);
        }
        DMFTCHECKNULL_GOTO(spSampleCopytee.Get(), done, E_OUTOFMEMORY);
        (void)spSampleCopytee->SetD3DManager(punkManager);
        RETURN_IF_FAILED(spSampleCopytee->SetMediaTypes(inMediatype, outMediatype));
        m_spTeer = spSampleCopytee.Get();
    }
done:
    return hr;
}
#endif // ((defined NTDDI_WIN10_VB) && (NTDDI_VERSION >= NTDDI_WIN10_VB))
#ifdef MF_DEVICEMFT_ADD_GRAYSCALER_
STDMETHODIMP CPinQueueWithGrayScale::RecreateTee( _In_  IMFMediaType *inMediatype,
    _In_ IMFMediaType *outMediatype,
    _In_opt_ IUnknown* punkManager)
{
    HRESULT hr = S_OK;
    
    GUID      gInputSubType = GUID_NULL;
    DMFTCHECKNULL_GOTO(inMediatype, done, E_INVALIDARG);
    DMFTCHECKNULL_GOTO(outMediatype, done, E_INVALIDARG);
    DMFTCHECKHR_GOTO(CPinQueue::RecreateTee(inMediatype,
        outMediatype,
        punkManager),done);
    DMFTCHECKNULL_GOTO(m_spTeer, done, E_UNEXPECTED);
    // Wrap the media type with Gray scale tee only if the input media type is a YUY2, UYVY, NV12 or RGB32
    DMFTCHECKHR_GOTO(inMediatype->GetGUID(MF_MT_SUBTYPE, &gInputSubType), done);
    if (IsEqualCLSID(gInputSubType, MFVideoFormat_NV12)
        ||IsEqualCLSID(gInputSubType, MFVideoFormat_YUY2)
        ||IsEqualCLSID(gInputSubType, MFVideoFormat_UYVY)
        ||IsEqualCLSID(gInputSubType, MFVideoFormat_RGB32))
    {
        CGrayTee *pTee = NULL;
        pTee = new (std::nothrow) CGrayTee(m_spTeer.Get());
        DMFTCHECKHR_GOTO(pTee->SetMediaTypes(inMediatype, outMediatype), done);
        m_spTeer = dynamic_cast< Ctee* >(pTee);
    }
    else
    {
        DMFTRACE(DMFT_GENERAL, TRACE_LEVEL_INFORMATION, "%!FUNC! Skipping Gray scale Effect");
    }
done:
    return hr;
}
#endif

/*++
Description:
    This is the passthrough Tee in the literal sense i.e. It just dumps the sample
    into the queue qupplied as an argument
--*/
STDMETHODIMP CNullTee::PassThrough( _In_ IMFSample *pSample )
{
    HRESULT hr = S_OK;
    DMFTCHECKNULL_GOTO(pSample, done, S_OK); //No OP for a NULL Sample!!!
    DMFTCHECKNULL_GOTO(m_Queue.Get(), done, E_UNEXPECTED); // State not set correctly
    //
    // @@@@ README: If there is a deocding operation that happens then the output samples will be here
    // Any processing work like sticthing etc for 360 can happen here
    //
    m_Queue->InsertInternal(pSample);

    done:
    return hr;
}

/*++
Description:
    This function sets the Media types on the XVP or the decoder transform. Here is where we configure the XVP
    or the Decoder or the Encoder tee if present. The first iteration only has the XVP added
--*/
STDMETHODIMP CWrapTee::SetMediaTypes( _In_ IMFMediaType* pInMediaType, _In_ IMFMediaType* pOutMediaType )
{
    m_pInputMediaType = pInMediaType;
    m_pOutputMediaType = pOutMediaType;
    return S_OK;
}

/*++
Description:
This is used when an XVP is present in the Pin. If the path is a passthrough i.e.
the media type on the input and the output pins are the same then we will not see
this path traversed. This function feeds the sample to the XVP or the decoding Tee
--*/
STDMETHODIMP CWrapTee::PassThrough( _In_ IMFSample* pInSample )
{
    HRESULT hr = S_OK;
    ComPtr<IMFSample> spOutSample = nullptr;
    DMFTCHECKNULL_GOTO(pInSample, done, S_OK); // pass through for no sample
    DMFTCHECKHR_GOTO(Do(pInSample, spOutSample.ReleaseAndGetAddressOf()), done);
    
    if (m_spObjectWrapped)
    {
        DMFTCHECKHR_GOTO(m_spObjectWrapped->PassThrough(spOutSample.Get()), done);
    }

done:
    if (FAILED(hr))
    {
        DMFTRACE(DMFT_GENERAL, TRACE_LEVEL_INFORMATION, "%!FUNC! exiting %x = %!HRESULT!", hr, hr);
    }

    return hr;
}

STDMETHODIMP CVideoProcTee::StartStreaming()
{
    return S_OK;
}
HRESULT CVideoProcTee::SetMediaTypes(_In_ IMFMediaType* pInMediaType, _In_ IMFMediaType* pOutMediaType)
{
    HRESULT hr = S_OK;
    ComPtr<IMFTransform> spTransform;
    DMFTCHECKHR_GOTO(CWrapTee::SetMediaTypes(pInMediaType, pOutMediaType),done);
    DMFTCHECKHR_GOTO(Configure(pInMediaType, pOutMediaType, spTransform.ReleaseAndGetAddressOf()), done);
    m_spVideoProcessor = spTransform;
    //
    // Start streaming
    //
    DMFTCHECKHR_GOTO(StartStreaming(), done);
done:
    return hr;
}



HRESULT CVideoProcTee::CreateAllocator()
{
    HRESULT hr = S_OK;
    if (m_bProducesSamples)
    {
        // Don't create the Allocator as Samples are created by the Encoder/ Decoder
        return S_OK;
    }

    if (m_spPrivateAllocator)
    {
        // Release what we have currently
        RETURN_IF_FAILED(m_spPrivateAllocator->UninitializeSampleAllocator());
        m_spPrivateAllocator = nullptr;
    }
#if ((defined NTDDI_WIN10_VB) && (NTDDI_VERSION >= NTDDI_WIN10_VB))
    if (m_spDefaultAllocator)
    {
        // Configure default allocator
        m_spDefaultAllocator->UninitializeSampleAllocator();
        RETURN_IF_FAILED(::ConfigureAllocator(
            m_pOutputMediaType.Get(),
            m_streamCategory,
            m_spDeviceManagerUnk.Get(),
            m_fSetD3DManager,
            m_spDefaultAllocator.Get()));
    }
    else
#endif // ((defined NTDDI_WIN10_VB) && (NTDDI_VERSION >= NTDDI_WIN10_VB))
    {
        // Create and configure private allocator
        RETURN_IF_FAILED(MFCreateVideoSampleAllocatorEx(IID_PPV_ARGS(m_spPrivateAllocator.ReleaseAndGetAddressOf())));
        RETURN_IF_FAILED(::ConfigureAllocator(
            m_pOutputMediaType.Get(),
            m_streamCategory,
            m_spDeviceManagerUnk.Get(),
            m_fSetD3DManager,
            m_spPrivateAllocator.Get()));
    }

    return hr;
}

CVideoProcTee::~CVideoProcTee()
{
    if (m_spPrivateAllocator.Get())
    {
        (VOID)m_spPrivateAllocator->UninitializeSampleAllocator();
        m_spPrivateAllocator = nullptr;
    }
}

// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@//
// @@@@ README: Video Processor functions below
//
// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@//

CXvptee::CXvptee(_In_ Ctee *tee, GUID category) :
    CVideoProcTee(tee, category)
{

}

CXvptee::~CXvptee()
{
    (VOID)StopStreaming();
    m_spDeviceManagerUnk = nullptr;
}

HRESULT CXvptee::StartStreaming()
{
    HRESULT hr = S_OK;
    CAutoLock Lock(m_Lock);
    DMFTCHECKHR_GOTO(Transform()->MFTProcessMessage(MFT_MESSAGE_NOTIFY_BEGIN_STREAMING, 0), done); // ulParam set to zero
    DMFTCHECKHR_GOTO(Transform()->MFTProcessMessage(MFT_MESSAGE_NOTIFY_START_OF_STREAM, 0), done); // ulParam set to zero
done:
    return hr;
}

HRESULT CXvptee::StopStreaming()
{
    HRESULT hr = S_OK;
    CAutoLock Lock(m_Lock);
    SetAsyncStatus(MF_E_SHUTDOWN);
    Transform()->MFTProcessMessage(MFT_MESSAGE_COMMAND_FLUSH, 0); // Flush the stream
    Transform()->MFTProcessMessage(MFT_MESSAGE_NOTIFY_END_OF_STREAM, 0); // Notify end of stream
    Transform()->MFTProcessMessage(MFT_MESSAGE_NOTIFY_END_STREAMING, 0); // Notify end of streaming
    return hr;
}

/*++
CXvptee::Do
Description:
Called to do what an XVP is supposed to do i.e. scaling, resize etc.
This function is called when the samples are desposited from the input pin to the
output pin queue during a ProcessInput call on the device transform. Of course the 
outpin should be in Open state for the sample to reach the XVP and consequetively the
Output Pin.

--*/
STDMETHODIMP CXvptee::Do(_In_ IMFSample *pSample, _Outptr_ IMFSample** ppOutSample)
{
    HRESULT                  hr = S_OK;
    MFT_OUTPUT_DATA_BUFFER   outputSample;
    ComPtr <IMFSample>       spXVPOutputSample;
    DWORD dwStatus = 0;
    CAutoLock Lock(m_Lock);

    DMFTRACE(DMFT_GENERAL, TRACE_LEVEL_INFORMATION, "%!FUNC! XVP ProcessOutput Processing Sample =%p", pSample);
    DMFTCHECKNULL_GOTO(ppOutSample, done, E_INVALIDARG);
    *ppOutSample = nullptr;
    DMFTCHECKHR_GOTO(GetAsyncStatus(), done);
    DMFTCHECKHR_GOTO(Transform()->MFTProcessInput(0, pSample, 0),done);

    outputSample.dwStreamID = 0;
    outputSample.pSample = NULL;
    DMFTCHECKHR_GOTO(m_spPrivateAllocator->AllocateSample(&outputSample.pSample), done);

    hr = Transform()->MFTProcessOutput(0, 1, &outputSample, &dwStatus);

     if ( SUCCEEDED(hr) && pSample)
    {
        spXVPOutputSample.Attach(outputSample.pSample);
        //Let us copy the timestamps
        LONGLONG hnsSampleDuration = 0;
        LONGLONG hnsSampleTime = 0;
        DMFTCHECKHR_GOTO(MergeSampleAttributes(pSample, spXVPOutputSample.Get()), done);

        if (SUCCEEDED(pSample->GetSampleDuration(&hnsSampleDuration)))
        {
            spXVPOutputSample->SetSampleDuration(hnsSampleDuration);
        }

        if (SUCCEEDED(pSample->GetSampleTime(&hnsSampleTime)))
        {
            spXVPOutputSample->SetSampleTime(hnsSampleTime);
        }
    }
    else
    {
        SAFE_RELEASE(outputSample.pSample);
    }
 
    if (spXVPOutputSample.Get())
    {
        *ppOutSample = spXVPOutputSample.Detach();
    }
done:
    if (SUCCEEDED(hr))
    {
        DMFTRACE(DMFT_GENERAL, TRACE_LEVEL_INFORMATION, "%!FUNC! [hr=0x%x Sample=%p] ", hr, *ppOutSample);
    }
    else
    {
        DMFTRACE(DMFT_GENERAL, TRACE_LEVEL_INFORMATION, "%!FUNC! [Error hr=0x%x Sample=%p] ", hr, *ppOutSample);
    }
    return hr;
}


/*++
Descrtiption:
    Create the XVP Transform with the media types supplied. 
    This is invoked when an XVP is added to the Queue in the Output pins
    and the input pins media type and the output pins media type don't match.

--*/
STDMETHODIMP CXvptee::Configure(
    _In_opt_ IMFMediaType* inMediaType,
    _In_opt_ IMFMediaType* outMediaType,
    _Outptr_ IMFTransform **ppTransform
    )
{
    HRESULT      hr = S_OK;
    ComPtr<IMFAttributes> spXvpTransformAttributes = nullptr;
    DMFTCHECKNULL_GOTO(ppTransform, done, E_INVALIDARG);

    //
    //Print out the Media type set on the XVP
    //
    {
        CMediaTypePrinter inType(inMediaType);
        CMediaTypePrinter outType(outMediaType);
        DMFTRACE(DMFT_GENERAL, TRACE_LEVEL_INFORMATION, "%!FUNC! Input MediaType %s", inType.ToString());
        DMFTRACE(DMFT_GENERAL, TRACE_LEVEL_INFORMATION, "%!FUNC! Output MediaType %s", outType.ToString());
    }

    DMFTCHECKHR_GOTO( CoCreateInstance(
        CLSID_VideoProcessorMFT,
        NULL,
        CLSCTX_INPROC_SERVER,
        IID_PPV_ARGS(ppTransform)), done);


    DMFTCHECKHR_GOTO((*ppTransform)->GetAttributes(spXvpTransformAttributes.GetAddressOf()),done);
    DMFTCHECKHR_GOTO(spXvpTransformAttributes->SetUINT32(MF_XVP_DISABLE_FRC, TRUE),done);
    // Use caller allocated memory.. We will create the allocator for the XVP
    DMFTCHECKHR_GOTO(spXvpTransformAttributes->SetUINT32(MF_XVP_CALLER_ALLOCATES_OUTPUT, TRUE), done);
    m_bProducesSamples = FALSE;

    DMFTCHECKHR_GOTO((*ppTransform)->MFTSetInputType(0, inMediaType, 0), done);
    
    DMFTCHECKHR_GOTO((*ppTransform)->MFTSetOutputType(0, outMediaType, 0), done);

    DMFTCHECKHR_GOTO(CreateAllocator(), done);

    if (m_fSetD3DManager)
    {
        DMFTCHECKHR_GOTO((*ppTransform)->MFTProcessMessage(MFT_MESSAGE_SET_D3D_MANAGER, reinterpret_cast<UINT_PTR>(m_spDeviceManagerUnk.Get())), done);
    }
done:
    return hr;

}

// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@//
// @@@@ README: Decoder related functions below
//
// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@//

CDecoderTee::~CDecoderTee()
{
    (VOID)StopStreaming();
    MFUnlockWorkQueue(m_dwCameraStreamWorkQueueId);
}

HRESULT CDecoderTee::StartStreaming()
{
    HRESULT hr = S_OK;
    CAutoLock Lock(&m_Lock);
    DMFTCHECKNULL_GOTO(Transform(), done, MF_E_UNEXPECTED);
    DMFTCHECKHR_GOTO(Transform()->MFTProcessMessage(MFT_MESSAGE_NOTIFY_BEGIN_STREAMING, 0), done);
    DMFTCHECKHR_GOTO(Transform()->MFTProcessMessage(MFT_MESSAGE_NOTIFY_START_OF_STREAM, 0), done);
 done:
    return hr;
}

HRESULT CDecoderTee::StopStreaming()
{
    HRESULT hr = S_OK;
    ComPtr<IMFTransform> spTransform;
    {
        CAutoLock Lock(&m_Lock);
        spTransform = Transform();
        SetAsyncStatus(MF_E_SHUTDOWN);
    }
    if (spTransform.Get())
    {
        ComPtr<IMFShutdown> spShutdown;
        spTransform->MFTProcessMessage(MFT_MESSAGE_NOTIFY_END_OF_STREAM, m_dwMFTInputId);
        spTransform->MFTProcessMessage(MFT_MESSAGE_COMMAND_FLUSH, 0);
        spTransform->MFTProcessMessage(MFT_MESSAGE_NOTIFY_END_STREAMING, 0);
        // Shut it down
        if (SUCCEEDED(spTransform->QueryInterface(IID_PPV_ARGS(&spShutdown))))
        {
            (void)spShutdown->Shutdown();
        }
        
        spTransform = nullptr;
    }
    return hr;
}

STDMETHODIMP CDecoderTee::Configure(_In_opt_ IMFMediaType *inType,
    _In_opt_ IMFMediaType *outType,
    _Outptr_ IMFTransform** ppTransform)
{
    HRESULT hr = S_OK;
    ComPtr<IMFTransform> spTransform;
    GUID gInSubType = GUID_NULL, gOutSubType = GUID_NULL;

    DMFTCHECKNULL_GOTO(ppTransform, done, E_INVALIDARG);
    DMFTCHECKNULL_GOTO(inType, done, E_INVALIDARG);
    DMFTCHECKNULL_GOTO(outType, done, E_INVALIDARG);
    *ppTransform = nullptr;

    if (!(SUCCEEDED(inType->GetGUID(MF_MT_SUBTYPE, &gInSubType))
        && SUCCEEDED(outType->GetGUID(MF_MT_SUBTYPE, &gOutSubType))))
    {
        DMFTCHECKHR_GOTO(E_UNEXPECTED, done);
    }

    DMFTCHECKHR_GOTO(MFAllocateSerialWorkQueue(m_dwQueueId, &m_dwCameraStreamWorkQueueId), done);

    if (IsEqualCLSID(gInSubType, MFVideoFormat_H264) || (IsEqualCLSID(gInSubType, MFVideoFormat_MJPG)))
    {
        MFT_OUTPUT_STREAM_INFO outputStreamInfo;
        if (SUCCEEDED(hr = CreateDecoderHW(
            reinterpret_cast<IMFDXGIDeviceManager*>(m_spDeviceManagerUnk.Get()),
            m_pInputMediaType.Get(),
            m_pOutputMediaType.Get(),
            spTransform.ReleaseAndGetAddressOf(), m_hwMFT)))
        {
            m_hwMFT = TRUE;
            hr = ConfigDecoder(spTransform.Get(), gInSubType);
        }
        if (FAILED(hr))
        {
            // Try creating SW deocder
            hr = S_OK;
            m_hwMFT = FALSE;
            DMFTCHECKHR_GOTO(EnumSWDecoder(spTransform.ReleaseAndGetAddressOf(), gInSubType), done);
            DMFTCHECKHR_GOTO(ConfigDecoder(spTransform.Get(), gInSubType), done);
        }
        DMFTCHECKNULL_GOTO(spTransform.Get(), done, E_UNEXPECTED);

        DMFTCHECKHR_GOTO(spTransform->MFTGetOutputStreamInfo(m_dwMFTOutputId, &outputStreamInfo), done);

        m_bProducesSamples = (outputStreamInfo.dwFlags & (MFT_OUTPUT_STREAM_PROVIDES_SAMPLES | MFT_OUTPUT_STREAM_CAN_PROVIDE_SAMPLES));

        if (!m_bProducesSamples)
        {
            // Create own allocator as the decoder doesn't produce samples
            DMFTCHECKHR_GOTO(CreateAllocator(), done); // Sets fsetd3dmanager
        }

        // Create a callback for processoutputs to be called on
        m_asyncCallback = new (std::nothrow) CDMFTAsyncCallback<CDecoderTee, &CDecoderTee::Invoke>(this, m_dwQueueId);
        DMFTCHECKNULL_GOTO(m_asyncCallback.Get(), done, E_OUTOFMEMORY);

        if (m_fAsyncMFT)
        {
            ComPtr<IMFMediaEventGenerator> spGenerator;
            if (SUCCEEDED(spTransform.As(&spGenerator)) && spGenerator.Get())
            {
                // Start listening to decoder events
                DMFTCHECKHR_GOTO(spGenerator->BeginGetEvent(m_asyncCallback.Get(), spGenerator.Get()), done);
            }
        }
    }
    else
    {
        // We only support the H264 and MJPG decoding
        DMFTCHECKHR_GOTO(MF_E_NOT_AVAILABLE, done);
    }
    *ppTransform = spTransform.Detach();
done:
    return hr;
}


STDMETHODIMP CDecoderTee::Do(_In_ IMFSample* pSample, _Outptr_ IMFSample **ppoutSample)
{
    HRESULT hr = S_OK;
    ComPtr<IMFSample> spOutputSample;
    CAutoLock lock(m_Lock);
    ComPtr<IMFTransform> spTransform = Transform();

    DMFTCHECKNULL_GOTO(ppoutSample, done, E_INVALIDARG);
    *ppoutSample = nullptr;
    DMFTCHECKHR_GOTO(GetAsyncStatus(), done);
    
    DMFTCHECKNULL_GOTO(spTransform, done, E_UNEXPECTED);

    if (!m_fAsyncMFT)
    {
        // Synchronous MFTs. Feed ProcessInputs till we get MF_E_NOTACCEPTING. Processoutputs on seperate thread
        DMFTCHECKHR_GOTO(ExceptionBoundary([&]()
        {
            m_InputSampleList.push_front(pSample);
        }),done);

        for (UINT32 uiIndex = (UINT32)m_InputSampleList.size(); uiIndex > 0; uiIndex--)
        {
            ComPtr<IMFSample> spInputSample;
            spInputSample.Attach(m_InputSampleList[uiIndex-1].Get());
            hr = spTransform->MFTProcessInput(m_dwMFTInputId, spInputSample.Get(), 0);
            if (hr == MF_E_NOTACCEPTING)
            {
                spInputSample.Detach();
                hr = S_OK;
                // Queue a work item for process output
                DMFTCHECKHR_GOTO(MFPutWorkItem(m_dwCameraStreamWorkQueueId, m_asyncCallback.Get(), nullptr), done);
                break;
            }
            m_InputSampleList[uiIndex - 1].Detach(); 
            DMFTCHECKHR_GOTO(hr, done);
            m_InputSampleList.pop_back(); 
        }
        DMFTCHECKHR_GOTO(hr, done);
    }
    else
    {
        // Asynchrnous mode. THE MFT asks for METransformNeedInput and we manage a count of processinputs . We feed it as
        // much as it asks for.
        UINT32 uiIndex = 0;

        DMFTCHECKHR_GOTO(ExceptionBoundary([&]()
        {
            m_InputSampleList.push_front(pSample);
        }), done);

        if (m_lNeedInputRequest > 0)
        {
            for(uiIndex = (UINT32)m_InputSampleList.size(); (uiIndex > 0 && m_lNeedInputRequest > 0);uiIndex--)
            {
                ComPtr<IMFSample> spInputSample;
                spInputSample.Attach(m_InputSampleList[uiIndex-1].Detach());
                hr = spTransform->MFTProcessInput(0, spInputSample.Get(), 0);
                if (FAILED(hr))
                    break;
                m_InputSampleList.pop_back();
                m_lNeedInputRequest--;
            }
        }
    }
done:
    if (FAILED(hr))
    {
        SetAsyncStatus(hr);
    }
    return hr;
}

//
// Asynchronous callback for the decoder, if the decoder runs in asynchronous mode
// This is serial since we give the eventgenerator a serial thread queue
//
HRESULT CDecoderTee::Invoke(_In_ IMFAsyncResult* pResult)
{
    HRESULT                 hr = S_OK;
    HRESULT                 hrEventStatus = S_OK;
    MediaEventType          met = MEUnknown;
    ComPtr<IMFMediaEventGenerator> spEventGenerator;
    ComPtr<IMFMediaEvent>   spEvent;
    ComPtr<IMFTransform>    spDecoderTemp;
    ComPtr<IMFSample>       spOutputSample;
    BOOL                    bSendSample = FALSE;
    CAutoLock lock(m_Lock);

    spDecoderTemp = Transform();

    DMFTCHECKNULL_GOTO(pResult, done, E_UNEXPECTED);
    if (m_fAsyncMFT)
    {
        
        DMFTCHECKHR_GOTO(Transform()->QueryInterface(IID_PPV_ARGS(&spEventGenerator)), done);
        DMFTCHECKHR_GOTO(spEventGenerator->EndGetEvent(pResult, &spEvent), done);
        DMFTCHECKHR_GOTO(spEvent->GetType(&met), done);
        DMFTCHECKHR_GOTO(spEvent->GetStatus(&hrEventStatus), done);
        if (nullptr != spDecoderTemp)
        {
            DMFTCHECKHR_GOTO(GetAsyncStatus(), done);
            switch (met)
            {
            case MEError:
                __fallthrough;
            case MENonFatalError:
                SetAsyncStatus(hrEventStatus);
                break;
            case METransformNeedInput:
                m_lNeedInputRequest++;
                break;
            case METransformHaveOutput:
            {
                ComPtr<IMFSample> spDecodedSample;
                m_Lock.Unlock();
                //
                // The processoutput call blocks for H264 sometimes. This call back is serialized
                //
                hr = ProcessOutputSync(spDecodedSample.GetAddressOf());
                m_Lock.Lock();
                // Did we error out when we released the lock?
                DMFTCHECKHR_GOTO(GetAsyncStatus(), done);
               if (SUCCEEDED(hr))
                {
                   bSendSample = TRUE;
                    spOutputSample.Attach(spDecodedSample.Detach());
                }
                else if (hr != MF_E_TRANSFORM_NEED_MORE_INPUT)
                {
                    SetAsyncStatus(hr);
                    DMFTCHECKHR_GOTO(hr, done);
                }
               hr = S_OK;
            }
            break;
            default:
                // All other events are simply discarded.
                break;
            }

        }
    }
    else
    {
        // Synchronous mode. We only need it for processoutputs
        ComPtr<IMFSample> spDecodedSample;
        DMFTCHECKHR_GOTO(GetAsyncStatus(), done);
        DMFTCHECKNULL_GOTO(pResult, done, E_UNEXPECTED);
        DMFTCHECKHR_GOTO(pResult->GetStatus(), done);
        m_Lock.Unlock();
        hr = ProcessOutputSync(spDecodedSample.GetAddressOf());
        m_Lock.Lock();
        if (SUCCEEDED(hr))
        {
            // send the sample on its way
            bSendSample = TRUE;
            spOutputSample.Attach(spDecodedSample.Detach());
        }
        else if (hr != MF_E_TRANSFORM_NEED_MORE_INPUT)
        {
            SetAsyncStatus(hr);
        }
        hr = S_OK;
    }
done:
    if(SUCCEEDED(hr))
    {
        if (bSendSample && spOutputSample.Get())
        {
       
            if (m_spObjectWrapped)
            {
                hr = m_spObjectWrapped->PassThrough(spOutputSample.Get());
                spOutputSample = nullptr;
            }
        }
        //
        // Keep listening for events
        //
        if (m_fAsyncMFT)
        {
            spEventGenerator->BeginGetEvent(m_asyncCallback.Get(), spEventGenerator.Get());
        }
    }
    if (FAILED(hr))
    {
            SetAsyncStatus(hr);
    }
    return hr;
}

//
// @@@@README If the Allocator is created for a decoder and the pipeline
// is holding on to the buffers, could be the xvp then this function will error
// out the pipeline. you should see the ALLOCATOR_E_EMPTY error pop up
//
HRESULT CDecoderTee::GetSample( IMFSample** ppSample )
{
    if (!m_bProducesSamples && m_spPrivateAllocator.Get())
    {
        return m_spPrivateAllocator->AllocateSample(ppSample);
    }
    return S_OK;
}

HRESULT CDecoderTee::ConfigRealTimeMFT( _In_ IMFTransform* pTransform )
{
    DWORD dwTaskID = 0;
    ComPtr<IMFRealTimeClientEx> spRTClientEx;

    (void)MFGetWorkQueueMMCSSTaskId(m_dwQueueId, &dwTaskID);

    // If MFT supports IMFRealTimeClientEx/IMFRealTimeClient, set workqueue on it.
    (void)pTransform->QueryInterface(IID_PPV_ARGS(&spRTClientEx));
    if (spRTClientEx != nullptr)
    {
        (void)spRTClientEx->SetWorkQueueEx(m_dwCameraStreamWorkQueueId, 0);
        (void)spRTClientEx->RegisterThreadsEx(&dwTaskID, L"Capture", 0);
    }
    else
    {
        ComPtr<IMFRealTimeClient> spRTClient;

        (void)pTransform->QueryInterface(IID_PPV_ARGS(&spRTClient));
        if (spRTClient != nullptr)
        {
            (void)spRTClient->SetWorkQueue(m_dwCameraStreamWorkQueueId);
            (void)spRTClient->RegisterThreads(dwTaskID, L"Capture");
        }
    }

    return S_OK;
}

HRESULT CDecoderTee::ProcessFormatChange()
{
    HRESULT hr = S_OK;
    ComPtr<IMFMediaType> spDecoderOutputMediaType;
    ComPtr<IMFTransform> spTransform = Transform();

    DMFTCHECKNULL_GOTO(spTransform, done, E_UNEXPECTED);

    if (FAILED(spTransform->MFTSetOutputType(m_dwMFTOutputId, m_pOutputMediaType.Get(), 0)))
    {
        GUID guidPreviousSubType = GUID_NULL;
        //
        // @@@@ Readme: An XVP will be needed here as the decoder doesn't like the output media type
        // Also note, The platform doesn't support dynamic media type changes from the stream coming from the
        // source.
        //
        ComPtr<CVideoProcTee> spXvpTee;
        DMFTCHECKHR_GOTO(m_pOutputMediaType->GetGUID(MF_MT_SUBTYPE, &guidPreviousSubType), done);

        for (DWORD i = 0; ; i++)
        {
            GUID guidSubType = GUID_NULL;
          
            hr = spTransform->MFTGetOutputAvailableType(m_dwMFTOutputId, i, spDecoderOutputMediaType.GetAddressOf());
            if (MF_E_NO_MORE_TYPES == hr)
            {
                hr = S_OK;
                break;
            }
            DMFTCHECKHR_GOTO(hr, done);
            DMFTCHECKHR_GOTO(spDecoderOutputMediaType->GetGUID(MF_MT_SUBTYPE, &guidSubType), done);
          
            if (IsEqualGUID(guidSubType, guidPreviousSubType))
            {
                break;
            }
            spDecoderOutputMediaType = nullptr;
        }
        DMFTCHECKNULL_GOTO(spDecoderOutputMediaType.Get(), done, MF_E_INVALIDMEDIATYPE);

        DMFTCHECKHR_GOTO(spTransform->MFTSetOutputType(m_dwMFTOutputId, spDecoderOutputMediaType.Get(), 0), done);
        //
        // Create the XVP and insert it into the chain manually. set the output to the mediatype requested by the platform
        //
        if (m_bXvpAdded && m_spXvp.Get())
        {
            // The XVP was already created. change the xvp to handle format change
            spXvpTee = m_spXvp;
        }
        else
        {
            spXvpTee = new (std::nothrow) CXvptee(m_spObjectWrapped.Get(), m_streamCategory);
            m_spXvp = spXvpTee;
        }
        DMFTCHECKNULL_GOTO(spXvpTee.Get(), done, E_OUTOFMEMORY);
        if(m_hwMFT)
        {
            (VOID)spXvpTee->SetD3DManager(m_spDeviceManagerUnk.Get());
        }
        DMFTCHECKHR_GOTO(spXvpTee->SetMediaTypes(spDecoderOutputMediaType.Get(), m_pOutputMediaType.Get()), done);
        m_spObjectWrapped = spXvpTee;
        
        spDecoderOutputMediaType = nullptr;

        DMFTCHECKHR_GOTO(Transform()->MFTGetOutputCurrentType(m_dwMFTOutputId, spDecoderOutputMediaType.GetAddressOf()), done);

        m_pOutputMediaType = spDecoderOutputMediaType;

        //Recreate the Allocator
        DMFTCHECKHR_GOTO(CreateAllocator(), done);
        m_bXvpAdded = TRUE;
    }
done:
    return hr;
}

HRESULT CDecoderTee::ConfigDecoder(_In_ IMFTransform* pTransform, _In_ GUID guidSubType )
{
    HRESULT hr = S_OK;
    ComPtr<IMFAttributes> spMFTAttributes;
    ComPtr<IMFMediaType> spMediaType;
    GUID guidMajorType;
    GUID guidSubtype;
    DWORD dwMediaTypeIndex = 0;
    DWORD dwFlags = 0;
    ComPtr<IMFDXGIDeviceManager> spDxgiManager;
    DWORD dwDesiredFlags = MF_MEDIATYPE_EQUAL_MAJOR_TYPES | MF_MEDIATYPE_EQUAL_FORMAT_TYPES | MF_MEDIATYPE_EQUAL_FORMAT_DATA;

    UNREFERENCED_PARAMETER(guidSubType);
    DMFTCHECKNULL_GOTO(pTransform, done, E_INVALIDARG);

    if (SUCCEEDED(pTransform->GetAttributes(spMFTAttributes.GetAddressOf())) &&
        (nullptr != spMFTAttributes))
    {
        m_fAsyncMFT = (BOOL)MFGetAttributeUINT32(spMFTAttributes.Get(), MF_TRANSFORM_ASYNC, FALSE);
        if (m_fAsyncMFT)
        {
            // if async MFT, unlock the Async MFT functionality
            DMFTCHECKHR_GOTO(spMFTAttributes->SetUINT32(MF_TRANSFORM_ASYNC_UNLOCK, TRUE), done);
        }

        if ((MFGetAttributeUINT32(spMFTAttributes.Get(), MF_SA_D3D11_AWARE, FALSE) != 0) ||
            (MFGetAttributeUINT32(spMFTAttributes.Get(), MF_SA_D3D_AWARE, FALSE) != 0))
        {
            m_D3daware = TRUE;
        }
    }
    if (IsEqualCLSID(guidSubType, MFVideoFormat_H264))
    {
        // H264 specific declarations
        DMFTCHECKHR_GOTO(spMFTAttributes->SetUINT32(CODECAPI_AVLowLatencyMode, TRUE), done);
    }
    (void)ConfigRealTimeMFT(pTransform);
    (void)pTransform->MFTGetStreamIDs(1, &m_dwMFTInputId, 1, &m_dwMFTOutputId);

    DMFTCHECKHR_GOTO(m_pOutputMediaType->GetMajorType(&guidMajorType), done);
    DMFTCHECKHR_GOTO(m_pOutputMediaType->GetGUID(MF_MT_SUBTYPE, &guidSubtype), done);

    if (m_hwMFT && m_spDeviceManagerUnk.Get())
    {
        DMFTCHECKHR_GOTO(m_spDeviceManagerUnk.As(&spDxgiManager), done);
        if (m_D3daware && SUCCEEDED(IsDXFormatSupported(spDxgiManager.Get(), guidSubtype, nullptr, nullptr)))
        {
            // Set the DXGI Manager here, before the input type is set
            DMFTCHECKHR_GOTO(pTransform->MFTProcessMessage(MFT_MESSAGE_SET_D3D_MANAGER, (ULONG_PTR)m_spDeviceManagerUnk.Get()), done);
        }
    }
    DMFTCHECKHR_GOTO(pTransform->MFTSetInputType(m_dwMFTInputId, m_pInputMediaType.Get(), 0), done);

    // Find a matching output mediatype that has the same major/Subtype as pOutputType

    while (SUCCEEDED(pTransform->MFTGetOutputAvailableType(m_dwMFTOutputId, dwMediaTypeIndex, &spMediaType)))
    {
        GUID guidMajorType2;
        GUID guidSubtype2;

        if (SUCCEEDED(spMediaType->GetMajorType(&guidMajorType2)) &&
            IsEqualGUID(guidMajorType, guidMajorType2) &&
            SUCCEEDED(spMediaType->GetGUID(MF_MT_SUBTYPE, &guidSubtype2)) &&
            IsEqualGUID(guidSubtype, guidSubtype2))
        {
            // Found the same major/subtype
            break;
        }

        spMediaType = nullptr;
        dwMediaTypeIndex++;
    }
    // If cannot find a matchig mediatype, bail out.
    DMFTCHECKNULL_GOTO(spMediaType.Get(), done, MF_E_INVALIDMEDIATYPE);

    hr = spMediaType->IsEqual(m_pOutputMediaType.Get(), &dwFlags);
    if ((S_OK == hr) ||
        (hr == S_FALSE && ((dwFlags & dwDesiredFlags) == dwDesiredFlags)))
    {
        // Try to set output type on the MJPG decoder.
        DMFTCHECKHR_GOTO(pTransform->MFTSetOutputType(m_dwMFTOutputId, spMediaType.Get(), 0), done);
    }
    else
    {
        // Set the media type and also create an XVP to manage the conversion
        DMFTCHECKHR_GOTO(pTransform->MFTSetOutputType(m_dwMFTOutputId, spMediaType.Get(), 0), done);
        ComPtr<CXvptee> spXvpTee = new (std::nothrow) CXvptee(m_spObjectWrapped.Get(), m_streamCategory);
        DMFTCHECKNULL_GOTO(spXvpTee.Get(), done, E_OUTOFMEMORY);
        if (m_hwMFT)
        {
            (VOID)spXvpTee->SetD3DManager(m_spDeviceManagerUnk.Get());
        }
        DMFTCHECKHR_GOTO(spXvpTee->SetMediaTypes(spMediaType.Get(), m_pOutputMediaType.Get()), done);
        m_spObjectWrapped = spXvpTee;
        m_pOutputMediaType = spMediaType;
        //Recreate the Allocator
        m_bXvpAdded = TRUE;
    }
done:
    if (FAILED(hr))
    {
        m_fAsyncMFT = FALSE;
        m_D3daware  = FALSE;
    }
    return hr;
}


HRESULT CDecoderTee::ProcessOutputSync(_COM_Outptr_opt_ IMFSample** ppSample)
{
    HRESULT hr = S_OK;
    ComPtr<IMFSample> spOutputSample;
    DWORD dwStatus = 0;
    MFT_OUTPUT_DATA_BUFFER   opDataBuffer = {};
    ComPtr<IMFTransform> spTransform = Transform();

    DMFTCHECKNULL_GOTO(ppSample, done, E_INVALIDARG);
    DMFTCHECKNULL_GOTO(spTransform, done, E_UNEXPECTED);
    *ppSample = nullptr;

    // Setup the output buffer
    DMFTCHECKHR_GOTO(GetSample(spOutputSample.ReleaseAndGetAddressOf()), done); // Will fail if allocator empty.. keep up
    opDataBuffer.dwStreamID = m_dwMFTOutputId;
    opDataBuffer.pSample    = spOutputSample.Detach();
    hr = spTransform->MFTProcessOutput(0, 1, &opDataBuffer, &dwStatus);
    if (hr == MF_E_TRANSFORM_NEED_MORE_INPUT)
    {
        // Feed it more input, next time
        SAFE_RELEASE(opDataBuffer.pSample);
        goto done;
    } else if ( hr == MF_E_TRANSFORM_STREAM_CHANGE )
    {
        SAFE_RELEASE(opDataBuffer.pSample)
        DMFTCHECKHR_GOTO(ProcessFormatChange(), done);
        // try feeding it a new sample again
        DMFTCHECKHR_GOTO(GetSample(spOutputSample.ReleaseAndGetAddressOf()),done);
        opDataBuffer.dwStreamID = m_dwMFTOutputId;
        opDataBuffer.dwStatus = 0;
        opDataBuffer.pSample = spOutputSample.Detach();

        hr = spTransform->MFTProcessOutput(0, 1, &opDataBuffer, &dwStatus);
        if (FAILED(hr))
        {
            // need more input
            SAFE_RELEASE(opDataBuffer.pSample);
            goto done;
        }
    }
    if (SUCCEEDED(hr))
    {
        spOutputSample.Attach(opDataBuffer.pSample);
        *ppSample = spOutputSample.Detach();
    }
 done:
    return hr;
}

VOID CDecoderTee::ShutdownTee()
{
    CAutoLock lock(m_Lock);
    SetAsyncStatus(MF_E_SHUTDOWN);
    if (m_asyncCallback.Get())
    {
        // Break the circular reference between the
        // internal async callback and the decoder
        m_asyncCallback->Shutdown();
    }
}

#ifdef MF_DEVICEMFT_ADD_GRAYSCALER_
CGrayTee::CGrayTee(_In_ Ctee *tee) : CWrapTee(tee),m_transformfn(nullptr)
{
}
STDMETHODIMP CGrayTee::Do(_In_ IMFSample *pSample, _Outptr_ IMFSample** ppOutSample)
{
    HRESULT                 hr = S_OK;
    ComPtr<IMFSample>       spOutputSample;
    ComPtr <IMFMediaBuffer> spMediaBufInput, spMediaBufOutput;
    LONG                    lDefaultStride = 0,    lSrcStride = 0,lDestStride = 0;
    GUID                    guidOutputSubType = GUID_NULL;
    LONGLONG                hnsDuration, hnsTime = 0;
    DWORD dwTotalLength = 0;
    BYTE                    *pDest = NULL, *pSrc = NULL;
    DWORD                    cbBuffer = 0;
    ComPtr<IMFMediaType>    spMediaType = getOutMediaType();

    DMFTCHECKNULL_GOTO(ppOutSample, done, E_INVALIDARG);
    DMFTCHECKNULL_GOTO(pSample, done, E_INVALIDARG);

    DMFTCHECKHR_GOTO(pSample->GetTotalLength(&dwTotalLength), done);
    *ppOutSample = nullptr;
    DMFTCHECKHR_GOTO(pSample->ConvertToContiguousBuffer(spMediaBufInput.GetAddressOf()), done);
    
    {
        VideoBufferLock inputLock(spMediaBufInput.Get());

        DMFTCHECKHR_GOTO(spMediaType->GetGUID(MF_MT_SUBTYPE, &guidOutputSubType), done);
        DMFTCHECKHR_GOTO(MFGetStrideForBitmapInfoHeader(guidOutputSubType.Data1, m_rect.right, &lDefaultStride), done);
        DMFTCHECKHR_GOTO(MFCreateSample(spOutputSample.GetAddressOf()), done);
        DMFTCHECKHR_GOTO(pSample->CopyAllItems(spOutputSample.Get()), done);
        DMFTCHECKHR_GOTO(inputLock.LockBuffer(lDefaultStride, m_rect.bottom, &pSrc, &lSrcStride, &cbBuffer), done);
        dwTotalLength = max(dwTotalLength, (DWORD)(abs(lSrcStride*m_rect.bottom)));
        // Create the MediaBuffer for the new sample
        hr = MFCreate2DMediaBuffer(
            m_rect.right,
            m_rect.bottom,
            guidOutputSubType.Data1,
            (lSrcStride < 0), // top-down buffer (DX compatible)
            &spMediaBufOutput);
        if (FAILED(hr))
        {
            spMediaBufOutput = nullptr;
            DMFTCHECKHR_GOTO(MFCreateAlignedMemoryBuffer(
                dwTotalLength,
                MF_1_BYTE_ALIGNMENT,
                &spMediaBufOutput), done);
        }
        // Add the media buffer to the output sample
        DMFTCHECKHR_GOTO(spOutputSample->AddBuffer(spMediaBufOutput.Get()), done);
        {
            VideoBufferLock outputLock(spMediaBufOutput.Get());
            DMFTCHECKHR_GOTO(outputLock.LockBuffer(lDefaultStride, m_rect.bottom, &pDest, &lDestStride, &cbBuffer, FALSE), done);
            if (m_transformfn)
            {
                m_transformfn(m_rect, pDest, lDestStride, pSrc, lSrcStride, m_rect.right, m_rect.bottom);
            }
        }
        if (SUCCEEDED(pSample->GetSampleDuration(&hnsDuration)))
        {
            DMFTCHECKHR_GOTO(spOutputSample->SetSampleDuration(hnsDuration), done);
        }
        if (SUCCEEDED(pSample->GetSampleTime(&hnsTime)))
        {
            DMFTCHECKHR_GOTO(spOutputSample->SetSampleTime(hnsTime), done);
        }
    }
    if (spOutputSample.Get())
    {
        *ppOutSample = spOutputSample.Detach();
    }
done:
    return hr;
}

HRESULT CGrayTee::Configure(
    _In_ IMFMediaType* pInMediaType,
    _In_ IMFMediaType* pOutMediaType,
    _In_opt_ IMFTransform** ppTransform)
{
    HRESULT hr = S_OK;
    DWORD dwFlags = 0;
    UINT32 ulWidth = 0, ulHeight = 0;
    GUID   guidOutputSubType = GUID_NULL;
    UNREFERENCED_PARAMETER(ppTransform);
    DMFTCHECKNULL_GOTO(pInMediaType, done, E_INVALIDARG);
    DMFTCHECKNULL_GOTO(pOutMediaType, done, E_INVALIDARG);
    DMFTCHECKHR_GOTO((pInMediaType->IsEqual(pOutMediaType, &dwFlags)), done);
    DMFTCHECKHR_GOTO(pInMediaType->GetGUID(MF_MT_SUBTYPE, &guidOutputSubType), done);
    if (IsEqualCLSID(guidOutputSubType, MFVideoFormat_YUY2))
    {
        m_transformfn = TransformImage_YUY2;
    }
    else if (IsEqualCLSID(guidOutputSubType, MFVideoFormat_UYVY))
    {
        m_transformfn = TransformImage_UYVY;
    }
    else if (IsEqualCLSID(guidOutputSubType, MFVideoFormat_NV12))
    {
        m_transformfn = TransformImage_NV12;
    }
    else if (IsEqualCLSID(guidOutputSubType, MFVideoFormat_RGB32))
    {
        m_transformfn = TransformImage_RGB32;
    }
    DMFTCHECKHR_GOTO(MFGetAttributeSize(pInMediaType, MF_MT_FRAME_SIZE, &ulWidth, &ulHeight), done);
    m_rect = { 0, 0, (LONG)ulWidth, (LONG)ulHeight };
done:
    return hr;
}
#endif

// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@//
// @@@@ README: Sample copy related functions below
//
// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@//

CSampleCopytee::CSampleCopytee(_In_ Ctee *tee, GUID category
    , IMFVideoSampleAllocator* sampleAllocator
) :
    CVideoProcTee(tee, category
        , sampleAllocator
    )
{

}

CSampleCopytee::~CSampleCopytee()
{
    (VOID)StopStreaming();
    m_spDeviceManagerUnk = nullptr;
}

HRESULT CSampleCopytee::StartStreaming()
{
    CAutoLock lock(m_Lock);
    RETURN_IF_FAILED(Transform()->MFTProcessMessage(MFT_MESSAGE_NOTIFY_BEGIN_STREAMING, 0)); 
    RETURN_IF_FAILED(Transform()->MFTProcessMessage(MFT_MESSAGE_NOTIFY_START_OF_STREAM, 0));

    return S_OK;
}

HRESULT CSampleCopytee::StopStreaming()
{
    CAutoLock lock(m_Lock);
    SetAsyncStatus(MF_E_SHUTDOWN);
    RETURN_IF_FAILED(Transform()->MFTProcessMessage(MFT_MESSAGE_COMMAND_FLUSH, 0)); // Flush the stream
    RETURN_IF_FAILED(Transform()->MFTProcessMessage(MFT_MESSAGE_NOTIFY_END_OF_STREAM, 0)); // Notify end of stream
    RETURN_IF_FAILED(Transform()->MFTProcessMessage(MFT_MESSAGE_NOTIFY_END_STREAMING, 0)); // Notify end of streaming

    return S_OK;
}

STDMETHODIMP CSampleCopytee::Do(_In_ IMFSample *pSample, _Outptr_ IMFSample** ppOutSample)
{

    HRESULT hr = S_OK;
    MFT_OUTPUT_DATA_BUFFER   outputSample;
    ComPtr <IMFSample>       spXVPOutputSample;
    DWORD dwStatus = 0;
    CAutoLock lock(m_Lock);

    DMFTRACE(DMFT_GENERAL, TRACE_LEVEL_INFORMATION, "%!FUNC! Processing Sample =%p", pSample);
    RETURN_HR_IF_NULL(E_INVALIDARG, ppOutSample);

    *ppOutSample = nullptr;
    RETURN_IF_FAILED(GetAsyncStatus());
    RETURN_IF_FAILED(Transform()->MFTProcessInput(0, pSample, 0));

    outputSample.dwStreamID = 0;
    outputSample.pSample = NULL;
#if ((defined NTDDI_WIN10_VB) && (NTDDI_VERSION >= NTDDI_WIN10_VB))
    if (m_spDefaultAllocator)
    {
        RETURN_IF_FAILED(m_spDefaultAllocator->AllocateSample(&outputSample.pSample)); 
    }
    else
#endif // ((defined NTDDI_WIN10_VB) && (NTDDI_VERSION >= NTDDI_WIN10_VB))
    {
        RETURN_IF_FAILED(m_spPrivateAllocator->AllocateSample(&outputSample.pSample));
    }


    hr = Transform()->MFTProcessOutput(0, 1, &outputSample, &dwStatus);

    if (SUCCEEDED(hr) && pSample)
    {
        spXVPOutputSample.Attach(outputSample.pSample);
    }
    else
    {
        SAFE_RELEASE(outputSample.pSample);
    }

    if (spXVPOutputSample.Get())
    {
        *ppOutSample = spXVPOutputSample.Detach();
    }

    return hr;
}

STDMETHODIMP CSampleCopytee::Configure(
    _In_opt_ IMFMediaType* inMediaType,
    _In_opt_ IMFMediaType* outMediaType,
    _Outptr_ IMFTransform **ppTransform
)
{
    RETURN_HR_IF_NULL(E_INVALIDARG, ppTransform);

    CMediaTypePrinter inType(inMediaType);
    CMediaTypePrinter outType(outMediaType);
    DMFTRACE(DMFT_GENERAL, TRACE_LEVEL_INFORMATION, "%!FUNC! Input MediaType %s", inType.ToString());
    DMFTRACE(DMFT_GENERAL, TRACE_LEVEL_INFORMATION, "%!FUNC! Output MediaType %s", outType.ToString());

    RETURN_IF_FAILED(MFCreateSampleCopierMFT(ppTransform));
    RETURN_IF_FAILED(CreateAllocator());

    if (m_fSetD3DManager)
    {
        RETURN_IF_FAILED((*ppTransform)->MFTProcessMessage(MFT_MESSAGE_SET_D3D_MANAGER, reinterpret_cast<UINT_PTR>(m_spDeviceManagerUnk.Get())));
    }

    return S_OK;
}


/*++
Descrtiption:
    Handles events sent by the pipeline
 --*/
HRESULT CDMFTEventHandler::KSEvent(
    _In_reads_bytes_(ulEventLength) PKSEVENT pEvent,
    _In_ ULONG ulEventLength,
    _Inout_updates_bytes_opt_(ulDataLength) LPVOID pEventData,
    _In_ ULONG ulDataLength,
    _Inout_ ULONG* pBytesReturned)
{
    HRESULT hr = S_OK;
    UNREFERENCED_PARAMETER(ulDataLength);
    UNREFERENCED_PARAMETER(pBytesReturned);

    if (pEvent == nullptr)
    {
        //
        // We can have an Event Reset here
        //
        if (pEventData == nullptr)
        {
            //
            // Invalid input. KSEVENT and KSEVENTDATA cannot both be null
            //
            DMFTCHECKHR_GOTO(E_INVALIDARG, done);
        }
        //
        // The way KSevents work are, KSEVENT and KSEVENTDATA both have to be non null for events to be Set
        // on the driver. If the pipeline wishes to reset the event we pass the KSEVENT structure as NULL and
        // the KSEVENTDATA to be nonnull.
        //   ********** EVENT RESET here since KSEVENT is null and KSEVENTDATA is not**********
        // We will never get Event Reset for One shot events
        // Search the list of regular events to find the KSEVENTDATA pointer and then free the entry  
        //
        for ( vector<PDMFTEventEntry>::iterator it = m_RegularEventList.begin(); it != m_RegularEventList.end(); ++it )
        {
            PDMFTEventEntry pEntry = nullptr;
            pEntry = *it;
            DMFTCHECKNULL_GOTO(pEntry, done, E_UNEXPECTED); // We should never see a null data structure entry 
            if (pEntry->m_pEventData == pEventData)
            {
                it = m_RegularEventList.erase(it);
                delete(pEntry); // Delete the entry!
                goto done;
            }
        }

        DMFTCHECKHR_GOTO(E_NOT_SET, done);
    }
    else
    {
        //
        // If we are here the pipeline wants to do the following things
        // 1) Store a regular event
        // 2) Store a oneshot event
        //
        HANDLE          evtHandle   = nullptr;
        PKSEVENT        pEvt        = pEvent;
        PKSEVENTDATA    pEvtdata    = reinterpret_cast<PKSEVENTDATA>(pEventData);

        if (ulEventLength < sizeof(KSEVENT))
        {
            DMFTCHECKNULL_GOTO(pBytesReturned, done, E_INVALIDARG);
            *pBytesReturned = sizeof(KSEVENT);
            DMFTCHECKHR_GOTO(HRESULT_FROM_WIN32(ERROR_MORE_DATA), done);
        }

        if (!pEvtdata)
        {
            //
            // Need the event data  to be present to service the call properly!
            //
            DMFTCHECKHR_GOTO(E_INVALIDARG, done);
        }

        if (pEvt->Flags & KSEVENT_TYPE_ENABLE) // Regular/ Manual Reset Event 
        {
            vector<PDMFTEventEntry>::iterator it = m_RegularEventList.begin();
            for (; it != m_RegularEventList.end(); it++)
            {
                PDMFTEventEntry pEntry = *it;
                DMFTCHECKNULL_GOTO(pEntry, done, E_UNEXPECTED); 
                if (pEntry->m_pEventData == pEventData)
                {
                    break;
                }
            }
            if (it != m_RegularEventList.end())
            {
                //
                // Duplicate entry found. 
                //
                DMFTCHECKHR_GOTO(E_NOT_VALID_STATE, done);
            }
        }
        else if (pEvt->Flags & KSEVENT_TYPE_ONESHOT)
        {
            ULONG ulEventCommand = pEvt->Id;
            hr = ExceptionBoundary([&]()
            {
                map<ULONG, HANDLE>::iterator it = m_OneShotEventMap.find(ulEventCommand);
                if (it != m_OneShotEventMap.end())
                {
                    //
                    // Duplicate entry found. 
                    //
                    hr = E_NOT_VALID_STATE;
                }
            });
            DMFTCHECKHR_GOTO(hr, done);
        }
        else
        {
            DMFTCHECKHR_GOTO(E_NOTIMPL, done);
        }
 
        //
        // Duplicate the handle
        //
        DMFTCHECKHR_GOTO(Dupe(pEvtdata->EventHandle.Event, &evtHandle),done);
        ULONG ulEventCommand = pEvt->Id;
        
        if (pEvt->Flags & KSEVENT_TYPE_ENABLE)
        {
            //  ************** REGULAR EVENT ****************************
            // Regualar event. When this needs to be cleqared we will geta  call with KSEVENT = null
            // and KSEVENTDATA with the same address as this call(pEvtdata). Hence store it
            // To set the event all we need is the EVENT id.
            //
            PDMFTEventEntry pEntry = new (std::nothrow) DMFTEventEntry(ulEventCommand, pEvtdata, evtHandle);
            DMFTCHECKNULL_GOTO(pEntry, done, E_OUTOFMEMORY);
            hr = ExceptionBoundary([&]()
            {
                (VOID)m_RegularEventList.push_back(pEntry);
            });
        }
        else if(pEvt->Flags & KSEVENT_TYPE_ONESHOT)
        {
            // *************** SINGLE SHOT EVENT ************************
            // Single Shot event store. Store the event Id and the Handle.
            //
            hr = ExceptionBoundary([&]()
            {
                (VOID)m_OneShotEventMap.insert(std::pair<ULONG, HANDLE>(ulEventCommand, evtHandle));
            });
            DMFTCHECKHR_GOTO(hr, done);
        }
    }

done:
    return hr;
}

/*++
Descrtiption:
 Used to set the One shot events sent by the pipeline.
 One shot events should be closed by the component after firing. The Pipeline
 will not send a reset or a clear for one shot events
--*/

HRESULT CDMFTEventHandler::SetOneShot( ULONG ulEventId )
{
    HRESULT hr = S_OK;
    hr = ExceptionBoundary([&]()
    {
        map<ULONG, HANDLE>::iterator it = m_OneShotEventMap.find(ulEventId);
        DMFTRACE(DMFT_GENERAL, TRACE_LEVEL_INFORMATION, "%!FUNC! trying to Set  OneShot  %d ", ulEventId);

        if (it != m_OneShotEventMap.end())
        {
            // Found the event
            HANDLE hOneShot = it->second;
            if (SetEvent(hOneShot))
            {
                DMFTRACE(DMFT_GENERAL, TRACE_LEVEL_INFORMATION, "%!FUNC! Setting  OneShot  %d Succeeded",
                    ulEventId);
                CloseHandle(hOneShot);
                m_OneShotEventMap.erase(it);
            }
            else
            {
                DMFTRACE(DMFT_GENERAL, TRACE_LEVEL_INFORMATION, "%!FUNC! Setting  OneShot  %d failed %x",
                    ulEventId,
                    HRESULT_FROM_WIN32(GetLastError()));
            }
        }
        else
        {
            hr = E_NOT_SET;
        }
    });
    DMFTCHECKHR_GOTO(hr, done);
done:
    DMFTRACE(DMFT_GENERAL, TRACE_LEVEL_INFORMATION, "%!FUNC! Setting  OneShot  %d failed %x",
        ulEventId,
        HRESULT_FROM_WIN32(GetLastError()));
    return hr;
}

/*++
Descrtiption:
    Used to set the Regular events sent by the pipeline.
    Regular events unlike the one shot events must be persisted
    until the pipeline explicitly needs the event to be unset.
--*/

HRESULT CDMFTEventHandler::SetRegularEvent(ULONG ulEventId)
{
    BOOL bEvtFound = FALSE;
    HRESULT hr = ExceptionBoundary([&]()
    {
        
        for (vector<PDMFTEventEntry>::iterator it = m_RegularEventList.begin(); it != m_RegularEventList.end(); ++it)
        {
            PDMFTEventEntry pEntry = *it;
            if (ulEventId == pEntry->m_ulEventId)
            {
                // Found the event
                bEvtFound = TRUE;
                if (!::SetEvent(pEntry->m_hHandle))
                {
                    hr = HRESULT_FROM_WIN32(GetLastError());
                    DMFTRACE(DMFT_GENERAL, TRACE_LEVEL_INFORMATION, "%!FUNC! Setting  Event  %d Failed 0x%x",
                        ulEventId, hr);
                }
                else
                {
                    DMFTRACE(DMFT_GENERAL, TRACE_LEVEL_INFORMATION, "%!FUNC! Setting  Event  %d s",
                        ulEventId);
                }
            }
        }
    });
    if (!bEvtFound)
    {
        DMFTCHECKHR_GOTO(E_NOT_SET, done);
    }
done:
    return hr;
}

/*++
Descrtiption:
    Duplicates the handle passed.
--*/

HRESULT CDMFTEventHandler::Dupe(_In_ HANDLE hEventHandle, _Outptr_ LPHANDLE lpTargetHandle)
{
    HRESULT hr = S_OK;
    DMFTCHECKNULL_GOTO(hEventHandle, done, E_INVALIDARG);
    DMFTCHECKNULL_GOTO(lpTargetHandle, done, E_INVALIDARG);
    //
    // Duplicate handle and store it for when it needs to be fired
    //
    if (!DuplicateHandle(GetCurrentProcess(), hEventHandle, 
        GetCurrentProcess(), lpTargetHandle, 
        0, 
        false,   
        DUPLICATE_SAME_ACCESS
        ))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        DMFTRACE(DMFT_GENERAL, TRACE_LEVEL_INFORMATION, "%!FUNC! Duplicating Event Handle failed %x",
            hr);
        DMFTCHECKHR_GOTO(hr, done);
    }
done:
    return hr;
}

/*++

Descrtiption:
 Cleans the event lists, both the Single shot and the Regular ones

--*/
HRESULT CDMFTEventHandler::Clear()
{
    HRESULT hr = S_OK;
    hr = ExceptionBoundary([&]()
    {
        map<ULONG, HANDLE>::iterator it = m_OneShotEventMap.begin();
        for (; it != m_OneShotEventMap.end(); ++it)
        {
            //
            // Delete the one shot entries. We should not see this path usually
            // as the pipeline will send a one shot event and set it soon. We 
            // should remore the entry then.
            //
            CloseHandle(it->second);
            it = m_OneShotEventMap.erase(it);
            if (it == m_OneShotEventMap.end())
            {
                // We've reached the end break
                break;
            }
        }
    });
    DMFTCHECKHR_GOTO(hr, done);
    hr = ExceptionBoundary([=]()
    {
        vector<PDMFTEventEntry>::iterator it2 = m_RegularEventList.begin();
        for (; it2 != m_RegularEventList.end(); ++it2)
        {
            //
            // Delete the entries. This will be traversed
            //
            PDMFTEventEntry pEntry = *it2;
            it2 = m_RegularEventList.erase(it2);
            SAFE_DELETE(pEntry);
            if (it2 == m_RegularEventList.end())
            {
                break;
            }
        }
    });
    DMFTCHECKHR_GOTO(hr, done);
done:
    return hr;
}

HRESULT CPinCreationFactory::CreatePin(_In_ ULONG ulInputStreamId, /* The Input stream Id*/
    _In_ ULONG ulOutStreamId, /*The output stream Id*/
    _In_ type_pin type, /*Input Pin or the ourput pin*/
    _Outptr_ CBasePin** ppPin, /*Output*/
    _In_ BOOL& isCustom)
{
    HRESULT hr = S_OK;
    ComPtr<IMFAttributes> spAttributes;
    GUID    streamCategory = GUID_NULL;
    DMFTCHECKNULL_GOTO(ppPin, done, E_INVALIDARG);
    *ppPin = nullptr;
    isCustom = FALSE;
    DMFTCHECKHR_GOTO(m_spDeviceTransform->Parent()->GetOutputStreamAttributes(ulInputStreamId, &spAttributes),done);
    if (type == DMFT_PIN_INPUT)
    {
        ComPtr<CInPin>  spInPin;
        DMFTCHECKHR_GOTO(spAttributes->GetGUID(MF_DEVICESTREAM_STREAM_CATEGORY, &streamCategory), done);
        // Create Cutom Pin
        if (IsEqualCLSID(streamCategory, AVSTREAM_CUSTOM_PIN_IMAGE))
        {
            spInPin = new (std::nothrow) CCustomPin(spAttributes.Get(), ulInputStreamId, static_cast<CMultipinMft*> (m_spDeviceTransform.Get()));
            isCustom = TRUE;
        }
        else
        {
#if defined MF_DEVICEMFT_ASYNCPIN_NEEDED
            spInPin = new (std::nothrow) CAsyncInPin(spAttributes.Get(), ulInputStreamId, m_spDeviceTransform.Get()); // Asynchronous PIn, if you need it
#else
            spInPin = new (std::nothrow) CInPin(spAttributes.Get(), ulInputStreamId, m_spDeviceTransform.Get());
#endif
        }
        DMFTCHECKNULL_GOTO(spInPin.Get(), done, E_OUTOFMEMORY);
        *ppPin = spInPin.Detach();

    }
    else if(type == DMFT_PIN_OUTPUT)
    {
        ComPtr<COutPin> spOutPin;
        ComPtr<IKsControl>  spKscontrol;
        ComPtr<CInPin>      spInPin;
        GUID                pinGuid = GUID_NULL;
        UINT32              uiFrameSourceType = 0;

        spInPin = static_cast<CInPin*>(m_spDeviceTransform->GetInPin(ulInputStreamId));              // Get the Input Pin connected to the Output pin
        DMFTCHECKNULL_GOTO(spInPin.Get(), done, E_INVALIDARG);
        DMFTCHECKHR_GOTO(spInPin.As(&spKscontrol), done);   // Grab the IKSControl off the input pin
        DMFTCHECKHR_GOTO(spInPin->GetGUID(MF_DEVICESTREAM_STREAM_CATEGORY, &pinGuid), done);         // Get the Stream Category. Advertise on the output pin

#if defined MF_DEVICEMFT_DECODING_MEDIATYPE_NEEDED
        spOutPin = new (std::nothrow) CTranslateOutPin(ulOutStreamId, m_spDeviceTransform.Get(), spKscontrol.Get());         // Create the output pin
#else
        spOutPin = new (std::nothrow) COutPin(ulOutStreamId, m_spDeviceTransform.Get(), spKscontrol.Get()
#if ((defined NTDDI_WIN10_VB) && (NTDDI_VERSION >= NTDDI_WIN10_VB))
            , MFSampleAllocatorUsage_DoesNotAllocate
#endif
        );         // Create the output pin
#endif
        DMFTCHECKNULL_GOTO(spOutPin.Get(), done, E_OUTOFMEMORY);
        
        DMFTCHECKHR_GOTO(spOutPin->SetGUID(MF_DEVICESTREAM_STREAM_CATEGORY, pinGuid), done);         // Advertise the Stream category to the Pipeline
        DMFTCHECKHR_GOTO(spOutPin->SetUINT32(MF_DEVICESTREAM_STREAM_ID, ulOutStreamId), done);       // Advertise the stream Id to the Pipeline
        //
        // @@@@ README
        // Note H264 pins are tagged MFFrameSourceTypes_Custom. Since we are decoding H264 if we enable decoding,
        // lets change it to color, because we are producing an uncompressed format type, hence change it to 
        //    MFFrameSourceTypes_Color, MFFrameSourceTypes_Infrared or MFFrameSourceTypes_Depth
        //
        if (SUCCEEDED(spInPin->GetUINT32(MF_DEVICESTREAM_ATTRIBUTE_FRAMESOURCE_TYPES, &uiFrameSourceType)))
        {
#if defined MF_DEVICEMFT_DECODING_MEDIATYPE_NEEDED
            uiFrameSourceType = (uiFrameSourceType == MFFrameSourceTypes_Custom) ? MFFrameSourceTypes_Color : uiFrameSourceType;
#endif
            DMFTCHECKHR_GOTO(spOutPin->SetUINT32(MF_DEVICESTREAM_ATTRIBUTE_FRAMESOURCE_TYPES, uiFrameSourceType),done);   // Copy over the Frame Source Type.
        }

#if defined (MF_DEVICEMFT_ALLOW_MFT0_LOAD) && defined (MFT_UNIQUE_METHOD_NAMES)
        //
        // If we wish to load MFT0 as well as Device MFT then we should be doing the following
        // Copy over the GUID attribute MF_DEVICESTREAM_EXTENSION_PLUGIN_CLSID from the input
        // pin to the output pin. This is because Device MFT is the new face of the filter now
        // and MFT0 will now get loaded for the output pins exposed from Device MFT rather than
        // DevProxy!
        //

        GUID        guidMFT0 = GUID_NULL;
        if (SUCCEEDED(spInPin->GetGUID(MF_DEVICESTREAM_EXTENSION_PLUGIN_CLSID, &guidMFT0)))
        {
            //
            // This stream has an MFT0 .. Attach the GUID to the Outpin pin attribute
            // The downstream will query this attribute  on the pins exposed from device MFT
            //
            DMFTRACE(DMFT_GENERAL, TRACE_LEVEL_INFORMATION, "%!FUNC! setting Mft0 guid on pin %d", ulOutStreamId);

            DMFTCHECKHR_GOTO(spOutPin->SetGUID(MF_DEVICESTREAM_EXTENSION_PLUGIN_CLSID, guidMFT0), done);

            DMFTCHECKHR_GOTO(spOutPin->SetUnknown(MF_DEVICESTREAM_EXTENSION_PLUGIN_CONNECTION_POINT,
                static_cast< IUnknown* >(static_cast < IKsControl * >(m_spDeviceTransform.Get()))), done);

        }
#endif
        *ppPin = spOutPin.Detach();
    }
    else
    {
        DMFTCHECKHR_GOTO(E_INVALIDARG, done);
    }

done:
    return hr;
}

HRESULT CheckPinType(_In_ IMFAttributes* pAttributes, _In_ GUID pinType, _Out_ PBOOL pResult)
{
    HRESULT hr = S_OK;
    GUID pinClsid = GUID_NULL;
   
    DMFTCHECKNULL_GOTO(pAttributes, done, E_INVALIDARG);
    DMFTCHECKNULL_GOTO(pResult, done, E_INVALIDARG);
    *pResult = FALSE;

    if (SUCCEEDED(pAttributes->GetGUID(MF_DEVICESTREAM_STREAM_CATEGORY, &pinClsid))
        && (IsEqualCLSID(pinClsid, pinType)))
    {
        *pResult = TRUE;
    }
done:
    return hr;
}

BOOL CheckImagePin( _In_ IMFAttributes* pAttributes, _Out_ PBOOL pbIsImagePin )
{
    return ((SUCCEEDED(CheckPinType(pAttributes, PINNAME_IMAGE, pbIsImagePin)) && pbIsImagePin) ||
        (SUCCEEDED(CheckPinType(pAttributes, PINNAME_VIDEO_STILL, pbIsImagePin)) && pbIsImagePin));
}

HRESULT CheckPreviewPin( _In_ IMFAttributes* pAttributes, _Out_ PBOOL pbIsPreviewPin)
{
    return CheckPinType(pAttributes, PINNAME_PREVIEW, pbIsPreviewPin);
}
