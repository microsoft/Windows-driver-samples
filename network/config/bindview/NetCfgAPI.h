//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2001.
//
//  File:       N E T C F G A P I . H
//
//  Contents:   Functions Prototypes
//
//  Notes:      
//
//  Author:     Alok Sinha    15-May-01
//
//----------------------------------------------------------------------------

#ifndef _NETCFGAPI_H_INCLUDED

#define _NETCFGAPI_H_INCLUDED


#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <wchar.h>
#include <netcfgx.h>
#include <netcfgn.h>
#include <setupapi.h>
#include <devguid.h>
#include <objbase.h>
#include <strsafe.h>

#define celems(_x)          (sizeof(_x) / sizeof(_x[0]))

#define LOCK_TIME_OUT     5000

HRESULT HrGetINetCfg (IN BOOL fGetWriteLock,
                      IN LPCWSTR lpszAppName,
                      OUT INetCfg** ppnc,
                      _Outptr_opt_result_maybenull_ LPWSTR *lpszLockedBy);

HRESULT HrReleaseINetCfg (INetCfg* pnc,
                          BOOL fHasWriteLock);

HRESULT HrInstallNetComponent (IN INetCfg *pnc,
                               IN LPCWSTR szComponentId,
                               IN const GUID    *pguildClass,
                               IN LPCWSTR lpszInfFullPath);

HRESULT HrInstallComponent(IN INetCfg* pnc,
                           IN LPCWSTR szComponentId,
                           IN const GUID* pguidClass);

HRESULT HrUninstallNetComponent(IN INetCfg* pnc,
                                IN LPCWSTR szComponentId);

HRESULT HrGetComponentEnum (INetCfg* pnc,
                            IN const GUID* pguidClass,
                            IEnumNetCfgComponent **ppencc);

HRESULT HrGetFirstComponent (IEnumNetCfgComponent* pencc,
                             INetCfgComponent **ppncc);

HRESULT HrGetNextComponent (IEnumNetCfgComponent* pencc,
                            INetCfgComponent **ppncc);

HRESULT HrFindNetComponentByPnpId (IN INetCfg *pnc,
                                   IN _In_ LPWSTR lpszPnpDevNodeId,
                                   OUT INetCfgComponent **ppncc);

HRESULT HrGetBindingPathEnum (INetCfgComponent *pncc,
                              DWORD dwBindingType,
                              IEnumNetCfgBindingPath **ppencbp);

HRESULT HrGetFirstBindingPath (IEnumNetCfgBindingPath *pencbp,
                               INetCfgBindingPath **ppncbp);

HRESULT HrGetNextBindingPath (IEnumNetCfgBindingPath *pencbp,
                               INetCfgBindingPath **ppncbp);

HRESULT HrGetBindingInterfaceEnum (INetCfgBindingPath *pncbp,
                                   IEnumNetCfgBindingInterface **ppencbi);

HRESULT HrGetFirstBindingInterface (IEnumNetCfgBindingInterface *pencbi,
                                    INetCfgBindingInterface **ppncbi);

HRESULT HrGetNextBindingInterface (IEnumNetCfgBindingInterface *pencbi,
                                   INetCfgBindingInterface **ppncbi);

VOID ReleaseRef (IUnknown* punk);

#endif

