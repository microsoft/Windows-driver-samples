////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2014 Microsoft Corporation.  All Rights Reserved.
//
//   Module Name:
//      HelperFunctions_FlowContext.h
//
//   Abstract:
//      This module contains prototypes for kernel helper functions that assist with FLOW_CONTEXT.
//
//   Author:
//      Dusty Harper      (DHarper)
//
//   Revision History:
//
//      [ Month ][Day] [Year] - [Revision]-[ Comments ]
//      December  13,   2013  -     1.1   -  Creation
//
////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef HELPERFUNCTIONS_FLOW_CONTEXT_H
#define HELPERFUNCTIONS_FLOW_CONTEXT_H

typedef struct SERIALIZATION_LIST_
{
   KSPIN_LOCK  spinLock;
   WDFWAITLOCK waitLock;
   LIST_ENTRY  listHead;
   INT64       numEntries;
}SERIALIZATION_LIST, *PSERIALIZATION_LIST;

typedef enum CONTEXT_TYPE_
{
   CONTEXT_TYPE_DEFAULT = 0,
   CONTEXT_TYPE_STREAM,

#if(NTDDI_VERSION >= NTDDI_WIN7)

   CONTEXT_TYPE_ALE_ENDPOINT_CLOSURE,

#endif /// (NTDDI_VERSION >= NTDDI_WIN7)

   CONTEXT_TYPE_MAX
}CONTEXT_TYPE;

typedef struct STREAM_CONTEXT_
{
   UINT64             filterID;
   UINT32             processorID;
   BYTE               pReserved[1];
   SERIALIZATION_LIST serializationList;
}STREAM_CONTEXT, *PSTREAM_CONTEXT;

#if(NTDDI_VERSION >= NTDDI_WIN7)

typedef struct ALE_ENDPOINT_CLOSURE_CONTEXT_
{
   UINT64      filterID;
   KSPIN_LOCK  spinLock;
   BYTE        pReserved[3];
   VOID*       pPendData;

}ALE_ENDPOINT_CLOSURE_CONTEXT, *PALE_ENDPOINT_CLOSURE_CONTEXT;

#endif /// (NTDDI_VERSION >= NTDDI_WIN7)

typedef struct FLOW_CONTEXT_
{
   UINT64                        flowID;
   UINT32                        calloutID;
   UINT16                        layerID;
   UINT16                        contextType;
   union
   {
      VOID*                      pContext;
      STREAM_CONTEXT*            pStreamContext;

#if(NTDDI_VERSION >= NTDDI_WIN7)

      ALE_ENDPOINT_CLOSURE_CONTEXT* pALEEndpointClosureContext;


   };
   union
   {
      UINT32                     aecCalloutID;
      UINT32                     injectionCalloutID;
   };
   union
   {
      UINT16                     aecLayerID;
      UINT16                     injectionLayerID;
   };
   BYTE                          pReserved[2];

#else

   };

#endif /// (NTDDI_VERSION >= NTDDI_WIN7)

}FLOW_CONTEXT, *PFLOW_CONTEXT;

_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Success_(return == STATUS_SUCCESS)
inline NTSTATUS KrnlHlprFlowContextPurge(_Inout_ FLOW_CONTEXT* pFlowContext);

_At_(*ppFlowContext, _Pre_ _Notnull_)
_At_(*ppFlowContext, _Post_ _Null_ __drv_freesMem(Pool))
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Success_(return == STATUS_SUCCESS && *ppFlowContext == 0)
inline NTSTATUS KrnlHlprFlowContextDestroy(_Inout_ FLOW_CONTEXT** ppFlowContext);

_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Check_return_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprFlowContextPopulate(_Inout_ FLOW_CONTEXT* pFlowContext,
                                     _In_ UINT64 flowHandle,
                                     _In_ UINT16 layerID,
                                     _In_ UINT32 calloutID,
                                     _In_ UINT8 contextType = CONTEXT_TYPE_DEFAULT,
                                     _In_opt_ const VOID* pProviderContext = 0);

_At_(*ppFlowContext, _Pre_ _Null_)
_When_(return != STATUS_SUCCESS, _At_(*ppFlowContext, _Post_ _Null_))
_When_(return == STATUS_SUCCESS, _At_(*ppFlowContext, _Post_ _Notnull_ __drv_allocatesMem(Pool)))
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Check_return_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprFlowContextCreate(_Outptr_ FLOW_CONTEXT** ppFlowContext,
                                   _In_ UINT64 flowHandle,
                                   _In_ UINT16 layerID,
                                   _In_ UINT32 calloutID,
                                   _In_ UINT8 contextType = CONTEXT_TYPE_DEFAULT,
                                   _In_opt_ const VOID* pProviderContext = 0);

#endif /// HELPERFUNCTIONS_FLOW_CONTEXT_H
