////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2012 Microsoft Corporation.  All Rights Reserved.
//
//   Module Name:
//      HelperFunctions_ClassifyData.cpp
//
//   Abstract:
//      This module contains kernel helper functions that assist with CLASSIFY_DATA.
//
//   Naming Convention:
//
//      <Module><Object><Action><Modifier>
//  
//      i.e.
//
//       KrnlHlprClassifyDataCreateLocalCopy
//
//       <Module>
//          KrnlHlpr     -       Function is located in syslib\ and applies to kernel mode.
//       <Object>
//          ClassifyData -       Function pertains to CLASSIFY_DATA objects.
//       <Action>
//          {
//            Acquire    -       Function will take a reference on an object
//            Create     -       Function allocates and fills memory.
//            Destroy    -       Function cleans up and frees memory.
//            Release    -       Function releases the reference on the object
//          }
//       <Modifer>
//          LocalCopy    -       Function performs a deep copy of the parameters or takes references
//                                  on complex structures.
//
//   Private Functions:
//
//   Public Functions:
//      KrnlHlprAcquireDataCreateLocalCopy(),
//      KrnlHlprClassifyDataCreateLocalCopy(),
//      KrnlHlprClassifyDataDestroyLocalCopy(),
//      KrnlHlprClassifyDataReleaseLocalCopy(),
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

#include "HelperFunctions_Include.h"        /// .
#include "HelperFunctions_ClassifyData.tmh" /// $(OBJ_PATH)\$(O)\

INT64 g_OutstandingNBLReferences = 0;

