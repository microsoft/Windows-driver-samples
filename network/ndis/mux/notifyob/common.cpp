//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992-2001.
//
//  File:       COMMON.CPP
//
//  Contents:   Debug & Utility functions
//
//  Notes:
//
//  Author:     Alok Sinha
//
//----------------------------------------------------------------------------

#include <windows.h>
#include <stdio.h>
#include <netcfgn.h>
#include "common.h"

#ifdef DBG

void
TraceMsg (
    _In_ LPWSTR szFormat,
    ...)
{
    static WCHAR szTempBuf[4096];

    va_list arglist;

    va_start(arglist, szFormat);

    StringCchVPrintfW ( szTempBuf,
        celems(szTempBuf),
        szFormat,
        arglist );

    OutputDebugStringW( szTempBuf );

    va_end(arglist);
}

void DumpChangeFlag (DWORD dwChangeFlag)
{
    TraceMsg( L"   ChangeFlag:" );

    if ( dwChangeFlag & NCN_ADD ) {
        TraceMsg( L" NCN_ADD" );
    }

    if ( dwChangeFlag & NCN_REMOVE ) {
        TraceMsg( L" NCN_REMOVE" );
    }

    if ( dwChangeFlag & NCN_UPDATE ) {
        TraceMsg( L" NCN_UPDATE" );
    }

    if ( dwChangeFlag & NCN_ENABLE ) {
        TraceMsg( L" NCN_ENABLE" );
    }

    if ( dwChangeFlag & NCN_DISABLE ) {
        TraceMsg( L" NCN_DISABLE" );
    }

    if ( dwChangeFlag & NCN_BINDING_PATH ) {
        TraceMsg( L" NCN_BINDING_PATH" );
    }

    if ( dwChangeFlag & NCN_PROPERTYCHANGE ) {
        TraceMsg( L" NCN_PROPERTYCHANGE" );
    }

    if ( dwChangeFlag & NCN_NET ) {
        TraceMsg( L" NCN_NET" );
    }

    if ( dwChangeFlag & NCN_NETTRANS ) {
        TraceMsg( L" NCN_NETTRANS" );
    }

    if ( dwChangeFlag & NCN_NETCLIENT ) {
        TraceMsg( L" NCN_NETCLIENT" );
    }

    if ( dwChangeFlag & NCN_NETSERVICE ) {
        TraceMsg( L" NCN_NETSERVICE" );
    }

    TraceMsg( L"\n" );
    return;
}

void DumpBindingPath (INetCfgBindingPath *pncbp)
{
    LPWSTR                       lpsz;
    HRESULT                      hr;

#ifdef VERBOSE_TRACE
    INetCfgComponent             *pncc;
    IEnumNetCfgBindingInterface  *pencbi;
    INetCfgBindingInterface      *pncbi;
    DWORD                        dwIndex;
    ULONG                        ulCount;  
#endif 

    hr = pncbp->GetPathToken( &lpsz );

    if ( hr == S_OK ) {

        TraceMsg( L"   BindingPath: %s\n",
               lpsz );

        CoTaskMemFree( lpsz );
    }
    else {

    TraceMsg( L"   BindingPath: GetPathToken failed(HRESULT %x).\n",
           hr );
    }

#ifdef VERBOSE_TRACE

    hr = pncbp->EnumBindingInterfaces( &pencbi );

    if ( hr == S_OK ) {

        hr = pencbi ->Next( 1, &pncbi, &ulCount );

        for (dwIndex=0; hr == S_OK; dwIndex++ ) {

            hr = pncbi->GetName( &lpsz );

            if ( hr == S_OK ) {

               TraceMsg( L"   BindingInterface(%d): %s\n",
                         dwIndex, lpsz );

               CoTaskMemFree( lpsz );
            }
            else {

               TraceMsg( L"   BindingInterface(%d): GetName failed(HRESULT %x).\n",
                         dwIndex, hr );
            }

            hr = pncbi->GetUpperComponent( &pncc );

            if ( hr == S_OK ) {

                TraceMsg( L"   \tUpperComponent of the interface(%d)...\n",
                         dwIndex );

                DumpComponent( pncc );

                ReleaseObj( pncc );
            }
            else {

                TraceMsg( L"   UpperComponent: GetUpperComponent failed(HRESULT = %x).\n",
                         hr );
            }
            hr = pncbi->GetLowerComponent( &pncc );

            if ( hr == S_OK ) {

                TraceMsg( L"   \tLowerComponent of the interface(%d)...\n",
                         dwIndex );
                DumpComponent( pncc );

                ReleaseObj( pncc );
            }
            else {

                TraceMsg( L"   LowerComponent: GetLowerComponent failed(HRESULT = %x).\n",
                         hr );
            }

            ReleaseObj( pncbi );

            hr = pencbi ->Next( 1,
                                     &pncbi,
                                     &ulCount );
        }

        ReleaseObj( pencbi );
    }
    else {

        TraceMsg( L"   EnumBindingInterfaces failed, (HRESULT = %x)\n",
                  hr ); 
    }
#endif 
    return;
}

