//
// Copyright (C) Microsoft Corporation 2005
// IHV UI Extension sample
//

#include "precomp.h"

HRESULT
Wstr2Bstr
(
    _In_        LPCWSTR     pszSrc,
    _Outptr_ BSTR*       pbstrDest
)
{
    HRESULT hr  =   S_OK;

    if ( !pbstrDest )
    {
        hr = E_INVALIDARG;
        BAIL_ON_FAILURE( hr );
    }

    (*pbstrDest) = NULL;
    if ( !pszSrc )
    {
        BAIL( );
    }

    (*pbstrDest) = SysAllocString( pszSrc );
    if ( !(*pbstrDest) )
    {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE( hr );
    }

error:
    return hr;
}




HRESULT
Wstr2Wstr
(
   _In_        LPCWSTR     pszSrc,
   _Outptr_ LPWSTR*     ppszDest
)
{
    HRESULT hr  =   S_OK;

    if ( !ppszDest )
    {
        hr = E_INVALIDARG;
        BAIL_ON_FAILURE( hr );
    }

    (*ppszDest) = NULL;
    if ( !pszSrc )
    {
        BAIL( );
    }

    *ppszDest = _wcsdup( pszSrc );

    if ( !(*ppszDest) )
    {
        hr = E_OUTOFMEMORY;
    }
    
error:
    return hr;
}




HRESULT
Wstr2Dword
(
    IN  LPCWSTR     pszSrc,
    OUT DWORD*      pdwDest
)
{
    HRESULT hr  =   S_OK;

    if ( (!pdwDest) || (!pszSrc) )
    {
        hr = E_INVALIDARG;
        BAIL_ON_FAILURE( hr );
    }

    (*pdwDest) = (DWORD) _wtol( pszSrc );

error:
    return hr;
}

/*
Note:
2^32 = 2^(4*8) = 16^8 < 100^8 = 10^16
This implies that 20 decimal digits
are more than enough for a DWORD.
*/

HRESULT
Dword2Bstr
(
    IN  DWORD       dwSrc,
    OUT BSTR*       pbstrDest
)
{
    HRESULT     hr              =   S_OK;
    WCHAR       szBuffer[25]    =   {0};

    hr =
    StringCchPrintf
    (
        szBuffer,
        sizeof(szBuffer)/sizeof(szBuffer[0]),
        L"%u",
        dwSrc
    );
    BAIL_ON_FAILURE( hr );

    hr =
    Wstr2Bstr
    (
        szBuffer,
        pbstrDest
    );
    BAIL_ON_FAILURE( hr );


error:
    return hr;
}


HRESULT
Wstr2Bool
(
    IN  LPCWSTR     pszSrc,
    OUT BOOL*       pbDest
)
{
    HRESULT hr  =   S_OK;

    if ( (!pbDest) || (!pszSrc) )
    {
        hr = E_INVALIDARG;
        BAIL_ON_FAILURE( hr );
    }

    if ( 0 == wcscmp( L"TRUE", pszSrc ) )
    {
        (*pbDest) = TRUE;
    }
    else if ( 0 == wcscmp( L"FALSE", pszSrc ) )
    {
        (*pbDest) = FALSE;
    }
    else
    {
        hr = E_INVALIDARG;
        BAIL_ON_FAILURE( hr );
    }

error:
    return hr;
}


HRESULT
Bool2Bstr
(
    IN  BOOL        bSrc,
    OUT BSTR*       pbstrDest
)
{
    HRESULT hr  =   S_OK;

    if ( !pbstrDest )
    {
        hr = E_INVALIDARG;
        BAIL_ON_FAILURE( hr );
    }

    if ( bSrc )
    {
        (*pbstrDest) = SysAllocString( L"TRUE" );
    }
    else
    {
        (*pbstrDest) = SysAllocString( L"FALSE" );
    }
    if ( !(*pbstrDest) )
    {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE( hr );
    }

error:
    return hr;
}




HRESULT
Wstr2AuthType
(
    IN  LPCWSTR             pszSrc,
    OUT PIHV_AUTH_TYPE      pAuthType
)
{
    HRESULT     hr      =   S_OK;
    DWORD       dwIndex =   0;

    if ( (!pAuthType) || (!pszSrc) )
    {
        hr = E_INVALIDARG;
        BAIL_ON_FAILURE( hr );
    }

    for ( dwIndex = 0; dwIndex < MAX_AUTH_TYPES; dwIndex++ )
    {
        if ( 0 == wcscmp( gppszIhvAuthTypes[dwIndex], pszSrc ) )
        {
            (*pAuthType) = (IHV_AUTH_TYPE) dwIndex;
            BAIL( );
        }
    }

    // String not found.
    hr = E_INVALIDARG;
    BAIL_ON_FAILURE( hr );


error:
    return hr;
}

