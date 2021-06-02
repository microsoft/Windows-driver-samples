//*@@@+++@@@@******************************************************************
//
// Microsoft Windows Media Foundation
// Copyright (C) Microsoft Corporation. All rights reserved.
//
//*@@@---@@@@******************************************************************
//

#include "stdafx.h"
#include "common.h"
#include "multipinmft.h"
#include "basepin.h"

#pragma comment(lib, "d2d1") 
#ifdef MF_WPP
#include "multipinmftutils.tmh"    //--REF_ANALYZER_DONT_REMOVE--
#endif

// Critical sections

CCritSec::CCritSec()
{
    InitializeCriticalSection(&m_criticalSection);
}

CCritSec::~CCritSec()
{
    DeleteCriticalSection(&m_criticalSection);
}

_Requires_lock_not_held_(m_criticalSection) _Acquires_lock_(m_criticalSection)
void CCritSec::Lock()
{
    EnterCriticalSection(&m_criticalSection);
}

_Requires_lock_held_(m_criticalSection) _Releases_lock_(m_criticalSection)
void CCritSec::Unlock()
{
    LeaveCriticalSection(&m_criticalSection);
}


_Acquires_lock_(this->m_pCriticalSection->m_criticalSection)
CAutoLock::CAutoLock(CCritSec& crit)
{
    m_pCriticalSection = &crit;
    m_pCriticalSection->Lock();
}
_Acquires_lock_(this->m_pCriticalSection->m_criticalSection)
CAutoLock::CAutoLock(CCritSec* crit)
{
    m_pCriticalSection = crit;
    m_pCriticalSection->Lock();
}
_Releases_lock_(this->m_pCriticalSection->m_criticalSection)
CAutoLock::~CAutoLock()
{
    m_pCriticalSection->Unlock();
}

//
//Some utility functions..
//

/*++
    Description:
    Used to check if the input and the output Image types are optinmized or not
--*/

STDMETHODIMP   IsOptimizedPlanarVideoInputImageOutputPair(
    _In_ IMFMediaType *inMediaType,
    _In_ IMFMediaType *outMediaType,
    _Out_ bool *optimized,
    _Out_ bool *optimizedxvpneeded )
{
    HRESULT         hr                  = S_OK;
    GUID            guidInputSubType    = GUID_NULL;
    GUID            guidOutputSubType   = GUID_NULL;
    UINT32          uWidthIn = 0, uHeightIn = 0, uWidthOut = 0, uHeightOut = 0;


    DMFTCHECKHR_GOTO(inMediaType->GetGUID(MF_MT_SUBTYPE, &guidInputSubType), done);
    
    DMFTCHECKHR_GOTO(outMediaType->GetGUID(MF_MT_SUBTYPE, &guidOutputSubType), done);

    *optimized = false;         //Assume we aren't optimized . Optimized = (ip = YU12|NV12 and  op = JPEG) 
    *optimizedxvpneeded = true; //Assume we need xvps

    if (IsEqualGUID(guidInputSubType, MFVideoFormat_YV12) || IsEqualGUID(guidInputSubType, MFVideoFormat_NV12))
    {
        if (IsEqualGUID(guidOutputSubType, GUID_ContainerFormatJpeg))
        {
            *optimized = true;
        }
    }

    if (!*optimized)
    {
        goto done;
    }

    DMFTCHECKHR_GOTO(MFGetAttributeSize(inMediaType, MF_MT_FRAME_SIZE, &uWidthIn, &uHeightIn), done);
    DMFTCHECKHR_GOTO(MFGetAttributeSize(outMediaType, MF_MT_FRAME_SIZE, &uWidthOut, &uHeightOut), done);

    if ((uWidthIn == uWidthOut) && (uHeightIn == uHeightOut))
    {
        *optimizedxvpneeded = false;
    }
    if (!*optimizedxvpneeded)
    {
        UINT32 nominalRange;
        hr = inMediaType->GetUINT32(MF_MT_VIDEO_NOMINAL_RANGE, &nominalRange);

        if (FAILED(hr) || nominalRange != MFNominalRange_0_255)
        {
            //XVP needed since nominal range is not 0-255 for YV12 or NV12 fed into WIC
            *optimizedxvpneeded = true;
        }
        hr = S_OK;
    }

done:
    return hr;
}


/*
Description:

    This is used whenever there is a media type change on an output pin and the 
    Output queue is being reconfigured.
    The possible return values for the function are as follows
    
    DeviceMftTransformXVPIllegal        -> If either of the mediatypes or both are NULL
    DeviceMftTransformXVPDisruptiveIn   -> If the mediatype  at the output pin is greater than the input pin. This will result in change of the media type on the input
    DeviceMftTransformXVPDisruptiveOut  -> This is a reconfiguration or addition of the XVP in the Output pin queue
    DeviceMftTransformXVPCurrent        -> No XVP needed at all
*/
STDMETHODIMP CompareMediaTypesForConverter(
    _In_opt_ IMFMediaType *inMediaType,
    _In_    IMFMediaType  *newMediaType,
    _Inout_ PDMFT_conversion_type  operation
    )
{
    HRESULT hr          = S_OK;
    GUID    guidTypeA   = GUID_NULL;
    GUID    guidTypeB   = GUID_NULL;
    
    *operation = DeviceMftTransformTypeIllegal;
    if ((!inMediaType) || (!newMediaType))
    {
       goto done;
    }

      if ( SUCCEEDED( inMediaType->GetGUID(  MF_MT_MAJOR_TYPE, &guidTypeA ) ) &&
         SUCCEEDED( newMediaType->GetGUID( MF_MT_MAJOR_TYPE, &guidTypeB ) ) &&
        IsEqualGUID( guidTypeA, guidTypeB ) )
    {
          if ( SUCCEEDED( inMediaType->GetGUID ( MF_MT_SUBTYPE, &guidTypeA ) ) &&
              SUCCEEDED( newMediaType->GetGUID( MF_MT_SUBTYPE, &guidTypeB ) ) &&
              IsEqualGUID( guidTypeA, guidTypeB ) )
          {
              BOOL passThrough = TRUE;
              DMFTCHECKHR_GOTO(CheckPassthroughMediaType(inMediaType, newMediaType, passThrough), done);
              *operation = passThrough?DeviceMftTransformTypeEqual: DeviceMftTransformTypeXVP;
          }
          else
          {
              //This is a disruptive operation. Actually a decoder operation!
              *operation = DeviceMftTransformTypeDecoder;
          }
      }
  done:
    return hr;
}

/*++
Description: Used to test if the sample passed is a DX sample or not?
--*/
HRESULT IsInputDxSample(
    _In_ IMFSample* pSample,
    _Inout_ BOOL *isDxSample
    )
{
    HRESULT hr = S_OK;
    ComPtr<IMFMediaBuffer> spBuffer = nullptr;
    DMFTCHECKNULL_GOTO( pSample, done, E_INVALIDARG );

    *isDxSample = false;
    if ( SUCCEEDED( pSample->GetBufferByIndex( 0, &spBuffer ) ) )
    {
        ComPtr<IMFDXGIBuffer> spDXGIBuffer;
        ComPtr<ID3D11Texture2D> spResourceTexture;
        if ( SUCCEEDED( spBuffer.As(&spDXGIBuffer  ) ) &&
            SUCCEEDED( spDXGIBuffer-> GetResource( IID_PPV_ARGS( &spResourceTexture ) ) ) )
        {
            *isDxSample = true;
        }
    }
done:
    return hr;
}


/*++
    Description:
        Helper function to return back if the Pin is in stopped stateo or not
--*/
STDMETHODIMP_(BOOL) IsPinStateInActive( _In_ DeviceStreamState state)
{
    if ((state == DeviceStreamState_Disabled) ||
        (state == DeviceStreamState_Stop))
    {
        return TRUE;
    }
    return FALSE;
}

/*
Description:
    A Helper function to return if the Mediatype is a compressed video type or not!
*/
STDMETHODIMP_(BOOL) IsKnownUncompressedVideoType(_In_ GUID guidSubType)
{
    if (guidSubType == MFVideoFormat_ARGB32 ||
        guidSubType == MFVideoFormat_RGB24 ||
        guidSubType == MFVideoFormat_RGB555 ||
        guidSubType ==  MFVideoFormat_RGB565 ||  
        guidSubType ==  MFVideoFormat_RGB32 ||          
        guidSubType ==  MFVideoFormat_RGB8 ||  
        guidSubType ==  MFVideoFormat_AI44 ||  
        guidSubType ==  MFVideoFormat_AYUV ||  
        guidSubType ==  MFVideoFormat_YUY2 ||  
        guidSubType ==  MFVideoFormat_YVYU ||  
        guidSubType ==  MFVideoFormat_YVU9 ||  
        guidSubType ==  MFVideoFormat_UYVY ||  
        guidSubType ==  MFVideoFormat_NV11 ||  
        guidSubType ==  MFVideoFormat_NV12 ||  
        guidSubType ==  MFVideoFormat_YV12 ||  
        guidSubType ==  MFVideoFormat_I420 ||  
        guidSubType ==  MFVideoFormat_IYUV ||  
        guidSubType ==  MFVideoFormat_Y210 ||  
        guidSubType ==  MFVideoFormat_Y216 ||  
        guidSubType ==  MFVideoFormat_Y410 ||  
        guidSubType ==  MFVideoFormat_Y416 ||  
        guidSubType ==  MFVideoFormat_Y41P ||  
        guidSubType ==  MFVideoFormat_Y41T ||  
        guidSubType ==  MFVideoFormat_Y42T ||  
        guidSubType ==  MFVideoFormat_P210 ||  
        guidSubType ==  MFVideoFormat_P216 ||  
        guidSubType ==  MFVideoFormat_P010 ||  
        guidSubType ==  MFVideoFormat_P016 ||  
        guidSubType ==  MFVideoFormat_v210 ||  
        guidSubType ==  MFVideoFormat_v216 ||  
        guidSubType ==  MFVideoFormat_v410 ||
        guidSubType ==  MFVideoFormat_L8   ||
        guidSubType ==  MFVideoFormat_L16  ||
        guidSubType == MFVideoFormat_D16)
    {  
        return( TRUE );  
    }  

    return( FALSE ); 
}