void DumpComponent (INetCfgComponent *pncc)
{
    LPWSTR  lpsz;
    DWORD   dwChars;
    ULONG   ulStatus;
    HRESULT hr;
    hr = pncc->GetDisplayName( &lpsz );
   
    if ( hr == S_OK ) {

        TraceMsg( L"   \t\tComponent: %s\n",
               lpsz );

        CoTaskMemFree( lpsz );
    }
    else {

        TraceMsg( L"   GetDisplay failed(HRESULT %x).\n",
               hr );
    }

    hr = pncc->GetCharacteristics( &dwChars );

    if ( hr == S_OK ) {

        TraceMsg( L"   \t\tCharacteristics:" );

        if ( dwChars & NCF_HIDDEN ) {
           TraceMsg( L" NCF_HIDDEN" );
        }

        if ( dwChars & NCF_NO_SERVICE ) {
           TraceMsg( L" NCF_NO_SERVICE" );
        }

        if ( dwChars & NCF_VIRTUAL ) {
           TraceMsg( L" NCF_VIRTUAL" );
        }

        if ( dwChars & NCF_PHYSICAL ) {
           TraceMsg( L" NCF_PHYSICAL" );
        }

        if ( dwChars & NCF_FILTER ) {
           TraceMsg( L" NCF_FILTER" );
        }

        if ( dwChars & NCF_NOT_USER_REMOVABLE ) {
           TraceMsg( L" NCF_NOT_USER_REMOVABLE" );
        }

        if ( dwChars & NCF_HAS_UI ) {
           TraceMsg( L" NCF_HAS_UI" );
        }

        if ( dwChars & NCF_SOFTWARE_ENUMERATED ) {
           TraceMsg( L" NCF_SOFTWARE_ENUMERATED" );
        }

        if ( dwChars & NCF_MULTIPORT_INSTANCED_ADAPTER ) {
           TraceMsg( L" NCF_MULTIPORT_INSTANCED_ADAPTER" );
        }

        TraceMsg( L"\n" );
    }
    else {

        TraceMsg( L"   GetCharacteristics failed(HRESULT %x).\n",
               hr );
    }

    hr = pncc->GetId( &lpsz );

    if ( hr == S_OK ) {

        TraceMsg( L"   \t\tHardware Id: %s\n",
               lpsz );

        CoTaskMemFree( lpsz );
    }
    else {

        TraceMsg( L"   GetId failed(HRESULT %x).\n",
               hr );
    }

    hr = pncc->GetBindName( &lpsz );

    if ( hr == S_OK ) {

        TraceMsg( L"   \t\tBindName: %s\n",
               lpsz );

        CoTaskMemFree( lpsz );
    }
    else {

        TraceMsg( L"   GetBindName failed(HRESULT %x).\n",
               hr );
    }

    if ( dwChars & NCF_PHYSICAL ) {
        hr = pncc->GetDeviceStatus( &ulStatus );

        if ( hr == S_OK ) {

            TraceMsg( L"   \t\tDeviceStatus: %#x\n",
                      ulStatus );
        }
        else {

            TraceMsg( L"   GetDeviceStatus failed(HRESULT %x).\n",
                      hr );
        }
    }

  return;
}

#endif

