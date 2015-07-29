/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.


Module Name:

    Luminous.c

Abstract:

    Library for managing the state of all firefly devices

Environment:

    User Mode library

--*/
#include "luminous.h"

CLuminous::CLuminous(
    VOID
    )
{
    m_pIWbemServices = NULL;
    m_pIWbemClassObject = NULL;
    m_bCOMInitialized = FALSE;
}


CLuminous::~CLuminous(
    VOID
    )
{
    Close();
}


BOOL
CLuminous::Open(
    VOID
    )
{
    HRESULT           hResult;
    BOOL                bRet = FALSE;

    if (m_pIWbemServices) {

        return TRUE;
    }

    //
    // Initialize COM library. Must be done before invoking any
    // other COM function.
    //

    hResult = CoInitialize( NULL );

    if ( FAILED (hResult)) {
        _tprintf( TEXT("Error %lx: Failed to initialize COM library\n"), hResult );
        goto OpenCleanup;
    }

    m_bCOMInitialized = TRUE;

    m_pIWbemServices = ConnectToNamespace( NAME_SPACE);
    if (!m_pIWbemServices ) {
        _tprintf( TEXT("Could not connect name.\n") );
        goto OpenCleanup;
    }

     m_pIWbemClassObject = GetInstanceReference( m_pIWbemServices, CLASS_NAME);
     if ( !m_pIWbemClassObject ) {
        _tprintf( TEXT("Could not find the instance.\n") );
        goto OpenCleanup;
    }

     bRet = TRUE;

OpenCleanup:
    if(!bRet) {
        if(m_pIWbemClassObject) {
            m_pIWbemClassObject->Release();
            m_pIWbemClassObject = NULL;
        }
        if(m_pIWbemServices) {
            m_pIWbemServices->Release();
            m_pIWbemServices = NULL;
        }
        if(m_bCOMInitialized) {
            CoUninitialize();
            m_bCOMInitialized = FALSE;
        }
    }

    return bRet;
}

VOID
CLuminous::Close(
    VOID
    )
{
    if (m_pIWbemServices) {
        m_pIWbemServices->Release();
        m_pIWbemServices = NULL;
    }

    if (m_pIWbemClassObject) {
        m_pIWbemClassObject->Release();
        m_pIWbemClassObject = NULL;
    }

    if(m_bCOMInitialized) {
        CoUninitialize();
        m_bCOMInitialized = FALSE;
    }

}


BOOL
CLuminous::Get(
    _In_ BOOL *Enabled
    )
{

    VARIANT     varPropVal;
    BSTR          bstrPropertyName = NULL;
    HRESULT     hResult;
    CIMTYPE     cimType;
    BOOL          bRet= FALSE;

    if (!m_pIWbemServices || !m_pIWbemClassObject) {
        return FALSE;
    }

    VariantInit( &varPropVal );

    bstrPropertyName = AnsiToBstr( PROPERTY_NAME, -1 );

    if ( !bstrPropertyName ) {
        _tprintf( TEXT("Error out of memory.\n") );
        VariantClear( &varPropVal );
        return FALSE ;
    }

    //
    // Get the property value.
    //

    hResult = m_pIWbemClassObject->Get(
                             bstrPropertyName,
                             0,
                             &varPropVal,
                             &cimType,
                             NULL );

    if ( hResult != WBEM_S_NO_ERROR ) {

        _tprintf( TEXT("Error %lX: Failed to read property value of %s.\n"),
                        hResult, PROPERTY_NAME);

    } else {
        if(varPropVal.vt == VT_BOOL) {
            *Enabled = varPropVal.boolVal;
            bRet = TRUE;
        }
    }

    if(bstrPropertyName) {
        SysFreeString( bstrPropertyName );
    }

    VariantClear( &varPropVal );

    return bRet;

}

