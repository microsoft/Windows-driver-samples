////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2014 Microsoft Corporation.  All Rights Reserved.
//
//   Module Name:
//      HelperFunctions_FwpObjects.h
//
//   Abstract:
//      This module contains prototypes for kernel helper functions that allocate, populate, purge, 
//         and free structures in memory.
//
//   Author:
//      Dusty Harper      (DHarper)
//
//   Revision History:
//
//      [ Month ][Day] [Year] - [Revision]-[ Comments ]
//      March     11,   2010  -     1.0   -  Creation
//      December  13,   2013  -     1.1   -  Add support for multiple injectors and add 
//                                              KrnlHlprFwpValueGetStringForFwpsIncomingValue, 
//                                              KrnlHlprFwpsRedirectHandleCreate,
//                                              KrnlHlprFwpsRedirecthandleDestroy,
//                                              KrnlHlprFwpsLayerIsDiscard, 
//                                              KrnlHlprFwpsLayerIDToString functions.
//
////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef HELPERFUNCTIONS_FWP_OBJECTS_H
#define HELPERFUNCTIONS_FWP_OBJECTS_H

#define ADDRESS_VALUE        1
#define FLAGS_VALUE          2
#define L2_FLAGS_VALUE       3
#define UNICODE_STRING_VALUE 4

extern HANDLE g_EngineHandle;
extern HANDLE g_pIPv4InboundMACInjectionHandles[2];
extern HANDLE g_pIPv4IngressVSwitchEthernetInjectionHandles[2];
extern HANDLE g_pIPv4InboundForwardInjectionHandles[2];
extern HANDLE g_pIPv4InboundNetworkInjectionHandles[2];
extern HANDLE g_pIPv4InboundTransportInjectionHandles[2];
extern HANDLE g_pIPv4InboundStreamInjectionHandles[2];
extern HANDLE g_pIPv4OutboundMACInjectionHandles[2];
extern HANDLE g_pIPv4EgressVSwitchEthernetInjectionHandles[2];
extern HANDLE g_pIPv4OutboundForwardInjectionHandles[2];
extern HANDLE g_pIPv4OutboundNetworkInjectionHandles[2];
extern HANDLE g_pIPv4OutboundTransportInjectionHandles[2];
extern HANDLE g_pIPv4OutboundStreamInjectionHandles[2];
extern HANDLE g_pIPv6InboundMACInjectionHandles[2];
extern HANDLE g_pIPv6IngressVSwitchEthernetInjectionHandles[2];
extern HANDLE g_pIPv6InboundForwardInjectionHandles[2];
extern HANDLE g_pIPv6InboundNetworkInjectionHandles[2];
extern HANDLE g_pIPv6InboundTransportInjectionHandles[2];
extern HANDLE g_pIPv6InboundStreamInjectionHandles[2];
extern HANDLE g_pIPv6OutboundMACInjectionHandles[2];
extern HANDLE g_pIPv6EgressVSwitchEthernetInjectionHandles[2];
extern HANDLE g_pIPv6OutboundForwardInjectionHandles[2];
extern HANDLE g_pIPv6OutboundNetworkInjectionHandles[2];
extern HANDLE g_pIPv6OutboundTransportInjectionHandles[2];
extern HANDLE g_pIPv6OutboundStreamInjectionHandles[2];
extern HANDLE g_pInboundMACInjectionHandles[2];
extern HANDLE g_pIngressVSwitchEthernetInjectionHandles[2];
extern HANDLE g_pInboundForwardInjectionHandles[2];
extern HANDLE g_pInboundNetworkInjectionHandles[2];
extern HANDLE g_pInboundTransportInjectionHandles[2];
extern HANDLE g_pInboundStreamInjectionHandles[2];
extern HANDLE g_pOutboundMACInjectionHandles[2];
extern HANDLE g_pEgressVSwitchEthernetInjectionHandles[2];
extern HANDLE g_pOutboundForwardInjectionHandles[2];
extern HANDLE g_pOutboundNetworkInjectionHandles[2];
extern HANDLE g_pOutboundTransportInjectionHandles[2];
extern HANDLE g_pOutboundStreamInjectionHandles[2];
extern HANDLE g_pRedirectionHandles[2];