#ifndef IF_EQUAL_RETURN
#define IF_EQUAL_RETURN(param, val) if(val == param) return #val
#endif

#define checkAdjustBufferCap(a,len){\
    char* tStore = NULL; \
if (a && strlen(a) > ((len * 7) / 10)){\
    tStore = a; \
    len *= 2; \
    a = new (std::nothrow) char[len]; \
if (!a){\
goto done;}\
    a[0] = 0; \
    strcat_s(a, len, tStore); \
    delete(tStore); }\
}


/*++
Description:
    returns an ascii buffer for the GUID passed. The Caller should release the memory allocated
--*/
LPSTR DumpGUIDA(_In_ REFGUID guid)
{
    LPOLESTR lpszGuidString = NULL;
    char *ansiguidStr = NULL;
    if (SUCCEEDED(StringFromCLSID(guid, &lpszGuidString)))
    {
        int mbGuidLen = 0;
        mbGuidLen = WideCharToMultiByte( CP_ACP, WC_NO_BEST_FIT_CHARS, lpszGuidString, -1, NULL, 0, NULL, NULL);
        if (mbGuidLen > 0)
        {
            mf_assert(mbGuidLen == (int)wcslen(lpszGuidString));
            ansiguidStr = new (std::nothrow) char[mbGuidLen];
            if (ansiguidStr)
            {
                WideCharToMultiByte(CP_ACP, WC_NO_BEST_FIT_CHARS, lpszGuidString, -1, ansiguidStr, mbGuidLen, NULL, NULL);
                CoTaskMemFree(lpszGuidString);
                ansiguidStr[mbGuidLen - 1] = 0;
            }
        }
    }
    return ansiguidStr;
}