BOOL
CLuminous::Set(
    _In_ BOOL Enabled
    )
{
    VARIANT     varPropVal;
    BSTR          bstrPropertyName = NULL;
    HRESULT     hResult;
    CIMTYPE     cimType;
    BOOL         bRet = FALSE;
    LPTSTR      lpProperty = PROPERTY_NAME;

    if (!m_pIWbemServices || !m_pIWbemClassObject) {
        return FALSE;
    }

    VariantInit( &varPropVal );

    bstrPropertyName = AnsiToBstr( lpProperty, -1 );

    if ( !bstrPropertyName ) {
        _tprintf( TEXT("Error out of memory.\n") );
        goto End ;
    }

    //
    // Get the property value.
    //

    hResult = m_pIWbemClassObject->Get(
                             bstrPropertyName,
                             0,
                             &varPropVal,
                             &cimType,
                             NULL );

    if ( hResult != WBEM_S_NO_ERROR ) {

        _tprintf( TEXT("Error %lX: Failed to read property value of %s.\n"),
                            hResult, lpProperty );
        goto End;

    }

    if(varPropVal.vt == VT_BOOL) {
        varPropVal.boolVal = (VARIANT_BOOL)Enabled;

        //
        // Set the property value
        //

        hResult = m_pIWbemClassObject->Put(
                                    bstrPropertyName,
                                    0,
                                    &varPropVal,
                                    cimType
                                    );


        if ( hResult == WBEM_S_NO_ERROR ) {
            hResult = m_pIWbemServices->PutInstance(
                                            m_pIWbemClassObject,
                                            WBEM_FLAG_UPDATE_ONLY,
                                            NULL,
                                            NULL );

            if ( hResult != WBEM_S_NO_ERROR ) {
                _tprintf( TEXT("Failed to save the instance,")
                                    TEXT(" %s will not be updated.\n"),
                                    lpProperty );
            } else {
                bRet = TRUE;
            }
        }
        else {

            _tprintf( TEXT("Error %lX: Failed to set property value of %s.\n"),
            hResult, lpProperty );
        }
    }

End:

    if(bstrPropertyName) {
        SysFreeString( bstrPropertyName );
    }

    VariantClear( &varPropVal );

    return bRet;
}

//
// The function connects to the namespace specified by the user.
//

IWbemServices *ConnectToNamespace (_In_ LPTSTR chNamespace)
{
    IWbemServices *pIWbemServices = NULL;
    IWbemLocator  *pIWbemLocator = NULL;
    BSTR          bstrNamespace;
    HRESULT       hResult;

    //
    // Create an instance of WbemLocator interface.
    //

    hResult = CoCreateInstance(
                           CLSID_WbemLocator,
                           NULL,
                           CLSCTX_INPROC_SERVER,
                           IID_IWbemLocator,
                           (LPVOID *)&pIWbemLocator );

    if ( hResult != S_OK ) {
        _tprintf( TEXT("Error %lX: Could not create instance of IWbemLocator interface.\n"),
           hResult );
        return NULL;
    }

    //
    // Namespaces are passed to COM in BSTRs.
    //

    bstrNamespace = AnsiToBstr( chNamespace,   -1 );

    if ( !bstrNamespace ) {
        _tprintf( TEXT("Out of memory.\n") );
        pIWbemLocator->Release( );
        return NULL;
    }

    //
    // Using the locator, connect to COM in the given namespace.
    //

    hResult = pIWbemLocator->ConnectServer(
                  bstrNamespace,
                  NULL,   // NULL means current account, for simplicity.
                  NULL,   // NULL means current password, for simplicity.
                  0L,     // locale
                  0L,     // securityFlags
                  NULL,   // authority (domain for NTLM)
                  NULL,   // context
                  &pIWbemServices ); // Returned IWbemServices.

    //
    // Done with Namespace.
    //

    SysFreeString( bstrNamespace );

    if ( hResult != WBEM_S_NO_ERROR) {
        _tprintf( TEXT("Error %lX: Failed to connect to namespace %s.\n"),
                            hResult, chNamespace );

        pIWbemLocator->Release( );
        return NULL;
    }

    //
    // Switch the security level to IMPERSONATE so that provider(s)
    // will grant access to system-level objects, and so that
    // CALL authorization will be used.
    //

    hResult = CoSetProxyBlanket(
                   (IUnknown *)pIWbemServices, // proxy
                   RPC_C_AUTHN_WINNT,        // authentication service
                   RPC_C_AUTHZ_NONE,         // authorization service
                   NULL,                     // server principle name
                   RPC_C_AUTHN_LEVEL_CALL,   // authentication level
                   RPC_C_IMP_LEVEL_IMPERSONATE, // impersonation level
                   NULL,                     // identity of the client
                   0 );                      // capability flags

    if ( hResult != S_OK ) {
        _tprintf( TEXT("Error %lX: Failed to impersonate.\n"), hResult );

        pIWbemLocator->Release();
        pIWbemServices->Release();
        return NULL;
    }

    pIWbemLocator->Release();
    return pIWbemServices;
}

