////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2014 Microsoft Corporation.  All Rights Reserved.
//
//   Module Name:
//      HelperFunctions_InjectionData.h
//
//   Abstract:
//      This module contains prototypes for kernel helper functions that assist with INJECTION_DATA.
//
//   Author:
//      Dusty Harper      (DHarper)
//
//   Revision History:
//
//      [ Month ][Day] [Year] - [Revision]-[ Comments ]
//      May       01,   2010  -     1.0   -  Creation
//      December  13,   2013  -     1.1   -  Enhance annotations, add multiple injector support, and 
//                                              add support for controlData.
//
////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef HELPERFUNCTIONS_INJECTION_DATA_H
#define HELPERFUNCTIONS_INJECTION_DATA_H

typedef struct INJECTION_DATA_
{
   ADDRESS_FAMILY              addressFamily;
   FWP_DIRECTION               direction;
   BOOLEAN                     isIPsecSecured;
   HANDLE                      injectionHandle;
   HANDLE                      injectionContext;
   FWPS_PACKET_INJECTION_STATE injectionState;
   VOID*                       pContext;
   BYTE*                       pControlData;
   UINT32                      controlDataLength;
}INJECTION_DATA, *PINJECTION_DATA;

typedef struct INJECTION_HANDLE_DATA_
{
   HANDLE* pMACHandle;
   HANDLE* pVSwitchEthernetHandle;
   HANDLE* pForwardHandle;
   HANDLE* pNetworkHandle;
   HANDLE* pTransportHandle;
   HANDLE* pStreamHandle;
}INJECTION_HANDLE_DATA, *PINJECTION_HANDLE_DATA;

_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
inline VOID KrnlHlprInjectionDataPurge(_Inout_ INJECTION_DATA* pInjectionData);

_At_(*ppInjectionData, _Pre_ _Notnull_)
_At_(*ppInjectionData, _Post_ _Null_ __drv_freesMem(Pool))
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Success_(*ppInjectionData == 0)
inline VOID KrnlHlprInjectionDataDestroy(_Inout_ INJECTION_DATA** ppInjectionData);

_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Check_return_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprInjectionDataPopulate(_Inout_ INJECTION_DATA* pInjectionData,
                                       _In_ const FWPS_INCOMING_VALUES* pClassifyValues,
                                       _In_ const FWPS_INCOMING_METADATA_VALUES0* pMetadataValues,
                                       _In_opt_ const NET_BUFFER_LIST* pNetBufferList);

_At_(*ppInjectionData, _Pre_ _Null_)
_When_(return != STATUS_SUCCESS, _At_(*ppInjectionData, _Post_ _Null_))
_When_(return == STATUS_SUCCESS, _At_(*ppInjectionData, _Post_ _Notnull_ __drv_allocatesMem(Pool)))
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Check_return_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprInjectionDataCreate(_Outptr_ INJECTION_DATA** ppInjectionData,
                                     _In_ const FWPS_INCOMING_VALUES* pClassifyValues,
                                     _In_ const FWPS_INCOMING_METADATA_VALUES0* pMetadataValues,
                                     _In_opt_ const NET_BUFFER_LIST* pNetBufferList,
                                     _In_ const FWPS_FILTER* pFilter);

_At_(*ppInjectionHandleData, _Pre_ _Notnull_)
_At_(*ppInjectionHandleData, _Post_ _Null_ __drv_freesMem(Pool))
_IRQL_requires_(PASSIVE_LEVEL)
_IRQL_requires_same_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprInjectionHandleDataDestroy(_Inout_ INJECTION_HANDLE_DATA** ppInjectionHandleData);

_At_(*ppInjectionHandleData, _Pre_ _Null_)
_When_(return != STATUS_SUCCESS, _At_(*ppInjectionHandleData, _Post_ _Null_))
_When_(return == STATUS_SUCCESS, _At_(*ppInjectionHandleData, _Post_ _Notnull_ __drv_allocatesMem(Pool)))
_IRQL_requires_(PASSIVE_LEVEL)
_IRQL_requires_same_
_Check_return_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprInjectionHandleDataCreate(_Outptr_  INJECTION_HANDLE_DATA** ppInjectionHandleData,
                                           _In_ ADDRESS_FAMILY addressFamily = AF_INET,
                                           _In_ BOOLEAN isInbound = TRUE,
                                           _In_ UINT32 index = WFPSAMPLER_INDEX);

#endif /// HELPERFUNCTIONS_INJECTION_DATA_H