//
//Borrrowed from MDSN sample
//
LPCSTR GetGUIDNameConst(const GUID& guid)
{
    IF_EQUAL_RETURN(guid, MF_MT_MAJOR_TYPE);
    IF_EQUAL_RETURN(guid, MF_MT_MAJOR_TYPE);
    IF_EQUAL_RETURN(guid, MF_MT_SUBTYPE);
    IF_EQUAL_RETURN(guid, MF_MT_ALL_SAMPLES_INDEPENDENT);
    IF_EQUAL_RETURN(guid, MF_MT_FIXED_SIZE_SAMPLES);
    IF_EQUAL_RETURN(guid, MF_MT_COMPRESSED);
    IF_EQUAL_RETURN(guid, MF_MT_SAMPLE_SIZE);
    IF_EQUAL_RETURN(guid, MF_MT_WRAPPED_TYPE);
    IF_EQUAL_RETURN(guid, MF_MT_AUDIO_NUM_CHANNELS);
    IF_EQUAL_RETURN(guid, MF_MT_AUDIO_SAMPLES_PER_SECOND);
    IF_EQUAL_RETURN(guid, MF_MT_AUDIO_FLOAT_SAMPLES_PER_SECOND);
    IF_EQUAL_RETURN(guid, MF_MT_AUDIO_AVG_BYTES_PER_SECOND);
    IF_EQUAL_RETURN(guid, MF_MT_AUDIO_BLOCK_ALIGNMENT);
    IF_EQUAL_RETURN(guid, MF_MT_AUDIO_BITS_PER_SAMPLE);
    IF_EQUAL_RETURN(guid, MF_MT_AUDIO_VALID_BITS_PER_SAMPLE);
    IF_EQUAL_RETURN(guid, MF_MT_AUDIO_SAMPLES_PER_BLOCK);
    IF_EQUAL_RETURN(guid, MF_MT_AUDIO_CHANNEL_MASK);
    IF_EQUAL_RETURN(guid, MF_MT_AUDIO_FOLDDOWN_MATRIX);
    IF_EQUAL_RETURN(guid, MF_MT_AUDIO_WMADRC_PEAKREF);
    IF_EQUAL_RETURN(guid, MF_MT_AUDIO_WMADRC_PEAKTARGET);
    IF_EQUAL_RETURN(guid, MF_MT_AUDIO_WMADRC_AVGREF);
    IF_EQUAL_RETURN(guid, MF_MT_AUDIO_WMADRC_AVGTARGET);
    IF_EQUAL_RETURN(guid, MF_MT_AUDIO_PREFER_WAVEFORMATEX);
    IF_EQUAL_RETURN(guid, MF_MT_AAC_PAYLOAD_TYPE);
    IF_EQUAL_RETURN(guid, MF_MT_AAC_AUDIO_PROFILE_LEVEL_INDICATION);
    IF_EQUAL_RETURN(guid, MF_MT_FRAME_SIZE);
    IF_EQUAL_RETURN(guid, MF_MT_FRAME_RATE);
    IF_EQUAL_RETURN(guid, MF_MT_FRAME_RATE_RANGE_MAX);
    IF_EQUAL_RETURN(guid, MF_MT_FRAME_RATE_RANGE_MIN);
    IF_EQUAL_RETURN(guid, MF_MT_PIXEL_ASPECT_RATIO);
    IF_EQUAL_RETURN(guid, MF_MT_DRM_FLAGS);
    IF_EQUAL_RETURN(guid, MF_MT_PAD_CONTROL_FLAGS);
    IF_EQUAL_RETURN(guid, MF_MT_SOURCE_CONTENT_HINT);
    IF_EQUAL_RETURN(guid, MF_MT_VIDEO_CHROMA_SITING);
    IF_EQUAL_RETURN(guid, MF_MT_INTERLACE_MODE);
    IF_EQUAL_RETURN(guid, MF_MT_TRANSFER_FUNCTION);
    IF_EQUAL_RETURN(guid, MF_MT_VIDEO_PRIMARIES);
    IF_EQUAL_RETURN(guid, MF_MT_CUSTOM_VIDEO_PRIMARIES);
    IF_EQUAL_RETURN(guid, MF_MT_YUV_MATRIX);
    IF_EQUAL_RETURN(guid, MF_MT_VIDEO_LIGHTING);
    IF_EQUAL_RETURN(guid, MF_MT_VIDEO_NOMINAL_RANGE);
    IF_EQUAL_RETURN(guid, MF_MT_GEOMETRIC_APERTURE);
    IF_EQUAL_RETURN(guid, MF_MT_MINIMUM_DISPLAY_APERTURE);
    IF_EQUAL_RETURN(guid, MF_MT_PAN_SCAN_APERTURE);
    IF_EQUAL_RETURN(guid, MF_MT_PAN_SCAN_ENABLED);
    IF_EQUAL_RETURN(guid, MF_MT_AVG_BITRATE);
    IF_EQUAL_RETURN(guid, MF_MT_AVG_BIT_ERROR_RATE);
    IF_EQUAL_RETURN(guid, MF_MT_MAX_KEYFRAME_SPACING);
    IF_EQUAL_RETURN(guid, MF_MT_DEFAULT_STRIDE);
    IF_EQUAL_RETURN(guid, MF_MT_PALETTE);
    IF_EQUAL_RETURN(guid, MF_MT_USER_DATA);
    IF_EQUAL_RETURN(guid, MF_MT_AM_FORMAT_TYPE);
    IF_EQUAL_RETURN(guid, MF_MT_MPEG_START_TIME_CODE);
    IF_EQUAL_RETURN(guid, MF_MT_MPEG2_PROFILE);
    IF_EQUAL_RETURN(guid, MF_MT_MPEG2_LEVEL);
    IF_EQUAL_RETURN(guid, MF_MT_MPEG2_FLAGS);
    IF_EQUAL_RETURN(guid, MF_MT_MPEG_SEQUENCE_HEADER);
    IF_EQUAL_RETURN(guid, MF_MT_DV_AAUX_SRC_PACK_0);
    IF_EQUAL_RETURN(guid, MF_MT_DV_AAUX_CTRL_PACK_0);
    IF_EQUAL_RETURN(guid, MF_MT_DV_AAUX_SRC_PACK_1);
    IF_EQUAL_RETURN(guid, MF_MT_DV_AAUX_CTRL_PACK_1);
    IF_EQUAL_RETURN(guid, MF_MT_DV_VAUX_SRC_PACK);
    IF_EQUAL_RETURN(guid, MF_MT_DV_VAUX_CTRL_PACK);
    IF_EQUAL_RETURN(guid, MF_MT_ARBITRARY_HEADER);
    IF_EQUAL_RETURN(guid, MF_MT_ARBITRARY_FORMAT);
    IF_EQUAL_RETURN(guid, MF_MT_IMAGE_LOSS_TOLERANT);
    IF_EQUAL_RETURN(guid, MF_MT_MPEG4_SAMPLE_DESCRIPTION);
    IF_EQUAL_RETURN(guid, MF_MT_MPEG4_CURRENT_SAMPLE_ENTRY);
    IF_EQUAL_RETURN(guid, MF_MT_ORIGINAL_4CC);
    IF_EQUAL_RETURN(guid, MF_MT_ORIGINAL_WAVE_FORMAT_TAG);

    // Media types

    IF_EQUAL_RETURN(guid, MFMediaType_Audio);
    IF_EQUAL_RETURN(guid, MFMediaType_Video);
    IF_EQUAL_RETURN(guid, MFMediaType_Protected);
    IF_EQUAL_RETURN(guid, MFMediaType_SAMI);
    IF_EQUAL_RETURN(guid, MFMediaType_Script);
    IF_EQUAL_RETURN(guid, MFMediaType_Image);
    IF_EQUAL_RETURN(guid, MFMediaType_HTML);
    IF_EQUAL_RETURN(guid, MFMediaType_Binary);
    IF_EQUAL_RETURN(guid, MFMediaType_FileTransfer);

    IF_EQUAL_RETURN(guid, MFVideoFormat_AI44); //     FCC('AI44')
    IF_EQUAL_RETURN(guid, MFVideoFormat_ARGB32); //   D3DFMT_A8R8G8B8 
    IF_EQUAL_RETURN(guid, MFVideoFormat_AYUV); //     FCC('AYUV')
    IF_EQUAL_RETURN(guid, MFVideoFormat_DV25); //     FCC('dv25')
    IF_EQUAL_RETURN(guid, MFVideoFormat_DV50); //     FCC('dv50')
    IF_EQUAL_RETURN(guid, MFVideoFormat_DVH1); //     FCC('dvh1')
    IF_EQUAL_RETURN(guid, MFVideoFormat_DVSD); //     FCC('dvsd')
    IF_EQUAL_RETURN(guid, MFVideoFormat_DVSL); //     FCC('dvsl')
    IF_EQUAL_RETURN(guid, MFVideoFormat_H264); //     FCC('H264')
    IF_EQUAL_RETURN(guid, MFVideoFormat_I420); //     FCC('I420')
    IF_EQUAL_RETURN(guid, MFVideoFormat_IYUV); //     FCC('IYUV')
    IF_EQUAL_RETURN(guid, MFVideoFormat_M4S2); //     FCC('M4S2')
    IF_EQUAL_RETURN(guid, MFVideoFormat_MJPG);
    IF_EQUAL_RETURN(guid, MFVideoFormat_MP43); //     FCC('MP43')
    IF_EQUAL_RETURN(guid, MFVideoFormat_MP4S); //     FCC('MP4S')
    IF_EQUAL_RETURN(guid, MFVideoFormat_MP4V); //     FCC('MP4V')
    IF_EQUAL_RETURN(guid, MFVideoFormat_MPG1); //     FCC('MPG1')
    IF_EQUAL_RETURN(guid, MFVideoFormat_MSS1); //     FCC('MSS1')
    IF_EQUAL_RETURN(guid, MFVideoFormat_MSS2); //     FCC('MSS2')
    IF_EQUAL_RETURN(guid, MFVideoFormat_NV11); //     FCC('NV11')
    IF_EQUAL_RETURN(guid, MFVideoFormat_NV12); //     FCC('NV12')
    IF_EQUAL_RETURN(guid, MFVideoFormat_P010); //     FCC('P010')
    IF_EQUAL_RETURN(guid, MFVideoFormat_P016); //     FCC('P016')
    IF_EQUAL_RETURN(guid, MFVideoFormat_P210); //     FCC('P210')
    IF_EQUAL_RETURN(guid, MFVideoFormat_P216); //     FCC('P216')
    IF_EQUAL_RETURN(guid, MFVideoFormat_RGB24); //    D3DFMT_R8G8B8 
    IF_EQUAL_RETURN(guid, MFVideoFormat_RGB32); //    D3DFMT_X8R8G8B8 
    IF_EQUAL_RETURN(guid, MFVideoFormat_RGB555); //   D3DFMT_X1R5G5B5 
    IF_EQUAL_RETURN(guid, MFVideoFormat_RGB565); //   D3DFMT_R5G6B5 
    IF_EQUAL_RETURN(guid, MFVideoFormat_RGB8);
    IF_EQUAL_RETURN(guid, MFVideoFormat_UYVY); //     FCC('UYVY')
    IF_EQUAL_RETURN(guid, MFVideoFormat_v210); //     FCC('v210')
    IF_EQUAL_RETURN(guid, MFVideoFormat_v410); //     FCC('v410')
    IF_EQUAL_RETURN(guid, MFVideoFormat_WMV1); //     FCC('WMV1')
    IF_EQUAL_RETURN(guid, MFVideoFormat_WMV2); //     FCC('WMV2')
    IF_EQUAL_RETURN(guid, MFVideoFormat_WMV3); //     FCC('WMV3')
    IF_EQUAL_RETURN(guid, MFVideoFormat_WVC1); //     FCC('WVC1')
    IF_EQUAL_RETURN(guid, MFVideoFormat_Y210); //     FCC('Y210')
    IF_EQUAL_RETURN(guid, MFVideoFormat_Y216); //     FCC('Y216')
    IF_EQUAL_RETURN(guid, MFVideoFormat_Y410); //     FCC('Y410')
    IF_EQUAL_RETURN(guid, MFVideoFormat_Y416); //     FCC('Y416')
    IF_EQUAL_RETURN(guid, MFVideoFormat_Y41P);
    IF_EQUAL_RETURN(guid, MFVideoFormat_Y41T);
    IF_EQUAL_RETURN(guid, MFVideoFormat_YUY2); //     FCC('YUY2')
    IF_EQUAL_RETURN(guid, MFVideoFormat_YV12); //     FCC('YV12')
    IF_EQUAL_RETURN(guid, MFVideoFormat_YVYU);

    IF_EQUAL_RETURN(guid, MFAudioFormat_PCM); //              WAVE_FORMAT_PCM 
    IF_EQUAL_RETURN(guid, MFAudioFormat_Float); //            WAVE_FORMAT_IEEE_FLOAT 
    IF_EQUAL_RETURN(guid, MFAudioFormat_DTS); //              WAVE_FORMAT_DTS 
    IF_EQUAL_RETURN(guid, MFAudioFormat_Dolby_AC3_SPDIF); //  WAVE_FORMAT_DOLBY_AC3_SPDIF 
    IF_EQUAL_RETURN(guid, MFAudioFormat_DRM); //              WAVE_FORMAT_DRM 
    IF_EQUAL_RETURN(guid, MFAudioFormat_WMAudioV8); //        WAVE_FORMAT_WMAUDIO2 
    IF_EQUAL_RETURN(guid, MFAudioFormat_WMAudioV9); //        WAVE_FORMAT_WMAUDIO3 
    IF_EQUAL_RETURN(guid, MFAudioFormat_WMAudio_Lossless); // WAVE_FORMAT_WMAUDIO_LOSSLESS 
    IF_EQUAL_RETURN(guid, MFAudioFormat_WMASPDIF); //         WAVE_FORMAT_WMASPDIF 
    IF_EQUAL_RETURN(guid, MFAudioFormat_MSP1); //             WAVE_FORMAT_WMAVOICE9 
    IF_EQUAL_RETURN(guid, MFAudioFormat_MP3); //              WAVE_FORMAT_MPEGLAYER3 
    IF_EQUAL_RETURN(guid, MFAudioFormat_MPEG); //             WAVE_FORMAT_MPEG 
    IF_EQUAL_RETURN(guid, MFAudioFormat_AAC); //              WAVE_FORMAT_MPEG_HEAAC 
    IF_EQUAL_RETURN(guid, MFAudioFormat_ADTS); //             WAVE_FORMAT_MPEG_ADTS_AAC 

    return NULL;
}


LPSTR DumpAttribute( _In_ const MF_ATTRIBUTE_TYPE& type,
    _In_ REFPROPVARIANT var)
{
    CHAR *tempStr = NULL;
    tempStr = new (std::nothrow) CHAR[256];
    switch (type)
    {
    case MF_ATTRIBUTE_UINT32:
        if (var.vt == VT_UI4)
        {
            sprintf_s(tempStr, 256, "%u", var.ulVal);
        }
        break;
    case MF_ATTRIBUTE_UINT64:
        if (var.vt == VT_UI8)
        {
            sprintf_s(tempStr, 256, "%I64d  (high: %d low: %d)", var.uhVal.QuadPart, var.uhVal.HighPart, var.uhVal.LowPart);
        }
        break;
    case MF_ATTRIBUTE_DOUBLE:
        if (var.vt == VT_R8)
        {
            sprintf_s(tempStr, 256, "%.4f", var.dblVal);
        }
        break;
    case MF_ATTRIBUTE_GUID:
        if (var.vt == VT_CLSID)
        {
            return DumpGUIDA(*var.puuid);
        }
        break;
    case MF_ATTRIBUTE_STRING:
        if (var.vt == VT_LPWSTR)
        {
            sprintf_s(tempStr, 256, "%S", var.pwszVal);
        }
        break;
    case MF_ATTRIBUTE_IUNKNOWN:
        break;
    default:
        printf("(Unknown Attribute Type = %d) ", type);
        break;
    }
    return tempStr;
}

CMediaTypePrinter::CMediaTypePrinter( 
    _In_ IMFMediaType *_pMediaType  )
    :   pMediaType(_pMediaType),
        m_pBuffer(NULL)
{
}

