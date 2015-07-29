#include "stdafx.h"
#include "common.h"
#include "multipinmfthelpers.h"
#include "multipinmft.h"
#include "basepin.h"
#include <wincodec.h>

#ifdef MF_WPP
#include "multipinmfthelpers.tmh"
#endif
class CMediaTypePrinter;
//
//Queue implementation
//

CPinQueue::CPinQueue( _In_ DWORD _InpinId )
:   m_teer(0),
    m_discotinuity(0),
    m_sampleCount(0),
    m_dwInPinId(_InpinId)
    /*
    Description
    _InpinId is the input pin Id to which this queue corresponds
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
    m_sampleList.push_back( pSample );
}

STDMETHODIMP_(BOOL) CPinQueue::Insert( _In_ IMFSample *pSample )
//
//m_teer is the wraptee.. this could be a null tee which is a passthrough, an xvp tee, which inserts an xvp into the queue
//
{
    return SUCCEEDED( m_teer->PassThrough( pSample, this ) );
}


/*++
Description:
    This extracts the first sample from the queue. called from Pin's ProcessOutput 
--*/

STDMETHODIMP_(BOOL) CPinQueue::Remove( _Outptr_result_maybenull_ IMFSample **ppSample)
{
    HRESULT hr = S_OK;
    DMFTCHECKNULL_GOTO( ppSample, done,E_INVALIDARG );
    *ppSample = nullptr;

    if ( !m_sampleList.empty() )
    {
        *ppSample = m_sampleList.front();
    }

    DMFTCHECKNULL_GOTO( *ppSample, done, MF_E_TRANSFORM_NEED_MORE_INPUT );
    
    (*ppSample)->Release( );
    
    m_sampleList.erase( m_sampleList.begin() );
done:
    return SUCCEEDED( hr ) ? TRUE : FALSE;
}

