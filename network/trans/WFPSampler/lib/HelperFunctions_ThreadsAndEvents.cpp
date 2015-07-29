////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2012 Microsoft Corporation.  All Rights Reserved.
//
//   Module Name:
//      HelperFunctions_ThreadsAndEvents.cpp
//
//   Abstract:
//      This module contains functions which functions which simplify threads and eventing.
//
//   Naming Convention:
//
//      <Scope><Module><Object><Action><Modifier>
//  
//      i.e.
//
//       <Scope>
//          {
//                          - Function is likely visible to other modules
//          }
//       <Module>
//          {
//            Hlpr          - Function is from HelperFunctions_* Modules.
//          }
//       <Object>
//          {
//            Event         - Function pertains to events.
//            Thread        - Function pertains to threads.
//          }
//       <Action>
//          {
//            Cleanup       -
//            Set           -
//            Start         - 
//            Stop          -
//            Wait          -
//          }
//       <Modifier>
//          {
//            ForCompletion -
//            ForEvent      - Function determines equality between values.
//          }
//
//   Private Functions:
//
//   Public Functions:
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

#include "HelperFunctions_Include.h" /// .

/**
 @helper_function="HlprEventReset"
 
   Purpose:  Sets the event to nonsignaled.                                                     <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-Us/Library/Windows/Desktop/MS685081.aspx              <br>
*/
VOID HlprEventReset(_In_opt_ HANDLE event)
{
   if(event)
      ResetEvent(event);

   return;
}


/**
 @helper_function="HlprEventSet"
 
   Purpose:  Signals an event.                                                                  <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-Us/Library/Windows/Desktop/MS686211.aspx              <br>
*/
VOID HlprEventSet(_In_opt_ HANDLE event)
{
   if(event)
      SetEvent(event);

   return;
}

/**
 @helper_function="HlprThreadStart"
 
   Purpose:  Generates a new thread.                                                            <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-Us/Library/Windows/Desktop/MS682396.aspx              <br>
             HTTP://MSDN.Microsoft.com/En-Us/Library/Windows/Desktop/MS682453.aspx              <br>
*/
_Success_(return == NO_ERROR)
UINT32 HlprThreadStart(_Inout_ THREAD_DATA* pThreadData,
                       _In_opt_ VOID* pData)             /* 0 */
{
   UINT32 status = NO_ERROR;

   if(pThreadData)
   {
      HlprThreadCleanup(pThreadData);

      if(pThreadData->threadStartRoutine)
      {
         pThreadData->threadStartEvent = CreateEvent(0,
                                                     TRUE,
                                                     FALSE,
                                                     0);

         pThreadData->threadStopEvent = CreateEvent(0,
                                                    TRUE,
                                                    FALSE,
                                                    0);

         pThreadData->threadContinueEvent = CreateEvent(0,
                                                        TRUE,
                                                        FALSE,
                                                        0);

         pThreadData->thread = CreateThread(0,
                                            0,
                                            (LPTHREAD_START_ROUTINE)pThreadData->threadStartRoutine,
                                            pData ? pData : pThreadData,
                                            0,
                                            (LPDWORD)&(pThreadData->threadId));

         if(pThreadData->threadStartEvent &&
            pThreadData->threadStopEvent &&
            pThreadData->threadContinueEvent &&
            pThreadData->thread)
            HlprThreadWaitForEvent(pThreadData->threadStartEvent,
                                   pThreadData);
         else
         {
            status = ERROR_GEN_FAILURE;

            HlprThreadCleanup(pThreadData);

            HlprLogError(L"HlprThreadStart() [status: %#x]",
                         status);
         }
      }
      else
      {
         status = ERROR_INVALID_DATA;

         HlprLogError(L"HlprThreadStart() [status: %#x][pThreadData->threadStartRoutine: %#x]",
                      status,
                      pThreadData->threadStartRoutine);
      }
   }
   else
   {
      status = ERROR_INVALID_PARAMETER;

      HlprLogError(L"HlprThreadStart() [status: %#x][pThread: %#x]",
                   status,
                   pThreadData);
   }

   return status;
}

/**
 @helper_function="HlprThreadStop"
 
   Purpose:  Signals a thread to stop and cleans it up.                                         <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-Us/Library/Windows/Desktop/MS682396.aspx              <br>                                                                                   <br>
*/
_Success_(return == NO_ERROR)
UINT32 HlprThreadStop(_Inout_ THREAD_DATA* pThreadData)
{
   UINT32       status         = NO_ERROR;
   const UINT32 RETRY_ATTEMPTS = 5;

   if(pThreadData)
   {
      for(UINT32 i = 0;
          status != WAIT_TIMEOUT &&
          i < RETRY_ATTEMPTS;
          i++)
      {
         HlprEventSet(pThreadData->threadStopEvent);

         status = HlprThreadWaitForCompletion(pThreadData);
      }

      HlprThreadCleanup(pThreadData);
   }

   return status;
}

