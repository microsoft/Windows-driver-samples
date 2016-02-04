//
// Copyright (C) Microsoft Corporation 2005
// IHV UI Extension sample
//

#include "precomp.h"

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

    if ( !m_pRootNode )
    {
        hr = E_UNEXPECTED;
        BAIL_ON_FAILURE( hr );
    }

    if ( (!pszQuery) || (!pbstrText) )
    {
        hr = E_INVALIDARG;
        BAIL_ON_FAILURE( hr );
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


HRESULT
CIhvProfileBase::PutTextInNode
(
    IN  LPCWSTR         pszQuery,
    IN  BSTR            bstrText
)
{
    HRESULT         hr          =   S_OK;
    BSTR            bstrQuery   =   NULL;
    BSTR            bstrOrig    =   NULL;
    BOOL            bPut        =   TRUE;
    IXMLDOMNode*    pQueryNode  =   NULL;

    if ( !m_pRootNode )
    {
        hr = E_UNEXPECTED;
        BAIL_ON_FAILURE( hr );
    }

    if ( (!pszQuery) || (!bstrText) )
    {
        hr = E_INVALIDARG;
        BAIL_ON_FAILURE( hr );
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

    hr = pQueryNode->get_text( &bstrOrig );
    BAIL_ON_FAILURE( hr );

    if ( bstrOrig && ( 0 == wcscmp( bstrOrig, bstrText ) ) )
    {
        bPut = FALSE;
    }

    if ( bPut )
    {
        hr = pQueryNode->put_text( bstrText );
        BAIL_ON_FAILURE( hr );

        SetModified( );
    }

error:
    RELEASE_INTERFACE( pQueryNode );
    SYS_FREE_STRING( bstrQuery );
    SYS_FREE_STRING( bstrOrig  );
    return hr;
}


HRESULT
CIhvProfileBase::LoadXml
(
    IN  BSTR    bstrProfileData
)
{
    HRESULT             hr              =   S_OK;
    IXMLDOMDocument*    pDOMDoc         =   NULL;
    IXMLDOMElement*     pDocElem        =   NULL;
    BSTR                bstrIhvProfile  =   NULL;
    VARIANT_BOOL        vfSuccess;

    if ( !bstrProfileData )
    {
        hr = GetDefaultXml( &bstrIhvProfile );
        BAIL_ON_FAILURE( hr );
    }
    else
    {
		bstrIhvProfile = bstrProfileData;
    }

    if ( m_pRootNode )
    {
        hr = E_UNEXPECTED;
        BAIL_ON_FAILURE( hr );
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
    if ( !bstrProfileData )
    {
	    SYS_FREE_STRING( bstrIhvProfile );
    }
    RELEASE_INTERFACE( pDOMDoc  );
    RELEASE_INTERFACE( pDocElem );
    return hr;
}


HRESULT
CIhvProfileBase::EmitXml
(
    OUT BSTR*   pbstrIhvProfile
)
{
    HRESULT hr  =   S_OK;

    if ( !pbstrIhvProfile )
    {
        hr = E_INVALIDARG;
        BAIL_ON_FAILURE( hr );
    }

    if ( !m_pRootNode )
    {
        hr = E_UNEXPECTED;
        BAIL_ON_FAILURE( hr );
    }

    hr = m_pRootNode->get_xml( pbstrIhvProfile );
    BAIL_ON_FAILURE( hr );

error:
    return hr;
}


//////////////////////////////////////////////////////////

WCHAR	g_szDefaultConnectivityProfile[] = 
L"<IhvConnectivity xmlns=\"http://www.sampleihv.com/nwifi/profile\">"
L"            <IHVConnectivityParam1>0</IHVConnectivityParam1>"
L"            <IHVConnectivityParam2>parameter value</IHVConnectivityParam2>"
L"</IhvConnectivity>"
;



HRESULT
CIhvConnectivityProfile::GetDefaultXml
(
    BSTR* pbstrDefault
)
{
    SetModified();
	return Wstr2Bstr( g_szDefaultConnectivityProfile, pbstrDefault );
}



HRESULT
CIhvConnectivityProfile::GetParamDWORD
(
    DWORD*  pdwParam1
)
{
    HRESULT     hr          =   S_OK;
    BSTR        bstrData    =   NULL;

    hr =
    GetTextFromNode
    (
        CON_PARAM2_XPATH,
        &bstrData
    );
    BAIL_ON_FAILURE( hr );

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


HRESULT
CIhvConnectivityProfile::SetParamDWORD
(
    DWORD   dwNewValue
)
{
    HRESULT     hr              =   S_OK;
    BSTR        bstrText        =   NULL;

    hr =
    Dword2Bstr
    (
        dwNewValue,
        &bstrText
    );
    BAIL_ON_FAILURE( hr );

    hr =
    PutTextInNode
    (
        CON_PARAM2_XPATH,
        bstrText
    );
    BAIL_ON_FAILURE( hr );

error:
    SYS_FREE_STRING( bstrText );
    return hr;
}



HRESULT
CIhvConnectivityProfile::GetParamBSTR
(
    BSTR*   pbstrValue
)
{
    HRESULT hr  =   S_OK;

    hr =
    GetTextFromNode
    (
        CON_PARAM1_XPATH,
        pbstrValue
    );
    BAIL_ON_FAILURE( hr );


error:
    return hr;
}



HRESULT
CIhvConnectivityProfile::SetParamBSTR
(
    BSTR    bstrNewValue
)
{
    HRESULT     hr  =   S_OK;

    hr =
    PutTextInNode
    (
        CON_PARAM1_XPATH,
        bstrNewValue
    );
    BAIL_ON_FAILURE( hr );


error:
    return hr;
}

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

    pIhvProfile = (PIHV_CONNECTIVITY_PROFILE) malloc( sizeof( IHV_CONNECTIVITY_PROFILE ) );
    if ( !pIhvProfile )
    {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE( hr );
    }
    ZeroMemory( pIhvProfile, sizeof( IHV_CONNECTIVITY_PROFILE ) );

    hr =
    GetParamBSTR
    (
        &bstrParam2
    );
    BAIL_ON_FAILURE( hr );

    if ( NULL == bstrParam2 ) {
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
    GetParamDWORD
    (
        &(pIhvProfile->dwParam1)
    );
    BAIL_ON_FAILURE( hr );

    // Transfering local cache to OUT parameter.
    (*ppvData) = pIhvProfile;
    pIhvProfile = NULL;

error:
    if ( pIhvProfile )
    {
        free( pIhvProfile->pszParam2 ); // NULL Safe.
        free( pIhvProfile );
    }
    SYS_FREE_STRING( bstrParam2 );
    return hr;
}



///////////////////////////////////////////

LPCWSTR     gppszIhvAuthTypes[] =
{
    L"IHVAuthV1",
    L"IHVAuthV2",
    L"IHVAuthV3"
};

LPCWSTR     gppszIhvSecurityTypes[] =
{
    L"IHVSecurityV1",
    L"IHVSecurityV2",
};

LPCWSTR     gppszIhvCipherTypes[] =
{
    L"None",
    L"IHVCipher1",
    L"IHVCipher2",
    L"IHVCipher3"
};


WCHAR	g_szDefaultSecurityProfile[] = 
L"<IhvSecurity xmlns=\"http://www.sampleihv.com/nwifi/profile\">"
L"            <IHVUsesFullSecurity>TRUE</IHVUsesFullSecurity>"
L"            <IHVAuthentication>IHVAuthV1</IHVAuthentication>"
L"            <IHVEncryption>IHVCipher1</IHVEncryption>"
L"            <IHVSecurityParam1>0</IHVSecurityParam1>"
L"            <IHVSecurityParam2>parameter value</IHVSecurityParam2>"
L"</IhvSecurity>";



HRESULT
CIhvSecurityProfile::GetDefaultXml
(
    BSTR* pbstrDefault
)
{
    SetModified();
	return Wstr2Bstr( g_szDefaultSecurityProfile, pbstrDefault );
}

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

    pIhvProfile = (PIHV_SECURITY_PROFILE) malloc( sizeof( IHV_SECURITY_PROFILE ) );
    if ( !pIhvProfile )
    {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE( hr );
    }
    ZeroMemory( pIhvProfile, sizeof( IHV_SECURITY_PROFILE ) );

    hr =
    GetFullSecurityFlag
    (
        &(pIhvProfile->bUseFullSecurity)
    );
    BAIL_ON_FAILURE( hr );

    hr =
    GetAuthType
    (
        &(pIhvProfile->AuthType)
    );
    BAIL_ON_FAILURE( hr );

    hr =
    GetCipherType
    (
        &(pIhvProfile->CipherType)
    );
    BAIL_ON_FAILURE( hr );

    hr =
    GetParamDWORD
    (
        &(pIhvProfile->dwParam1)
    );
    BAIL_ON_FAILURE( hr );

    hr =
    GetParamBSTR
    (
        &bstrParam2
    );
    BAIL_ON_FAILURE( hr );

    if ( NULL == bstrParam2 ) {
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

    // Transfering local cache to OUT parameter.
    (*ppvData) = pIhvProfile;
    pIhvProfile = NULL;

error:
    if ( pIhvProfile )
    {
        free( pIhvProfile->pszParam2 ); // NULL Safe.
        free( pIhvProfile );
    }
    SYS_FREE_STRING( bstrParam2 );
    return hr;

}



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


HRESULT
CIhvSecurityProfile::SetFullSecurityFlag
(
    BOOL    bUseFullSecurity
)
{
    HRESULT     hr              =   S_OK;
    BSTR        bstrText        =   NULL;

    hr =
    Bool2Bstr
    (
        bUseFullSecurity,
        &bstrText
    );
    BAIL_ON_FAILURE( hr );

    hr =
    PutTextInNode
    (
        SEC_FSFLAG_XPATH,
        bstrText
    );
    BAIL_ON_FAILURE( hr );

error:
    SYS_FREE_STRING( bstrText );
    return hr;
}



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


HRESULT
CIhvSecurityProfile::SetAuthType
(
    IHV_AUTH_TYPE  AuthType
)
{
    HRESULT     hr              =   S_OK;
    BSTR        bstrText        =   NULL;

    hr =
    AuthType2Bstr
    (
        AuthType,
        &bstrText
    );
    BAIL_ON_FAILURE( hr );

    hr =
    PutTextInNode
    (
        SEC_ATYPE_XPATH,
        bstrText
    );
    BAIL_ON_FAILURE( hr );

error:
    SYS_FREE_STRING( bstrText );
    return hr;
}


HRESULT
CIhvSecurityProfile::GetSecurityType
(
    PIHV_SECURITY_TYPE  pSecurityType
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

    hr =
    Wstr2SecurityType
    (
        bstrData,
        pSecurityType
    );
    BAIL_ON_FAILURE( hr );

error:
    SYS_FREE_STRING( bstrData );
    return hr;
}

HRESULT
CIhvSecurityProfile::SetSecurityType
(
    IHV_SECURITY_TYPE  SecurityType
)
{
    HRESULT     hr              =   S_OK;
    BSTR        bstrText        =   NULL;

    hr =
    SecurityType2Bstr
    (
        SecurityType,
        &bstrText
    );
    BAIL_ON_FAILURE( hr );

    hr =
    PutTextInNode
    (
        SEC_ATYPE_XPATH,
        bstrText
    );
    BAIL_ON_FAILURE( hr );

error:
    SYS_FREE_STRING( bstrText );
    return hr;
}


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


HRESULT
CIhvSecurityProfile::SetCipherType
(
    IHV_CIPHER_TYPE  CipherType
)
{
    HRESULT     hr              =   S_OK;
    BSTR        bstrText        =   NULL;

    hr =
    CipherType2Bstr
    (
        CipherType,
        &bstrText
    );
    BAIL_ON_FAILURE( hr );

    hr =
    PutTextInNode
    (
        SEC_ETYPE_XPATH,
        bstrText
    );
    BAIL_ON_FAILURE( hr );

error:
    SYS_FREE_STRING( bstrText );
    return hr;
}


HRESULT
CIhvSecurityProfile::GetParamDWORD
(
    DWORD*  pdwParam1
)
{
    HRESULT     hr          =   S_OK;
    BSTR        bstrData    =   NULL;

    hr =
    GetTextFromNode
    (
        SEC_PARAM2_XPATH,
        &bstrData
    );
    BAIL_ON_FAILURE( hr );

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


HRESULT
CIhvSecurityProfile::SetParamDWORD
(
    DWORD   dwNewValue
)
{
    HRESULT     hr              =   S_OK;
    BSTR        bstrText        =   NULL;

    hr =
    Dword2Bstr
    (
        dwNewValue,
        &bstrText
    );
    BAIL_ON_FAILURE( hr );

    hr =
    PutTextInNode
    (
        SEC_PARAM2_XPATH,
        bstrText
    );
    BAIL_ON_FAILURE( hr );

error:
    SYS_FREE_STRING( bstrText );
    return hr;
}



HRESULT
CIhvSecurityProfile::GetParamBSTR
(
    BSTR*   pbstrValue
)
{
    HRESULT hr  =   S_OK;

    hr =
    GetTextFromNode
    (
        SEC_PARAM1_XPATH,
        pbstrValue
    );
    BAIL_ON_FAILURE( hr );


error:
    return hr;
}


HRESULT
CIhvSecurityProfile::SetParamBSTR
(
    BSTR    bstrNewValue
)
{
    HRESULT     hr  =   S_OK;

    hr =
    PutTextInNode
    (
        SEC_PARAM1_XPATH,
        bstrNewValue
    );
    BAIL_ON_FAILURE( hr );


error:
    return hr;
}
