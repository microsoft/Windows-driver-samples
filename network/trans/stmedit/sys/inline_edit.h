/*++

Copyright (c) Microsoft Corporation. All rights reserved

Abstract:

    Stream Edit Callout Driver Sample.
    
    This sample demonstrates inline stream inspection/editing 
    via the WFP stream API.

Environment:

    Kernel mode

--*/

#ifndef _INLINE_EDIT_H
#define _INLINE_EDIT_H

typedef enum INLINE_EDIT_STATE_
{
   INLINE_EDIT_WAITING_FOR_DATA,
   INLINE_EDIT_SKIPPING,
   INLINE_EDIT_MODIFYING,
   INLINE_EDIT_SCANNING
} INLINE_EDIT_STATE;

typedef struct STREAM_EDITOR_ STREAM_EDITOR;

void
InlineEditInit(
   _Out_ STREAM_EDITOR*
   );

#if(NTDDI_VERSION >= NTDDI_WIN7)

void 
NTAPI
StreamInlineEditClassify(
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
StreamInlineEditClassify(
   _In_ const FWPS_INCOMING_VALUES* inFixedValues,
   _In_ const FWPS_INCOMING_METADATA_VALUES* inMetaValues,
   _Inout_ void* layerData,
   _In_ const FWPS_FILTER* filter,
   _In_ UINT64 flowContext,
   _Inout_ FWPS_CLASSIFY_OUT* classifyOut
   );

#endif /// (NTDDI_VERSION >= NTDDI_WIN7)

#endif // _INLINE_EDIT_H