_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
inline VOID KrnlHlprFwpByteBlobPurgeLocalCopy(_Inout_ FWP_BYTE_BLOB* pBlob);

_At_(*ppBlob, _Pre_ _Notnull_)
_At_(*ppBlob, _Post_ _Null_ __drv_freesMem(Pool))
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Success_(*ppBlob == 0)
inline VOID KrnlHlprFwpByteBlobDestroyLocalCopy(_Inout_ FWP_BYTE_BLOB** ppBlob);

_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Check_return_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprFwpByteBlobPopulateLocalCopy(_In_ const FWP_BYTE_BLOB* pOriginalBlob,
                                              _Inout_ FWP_BYTE_BLOB* pBlob);

__drv_allocatesMem(Pool)
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Must_inspect_result_
_Success_(return != 0)
FWP_BYTE_BLOB* KrnlHlprFwpByteBlobCreateLocalCopy(_In_ const FWP_BYTE_BLOB* pOriginalBlob);

_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
inline VOID KrnlHlprFwpmDisplayDataPurgeLocalCopy(_Inout_ FWPM_DISPLAY_DATA* pData);

_At_(*ppDisplayData, _Pre_ _Notnull_)
_At_(*ppDisplayData, _Post_ _Null_ __drv_freesMem(Pool))
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Success_(*ppDisplayData == 0)
inline VOID KrnlHlprFwpmDisplayDataDestroyLocalCopy(_Inout_ FWPM_DISPLAY_DATA** ppDisplayData);

_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Check_return_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprFwpmDisplayDataPopulateLocalCopy(_In_ const FWPM_DISPLAY_DATA* pOriginalData,
                                                  _Inout_ FWPM_DISPLAY_DATA* pData);

__drv_allocatesMem(Pool)
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Must_inspect_result_
   _Success_(return != 0)

FWPM_DISPLAY_DATA* KrnlHlprFwpmDisplayDataCreateLocalCopy(_In_ const FWPM_DISPLAY_DATA* pOriginalDisplayData);

_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
inline VOID KrnlHlprFwpmClassifyOptionPurgeLocalCopy(_Inout_ FWPM_CLASSIFY_OPTION* pOption);

_At_(*ppOption, _Pre_ _Notnull_)
_At_(*ppOption, _Post_ _Null_ __drv_freesMem(Pool))
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Success_(*ppOption == 0)
inline VOID KrnlHlprFwpmClassifyOptionDestroyLocalCopy(_Inout_ FWPM_CLASSIFY_OPTION** ppOption);

_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Check_return_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprFwpmClassifyOptionPopulateLocalCopy(_In_ const FWPM_CLASSIFY_OPTION* pOriginalOption,
                                                     _Inout_ FWPM_CLASSIFY_OPTION* pOption);

__drv_allocatesMem(Pool)
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Must_inspect_result_
_Success_(return != 0)
FWPM_CLASSIFY_OPTION* KrnlHlprFwpmClassifyOptionCreateLocalCopy(_In_ const FWPM_CLASSIFY_OPTION* pOption);

_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
inline VOID KrnlHlprFwpmClassifyOptionsPurgeLocalCopy(_Inout_ FWPM_CLASSIFY_OPTIONS* pOptions);

_At_(*ppOptions, _Pre_ _Notnull_)
_At_(*ppOptions, _Post_ _Null_ __drv_freesMem(Pool))
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Success_(*ppOptions == 0)
inline VOID KrnlHlprFwpmClassifyOptionsDestroyLocalCopy(_Inout_ FWPM_CLASSIFY_OPTIONS** ppOptions);

_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Check_return_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprFwpmClassifyOptionsPopulateLocalCopy(_In_ const FWPM_CLASSIFY_OPTIONS* pOriginalOptions,
                                                      _Inout_ FWPM_CLASSIFY_OPTIONS* pOptions);