HRESULT HrFindInstance (INetCfg *pnc,
                        GUID &guidInstance,
                        INetCfgComponent **ppnccMiniport)
{
    IEnumNetCfgComponent  *pencc;
    INetCfgComponent      *pncc;
    GUID                  guid;
    WCHAR                 szGuid[MAX_PATH+1];
    ULONG                 ulCount;
    BOOL                  found;
    HRESULT               hr;
    int                   numChars;

    TraceMsg( L"-->HrFindInstance.\n" );

    hr = pnc->EnumComponents( &GUID_DEVCLASS_NET,
                            &pencc );

    if ( hr == S_OK ) {

        numChars = StringFromGUID2( guidInstance,
                      szGuid,
                      MAX_PATH+1 );

        if (numChars == 0) {

            ReleaseObj(pencc);

            return HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW);
        }

        TraceMsg( L"  Looking for component with InstanceGuid %s\n",
               szGuid );

        hr = pencc->Next( 1,
                          &pncc,
                          &ulCount );

        for ( found=FALSE; (hr == S_OK) && (found == FALSE); ) {

            hr = pncc->GetInstanceGuid( &guid );

            if ( hr == S_OK ) {

                numChars = StringFromGUID2( guid,
                                szGuid,
                                MAX_PATH+1 );

                if (numChars == 0) {

                    ReleaseObj(pncc);

                    ReleaseObj(pencc);

                    return HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW);
                }


                TraceMsg( L"  Found component with InstanceGuid %s\n",
                         szGuid );

                found = IsEqualGUID( guid,
                                    guidInstance );

                if ( found == FALSE ) {

                    ReleaseObj( pncc );

                    hr = pencc->Next( 1,
                                    &pncc,
                                    &ulCount );
                }
                else {
                    *ppnccMiniport = pncc;
                }
            }
        }   

        ReleaseObj( pencc );
    }
    else {

     TraceMsg( L"   EnumComponents failed(HRESULT = %x).\n",
               hr );
    }

    TraceMsg( L"<--HrFindInstance(HRESULT = %x).\n",
            hr );

    return hr;
}

LONG
AddToMultiSzValue (
    HKEY hkeyAdapterGuid,
    _In_ LPWSTR szMiniportGuid)
{
    LPWSTR lpCurrentValue=NULL;
    LPWSTR lpNewValue=NULL;
    DWORD  dwLen;
    DWORD  dwNewLen;
    LONG   lResult;

    dwLen = 0;
    lResult =  RegQueryValueExW(
                        hkeyAdapterGuid,
                        c_szUpperBindings,
                        NULL,
                        NULL,
                        NULL,
                        &dwLen );
    if( lResult != ERROR_SUCCESS ){
       //you may do something
    }

    if ( dwLen != 0 ) {
        lpCurrentValue = (LPWSTR)calloc( dwLen, 1 );

        if ( lpCurrentValue ) {

        lResult =  RegQueryValueExW( hkeyAdapterGuid,
                                     c_szUpperBindings,
                                     NULL,
                                     NULL,
                                     (LPBYTE)lpCurrentValue,
                                     &dwLen );

        }
        else {

            lResult = ERROR_NOT_ENOUGH_MEMORY;
        }
    }
    else {
        dwLen = sizeof(WCHAR);
        lpCurrentValue = (LPWSTR)calloc( dwLen, 1 );

        if ( !lpCurrentValue ) {
            lResult = ERROR_NOT_ENOUGH_MEMORY;
        }
        else {
            lResult = ERROR_SUCCESS;
        }
    }

    if ( lResult == ERROR_SUCCESS ) {

        dwNewLen =(DWORD) (dwLen + ((wcslen(szMiniportGuid) + 1) * sizeof(WCHAR)));

        lpNewValue = (LPWSTR)malloc( dwNewLen );

        if ( lpNewValue ) {

            StringCbCopyW ( lpNewValue,
                dwNewLen,
                szMiniportGuid );

            CopyMemory( lpNewValue+wcslen(szMiniportGuid)+1,
                        lpCurrentValue,
                        dwLen );

            lResult = RegSetValueExW( hkeyAdapterGuid,
                                    c_szUpperBindings,
                                    0,
                                    REG_MULTI_SZ,
                                    (LPBYTE)lpNewValue,
                                    dwNewLen );
        }
        else {
            lResult = ERROR_NOT_ENOUGH_MEMORY;
        }
    }

    if ( lpCurrentValue ) {

#ifndef DISABLE_PREFAST_PRAGMA
#pragma prefast(suppress:__WARNING_MISSING_ZERO_TERMINATION, "lpCurrentValue is used as a counted string.")
#endif

        free( lpCurrentValue );
    }

    if ( lpNewValue ) {
        free( lpNewValue );
    }

    return lResult;
}