//
// The function returns an interface pointer to the instance given its
// list-index.
//

IWbemClassObject *GetInstanceReference (
                                    IWbemServices *pIWbemServices,
                                    _In_ LPTSTR lpClassName)
{
    IWbemClassObject     *pInst = NULL;
    IEnumWbemClassObject *pEnumInst;
    BSTR                 bstrClassName;
    BOOL                 bFound;
    ULONG                ulCount;
    HRESULT              hResult;


    bstrClassName = AnsiToBstr( lpClassName, -1 );

    if ( !bstrClassName ) {
        _tprintf( TEXT("Out of memory.\n") );
        return NULL;
    }

    //
    // Get Instance Enumerator Interface.
    //

    pEnumInst = NULL;

    hResult = pIWbemServices->CreateInstanceEnum(
                                bstrClassName,          // Name of the root class.
                                WBEM_FLAG_SHALLOW |     // Enumerate at current root only.
                                WBEM_FLAG_FORWARD_ONLY, // Forward-only enumeration.
                                NULL,                   // Context.
                                &pEnumInst );          // pointer to class enumerator

    if ( hResult != WBEM_S_NO_ERROR || pEnumInst == NULL ) {
        _tprintf( TEXT("Error %lX: Failed to get a reference")
        TEXT(" to instance enumerator.\n"), hResult );
    }
    else {
        //
        // Get pointer to the instance.
        //

        hResult = WBEM_S_NO_ERROR;
        bFound = FALSE;

        while ( (hResult == WBEM_S_NO_ERROR) && (bFound == FALSE) ) {

            hResult = pEnumInst->Next(
                                        2000,      // two seconds timeout
                                        1,         // return just one instance.
                                        &pInst,    // pointer to instance.
                                        &ulCount); // Number of instances returned.

            if ( ulCount > 0 ) {

                bFound = TRUE;
                break;
            }
        }

        if ( bFound == FALSE && pInst) {
            pInst->Release(  );
            pInst = NULL;
        }
    }

    //
    // Done with the instance enumerator.
    //

    if(pEnumInst) {
        pEnumInst->Release( );
    }
    SysFreeString( bstrClassName );

    return pInst;
}

//
// The function converts an ANSI string into BSTR and returns it in an
// allocated memory. The memory must be freed by the caller using free()
// function. If nLenSrc is -1, the string is null terminated.
//

BSTR AnsiToBstr (_In_ LPTSTR lpSrc, _In_ int nLenSrc)
{
    BSTR lpDest;

    //
    // In case of ANSI version, we need to change the ANSI string to UNICODE since
    // BSTRs are essentially UNICODE strings.
    //

#ifndef UNICODE
    int  nLenDest;

    nLenDest = MultiByteToWideChar( CP_ACP, 0, lpSrc, nLenSrc, NULL, 0);

    lpDest = SysAllocStringLen( NULL, nLenDest );

    if ( lpDest ) {
        MultiByteToWideChar( CP_ACP, 0, lpSrc, nLenSrc, lpDest, nLenDest );
    }

    //
    // In case of UNICODE version, we simply allocate memory and copy the string.
    //

#else
    if ( lpSrc == NULL ) {
        nLenSrc = 0;
    }
    else {
        if ( nLenSrc == - 1 ) {
            size_t temp = _tcslen( lpSrc );
            if (temp > INT_MAX - 1) {
                return NULL;
            }
            nLenSrc = (int) temp + 1;
        }
    }

    lpDest = SysAllocStringLen( lpSrc, nLenSrc );
#endif

    return lpDest;
}





