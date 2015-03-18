/*++

Copyright (c) Microsoft Corporation. All rights reserved

Abstract:

   Stream Edit Callout Driver Sample.

   This sample demonstrates finding and replacing a string pattern from a
   live TCP stream via the WFP stream API.

--*/

#ifndef _STREAM_CALLOUT_H
#define _STREAM_CALLOUT_H

extern MDL* gStringToReplaceMdl;
extern HANDLE gInjectionHandle;
extern NDIS_HANDLE gNetBufferListPool;
extern STREAM_EDITOR gStreamEditor;

// 
// Configurable parameters
//

extern USHORT  configInspectionPort;
extern BOOLEAN configInspectionOutbound; 
extern BOOLEAN configEditInline;

extern CHAR configStringToFind[];
extern CHAR configStringToReplace[];

#pragma warning(push)
#pragma warning(disable:4201)       // unnamed struct/union

typedef struct STREAM_EDITOR_
{
   BOOLEAN editInline;

   union
   {
      INLINE_EDIT_STATE inlineEditState;
      struct
      {
         OOB_EDIT_STATE editState;
         BOOLEAN shuttingDown;

         KSPIN_LOCK editLock;
         NET_BUFFER_LIST* nblHead;
         NET_BUFFER_LIST* nblTail;
         size_t totalDataLength;
         BOOLEAN noMoreData;
         NET_BUFFER_LIST* nblEof;
         size_t busyThreshold;
         UINT64 flowId;
         UINT32 calloutId;
         UINT16 layerId;
         DWORD streamFlags;
         KEVENT editEvent;
         LIST_ENTRY outgoingDataQueue;
      } oobEditInfo;
   };

   void* scratchBuffer;
   size_t bufferSize;
   size_t dataOffset;
   size_t dataLength;

}STREAM_EDITOR;

#pragma warning(pop)

BOOLEAN
StreamCopyDataForInspection(
   _Inout_ STREAM_EDITOR* streamEditor,
   const FWPS_STREAM_DATA* streamData
   );

#endif // _STREAM_CALLOUT_H
