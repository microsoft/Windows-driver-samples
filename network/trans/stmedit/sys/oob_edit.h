/*++

Copyright (c) Microsoft Corporation. All rights reserved

Abstract:

    Stream Edit Callout Driver Sample.
    
    This sample demonstrates Out-of-band (OOB) stream inspection/editing 
    via the WFP stream API.

Environment:

    Kernel mode

--*/

#ifndef _OOB_EDIT_H
#define _OOB_EDIT_H

typedef enum OOB_EDIT_STATE_
{
   OOB_EDIT_IDLE,
   OOB_EDIT_PROCESSING,
   OOB_EDIT_BUSY,
   OOB_EDIT_SHUT_DOWN,
   OOB_EDIT_ERROR
} OOB_EDIT_STATE;

typedef struct OUTGOING_STREAM_DATA_ 
{
   LIST_ENTRY listEntry;

   NET_BUFFER_LIST* netBufferList;
   BOOLEAN isClone;
   size_t dataLength;
   DWORD streamFlags;
   MDL* mdl;
} OUTGOING_STREAM_DATA;

typedef struct STREAM_EDITOR_ STREAM_EDITOR;

NTSTATUS
OobEditInit(
   _Out_ STREAM_EDITOR*
   );

void
OobEditShutdown(
   _Out_ STREAM_EDITOR* streamEditor
   );

#if(NTDDI_VERSION >= NTDDI_WIN7)

void 
NTAPI
StreamOobEditClassify(
   _In_ const FWPS_INCOMING_VALUES* inFixedValues,
   _In_ const FWPS_INCOMING_METADATA_VALUES* inMetaValues,
   _Inout_ void* layerData,
   _In_ const void* classifyContext,
   _In_ const FWPS_FILTER* filter,
   _In_ UINT64 flowContext,
   _Inout_ FWPS_CLASSIFY_OUT* classifyOut
   );

#else

void 
NTAPI
StreamOobEditClassify(
   _In_ const FWPS_INCOMING_VALUES* inFixedValues,
   _In_ const FWPS_INCOMING_METADATA_VALUES* inMetaValues,
   _Inout_ void* layerData,
   _In_ const FWPS_FILTER* filter,
   _In_ UINT64 flowContext,
   _Inout_ FWPS_CLASSIFY_OUT* classifyOut
   );

#endif /// (NTDDI_VERSION >= NTDDI_WIN7)


#endif // _OOB_EDIT_H