/**
 @helper_function="HlprThreadStop"
 
   Purpose:  Signals a thread to stop and cleans it up, and frees any allocated memory.         <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_At_(*ppThreadData, _Pre_ _Notnull_)
_When_(return != NO_ERROR, _At_(*ppThreadData, _Post_ _Notnull_))
_When_(return == NO_ERROR, _At_(*ppThreadData, _Post_ _Null_))
_Success_(return == NO_ERROR && *ppThreadData == 0)
UINT32 HlprThreadStop(_Inout_ THREAD_DATA** ppThreadData)
{
   UINT32 status = NO_ERROR;

   if(ppThreadData)
   {
      status = HlprThreadStop(*ppThreadData);
      if(status == NO_ERROR)
      {
         HLPR_DELETE(*ppThreadData)
      }
   }

   return status;
}

/**
 @helper_function="HlprThreadCleanup"
 
   Purpose:  Cleans up a previously allocated thread.                                           <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-Us/Library/Windows/Desktop/MS683190.aspx              <br>
             HTTP://MSDN.Microsoft.com/En-Us/Library/Windows/Desktop/MS686717.aspx              <br>
*/
VOID HlprThreadCleanup(_Inout_opt_ THREAD_DATA* pThreadData)
{
   if(pThreadData)
   {
      LPTHREAD_START_ROUTINE threadStartRoutine = pThreadData->threadStartRoutine;
      UINT32                 status             = NO_ERROR;

      if(pThreadData->thread)
      {
         if(GetExitCodeThread(pThreadData->thread,
                              (DWORD*)&status))
         {
            if(status == STILL_ACTIVE)
            {
               UINT64       timeAlpha    = GetTickCount64();
               const UINT32 FOUR_MINUTES = 240000;

               for(UINT64 timeOmega = GetTickCount64();
                   timeOmega - timeAlpha < FOUR_MINUTES;
                   timeOmega = GetTickCount64())
               {
                  HlprEventSet(pThreadData->threadStopEvent);

                  status = HlprThreadWaitForCompletion(pThreadData);
                  if(status == NO_ERROR)
                     break;
               }

               if(status != NO_ERROR)
               {
                  HlprLogInfo(L"Possible Runaway Thread");

#pragma warning(push)
#pragma warning(disable: 6258) /// This is a last resort

                  TerminateThread(pThreadData->thread,
                                  ERROR_THREAD_WAS_SUSPENDED);

#pragma warning(pop)
               }
            }
         }
         else
         {
            status = GetLastError();

            HlprLogError(L"HlprThreadCleanup() [status:%#x]",
                         status);
         }
      }

      HLPR_CLOSE_HANDLE(pThreadData->threadStopEvent);

      HLPR_CLOSE_HANDLE(pThreadData->threadStartEvent);

      HLPR_CLOSE_HANDLE(pThreadData->threadContinueEvent);

      ZeroMemory(pThreadData,
                 sizeof(THREAD_DATA));

      pThreadData->threadStartRoutine = threadStartRoutine;
   }

   return;
}

/**
 @helper_function="HlprThreadWaitForCompletion"
 
   Purpose: Waits for a thread to complete.                                                     <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_Success_(return == NO_ERROR)
UINT32 HlprThreadWaitForCompletion(_Inout_opt_ THREAD_DATA* pThreadData)
{
   UINT32 status = NO_ERROR;

   if(pThreadData)
   {
      HlprEventSet(pThreadData->threadStopEvent);

      status = HlprThreadWaitForEvent(pThreadData->thread,
                                      pThreadData);
   }

   return status;
}

/**
 @helper_function="HlprThreadWaitForEvent"
 
   Purpose:  Waits for a particular event to be set within the thread.                          <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_Success_(return == NO_ERROR)
UINT32 HlprThreadWaitForEvent(_In_ HANDLE eventHandle,
                              _In_ THREAD_DATA* pThreadData)
{
   UINT32       status         = NO_ERROR;
   const UINT32 RETRY_ATTEMPTS = 6;
   WCHAR*       pEventName     = 0;

   if(eventHandle &&
      pThreadData)
   {
      if(eventHandle == pThreadData->threadContinueEvent)
         pEventName = L"Continue Event";
      else if(eventHandle == pThreadData->threadStartEvent)
         pEventName = L"Start Event";
      else if(eventHandle == pThreadData->threadStopEvent)
         pEventName = L"Stop Event";
      else if(eventHandle == pThreadData->thread)
         pEventName = L"Thread";

      /// Try for 30 seconds before bailing
      for(UINT32 i = 0;
          WaitForSingleObject(eventHandle,
                              5000) == WAIT_TIMEOUT;
          i++)
      {
         HlprLogInfo(L"HlprThreadWaitForEvent() Waiting for %s ...",
                     pEventName);

         if(i == RETRY_ATTEMPTS - 1)
         {
            status = WAIT_TIMEOUT;

            HlprLogInfo(L"HlprThreadWaitForEvent() [status: %#x]");

            break;
         }
      }
   }

   return status;
}
