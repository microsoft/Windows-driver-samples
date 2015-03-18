//
// Copyright (C) Microsoft Corporation 2005
// IHV UI Extension sample
//

#include "precomp.h"




//
// Free BSTR
//
#define SYS_FREE_STRING( _s )       \
    if ( _s )                       \
    {                               \
        SysFreeString( _s );        \
        (_s) = NULL;                \
    }                               \


//
// Release interface.
//
#define RELEASE_INTERFACE( _p )     \
    if ( _p )                       \
    {                               \
        (_p)->Release( );           \
        (_p) = NULL;                \
    }                               \


// XPath strings for parsing xml blobs,

#define     CON_PARAM1_XPATH    L"/IhvConnectivity/IHVConnectivityParam1"
#define     CON_PARAM2_XPATH    L"/IhvConnectivity/IHVConnectivityParam2"


#define     SEC_FSFLAG_XPATH    L"/IhvSecurity/IHVUsesFullSecurity"
#define     SEC_ATYPE_XPATH     L"/IhvSecurity/IHVAuthentication"
#define     SEC_ETYPE_XPATH     L"/IhvSecurity/IHVEncryption"
#define     SEC_PARAM1_XPATH    L"/IhvSecurity/IHVSecurityParam1"
#define     SEC_PARAM2_XPATH    L"/IhvSecurity/IHVSecurityParam2"


// Strings to match the profile with internal data types.
LPCWSTR
gppszIhvAuthTypes[] =
{
    L"IHVAuthV1",
    L"IHVAuthV2",
    L"IHVAuthV3"
};


LPCWSTR
gppszIhvCipherTypes[] =
{
    L"None",
    L"IHVCipher1",
    L"IHVCipher2",
    L"IHVCipher3"
};


//
// Base class for profile APIs.
//
class CIhvProfileBase
{
public:

    // Constructor
    CIhvProfileBase( )
    {
        m_pRootNode = NULL;
    }

    // Destructor
    ~CIhvProfileBase( )
    {
        RELEASE_INTERFACE( m_pRootNode );
    }

    HRESULT
    LoadXml
    (
        IN  BSTR    bstrIhvProfile
    );


    // Caller needs to know what type to
    // cast the pointer to depending upon
    // the type of the derived class.
    // Caller needs to free memory recursively
    // by using the free( ) function.
    virtual
    HRESULT
    GetNativeData
    (
        LPVOID* ppvData
    )
    = 0;

protected:

    HRESULT
    GetTextFromNode
    (
        IN  LPCWSTR         pszQuery,
        OUT BSTR*           pbstrText
    );


    IXMLDOMElement*     m_pRootNode;

};





//
// Derived class for connectivity profiles.
//
class CIhvConnectivityProfile : public CIhvProfileBase
{

public:

    // Constructor Destructor
    CIhvConnectivityProfile( ) { }
    ~CIhvConnectivityProfile( ) { }

    // Caller needs to know what type to
    // cast the pointer to depending upon
    // the type of the derived class.
    // Caller needs to free memory recursively
    // by using the PrivateMemoryFree( ) function.
    HRESULT
    GetNativeData
    (
        LPVOID* ppvData
    );


    // Accessor dwParam1
    HRESULT
    GetParam1
    (
        DWORD*  pdwParam1
    );


    // Accessor for pszParam2
    HRESULT
    GetParam2
    (
        BSTR*   pbstrValue
    );


};






//
// Derived class for security profiles.
//

class CIhvSecurityProfile
    : public CIhvProfileBase
{

public:

    // Constructor Destructor
    CIhvSecurityProfile( ) { }
    ~CIhvSecurityProfile( ) { }

    // Caller needs to know what type to
    // cast the pointer to depending upon
    // the type of the derived class.
    // Caller needs to free memory recursively
    // by using the PrivateMemoryFree( ) function.
    HRESULT
    GetNativeData
    (
        LPVOID* ppvData
    );


    // Accessor and Modifier for bUseFullSecurity
    HRESULT
    GetFullSecurityFlag
    (
        BOOL*  pbUseFullSecurity
    );


    // Accessor for AuthType
    HRESULT
    GetAuthType
    (
        PIHV_AUTH_TYPE  pAuthType
    );

    // Accessor for CipherType
    HRESULT
    GetCipherType
    (
        PIHV_CIPHER_TYPE  pCipherType
    );

    // Accessor for dwParam1
    HRESULT
    GetParam1
    (
        DWORD*  pdwParam1
    );

    // Accessor for pszParam2
    HRESULT
    GetParam2
    (
        BSTR*   pbstrValue
    );

};


