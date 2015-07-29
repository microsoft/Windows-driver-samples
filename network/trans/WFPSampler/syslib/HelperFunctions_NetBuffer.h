////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2014 Microsoft Corporation.  All Rights Reserved.
//
//   Module Name:
//      HelperFunctions_NetBuffer.h
//
//   Abstract:
//      This module contains prototypes for kernel helper functions that assist with NET_BUFFER and
//         NET_BUFFER_LIST.
//
//   Author:
//      Dusty Harper      (DHarper)
//
//   Revision History:
//
//      [ Month ][Day] [Year] - [Revision]-[ Comments ]
//      May       01,   2010  -     1.0   -  Creation
//      December  13,   2013  -     1.1   -  Add KrnlHlprNBLCreateFromBuffer,
//                                              KrnlHlprNBLCopyToBuffer,
//                                              KrnlHlprNBLDestroyNew,
//                                              KrnlHlprNBLCreateNew
//
////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef HELPERFUNCTIONS_NET_BUFFER_H
#define HELPERFUNCTIONS_NET_BUFFER_H

_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Success_(return != 0)
NET_BUFFER_LIST* KrnlHlprNBLCreateFromBuffer(_In_ NDIS_HANDLE nblPoolHandle,
                                             _In_reads_(bufferSize) BYTE* pBuffer,
                                             _In_ UINT32 bufferSize,
                                             _Outptr_opt_result_maybenull_ PMDL* ppMDL);


_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Success_(return != 0)
BYTE* KrnlHlprNBLCopyToBuffer(_In_opt_ NET_BUFFER_LIST* pTemplateNBL,
                              _Out_ UINT32* pSize,
                              _In_ UINT32 additionalSpace = 0);

_Success_(*ppNetBufferList == 0 && *ppMDL == 0 && *ppAllocatedBuffer == 0)
VOID KrnlHlprNBLDestroyNew(_Inout_opt_ NET_BUFFER_LIST** ppNetBufferList,
                           _Inout_opt_ PMDL* ppMDL,
                           _Inout_opt_ BYTE** ppAllocatedBuffer);

_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Success_(return != 0)
NET_BUFFER_LIST* KrnlHlprNBLCreateNew(_In_ NDIS_HANDLE nblPoolHandle,
                                      _In_opt_ NET_BUFFER_LIST* pTemplateNBL,
                                      _Outptr_opt_result_buffer_maybenull_(*pSize) BYTE** ppAllocatedBuffer,
                                      _Out_ UINT32* pSize,
                                      _Outptr_opt_result_maybenull_ PMDL* ppMDL,
                                      _In_ UINT32 additionalSpace = 0,
                                      _In_ BOOLEAN isOutbound = FALSE);

_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
UINT32 KrnlHlprNBLGetRequiredRefCount(_In_ const NET_BUFFER_LIST* pNBL,
                                      _In_ BOOLEAN isChained = FALSE);

#endif /// HELPERFUNCTIONS_NET_BUFFER_H