CMediaTypePrinter::~CMediaTypePrinter()
{
    if (m_pBuffer)
    {
        delete(m_pBuffer);
    }
}

/*++
Description:
Rudimentary function to print the complete Media type
--*/
PCHAR CMediaTypePrinter::ToCompleteString( )
{
    HRESULT             hr          = S_OK;
    UINT32              attrCount   = 0;
    GUID                attrGuid    = { 0 };
    char                *tempStore  = nullptr;
    PROPVARIANT var;
    LPSTR pTempBaseStr;
    MF_ATTRIBUTE_TYPE   pType;
    
    if ( pMediaType && !m_pBuffer )
    {
        DMFTCHECKHR_GOTO(pMediaType->GetCount(&attrCount), done);
        buffLen = MEDIAPRINTER_STARTLEN;
        m_pBuffer       = new char[buffLen];
        DMFTCHECKNULL_GOTO(m_pBuffer, done, E_OUTOFMEMORY);
        m_pBuffer[0]    = 0;
        for ( UINT32 ulIndex = 0; ulIndex < attrCount; ulIndex++ )
        {
            PropVariantInit( &var );
            checkAdjustBufferCap( m_pBuffer, buffLen );
            DMFTCHECKHR_GOTO( pMediaType->GetItemByIndex( ulIndex, &attrGuid, &var ), done );
            DMFTCHECKHR_GOTO( pMediaType->GetItemType( attrGuid, &pType ), done );
            if ( ulIndex > 0 )
                strcat_s(m_pBuffer, MEDIAPRINTER_STARTLEN, " : ");
            strcat_s( m_pBuffer, buffLen, GetGUIDNameConst( attrGuid ) );
            strcat_s( m_pBuffer, buffLen, "=" );
            pTempBaseStr = DumpAttribute( pType, var );
            strcat_s( m_pBuffer, buffLen, pTempBaseStr );
            delete( pTempBaseStr );
            PropVariantClear( &var );
        }
    done:
        if ( tempStore )
        {
            delete( tempStore );
        }
    }
    return m_pBuffer;
}

/*++
Description:
Rudimentary function to print the Media type
--*/

PCHAR CMediaTypePrinter::ToString()
{
    //
    //Following are the important ones of Mediatype attributes
    //

    HRESULT     hr = S_OK;
    PROPVARIANT var;
    LPSTR pTempBaseStr;
    MF_ATTRIBUTE_TYPE   pType;
    GUID                attrGuid;
    GUID impGuids[] = {
        MF_MT_SUBTYPE,
        MF_MT_FRAME_SIZE,
        MF_MT_SAMPLE_SIZE,
        MF_MT_FRAME_RATE,
        MF_MT_DEFAULT_STRIDE,
        MF_XVP_DISABLE_FRC
    };

    if (pMediaType && !m_pBuffer)
    {
        buffLen = MEDIAPRINTER_STARTLEN;
        m_pBuffer = new (std::nothrow) char[buffLen];
        DMFTCHECKNULL_GOTO(m_pBuffer, done, E_OUTOFMEMORY);
        m_pBuffer[0] = 0;
        for (UINT32 ulIndex = 0; ulIndex < ARRAYSIZE(impGuids); ulIndex++)
        {
            PropVariantInit(&var);
            checkAdjustBufferCap(m_pBuffer, buffLen);
            attrGuid = impGuids[ulIndex];
            DMFTCHECKHR_GOTO(pMediaType->GetItemType(attrGuid, &pType), done);
            DMFTCHECKHR_GOTO(pMediaType->GetItem(attrGuid, &var), done);
            if (ulIndex > 0)
                strcat_s(m_pBuffer, MEDIAPRINTER_STARTLEN, " : ");
            strcat_s(m_pBuffer, buffLen, GetGUIDNameConst(attrGuid));
            strcat_s(m_pBuffer, buffLen, "=");
            pTempBaseStr = DumpAttribute(pType, var);
            strcat_s(m_pBuffer, buffLen, pTempBaseStr);
            delete(pTempBaseStr);
            PropVariantClear(&var);
        }
    }
done:
    return m_pBuffer;
}
/*++
Description:
    Debug message printer to print the message passed through WPP
--*/
void printMessageEvent(MFT_MESSAGE_TYPE msg)
{
    switch (msg)
    {
    case MFT_MESSAGE_COMMAND_FLUSH:
        DMFTRACE(DMFT_GENERAL, TRACE_LEVEL_INFORMATION, "%!FUNC! :PROCESSMESSAGE: MFT_MESSAGE_COMMAND_FLUSH");
        break;
    case MFT_MESSAGE_COMMAND_DRAIN:
        DMFTRACE(DMFT_GENERAL, TRACE_LEVEL_INFORMATION, "%!FUNC! :PROCESSMESSAGE: MFT_MESSAGE_COMMAND_DRAIN");
        break;
    case MFT_MESSAGE_COMMAND_MARKER:
        DMFTRACE(DMFT_GENERAL, TRACE_LEVEL_INFORMATION, "%!FUNC! :PROCESSMESSAGE: MFT_MESSAGE_COMMAND_MARKER");
        break;
    case MFT_MESSAGE_COMMAND_TICK:
        DMFTRACE(DMFT_GENERAL, TRACE_LEVEL_INFORMATION, "%!FUNC! :PROCESSMESSAGE: MFT_MESSAGE_COMMAND_TICK");
        break;
    case MFT_MESSAGE_NOTIFY_END_OF_STREAM:
        DMFTRACE(DMFT_GENERAL, TRACE_LEVEL_INFORMATION, "%!FUNC! :PROCESSMESSAGE: MFT_MESSAGE_NOTIFY_END_OF_STREAM");
        break;
    case MFT_MESSAGE_NOTIFY_BEGIN_STREAMING:
        DMFTRACE(DMFT_GENERAL, TRACE_LEVEL_INFORMATION, "%!FUNC! :PROCESSMESSAGE: MFT_MESSAGE_NOTIFY_BEGIN_STREAMING");
        break;
    case MFT_MESSAGE_NOTIFY_START_OF_STREAM:
        DMFTRACE(DMFT_GENERAL, TRACE_LEVEL_INFORMATION, "%!FUNC! :PROCESSMESSAGE: MFT_MESSAGE_NOTIFY_START_OF_STREAM");
        break;
    case  MFT_MESSAGE_DROP_SAMPLES:
        DMFTRACE(DMFT_GENERAL, TRACE_LEVEL_INFORMATION, "%!FUNC! :PROCESSMESSAGE: MFT_MESSAGE_DROP_SAMPLES");
        break;
    case MFT_MESSAGE_SET_D3D_MANAGER:
        DMFTRACE(DMFT_GENERAL, TRACE_LEVEL_INFORMATION, "%!FUNC! :PROCESSMESSAGE: MFT_MESSAGE_SET_D3D_MANAGER");
        break;

    }
}

//
// Used to check if the pin is a custom pin or not!!
//
STDMETHODIMP CheckCustomPin(
    _In_ CInPin * pPin,
    _Inout_ PBOOL  pIsCustom 
    )
{
    HRESULT hr = S_OK;
    DMFTCHECKNULL_GOTO( pPin, done, E_INVALIDARG);
    DMFTCHECKNULL_GOTO( pIsCustom, done, E_INVALIDARG);
    
    *pIsCustom = false;

#if defined _CPPRTTI
    if (dynamic_cast<CCustomPin*>(pPin) != nullptr)
    {
        *pIsCustom = true;
    }
    else
#endif
    {
        GUID categoryGUID = GUID_NULL;
        if (SUCCEEDED(pPin->GetGUID(MF_DEVICESTREAM_STREAM_CATEGORY, &categoryGUID))
            && IsEqualCLSID(categoryGUID, AVSTREAM_CUSTOM_PIN_IMAGE))
        {
            *pIsCustom = true;
        }
    }
done:
    return hr;
}


#ifdef MF_DEVICEMFT_ADD_GRAYSCALER_
//
// Please remove the below functions if you don't need them.
// Helper functions for the gray scaler
// 

void TransformImage_UYVY(
    const RECT &rcDest,
    _Inout_updates_(_Inexpressible_(lDestStride * dwHeightInPixels)) BYTE *pDest,
    _In_ LONG lDestStride,
    _In_reads_(_Inexpressible_(lSrcStride * dwHeightInPixels)) const BYTE *pSrc,
    _In_ LONG lSrcStride,
    _In_ DWORD dwWidthInPixels,
    _In_ DWORD dwHeightInPixels)
{
    DWORD y = 0;
    // Round down to the even value.
    const UINT32 left = rcDest.left & ~(1);
    const UINT32 right = rcDest.right & ~(1);
    const DWORD y0 = min((DWORD)rcDest.bottom, dwHeightInPixels);

    // Lines above the destination rectangle.
    for (; y < (DWORD)rcDest.top; y++)
    {
        CopyMemory(pDest, pSrc, dwWidthInPixels * 2);
        pSrc += lSrcStride;
        pDest += lDestStride;
    }

    // Lines within the destination rectangle.
    for (; y < y0; y++)
    {
        const WORD *pSrc_Pixel = reinterpret_cast<const WORD*>(pSrc);
        WORD *pDest_Pixel = reinterpret_cast<WORD*>(pDest);

        CopyMemory(pDest, pSrc, left * 2);
        for (DWORD x = left; (x + 1) < right; x += 2)
        {
            // Byte order is Y0 U0 Y1 V0
            // Each WORD is a byte pair (Y, U/V)
            // Windows is little-endian so the order appears reversed.

            DWORD tmp = *reinterpret_cast<const DWORD*>(&pSrc_Pixel[x]);
            *reinterpret_cast<DWORD*>(&pDest_Pixel[x]) = (tmp & 0xFF00FF00) | 0x00800080;
        }
        CopyMemory(pDest + (right * 2), pSrc + (right * 2), (dwWidthInPixels - right) * 2);

        pDest += lDestStride;
        pSrc += lSrcStride;
    }

    // Lines below the destination rectangle.
    for (; y < dwHeightInPixels; y++)
    {
        CopyMemory(pDest, pSrc, dwWidthInPixels * 2);
        pSrc += lSrcStride;
        pDest += lDestStride;
    }
}