/*++
Description:
    Empties the Queue. used by the flush
--*/
VOID CPinQueue::Clear( )
{
    IMFSample* pSample = nullptr;
    while ( !Empty() )
    {
        Remove( &pSample );
        SAFE_RELEASE( pSample );
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

//
//State implementation
//
CPinOpenState::CPinOpenState(CBasePin *_pin) 
:CPinState(DeviceStreamState_Run), m_pin(_pin)
{
   
}



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
    HRESULT                hr = S_OK,pohr = S_OK;
    MFT_OUTPUT_DATA_BUFFER outputSample;
    MFT_OUTPUT_STREAM_INFO StreamInfo;
    IMFSample*             spXVPOutputSample    = nullptr;
    IMFMediaType*          pOutMediaType        = nullptr;
    ComPtr <IMFMediaBuffer> spIMFMediaBuffer    = nullptr;
    GUID  guidOutputSubType = GUID_NULL;
    pOutMediaType = getOutMediaType();

    DMFTCHECKNULL_GOTO(pOutSample, done, E_INVALIDARG);

    DMFTCHECKHR_GOTO(pOutMediaType->GetGUID(MF_MT_SUBTYPE, &guidOutputSubType), done);
    
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

        DMFTCHECKHR_GOTO(pSample->CopyAllItems(spXVPOutputSample), done);

        outputSample.pSample = spXVPOutputSample;
        

        DMFTCHECKHR_GOTO(Transform()->MFTGetOutputStreamInfo(0, &StreamInfo), done);

        if (!m_isOutPutImage)
        {
            DMFTCHECKHR_GOTO(MFGetAttributeSize(pOutMediaType, MF_MT_FRAME_SIZE, &m_uWidth, &m_uHeight), done);
            hr = MFCreate2DMediaBuffer(
                m_uWidth,
                m_uHeight,
                guidOutputSubType.Data1,
                FALSE, // top-down buffer (DX compatible)
                &spIMFMediaBuffer);
            if (FAILED(hr))
            {
                spIMFMediaBuffer = nullptr;
                spXVPOutputSample->Release();

                DMFTCHECKHR_GOTO(MFCreateAlignedMemoryBuffer(
                    StreamInfo.cbSize,
                    StreamInfo.cbAlignment,
                    &spIMFMediaBuffer), done);
                DMFTRACE(DMFT_GENERAL, TRACE_LEVEL_INFORMATION, "%!FUNC! XVP Configured without DX, Created a 1D buffer");

            }
            else
            {
                DMFTRACE(DMFT_GENERAL, TRACE_LEVEL_INFORMATION, "%!FUNC! XVP Configured with DX, Created a 2D buffer");
            }
            DMFTCHECKHR_GOTO(spXVPOutputSample->AddBuffer(spIMFMediaBuffer.Get()), done);
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
        SAFERELEASE(spXVPOutputSample);
    }

    DMFTCHECKHR_GOTO(Transform()->MFTProcessMessage(MFT_MESSAGE_COMMAND_FLUSH, 0), done); // Flush the stream
    DMFTCHECKHR_GOTO(Transform()->MFTProcessMessage(MFT_MESSAGE_NOTIFY_END_OF_STREAM, 0), done); // Notify end of stream
    DMFTCHECKHR_GOTO(Transform()->MFTProcessMessage(MFT_MESSAGE_NOTIFY_END_STREAMING, 0), done); // Notify end of streaming

    spXVPOutputSample = outputSample.pSample;

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

    *pOutSample = spXVPOutputSample;
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
STDMETHODIMP_(VOID) CXvptee::SetD3DManager(IUnknown* pUnk)
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
    //Configuring DX Manager in SW mode. We will look to make it DX complaint in the next phase
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

//To be implemented..Not needed for the sample!!
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

STDMETHODIMP CDecoderTee::Do(_In_ IMFSample* pSample, _Out_ IMFSample **pOutSample )
{
    UNREFERENCED_PARAMETER(pSample);
    UNREFERENCED_PARAMETER(pOutSample);
    return S_OK;
}
STDMETHODIMP CDecoderTee::Configure( _In_opt_ IMFMediaType *inMediaType, _In_opt_ IMFMediaType *outMediaType, _Outptr_ IMFTransform** ppTransform)
{
    HRESULT hr              = S_OK;
    GUID    guidcategory    = GUID_NULL;
    BOOL    IsDecoder       = true;
    UINT32  unFlags         = 0;
    MFT_REGISTER_TYPE_INFO in;
    MFT_REGISTER_TYPE_INFO out;
    UINT32                 cActivates = 0;
    UINT32                 unMFTFlags = 0;
    ComPtr<IMFMediaType>  pInputMediaType   = nullptr;
    ComPtr<IMFMediaType>  pOutputMediaType  = nullptr;
    IMFActivate             **ppActivates   = nullptr; 
    DWORD dwInputStreams;
    DWORD dwOutputStreams;
    UNREFERENCED_PARAMETER(outMediaType);
    UNREFERENCED_PARAMETER(inMediaType);
    UNREFERENCED_PARAMETER(ppTransform);

    guidcategory = (IsDecoder)?MFT_CATEGORY_VIDEO_DECODER : MFT_CATEGORY_VIDEO_ENCODER;

    if (IsDecoder)
    {
        DMFTCHECKHR_GOTO(pInputMediaType->GetGUID(MF_MT_MAJOR_TYPE, &in.guidMajorType), done);
        DMFTCHECKHR_GOTO(pInputMediaType->GetGUID(MF_MT_SUBTYPE, &in.guidSubtype), done);
    }
    else
    {
        DMFTCHECKHR_GOTO(pOutputMediaType->GetGUID(MF_MT_MAJOR_TYPE, &out.guidMajorType), done);
        DMFTCHECKHR_GOTO(pOutputMediaType->GetGUID(MF_MT_SUBTYPE, &out.guidSubtype), done);
    }

    //
    //Use all possible plugins
    //

    unFlags = MFT_ENUM_FLAG_ASYNCMFT | MFT_ENUM_FLAG_SYNCMFT | MFT_ENUM_FLAG_SORTANDFILTER | MFT_ENUM_FLAG_SORTANDFILTER_WEB_ONLY;
    DMFTCHECKHR_GOTO(MFTEnumEx(guidcategory,
        unFlags,
        IsDecoder ? &in : NULL,
        IsDecoder ? NULL : &out,
        &ppActivates,
        &cActivates
        ),done);
    if (cActivates!=0)
    {
        DMFTCHECKHR_GOTO(MF_E_TOPO_CODEC_NOT_FOUND, done);
    }
    for (DWORD dwIndex = 0; dwIndex < cActivates; dwIndex++)
    {
        //
        //Check if this MFT requires an unlocking.. skip if it does
        //
        unMFTFlags = MFGetAttributeUINT32(ppActivates[dwIndex], MF_TRANSFORM_FLAGS_Attribute, 0);
        if (unMFTFlags & MFT_ENUM_FLAG_FIELDOFUSE)
        {
            //
            //If you have the unlocking key set MFT_FIELDOFUSE_UNLOCK_Attribute on the activate
            //
            continue;
        }
       
        hr = ppActivates[dwIndex]->ActivateObject(IID_PPV_ARGS(ppTransform));
        if (FAILED(hr))
            continue; //hope the next activate is activable
        ppActivates[dwIndex]->DetachObject();

        hr = (*ppTransform)->MFTGetStreamCount(&dwInputStreams, &dwOutputStreams);
        
        if (FAILED(hr) || dwInputStreams != 1 || dwOutputStreams > 127)//remove the hardcoding
        {
            //
            //Exception.. we can't use a stream that doesn't give us an idea about it's streams
            //
            continue;
        }
         if (IsDecoder)
        {
            hr = ConfigureDecoder(*ppTransform);
        }
        else
        {
            hr = ConfigureEncoder(*ppTransform);
        }
        if (FAILED(hr))
        {
            SAFE_RELEASE(*ppTransform);
        }
    }

done:
    return hr;
}

/*++
    
--*/

STDMETHODIMP CDecoderTee::ConfigureEncoder( _In_ IMFTransform *pTransform ) 
{
    HRESULT hr = S_OK;
    //
    //Set the input media type and the output media type and that should be pretty much it
    //
    ComPtr<IMFMediaType> pInputType  = nullptr;
    ComPtr<IMFMediaType> pOutputType = nullptr;
    
    pInputType =  getInMediaType();
    pOutputType = getOutMediaType();
    
    if (pInputType && pOutputType)
    {
        //We should be good by now
        DMFTCHECKHR_GOTO( pTransform->MFTSetInputType( 0,pInputType.Get(), 0), done);
        //If we have D3D enabled then try again with D3D disabled and see the result
        //
        DMFTCHECKHR_GOTO(pTransform->MFTSetOutputType(0,pOutputType.Get(), 0), done);
        //
        //If the above operation fails and we are allowing partial media types then
        //we have to find the full media type and then check again
    }
    else
    {
        hr = E_FAIL;
    }
done:
    return hr;
}

STDMETHODIMP CDecoderTee::ConfigureDecoder( _In_ IMFTransform *pTransform)
{
    UNREFERENCED_PARAMETER( pTransform );
    return S_OK;
}




