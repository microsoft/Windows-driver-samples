/*++

Copyright (c) Microsoft Corporation. All rights reserved

Abstract:

   This header files declares common data types and function prototypes used
   throughout the Datagram-Data transparent proxy sample.

Environment:

    Kernel mode

--*/

#ifndef _DD_PROXY_H_
#define _DD_PROXY_H_

typedef enum DD_PROXY_FLOW_TYPE_
{
   DD_PROXY_FLOW_ORIGINAL,
   DD_PROXY_FLOW_PROXY
} DD_PROXY_FLOW_TYPE;

//
// DD_PROXY_FLOW_CONTEXT is the object type we used to stored information
// specific flow. This callout driver maintains two kind of flow contexts --
// the original flow and the flow being proxied to.
//

typedef struct DD_PROXY_FLOW_CONTEXT_
{
   LIST_ENTRY listEntry;

   BOOLEAN deleted;

   DD_PROXY_FLOW_TYPE flowType;
   ADDRESS_FAMILY addressFamily;

   #pragma warning(push)
   #pragma warning(disable: 4201) //NAMELESS_STRUCT_UNION
   union
   {
      FWP_BYTE_ARRAY16 localAddr;
      UINT32 ipv4LocalAddr;
   };
   #pragma warning(pop)


   UINT8 protocol;

   UINT64 flowId;
   UINT16 layerId;
   UINT32 calloutId;

   UINT32 ipv4NetworkOrderStorage;

   //
   // For DD_PROXY_FLOW_ORIGINAL type, toRemote* is the new address/port
   // we are proxing to. For DD_PROXY_FLOW_PROXY type, it is the address/
   // port that we will need to revert to.
   //
   UINT8* toRemoteAddr;
   UINT16 toRemotePort;

   LONG refCount;
} DD_PROXY_FLOW_CONTEXT;

//
// DD_PROXY_PENDED_PACKET is the object type we used to store all information
// needed for out-of-band packet modification and re-injection. This type
// also points back to the flow context the packet belongs to.

typedef struct DD_PROXY_PENDED_PACKET_
{
   LIST_ENTRY listEntry;

   DD_PROXY_FLOW_CONTEXT* belongingFlow;
   FWP_DIRECTION  direction;

   //
   // Common fields for inbound and outbound traffic.
   //
   NET_BUFFER_LIST* netBufferList;
   COMPARTMENT_ID compartmentId;

   //
   // Data fields for outbound packet re-injection.
   //
   UINT64 endpointHandle;

   #pragma warning(push)
   #pragma warning(disable: 4201) //NAMELESS_STRUCT_UNION
   union
   {
      FWP_BYTE_ARRAY16 remoteAddr;
      UINT32 ipv4RemoteAddr;
   };
   #pragma warning(pop)

   SCOPE_ID remoteScopeId;
   WSACMSGHDR* controlData;
   ULONG controlDataLength;

   //
   // Data fields for inbound packet re-injection.
   //
   ULONG nblOffset;
   UINT32 ipHeaderSize;
   UINT32 transportHeaderSize;
   IF_INDEX interfaceIndex;
   IF_INDEX subInterfaceIndex;
} DD_PROXY_PENDED_PACKET;

//
// Pooltags used by this callout driver.
//
#define DD_PROXY_FLOW_CONTEXT_POOL_TAG 'olfD'
#define DD_PROXY_PENDED_PACKET_POOL_TAG 'kppD'
#define DD_PROXY_CONTROL_DATA_POOL_TAG 'dcdD'

//
// Shared global data.
//
extern UINT16 configInspectDestPort;
extern UINT8* configInspectDestAddrV4;
extern UINT8* configInspectDestAddrV6;

extern UINT16 configNewDestPort;
extern UINT8* configNewDestAddrV4;
extern UINT8* configNewDestAddrV6;

extern HANDLE gInjectionHandle;