// Convert YUY2 image.

void TransformImage_YUY2(
    const RECT &rcDest,
    _Inout_updates_(_Inexpressible_(lDestStride * dwHeightInPixels)) BYTE *pDest,
    _In_ LONG lDestStride,
    _In_reads_(_Inexpressible_(lSrcStride * dwHeightInPixels)) const BYTE *pSrc,
    _In_ LONG lSrcStride,
    _In_ DWORD dwWidthInPixels,
    _In_ DWORD dwHeightInPixels)
{
    DWORD y = 0;
    // Round down to the even value.
    const UINT32 left = rcDest.left & ~(1);
    const UINT32 right = rcDest.right & ~(1);
    const DWORD y0 = min((DWORD)rcDest.bottom, dwHeightInPixels);

    // Lines above the destination rectangle.
    for (; y < (DWORD)rcDest.top; y++)
    {
        CopyMemory(pDest, pSrc, dwWidthInPixels * 2);
        pSrc += lSrcStride;
        pDest += lDestStride;
    }

    // Lines within the destination rectangle.
    for (; y < y0; y++)
    {
        const WORD *pSrc_Pixel = reinterpret_cast<const WORD*>(pSrc);
        WORD *pDest_Pixel = reinterpret_cast<WORD*>(pDest);

        CopyMemory(pDest, pSrc, left * 2);
        for (DWORD x = left; (x + 1) < right; x += 2)
        {
            // Byte order is Y0 U0 Y1 V0
            // Each WORD is a byte pair (Y, U/V)
            // Windows is little-endian so the order appears reversed.

            DWORD tmp = *reinterpret_cast<const DWORD*>(&pSrc_Pixel[x]);
            *reinterpret_cast<DWORD*>(&pDest_Pixel[x]) = (tmp & 0x00FF00FF) | 0x80008000;
        }
        CopyMemory(pDest + (right * 2), pSrc + (right * 2), (dwWidthInPixels - right) * 2);

        pDest += lDestStride;
        pSrc += lSrcStride;
    }

    // Lines below the destination rectangle.
    for (; y < dwHeightInPixels; y++)
    {
        CopyMemory(pDest, pSrc, dwWidthInPixels * 2);
        pSrc += lSrcStride;
        pDest += lDestStride;
    }
}

// Convert NV12 image

void TransformImage_NV12(
    const RECT &rcDest,
    _Inout_updates_(_Inexpressible_(2 * lDestStride * dwHeightInPixels)) BYTE *pDest,
    _In_ LONG lDestStride,
    _In_reads_(_Inexpressible_(2 * lSrcStride * dwHeightInPixels)) const BYTE *pSrc,
    _In_ LONG lSrcStride,
    _In_ DWORD dwWidthInPixels,
    _In_ DWORD dwHeightInPixels)
{
    // NV12 is planar: Y plane, followed by packed U-V plane.

    // Y plane
    for (DWORD y = 0; y < dwHeightInPixels; y++)
    {
        CopyMemory(pDest, pSrc, dwWidthInPixels);
        pDest += lDestStride;
        pSrc += lSrcStride;
    }

    // U-V plane

    // NOTE: The U-V plane has 1/2 the number of lines as the Y plane.

    // Lines above the destination rectangle.
    DWORD y = 0;
    const DWORD y0 = min((DWORD)rcDest.bottom, dwHeightInPixels);

    for (; y < (DWORD)rcDest.top / 2; y++)
    {
        CopyMemory(pDest, pSrc, dwWidthInPixels);
        pSrc += lSrcStride;
        pDest += lDestStride;
    }

    // Lines within the destination rectangle.
    for (; y < y0 / 2; y++)
    {
        CopyMemory(pDest, pSrc, rcDest.left);
        FillMemory(pDest + rcDest.left, rcDest.right - rcDest.left, 128);
        CopyMemory(pDest + rcDest.right, pSrc + rcDest.right, dwWidthInPixels - rcDest.right);
        pDest += lDestStride;
        pSrc += lSrcStride;
    }

    // Lines below the destination rectangle.
    for (; y < dwHeightInPixels / 2; y++)
    {
        CopyMemory(pDest, pSrc, dwWidthInPixels);
        pSrc += lSrcStride;
        pDest += lDestStride;
    }
}



void TransformImage_RGB32(
    const RECT &rcDest,
    _Inout_updates_(_Inexpressible_(2 * lDestStride * dwHeightInPixels)) BYTE *pDest,
    _In_ LONG lDestStride,
    _In_reads_(_Inexpressible_(2 * lSrcStride * dwHeightInPixels)) const BYTE *pSrc,
    _In_ LONG lSrcStride,
    _In_ DWORD dwWidthInPixels,
    _In_ DWORD dwHeightInPixels)
{
    DWORD y = 0;
    const DWORD y0 = min((DWORD)rcDest.bottom, dwHeightInPixels);

    // Lines above the destination rectangle.
    for (; y < (DWORD)rcDest.top; y++)
    {
        memcpy(pDest, pSrc, dwWidthInPixels * 4);
        pSrc += lSrcStride;
        pDest += lDestStride;
    }

    // Lines within the destination rectangle.
    // Use the grayscale conversion Red = 30%, Green = 59%, Blue = 11%
    for (; y < y0; y++)
    {
        DWORD *pSrc_Pixel = (DWORD*)pSrc;
        DWORD *pDest_Pixel = (DWORD*)pDest;

        for (DWORD x = 0; (x + 1) < dwWidthInPixels; x++)
        {
            // Byte order is X8 R8 G8 B8

            if (x >= (DWORD)rcDest.left && x < (DWORD)rcDest.right)
            {
                WORD color;
                color = (pSrc_Pixel[x] & 0x000000FF);
                int r = (int)(color * 0.3);
                color = (pSrc_Pixel[x] & 0x0000FF00) >> 8;
                int g = (int)(color * 0.59);
                color = (pSrc_Pixel[x] & 0x00FF0000) >> 16;
                int b = (int)(color * 0.11);
                int gray = r + g + b;

                pDest_Pixel[x] = (gray & 0x000000FF) << 16 |
                    (gray & 0x000000FF) << 8 |
                    (gray & 0x000000FF);
            }
            else
            {
#pragma warning(push)
#pragma warning(disable: 6385)
#pragma warning(disable: 6386)
                pDest_Pixel[x] = pSrc_Pixel[x];
#pragma warning(pop)
            }
        }

        pDest += lDestStride;
        pSrc += lSrcStride;
    }

    // Lines below the destination rectangle.
    for (; y < dwHeightInPixels; y++)
    {
        memcpy(pDest, pSrc, dwWidthInPixels * 4);
        pSrc += lSrcStride;
        pDest += lDestStride;
    }
}

#endif

HRESULT GetDXGIAdapterLuid(_In_ IMFDXGIDeviceManager *pDXManager, _Out_ LUID &AdapterLuid)
{
    HRESULT hr = S_OK;
    ComPtr<ID3D11Device> spDevice;
    ComPtr<IDXGIDevice> spDXGIDevice;
    ComPtr<IDXGIAdapter> spDXGIAdapter;
    DXGI_ADAPTER_DESC adapterDesc = { 0 };
    HANDLE hDevice = NULL;

    DMFTCHECKNULL_GOTO(pDXManager, done, E_INVALIDARG);
  
    DMFTCHECKHR_GOTO(pDXManager->OpenDeviceHandle(&hDevice), done);
    DMFTCHECKHR_GOTO(pDXManager->GetVideoService(hDevice, IID_PPV_ARGS(&spDevice)), done);
    DMFTCHECKHR_GOTO(spDevice->QueryInterface(IID_PPV_ARGS(&spDXGIDevice)), done);
    DMFTCHECKHR_GOTO(spDXGIDevice->GetAdapter(&spDXGIAdapter), done);
    DMFTCHECKHR_GOTO(spDXGIAdapter->GetDesc(&adapterDesc), done);

    CopyMemory(&AdapterLuid, &adapterDesc.AdapterLuid, sizeof(LUID));

done:
    if (NULL != hDevice)
    {
        pDXManager->CloseDeviceHandle(hDevice);
    }

    return hr;
}

