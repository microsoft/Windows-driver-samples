//
// Copyright (C) Microsoft Corporation 2005
// IHV UI Extension sample
//

#pragma once

#ifndef _IHVSAMPLEPROFILE_H
#define _IHVSAMPLEPROFILE_H

#define RELEASE_INTERFACE( _p ) if ( _p ) { (_p)->Release( ); (_p) = NULL;}


class CIhvProfileBase
{
public:

    // Constructor
    CIhvProfileBase( )
    {
        m_bModified = FALSE;
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

    HRESULT
    EmitXml
    (
        OUT BSTR*   pbstrIhvProfile
    );

    BOOL GetModified( ) { return m_bModified; }

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

    virtual
    HRESULT
    GetDefaultXml
    (
        BSTR* pbstrDefault
    )
    = 0;
    
    HRESULT
    GetTextFromNode
    (
        IN  LPCWSTR         pszQuery,
        OUT BSTR*           pbstrText
    );

    HRESULT
    PutTextInNode
    (
        IN  LPCWSTR         pszQuery,
        IN  BSTR            bstrText
    );


    VOID SetModified( ) { m_bModified = TRUE; }

    IXMLDOMElement*     m_pRootNode;

private:
    BOOL    m_bModified;
};

///////////////////////////////////////////


typedef struct _IHV_CONNECTIVITY_PROFILE
{
#define     CON_PARAM1_XPATH    L"/IhvConnectivity/IHVConnectivityParam1"
#define     CON_PARAM2_XPATH    L"/IhvConnectivity/IHVConnectivityParam2"

    DWORD       dwParam1;
    LPWSTR      pszParam2;
}
IHV_CONNECTIVITY_PROFILE, *PIHV_CONNECTIVITY_PROFILE;


extern WCHAR	g_szDefaultConnectivityProfile[];

class CIhvConnectivityProfile
    : public CIhvProfileBase
{
protected:
    HRESULT
    GetDefaultXml
    (
        BSTR* pbstrDefault
    );

public:

    // Constructor Destructor
    CIhvConnectivityProfile( ) { }
    ~CIhvConnectivityProfile( ) { }

    // Caller needs to know what type to
    // cast the pointer to depending upon
    // the type of the derived class.
    // Caller needs to free memory recursively
    // by using the free( ) function.
    HRESULT
    GetNativeData
    (
        LPVOID* ppvData
    );


    // Accessor and Modifier for dwParam1
    HRESULT
    GetParamDWORD
    (
        DWORD*  pdwParam1
    );
    HRESULT
    SetParamDWORD
    (
        DWORD   dwNewValue
    );

    // Accessor and Modifier for pszParam2
    HRESULT
    GetParamBSTR
    (
        BSTR*   pbstrValue
    );
    HRESULT
    SetParamBSTR
    (
        BSTR    bstrNewValue
    );

};



///////////////////////////////////////////

extern LPCWSTR     gppszIhvSecurityTypes[];

#define MAX_AUTH_TYPES 3
extern LPCWSTR     gppszIhvAuthTypes[];

#define MAX_CIPHER_TYPES 4
extern LPCWSTR     gppszIhvCipherTypes[];

typedef struct _IHV_SECURITY_PROFILE
{
#define     SEC_FSFLAG_XPATH    L"/IhvSecurity/IHVUsesFullSecurity"
#define     SEC_ATYPE_XPATH     L"/IhvSecurity/IHVAuthentication"
#define     SEC_ETYPE_XPATH     L"/IhvSecurity/IHVEncryption"
#define     SEC_PARAM1_XPATH    L"/IhvSecurity/IHVSecurityParam1"
#define     SEC_PARAM2_XPATH    L"/IhvSecurity/IHVSecurityParam2"

    BOOL            bUseFullSecurity;
    IHV_AUTH_TYPE   AuthType;
    IHV_CIPHER_TYPE CipherType;
    DWORD           dwParam1;
    LPWSTR          pszParam2;
}
IHV_SECURITY_PROFILE, *PIHV_SECURITY_PROFILE;


extern WCHAR	g_szDefaultSecurityProfile[];

class CIhvSecurityProfile
    : public CIhvProfileBase
{
protected:
    HRESULT
    GetDefaultXml
    (
        BSTR* pbstrDefault
    );

public:

    // Constructor Destructor
    CIhvSecurityProfile( ) { }
    ~CIhvSecurityProfile( ) { }

    // Caller needs to know what type to
    // cast the pointer to depending upon
    // the type of the derived class.
    // Caller needs to free memory recursively
    // by using the free( ) function.
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
    HRESULT
    SetFullSecurityFlag
    (
        BOOL    bUseFullSecurity
    );


    // Accessor and Modifier for AuthType
    HRESULT
    GetAuthType
    (
        PIHV_AUTH_TYPE  pAuthType
    );
    HRESULT
    SetAuthType
    (
        IHV_AUTH_TYPE  AuthType
    );

    // Accessor and Modifier for SecurityType
    HRESULT
    GetSecurityType
    (
        PIHV_SECURITY_TYPE  pSecurityType
    );
    HRESULT
    SetSecurityType
    (
        IHV_SECURITY_TYPE  SecurityType
    );    

    // Accessor and Modifier for CipherType
    HRESULT
    GetCipherType
    (
        PIHV_CIPHER_TYPE  pCipherType
    );
    HRESULT
    SetCipherType
    (
        IHV_CIPHER_TYPE  CipherType
    );

    // Accessor and Modifier for dwParam1
    HRESULT
    GetParamDWORD
    (
        DWORD*  pdwParam1
    );
    HRESULT
    SetParamDWORD
    (
        DWORD   dwNewValue
    );

    // Accessor and Modifier for pszParam2
    HRESULT
    GetParamBSTR
    (
        BSTR*   pbstrValue
    );
    HRESULT
    SetParamBSTR
    (
        BSTR    bstrNewValue
    );

};

#endif _IHVSAMPLEPROFILE_H

