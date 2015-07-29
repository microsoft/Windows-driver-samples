////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2012 Microsoft Corporation. All Rights Reserved.
//
//   Module Name:
//      HelperFunctions_ThreadsAndEvents.cpp
//
//   Abstract:
//      This module contains prototypes for functions which simplify threads and eventing.
//
//   Exported Functions:
//
//   Internal Functions:
//
//   Author:
//      Dusty Harper      (DHarper)
//
//   Revision History:
//
//      [ Month ][Day] [Year] - [Revision]-[ Comments ]
//      May       01,   2010  -     1.0   -  Creation
//
////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef HELPERFUNCTIONS_THREADS_AND_EVENTS_H
#define HELPERFUNCTIONS_THREADS_AND_EVENTS_H

typedef struct THREAD_DATA_
{
   LPTHREAD_START_ROUTINE threadStartRoutine;
   UINT32                 threadId;
   HANDLE                 thread;
   HANDLE                 threadStartEvent;
   HANDLE                 threadStopEvent;
   HANDLE                 threadContinueEvent;
}THREAD_DATA, *PTHREAD_DATA;

VOID HlprEventReset(_In_opt_ HANDLE event);

VOID HlprEventSet(_In_opt_ HANDLE event);

_Success_(return == NO_ERROR)
UINT32 HlprThreadStart(_Inout_ THREAD_DATA* pThreadData,
                       _In_opt_ VOID* pData = 0);

_Success_(return == NO_ERROR)
UINT32 HlprThreadStop(_Inout_ THREAD_DATA* pThreadData);
_At_(*ppThreadData, _Pre_ _Notnull_)
_When_(return != NO_ERROR, _At_(*ppThreadData, _Post_ _Notnull_))
_When_(return == NO_ERROR, _At_(*ppThreadData, _Post_ _Null_))
_Success_(return == NO_ERROR && *ppThreadData == 0)
UINT32 HlprThreadStop(_Inout_ THREAD_DATA** ppThreadData);

VOID HlprThreadCleanup(_Inout_opt_ THREAD_DATA* pThreadData);

_Success_(return == NO_ERROR)
UINT32 HlprThreadWaitForCompletion(_Inout_opt_ THREAD_DATA* pThreadData);

_Success_(return == NO_ERROR)
UINT32 HlprThreadWaitForEvent(_In_ HANDLE eventHandle,
                              _In_ THREAD_DATA* pThreadData);

#endif /// HELPERFUNCTIONS_THREADS_AND_EVENTS_H