__drv_allocatesMem(Pool)
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Must_inspect_result_
_Success_(return != 0)
FWPM_CLASSIFY_OPTIONS* KrnlHlprFwpmClassifyOptionsCreateLocalCopy(_In_ const FWPM_CLASSIFY_OPTIONS* pOriginalOptions);

#if(NTDDI_VERSION >= NTDDI_WIN7)

_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
inline VOID KrnlHlprIPsecDoSPOptionsPurgeLocalCopy(_Inout_ IPSEC_DOSP_OPTIONS* pOptions);

_At_(*ppOptions, _Pre_ _Notnull_)
_At_(*ppOptions, _Post_ _Null_ __drv_freesMem(Pool))
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Success_(*ppOptions == 0)
inline VOID KrnlHlprIPsecDoSPOptionsDestroyLocalCopy(_Inout_ IPSEC_DOSP_OPTIONS** ppOptions);

_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Check_return_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprIPsecDoSPOptionsPopulateLocalCopy(_In_ const IPSEC_DOSP_OPTIONS* pOriginalOptions,
                                                   _Inout_ IPSEC_DOSP_OPTIONS* pOptions);

__drv_allocatesMem(Pool)
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Must_inspect_result_
_Success_(return != 0)
IPSEC_DOSP_OPTIONS* KrnlHlprIPsecDoSPOptionsCreateLocalCopy(_In_ const IPSEC_DOSP_OPTIONS* pOriginalOptions);

#endif // (NTDDI_VERSION >= NTDDI_WIN7)

_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
inline VOID KrnlHlprFwpTokenInformationPurgeLocalCopy(_Inout_ FWP_TOKEN_INFORMATION* pTokenInfo);

_At_(*ppTokenInfo, _Pre_ _Notnull_)
_At_(*ppTokenInfo, _Post_ _Null_ __drv_freesMem(Pool))
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Success_(*ppTokenInfo == 0)
inline VOID KrnlHlprFwpTokenInformationDestroyLocalCopy(_Inout_ FWP_TOKEN_INFORMATION** ppTokenInfo);

_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Check_return_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprFwpTokenInformationPopulateLocalCopy(_In_ const FWP_TOKEN_INFORMATION* pOriginalTokenInfo,
                                                      _Inout_ FWP_TOKEN_INFORMATION* pTokenInfo);

__drv_allocatesMem(Pool)
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Must_inspect_result_
_Success_(return != 0)
FWP_TOKEN_INFORMATION* KrnlHlprFwpTokenInformationCreateLocalCopy(_In_ const FWP_TOKEN_INFORMATION* pOriginalTokenInfo);

_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Success_(return != 0)
PSTR KrnlHlprFwpValueGetStringForFwpsIncomingValue(_In_ UINT32 layerID,
                                                   _In_ UINT32 fieldID,
                                                   _Out_opt_ BOOLEAN*  pFormat = 0);

_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Must_inspect_result_
_Success_(return != 0)
FWP_VALUE* KrnlHlprFwpValueGetFromFwpsIncomingValues(_In_ const FWPS_INCOMING_VALUES* pClassifyValues,
                                                     _In_ const GUID* pConditionKey);

_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
inline VOID KrnlHlprFwpValuePurgeLocalCopy(_Inout_ FWP_VALUE* pValue);

_At_(*ppValue, _Pre_ _Notnull_)
_At_(*ppValue, _Post_ _Null_ __drv_freesMem(Pool))
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Success_(*ppValue == 0)
inline VOID KrnlHlprFwpValueDestroyLocalCopy(_Inout_ FWP_VALUE** ppValue);

_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Check_return_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprFwpValuePopulateLocalCopy(_In_ const FWP_VALUE* pOriginalValue,
                                           _Inout_ FWP_VALUE* pValue);

__drv_allocatesMem(Pool)
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Must_inspect_result_
_Success_(return != 0)
FWP_VALUE* KrnlHlprFwpValueCreateLocalCopy(_In_ const FWP_VALUE* pOriginalValue);

