////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2014 Microsoft Corporation.  All Rights Reserved.
//
//   Module Name:
//      HelperFunctions_NetBuffer.cpp
//
//   Abstract:
//      This module contains kernel helper functions that assist with NET_BUFFER and NET_BUFFER_LIST.
//
//   Naming Convention:
//
//      <Module><Object><Action><Modifier>
//  
//      i.e.
//
//       KrnlHlprNBLGetRequiredRefCount
//
//       <Module>
//          KrnlHlpr           -       Function is located in syslib\ and applies to kernel mode.
//       <Object>
//          NBL                -       Function pertains to NET_BUFFER_LIST objects.
//       <Action>
//          {
//            Get              -       Function retrieves data.
//          }
//       <Modifier>
//          {
//            RequiredRefCount -       Function returns a refCount for the NBL / NBL chain.
//          }
//
//   Private Functions:
//
//   Public Functions:
//      KrnlHlprNBLGetRequiredRefCount(),
//
//   Author:
//      Dusty Harper      (DHarper)
//
//   Revision History:
//
//      [ Month ][Day] [Year] - [Revision]-[ Comments ]
//      May       01,   2010  -     1.0   -  Creation
//      December  13,   2013  -     1.1   -  Add KrnlHlprNBLCreateFromBuffer,
//                                              KrnlHlprNBLCopyToBuffer,
//                                              KrnlHlprNBLDestroyNew,
//                                              KrnlHlprNBLCreateNew
//
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "HelperFunctions_Include.h"     /// . 
#include "HelperFunctions_NetBuffer.tmh" /// $(OBJ_PATH)\$(O)\ 


/**
 @kernel_helper_function="KrnlHlprNBLCreateFromBuffer"
 
   Purpose:  Creates a new NBL from the data contained in the supplied buffer.                  <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Success_(return != 0)
NET_BUFFER_LIST* KrnlHlprNBLCreateFromBuffer(_In_ NDIS_HANDLE nblPoolHandle,
                                             _In_reads_(bufferSize) BYTE* pBuffer,
                                             _In_ UINT32 bufferSize,
                                             _Outptr_opt_result_maybenull_ PMDL* ppMDL)
{
#if DBG

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprNBLCreateFromBuffer()\n");

#endif /// DBG

   NTSTATUS         status = STATUS_SUCCESS;
   NET_BUFFER_LIST* pNBL   = 0;
   PMDL             pMDL   = 0;

   pMDL = IoAllocateMdl(pBuffer,
                        bufferSize,
                        FALSE,
                        FALSE,
                        0);
   if(!pMDL)
   {
      DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                 DPFLTR_ERROR_LEVEL,
                 " !!!! KrnlHlprNBLCreateFromBuffer : IoAllocateMdl() [pMDL: %#p]\n",
                 pMDL);

      HLPR_BAIL;
   }

   MmBuildMdlForNonPagedPool(pMDL);

   status = FwpsAllocateNetBufferAndNetBufferList(nblPoolHandle,
                                                  0,
                                                  0,
                                                  pMDL,
                                                  0,
                                                  bufferSize,
                                                  &pNBL);
   if(status != STATUS_SUCCESS)
   {
      DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                 DPFLTR_ERROR_LEVEL,
                 " !!!! KrnlHlprNBLCreateFromBuffer : FwpsAllocateNetBufferAndNetBufferList() [status: %#x]\n",
                 status);

      HLPR_BAIL;
   }

   HLPR_BAIL_LABEL:

   if(status != STATUS_SUCCESS)
   {
      IoFreeMdl(pMDL);

      pMDL = 0;
   }

   if(ppMDL)
      *ppMDL = pMDL;

#if DBG

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprNBLCreateFromBuffer() [pNBL: %#p]\n",
              pNBL);

#endif /// DBG

   return pNBL;
}

/**
 @kernel_helper_function="KrnlHlprNBLCopyToBuffer"
 
   Purpose:  Copies the NBL to a buffer.                                                        <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Success_(return != 0)
BYTE* KrnlHlprNBLCopyToBuffer(_In_opt_ NET_BUFFER_LIST* pTemplateNBL,
                              _Out_ UINT32* pSize,
                              _In_ UINT32 additionalSpace)             /* 0 */
{
#if DBG

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprNBLCopyToBuffer()\n");

#endif /// DBG

   NTSTATUS status   = STATUS_SUCCESS;
   BYTE*    pBuffer  = 0;
   UINT32   numBytes = additionalSpace;

   *pSize = 0;

   if(pTemplateNBL)
   {
      for(NET_BUFFER* pNB = NET_BUFFER_LIST_FIRST_NB(pTemplateNBL);
          pNB;
          pNB = NET_BUFFER_NEXT_NB(pNB))
      {
         numBytes += NET_BUFFER_DATA_LENGTH(pNB);
      }
   }

   if(numBytes)
   {
      HLPR_NEW_ARRAY(pBuffer,
                     BYTE,
                     numBytes,
                     WFPSAMPLER_SYSLIB_TAG);
      HLPR_BAIL_ON_ALLOC_FAILURE(pBuffer,
                                 status);

      if(pTemplateNBL)
      {
         NET_BUFFER* pNB = NET_BUFFER_LIST_FIRST_NB(pTemplateNBL);

         for(UINT32 bytesCopied = 0;
             bytesCopied < numBytes &&
             pNB;
             pNB = NET_BUFFER_NEXT_NB(pNB))
         {
            BYTE*  pContiguousBuffer = 0;
            BYTE*  pAllocatedBuffer  = 0;
            UINT32 bytesNeeded       = NET_BUFFER_DATA_LENGTH(pNB);

            if(bytesNeeded)
            {
               HLPR_NEW_ARRAY(pAllocatedBuffer,
                              BYTE,
                              bytesNeeded,
                              WFPSAMPLER_SYSLIB_TAG);
               HLPR_BAIL_ON_ALLOC_FAILURE(pAllocatedBuffer,
                                          status);

               pContiguousBuffer = (BYTE*)NdisGetDataBuffer(pNB,
                                                            bytesNeeded,
                                                            pAllocatedBuffer,
                                                            1,
                                                            0);

               RtlCopyMemory(&(pBuffer[bytesCopied]),
                             pContiguousBuffer ? pContiguousBuffer : pAllocatedBuffer,
                             bytesNeeded);

               bytesCopied += bytesNeeded;

               HLPR_DELETE_ARRAY(pAllocatedBuffer,
                                 WFPSAMPLER_SYSLIB_TAG);
            }
         }
      }

      HLPR_BAIL_LABEL:

      if(status != STATUS_SUCCESS)
      {
         HLPR_DELETE_ARRAY(pBuffer,
                           WFPSAMPLER_SYSLIB_TAG);
      }
      else
         *pSize = numBytes;
   }

#if DBG

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprNBLCopyToBuffer() [pBuffer: %#p]\n",
              pBuffer);

#endif /// DBG

   return pBuffer;
}

