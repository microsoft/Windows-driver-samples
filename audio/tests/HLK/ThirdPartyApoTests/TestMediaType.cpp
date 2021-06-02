// AudioMediaType.cpp -- Copyright (c) 2003 Microsoft Corporation
//
//
// Description:
//
//      Implementation of the TestMediaType
//

#include "TestMediaType.h"
#include <memory>
#include <wil\resultmacros.h>

//-----------------------------------------------------------------------------
// Description:
//
//  Creates an CTestAudioMediaType object with the given format.
//
// Parameters:
//
//      pFormat - [in] Format for the new AudioMediaType.
//      ppIAudioMediaType - [out] upon success returns the pointer to the 
//                                new interface.
//
// Return values:
//  
//      S_OK            Success
//      E_OUTOFMEMORY   On memory failure.
//      E_INVALIDARG    Null parameter
//
// Remarks:
//
HRESULT CTestAudioMediaType::Create
(  
    const UNCOMPRESSEDAUDIOFORMAT* pFormat,
    IAudioMediaType** ppIAudioMediaType
)
{
    if (!pFormat || !ppIAudioMediaType)
    {
        return E_INVALIDARG;
    }

    HRESULT hResult = S_OK;

    *ppIAudioMediaType = NULL;

    std::unique_ptr<CTestAudioMediaType> pObj(new(std::nothrow) CTestAudioMediaType());

    pObj->m_Format = *pFormat;

    // TODO: iter8 fix this to use WAVEFORMATEXTENSIBLE (commented lines) 
    // after KSCommon lib is fixed
    if (KSDATAFORMAT_SUBTYPE_PCM ==  pFormat->guidFormatType)
    {
        pObj->m_Wfx.Format.wFormatTag = WAVE_FORMAT_PCM;
    }
    else
    {
        pObj->m_Wfx.Format.wFormatTag = WAVE_FORMAT_IEEE_FLOAT;
    }
    pObj->m_Wfx.Format.nChannels = (WORD)pObj->m_Format.dwSamplesPerFrame;
    pObj->m_Wfx.Format.nSamplesPerSec = (DWORD)pObj->m_Format.fFramesPerSecond;
    pObj->m_Wfx.Format.nAvgBytesPerSec = (DWORD)(pObj->m_Format.dwBytesPerSampleContainer * 
        pObj->m_Format.fFramesPerSecond * pObj->m_Format.dwSamplesPerFrame);
    pObj->m_Wfx.Format.nBlockAlign = (WORD)(pObj->m_Format.dwBytesPerSampleContainer * pObj->m_Format.dwSamplesPerFrame);
    pObj->m_Wfx.Format.wBitsPerSample = (WORD)pObj->m_Format.dwValidBitsPerSample;
    pObj->m_Wfx.Format.cbSize = 0;

    hResult = pObj->QueryInterface( 
        __uuidof(IAudioMediaType), 
        (LPVOID *) ppIAudioMediaType );

    return( hResult );
} // Create

//-----------------------------------------------------------------------------
// Description:
//
//  Used to addref an interface
//
// Results:
//
//     New reference count
//
ULONG STDMETHODCALLTYPE CTestAudioMediaType::AddRef()
{
    return (ULONG) InterlockedIncrement( (LONG *) &m_cRef );
} // AddRef

//-----------------------------------------------------------------------------
// Description:
//
//  Used to release an interface
//
// Results:
//
//     New reference count
//
ULONG STDMETHODCALLTYPE CTestAudioMediaType::Release()
{
    ULONG ulNewRefCount = (ULONG) InterlockedDecrement( (LONG *) &m_cRef );

    //
    //  If our new refcount is zero, then we need to delete ourselves
    //
    if( 0 == ulNewRefCount )
    {
        delete this;
    }
    
    return( ulNewRefCount );
} // Release

//-----------------------------------------------------------------------------
// Description:
//
//  Used to query an interface
//
// Parameters:
//
//     riid - [in] id to look for
//     ppvObject - [out] pointer to object
//
// Return Values:
//
//    S_OK             On success.
//    E_POINTER        Invalid destination pointer
//    E_NOINTERFACE    No interface of that type
//    error            Other
//
STDMETHODIMP CTestAudioMediaType::QueryInterface( 
    REFIID riid, LPVOID *ppvObject )
{
    if (!ppvObject)
    {
        return E_POINTER;
    }

    HRESULT     hResult;

    hResult = S_OK;
    *ppvObject = NULL;

    if( ( riid == __uuidof(IAudioMediaType) ) ||
        ( riid == __uuidof(IUnknown) ) )
    {
        *ppvObject = ( IAudioMediaType *)this;
    }
    else
    {
        hResult = E_NOINTERFACE;
    }

    if ( SUCCEEDED( hResult ) )
    {
        ((LPUNKNOWN) *ppvObject)->AddRef( );
    }

    return( hResult );
} // QueryInterface
      
//-----------------------------------------------------------------------------
// Description:
//
//  Not implemented
//
// Parameters:
//
//     pAudioMediaType - [in] media type
//     pdwFlags - [out] flags
//
// Return Values:
//
//    E_NOTIMPL       Not implemented.
//
HRESULT STDMETHODCALLTYPE CTestAudioMediaType::IsEqual( 
    IAudioMediaType * /* pAudioMediaType */,
    DWORD * /* pdwFlags */
)
{
    return E_NOTIMPL;
} // IsEqual

//-----------------------------------------------------------------------------
// Description:
//
//  Returns pointer to internal member variable m_Wfx.
//
// Results:
//
//     Returns pointer to WaveFormatEx representing the format.
//
const WAVEFORMATEX *STDMETHODCALLTYPE CTestAudioMediaType::GetAudioFormat( void)
{
    return reinterpret_cast<WAVEFORMATEX*>(&m_Wfx);
} // GetAudioFormat
 
//-----------------------------------------------------------------------------
// Description:
//
//  Copies the uncompressed data to the parameter.
//
// Parameters:
//
//     pUncompressedAudioFormat - [in] format pointer
//
// Return Values:
//
//    S_OK         On success
//    E_POINTER    Invalid pointer
//
HRESULT STDMETHODCALLTYPE CTestAudioMediaType::GetUncompressedAudioFormat( 
    UNCOMPRESSEDAUDIOFORMAT *pUncompressedAudioFormat 
)
{
    RETURN_HR_IF(E_FAIL, (KSDATAFORMAT_SUBTYPE_PCM != m_Format.guidFormatType) && 
        (KSDATAFORMAT_SUBTYPE_IEEE_FLOAT != m_Format.guidFormatType));

    if (!pUncompressedAudioFormat)
    {
        return E_POINTER;
    }

    CopyMemory(pUncompressedAudioFormat, &m_Format, sizeof(UNCOMPRESSEDAUDIOFORMAT));

    return S_OK;
} // GetUncompressedAudioFormat