HRESULT EnumSWDecoder(_Outptr_ IMFTransform** ppTransform, _In_ GUID subType)
{
    HRESULT hr = S_OK;
    IMFActivate** ppActivate = nullptr;
    UINT32 count = 0;
    ComPtr<IMFTransform> spTransform;
    LPWSTR pszFriendlyName = nullptr;

    MFT_REGISTER_TYPE_INFO info = { MFMediaType_Video, subType };
    UINT32 unFlags = MFT_ENUM_FLAG_SYNCMFT |
        MFT_ENUM_FLAG_LOCALMFT |
        MFT_ENUM_FLAG_SORTANDFILTER;

    DMFTCHECKNULL_GOTO(ppTransform, done, E_INVALIDARG);
    *ppTransform = nullptr;

    DMFTCHECKHR_GOTO(MFTEnumEx(MFT_CATEGORY_VIDEO_DECODER, unFlags,
        &info,      // Input type
        NULL,       // Output type
        &ppActivate,
        &count
    ), done);


    if (count == 0)
    {
        DMFTCHECKHR_GOTO(MF_E_TOPO_CODEC_NOT_FOUND, done);
    }

    if (SUCCEEDED(MFGetAttributeString(ppActivate[0], MFT_FRIENDLY_NAME_Attribute, &pszFriendlyName)))
    {
        //
        // Log the friendly name of the decoder
        //
        DMFTRACE(DMFT_GENERAL, TRACE_LEVEL_INFORMATION, "%!FUNC! decoder name: %S ", (pszFriendlyName ? pszFriendlyName : L"UnknownCodec"));
        SAFE_COTASKMEMFREE(pszFriendlyName);
    }

    DMFTCHECKHR_GOTO(ppActivate[0]->ActivateObject(IID_PPV_ARGS(spTransform.GetAddressOf())), done);

    *ppTransform = spTransform.Detach();

done:
    if (ppActivate)
    {
        for (UINT32 uiIndex = 0; uiIndex < count; uiIndex++)
        {
            ppActivate[uiIndex]->Release();
        }
        CoTaskMemFree(ppActivate);
    }
    return hr;
}
//
// @@@@ README Create the decoder
//
HRESULT CreateDecoderFromLuid( _In_ LUID ullAdapterLuidRunningOn,
    _In_ IMFMediaType* pInputType,
    _In_ IMFMediaType* pOutputType,
    _Inout_ BOOL fHwMft,
    _Outptr_ IMFTransform** ppDecoder)
{
    HRESULT hr = S_OK;
    ComPtr<IMFAttributes> spAttribs;
    ComPtr<IMFTransform> spMJPGDecoder;
    IMFActivate **ppActivates = nullptr;
    DWORD dwFlags = MFT_ENUM_FLAG_SYNCMFT | MFT_ENUM_FLAG_ASYNCMFT | MFT_ENUM_FLAG_SORTANDFILTER | MFT_ENUM_FLAG_HARDWARE | 0x10000000 /*Internal attribute*/;
    UINT32 cMFTActivate = 0;
    MFT_REGISTER_TYPE_INFO InputType;
    MFT_REGISTER_TYPE_INFO OutputType;
    GUID                   compressedGUID;
    ComPtr<ID3D11Device>  spD3D11Device;

    fHwMft = FALSE;

    
    DMFTCHECKNULL_GOTO(pInputType, done, E_INVALIDARG);
    DMFTCHECKNULL_GOTO(pOutputType, done, E_INVALIDARG);
    DMFTCHECKNULL_GOTO(ppDecoder, done, E_INVALIDARG);
    *ppDecoder = nullptr;
    DMFTCHECKHR_GOTO(pInputType->GetGUID(MF_MT_SUBTYPE, &compressedGUID), done);

    InputType.guidMajorType = MFMediaType_Video;
    InputType.guidSubtype = compressedGUID;

    DMFTCHECKHR_GOTO(pOutputType->GetMajorType(&OutputType.guidMajorType), done);
    DMFTCHECKHR_GOTO(pOutputType->GetGUID(MF_MT_SUBTYPE, &OutputType.guidSubtype), done);

    DMFTCHECKHR_GOTO(MFCreateAttributes(&spAttribs, 1), done);
    DMFTCHECKHR_GOTO(spAttribs->SetBlob(MFT_ENUM_ADAPTER_LUID, (byte*)&ullAdapterLuidRunningOn, sizeof(ullAdapterLuidRunningOn)), done);
    DMFTCHECKHR_GOTO(MFTEnum2(MFT_CATEGORY_VIDEO_DECODER, dwFlags, &InputType, &OutputType, spAttribs.Get(), &ppActivates, &cMFTActivate), done);

    for (DWORD i = 0; i < cMFTActivate; i++)
    {
        HRESULT hrTemp = ppActivates[i]->ActivateObject(IID_PPV_ARGS(spMJPGDecoder.GetAddressOf()));
        fHwMft = (MFT_ENUM_FLAG_HARDWARE & MFGetAttributeUINT32(ppActivates[i], MF_TRANSFORM_FLAGS_Attribute, 0));
        // Pickup the first MFT enumerated. Normally, it should be a HW MFT, if HWMFT is available.
        if (SUCCEEDED(hrTemp))
        {
            break;
        }
        else
        {
            spMJPGDecoder = nullptr;
        }
    }
    if (spMJPGDecoder == nullptr)
    {
        hr = MF_E_NOT_FOUND;
    }
    *ppDecoder = spMJPGDecoder.Detach();
done:

    for (UINT32 i = 0; i < cMFTActivate; i++)
    {
        if (nullptr != ppActivates[i])
        {
            ppActivates[i]->Release();
        }
    }
    SAFE_COTASKMEMFREE(ppActivates);
    return hr;
}

HRESULT SetDX11BindFlags( _In_  IUnknown *pUnkManager,
    _In_ GUID guidPinCategory,
    _Inout_ DWORD &dwBindFlags)
{
    HRESULT hr = S_OK;
    ComPtr<IUnknown> spDeviceManager;
    BOOL fFL10 = FALSE;
    
    auto
        IsFeatureLevel10OrBetter = [&]() -> BOOL 
    {
        ComPtr<IMFDXGIDeviceManager> spDXGIManager;
        ComPtr<ID3D11Device> spDevice;
        HANDLE hDevice = 0;
        BOOL fRet = FALSE;
        UINT level = 0, result = 0;
        
        if (SUCCEEDED(pUnkManager->QueryInterface(IID_PPV_ARGS(&spDXGIManager))) &&
            SUCCEEDED(spDXGIManager->OpenDeviceHandle(&hDevice)) &&
            SUCCEEDED(spDXGIManager->LockDevice(hDevice, IID_PPV_ARGS(&spDevice), FALSE)))
        {

            level = (UINT)spDevice->GetFeatureLevel();
            fRet = (level >= D3D_FEATURE_LEVEL_10_0);
            if (!fRet)
            {
                if (SUCCEEDED(spDevice->CheckFormatSupport(DXGI_FORMAT_NV12, &result)))
                {
                    fRet = (result & D3D11_FORMAT_SUPPORT_TEXTURE2D) && (result & D3D11_FORMAT_SUPPORT_RENDER_TARGET);
                }
            }
            (void)spDXGIManager->UnlockDevice(hDevice, FALSE);
            (void)spDXGIManager->CloseDeviceHandle(hDevice);
        }
        return fRet;
    };

    DMFTCHECKNULL_GOTO(pUnkManager, done, E_INVALIDARG);
    fFL10 = IsFeatureLevel10OrBetter();

    dwBindFlags |= fFL10 ? D3D11_BIND_SHADER_RESOURCE : 0;
    dwBindFlags |= (guidPinCategory != PINNAME_VIDEO_PREVIEW) ? D3D11_BIND_VIDEO_ENCODER : 0;
    if ((dwBindFlags != 0) && ((dwBindFlags & ~(D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_VIDEO_ENCODER)) == 0))
    {
        dwBindFlags |= D3D11_BIND_RENDER_TARGET;
    }
done:
    return hr;
}

// @@@@ README: Create the Decoder
HRESULT CreateDecoderHW(_In_ IMFDXGIDeviceManager* pManager,
    _In_ IMFMediaType* inType,
    _In_ IMFMediaType* outType,
    _Outptr_ IMFTransform** ppTransform,
    _Inout_  BOOL&           fHwMft)
{
    HRESULT hr = S_OK;
    LUID luid;
    ComPtr<IMFTransform> spTransform;
    DMFTCHECKNULL_GOTO(ppTransform, done, E_INVALIDARG);
    *ppTransform = nullptr;

    if (pManager)
    {
        if (SUCCEEDED(hr = GetDXGIAdapterLuid(pManager, luid)))
        {
            hr = CreateDecoderFromLuid(luid, inType, outType, fHwMft, spTransform.GetAddressOf());
            if (FAILED(hr))
            {
                DMFTRACE(DMFT_GENERAL, TRACE_LEVEL_INFORMATION, "%!FUNC! Error creating HW Decoder with LUID %d-%d", luid.HighPart, luid.LowPart);
                DMFTCHECKHR_GOTO(hr, done);
            }
        }
    }
    *ppTransform = spTransform.Detach();
done:
    return hr;
}