extern LIST_ENTRY gFlowList;
extern KSPIN_LOCK gFlowListLock;

extern LIST_ENTRY gPacketQueue;
extern KSPIN_LOCK gPacketQueueLock;
extern KEVENT gPacketQueueEvent;

extern UINT32 gCalloutIdV4;
extern UINT32 gCalloutIdV6;

extern BOOLEAN gDriverUnloading;

//
// Utility functions
//

__inline void
DDProxyReferenceFlowContext(
   _Inout_ DD_PROXY_FLOW_CONTEXT* flowContext
   )
{
   NT_ASSERT(flowContext->refCount > 0);
   InterlockedIncrement(&flowContext->refCount);
}

_IRQL_requires_max_(DISPATCH_LEVEL)
__inline
void
DDProxyDereferenceFlowContext(
   _Inout_ DD_PROXY_FLOW_CONTEXT* flowContext
   )
{
   NT_ASSERT(flowContext->refCount > 0);
   InterlockedDecrement(&flowContext->refCount);
   if (flowContext->refCount == 0)
   {
      ExFreePoolWithTag(flowContext, DD_PROXY_FLOW_CONTEXT_POOL_TAG);
   }
}

//
// Shared function prototypes
//

#if(NTDDI_VERSION >= NTDDI_WIN7)

void
DDProxyFlowEstablishedClassify(
   _In_ const FWPS_INCOMING_VALUES* inFixedValues,
   _In_ const FWPS_INCOMING_METADATA_VALUES* inMetaValues,
   _Inout_opt_ void* layerData,
   _In_opt_ const void* classifyContext,
   _In_ const FWPS_FILTER* filter,
   _In_ UINT64 flowContext,
   _Inout_ FWPS_CLASSIFY_OUT* classifyOut
   );

void
DDProxyClassify(
   _In_ const FWPS_INCOMING_VALUES* inFixedValues,
   _In_ const FWPS_INCOMING_METADATA_VALUES* inMetaValues,
   _Inout_opt_ void* layerData,
   _In_opt_ const void* classifyContext,
   _In_ const FWPS_FILTER* filter,
   _In_ UINT64 flowContext,
   _Inout_ FWPS_CLASSIFY_OUT* classifyOut
   );

#else

void
DDProxyFlowEstablishedClassify(
   _In_ const FWPS_INCOMING_VALUES* inFixedValues,
   _In_ const FWPS_INCOMING_METADATA_VALUES* inMetaValues,
   _Inout_opt_ void* layerData,
   _In_ const FWPS_FILTER* filter,
   _In_ UINT64 flowContext,
   _Inout_ FWPS_CLASSIFY_OUT* classifyOut
   );

void
DDProxyClassify(
   _In_ const FWPS_INCOMING_VALUES* inFixedValues,
   _In_ const FWPS_INCOMING_METADATA_VALUES* inMetaValues,
   _Inout_opt_ void* layerData,
   _In_ const FWPS_FILTER* filter,
   _In_ UINT64 flowContext,
   _Inout_ FWPS_CLASSIFY_OUT* classifyOut
   );

#endif /// (NTDDI_VERSION >= NTDDI_WIN7)

void
DDProxyFlowDelete(
   _In_ UINT16 layerId,
   _In_ UINT32 calloutId,
   _In_ UINT64 flowContext
   );

NTSTATUS
DDProxyFlowEstablishedNotify(
   _In_ FWPS_CALLOUT_NOTIFY_TYPE notifyType,
   _In_ const GUID* filterKey,
   _Inout_ const FWPS_FILTER* filter
   );


NTSTATUS
DDProxyNotify(
   _In_ FWPS_CALLOUT_NOTIFY_TYPE notifyType,
   _In_ const GUID* filterKey,
   _Inout_ const FWPS_FILTER* filter
   );

KSTART_ROUTINE DDProxyWorker;

#endif // _DD_PROXY_H_
