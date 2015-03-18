/*++

Copyright (c) Microsoft Corporation. All rights reserved

Abstract:

   This header files declares common data types and function prototypes used
   throughout the Transport Inspect sample.

Environment:

    Kernel mode

--*/

#ifndef _TL_INSPECT_H_
#define _TL_INSPECT_H_

typedef enum TL_INSPECT_PACKET_TYPE_
{
   TL_INSPECT_CONNECT_PACKET,
   TL_INSPECT_DATA_PACKET,
   TL_INSPECT_REAUTH_PACKET
} TL_INSPECT_PACKET_TYPE;

//
// TL_INSPECT_PENDED_PACKET is the object type we used to store all information
// needed for out-of-band packet modification and re-injection. This type
// also points back to the flow context the packet belongs to.

#pragma warning(push)
#pragma warning(disable: 4201) //NAMELESS_STRUCT_UNION

typedef struct TL_INSPECT_PENDED_PACKET_
{
   LIST_ENTRY listEntry;

   ADDRESS_FAMILY addressFamily;
   TL_INSPECT_PACKET_TYPE type;
   FWP_DIRECTION  direction;
   
   UINT32 authConnectDecision;
   HANDLE completionContext;

   //
   // Common fields for inbound and outbound traffic.
   //
   UINT8 protocol;
   NET_BUFFER_LIST* netBufferList;
   COMPARTMENT_ID compartmentId;
   union
   {
      FWP_BYTE_ARRAY16 localAddr;
      UINT32 ipv4LocalAddr;
   };
   union
   {
      UINT16 localPort;
      UINT16 icmpType;
   };
   union
   {
      UINT16 remotePort;
      UINT16 icmpCode;
   };

   //
   // Data fields for outbound packet re-injection.
   //
   UINT64 endpointHandle;
   union
   {
      FWP_BYTE_ARRAY16 remoteAddr;
      UINT32 ipv4RemoteAddr;
   };

   SCOPE_ID remoteScopeId;
   WSACMSGHDR* controlData;
   ULONG controlDataLength;

   //
   // Data fields for inbound packet re-injection.
   //
   BOOLEAN ipSecProtected;
   ULONG nblOffset;
   UINT32 ipHeaderSize;
   UINT32 transportHeaderSize;
   IF_INDEX interfaceIndex;
   IF_INDEX subInterfaceIndex;
} TL_INSPECT_PENDED_PACKET;

#pragma warning(pop)

//
// Pooltags used by this callout driver.
//
#define TL_INSPECT_CONNECTION_POOL_TAG 'olfD'
#define TL_INSPECT_PENDED_PACKET_POOL_TAG 'kppD'
#define TL_INSPECT_CONTROL_DATA_POOL_TAG 'dcdD'

//
// Shared global data.
//
extern BOOLEAN configPermitTraffic;

extern HANDLE gInjectionHandle;

extern LIST_ENTRY gConnList;
extern KSPIN_LOCK gConnListLock;

extern LIST_ENTRY gPacketQueue;
extern KSPIN_LOCK gPacketQueueLock;

extern KEVENT gWorkerEvent;

extern BOOLEAN gDriverUnloading;

//
// Shared function prototypes
//

#if(NTDDI_VERSION >= NTDDI_WIN7)

void
TLInspectALEConnectClassify(
   _In_ const FWPS_INCOMING_VALUES* inFixedValues,
   _In_ const FWPS_INCOMING_METADATA_VALUES* inMetaValues,
   _Inout_opt_ void* layerData,
   _In_opt_ const void* classifyContext,
   _In_ const FWPS_FILTER* filter,
   _In_ UINT64 flowContext,
   _Inout_ FWPS_CLASSIFY_OUT* classifyOut
   );

void
TLInspectALERecvAcceptClassify(
   _In_ const FWPS_INCOMING_VALUES* inFixedValues,
   _In_ const FWPS_INCOMING_METADATA_VALUES* inMetaValues,
   _Inout_opt_ void* layerData,
   _In_opt_ const void* classifyContext,
   _In_ const FWPS_FILTER* filter,
   _In_ UINT64 flowContext,
   _Inout_ FWPS_CLASSIFY_OUT* classifyOut
   );

void
TLInspectTransportClassify(
   _In_ const FWPS_INCOMING_VALUES* inFixedValues,
   _In_ const FWPS_INCOMING_METADATA_VALUES* inMetaValues,
   _Inout_opt_ void* layerData,
   _In_opt_ const void* classifyContext,
   _In_ const FWPS_FILTER* filter,
   _In_ UINT64 flowContext,
   _Inout_ FWPS_CLASSIFY_OUT* classifyOut
   );

#else /// (NTDDI_VERSION >= NTDDI_WIN7)

void
TLInspectALEConnectClassify(
   _In_ const FWPS_INCOMING_VALUES* inFixedValues,
   _In_ const FWPS_INCOMING_METADATA_VALUES* inMetaValues,
   _Inout_opt_ void* layerData,
   _In_ const FWPS_FILTER* filter,
   _In_ UINT64 flowContext,
   _Inout_ FWPS_CLASSIFY_OUT* classifyOut
   );

void
TLInspectALERecvAcceptClassify(
   _In_ const FWPS_INCOMING_VALUES* inFixedValues,
   _In_ const FWPS_INCOMING_METADATA_VALUES* inMetaValues,
   _Inout_opt_ void* layerData,
   _In_ const FWPS_FILTER* filter,
   _In_ UINT64 flowContext,
   _Inout_ FWPS_CLASSIFY_OUT* classifyOut
   );

void
TLInspectTransportClassify(
   _In_ const FWPS_INCOMING_VALUES* inFixedValues,
   _In_ const FWPS_INCOMING_METADATA_VALUES* inMetaValues,
   _Inout_opt_ void* layerData,
   _In_ const FWPS_FILTER* filter,
   _In_ UINT64 flowContext,
   _Inout_ FWPS_CLASSIFY_OUT* classifyOut
   );

#endif /// (NTDDI_VERSION >= NTDDI_WIN7)

NTSTATUS
TLInspectALEConnectNotify(
   _In_ FWPS_CALLOUT_NOTIFY_TYPE notifyType,
   _In_ const GUID* filterKey,
   _Inout_ const FWPS_FILTER* filter
   );

NTSTATUS
TLInspectALERecvAcceptNotify(
   _In_ FWPS_CALLOUT_NOTIFY_TYPE notifyType,
   _In_ const GUID* filterKey,
   _Inout_ const FWPS_FILTER* filter
   );

NTSTATUS
TLInspectTransportNotify(
   _In_ FWPS_CALLOUT_NOTIFY_TYPE notifyType,
   _In_ const GUID* filterKey,
   _Inout_ const FWPS_FILTER* filter
   );

KSTART_ROUTINE TLInspectWorker;

#endif // _TL_INSPECT_H_