_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
inline VOID KrnlHlprFwpV4AddrAndMaskPurgeLocalCopy(_Inout_ FWP_V4_ADDR_AND_MASK* pAddrMask);

_At_(*ppAddrMask, _Pre_ _Notnull_)
_At_(*ppAddrMask, _Post_ _Null_ __drv_freesMem(Pool))
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Success_(*ppAddrMask == 0)
inline VOID KrnlHlprFwpV4AddrAndMaskDestroyLocalCopy(_Inout_ FWP_V4_ADDR_AND_MASK** ppAddrMask);

_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Check_return_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprFwpV4AddrAndMaskPopulateLocalCopy(_In_ const FWP_V4_ADDR_AND_MASK* pOriginalAddrMask,
                                                   _Inout_ FWP_V4_ADDR_AND_MASK* pAddrMask);

__drv_allocatesMem(Pool)
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Must_inspect_result_
_Success_(return != 0)
FWP_V4_ADDR_AND_MASK* KrnlHlprFwpV4AddrAndMaskCreateLocalCopy(_In_ const FWP_V4_ADDR_AND_MASK* pOriginalAddrMask);

_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
inline VOID KrnlHlprFwpV6AddrAndMaskPurgeLocalCopy(_Inout_ FWP_V6_ADDR_AND_MASK* pAddrMask);

_At_(*ppAddrMask, _Pre_ _Notnull_)
_At_(*ppAddrMask, _Post_ _Null_ __drv_freesMem(Pool))
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Success_(*ppAddrMask == 0)
inline VOID KrnlHlprFwpV6AddrAndMaskDestroyLocalCopy(_Inout_ FWP_V6_ADDR_AND_MASK** ppAddrMask);

__drv_allocatesMem(Pool)
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Check_return_
_Success_(return == STATUS_SUCCESS)
_Success_(return != 0)
NTSTATUS KrnlHlprFwpV6AddrAndMaskCreateLocalCopy(_In_ const FWP_V6_ADDR_AND_MASK* pOriginalAddrMask,
                                                 _Inout_ FWP_V6_ADDR_AND_MASK* pAddrMask);

__drv_allocatesMem(Pool)
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Must_inspect_result_
_Success_(return != 0)
FWP_V6_ADDR_AND_MASK* KrnlHlprFwpV6AddrAndMaskCreateLocalCopy(_In_ const FWP_V6_ADDR_AND_MASK* pOriginalAddrMask);

_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
inline VOID KrnlHlprFwpRangePurgeLocalCopy(_Inout_ FWP_RANGE* pRange);

_At_(*ppRange, _Pre_ _Notnull_)
_At_(*ppRange, _Post_ _Null_ __drv_freesMem(Pool))
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Success_(*ppRange == 0)
inline VOID KrnlHlprFwpRangeDestroyLocalCopy(_Inout_ FWP_RANGE** ppRange);

_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Check_return_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprFwpRangePopulateLocalCopy(_In_ const FWP_RANGE* pOriginalRange,
                                           _Inout_ FWP_RANGE* pRange);

__drv_allocatesMem(Pool)
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Must_inspect_result_
_Success_(return != 0)
FWP_RANGE* KrnlHlprFwpRangeCreateLocalCopy(_In_ const FWP_RANGE* pOriginalRange);

_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
inline VOID KrnlHlprFwpConditionValuePurgeLocalCopy(_Inout_ FWP_CONDITION_VALUE* pValue);

_At_(*ppValue, _Pre_ _Notnull_)
_At_(*ppValue, _Post_ _Null_ __drv_freesMem(Pool))
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Success_(*ppValue == 0)
inline VOID KrnlHlprFwpConditionValueDestroyLocalCopy(_Inout_ FWP_CONDITION_VALUE** ppValue);

_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Check_return_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprFwpConditionValuePopulateLocalCopy(_In_ const FWP_CONDITION_VALUE* pOriginalValue,
                                                    _Inout_ FWP_CONDITION_VALUE* pValue);