HRESULT
SubtypeToDXGIFormat(
    _In_    GUID subType,
    _Inout_ DXGI_FORMAT &format
)
{
    HRESULT hr = S_OK;
    typedef struct {
        GUID subType;
        DXGI_FORMAT format;
    }DXGIFormatMap;
    DXGIFormatMap formatMap[]
    {
    { MFVideoFormat_NV12,      DXGI_FORMAT_NV12 },
    { MFVideoFormat_YUY2,      DXGI_FORMAT_YUY2 },
    { MFVideoFormat_RGB32,      DXGI_FORMAT_B8G8R8X8_UNORM },
    { MFVideoFormat_ARGB32,     DXGI_FORMAT_B8G8R8A8_UNORM },
    { MFVideoFormat_AYUV,      DXGI_FORMAT_AYUV },
    { MFVideoFormat_NV11,      DXGI_FORMAT_NV11 },
    { MFVideoFormat_AI44,      DXGI_FORMAT_AI44 },
    { MFVideoFormat_P010,      DXGI_FORMAT_P010 },
    { MFVideoFormat_P016,      DXGI_FORMAT_P016 },
    { MFVideoFormat_Y210,      DXGI_FORMAT_Y210 },
    { MFVideoFormat_Y216,      DXGI_FORMAT_Y216 },
    { MFVideoFormat_Y410,      DXGI_FORMAT_Y410 },
    { MFVideoFormat_Y416,      DXGI_FORMAT_Y416 },
    };
    format = DXGI_FORMAT_UNKNOWN;
    for (UINT32 uiIndex = 0; uiIndex < _countof(formatMap); uiIndex++)
    {
        if (formatMap[uiIndex].subType == subType)
        {
            format = formatMap[uiIndex].format;
            break;
        }
    }
    // compressed media type or unsupported of the range
    hr = (format == DXGI_FORMAT_UNKNOWN) ? E_NOTIMPL : S_OK;
    return hr;
}

HRESULT IsDXFormatSupported(
    _In_ IMFDXGIDeviceManager* pDeviceManager ,
    _In_ GUID subType,
    _Outptr_opt_ ID3D11Device** ppDevice,
    _In_opt_ PUINT32 pSupportedFormat)
{
    HRESULT hr = S_OK;
    DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
    UINT32 supportedFormat = 0;
    HANDLE hDevice = NULL;
    ComPtr<ID3D11Device> spDevice;
    DMFTCHECKNULL_GOTO(pDeviceManager, done, E_INVALIDARG);
    DMFTCHECKHR_GOTO(pDeviceManager->OpenDeviceHandle(&hDevice), done);
    DMFTCHECKHR_GOTO(pDeviceManager->GetVideoService(hDevice, IID_PPV_ARGS(&spDevice)), done);
    DMFTCHECKHR_GOTO(SubtypeToDXGIFormat(subType, format), done);
    DMFTCHECKHR_GOTO(spDevice->CheckFormatSupport(format, &supportedFormat), done);
    if (pSupportedFormat)
    {
        *pSupportedFormat = supportedFormat;
    }
    if (ppDevice)
    {
        *ppDevice = spDevice.Detach();
    }
done:
    if (hDevice != NULL)
    {
        pDeviceManager->CloseDeviceHandle(hDevice);
    }
    return hr;
}

HRESULT
UpdateAllocatorAttributes(
    _In_ IMFAttributes *pAttributes,
    _In_ REFGUID guidStreamCategory,
    _In_ GUID subType,
    _In_ IMFDXGIDeviceManager *pDeviceManager
)
{
    HRESULT hr = S_OK;
    UINT32 supportedFormat = 0;
    ComPtr<ID3D11Device> spDevice;
    DWORD dwBindFlags = 0;

    DMFTCHECKHR_GOTO(IsDXFormatSupported(pDeviceManager,subType,
        spDevice.GetAddressOf(),&supportedFormat),done);
 
    // MF_SA_D3D11_USAGE should be D3D11_USAGE_DEFAULT, same as previous release.
    DMFTCHECKHR_GOTO(pAttributes->SetUINT32(MF_SA_D3D11_USAGE, D3D11_USAGE_DEFAULT), done);

    DMFTCHECKHR_GOTO(pAttributes->SetUINT32(MF_SA_D3D11_SHARED_WITHOUT_MUTEX, TRUE), done);
    DMFTCHECKHR_GOTO(pAttributes->SetUINT32(MF_SA_D3D11_SHARED_WITH_NTHANDLE_PRIVATE, TRUE), done);

    // Bind Flags
    dwBindFlags |= ((supportedFormat & D3D11_FORMAT_SUPPORT_RENDER_TARGET) != 0 ? D3D11_BIND_RENDER_TARGET : 0);

    if (((supportedFormat & D3D11_FORMAT_SUPPORT_VIDEO_ENCODER) != 0) &&
        (!IsEqualGUID(guidStreamCategory, PINNAME_VIDEO_PREVIEW)) &&
        IsEqualGUID(subType, MFVideoFormat_NV12))
    {
        D3D11_FEATURE_DATA_D3D11_OPTIONS4 Options = { 0 };

        // "D3D11_BIND_VIDEO_ENCODER + sharing" is only supported on those devices that
        // advertise ExtendedNV12SharedTextureSupported.
        DMFTCHECKHR_GOTO(spDevice->CheckFeatureSupport(D3D11_FEATURE_D3D11_OPTIONS4, &Options, sizeof(Options)), done);
        if (Options.ExtendedNV12SharedTextureSupported)
        {
            dwBindFlags |= D3D11_BIND_VIDEO_ENCODER;
        }
    }

    // D3D requires one of the flags (D3D11_BIND_RENDER_TARGET, D3D11_PRIVATE_BIND_CAPTURE, or D3D11_BIND_VIDEO_ENCODER) to be set for non-zero bind flags.
    if (dwBindFlags != 0)
    {
        D3D_FEATURE_LEVEL level;

        // D3D11_BIND_SHADER_RESOURCE
        level = spDevice->GetFeatureLevel();
        dwBindFlags |= ((level >= D3D_FEATURE_LEVEL_10_0) ? D3D11_BIND_SHADER_RESOURCE : 0);
    }

    DMFTCHECKHR_GOTO(pAttributes->SetUINT32(MF_SA_D3D11_BINDFLAGS, dwBindFlags), done);

done:
    return hr;
}

const UINT32 ALLOCATOR_MIN_SAMPLES = 10;
const UINT32 ALLOCATOR_MAX_SAMPLES = 50;

HRESULT ConfigureAllocator(
    _In_ IMFMediaType* pOutputMediaType,
    _In_ GUID streamCategory,
    _In_ IUnknown* pDeviceManagerUnk,
    _In_ BOOL &bDxAllocator,
    _In_ IMFVideoSampleAllocator* pAllocator)
{
    HRESULT hr = S_OK;
    wil::com_ptr_nothrow<IMFVideoSampleAllocatorEx> spPrivateAllocator;
#if ((defined NTDDI_WIN10_VB) && (NTDDI_VERSION >= NTDDI_WIN10_VB))
    wil::com_ptr_nothrow<IMFVideoCaptureSampleAllocator> spDefaultAllocator;
#endif
    wil::com_ptr_nothrow<IMFAttributes> spAllocatorAttributes;
    GUID guidMajorType = GUID_NULL;
    GUID guidSubtype = GUID_NULL;
    BOOL fDXAllocator = FALSE;

    RETURN_HR_IF_NULL(E_INVALIDARG, pAllocator );
    RETURN_HR_IF_NULL(E_INVALIDARG, pOutputMediaType );

    RETURN_IF_FAILED(pOutputMediaType->GetMajorType(&guidMajorType));

    if (!IsEqualGUID(guidMajorType, MFMediaType_Video))
    {
        RETURN_HR(MF_E_INVALIDMEDIATYPE);
    }

    RETURN_IF_FAILED(pOutputMediaType->GetGUID(MF_MT_SUBTYPE, &guidSubtype));
    //
    // Set Attributes on the allocator we need. First get the Bind Flags.
    //
    RETURN_IF_FAILED(MFCreateAttributes(&spAllocatorAttributes, 8));
    RETURN_IF_FAILED(spAllocatorAttributes->SetUINT32(MF_SA_BUFFERS_PER_SAMPLE, 1));

    if (pDeviceManagerUnk != nullptr)
    {
        if (SUCCEEDED(UpdateAllocatorAttributes(spAllocatorAttributes.get(), streamCategory, guidSubtype, (IMFDXGIDeviceManager*)pDeviceManagerUnk)))
        {
            fDXAllocator = TRUE;
        }
    }

    if (fDXAllocator)
    {
        RETURN_IF_FAILED(pAllocator->SetDirectXManager(pDeviceManagerUnk));
    }

    if (SUCCEEDED(pAllocator->QueryInterface(IID_PPV_ARGS(&spPrivateAllocator))))
    {
        RETURN_IF_FAILED(spPrivateAllocator->InitializeSampleAllocatorEx(
            ALLOCATOR_MIN_SAMPLES, 
            ALLOCATOR_MAX_SAMPLES, 
            spAllocatorAttributes.get(), 
            pOutputMediaType));
    }
#if ((defined NTDDI_WIN10_VB) && (NTDDI_VERSION >= NTDDI_WIN10_VB))
    else if (SUCCEEDED(pAllocator->QueryInterface(IID_PPV_ARGS(&spDefaultAllocator))))
    {
        RETURN_IF_FAILED(spDefaultAllocator->InitializeCaptureSampleAllocator(
            0, /*use sample size by MediaType*/
            0, /*metadata size*/
            0, /*default alignment*/
            ALLOCATOR_MIN_SAMPLES,
            spAllocatorAttributes.get(),
            pOutputMediaType));
    }
#endif
    else
    {
        hr = E_INVALIDARG;
    }

    bDxAllocator = fDXAllocator;

    return hr;
}
//
//@@@@ README: Creating an Allocator.. Please don't allocate samples individually using MFCreateSample as that can lead to fragmentation and is
// extremely inefficent. Instead create an Allocator which will create a fixed number of samples which are recycled when the pipeline returns back
// the buffers. The above has defines will enable to create a circular allocator which will have maximum 20 samples flowing in the pipeline
// This way we can also catch samples leaks if any.
//
HRESULT CreateAllocator( _In_ IMFMediaType* pOutputMediaType,
    _In_ GUID streamCategory,
    _In_ IUnknown* pDeviceManagerUnk,
    _In_ BOOL &bDxAllocator,
    _Outptr_ IMFVideoSampleAllocatorEx** ppAllocator )
{
    HRESULT hr = S_OK;
    ComPtr<IMFVideoSampleAllocatorEx> spVideoSampleAllocator;
    ComPtr<IMFAttributes> spAllocatorAttributes;
    GUID guidMajorType = GUID_NULL;
    GUID guidSubtype = GUID_NULL;
    BOOL fDXAllocator = FALSE;

    DMFTCHECKNULL_GOTO(ppAllocator, done, E_INVALIDARG);
    DMFTCHECKNULL_GOTO(pOutputMediaType, done, E_INVALIDARG);
    DMFTCHECKHR_GOTO(pOutputMediaType->GetMajorType(&guidMajorType), done);

    *ppAllocator = nullptr;
    if (!IsEqualGUID(guidMajorType, MFMediaType_Video))
    {
        DMFTCHECKHR_GOTO(MF_E_INVALIDMEDIATYPE, done);
    }

    DMFTCHECKHR_GOTO(pOutputMediaType->GetGUID(MF_MT_SUBTYPE, &guidSubtype), done);
    //
    // Set Attributes on the allocator we need. First get the Bind Flags.
    //
    DMFTCHECKHR_GOTO(MFCreateAttributes(&spAllocatorAttributes, 8), done);
    DMFTCHECKHR_GOTO(spAllocatorAttributes->SetUINT32(MF_SA_BUFFERS_PER_SAMPLE, 1), done);

    if (pDeviceManagerUnk != nullptr)
    {
         if (SUCCEEDED(UpdateAllocatorAttributes(spAllocatorAttributes.Get(), streamCategory,guidSubtype, (IMFDXGIDeviceManager*)pDeviceManagerUnk)))
         {
             fDXAllocator = TRUE;
         }
    }
    DMFTCHECKHR_GOTO(MFCreateVideoSampleAllocatorEx(IID_PPV_ARGS(spVideoSampleAllocator.ReleaseAndGetAddressOf())), done);
    if (fDXAllocator)
    {
        DMFTCHECKHR_GOTO(spVideoSampleAllocator->SetDirectXManager(pDeviceManagerUnk), done);
    }

    DMFTCHECKHR_GOTO(spVideoSampleAllocator->InitializeSampleAllocatorEx(ALLOCATOR_MIN_SAMPLES, ALLOCATOR_MAX_SAMPLES, spAllocatorAttributes.Get(), pOutputMediaType), done);
    
    *ppAllocator = spVideoSampleAllocator.Detach();
    bDxAllocator = fDXAllocator;
done:
    return hr;
}

