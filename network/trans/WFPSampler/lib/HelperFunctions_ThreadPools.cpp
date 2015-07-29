////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2012 Microsoft Corporation. All Rights Reserved.
//
//   Module Name:
//      HelperFunctions_ThreadPools.cpp
//
//   Abstract:
//      This module contains functions for assisting in operations pertaining to Thread Pools.
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
 @helper_function="HlprThreadPoolDataPurge"
 
   Purpose:  Cleanup a THREADPOOL_DATA object.                                                  <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/MS682036.aspx              <br>
             HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/MS682033.aspx              <br>
             HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/MS682030.aspx              <br>
             HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/MS682576.aspx              <br>
*/
inline VOID HlprThreadPoolDataPurge(_Inout_ THREADPOOL_DATA* pThreadPoolData)
{
   ASSERT(pThreadPoolData);

   if(pThreadPoolData->pCleanupGroup)
   {
      CloseThreadpoolCleanupGroupMembers(pThreadPoolData->pCleanupGroup,
                                         FALSE,
                                         0);

      CloseThreadpoolCleanupGroup(pThreadPoolData->pCleanupGroup);
   }

   if(pThreadPoolData->pThreadPool)
      CloseThreadpool(pThreadPoolData->pThreadPool);

   DestroyThreadpoolEnvironment(&(pThreadPoolData->callbackEnvironment));

   ZeroMemory(pThreadPoolData,
              sizeof(THREADPOOL_DATA));

   return;
}

/**
 @helper_function="HlprThreadPoolDataDestroy"
 
   Purpose:  Cleanup and free a THREADPOOL_DATA object.                                         <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_Success_(*ppThreadPoolData == 0)
inline VOID HlprThreadPoolDataDestroy(_Inout_ THREADPOOL_DATA** ppThreadPoolData)
{
   ASSERT(ppThreadPoolData);

   if(*ppThreadPoolData)
      HlprThreadPoolDataPurge(*ppThreadPoolData);

   HLPR_DELETE(*ppThreadPoolData);

   return;
}

/**
 @helper_function="HlprThreadPoolDataPurge"
 
   Purpose:  Cleanup a THREADPOOL_DATA object.                                                  <br>
                                                                                                <br>
   Notes:    The caller is responsible for freeing any allocated memory using
             HlprThreadPoolDataPurge().                                                         <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/MS683486.aspx              <br>
             HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/MS682456.aspx              <br>
             HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/MS682462.aspx              <br>
             HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/MS686261.aspx              <br>
             HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/MS686255.aspx              <br>

*/
_Success_(return == NO_ERROR)
UINT32 HlprThreadPoolDataPopulate(_Inout_ THREADPOOL_DATA* pThreadPoolData,
                                  _In_ const PTP_CLEANUP_GROUP_CANCEL_CALLBACK pGroupCancelFn)
{
   ASSERT(pThreadPoolData);

   UINT32 status = NO_ERROR;

   InitializeThreadpoolEnvironment(&(pThreadPoolData->callbackEnvironment));

   pThreadPoolData->pThreadPool = CreateThreadpool(0);
   if(pThreadPoolData->pThreadPool == 0)
   {
      status = GetLastError();

      HlprLogError(L"HlprThreadPoolDataPopulate : CreateThreadPool() [status: %#x]",
                   status);

      HLPR_BAIL;
   }

   pThreadPoolData->pCleanupGroup = CreateThreadpoolCleanupGroup();
   if(pThreadPoolData->pCleanupGroup == 0)
   {
      status = GetLastError();

      HlprLogError(L"HlprThreadPoolDataPopulate : CreateThreadPool() [status: %#x]",
                   status);

      HLPR_BAIL;
   }  

   SetThreadpoolCallbackPool(&(pThreadPoolData->callbackEnvironment),
                             pThreadPoolData->pThreadPool);

   SetThreadpoolCallbackCleanupGroup(&(pThreadPoolData->callbackEnvironment),
                                     pThreadPoolData->pCleanupGroup,
                                     pGroupCancelFn);

   HLPR_BAIL_LABEL:

   if(status != NO_ERROR)
      HlprThreadPoolDataPurge(pThreadPoolData);

   return status;
}

/**
 @helper_function="HlprThreadPoolDataDestroy"
 
   Purpose:  Allocate and populate a THREADPOOL_DATA object.                                    <br>
                                                                                                <br>
   Notes:    The caller is responsible for freeing any allocated memory using
             HlprThreadPoolDataDestroy().                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_At_(*ppThreadPoolData, _Pre_ _Null_)
_When_(return != NO_ERROR, _At_(*ppThreadPoolData, _Post_ _Null_))
_When_(return == NO_ERROR, _At_(*ppThreadPoolData, _Post_ _Notnull_))
_Success_(return == NO_ERROR)
UINT32 HlprThreadPoolDataCreate(_Outptr_ THREADPOOL_DATA** ppThreadPoolData,
                                _In_ const PTP_CLEANUP_GROUP_CANCEL_CALLBACK pGroupCancelFn)
{
   ASSERT(ppThreadPoolData);

   UINT32 status = NO_ERROR;

   HLPR_NEW(*ppThreadPoolData,
            THREADPOOL_DATA);
   HLPR_BAIL_ON_ALLOC_FAILURE(*ppThreadPoolData,
                              status);

   status = HlprThreadPoolDataPopulate(*ppThreadPoolData,
                                       pGroupCancelFn);

   HLPR_BAIL_LABEL:

   if(status != NO_ERROR)
      HlprThreadPoolDataDestroy(ppThreadPoolData);


   return status;
}
