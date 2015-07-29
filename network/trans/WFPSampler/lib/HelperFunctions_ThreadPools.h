////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2012 Microsoft Corporation.  All Rights Reserved.
//
//   Module Name:
//      HelperFunctions_ThreadPools.h
//
//   Abstract:
//      This module contains prototypes of functions which assist in operations pertaining to 
//         Thread Pools.
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

#ifndef HELPERFUNCTIONS_THREAD_POOLS_H
#define HELPERFUNCTIONS_THREAD_POOLS_H

typedef struct THREADPOOL_DATA_
{
   TP_CALLBACK_ENVIRON callbackEnvironment;
   TP_POOL*            pThreadPool;
   TP_CLEANUP_GROUP*   pCleanupGroup;

}THREADPOOL_DATA, *PTHREADPOOL_DATA;

inline VOID HlprThreadPoolDataPurge(_Inout_ THREADPOOL_DATA* pThreadPoolData);

_Success_(*ppThreadPoolData == 0)
inline VOID HlprThreadPoolDataDestroy(_Inout_ THREADPOOL_DATA** ppThreadPoolData);

_Success_(return == NO_ERROR)
UINT32 HlprThreadPoolDataPopulate(_Inout_ THREADPOOL_DATA* pThreadPoolData,
                                  _In_ const PTP_CLEANUP_GROUP_CANCEL_CALLBACK pGroupCancelFn);

_At_(*ppThreadPoolData, _Pre_ _Null_)
_When_(return != NO_ERROR, _At_(*ppThreadPoolData, _Post_ _Null_))
_When_(return == NO_ERROR, _At_(*ppThreadPoolData, _Post_ _Notnull_))
_Success_(return == NO_ERROR)
UINT32 HlprThreadPoolDataCreate(_Outptr_ THREADPOOL_DATA** ppThreadPoolData,
                                _In_ const PTP_CLEANUP_GROUP_CANCEL_CALLBACK pGroupCancelFn);

#endif /// HELPERFUNCTIONS_THREAD_POOLS_H
