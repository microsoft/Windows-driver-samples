////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2012 Microsoft Corporation.  All Rights Reserved.
//
//   Module Name:
//      HelperFunctions_NDIS.cpp
//
//   Abstract:
//      This module contains kernel helper functions that assist with NDIS operations.
//
//   Naming Convention:
//
//      <Module><Object><Action>
//  
//      i.e.
//
//       KrnlHlprNDISPoolDataDestroy
//
//       <Module>
//          KrnlHlpr     -       Function is located in syslib\ and applies to kernel mode.
//       <Object>
//          NDISPoolData -       Function pertains to NDIS_POOL_DATA.
//       <Action>
//          {
//            Create     -       Function allocates and fills memory.
//            Destroy    -       Function cleans up and frees memory.
//            Populate   -       Function fills memory with values.
//            Purge      -       Function cleans up values.
//          }
//
//   Private Functions:
//
//   Public Functions:
//      KrnlHlprPendDataDataCreate(),
//      KrnlHlprPendDataDataDestroy(),
//      KrnlHlprPendDataDataPopulate(),
//      KrnlHlprPendDataDataPurge(),
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
#include "HelperFunctions_NDIS.tmh"  /// $(OBJ_PATH)\$(O)\

/**
 @kernel_helper_function="KrnlHlprNDISPoolDataDestroy"
 
   Purpose:  Cleanup a NDIS_POOL_DATA object.                                                   <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/FF562592.aspx                              <br>
             HTTP://MSDN.Microsoft.com/En-US/Library/FF562590.aspx                              <br>
             HTTP://MSDN.Microsoft.com/En-US/Library/FF561850.aspx                              <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
inline VOID KrnlHlprNDISPoolDataPurge(_Inout_ NDIS_POOL_DATA* pNDISPoolData)
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprNDISPoolDataPurge()\n");

#endif /// DBG
   
   NT_ASSERT(pNDISPoolData);

   if(pNDISPoolData->ndisHandle)
   {
      if(pNDISPoolData->nbPoolHandle)
      {
         NdisFreeNetBufferPool(pNDISPoolData->nbPoolHandle);

         pNDISPoolData->nbPoolHandle = 0;
      }

      if(pNDISPoolData->nblPoolHandle)
      {
         NdisFreeNetBufferListPool(pNDISPoolData->nblPoolHandle);

         pNDISPoolData->nblPoolHandle = 0;
      }

      NdisFreeGenericObject((PNDIS_GENERIC_OBJECT)(pNDISPoolData->ndisHandle));

      pNDISPoolData->ndisHandle = 0;
   }

   RtlZeroMemory(pNDISPoolData,
                 sizeof(NDIS_POOL_DATA));

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprNDISPoolDataPurge()\n");

#endif /// DBG
   
   return;
}

/**
 @kernel_helper_function="KrnlHlprNDISPoolDataDestroy"
 
   Purpose:  Cleanup a NDIS_POOL_DATA object.                                                   <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_At_(*ppNDISPoolData, _Pre_ _Notnull_)
_At_(*ppNDISPoolData, _Post_ _Null_ __drv_freesMem(Pool))
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Success_(*ppNDISPoolData == 0)
inline VOID KrnlHlprNDISPoolDataDestroy(_Inout_ NDIS_POOL_DATA** ppNDISPoolData)
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprNDISPoolDataDestroy()\n");

#endif /// DBG
   
   NT_ASSERT(ppNDISPoolData);

   if(*ppNDISPoolData)
   {
      KrnlHlprNDISPoolDataPurge(*ppNDISPoolData);

      HLPR_DELETE(*ppNDISPoolData,
                  WFPSAMPLER_SYSLIB_TAG);
   }

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprNDISPoolDataDestroy()\n");

#endif /// DBG
   
   return;
}

/**
 @kernel_helper_function="KrnlHlprPendDataCreate"
 
   Purpose:  Populates a NDIS_POOL_DATA object with the various NDIS Pools.                     <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/FF561603.aspx                              <br>
             HTTP://MSDN.Microsoft.com/En-US/Library/FF561611.aspx                              <br>
             HTTP://MSDN.Microsoft.com/En-US/Library/FF561613.aspx                              <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Check_return_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprNDISPoolDataPopulate(_Inout_ NDIS_POOL_DATA* pNDISPoolData,
                                      _In_opt_ UINT32 memoryTag)             /* WFPSAMPLER_NDIS_POOL_TAG */
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprNDISPoolDataPopulate()\n");

