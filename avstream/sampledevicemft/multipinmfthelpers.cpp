#include "stdafx.h"
#include "common.h"
#include "multipinmfthelpers.h"
#include "multipinmft.h"
#include "basepin.h"
#include <wincodec.h>

#ifdef MF_WPP
#include "multipinmfthelpers.tmh"    //--REF_ANALYZER_DONT_REMOVE--
#endif

#define MF_DEVICEMFT_ASYNCPIN_NEEDED 1

class CMediaTypePrinter;
//
//Queue implementation
//

CPinQueue::CPinQueue( _In_ DWORD dwPinId )
:   m_teer(0),
    m_dwInPinId(dwPinId)
    /*
    Description
    dwPinId is the input pin Id to which this queue corresponds
    */
{
}
CPinQueue::~CPinQueue( )
{
}

/*++
Description:
    Insert sample into the list once we reach the open queue
--*/
STDMETHODIMP_(VOID) CPinQueue::InsertInternal( _In_ IMFSample *pSample )
{
    pSample->AddRef();
    HRESULT hr = ExceptionBoundary([&]()
    {
        m_sampleList.push_back(pSample);
    });

    if (FAILED(hr))
    {
        DMFTRACE(DMFT_GENERAL, TRACE_LEVEL_INFORMATION, "%!FUNC! exiting %x = %!HRESULT!", hr, hr);
        SAFE_RELEASE(pSample);
    }
}