__drv_allocatesMem(Pool)
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Must_inspect_result_
_Success_(return != 0)
FWP_CONDITION_VALUE* KrnlHlprFwpConditionValueCreateLocalCopy(_In_ const FWP_CONDITION_VALUE* pOriginalValue);

_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Check_return_
BOOLEAN KrnlHlprFwpsIncomingValueConditionFlagsAreSet(_In_ const FWPS_INCOMING_VALUES* pClassifyValues,
                                                      _In_ UINT32 flags);

_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
inline VOID KrnlHlprFwpsIncomingValuesPurgeLocalCopy(_Inout_ FWPS_INCOMING_VALUES* pOriginalValues);

_At_(*ppValues, _Pre_ _Notnull_)
_At_(*ppValues, _Post_ _Null_ __drv_freesMem(Pool))
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Success_(*ppValues == 0)
inline VOID KrnlHlprFwpsIncomingValuesDestroyLocalCopy(_Inout_ FWPS_INCOMING_VALUES** ppValues);

_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Check_return_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprFwpsIncomingValuesPopulateLocalCopy(_In_ const FWPS_INCOMING_VALUES* pOriginalValues,
                                                     _Inout_ FWPS_INCOMING_VALUES* pValues);

__drv_allocatesMem(Pool)
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Must_inspect_result_
_Success_(return != 0)
FWPS_INCOMING_VALUES* KrnlHlprFwpsIncomingValuesCreateLocalCopy(_In_ const FWPS_INCOMING_VALUES* pOriginalValues);

_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
inline VOID KrnlHlprFwpsIncomingMetadataValuesPurgeLocalCopy(_Inout_ FWPS_INCOMING_METADATA_VALUES* pMetadata);

_At_(*ppMetadata, _Pre_ _Notnull_)
_At_(*ppMetadata, _Post_ _Null_ __drv_freesMem(Pool))
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Success_(*ppMetadata == 0)
inline VOID KrnlHlprFwpsIncomingMetadataValuesDestroyLocalCopy(_Inout_ FWPS_INCOMING_METADATA_VALUES** ppMetadata);

_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Check_return_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprFwpsIncomingMetadataValuesPopulateLocalCopy(_In_ const FWPS_INCOMING_METADATA_VALUES* pOriginalMetadata,
                                                             _Inout_ FWPS_INCOMING_METADATA_VALUES* pMetadata);


__drv_allocatesMem(Pool)
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Must_inspect_result_
_Success_(return != 0)
FWPS_INCOMING_METADATA_VALUES* KrnlHlprFwpsIncomingMetadataValuesCreateLocalCopy(_In_ const FWPS_INCOMING_METADATA_VALUES* pOriginalMetadata);

_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
inline VOID KrnlHlprFwpsClassifyOutPurgeLocalCopy(_Inout_ FWPS_CLASSIFY_OUT* pOriginalClassifyOut);

_At_(*ppClassifyOut, _Pre_ _Notnull_)
_At_(*ppClassifyOut, _Post_ _Null_ __drv_freesMem(Pool))
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Success_(*ppClassifyOut == 0)
VOID KrnlHlprFwpsClassifyOutDestroyLocalCopy(_Inout_ FWPS_CLASSIFY_OUT** ppClassifyOut);

_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
inline VOID KrnlHlprFwpsIncomingValuesPopulateLocalCopy(_In_ const FWPS_CLASSIFY_OUT* pOriginalClassifyOut,
                                                        _Inout_ FWPS_CLASSIFY_OUT* pClassifyOut);

__drv_allocatesMem(Pool)
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Must_inspect_result_
_Success_(return != 0)
FWPS_CLASSIFY_OUT* KrnlHlprFwpsClassifyOutCreateLocalCopy(_In_ const FWPS_CLASSIFY_OUT* pOriginalClassifyOut);


_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
inline VOID KrnlHlprFwpsStreamDataPurgeLocalCopy(_Inout_ FWPS_STREAM_DATA* pStreamData);

_At_(*ppStreamData, _Pre_ _Notnull_)
_At_(*ppStreamData, _Post_ _Null_ __drv_freesMem(Pool))
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Success_(*ppStreamData == 0)
inline VOID KrnlHlprFwpsStreamDataDestroyLocalCopy(_Inout_ FWPS_STREAM_DATA** ppStreamData);