/**
 @kernel_helper_function="KrnlHlprNBLDestroyNew"
 
   Purpose:  Destroys a new NBL.                                                                <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_Success_(*ppNetBufferList == 0 && *ppMDL == 0 && *ppAllocatedBuffer == 0)
VOID KrnlHlprNBLDestroyNew(_Inout_opt_ NET_BUFFER_LIST** ppNetBufferList,
                           _Inout_opt_ PMDL* ppMDL,
                           _Inout_opt_ BYTE** ppAllocatedBuffer)
{
#if DBG

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprNBLDestroyNew()\n");

#endif /// DBG

   if(ppNetBufferList)
   {
      FwpsFreeNetBufferList(*ppNetBufferList);

      *ppNetBufferList = 0;
   }

   if(ppMDL)
   {
      IoFreeMdl(*ppMDL);

      *ppMDL = 0;
   }

   if(ppAllocatedBuffer)
   {
      HLPR_DELETE_ARRAY(*ppAllocatedBuffer,
                        WFPSAMPLER_SYSLIB_TAG);
   }

#if DBG

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprNBLDestroyNew()\n");

#endif /// DBG

   return;
}

/**
 @kernel_helper_function="KrnlHlprNBLCreateNew"
 
   Purpose:  Creates a new NBL.                                                                 <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Success_(return != 0)
NET_BUFFER_LIST* KrnlHlprNBLCreateNew(_In_ NDIS_HANDLE nblPoolHandle,
                                      _In_opt_ NET_BUFFER_LIST* pTemplateNBL,
                                      _Outptr_opt_result_buffer_maybenull_(*pSize) BYTE** ppAllocatedBuffer,
                                      _Out_ UINT32* pSize,
                                      _Outptr_opt_result_maybenull_ PMDL* ppMDL, 
                                      _In_ UINT32 additionalSpace,                                 /* 0 */
                                      _In_ BOOLEAN isOutbound)                                     /* FALSE */
{
#if DBG

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprNBLCreateNew()\n");

#endif /// DBG

   NET_BUFFER_LIST* pNBL    = 0;
   UINT32           size    = 0;
   BYTE*            pBuffer = KrnlHlprNBLCopyToBuffer(pTemplateNBL,
                                                      &size,
                                                      additionalSpace);

   if(pBuffer &&
      size)
   {
      pNBL = KrnlHlprNBLCreateFromBuffer(nblPoolHandle,
                                         pBuffer,
                                         size,
                                         ppMDL);
      if(pNBL &&
         pTemplateNBL)
      {
         if(isOutbound)
            NdisCopySendNetBufferListInfo(pNBL,
                                          pTemplateNBL);
         else
            NdisCopyReceiveNetBufferListInfo(pNBL,
                                             pTemplateNBL);
      }
      else
      {
         HLPR_DELETE_ARRAY(pBuffer,
                           WFPSAMPLER_SYSLIB_TAG);

         size = 0;
      }
   }

   if(ppAllocatedBuffer)
      *ppAllocatedBuffer = pBuffer;

   *pSize = size;

#if DBG

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprNBLCreateNew() [pNBL: %#p]\n",
              pNBL);

#endif /// DBG

   return pNBL;
}


/**
 @kernel_helper_function="KrnlHlprNBLGetRequiredRefCount"
 
   Purpose:  Return a count of how many NBLs are within the NBL chain.                          <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
UINT32 KrnlHlprNBLGetRequiredRefCount(_In_ const NET_BUFFER_LIST* pNBL,
                                      _In_ BOOLEAN isChained)           /* FALSE */
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprNBLGetRequiredRefCount()\n");

#endif /// DBG

   NT_ASSERT(pNBL);

   UINT32 requiredRefCount = 0;

   if(isChained)
   {
      for(NET_BUFFER_LIST* pCurrentNBL = (NET_BUFFER_LIST*)pNBL;
          pCurrentNBL;
          pCurrentNBL = NET_BUFFER_LIST_NEXT_NBL(pCurrentNBL))
      {
         requiredRefCount++;
      }
   }
   else
      requiredRefCount = 1;

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprNBLGetRequiredRefCount() [refCount: %#d]\n",
              requiredRefCount);

#endif /// DBG
   
   return requiredRefCount;
}