STDMETHODIMP CPinQueue::Insert( _In_ IMFSample *pSample )
//
//m_teer is the wraptee.. this could be a null tee which is a passthrough, an xvp tee, which inserts an xvp into the queue
//
{
    return m_teer->PassThrough( pSample, this );
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
        *ppSample = m_sampleList.front();
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

STDMETHODIMP CPinQueue::RecreateTee( _In_  IMFMediaType *inMediatype,
    _In_ IMFMediaType *outMediatype,
    _In_opt_ IUnknown* punkManager )
{
    HRESULT hr = S_OK;
    MF_TRANSFORM_XVP_OPERATION operation = DeviceMftTransformXVPIllegal;
    
    DMFTCHECKNULL_GOTO(inMediatype, done, E_INVALIDARG);
    DMFTCHECKNULL_GOTO(outMediatype, done, E_INVALIDARG);

    SAFE_DELETE(m_teer);

    CNullTee *nulltee = new (std::nothrow) CNullTee();
    DMFTCHECKNULL_GOTO( nulltee, done, E_OUTOFMEMORY);

    DMFTCHECKHR_GOTO( CompareMediaTypesForXVP(inMediatype, outMediatype, &operation), done);

    if ( (operation != DeviceMftTransformXVPCurrent) && (operation!= DeviceMftTransformXVPIllegal) )
    {
        CXvptee* pXvptee = new (std::nothrow) CXvptee(nulltee);
        DMFTCHECKNULL_GOTO(pXvptee, done, E_OUTOFMEMORY);
        (void)pXvptee->SetD3DManager( punkManager );
        (void)pXvptee->SetMediaTypes( inMediatype, outMediatype );
        m_teer = dynamic_cast< Ctee* >( pXvptee );
    }
    else
    {
        m_teer = nulltee; /*A simple passthrough*/
    }

done:
    DMFTRACE( DMFT_GENERAL, TRACE_LEVEL_INFORMATION, "%!FUNC! exiting %x = %!HRESULT!", hr, hr );

    if (FAILED(hr))
    {
        if (m_teer){
            delete(m_teer);
            m_teer = NULL;
        }

    }
    return hr;
}
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
    DMFTCHECKNULL_GOTO(m_teer, done, E_UNEXPECTED);
    // Wrap the media type with Gray scale tee only if the input media type is a YUY2, UYVY or NV12
    DMFTCHECKHR_GOTO(inMediatype->GetGUID(MF_MT_SUBTYPE, &gInputSubType), done);
    if (IsEqualCLSID(gInputSubType, MFVideoFormat_NV12)
        ||IsEqualCLSID(gInputSubType, MFVideoFormat_YUY2)
        ||IsEqualCLSID(gInputSubType, MFVideoFormat_UYVY)
        ||IsEqualCLSID(gInputSubType, MFVideoFormat_RGB32))
    {
        CGrayTee *pTee = NULL;
        pTee = new CGrayTee(m_teer);
        DMFTCHECKHR_GOTO(pTee->SetMediaTypes(inMediatype, outMediatype), done);
        m_teer = dynamic_cast< Ctee* >(pTee);
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
STDMETHODIMP CNullTee::PassThrough( _In_ IMFSample *pSample, _In_ CPinQueue *que )
{
    HRESULT hr = S_OK;

    DMFTCHECKNULL_GOTO(pSample, done, S_OK); //No OP for a NULL Sample!!!
    (VOID)que->InsertInternal(pSample);

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
    HRESULT       hr = S_OK;
    IMFTransform *pTransform = nullptr;

    DMFTCHECKHR_GOTO(Configure( pInMediaType, pOutMediaType, &pTransform ),done);
    m_videoProcessor = pTransform;
    m_pInputMediaType  = pInMediaType;
    m_pOutputMediaType = pOutMediaType;

done:
    SAFE_RELEASE(pTransform);
    return hr;
}

/*++
Description:
This is used when an XVP is present in the Pin. If the path is a passthrough i.e.
the media type on the input and the output pins are the same then we will not see
this path traversed. This function feeds the sample to the XVP or the decoding Tee
--*/
STDMETHODIMP CWrapTee::PassThrough(_In_ IMFSample* pInSample, _In_ CPinQueue *pQue)
{
    HRESULT hr = S_OK;
    IMFSample* pOutSample = nullptr;

    DMFTCHECKHR_GOTO(Do(pInSample, &pOutSample),done);
    
    if (m_objectWrapped)
    {
        hr = m_objectWrapped->PassThrough(pOutSample, pQue);
    }

done:
    if (FAILED(hr))
    {
        DMFTRACE(DMFT_GENERAL, TRACE_LEVEL_INFORMATION, "%!FUNC! exiting %x = %!HRESULT!", hr, hr);
    }

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

If the DX manager is set on the Transform and we receieve a DX sample then we will
undertake the DX transform in the XVP else we will skip it and go the software way
--*/
STDMETHODIMP CXvptee::Do(_In_ IMFSample *pSample, _Outptr_ IMFSample** pOutSample)
{
    //
    //Since we are streaming in the software mode we will create a new sample
    //
    HRESULT                  hr = S_OK,pohr = S_OK;
    MFT_OUTPUT_DATA_BUFFER   outputSample;
    MFT_OUTPUT_STREAM_INFO   StreamInfo;
    ComPtr <IMFSample>       spXVPOutputSample;
    ComPtr <IMFMediaType>    spOutMediaType;
    ComPtr <IMFMediaBuffer>  spIMFMediaBuffer;
    GUID  guidOutputSubType = GUID_NULL;
    
    spOutMediaType.Attach(getOutMediaType());

    DMFTCHECKNULL_GOTO(pOutSample, done, E_INVALIDARG);

    DMFTCHECKHR_GOTO(spOutMediaType->GetGUID(MF_MT_SUBTYPE, &guidOutputSubType), done);
    
    BOOL isDx = false;

    if (SUCCEEDED(IsInputDxSample(pSample, &isDx)) && isDx)
    {
        DMFTRACE(DMFT_GENERAL, TRACE_LEVEL_INFORMATION, "%!FUNC! DX Sample sent to XVP %p", pSample);
    }
    
    outputSample.dwStreamID = 0;
    outputSample.pSample = NULL;

    if (!(isDx && m_spDeviceManagerUnk!=nullptr))
    {
        //
        //Create a new sample for software encoding
        //
        DMFTCHECKHR_GOTO(MFCreateSample(&spXVPOutputSample), done);

        DMFTCHECKHR_GOTO(pSample->CopyAllItems(spXVPOutputSample.Get()), done);

        DMFTCHECKHR_GOTO(Transform()->MFTGetOutputStreamInfo(0, &StreamInfo), done);

        if (!m_isOutPutImage)
        {
            DMFTCHECKHR_GOTO(MFGetAttributeSize(spOutMediaType.Get(), MF_MT_FRAME_SIZE, &m_uWidth, &m_uHeight), done);
            hr = MFCreate2DMediaBuffer(
                m_uWidth,
                m_uHeight,
                guidOutputSubType.Data1,
                FALSE, // top-down buffer (DX compatible)
                spIMFMediaBuffer.GetAddressOf());
            if (FAILED(hr))
            {
                DMFTCHECKHR_GOTO(MFCreateAlignedMemoryBuffer(
                    StreamInfo.cbSize,
                    StreamInfo.cbAlignment,
                    spIMFMediaBuffer.GetAddressOf()), done);
                DMFTRACE(DMFT_GENERAL, TRACE_LEVEL_INFORMATION, "%!FUNC! XVP Configured without DX, Created a 1D buffer");

            }
            else
            {
                DMFTRACE(DMFT_GENERAL, TRACE_LEVEL_INFORMATION, "%!FUNC! XVP Configured with DX, Created a 2D buffer");
            }
            DMFTCHECKHR_GOTO(spXVPOutputSample->AddBuffer(spIMFMediaBuffer.Get()), done);

            outputSample.pSample = spXVPOutputSample.Detach();
        }
    }
    //
    //Start streaming the xvp transform..
    //
    DMFTCHECKHR_GOTO(Transform()->MFTProcessMessage(MFT_MESSAGE_NOTIFY_BEGIN_STREAMING, 0), done); // ulParam set to zero
    DMFTCHECKHR_GOTO(Transform()->MFTProcessMessage(MFT_MESSAGE_NOTIFY_START_OF_STREAM, 0), done); // ulParam set to zero


    DMFTCHECKHR_GOTO(Transform()->MFTProcessInput(0, pSample, 0),done);

    DWORD dwStatus = 0;

    pohr = Transform()->MFTProcessOutput(0, 1, &outputSample, &dwStatus);

    if (FAILED(pohr))
    {
        DMFTRACE(DMFT_GENERAL, TRACE_LEVEL_INFORMATION, "%!FUNC! XVP ProcessOutput,failure %x sample =%p", pohr, &outputSample);
    }

    DMFTCHECKHR_GOTO(Transform()->MFTProcessMessage(MFT_MESSAGE_COMMAND_FLUSH, 0), done); // Flush the stream
    DMFTCHECKHR_GOTO(Transform()->MFTProcessMessage(MFT_MESSAGE_NOTIFY_END_OF_STREAM, 0), done); // Notify end of stream
    DMFTCHECKHR_GOTO(Transform()->MFTProcessMessage(MFT_MESSAGE_NOTIFY_END_STREAMING, 0), done); // Notify end of streaming

    spXVPOutputSample.Attach(outputSample.pSample);

    if ( SUCCEEDED(pohr) )
    {
        //Let us copy the timestamps
        LONGLONG hnsSampleDuration = 0;
        LONGLONG hnsSampleTime = 0;

        pSample->GetSampleDuration(&hnsSampleDuration);
        pSample->GetSampleTime(&hnsSampleTime);
        spXVPOutputSample->SetSampleDuration(hnsSampleDuration);
        spXVPOutputSample->SetSampleTime(hnsSampleTime);
    }

    *pOutSample = spXVPOutputSample.Detach();
    //
    //Release the Sample back to the pipeline
    //
    pSample->Release();

done:
    hr = FAILED(pohr) ? pohr : hr;
    return hr;
}

/*++
Description:
    Set the D3D Manager on the XVP
--*/
STDMETHODIMP_(VOID) CXvptee::SetD3DManager( _In_opt_ IUnknown* pUnk)
{
    m_spDeviceManagerUnk = pUnk;
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
    GUID         inMajorType  = GUID_NULL;
    GUID         outMajorType = GUID_NULL;
    bool         imageType = false;
    bool         optimized = false;
    bool         optimizedxvpneeded = false;
    IMFMediaType *pXvpOutputMediaType = nullptr;
    ComPtr<IMFAttributes> pXvpTransformAttributes = nullptr;
    UINT32      width = 0, height = 0;
    
    DMFTCHECKNULL_GOTO(ppTransform, done, E_INVALIDARG);

    m_isOutPutImage = false;
    m_isoptimizedPlanarInputOutput = false;

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



    DMFTCHECKHR_GOTO( inMediaType->GetMajorType( &inMajorType ),done );
    DMFTCHECKHR_GOTO( outMediaType->GetMajorType( &outMajorType ), done );
    //
    //Configuring DX Manager in SW mode. 
    //
    if ( !IsEqualGUID(MFMediaType_Video, outMajorType ) )
    {
        imageType = true;
        m_isOutPutImage = true;
        IsOptimizedPlanarVideoInputImageOutputPair(
            inMediaType,
            outMediaType,
            &optimized,
            &optimizedxvpneeded );
    }
    
    //
    //Disable frame rate conversion
    //
    DMFTCHECKHR_GOTO((*ppTransform)->GetAttributes(&pXvpTransformAttributes),done);
    pXvpTransformAttributes->SetUINT32(MF_XVP_DISABLE_FRC, TRUE);
    DMFTCHECKHR_GOTO( (*ppTransform)->MFTProcessMessage( MFT_MESSAGE_SET_D3D_MANAGER, NULL ), done );
    DMFTCHECKHR_GOTO((*ppTransform)->MFTSetInputType(0, inMediaType, 0), done);
    
    
    DMFTCHECKHR_GOTO(MFCreateMediaType(&pXvpOutputMediaType), done);
    DMFTCHECKHR_GOTO(outMediaType->CopyAllItems(pXvpOutputMediaType), done);
    DMFTCHECKHR_GOTO(pXvpOutputMediaType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video), done); //Video propcessor doesn't understand image types


    if ( imageType )
    {
        GUID guidSub;

        if ( !optimized )
        {
            DMFTCHECKHR_GOTO( inMediaType->GetGUID(MF_MT_SUBTYPE, &guidSub ), done );
            if ( IsEqualGUID( guidSub, MFVideoFormat_ARGB32 ) )
            {
                DMFTCHECKHR_GOTO(pXvpOutputMediaType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_ARGB32), done);
            }
            else
            {
                DMFTCHECKHR_GOTO(pXvpOutputMediaType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32), done);
            }
            DMFTCHECKHR_GOTO(MFGetAttributeSize(pXvpOutputMediaType, MF_MT_FRAME_SIZE, &width, &height), done);
            
            m_uWidth   = width;
            m_uHeight = height;

            DMFTCHECKHR_GOTO(pXvpOutputMediaType->SetUINT32(MF_MT_DEFAULT_STRIDE, width * 4), done); //So we always get a top down buffer

            DMFTCHECKHR_GOTO((*ppTransform)->MFTSetOutputType(0, pXvpOutputMediaType, 0), done);
        }
        else
        {
            //*** If we come here we know that the XVP is needed
            //This case should be for non planar and xvp needed cases which we will always assume
            LONG cStride = 0;
            //For now we will just assume that we need the xvp in all cases
            DMFTCHECKHR_GOTO( inMediaType->GetGUID(MF_MT_SUBTYPE, &guidSub ), done );

            DMFTCHECKHR_GOTO(pXvpOutputMediaType->SetGUID(MF_MT_SUBTYPE, guidSub), done);

            DMFTCHECKHR_GOTO(MFGetAttributeSize(pXvpOutputMediaType, MF_MT_FRAME_SIZE, &width, &height), done);
            
            DMFTCHECKHR_GOTO( MFGetStrideForBitmapInfoHeader( guidSub.Data1, width, &cStride ), done );

            DMFTCHECKHR_GOTO(pXvpOutputMediaType->SetUINT32(MF_MT_DEFAULT_STRIDE, cStride), done); //So we always get a top down buffer for Nv12/Yv12

            DMFTCHECKHR_GOTO(pXvpOutputMediaType->SetUINT32(MF_MT_VIDEO_NOMINAL_RANGE, MFNominalRange_0_255), done); //Always set nominal range to 0-255 to feed into a WIC encoder

            DMFTCHECKHR_GOTO((*ppTransform)->MFTSetOutputType(0, pXvpOutputMediaType, 0), done);
        }
    }
    else
    {
        DMFTCHECKHR_GOTO((*ppTransform)->MFTSetOutputType(0, outMediaType, 0), done);
    }
    if ((m_spDeviceManagerUnk != nullptr))
    {
        //
        //Set the D3D Manager on the XVP if present
        //
        DMFTCHECKHR_GOTO((*ppTransform)->MFTProcessMessage(MFT_MESSAGE_SET_D3D_MANAGER,
            reinterpret_cast<ULONG_PTR>(m_spDeviceManagerUnk.Get())), done);
    }
done:
    SAFE_RELEASE(pXvpOutputMediaType);
    return hr;

}