// Convert unicode string to BSTR. NULL safe.
HRESULT
Wstr2Bstr
(
    _In_     LPCWSTR     pszSrc,
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




// Convert unicode string to unicode string. NULL safe.
// string allocated by malloc and freed by free.
HRESULT
Wstr2Wstr
(
    _In_     LPCWSTR     pszSrc,
    _Outptr_ LPWSTR*     ppszDest
)
{
    HRESULT hr  =   S_OK;
    size_t  len =   0;

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

    len =   1 + wcslen( pszSrc );
    len *=  sizeof( WCHAR );

    (*ppszDest) = (LPWSTR) PrivateMemoryAlloc( len );
    if ( !(*ppszDest) )
    {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE( hr );
    }

    CopyMemory( (*ppszDest), pszSrc, len );

error:
    return hr;
}



//
// convert unicode string to DWORD
//
HRESULT
Wstr2Dword
(
    _In_  LPCWSTR     pszSrc,
    _Out_ DWORD*      pdwDest
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




//
// convert unicode string to BOOL
//
HRESULT
Wstr2Bool
(
    _In_  LPCWSTR     pszSrc,
    _Out_ BOOL*       pbDest
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







//
// convert unicode string to auth type
//
HRESULT
Wstr2AuthType
(
    _In_  LPCWSTR             pszSrc,
    _Out_ PIHV_AUTH_TYPE      pAuthType
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







//
// convert unicode string to cipher type
//
HRESULT
Wstr2CipherType
(
    _In_  LPCWSTR             pszSrc,
    _Out_ PIHV_CIPHER_TYPE    pCipherType
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




// base function to obtain text from
// node described by XPATH.
HRESULT
CIhvProfileBase::GetTextFromNode
(
    IN  LPCWSTR         pszQuery,
    OUT BSTR*           pbstrText
)
{
    HRESULT         hr          =   S_OK;
    BSTR            bstrQuery   =   NULL;
    IXMLDOMNode*    pQueryNode  =   NULL;

    ASSERT( pszQuery );
    ASSERT( pbstrText );

    // if node is NULL, return empty string.
    if ( !m_pRootNode )
    {
        hr =
        Wstr2Bstr
        (
            L"",
            pbstrText
        );
        BAIL( );
    }

    hr =
    Wstr2Bstr
    (
        pszQuery,
        &bstrQuery
    );
    BAIL_ON_FAILURE( hr );

    hr = m_pRootNode->selectSingleNode( bstrQuery, &pQueryNode );
    BAIL_ON_FAILURE( hr );

    if (!pQueryNode)
    {
        hr = E_UNEXPECTED;
        BAIL_ON_FAILURE( hr );
    }

    hr = pQueryNode->get_text( pbstrText );
    BAIL_ON_FAILURE( hr );

    if ( !(*pbstrText) )
    {
        hr = E_UNEXPECTED;
        BAIL_ON_FAILURE( hr );
    }

error:
    RELEASE_INTERFACE( pQueryNode );
    SYS_FREE_STRING( bstrQuery );
    return hr;
}



// Load node from xml string. If xml string is null this
// function is a NO_OP.
HRESULT
CIhvProfileBase::LoadXml
(
    IN  BSTR    bstrIhvProfile
)
{
    HRESULT             hr              =   S_OK;
    IXMLDOMDocument*    pDOMDoc         =   NULL;
    IXMLDOMElement*     pDocElem        =   NULL;
    VARIANT_BOOL        vfSuccess;

    if ( m_pRootNode )
    {
        hr = E_UNEXPECTED;
        BAIL_ON_FAILURE( hr );
    }

    if ( !bstrIhvProfile )
    {
        BAIL( );
    }

    hr =
    CoCreateInstance
    (
        CLSID_DOMDocument,
        NULL,
        CLSCTX_ALL,
        IID_IXMLDOMDocument,
        (LPVOID *) &pDOMDoc
    );
    BAIL_ON_FAILURE( hr );

    hr =
    pDOMDoc->loadXML
    (
        bstrIhvProfile,
        &vfSuccess
    );
    BAIL_ON_FAILURE( hr );

    if ( VARIANT_TRUE != vfSuccess )
    {
        hr = E_UNEXPECTED;
        BAIL_ON_FAILURE( hr );
    }

    hr =
    pDOMDoc->get_documentElement
    (
        &pDocElem
    );
    BAIL_ON_FAILURE( hr );

    // Caching the pointer to the document element
    // in a member variable.
    m_pRootNode =   pDocElem;
    pDocElem    =   NULL;


error:
    RELEASE_INTERFACE( pDOMDoc  );
    RELEASE_INTERFACE( pDocElem );
    return hr;
}





// Accessor.
HRESULT
CIhvConnectivityProfile::GetParam1
(
    DWORD*  pdwParam1
)
{
    HRESULT     hr          =   S_OK;
    BSTR        bstrData    =   NULL;

    hr =
    GetTextFromNode
    (
        CON_PARAM1_XPATH,
        &bstrData
    );
    BAIL_ON_FAILURE( hr );

    if ( NULL == bstrData )
    {
        hr = E_POINTER;
        BAIL_ON_FAILURE( hr );
    }

    hr =
    Wstr2Dword
    (
        bstrData,
        pdwParam1
    );
    BAIL_ON_FAILURE( hr );

error:
    SYS_FREE_STRING( bstrData );
    return hr;
}



// Accessor.
HRESULT
CIhvConnectivityProfile::GetParam2
(
    BSTR*   pbstrValue
)
{
    HRESULT hr  =   S_OK;

    hr =
    GetTextFromNode
    (
        CON_PARAM2_XPATH,
        pbstrValue
    );
    BAIL_ON_FAILURE( hr );


error:
    return hr;
}




//
// Calls the accessors to build native data.
//
HRESULT
CIhvConnectivityProfile::GetNativeData
(
    LPVOID* ppvData
)
{
    HRESULT                     hr          =   S_OK;
    PIHV_CONNECTIVITY_PROFILE   pIhvProfile =   NULL;
    BSTR                        bstrParam2  =   NULL;

    if ( !ppvData )
    {
        hr = E_INVALIDARG;
        BAIL_ON_FAILURE( hr );
    }

    pIhvProfile = (PIHV_CONNECTIVITY_PROFILE) PrivateMemoryAlloc( sizeof( IHV_CONNECTIVITY_PROFILE ) );
    if ( !pIhvProfile )
    {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE( hr );
    }

    // Ignoring errors since structure is already
    // populated with defaults.

    hr =
    GetParam2
    (
        &bstrParam2
    );
    BAIL_ON_FAILURE( hr );

    if ( NULL == bstrParam2 )
    {
        hr = E_POINTER;
        BAIL_ON_FAILURE( hr );
    }

    hr =
    Wstr2Wstr
    (
        bstrParam2,
        &(pIhvProfile->pszParam2)
    );
    BAIL_ON_FAILURE( hr );

    hr =
    GetParam1
    (
        &(pIhvProfile->dwParam1)
    );

    // Consuming earlier failures.
    hr = S_OK;

    // Transfering local cache to OUT parameter.
    (*ppvData) = pIhvProfile;
    pIhvProfile = NULL;

error:
    if ( pIhvProfile )
    {
        PrivateMemoryFree( pIhvProfile->pszParam2 ); // NULL Safe.
        PrivateMemoryFree( pIhvProfile );
    }
    SYS_FREE_STRING( bstrParam2 );
    return hr;
}





// Accessor.
HRESULT
CIhvSecurityProfile::GetFullSecurityFlag
(
    BOOL*  pbUseFullSecurity
)
{
    HRESULT     hr          =   S_OK;
    BSTR        bstrData    =   NULL;

    hr =
    GetTextFromNode
    (
        SEC_FSFLAG_XPATH,
        &bstrData
    );
    BAIL_ON_FAILURE( hr );

    if ( NULL == bstrData )
    {
        hr = E_POINTER;
        BAIL_ON_FAILURE( hr );
    }

    hr =
    Wstr2Bool
    (
        bstrData,
        pbUseFullSecurity
    );
    BAIL_ON_FAILURE( hr );

error:
    SYS_FREE_STRING( bstrData );
    return hr;
}





// Accessor.
HRESULT
CIhvSecurityProfile::GetAuthType
(
    PIHV_AUTH_TYPE  pAuthType
)
{
    HRESULT     hr          =   S_OK;
    BSTR        bstrData    =   NULL;

    hr =
    GetTextFromNode
    (
        SEC_ATYPE_XPATH,
        &bstrData
    );
    BAIL_ON_FAILURE( hr );

    if ( NULL == bstrData )
    {
        hr = E_POINTER;
        BAIL_ON_FAILURE( hr );
    }

    hr =
    Wstr2AuthType
    (
        bstrData,
        pAuthType
    );
    BAIL_ON_FAILURE( hr );

error:
    SYS_FREE_STRING( bstrData );
    return hr;
}




// Accessor.
HRESULT
CIhvSecurityProfile::GetCipherType
(
    PIHV_CIPHER_TYPE  pCipherType
)
{
    HRESULT     hr          =   S_OK;
    BSTR        bstrData    =   NULL;

    hr =
    GetTextFromNode
    (
        SEC_ETYPE_XPATH,
        &bstrData
    );
    BAIL_ON_FAILURE( hr );

    if ( NULL == bstrData )
    {
        hr = E_POINTER;
        BAIL_ON_FAILURE( hr );
    }

    hr =
    Wstr2CipherType
    (
        bstrData,
        pCipherType
    );
    BAIL_ON_FAILURE( hr );

error:
    SYS_FREE_STRING( bstrData );
    return hr;
}





// Accessor.
HRESULT
CIhvSecurityProfile::GetParam1
(
    DWORD*  pdwParam1
)
{
    HRESULT     hr          =   S_OK;
    BSTR        bstrData    =   NULL;

    hr =
    GetTextFromNode
    (
        SEC_PARAM1_XPATH,
        &bstrData
    );
    BAIL_ON_FAILURE( hr );

    if ( NULL == bstrData )
    {
        hr = E_POINTER;
        BAIL_ON_FAILURE( hr );
    }

    hr =
    Wstr2Dword
    (
        bstrData,
        pdwParam1
    );
    BAIL_ON_FAILURE( hr );

error:
    SYS_FREE_STRING( bstrData );
    return hr;
}






// Accessor.
HRESULT
CIhvSecurityProfile::GetParam2
(
    BSTR*   pbstrValue
)
{
    HRESULT hr  =   S_OK;

    hr =
    GetTextFromNode
    (
        SEC_PARAM2_XPATH,
        pbstrValue
    );
    BAIL_ON_FAILURE( hr );


error:
    return hr;
}


//
// Calls the accessors to build native data.
//
HRESULT
CIhvSecurityProfile::GetNativeData
(
    LPVOID* ppvData
)
{
    HRESULT                 hr          =   S_OK;
    PIHV_SECURITY_PROFILE   pIhvProfile =   NULL;
    BSTR                    bstrParam2  =   NULL;

    if ( !ppvData )
    {
        hr = E_INVALIDARG;
        BAIL_ON_FAILURE( hr );
    }

    pIhvProfile = (PIHV_SECURITY_PROFILE) PrivateMemoryAlloc( sizeof( IHV_SECURITY_PROFILE ) );
    if ( !pIhvProfile )
    {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE( hr );
    }

    pIhvProfile->bUseIhvConnectivityOnly = ( m_pRootNode == NULL );

    // Ignoring errors since structure is already
    // populated with defaults.
    hr =
    GetFullSecurityFlag
    (
        &(pIhvProfile->bUseFullSecurity)
    );

    hr =
    GetAuthType
    (
        &(pIhvProfile->AuthType)
    );

    hr =
    GetCipherType
    (
        &(pIhvProfile->CipherType)
    );

    hr =
    GetParam1
    (
        &(pIhvProfile->dwParam1)
    );

    hr =
    GetParam2
    (
        &bstrParam2
    );
    BAIL_ON_FAILURE( hr );

    if ( NULL == bstrParam2 )
    {
        hr = E_POINTER;
        BAIL_ON_FAILURE( hr );
    }

    hr =
    Wstr2Wstr
    (
        bstrParam2,
        &(pIhvProfile->pszParam2)
    );

    // Consuming earlier failures.
    hr = S_OK;

    // Transfering local cache to OUT parameter.
    (*ppvData) = pIhvProfile;
    pIhvProfile = NULL;

error:
    if ( pIhvProfile )
    {
        PrivateMemoryFree( pIhvProfile->pszParam2 ); // NULL Safe.
        PrivateMemoryFree( pIhvProfile );
    }
    SYS_FREE_STRING( bstrParam2 );
    return hr;
}


// Converts string to connectivity profile.
DWORD
GetIhvConnectivityProfile
(
    PDOT11EXT_IHV_CONNECTIVITY_PROFILE  pDot11ExtIhvConnProfile,
    PIHV_CONNECTIVITY_PROFILE*          ppConnectivityProfile
)
{
    HRESULT                     hr                      =   S_OK;
    BOOL                        bComInitialized         =   FALSE;
    BSTR                        bstrIhvProfile          =   NULL;
    PIHV_CONNECTIVITY_PROFILE   pConnectivityProfile    =   NULL;

    CIhvConnectivityProfile*    pIhvProfile             =   NULL;

    ASSERT( pDot11ExtIhvConnProfile );
    ASSERT( ppConnectivityProfile );


    // Data structure is multi-thread safe because
    // it is both allocated and freed by current
    // function.
    hr =
    CoInitializeEx
    (
        NULL,
        COINIT_MULTITHREADED
    );
    BAIL_ON_FAILURE( hr );
    bComInitialized = TRUE;


    pIhvProfile = new(std::nothrow) CIhvConnectivityProfile;
    if (!pIhvProfile)
    {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE( hr );
    }

    hr =
    Wstr2Bstr
    (
        pDot11ExtIhvConnProfile->pszXmlFragmentIhvConnectivity,
        &bstrIhvProfile
    );
    BAIL_ON_FAILURE( hr );

    hr =
    pIhvProfile->LoadXml
    (
        bstrIhvProfile
    );
    BAIL_ON_FAILURE( hr );

    hr =
    pIhvProfile->GetNativeData
    (
        (LPVOID*) &pConnectivityProfile
    );
    BAIL_ON_FAILURE( hr );


    (*ppConnectivityProfile)    =   pConnectivityProfile;
    pConnectivityProfile        =   NULL;


error:
    SYS_FREE_STRING( bstrIhvProfile );

    FreeIhvConnectivityProfile ( &pConnectivityProfile );

    delete pIhvProfile;

    if ( bComInitialized )
    {
        CoUninitialize( );
    }
    return WIN32_FROM_HRESULT( hr );
}




// free connectivity profile.

VOID
FreeIhvConnectivityProfile
(
    PIHV_CONNECTIVITY_PROFILE*  ppConnectivityProfile
)
{
    if ( ppConnectivityProfile && (*ppConnectivityProfile) )
    {
        PrivateMemoryFree( (*ppConnectivityProfile)->pszParam2 );
        PrivateMemoryFree( (*ppConnectivityProfile) );
        (*ppConnectivityProfile) = NULL;
    }
}


// Converts string to security profile.
DWORD
GetIhvSecurityProfile
(
    PDOT11EXT_IHV_SECURITY_PROFILE      pDot11ExtIhvSecProfile,
    PIHV_SECURITY_PROFILE*              ppSecurityProfile
)
{
    HRESULT                     hr                      =   S_OK;
    BOOL                        bComInitialized         =   FALSE;
    BSTR                        bstrIhvProfile          =   NULL;
    PIHV_SECURITY_PROFILE       pSecurityProfile        =   NULL;

    CIhvSecurityProfile*        pIhvProfile             =   NULL;

    ASSERT( pDot11ExtIhvSecProfile );
    ASSERT( ppSecurityProfile );


    // Data structure is multi-thread safe because
    // it is both allocated and freed by current
    // function.
    hr =
    CoInitializeEx
    (
        NULL,
        COINIT_MULTITHREADED
    );
    BAIL_ON_FAILURE( hr );
    bComInitialized = TRUE;



    pIhvProfile = new(std::nothrow) CIhvSecurityProfile;
    if (!pIhvProfile)
    {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE( hr );
    }

    hr =
    Wstr2Bstr
    (
        pDot11ExtIhvSecProfile->pszXmlFragmentIhvSecurity,
        &bstrIhvProfile
    );
    BAIL_ON_FAILURE( hr );

    hr =
    pIhvProfile->LoadXml
    (
        bstrIhvProfile
    );
    BAIL_ON_FAILURE( hr );

    hr =
    pIhvProfile->GetNativeData
    (
        (LPVOID*) &pSecurityProfile
    );
    BAIL_ON_FAILURE( hr );

    if ( pDot11ExtIhvSecProfile->bUseMSOnex && pSecurityProfile->bUseFullSecurity )
    {
        hr = E_UNEXPECTED;
        BAIL_ON_FAILURE( hr );
    }

    (*ppSecurityProfile)    =   pSecurityProfile;
    pSecurityProfile        =   NULL;


error:
    SYS_FREE_STRING( bstrIhvProfile );

    FreeIhvSecurityProfile ( &pSecurityProfile );

    delete pIhvProfile;

    if ( bComInitialized )
    {
        CoUninitialize( );
    }
    return WIN32_FROM_HRESULT( hr );
}





// free security profile.
VOID
FreeIhvSecurityProfile
(
    PIHV_SECURITY_PROFILE*  ppSecurityProfile
)
{
    if ( ppSecurityProfile && (*ppSecurityProfile) )
    {
        PrivateMemoryFree( (*ppSecurityProfile)->pszParam2 );
        PrivateMemoryFree( (*ppSecurityProfile) );
        (*ppSecurityProfile) = NULL;
    }
}