HRESULT
AuthType2Bstr
(
    IN  IHV_AUTH_TYPE       AuthType,
    OUT BSTR*               pbstrDest
)
{
    HRESULT hr  =   S_OK;

    if ( !pbstrDest )
    {
        hr = E_INVALIDARG;
        BAIL_ON_FAILURE( hr );
    }

    if ( AuthType < 0 || AuthType >= IHVAuthInvalid )
    {
        hr = E_UNEXPECTED;
        BAIL_ON_FAILURE( hr );
    }

    (*pbstrDest) = SysAllocString( gppszIhvAuthTypes[(DWORD) AuthType] );
    if ( !(*pbstrDest) )
    {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE( hr );
    }

error:
    return hr;
}


HRESULT
Wstr2SecurityType
(
    IN  LPCWSTR             pszSrc,
    OUT PIHV_SECURITY_TYPE  pSecurityType
)
{
    HRESULT     hr      =   S_OK;
    DWORD       dwIndex =   0;

    if ( (!pSecurityType) || (!pszSrc) )
    {
        hr = E_INVALIDARG;
        BAIL_ON_FAILURE( hr );
    }

    for ( dwIndex = 0; dwIndex < MAX_AUTH_TYPES; dwIndex++ )
    {
        if ( 0 == wcscmp( gppszIhvSecurityTypes[dwIndex], pszSrc ) )
        {
            (*pSecurityType) = (IHV_SECURITY_TYPE) dwIndex;
            BAIL( );
        }
    }

    // String not found.
    hr = E_INVALIDARG;
    BAIL_ON_FAILURE( hr );


error:
    return hr;
}

HRESULT
SecurityType2Bstr
(
    IN  IHV_SECURITY_TYPE   SecurityType,
    OUT BSTR*               pbstrDest
)
{
    HRESULT hr  =   S_OK;

    if ( !pbstrDest )
    {
        hr = E_INVALIDARG;
        BAIL_ON_FAILURE( hr );
    }

    if ( SecurityType < 0 || SecurityType >= IHVSecurityInvalid )
    {
        hr = E_UNEXPECTED;
        BAIL_ON_FAILURE( hr );
    }

    (*pbstrDest) = SysAllocString( gppszIhvSecurityTypes[(DWORD) SecurityType] );
    if ( !(*pbstrDest) )
    {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE( hr );
    }

error:
    return hr;
}

HRESULT
Wstr2CipherType
(
    IN  LPCWSTR             pszSrc,
    OUT PIHV_CIPHER_TYPE    pCipherType
)
{
    HRESULT     hr      =   S_OK;
    DWORD       dwIndex =   0;

    if ( (!pCipherType) || (!pszSrc) )
    {
        hr = E_INVALIDARG;
        BAIL_ON_FAILURE( hr );
    }

    for ( dwIndex = 0; dwIndex < MAX_CIPHER_TYPES; dwIndex++ )
    {
        if ( 0 == wcscmp( gppszIhvCipherTypes[dwIndex], pszSrc ) )
        {
            (*pCipherType) = (IHV_CIPHER_TYPE) dwIndex;
            BAIL( );
        }
    }

    // String not found.
    hr = E_INVALIDARG;
    BAIL_ON_FAILURE( hr );


error:
    return hr;
}


HRESULT
CipherType2Bstr
(
    IN  IHV_CIPHER_TYPE     CipherType,
    OUT BSTR*               pbstrDest
)
{
    HRESULT hr  =   S_OK;

    if ( !pbstrDest )
    {
        hr = E_INVALIDARG;
        BAIL_ON_FAILURE( hr );
    }

    if ( CipherType < None || CipherType >= IHVCipherInvalid )
    {
        hr = E_UNEXPECTED;
        BAIL_ON_FAILURE( hr );
    }

    (*pbstrDest) = SysAllocString( gppszIhvCipherTypes[(DWORD) CipherType] );
    if ( !(*pbstrDest) )
    {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE( hr );
    }

error:
    return hr;
}