_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Check_return_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprFwpsStreamDataPopulateLocalCopy(_In_ const FWPS_STREAM_DATA* pOriginalStreamData,
                                                 _Inout_ FWPS_STREAM_DATA* pStreamData);

__drv_allocatesMem(Pool)
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Must_inspect_result_
_Success_(return != 0)
FWPS_STREAM_DATA* KrnlHlprFwpsStreamDataCreateLocalCopy(_In_ const FWPS_STREAM_DATA* pOriginalStreamData);

_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
inline VOID KrnlHlprFwpsStreamCalloutIOPacketPurgeLocalCopy(_Inout_ FWPS_STREAM_CALLOUT_IO_PACKET* pIOPacket);

_At_(*ppIOPacket, _Pre_ _Notnull_)
_At_(*ppIOPacket, _Post_ _Null_ __drv_freesMem(Pool))
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Success_(*ppIOPacket == 0)
inline VOID KrnlHlprFwpsStreamCalloutIOPacketDestroyLocalCopy(_Inout_ FWPS_STREAM_CALLOUT_IO_PACKET** ppIOPacket);

_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Check_return_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprFwpsStreamCalloutIOPacketPopulateLocalCopy(_In_ const FWPS_STREAM_CALLOUT_IO_PACKET* pOriginalIOPacket,
                                                            _Inout_ FWPS_STREAM_CALLOUT_IO_PACKET* pIOPacket);

__drv_allocatesMem(Pool)
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Must_inspect_result_
_Success_(return != 0)
FWPS_STREAM_CALLOUT_IO_PACKET* KrnlHlprFwpsStreamCalloutIOPacketCreateLocalCopy(_In_ const FWPS_STREAM_CALLOUT_IO_PACKET* pOriginalIOPacket);

_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
inline VOID KrnlHlprFwpmProviderContextPurgeLocalCopy(_Inout_ FWPM_PROVIDER_CONTEXT* pContext);

_At_(*ppContext, _Pre_ _Notnull_)
_At_(*ppContext, _Post_ _Null_ __drv_freesMem(Pool))
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Success_(*ppContext == 0)
inline VOID KrnlHlprFwpmProviderContextDestroyLocalCopy(_Inout_ FWPM_PROVIDER_CONTEXT** ppContext);

__drv_allocatesMem(Pool)
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Check_return_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprFwpmProviderContextCreateLocalCopy(_In_ const FWPM_PROVIDER_CONTEXT* pOriginalContext,
                                                    _Inout_ FWPM_PROVIDER_CONTEXT* pContext);

__drv_allocatesMem(Pool)
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Must_inspect_result_
_Success_(return != 0)
FWPM_PROVIDER_CONTEXT* KrnlHlprFwpmProviderContextCreateLocalCopy(_In_ const FWPM_PROVIDER_CONTEXT* pOriginalContext);

_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
inline VOID KrnlHlprFwpsFilterConditionPurgeLocalCopy(_Inout_ FWPS_FILTER_CONDITION* pCondition);

_At_(*ppConditions, _Pre_ _Notnull_)
_At_(*ppConditions, _Post_ _Null_ __drv_freesMem(Pool))
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Success_(*ppConditions == 0)
inline VOID KrnlHlprFwpsFilterConditionDestroyLocalCopy(_Inout_ FWPS_FILTER_CONDITION** ppConditions,
                                                        _In_ UINT32 numConditions = 1);

_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Check_return_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprFwpsFilterConditionPopulateLocalCopy(_In_ const FWPS_FILTER_CONDITION* pOriginalCondition,
                                                      _Inout_ FWPS_FILTER_CONDITION* pCondition);

__drv_allocatesMem(Pool)
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Must_inspect_result_
_Success_(return != 0)
FWPS_FILTER_CONDITION* KrnlHlprFwpsFilterConditionCreateLocalCopy(_In_reads_(numConditions) const FWPS_FILTER_CONDITION* pOriginalConditions,
                                                                  _In_ UINT32 numConditions = 1);