#endif /// DBG
   
   NT_ASSERT(pNDISPoolData);

   NTSTATUS                        status            = STATUS_SUCCESS;
   NET_BUFFER_LIST_POOL_PARAMETERS nblPoolParameters = {0};
   NET_BUFFER_POOL_PARAMETERS      nbPoolParameters  = {0};

   pNDISPoolData->ndisHandle = NdisAllocateGenericObject(0,
                                                         memoryTag,
                                                         0);
   if(pNDISPoolData->ndisHandle == 0)
   {
      status = STATUS_INVALID_HANDLE;

      DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                 DPFLTR_ERROR_LEVEL,
                 " !!!! KrnlHlprNDISPoolDataPopulate : NdisAllocateGenericObject() [status: %#x]\n",
                 status);

      HLPR_BAIL;
   }

   nblPoolParameters.Header.Type        = NDIS_OBJECT_TYPE_DEFAULT;
   nblPoolParameters.Header.Revision    = NET_BUFFER_LIST_POOL_PARAMETERS_REVISION_1;
   nblPoolParameters.Header.Size        = NDIS_SIZEOF_NET_BUFFER_LIST_POOL_PARAMETERS_REVISION_1;
   nblPoolParameters.fAllocateNetBuffer = TRUE;
   nblPoolParameters.DataSize           = 0;
   nblPoolParameters.PoolTag            = memoryTag;

   pNDISPoolData->nblPoolHandle = NdisAllocateNetBufferListPool(pNDISPoolData->ndisHandle,
                                                                &nblPoolParameters);
   if(pNDISPoolData->nblPoolHandle == 0)
   {
      status = STATUS_INVALID_HANDLE;

      DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                 DPFLTR_ERROR_LEVEL,
                 " !!!! KrnlHlprNDISPoolDataPopulate : NdisAllocateNetBufferListPool() [status: %#x]\n",
                 status);

      HLPR_BAIL;
   }

   nbPoolParameters.Header.Type     = NDIS_OBJECT_TYPE_DEFAULT;
   nbPoolParameters.Header.Revision = NET_BUFFER_POOL_PARAMETERS_REVISION_1;
   nbPoolParameters.Header.Size     = NDIS_SIZEOF_NET_BUFFER_POOL_PARAMETERS_REVISION_1;
   nbPoolParameters.PoolTag         = memoryTag;
   nbPoolParameters.DataSize        = 0;

   pNDISPoolData->nbPoolHandle = NdisAllocateNetBufferPool(pNDISPoolData->ndisHandle,
                                                           &nbPoolParameters);
   if(pNDISPoolData->nbPoolHandle == 0)
   {
      status = STATUS_INVALID_HANDLE;

      DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                 DPFLTR_ERROR_LEVEL,
                 " !!!! KrnlHlprNDISPoolDataPopulate : NdisAllocateNetBufferPool() [status: %#x]\n",
                 status);

      HLPR_BAIL;
   }

   HLPR_BAIL_LABEL:

   if(status != STATUS_SUCCESS)
      KrnlHlprNDISPoolDataPurge(pNDISPoolData);

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprNDISPoolDataPopulate()\n");

#endif /// DBG
   
   return status;
}

/**
 @kernel_helper_function="KrnlHlprPendDataCreate"
 
   Purpose:  Allocates and populates a NDIS_POOL_DATA object with the various NDIS Pools.       <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_At_(*ppNDISPoolData, _Pre_ _Null_)
_When_(return != STATUS_SUCCESS, _At_(*ppNDISPoolData, _Post_ _Null_))
_When_(return == STATUS_SUCCESS, _At_(*ppNDISPoolData, _Post_ _Notnull_ __drv_allocatesMem(Pool)))
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Check_return_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprNDISPoolDataCreate(_Outptr_ NDIS_POOL_DATA** ppNDISPoolData,
                                    _In_opt_ UINT32 memoryTag)                /* WFPSAMPLER_NDIS_POOL_TAG */
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprNDISPoolDataCreate()\n");

#endif /// DBG
   
   NT_ASSERT(ppNDISPoolData);

   NTSTATUS status = STATUS_SUCCESS;

   HLPR_NEW(*ppNDISPoolData,
            NDIS_POOL_DATA,
            WFPSAMPLER_SYSLIB_TAG);
   HLPR_BAIL_ON_ALLOC_FAILURE(*ppNDISPoolData,
                              status);

   status = KrnlHlprNDISPoolDataPopulate(*ppNDISPoolData,
                                         memoryTag);

   HLPR_BAIL_LABEL:

#pragma warning(push)
#pragma warning(disable: 6001) /// *ppNDISPoolData initialized with calls to HLPR_NEW & KrnlHlprNDISPoolDataPopulate

   if(status != STATUS_SUCCESS &&
      *ppNDISPoolData)
      KrnlHlprNDISPoolDataDestroy(ppNDISPoolData);

#pragma warning(pop)

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprNDISPoolDataCreate() [status: %#x]\n",
              status);

#endif /// DBG
   
   return status;
}