CXvptee::CXvptee( _In_ Ctee *tee) :
CWrapTee(tee),
    m_isOutPutImage(false),
    m_isoptimizedPlanarInputOutput(true),
    m_spDeviceManagerUnk(nullptr),
    m_uHeight(0),
    m_uWidth(0)

{

}


CXvptee::~CXvptee()
{
    m_spDeviceManagerUnk = nullptr;
}

CGrayTee::CGrayTee(_In_ Ctee *tee) : CWrapTee(tee),m_transformfn(nullptr)
{
}
STDMETHODIMP CGrayTee::Do(_In_ IMFSample *pSample, _Outptr_ IMFSample** ppOutSample)
{
    HRESULT                 hr = S_OK;
    ComPtr<IMFSample>       spOutputSample;
    ComPtr<IMFMediaType>    spMediaType = getOutMediaType();
    ComPtr <IMFMediaBuffer> spMediaBufInput, spMediaBufOutput;
    LONG                    lDefaultStride = 0,    lSrcStride = 0,lDestStride = 0;
    
    GUID                    guidOutputSubType = GUID_NULL;
    LONGLONG                hnsDuration, hnsTime = 0;
    DWORD dwTotalLength = 0;
    DMFTCHECKHR_GOTO(pSample->GetTotalLength(&dwTotalLength), done);

    DMFTCHECKNULL_GOTO(ppOutSample, done, E_INVALIDARG);
    DMFTCHECKNULL_GOTO(pSample, done, E_INVALIDARG);
    *ppOutSample = nullptr;
    DMFTCHECKHR_GOTO(pSample->ConvertToContiguousBuffer(spMediaBufInput.GetAddressOf()), done);
    DMFTCHECKHR_GOTO(getOutMediaType()->GetGUID(MF_MT_SUBTYPE, &guidOutputSubType), done);
    DMFTCHECKHR_GOTO(MFGetStrideForBitmapInfoHeader(guidOutputSubType.Data1, m_rect.right, &lDefaultStride), done);

    DMFTCHECKHR_GOTO(MFCreateSample(spOutputSample.GetAddressOf()), done);
    DMFTCHECKHR_GOTO(pSample->CopyAllItems(spOutputSample.Get()), done);
    // Create the MediaBuffer for the new sample
    hr = MFCreate2DMediaBuffer(
        m_rect.right,
        m_rect.bottom,
        guidOutputSubType.Data1,
        FALSE, // top-down buffer (DX compatible)
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
        BYTE                    *pDest = NULL, *pSrc = NULL;
        DWORD                    cbBuffer = 0;
        VideoBufferLock inputLock(spMediaBufInput.Get());
        VideoBufferLock outputLock(spMediaBufOutput.Get());
        DMFTCHECKHR_GOTO(inputLock.LockBuffer(lDefaultStride, m_rect.bottom, &pSrc, &lSrcStride,&cbBuffer), done);
        DMFTCHECKHR_GOTO(outputLock.LockBuffer(lDefaultStride, m_rect.bottom, &pDest, &lDestStride,&cbBuffer,FALSE), done);
        if (m_transformfn)
        {
            m_transformfn(m_rect, pDest, lDestStride, pSrc, lSrcStride, m_rect.right, m_rect.bottom);
        }
    }
    if (SUCCEEDED(pSample->GetSampleDuration(&hnsDuration)))
    {
        DMFTCHECKHR_GOTO(spOutputSample->SetSampleDuration(hnsDuration),done);
    }
    if (SUCCEEDED(pSample->GetSampleTime(&hnsTime)))
    {
        DMFTCHECKHR_GOTO(spOutputSample->SetSampleTime(hnsTime),done);
    }
    if (SUCCEEDED(spMediaBufInput->GetCurrentLength(&dwTotalLength)))
    {
        DMFTCHECKHR_GOTO(spMediaBufOutput->SetCurrentLength(dwTotalLength),done);
    }

    *ppOutSample = spOutputSample.Detach();
    pSample->Release();
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
                DMFTCHECKNULL_GOTO(pEntry, done, E_FAIL); 
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
            PDMFTEventEntry pEntry = new DMFTEventEntry(ulEventCommand, pEvtdata, evtHandle);
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
        CInPin  *pInPin = nullptr;
        DMFTCHECKHR_GOTO(spAttributes->GetGUID(MF_DEVICESTREAM_STREAM_CATEGORY, &streamCategory), done);
        // Create Cutom Pin
        if (IsEqualCLSID(streamCategory, AVSTREAM_CUSTOM_PIN_IMAGE))
        {
            pInPin = new CCustomPin(spAttributes.Get(), ulInputStreamId, static_cast<CMultipinMft*> (m_spDeviceTransform.Get()));
            isCustom = TRUE;
        }
        else
        {
#if defined MF_DEVICEMFT_ASYNCPIN_NEEDED
            pInPin = new CAsyncInPin(spAttributes.Get(), ulInputStreamId, m_spDeviceTransform.Get()); // Asynchronous PIn, if you need it
#else
            pInPin = new CInPin(spAttributes.Get(), ulInputStreamId, m_spDeviceTransform.Get());
#endif
        }
        DMFTCHECKNULL_GOTO(pInPin, done, E_OUTOFMEMORY);
        *ppPin = pInPin;

    }
    else if(type == DMFT_PIN_OUTPUT)
    {
        COutPin* pOutPin            = nullptr;
        ComPtr<IKsControl>  spKscontrol;
        CInPin*             pInPin  = nullptr;
        GUID                pinGuid = GUID_NULL;
        UINT32              uiFrameSourceType = 0;

        pInPin = static_cast<CInPin*>(m_spDeviceTransform->GetInPin(ulInputStreamId));              // Get the Input Pin connected to the Output pin
        DMFTCHECKNULL_GOTO(pInPin, done, E_INVALIDARG);
        DMFTCHECKHR_GOTO(pInPin->QueryInterface(IID_PPV_ARGS(spKscontrol.GetAddressOf())), done);   // Grab the IKSControl off the input pin
        DMFTCHECKHR_GOTO(pInPin->GetGUID(MF_DEVICESTREAM_STREAM_CATEGORY, &pinGuid), done);         // Get the Stream Category. Advertise on the output pin


        pOutPin = new COutPin(ulOutStreamId, m_spDeviceTransform.Get(), spKscontrol.Get());         // Create the output pin
        DMFTCHECKNULL_GOTO(pOutPin, done, E_OUTOFMEMORY);
        
        *ppPin = pOutPin;
        DMFTCHECKHR_GOTO(pOutPin->SetGUID(MF_DEVICESTREAM_STREAM_CATEGORY, pinGuid), done);         // Advertise the Stream category to the Pipeline
        DMFTCHECKHR_GOTO(pOutPin->SetUINT32(MF_DEVICESTREAM_STREAM_ID, ulOutStreamId), done);       // Advertise the stream Id to the Pipeline
        if (SUCCEEDED(pInPin->GetUINT32(MF_DEVICESTREAM_ATTRIBUTE_FRAMESOURCE_TYPES, &uiFrameSourceType)))
        {
            DMFTCHECKHR_GOTO(pOutPin->SetUINT32(MF_DEVICESTREAM_ATTRIBUTE_FRAMESOURCE_TYPES, uiFrameSourceType),done);   // Copy over the Frame Source Type.
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
        if (SUCCEEDED(pInPin->GetGUID(MF_DEVICESTREAM_EXTENSION_PLUGIN_CLSID, &guidMFT0)))
        {
            //
            // This stream has an MFT0 .. Attach the GUID to the Outpin pin attribute
            // The downstream will query this attribute  on the pins exposed from device MFT
            //
            DMFTRACE(DMFT_GENERAL, TRACE_LEVEL_INFORMATION, "%!FUNC! setting Mft0 guid on pin %d", ulOutStreamId);

            DMFTCHECKHR_GOTO(pOutPin->SetGUID(MF_DEVICESTREAM_EXTENSION_PLUGIN_CLSID, guidMFT0), done);

            DMFTCHECKHR_GOTO(pOutPin->SetUnknown(MF_DEVICESTREAM_EXTENSION_PLUGIN_CONNECTION_POINT,
                static_cast< IUnknown* >(static_cast < IKsControl * >(m_spDeviceTransform.Get()))), done);

        }
#endif
    }
    else
    {
        DMFTCHECKHR_GOTO(E_INVALIDARG, done);
    }

done:
    return hr;
}

HRESULT CheckImagePin( _In_ IMFAttributes* pAttributes, _Out_ PBOOL pbIsImagePin )
{
    HRESULT hr = S_OK;
    DMFTCHECKNULL_GOTO(pAttributes, done, E_INVALIDARG);
    DMFTCHECKNULL_GOTO(pbIsImagePin, done, E_INVALIDARG);
    {
        GUID pinClsid = GUID_NULL;
        *pbIsImagePin = FALSE;
        if (SUCCEEDED(pAttributes->GetGUID(MF_DEVICESTREAM_STREAM_CATEGORY, &pinClsid))
            && ((IsEqualCLSID(pinClsid, PINNAME_IMAGE)) || IsEqualCLSID(pinClsid, PINNAME_VIDEO_STILL)))
        {
            *pbIsImagePin = TRUE;
        }
    }
done:
    return hr;
}