_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
inline VOID KrnlHlprFwpsFilterPurgeLocalCopy(_Inout_ FWPS_FILTER* pFilter);

_At_(*ppFilter, _Pre_ _Notnull_)
_At_(*ppFilter, _Post_ _Null_ __drv_freesMem(Pool))
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Success_(*ppFilter == 0)
inline VOID KrnlHlprFwpsFilterDestroyLocalCopy(_Inout_ FWPS_FILTER** ppFilter);

_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Check_return_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprFwpsFilterPopulateLocalCopy(_In_ const FWPS_FILTER* pOriginalFilter,
                                             _Inout_ FWPS_FILTER* pFilter);

__drv_allocatesMem(Pool)
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Must_inspect_result_
_Success_(return != 0)
FWPS_FILTER* KrnlHlprFwpsFilterCreateLocalCopy(_In_ const FWPS_FILTER* pOriginalFilter);

_IRQL_requires_(PASSIVE_LEVEL)
_IRQL_requires_same_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprFwpmSessionReleaseHandle(_Inout_ HANDLE* pEngineHandle);

_At_(*ppEngineHandle, _Pre_ _Notnull_)
_At_(*ppEngineHandle, _Post_ _Null_)
_IRQL_requires_(PASSIVE_LEVEL)
_IRQL_requires_same_
_Success_(*ppEngineHandle == 0)
VOID KrnlHlprFwpmSessionDestroyEngineHandle(_Inout_ HANDLE** ppEngineHandle);

_IRQL_requires_(PASSIVE_LEVEL)
_IRQL_requires_same_
_Check_return_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprFwpmSessionAcquireHandle(_Inout_ HANDLE* pEngineHandle,
                                          _In_ const GUID* pSessionKey = &WFPSAMPLER_SESSION_KM);

_When_(return != STATUS_SUCCESS, _At_(*ppEngineHandle, _Post_ _Null_))
_When_(return == STATUS_SUCCESS, _At_(*ppEngineHandle, _Post_ _Notnull_))
_IRQL_requires_(PASSIVE_LEVEL)
_IRQL_requires_same_
_Check_return_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprFwpmSessionCreateEngineHandle(_Outptr_ HANDLE** ppEngineHandle,
                                               _In_ const GUID* pSessionKey = &WFPSAMPLER_SESSION_KM);

_IRQL_requires_(PASSIVE_LEVEL)
_IRQL_requires_same_
_Check_return_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprFwpsInjectionReleaseHandle(_In_ HANDLE* pInjectionHandle);

_IRQL_requires_(PASSIVE_LEVEL)
_IRQL_requires_same_
_Check_return_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprFwpsInjectionAcquireHandle(_Inout_ HANDLE* pInjectionHandle,
                                            _In_ ADDRESS_FAMILY addressFamily = AF_UNSPEC,
                                            _In_ UINT32 injectionType = 0);

_IRQL_requires_(PASSIVE_LEVEL)
_IRQL_requires_same_
VOID KrnlHlprFwpsRedirectHandleDestroy(_In_ HANDLE* pRedirectionHandle);

_IRQL_requires_(PASSIVE_LEVEL)
_IRQL_requires_same_
_Check_return_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprFwpsRedirectHandleCreate(_Inout_ HANDLE* pRedirectionHandle = 0);

_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
FWP_DIRECTION KrnlHlprFwpsLayerGetDirection(_In_ UINT32 layerID);

_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
BOOLEAN KrnlHlprFwpsLayerIsIPv4(_In_ UINT32 layerID);

_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
BOOLEAN KrnlHlprFwpsLayerIsIPv6(_In_ UINT32 layerID);

_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
BOOLEAN KrnlHlprFwpsLayerIsDiscard(_In_ UINT32 layerID);

_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Success_(return != 0)
PSTR KrnlHlprFwpsLayerIDToString(_In_ UINT32 layerID);

#endif /// HELPERFUNCTIONS_FWP_OBJECTS_H