/**
 @kernel_helper_function="KrnlHlprClassifyDataReleaseLocalCopy"
 
   Purpose:  Release reference on NBLs and cleanup a local copy of a CLASSIFY_DATA.             <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF551159.aspx             <br>
             HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF551208.aspx             <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
inline VOID KrnlHlprClassifyDataReleaseLocalCopy(_Inout_ CLASSIFY_DATA* pClassifyData)
{
#if DBG

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprClassifyDataReleaseLocalCopy()\n");

#endif /// DBG
   
   NT_ASSERT(pClassifyData);

   KrnlHlprFwpsClassifyOutDestroyLocalCopy((FWPS_CLASSIFY_OUT**)&(pClassifyData->pClassifyOut));

   pClassifyData->flowContext = 0;

   KrnlHlprFwpsFilterDestroyLocalCopy((FWPS_FILTER**)&(pClassifyData->pFilter));

#if(NTDDI_VERSION >= NTDDI_WIN7)
   
      if(pClassifyData->classifyContextHandle)
         FwpsReleaseClassifyHandle(pClassifyData->classifyContextHandle);
   
#endif /// (NTDDI_VERSION >= NTDDI_WIN7)

   if(pClassifyData->pPacket)
   {
      if(pClassifyData->pClassifyValues)
      {
         if(pClassifyData->pClassifyValues->layerId == FWPS_LAYER_STREAM_V4 ||
            pClassifyData->pClassifyValues->layerId == FWPS_LAYER_STREAM_V4_DISCARD ||
            pClassifyData->pClassifyValues->layerId == FWPS_LAYER_STREAM_V6 ||
            pClassifyData->pClassifyValues->layerId == FWPS_LAYER_STREAM_V6_DISCARD)
            KrnlHlprFwpsStreamCalloutIOPacketDestroyLocalCopy((FWPS_STREAM_CALLOUT_IO_PACKET**)&(pClassifyData->pPacket));
         else if(pClassifyData->chainedNBL)
         {
            BOOLEAN isDispatch	   = (KeGetCurrentIrql() == DISPATCH_LEVEL) ? TRUE : FALSE;
            UINT32  numChainedNBLs  = pClassifyData->numChainedNBLs;

            for(NET_BUFFER_LIST* pCurrentNBL = (NET_BUFFER_LIST*)pClassifyData->pPacket;
                pCurrentNBL &&
                numChainedNBLs;
                numChainedNBLs--)
            {
               NET_BUFFER_LIST* pNextNBL = NET_BUFFER_LIST_NEXT_NBL(pCurrentNBL);

               FwpsDereferenceNetBufferList(pCurrentNBL,
                                            isDispatch);

               pCurrentNBL = pNextNBL;

#if DBG

               InterlockedDecrement64((LONG64*)&(g_OutstandingNBLReferences));

#endif /// DBG

            }
         }
         else
         {
            BOOLEAN isDispatch = (KeGetCurrentIrql() == DISPATCH_LEVEL) ? TRUE : FALSE;

            FwpsDereferenceNetBufferList((NET_BUFFER_LIST*)pClassifyData->pPacket,
                                         isDispatch);

#if DBG

            InterlockedDecrement64((LONG64*)&(g_OutstandingNBLReferences));

#endif /// DBG

         }
      }

      pClassifyData->pPacket = 0;
   }

   KrnlHlprFwpsIncomingMetadataValuesDestroyLocalCopy((FWPS_INCOMING_METADATA_VALUES**)&(pClassifyData->pMetadataValues));

   if(pClassifyData->pClassifyValues)
      KrnlHlprFwpsIncomingValuesDestroyLocalCopy((FWPS_INCOMING_VALUES**)&(pClassifyData->pClassifyValues));

   RtlZeroMemory(pClassifyData,
                 sizeof(CLASSIFY_DATA));

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprClassifyDataReleaseLocalCopy()\n");

#endif /// DBG
   
   return;
}

/**
 @kernel_helper_function="KrnlHlprClassifyDataDestroyLocalCopy"
 
   Purpose:  Release reference on packet and cleanup and free a local copy of CLASSIFY_DATA.    <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_At_(*ppClassifyData, _Pre_ _Notnull_)
_At_(*ppClassifyData, _Post_ _Null_ __drv_freesMem(Pool))
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Success_(*ppClassifyData == 0)
inline VOID KrnlHlprClassifyDataDestroyLocalCopy(_Inout_ CLASSIFY_DATA** ppClassifyData)
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprClassifyDataDestroyLocalCopy()\n");

#endif /// DBG
   
   NT_ASSERT(ppClassifyData);

   if(*ppClassifyData)
   {
      KrnlHlprClassifyDataReleaseLocalCopy(*ppClassifyData);

      HLPR_DELETE(*ppClassifyData,
                  WFPSAMPLER_SYSLIB_TAG);
   }

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprClassifyDataDestroyLocalCopy()\n");

#endif /// DBG
   
   return;
}

/**
 @kernel_helper_function="KrnlHlprClassifyDataAcquireLocalCopy"
 
   Purpose:  Ppopulate a CLASSIFY_DATA with a local copy of data obtained from a 
             callout's classification. This local copy requiires taking a reference 
             on pPacket.                                                                        <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF550085.aspx             <br>
             HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF551206.aspx             <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprClassifyDataAcquireLocalCopy(_Inout_ CLASSIFY_DATA* pClassifyData,
                                              _In_ const FWPS_INCOMING_VALUES* pClassifyValues,
                                              _In_ const FWPS_INCOMING_METADATA_VALUES* pMetadata,
                                              _In_opt_ VOID* pPacket,
                                              _In_opt_ const VOID* pClassifyContext,
                                              _In_ const FWPS_FILTER* pFilter,
                                              _In_ const UINT64 flowContext,
                                              _In_ FWPS_CLASSIFY_OUT* pClassifyOut)
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprClassifyDataAcquireLocalCopy()\n");

#endif /// DBG
   
   NT_ASSERT(pClassifyData);
   NT_ASSERT(pClassifyValues);
   NT_ASSERT(pMetadata);
   NT_ASSERT(pFilter);
   NT_ASSERT(pClassifyOut);

   NTSTATUS status = STATUS_SUCCESS;

   pClassifyData->pClassifyValues = KrnlHlprFwpsIncomingValuesCreateLocalCopy(pClassifyValues);
   HLPR_BAIL_ON_NULL_POINTER_WITH_STATUS(pClassifyData->pClassifyValues,
                                         status);

   pClassifyData->pMetadataValues = KrnlHlprFwpsIncomingMetadataValuesCreateLocalCopy(pMetadata);
   HLPR_BAIL_ON_NULL_POINTER_WITH_STATUS(pClassifyData->pMetadataValues,
                                         status);

   if(pPacket)
   {
      if(pClassifyValues->layerId == FWPS_LAYER_STREAM_V4 ||
         pClassifyValues->layerId == FWPS_LAYER_STREAM_V4_DISCARD ||
         pClassifyValues->layerId == FWPS_LAYER_STREAM_V6 ||
         pClassifyValues->layerId == FWPS_LAYER_STREAM_V6_DISCARD)
      {
         pClassifyData->pPacket = KrnlHlprFwpsStreamCalloutIOPacketCreateLocalCopy((FWPS_STREAM_CALLOUT_IO_PACKET*)pPacket);
         HLPR_BAIL_ON_NULL_POINTER_WITH_STATUS(pClassifyData->pPacket,
                                               status);
      }

#if(NTDDI_VERSION >= NTDDI_WIN7)

      /// LayerData at the FWPM_LAYER_ALE_{BIND/CONNECT}_REDIRECT_V{4/6} is obtained via KrnlHlprRedirectDataCreate()
      else if(pClassifyValues->layerId == FWPS_LAYER_ALE_CONNECT_REDIRECT_V4 ||
              pClassifyValues->layerId == FWPS_LAYER_ALE_CONNECT_REDIRECT_V6 ||
              pClassifyValues->layerId == FWPS_LAYER_ALE_BIND_REDIRECT_V4 ||
              pClassifyValues->layerId == FWPS_LAYER_ALE_BIND_REDIRECT_V6)
      {
         pClassifyData->pPacket = 0;
      }

#endif /// (NTDDI_VERSION >= NTDDI_WIN7)

      else
      {
         if(NET_BUFFER_LIST_NEXT_NBL((NET_BUFFER_LIST*)pPacket))
         {
            pClassifyData->chainedNBL     = TRUE;
            pClassifyData->numChainedNBLs = 1;
         }

         if(pClassifyData->chainedNBL &&
            (
            /// The IPPACKET and IPFORWARD Layers allow for Fragment Grouping if the option is enabled
            pClassifyValues->layerId == FWPS_LAYER_INBOUND_IPPACKET_V4 ||
            pClassifyValues->layerId == FWPS_LAYER_INBOUND_IPPACKET_V6 ||
            pClassifyValues->layerId == FWPS_LAYER_IPFORWARD_V4 ||
            pClassifyValues->layerId == FWPS_LAYER_IPFORWARD_V6

#if(NTDDI_VERSION >= NTDDI_WIN8)

            /// The NDIS layers allow for batched NBLs provided the callout was registered with FWP_CALLOUT_FLAG_ALLOW_L2_BATCH_CLASSIFY set
            ||
            pClassifyValues->layerId == FWPS_LAYER_INBOUND_MAC_FRAME_ETHERNET ||
            pClassifyValues->layerId == FWPS_LAYER_OUTBOUND_MAC_FRAME_ETHERNET ||
            pClassifyValues->layerId == FWPS_LAYER_INBOUND_MAC_FRAME_NATIVE ||
            pClassifyValues->layerId == FWPS_LAYER_OUTBOUND_MAC_FRAME_NATIVE ||
            pClassifyValues->layerId == FWPS_LAYER_INGRESS_VSWITCH_ETHERNET ||
            pClassifyValues->layerId == FWPS_LAYER_EGRESS_VSWITCH_ETHERNET

#endif /// (NTDDI_VERSION >= NTDDI_WIN8)

            ))
         {
            for(NET_BUFFER_LIST* pCurrentNBL = (NET_BUFFER_LIST*)pPacket;
                pCurrentNBL;
                pClassifyData->numChainedNBLs++)
            {
               NET_BUFFER_LIST* pNextNBL = NET_BUFFER_LIST_NEXT_NBL(pCurrentNBL);

               FwpsReferenceNetBufferList(pCurrentNBL,
                                          TRUE);

               pCurrentNBL = pNextNBL;

#if DBG

               InterlockedIncrement64((LONG64*)&(g_OutstandingNBLReferences));

#endif /// DBG

            }

            pClassifyData->pPacket = pPacket;
         }
         else
         {
            /// Otherwise we expect to receive a single NBL
            NT_ASSERT(NET_BUFFER_LIST_NEXT_NBL((NET_BUFFER_LIST*)pPacket) == 0);

            FwpsReferenceNetBufferList((NET_BUFFER_LIST*)pPacket,
                                       TRUE);

            pClassifyData->pPacket = pPacket;

#if DBG
         
            InterlockedIncrement64((LONG64*)&(g_OutstandingNBLReferences));
         
#endif /// DBG

         }
      }
   }

#if(NTDDI_VERSION >= NTDDI_WIN7)
   
      if(pClassifyContext)
      {
         /// ClassifyHandle for these layers is obtained in REDIRECT_DATA
         if(pClassifyValues->layerId != FWPS_LAYER_ALE_CONNECT_REDIRECT_V4 &&
            pClassifyValues->layerId != FWPS_LAYER_ALE_CONNECT_REDIRECT_V6 &&
            pClassifyValues->layerId != FWPS_LAYER_ALE_BIND_REDIRECT_V4 &&
            pClassifyValues->layerId != FWPS_LAYER_ALE_BIND_REDIRECT_V6)
         {
            status = FwpsAcquireClassifyHandle((VOID*)pClassifyContext,
                                               0,
                                               &(pClassifyData->classifyContextHandle));
            HLPR_BAIL_ON_FAILURE(status);
         }
      }
#else
   
      UNREFERENCED_PARAMETER(pClassifyContext);
   
#endif /// (NTDDI_VERSION >= NTDDI_WIN7)

   if(pFilter)
   {
      pClassifyData->pFilter = KrnlHlprFwpsFilterCreateLocalCopy(pFilter);
      HLPR_BAIL_ON_NULL_POINTER_WITH_STATUS(pClassifyData->pFilter,
                                            status);
   }

   pClassifyData->flowContext = flowContext;

   if(pClassifyOut)
   {
      pClassifyData->pClassifyOut = KrnlHlprFwpsClassifyOutCreateLocalCopy(pClassifyOut);
      HLPR_BAIL_ON_NULL_POINTER_WITH_STATUS(pClassifyData->pClassifyOut,
                                            status);
   }

   HLPR_BAIL_LABEL:

   if(status != STATUS_SUCCESS)
      KrnlHlprClassifyDataReleaseLocalCopy(pClassifyData);

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprClassifyDataAcquireLocalCopy() [status: %#x]\n",
              status);

#endif /// DBG
   
   return status;
}

/**
 @kernel_helper_function="KrnlHlprClassifyDataCreateLocalCopy"
 
   Purpose:  Allocate and populate a CLASSIFY_DATA with a local copy of data obtained from a 
             callout's classifyFn. This local copy requiires taking a reference on pPacket.     <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_At_(*ppClassifyData, _Pre_ _Null_)
_When_(return != STATUS_SUCCESS, _At_(*ppClassifyData, _Post_ _Null_))
_When_(return == STATUS_SUCCESS, _At_(*ppClassifyData, _Post_ _Notnull_ __drv_allocatesMem(Pool)))
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Check_return_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprClassifyDataCreateLocalCopy(_Outptr_ CLASSIFY_DATA** ppClassifyData,
                                             _In_ const FWPS_INCOMING_VALUES* pClassifyValues,
                                             _In_ const FWPS_INCOMING_METADATA_VALUES* pMetadata,
                                             _In_opt_ VOID* pPacket,
                                             _In_opt_ const VOID* pClassifyContext,
                                             _In_ const FWPS_FILTER* pFilter,
                                             _In_ const UINT64 flowContext,
                                             _In_ FWPS_CLASSIFY_OUT* pClassifyOut)
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprClassifyDataCreateLocalCopy()\n");

#endif /// DBG
   
   NT_ASSERT(ppClassifyData);
   NT_ASSERT(pClassifyValues);
   NT_ASSERT(pMetadata);
   NT_ASSERT(pFilter);
   NT_ASSERT(pClassifyOut);

   NTSTATUS status = STATUS_SUCCESS;

   HLPR_NEW(*ppClassifyData,
            CLASSIFY_DATA,
            WFPSAMPLER_SYSLIB_TAG);
   HLPR_BAIL_ON_ALLOC_FAILURE(*ppClassifyData,
                              status);

   status = KrnlHlprClassifyDataAcquireLocalCopy(*ppClassifyData,
                                                 pClassifyValues,
                                                 pMetadata,
                                                 pPacket,
                                                 pClassifyContext,
                                                 pFilter,
                                                 flowContext,
                                                 pClassifyOut);

   HLPR_BAIL_LABEL:

#pragma warning(push)
#pragma warning(disable: 6001) /// *ppClassifyData initialized with call to HLPR_NEW & KrnlHlprClassifyDataAcquireLocalCopy 

   if(status != STATUS_SUCCESS &&
      *ppClassifyData)
      KrnlHlprClassifyDataDestroyLocalCopy(ppClassifyData);

#pragma warning(pop)

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprClassifyDataCreateLocalCopy() [status: %#x]\n",
              status);

#endif /// DBG

   return status;
}
