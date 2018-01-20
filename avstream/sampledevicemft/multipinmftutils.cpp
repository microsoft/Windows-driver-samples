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
    Note: This iteration doesn't support decoder. The next one will and this function will accordingly change
*/
STDMETHODIMP CompareMediaTypesForXVP(
    _In_opt_ IMFMediaType *inMediaType,
    _In_    IMFMediaType                *newMediaType,
    _Inout_ MF_TRANSFORM_XVP_OPERATION  *operation 
    )
{
    UINT32  unWidthin, unHeightin, unWidthNew, unHeightNew = 0;
    HRESULT hr          = S_OK;
    GUID    guidTypeA   = GUID_NULL;
    GUID    guidTypeB   = GUID_NULL;
    
    *operation = DeviceMftTransformXVPIllegal;
    if ((!inMediaType) || (!newMediaType))
    {
       goto done;
    }

    DMFTCHECKHR_GOTO( MFGetAttributeSize( inMediaType, MF_MT_FRAME_SIZE, &unWidthin, &unHeightin ), done );
    DMFTCHECKHR_GOTO( MFGetAttributeSize( newMediaType, MF_MT_FRAME_SIZE, &unWidthNew, &unHeightNew ), done );


    if ( SUCCEEDED( inMediaType->GetGUID(  MF_MT_MAJOR_TYPE, &guidTypeA ) ) &&
         SUCCEEDED( newMediaType->GetGUID( MF_MT_MAJOR_TYPE, &guidTypeB ) ) &&
        IsEqualGUID( guidTypeA, guidTypeB ) )
    {
        if ( SUCCEEDED( inMediaType->GetGUID ( MF_MT_SUBTYPE, &guidTypeA ) ) &&
             SUCCEEDED( newMediaType->GetGUID( MF_MT_SUBTYPE, &guidTypeB ) ) &&
            IsEqualGUID( guidTypeA, guidTypeB ) )
        {
            //Comparing the MF_MT_AM_FORMAT_TYPE for the directshow format guid
#if 0
            if (SUCCEEDED(inMediaType->GetGUID(MF_MT_AM_FORMAT_TYPE, &guidTypeA)) &&
                SUCCEEDED(newMediaType->GetGUID(MF_MT_AM_FORMAT_TYPE, &guidTypeB)) &&
                IsEqualGUID(guidTypeA, guidTypeB))
#endif
            {

                if (!(( unWidthin == unWidthNew ) &&
                    ( unHeightin == unHeightNew ) ) )
                {
                    if ( ( unWidthNew > unWidthin ) || ( unHeightNew > unHeightin ) )
                    {
                      *operation = DeviceMftTransformXVPDisruptiveIn; //Media type needs to change at input
                    }
                    else
                    {
                        *operation = DeviceMftTransformXVPDisruptiveOut; //Media type needs to change at output
                    }
                    goto done;
                }

                if ( MFGetAttributeUINT32( inMediaType,  MF_MT_SAMPLE_SIZE, 0 ) !=
                     MFGetAttributeUINT32( newMediaType, MF_MT_SAMPLE_SIZE, 0 ) )
                {
                    hr = S_FALSE; //Sample sizes differ. 
                    goto done;
                }
                else
                {
                    //Same media type.. No XVP needed or the current XVP is fine!
                    *operation = DeviceMftTransformXVPCurrent;
                }
            }
        }
        else
        {
            //This is a disruptive operation. Actually a decoder operation!
            *operation = DeviceMftTransformXVPDisruptiveIn;
        }
    }
 done:
    return hr;
}

/*++
Description:
    A Simple function to randomnize the media types supplied in an array
--*/
STDMETHODIMP RandomnizeMediaTypes(_In_ IMFMediaTypeArray &pMediaTypeArray)
{
    HRESULT hr = S_OK;
    IMFMediaType *pTempMediaType = nullptr;
    UNREFERENCED_PARAMETER(pTempMediaType);
    if (pMediaTypeArray.empty())
    {
        goto done;
    }

    srand(static_cast<unsigned long>(GetTickCount64()));
    
    for (DWORD dwIndex = (DWORD)pMediaTypeArray.size() - 1; dwIndex > 0; dwIndex--)
    {
        DWORD fromIndex = rand() % ( dwIndex + 1 );
        pTempMediaType					= pMediaTypeArray[dwIndex];
        pMediaTypeArray [ dwIndex ]		=  pMediaTypeArray[fromIndex] ;
        pMediaTypeArray [ fromIndex ]	=  pTempMediaType;
    }
    //
    //Just reverse the array
    //
    for (DWORD dwIndex = (DWORD)pMediaTypeArray.size() - 1, fromIndex = 0; dwIndex > 0; dwIndex--, fromIndex++)
    {
         
        pTempMediaType					= pMediaTypeArray[dwIndex];
        pMediaTypeArray [ dwIndex ]		= pMediaTypeArray[fromIndex];
        pMediaTypeArray [ fromIndex ]	=  pTempMediaType;
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
    a = new char[len]; \
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
            ansiguidStr = new char[mbGuidLen];
            WideCharToMultiByte(CP_ACP, WC_NO_BEST_FIT_CHARS, lpszGuidString, -1, ansiguidStr, mbGuidLen, NULL, NULL);
            CoTaskMemFree(lpszGuidString);
            ansiguidStr[mbGuidLen - 1] = 0;
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
    tempStr = new CHAR[256];
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
        m_pBuffer = new char[buffLen];
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