// Copyright (c) Microsoft Corporation
//
// Author:      Alper Selcuk
//
// Description:
//
//      Declaration of the AudioMediaType
//      The original code is copied from MF tree. 
//
//      Note that this implementation is only useful for some unit tests that need
//      to pass bogus media types.
//
#pragma once

#include <AudioMediaType.h>

//-----------------------------------------------------------------------------
// Description:
//
//  Implements a stripped down version of IAudioMediaType. 

// Remarks:
//
//  Most methods are not implemented, because they are not needed by the 
//  processor. 
//
//  NOTE: [alper] If this is passed to third party APO's then we may be forced
//  to implement all the methods.
//
class CTestAudioMediaType : public IAudioMediaType
{
public:
    
    //--------------------------------------------------------------------------
    // Description:
    //
    //   Used during creation
    //
    // Parameters:
    //
    //     pFormat - [in] the format
    //     ppIAudioMediaType - [out] Pointer to the created IAudioMediaType.
    //
    // Return Values:
    //
    //     S_OK         On success
    //     error        Otherwise
    //
    static HRESULT Create(  const UNCOMPRESSEDAUDIOFORMAT* pFormat,
                            IAudioMediaType** ppIAudioMediaType);

    // IUnknown methods
    ULONG STDMETHODCALLTYPE AddRef();
    ULONG STDMETHODCALLTYPE Release();
    STDMETHOD( QueryInterface )( REFIID riid, LPVOID *ppvObject );

    //--------------------------------------------------------------------------
    // Description:
    //
    //   Get Major Type
    //
    // Parameters:
    //
    //     pguidMajorType - [out] pointer
    //
    // Return Values:
    //
    //     E_NOTIMPL         Not supported
    //
    virtual HRESULT STDMETHODCALLTYPE GetMajorType(GUID * /* pguidMajorType */)
    {
        return E_NOTIMPL;
    }
       
    //--------------------------------------------------------------------------
    // Description:
    //
    //   Checks to see if the format is compressed.
    //
    // Parameters:
    //
    //     pfCompressed - [out] On S_OK, this is set TRUE.
    //
    // Return Values:
    //
    //     S_OK         On success
    //     E_POINTER    Bad pointer
    //
    virtual HRESULT STDMETHODCALLTYPE IsCompressedFormat(
        /* [out] */ BOOL* pfCompressed
        )
    {
        if (NULL == pfCompressed)
        {
            return( E_POINTER );
        }
        
        *pfCompressed = TRUE;

        return( S_OK );
    }

    //--------------------------------------------------------------------------
    // Description:
    //
    //   Checks to see if the media types are equal.
    //
    // Parameters:
    //
    //     pAudioMediaType - [in] Media type.
    //     pdwFlags - [out] flags
    //
    // Return Values:
    //
    //     S_OK         On success
    //     error        Otherwise
    //
    virtual HRESULT STDMETHODCALLTYPE IsEqual( IAudioMediaType *pAudioMediaType,DWORD *pdwFlags);
        
    //--------------------------------------------------------------------------
    // Description:
    //
    //   Gets the Audio Format.
    //
    // Results:
    //
    //     Returns a WAVEFORMATEX pointer.
    //
    virtual const WAVEFORMATEX *STDMETHODCALLTYPE GetAudioFormat( void);

    //--------------------------------------------------------------------------
    // Description:
    //
    //   Gets the Uncompressed Audio Format.
    //
    // Parameters:
    //
    //     pUncompressedAudioFormat - [out] gets the format.
    //
    // Return Values:
    //
    //     S_OK         On success
    //     error        Otherwise
    //
    virtual HRESULT STDMETHODCALLTYPE GetUncompressedAudioFormat( 
                        UNCOMPRESSEDAUDIOFORMAT* pUncompressedAudioFormat );
 
protected:
    CTestAudioMediaType();

protected:
    LONG                        m_cRef;
    UNCOMPRESSEDAUDIOFORMAT   m_Format;
    WAVEFORMATEXTENSIBLE        m_Wfx;
};

//-----------------------------------------------------------------------------
// Called on creation
inline CTestAudioMediaType::CTestAudioMediaType()
{
    m_cRef             = 0;
} // CTestAudioMediaType