HRESULT CheckPassthroughMediaType(_In_ IMFMediaType *pMediaType1,
    _In_ IMFMediaType *pMediaType2,
    _Out_ BOOL& pfPassThrough)
{
    HRESULT hr = S_OK;
    BOOL fPassThrough = FALSE;
    UINT32 uWidth, uHeight;
    UINT32 uWidth2, uHeight2;

    // Compare Width/Height.
    DMFTCHECKHR_GOTO(MFGetAttributeSize(pMediaType1, MF_MT_FRAME_SIZE, &uWidth, &uHeight), done);
    DMFTCHECKHR_GOTO(MFGetAttributeSize(pMediaType2, MF_MT_FRAME_SIZE, &uWidth2, &uHeight2), done);
    if ((uWidth != uWidth2) || (uHeight != uHeight2))
    {
        goto done;
    }

    // Compare rotation
    {
        UINT32 uRotation;
        UINT32 uRotation2;

        uRotation = MFGetAttributeUINT32(pMediaType1, MF_MT_VIDEO_ROTATION, 0);
        uRotation2 = MFGetAttributeUINT32(pMediaType2, MF_MT_VIDEO_ROTATION, 0);
        if (uRotation != uRotation2)
        {
            goto done;
        }
    }

    // Compare Aspect Ratio
    {
        UINT32 uNumerator = 1, uDenominator = 1;
        UINT32 uNumerator2 = 1, uDenominator2 = 1;

        (void)MFGetAttributeRatio(pMediaType1, MF_MT_FRAME_RATE, &uNumerator, &uDenominator);
        (void)MFGetAttributeRatio(pMediaType2, MF_MT_FRAME_RATE, &uNumerator2, &uDenominator2);
        if ((uNumerator * uDenominator2) != (uNumerator2 * uDenominator))
        {
            goto done;
        }
    }

    // Compare Nominal Range
    {
        UINT32 uNorminalRange;
        UINT32 uNorminalRange2;

        uNorminalRange = MFGetAttributeUINT32(pMediaType1, MF_MT_VIDEO_NOMINAL_RANGE, MFNominalRange_Unknown);
        uNorminalRange2 = MFGetAttributeUINT32(pMediaType2, MF_MT_VIDEO_NOMINAL_RANGE, MFNominalRange_Unknown);
        if (uNorminalRange != uNorminalRange2)
        {
            goto done;
        }
    }

    // Compare color primaries
    {
        UINT32 uPrimaries;
        UINT32 uPrimaries2;

        uPrimaries = MFGetAttributeUINT32(pMediaType1, MF_MT_VIDEO_PRIMARIES, MFVideoPrimaries_Unknown);
        uPrimaries2 = MFGetAttributeUINT32(pMediaType2, MF_MT_VIDEO_PRIMARIES, MFVideoPrimaries_Unknown);
        if (uPrimaries != uPrimaries2)
        {
            goto done;
        }
    }


    // Compare InterlaceMode
    {
        UINT32 uInterlaceMode;
        UINT32 uInterlaceMode2;

        uInterlaceMode = MFGetAttributeUINT32(pMediaType1, MF_MT_INTERLACE_MODE, MFVideoInterlace_Unknown);
        uInterlaceMode2 = MFGetAttributeUINT32(pMediaType2, MF_MT_INTERLACE_MODE, MFVideoInterlace_Unknown);
        if (uInterlaceMode != uInterlaceMode2)
        {
            goto done;
        }
    }

    // Compare display aperture
    {
        MFVideoArea VideoArea = { 0 };
        MFVideoArea VideoArea2 = { 0 };
        UINT32 cbBlobSize = 0;

        if (SUCCEEDED(pMediaType1->GetBlob(MF_MT_MINIMUM_DISPLAY_APERTURE, (UINT8*)&VideoArea, sizeof(MFVideoArea), &cbBlobSize)))
        {
            if ((VideoArea.OffsetX.value == 0 && VideoArea.OffsetX.fract == 0) &&
                (VideoArea.OffsetY.value == 0 && VideoArea.OffsetY.fract == 0) &&
                (VideoArea.Area.cx == (LONG)uWidth) &&
                (VideoArea.Area.cy == (LONG)uHeight))
            {
                // If the display aperture is the whole frame size, ignore it.
                ZeroMemory(&VideoArea, sizeof(VideoArea));
            }
        }

        if (SUCCEEDED(pMediaType2->GetBlob(MF_MT_MINIMUM_DISPLAY_APERTURE, (UINT8*)&VideoArea2, sizeof(MFVideoArea), &cbBlobSize)))
        {
            if ((VideoArea2.OffsetX.value == 0 && VideoArea2.OffsetX.fract == 0) &&
                (VideoArea2.OffsetY.value == 0 && VideoArea2.OffsetY.fract == 0) &&
                (VideoArea2.Area.cx == (LONG)uWidth) &&
                (VideoArea2.Area.cy == (LONG)uHeight))
            {
                // If the display aperture is the whole frame size, ignore it.
                ZeroMemory(&VideoArea, sizeof(VideoArea));
            }
        }

        if (memcmp(&VideoArea, &VideoArea2, sizeof(MFVideoArea)) != 0)
        {
            goto done;
        }
    }

    fPassThrough = TRUE;

done:
    pfPassThrough = fPassThrough;
    return hr;
}


HRESULT MergeSampleAttributes( _In_ IMFSample* pInSample, _Inout_ IMFSample* pOutSample)
{
    HRESULT hr = S_OK;
    UINT32 cAttributes = 0;
    GUID guidAttribute;
    PROPVARIANT varAttribute;
    PROPVARIANT varAttributeExists;
    PropVariantInit(&varAttribute);
    PropVariantInit(&varAttributeExists);

    DMFTCHECKNULL_GOTO(pInSample, done, E_INVALIDARG);
    DMFTCHECKNULL_GOTO(pOutSample, done, E_INVALIDARG);

    DMFTCHECKHR_GOTO(pInSample->GetCount(&cAttributes), done);
    for (UINT32 i = 0; i < cAttributes; i++)
    {
        PropVariantClear(&varAttribute);
        PropVariantClear(&varAttributeExists);

        DMFTCHECKHR_GOTO(pInSample->GetItemByIndex(i, &guidAttribute, &varAttribute), done);

        if (S_OK == pOutSample->GetItem(guidAttribute, &varAttributeExists))
        {
            // Exists.. dont clobber    
            continue;
        }

        DMFTCHECKHR_GOTO(pOutSample->SetItem(guidAttribute, varAttribute), done);
    }

done:
    PropVariantClear(&varAttribute);
    PropVariantClear(&varAttributeExists);
    return hr;
}