LONG
DeleteFromMultiSzValue (
    HKEY hkeyAdapterGuid,
    _In_ LPWSTR szMiniportGuid)
{
    LPWSTR lpCurrentValue=NULL;
    LPWSTR lpNewValue=NULL;
    LPWSTR lpCurrentValueTemp;
    LPWSTR lpNewValueTemp;
    DWORD  dwLen;
    DWORD  dwNewLen;
    LONG   lResult;

    dwLen = 0;
    lResult =  RegQueryValueExW(
                        hkeyAdapterGuid,
                        c_szUpperBindings,
                        NULL,
                        NULL,
                        NULL,
                        &dwLen );

    if ( lResult == ERROR_SUCCESS ) {
        lpCurrentValue = (LPWSTR)calloc( dwLen, 1 );
        lpNewValue = (LPWSTR)calloc( dwLen, 1 );

        if ( (lpCurrentValue != NULL) && (lpNewValue != NULL) ) {

            lResult =  RegQueryValueExW(
                              hkeyAdapterGuid,
                              c_szUpperBindings,
                              NULL,
                              NULL,
                              (LPBYTE)lpCurrentValue,
                              &dwLen );

            if ( lResult == ERROR_SUCCESS ) {

                lpCurrentValueTemp = lpCurrentValue;
                lpNewValueTemp = lpNewValue;

                dwNewLen = 0;

                lpCurrentValueTemp[dwLen-1]='\0';
                while( wcslen(lpCurrentValueTemp) > 0) {

                    //if a register in the existing register sequence do not match szMiniportGuid, copy to new register sequence 
                    if ( _wcsicmp(lpCurrentValueTemp, szMiniportGuid) != 0 ) {
                        StringCchCopyW ( lpNewValueTemp,
                        wcslen(lpCurrentValueTemp), //size of the register
                        lpCurrentValueTemp );
                                                
                        *(lpNewValueTemp+=wcslen(lpCurrentValueTemp)+1)='\0';
                        
                        lpNewValueTemp += wcslen(lpNewValueTemp) + 1;
                        dwNewLen += (DWORD)wcslen(lpNewValueTemp) + 1;
                    }

                    lpCurrentValueTemp += wcslen(lpCurrentValueTemp) + 1;
                } //end of while

                //the minimum length of a register sequence is 3 (start with '\\' and end with '\0') eg: '\\','a','\0'
                if ( dwNewLen > 2 ) {
                    lResult = RegSetValueExW( hkeyAdapterGuid,
                                          c_szUpperBindings,
                                          0,
                                          REG_MULTI_SZ,
                                          (LPBYTE)lpNewValue,
                                          dwNewLen );
                }
                else {
                    lResult = RegDeleteValueW( hkeyAdapterGuid,
                                               c_szUpperBindings );
                }
            }
        }
        else {
            lResult = ERROR_NOT_ENOUGH_MEMORY;
        }
    }

    if ( lpCurrentValue ) {
        free( lpCurrentValue );
    }

    if ( lpNewValue ) {
        free( lpNewValue );
    }

    return lResult;
}

LPWSTR
AddDevicePrefix (
    _In_ LPWSTR lpStr)
{
    LPWSTR lpNewStr;

    size_t cchNewStr = wcslen(lpStr) + wcslen(c_szDevicePrefix) + 1;
    lpNewStr = (LPWSTR)malloc( cchNewStr * sizeof(WCHAR) );    
    if ( lpNewStr )
    {
        StringCchCopyW (lpNewStr,
            cchNewStr,
            c_szDevicePrefix );
        lpNewStr[cchNewStr-1]='\0';
        StringCchCatW ( lpNewStr,
            cchNewStr,
            lpStr );
    }

    return lpNewStr;
}

LPWSTR
RemoveDevicePrefix (
    _In_ LPWSTR lpStr)
{
    LPWSTR lpNewStr;
    LPWSTR lpTemp;

    lpTemp = wcsrchr( lpStr, '\\' );

    if ( lpTemp != NULL ) {

        lpNewStr = _wcsdup( lpTemp+1 );
    }
    else {
        lpNewStr = NULL;
    }

    return lpNewStr;
}

