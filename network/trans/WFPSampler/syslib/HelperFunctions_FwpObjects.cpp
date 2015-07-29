////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2014 Microsoft Corporation.  All Rights Reserved.
//
//   Module Name:
//      HelperFunctions\FwpObjects.c
//
//   Abstract:
//      This module contains kernel helper functions that allocate, populate, purge, and free WFP 
//         structures in memory.
//
//   Naming Convention:
//
//      <Module><Object><Action><Modifier>
//  
//      i.e.
//
//       KrnlHlprFwpValueCreateLocalCopy
//
//       <Module>
//          KrnlHlpr                     -       Function is located in syslib\ and applies to kernel
//                                                  mode.
//       <Object>
//          {
//            FwpByteBlob                -       Function pertains to FWP_BYTE_BLOB objects.
//            FwpConditionValue          -       Function pertains to FWP_CONDITION_VALUE objects.
//            FwpmClassifyOption         -       Function pertains to FWPM_CLASSIFY_OPTION objects.
//            FwpmClassifyOptions        -       Function pertains to FWPM_CLASSIFY_OPTIONS objects.
//            FwpmDisplayData            -       Function pertains to FWPM_DISPLAY_DATA objects.
//            FwpmProviderContext        -       Function pertains to FWPM_PROVIDER_CONTEXT objects.
//            FwpRange                   -       Function pertains to FWP_RANGE objects.
//            FwpsClassifyOut            -       Function pertains to FWPS_CLASSIFY_OUT objects.
//            FwpsFilter                 -       Function pertains to FWPS_FILTER objects.
//            FwpsFilterCondition        -       Function pertains to FWPS_FILTER_CONDITION objects.
//            FwpsIncomingMetadataValues -       Function pertains to FWPS_INCOMING_METADATA 
//                                                  objects.
//            FwpsIncomingValues         -       Function pertains to FWPS_INCOMING_VALUES objects.
//            FwpsStreamCalloutIOPacket  -       Function pertains to FWPS_STREAM_DATA objects.
//            FwpsStreamData             -       Function pertains to FWPS_STREAM_DATA objects.
//            FwpTokenInformation        -       Function pertains to FWP_TOKEN_INFORMATION objects.
//            FwpV4AddrAndMask           -       Function pertains to FWP_V4_ADDR_AND_MASK objects.
//            FwpV6AddrAndMask           -       Function pertains to FWP_V6_ADDR_AND_MASK objects.
//            FwpValue                   -       Function pertains to FWP_VALUE objects.
//            IPsecDoSPOptions           -       Function pertains to IPSEC_DOSP_OPTIONS objects.
//          }
//       <Action>
//          {
//            Get             -       Function returns a pointer or value
//            Create          -       Function allocates and fills memory
//            Populate        -       Function fills memory with values
//            Acquire         -       Function will take a reference on an object
//            Release         -       Function releases the reference on the object
//            Purge           -       Function cleans up values
//            Destroy         -       Function cleans up and frees memory
//          }
//       <Modifier>
//          LocalCopy         -       Function acts on the LocalCopy (as opposed to the original)
//
//   Author:
//      Dusty Harper      (DHarper)
//
//   Revision History:
//
//      [ Month ][Day] [Year] - [Revision]-[ Comments ]
//      May       01,   2010  -     1.0   -  Creation
//      December  13,   2013  -     1.1   -  Add support for multiple injectors, add 
//                                              KrnlHlprFwpValueGetStringForFwpsIncomingValue, 
//                                              KrnlHlprFwpsRedirectHandleCreate,
//                                              KrnlHlprFwpsRedirecthandleDestroy,
//                                              KrnlHlprFwpsLayerIsDiscard, 
//                                              KrnlHlprFwpsLayerIDToString functions, add missing 
//                                              conditions, and fix invalid conditions.
//
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "HelperFunctions_Include.h"      /// .
#include "HelperFunctions_FwpObjects.tmh" /// $(OBJ_PATH)\$(O)\ 

HANDLE g_EngineHandle                                    = 0;
HANDLE g_pIPv4InboundMACInjectionHandles[2]              = {0};
HANDLE g_pIPv4IngressVSwitchEthernetInjectionHandles[2]  = {0};
HANDLE g_pIPv4InboundForwardInjectionHandles[2]          = {0};
HANDLE g_pIPv4InboundNetworkInjectionHandles[2]          = {0};
HANDLE g_pIPv4InboundTransportInjectionHandles[2]        = {0};
HANDLE g_pIPv4InboundStreamInjectionHandles[2]           = {0};
HANDLE g_pIPv4OutboundMACInjectionHandles[2]             = {0};
HANDLE g_pIPv4EgressVSwitchEthernetInjectionHandles[2]   = {0};
HANDLE g_pIPv4OutboundForwardInjectionHandles[2]         = {0};
HANDLE g_pIPv4OutboundNetworkInjectionHandles[2]         = {0};
HANDLE g_pIPv4OutboundTransportInjectionHandles[2]       = {0};
HANDLE g_pIPv4OutboundStreamInjectionHandles[2]          = {0};
HANDLE g_pIPv6InboundMACInjectionHandles[2]              = {0};
HANDLE g_pIPv6IngressVSwitchEthernetInjectionHandles[2]  = {0};
HANDLE g_pIPv6InboundForwardInjectionHandles[2]          = {0};
HANDLE g_pIPv6InboundNetworkInjectionHandles[2]          = {0};
HANDLE g_pIPv6InboundTransportInjectionHandles[2]        = {0};
HANDLE g_pIPv6InboundStreamInjectionHandles[2]           = {0};
HANDLE g_pIPv6OutboundForwardInjectionHandles[2]         = {0};
HANDLE g_pIPv6OutboundNetworkInjectionHandles[2]         = {0};
HANDLE g_pIPv6OutboundTransportInjectionHandles[2]       = {0};
HANDLE g_pIPv6OutboundStreamInjectionHandles[2]          = {0};
HANDLE g_pIPv6OutboundMACInjectionHandles[2]             = {0};
HANDLE g_pIPv6EgressVSwitchEthernetInjectionHandles[2]   = {0};
HANDLE g_pInboundForwardInjectionHandles[2]              = {0};
HANDLE g_pInboundNetworkInjectionHandles[2]              = {0};
HANDLE g_pInboundTransportInjectionHandles[2]            = {0};
HANDLE g_pInboundStreamInjectionHandles[2]               = {0};
HANDLE g_pInboundMACInjectionHandles[2]                  = {0};
HANDLE g_pIngressVSwitchEthernetInjectionHandles[2]      = {0};
HANDLE g_pOutboundForwardInjectionHandles[2]             = {0};
HANDLE g_pOutboundNetworkInjectionHandles[2]             = {0};
HANDLE g_pOutboundTransportInjectionHandles[2]           = {0};
HANDLE g_pOutboundStreamInjectionHandles[2]              = {0};
HANDLE g_pOutboundMACInjectionHandles[2]                 = {0};
HANDLE g_pEgressVSwitchEthernetInjectionHandles[2]       = {0};
HANDLE g_pRedirectionHandles[2]                          = {0};

#ifndef FWP_BYTE_BLOB____
#define FWP_BYTE_BLOB____

#pragma warning(disable: 24002) /// Code is IPv4 & IPv6 specific

/**
 @kernel_helper_function="KrnlHlprFwpByteBlobPurgeLocalCopy"
 
   Purpose:  Cleanup a local copy of an FWP_BYTE_BLOB.                                          <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF552427.aspx             <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
inline VOID KrnlHlprFwpByteBlobPurgeLocalCopy(_Inout_ FWP_BYTE_BLOB* pBlob)
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprFwpByteBlobPurgeLocalCopy()\n");

#endif /// DBG
   
   NT_ASSERT(pBlob);

   HLPR_DELETE_ARRAY(pBlob->data,
                     WFPSAMPLER_SYSLIB_TAG);

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprFwpByteBlobPurgeLocalCopy()\n");

#endif /// DBG

   return;
}

/**
 @kernel_helper_function="KrnlHlprFwpByteBlobDestroyLocalCopy"
 
   Purpose:  Cleanup and free a local copy of an FWP_BYTE_BLOB.                                 <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF552427.aspx             <br>
*/
_At_(*ppBlob, _Pre_ _Notnull_)
_At_(*ppBlob, _Post_ _Null_ __drv_freesMem(Pool))
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Success_(*ppBlob == 0)
inline VOID KrnlHlprFwpByteBlobDestroyLocalCopy(_Inout_ FWP_BYTE_BLOB** ppBlob)
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprFwpByteBlobDestroyLocalCopy()\n");

#endif /// DBG
   
   NT_ASSERT(ppBlob);

   if(*ppBlob)
      KrnlHlprFwpByteBlobPurgeLocalCopy(*ppBlob);

   HLPR_DELETE(*ppBlob,
               WFPSAMPLER_SYSLIB_TAG);

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprFwpByteBlobDestroyLocalCopy()\n");

#endif /// DBG

   return;
}

/**
 @kernel_helper_function="KrnlHlprFwpByteBlobPopulateLocalCopy"
 
   Purpose:  Populate a local copy of an FWP_BYTE_BLOB.                                         <br>
                                                                                                <br>
   Notes:    The caller is responsible for freeing any allocated memory using
             KrnlHlprFwpByteBlobPurgeLocalCopy().                                               <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF552427.aspx             <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Check_return_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprFwpByteBlobPopulateLocalCopy(_In_ const FWP_BYTE_BLOB* pOriginalBlob,
                                              _Inout_ FWP_BYTE_BLOB* pBlob)
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprFwpByteBlobPopulateLocalCopy()\n");

#endif /// DBG
   
   NT_ASSERT(pOriginalBlob);
   NT_ASSERT(pBlob);

   NTSTATUS status = STATUS_SUCCESS;

   pBlob->size = pOriginalBlob->size;

   if(pBlob->size)
   {
      HLPR_NEW_ARRAY(pBlob->data,
                     BYTE,
                     pBlob->size,
                     WFPSAMPLER_SYSLIB_TAG);
      HLPR_BAIL_ON_ALLOC_FAILURE(pBlob->data,
                                 status);

      RtlCopyMemory(pBlob->data,
                    pOriginalBlob->data,
                    pBlob->size);
   }

   HLPR_BAIL_LABEL:

   if(status != STATUS_SUCCESS)
      KrnlHlprFwpByteBlobPurgeLocalCopy(pBlob);

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprFwpByteBlobPopulateLocalCopy() [status: %#x]\n",
              status);

#endif /// DBG

   return status;
}

/**
 @kernel_helper_function="KrnlHlprFwpByteBlobCreateLocalCopy"
 
   Purpose:  Allocate and populate a local copy of an FWP_BYTE_BLOB.                            <br>
                                                                                                <br>
   Notes:    The caller is responsible for freeing the allocated memory using
             KrnlHlprFwpByteBlobDestroyLocalCopy().                                             <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF552427.aspx             <br>
*/
__drv_allocatesMem(Pool)
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Must_inspect_result_
_Success_(return != 0)
FWP_BYTE_BLOB* KrnlHlprFwpByteBlobCreateLocalCopy(_In_ const FWP_BYTE_BLOB* pOriginalBlob)
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprFwpByteBlobCreateLocalCopy()\n");

#endif /// DBG
   
   NT_ASSERT(pOriginalBlob);

   NTSTATUS       status    = STATUS_SUCCESS;
   FWP_BYTE_BLOB* pByteBlob = 0;

#pragma warning(push)
#pragma warning(disable: 6014) /// pByteBlob is expected to be cleaned up by caller using KrnlHlprFwpByteBlobDestroyLocalCopy

   HLPR_NEW(pByteBlob,
            FWP_BYTE_BLOB,
            WFPSAMPLER_SYSLIB_TAG);
   HLPR_BAIL_ON_ALLOC_FAILURE(pByteBlob,
                              status);

#pragma warning(pop)

   status = KrnlHlprFwpByteBlobPopulateLocalCopy(pOriginalBlob,
                                                 pByteBlob);
   HLPR_BAIL_ON_FAILURE(status);

   HLPR_BAIL_LABEL:

#pragma warning(push)
#pragma warning(disable: 6001) /// pByteBlob initialized with call to HLPR_NEW & KrnlHlprFwpByteBlobPopulateLocalCopy 

   if(status != STATUS_SUCCESS &&
      pByteBlob)
      KrnlHlprFwpByteBlobDestroyLocalCopy(&pByteBlob);

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprFwpByteBlobCreateLocalCopy() [pByteBlob: %#p]\n",
              pByteBlob);

#endif /// DBG

   return pByteBlob;

#pragma warning(pop)
}

#endif /// FWP_BYTE_BLOB____

#ifndef FWPM_DISPLAY_DATA____
#define FWPM_DISPLAY_DATA____

/**
 @kernel_helper_function="KrnlHlprFwpmDisplayDataPurgeLocalCopy"
 
   Purpose:  Cleanup a local copy of an FWPM_DISPLAY_DATA.                                      <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/AA364261.aspx              <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
inline VOID KrnlHlprFwpmDisplayDataPurgeLocalCopy(_Inout_ FWPM_DISPLAY_DATA* pData)
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprFwpmDisplayDataPurgeLocalCopy()\n");

#endif /// DBG
   
   NT_ASSERT(pData);

   HLPR_DELETE_ARRAY(pData->name,
                     WFPSAMPLER_SYSLIB_TAG);

   HLPR_DELETE_ARRAY(pData->description,
                     WFPSAMPLER_SYSLIB_TAG);

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprFwpmDisplayDataPurgeLocalCopy()\n");

#endif /// DBG

   return;
}

/**
 @kernel_helper_function="KrnlHlprFwpmDisplayDataDestroyLocalCopy"
 
   Purpose:  Cleanup and free a local copy of an FWPM_DISPLAY_DATA.                             <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/AA364261.aspx              <br>
*/
_At_(*ppDisplayData, _Pre_ _Notnull_)
_At_(*ppDisplayData, _Post_ _Null_ __drv_freesMem(Pool))
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Success_(*ppDisplayData == 0)
inline VOID KrnlHlprFwpmDisplayDataDestroyLocalCopy(_Inout_ FWPM_DISPLAY_DATA** ppDisplayData)
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprFwpmDisplayDataDestroyLocalCopy()\n");

#endif /// DBG
   
   NT_ASSERT(ppDisplayData);

   if(*ppDisplayData)
      KrnlHlprFwpmDisplayDataPurgeLocalCopy(*ppDisplayData);

   HLPR_DELETE(*ppDisplayData,
               WFPSAMPLER_SYSLIB_TAG);

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprFwpmDisplayDataDestroyLocalCopy()\n");

#endif /// DBG

   return;
}

/**
 @kernel_helper_function="KrnlHlprFwpmDisplayDataPopulateLocalCopy"
 
   Purpose:  Populate a local copy of an FWPM_DISPLAY_DATA.                                     <br>
                                                                                                <br>
   Notes:    The caller is responsible for freeing any allocated memory using
             KrnlHlprFwpmDisplayDataPurgeLocalCopy().                                           <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/AA364261.aspx              <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Check_return_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprFwpmDisplayDataPopulateLocalCopy(_In_ const FWPM_DISPLAY_DATA* pOriginalData,
                                                  _Inout_ FWPM_DISPLAY_DATA* pData)
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprFwpmDisplayDataPopulateLocalCopy()\n");

#endif /// DBG
   
   NT_ASSERT(pOriginalData);
   NT_ASSERT(pData);

   NTSTATUS status          = STATUS_SUCCESS;
   KIRQL    irql            = KeGetCurrentIrql();
   size_t   nameSize        = 0;
   size_t   descriptionSize = 0;      

   if(irql == PASSIVE_LEVEL)
   {
      status = RtlStringCchLengthW(pOriginalData->name,
                                   NTSTRSAFE_MAX_CCH,
                                   &nameSize);
      HLPR_BAIL_ON_FAILURE(status);

      if(pOriginalData->description)
      {
         status = RtlStringCchLengthW(pOriginalData->description,
                                      NTSTRSAFE_MAX_CCH,
                                      &descriptionSize);
         HLPR_BAIL_ON_FAILURE(status);
      }
   }
   else
   {
      nameSize = wcslen(pOriginalData->name);

      if(pOriginalData->description)
         descriptionSize = wcslen(pOriginalData->description);
   }

   /// name is required for the FWPM_DISPLAY_DATA to be valid
   if(nameSize)
   {
      HLPR_NEW_ARRAY(pData->name,
                     WCHAR,
                     nameSize + 1,                     /// add space for '\0'
                     WFPSAMPLER_SYSLIB_TAG);
      HLPR_BAIL_ON_ALLOC_FAILURE(pData->name,
                                 status);

      RtlCopyMemory(pData->name,
                    pOriginalData->name,
                    nameSize);

      pData->name[nameSize] = L'\0';
   }
   else
   {
      status = STATUS_INVALID_BUFFER_SIZE;

      HLPR_BAIL;
   }

   /// description is not required for the FWPM_DISPLAY_DATA to be valid
   if(descriptionSize)
   {
      HLPR_NEW_ARRAY(pData->description,
                     WCHAR,
                     descriptionSize + 1,                     /// add space for '\0'
                     WFPSAMPLER_SYSLIB_TAG);
      HLPR_BAIL_ON_ALLOC_FAILURE(pData->description,
                                 status);

      RtlCopyMemory(pData->description,
                    pOriginalData->description,
                    nameSize);

      pData->description[descriptionSize] = L'\0';
   }

   HLPR_BAIL_LABEL:

   if(status != STATUS_SUCCESS)
   {
      HLPR_DELETE_ARRAY(pData->name,
                        WFPSAMPLER_SYSLIB_TAG);

      HLPR_DELETE_ARRAY(pData->description,
                        WFPSAMPLER_SYSLIB_TAG);
   }

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprFwpmDisplayDataPopulateLocalCopy() [status: %#x]\n",
              status);

#endif /// DBG

   return status;
}

/**
 @kernel_helper_function="KrnlHlprFwpmDisplayDataCreateLocalCopy"
 
   Purpose:  ALlocate and populate a local copy of an FWPM_DISPLAY_DATA.                        <br>
                                                                                                <br>
   Notes:    The caller is responsible for freeing any allocated memory using
             KrnlHlprFwpmDisplayDataDestroyLocalCopy().                                         <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/AA364261.aspx              <br>
*/
__drv_allocatesMem(Pool)
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Must_inspect_result_
_Success_(return != 0)
FWPM_DISPLAY_DATA* KrnlHlprFwpmDisplayDataCreateLocalCopy(_In_ const FWPM_DISPLAY_DATA* pOriginalDisplayData)
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprFwpmDisplayDataCreateLocalCopy()\n");

#endif /// DBG
   
   NT_ASSERT(pOriginalDisplayData);

   NTSTATUS           status       = STATUS_SUCCESS;
   FWPM_DISPLAY_DATA* pDisplayData = 0;

#pragma warning(push)
#pragma warning(disable: 6014) /// pDisplayData is expected to be cleaned up by caller using KrnlHlprFwpmDisplayDataDestroyLocalCopy

   HLPR_NEW(pDisplayData,
            FWPM_DISPLAY_DATA,
            WFPSAMPLER_SYSLIB_TAG);
   HLPR_BAIL_ON_ALLOC_FAILURE(pDisplayData,
                              status);

#pragma warning(pop)

   status = KrnlHlprFwpmDisplayDataPopulateLocalCopy(pOriginalDisplayData,
                                                     pDisplayData);

   HLPR_BAIL_LABEL:

#pragma warning(push)
#pragma warning(disable: 6001) /// pDisplayData initialized with call to HLPR_NEW & KrnlHlprFwpmDisplayDataPopulateLocalCopy 

   if(status != STATUS_SUCCESS &&
      pDisplayData)
      KrnlHlprFwpmDisplayDataDestroyLocalCopy(&pDisplayData);

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprFwpmDisplayDataCreateLocalCopy() [pDisplayData: %#p]\n",
              pDisplayData);

#endif /// DBG

   return pDisplayData;

#pragma warning(pop)
}

#endif /// FWP_DISPLAY_DATA____

#ifndef FWPM_CLASSIFY_OPTION____
#define FWPM_CLASSIFY_OPTION____

/**
 @kernel_helper_function="KrnlHlprFwpmClassifyOptionPurgeLocalCopy"
 
   Purpose:  Cleanup a local copy of an FWPM_CLASSIFY_OPTION.                                   <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/AA364259.aspx              <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
inline VOID KrnlHlprFwpmClassifyOptionPurgeLocalCopy(_Inout_ FWPM_CLASSIFY_OPTION* pOption)
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprFwpmClassifyOptionPurgeLocalCopy()\n");

#endif /// DBG
   
   NT_ASSERT(pOption);

   KrnlHlprFwpValuePurgeLocalCopy(&(pOption->value));

   pOption->type = FWP_CLASSIFY_OPTION_MAX;

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprFwpmClassifyOptionPurgeLocalCopy()\n");

#endif /// DBG

   return;
}

/**
 @kernel_helper_function="KrnlHlprFwpmClassifyOptionDestroyLocalCopy"
 
   Purpose:  Cleanup and free a local copy of an FWPM_CLASSIFY_OPTION.                          <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/AA364259.aspx              <br>
*/
_At_(*ppOption, _Pre_ _Notnull_)
_At_(*ppOption, _Post_ _Null_  __drv_freesMem(Pool))
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Success_(*ppOption == 0)
inline VOID KrnlHlprFwpmClassifyOptionDestroyLocalCopy(_Inout_ FWPM_CLASSIFY_OPTION** ppOption)
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprFwpmClassifyOptionDestroyLocalCopy()\n");

#endif /// DBG
   
   NT_ASSERT(ppOption);

   if(*ppOption)
      KrnlHlprFwpmClassifyOptionPurgeLocalCopy(*ppOption);

   HLPR_DELETE(*ppOption,
               WFPSAMPLER_SYSLIB_TAG);

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprFwpmClassifyOptionDestroyLocalCopy()\n");

#endif /// DBG

   return;
}

/**
 @kernel_helper_function="KrnlHlprFwpmClassifyOptionPopulateLocalCopy"
 
   Purpose:  Populate a local copy of an FWPM_CLASSIFY_OPTION.                                  <br>
                                                                                                <br>
   Notes:    The caller is responsible for freeing any allocated memory using
             KrnlHlprFwpmClassifyOptionPurgeLocalCopy().                                        <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/AA364259.aspx              <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Check_return_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprFwpmClassifyOptionPopulateLocalCopy(_In_ const FWPM_CLASSIFY_OPTION* pOriginalOption,
                                                     _Inout_ FWPM_CLASSIFY_OPTION* pOption)
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprFwpmClassifyOptionPopulateLocalCopy()\n");

#endif /// DBG
   
   NT_ASSERT(pOriginalOption);
   NT_ASSERT(pOption);

   NTSTATUS status = STATUS_SUCCESS;

   status = KrnlHlprFwpValuePopulateLocalCopy(&(pOriginalOption->value),
                                              &(pOption->value));
   HLPR_BAIL_ON_FAILURE(status);

   pOption->type = pOriginalOption->type;

   HLPR_BAIL_LABEL:

   if(status != STATUS_SUCCESS)
   {
      KrnlHlprFwpValuePurgeLocalCopy(&(pOption->value));

      pOption->type = FWP_CLASSIFY_OPTION_MAX;
   }

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprFwpmClassifyOptionPopulateLocalCopy() [status: %#x]\n",
              status);

#endif /// DBG

   return status;
}

/**
 @kernel_helper_function="KrnlHlprFwpmClassifyOptionCreateLocalCopy"
 
   Purpose:  Allocate and populate a local copy of an FWPM_CLASSIFY_OPTION.                     <br>
                                                                                                <br>
   Notes:    The caller is responsible for freeing any allocated memory using
             KrnlHlprFwpmClassifyOptionDestroyLocalCopy().                                      <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/AA364259.aspx              <br>
*/
__drv_allocatesMem(Pool)
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Must_inspect_result_
_Success_(return != 0)
FWPM_CLASSIFY_OPTION* KrnlHlprFwpmClassifyOptionCreateLocalCopy(_In_ const FWPM_CLASSIFY_OPTION* pOriginalOption)
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprFwpmClassifyOptionCreateLocalCopy()\n");

#endif /// DBG
   
   FWPM_CLASSIFY_OPTION* pOption = 0;

   if(pOriginalOption)
   {
      NTSTATUS status = STATUS_SUCCESS;

#pragma warning(push)
#pragma warning(disable: 6014) /// pOption is expected to be cleaned up by caller using KrnlHlprFwpmClassifyOptionDestroyLocalCopy

      HLPR_NEW(pOption,
               FWPM_CLASSIFY_OPTION,
               WFPSAMPLER_SYSLIB_TAG);
      HLPR_BAIL_ON_ALLOC_FAILURE(pOption,
                                 status);

#pragma warning(pop)

      status = KrnlHlprFwpmClassifyOptionPopulateLocalCopy(pOriginalOption,
                                                           pOption);

      HLPR_BAIL_LABEL:

#pragma warning(push)
#pragma warning(disable: 6001) /// pOption initialized with call to HLPR_NEW & KrnlHlprFwpmClassifyOptionPopulateLocalCopy 

      if(status != STATUS_SUCCESS)
         KrnlHlprFwpmClassifyOptionDestroyLocalCopy(&pOption);
   }

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprFwpmClassifyOptionCreateLocalCopy() [pOption: %#p]\n",
              pOption);

#endif /// DBG

   return pOption;

#pragma warning(pop)
}

#endif /// FWPM_CLASSIFY_OPTION____

#ifndef FWPM_CLASSIFY_OPTIONS____
#define FWPM_CLASSIFY_OPTIONS____

/**
 @kernel_helper_function="KrnlHlprFwpmClassifyOptionsPurgeLocalCopy"
 
   Purpose:  Cleanup a local copy of an FWPM_CLASSIFY_OPTIONS.                                  <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/AA364260.aspx              <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
inline VOID KrnlHlprFwpmClassifyOptionsPurgeLocalCopy(_Inout_ FWPM_CLASSIFY_OPTIONS* pOptions)
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprFwpmClassifyOptionsPurgeLocalCopy()\n");

#endif /// DBG
   
   NT_ASSERT(pOptions);

   for(UINT32 optionIndex = 0;
       optionIndex < pOptions->numOptions;
       optionIndex++)
   {
      KrnlHlprFwpmClassifyOptionPurgeLocalCopy(&(pOptions->options[optionIndex]));
   }

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprFwpmClassifyOptionsPurgeLocalCopy()\n");

#endif /// DBG

   return;
}

/**
 @kernel_helper_function="KrnlHlprFwpmClassifyOptionsDestroyLocalCopy"
 
   Purpose:  Cleanup and free a local copy of an FWPM_CLASSIFY_OPTIONS.                         <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/AA364260.aspx              <br>
*/
_At_(*ppOptions, _Pre_ _Notnull_)
_At_(*ppOptions, _Post_ _Null_ __drv_freesMem(Pool))
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Success_(*ppOptions == 0)
inline VOID KrnlHlprFwpmClassifyOptionsDestroyLocalCopy(_Inout_ FWPM_CLASSIFY_OPTIONS** ppOptions)
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprFwpmClassifyOptionsDestroyLocalCopy()\n");

#endif /// DBG
   
   NT_ASSERT(ppOptions);

   if(*ppOptions)
      KrnlHlprFwpmClassifyOptionsPurgeLocalCopy(*ppOptions);

   HLPR_DELETE(*ppOptions,
               WFPSAMPLER_SYSLIB_TAG);

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprFwpmClassifyOptionsDestroyLocalCopy()\n");

#endif /// DBG

   return;
}

/**
 @kernel_helper_function="KrnlHlprFwpmClassifyOptionsPopulateLocalCopy"
 
   Purpose:  Populate a local copy of an FWPM_CLASSIFY_OPTIONS.                                 <br>
                                                                                                <br>
   Notes:    The caller is responsible for freeing the allocated memory using
             KrnlHlprFwpmClassifyOptionsPurgeLocalCopy().                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/AA364260.aspx              <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Check_return_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprFwpmClassifyOptionsPopulateLocalCopy(_In_ const FWPM_CLASSIFY_OPTIONS* pOriginalOptions,
                                                      _Inout_ FWPM_CLASSIFY_OPTIONS* pOptions)
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprFwpmClassifyOptionsPopulateLocalCopy()\n");

#endif /// DBG
   
   NT_ASSERT(pOriginalOptions);
   NT_ASSERT(pOptions);

   NTSTATUS status = STATUS_SUCCESS;

   if(pOriginalOptions->numOptions)
   {
      HLPR_NEW_ARRAY(pOptions->options,
                     FWPM_CLASSIFY_OPTION,
                     pOriginalOptions->numOptions,
                     WFPSAMPLER_SYSLIB_TAG);
      HLPR_BAIL_ON_ALLOC_FAILURE(pOptions->options,
                                 status);

      pOptions->numOptions = pOriginalOptions->numOptions;

      for(UINT32 optionIndex = 0;
          optionIndex < pOptions->numOptions;
          optionIndex++)
      {
         status = KrnlHlprFwpmClassifyOptionPopulateLocalCopy(&(pOriginalOptions->options[optionIndex]),
                                                              &(pOptions->options[optionIndex]));
         HLPR_BAIL_ON_FAILURE(status);
      }
   }

   HLPR_BAIL_LABEL:

   if(status != STATUS_SUCCESS)
      KrnlHlprFwpmClassifyOptionsPurgeLocalCopy(pOptions);

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprFwpmClassifyOptionsPopulateLocalCopy() [status: %#x]\n",
              status);

#endif /// DBG

   return status;
}

/**
 @kernel_helper_function="KrnlHlprFwpmClassifyOptionsCreateLocalCopy"
 
   Purpose:  Allocate and populate a local copy of an FWPM_CLASSIFY_OPTIONS.                    <br>
                                                                                                <br>
   Notes:    The caller is responsible for freeing the allocated memory using
             KrnlHlprFwpmClassifyOptionsDestroyLocalCopy().                                     <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/AA364260.aspx              <br>
*/
__drv_allocatesMem(Pool)
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Must_inspect_result_
_Success_(return != 0)
FWPM_CLASSIFY_OPTIONS* KrnlHlprFwpmClassifyOptionsCreateLocalCopy(_In_ const FWPM_CLASSIFY_OPTIONS* pOriginalOptions)
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprFwpmClassifyOptionsCreateLocalCopy()\n");

#endif /// DBG
   
   NT_ASSERT(pOriginalOptions);

   NTSTATUS               status   = STATUS_SUCCESS;
   FWPM_CLASSIFY_OPTIONS* pOptions = 0;

#pragma warning(push)
#pragma warning(disable: 6014) /// pOptions is expected to be cleaned up by caller using KrnlHlprFwpmClassifyOptionsDestroyLocalCopy

   HLPR_NEW(pOptions,
            FWPM_CLASSIFY_OPTIONS,
            WFPSAMPLER_SYSLIB_TAG);
   HLPR_BAIL_ON_ALLOC_FAILURE(pOptions,
                              status);

#pragma warning(pop)

   status = KrnlHlprFwpmClassifyOptionsPopulateLocalCopy(pOriginalOptions,
                                                         pOptions);

   HLPR_BAIL_LABEL:

#pragma warning(push)
#pragma warning(disable: 6001) /// pOptions initialized with call to HLPR_NEW & KrnlHlprFwpmClassifyOptionsPopulateLocalCopy 

   if(status != STATUS_SUCCESS &&
      pOptions)
      KrnlHlprFwpmClassifyOptionsDestroyLocalCopy(&pOptions);

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprFwpmClassifyOptionsCreateLocalCopy() [pOptions: %#p]\n",
              pOptions);

#endif /// DBG

   return pOptions;

#pragma warning(pop)
}

#endif /// FWPM_CLASSIFY_OPTIONS____

#ifndef IPSEC_DOSP_OPTIONS____
#define IPSEC_DOSP_OPTIONS____

#if(NTDDI_VERSION >= NTDDI_WIN7)

/**
 @kernel_helper_function="KrnlHlprIPsecDoSPOptionsPurgeLocalCopy"
 
   Purpose:  Cleanup a local copy of an IPSEC_DOSP_OPTIONS.                                     <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/DD744994.aspx              <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
inline VOID KrnlHlprIPsecDoSPOptionsPurgeLocalCopy(_Inout_ IPSEC_DOSP_OPTIONS* pOptions)
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprIPsecDoSPOptionsPurgeLocalCopy()\n");

#endif /// DBG
   
   NT_ASSERT(pOptions);

   HLPR_DELETE_ARRAY(pOptions->publicIFLuids,
                     WFPSAMPLER_SYSLIB_TAG);

   HLPR_DELETE_ARRAY(pOptions->internalIFLuids,
                     WFPSAMPLER_SYSLIB_TAG);

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprIPsecDoSPOptionsPurgeLocalCopy()\n");

#endif /// DBG

   return;
}

/**
 @kernel_helper_function="KrnlHlprIPsecDoSPOptionsDestroyLocalCopy"
 
   Purpose:  Cleanup and free a local copy of an IPSEC_DOSP_OPTIONS.                            <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/DD744994.aspx              <br>
*/
_At_(*ppOptions, _Pre_ _Notnull_)
_At_(*ppOptions, _Post_ _Null_ __drv_freesMem(Pool))
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Success_(*ppOptions == 0)
inline VOID KrnlHlprIPsecDoSPOptionsDestroyLocalCopy(_Inout_ IPSEC_DOSP_OPTIONS** ppOptions)
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprIPsecDoSPOptionsDestroyLocalCopy()\n");

#endif /// DBG
   
   NT_ASSERT(ppOptions);

   if(*ppOptions)
      KrnlHlprIPsecDoSPOptionsPurgeLocalCopy(*ppOptions);

   HLPR_DELETE(*ppOptions,
               WFPSAMPLER_SYSLIB_TAG);

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprIPsecDoSPOptionsDestroyLocalCopy()\n");

#endif /// DBG

   return;
}

/**
 @kernel_helper_function="KrnlHlprIPsecDoSPOptionsPopulateLocalCopy"
 
   Purpose:  Allocate and populate a local copy of an IPSEC_DOSP_OPTIONS.                       <br>
                                                                                                <br>
   Notes:    The caller is responsible for freeing any allocated memory using
             KrnlHlprIPsecDoSPOptionsDestroyLocalCopy().                                        <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/DD744994.aspx              <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Check_return_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprIPsecDoSPOptionsPopulateLocalCopy(_In_ const IPSEC_DOSP_OPTIONS* pOriginalOptions,
                                                   _Inout_ IPSEC_DOSP_OPTIONS* pOptions)
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprIPsecDoSPOptionsPopulateLocalCopy()\n");

#endif /// DBG
   
   NT_ASSERT(pOriginalOptions);
   NT_ASSERT(pOptions);

   NTSTATUS status = STATUS_SUCCESS;

   RtlCopyMemory(pOptions,
                 pOriginalOptions,
                 sizeof(IPSEC_DOSP_OPTIONS));

   pOptions->publicIFLuids   = 0;
   pOptions->internalIFLuids = 0;

   if(pOptions->numPublicIFLuids)
   {
      HLPR_NEW_ARRAY(pOptions->publicIFLuids,
                     UINT64,
                     pOptions->numPublicIFLuids,
                     WFPSAMPLER_SYSLIB_TAG);
      HLPR_BAIL_ON_ALLOC_FAILURE(pOptions->publicIFLuids,
                                 status);
   }

   if(pOptions->numInternalIFLuids)
   {
      HLPR_NEW_ARRAY(pOptions->internalIFLuids,
                     UINT64,
                     pOptions->numInternalIFLuids,
                     WFPSAMPLER_SYSLIB_TAG);
      HLPR_BAIL_ON_ALLOC_FAILURE(pOptions->internalIFLuids,
                                 status);
   }

   HLPR_BAIL_LABEL:

   if(status != STATUS_SUCCESS)
      KrnlHlprIPsecDoSPOptionsPurgeLocalCopy(pOptions);

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprIPsecDoSPOptionsPopulateLocalCopy() [status: %#x]\n",
              status);

#endif /// DBG

   return status;

}

/**
 @kernel_helper_function="KrnlHlprIPsecDoSPOptionsCreateLocalCopy"
 
   Purpose:  Allocate and populate a local copy of an IPSEC_DOSP_OPTIONS.                       <br>
                                                                                                <br>
   Notes:    The caller is responsible for freeing any allocated memory using
             KrnlHlprIPsecDoSPOptionsDestroyLocalCopy().                                        <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/DD744994.aspx              <br>
*/
__drv_allocatesMem(Pool)
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Must_inspect_result_
_Success_(return != 0)
IPSEC_DOSP_OPTIONS* KrnlHlprIPsecDoSPOptionsCreateLocalCopy(_In_ const IPSEC_DOSP_OPTIONS* pOriginalOptions)
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprIPsecDoSPOptionsCreateLocalCopy()\n");

#endif /// DBG
   
   NT_ASSERT(pOriginalOptions);

   NTSTATUS            status   = STATUS_SUCCESS;
   IPSEC_DOSP_OPTIONS* pOptions = 0;

#pragma warning(push)
#pragma warning(disable: 6014) /// pOptions is expected to be cleaned up by caller using KrnlHlprIPsecDoSPOptionsPopulateLocalCopy

   HLPR_NEW(pOptions,
            IPSEC_DOSP_OPTIONS,
            WFPSAMPLER_SYSLIB_TAG);
   HLPR_BAIL_ON_ALLOC_FAILURE(pOptions,
                              status);

#pragma warning(pop)

   status = KrnlHlprIPsecDoSPOptionsPopulateLocalCopy(pOriginalOptions,
                                                      pOptions);

   HLPR_BAIL_LABEL:

#pragma warning(push)
#pragma warning(disable: 6001) /// pOptions initialized with call to HLPR_NEW & KrnlHlprIPsecDoSPOptionsPopulateLocalCopy 

   if(status != STATUS_SUCCESS &&
      pOptions)
      KrnlHlprIPsecDoSPOptionsDestroyLocalCopy(&pOptions);

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprIPsecDoSPOptionsCreateLocalCopy() [pOptions: %#p]\n",
              pOptions);

#endif /// DBG

   return pOptions;

#pragma warning(pop)
}

#endif /// (NTDDI_VERSION >= NTDDI_WIN7)

#endif /// IPSEC_DOSP_OPTIONS____

#ifndef FWP_TOKEN_INFORMATION____
#define FWP_TOKEN_INFORMATION____

/**
 @kernel_helper_function="KrnlHlprFwpTokenInformationPurgeLocalCopy"
 
   Purpose:  Cleanup a local copy of an FWP_TOKEN_INFORMATION.                                  <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF552440.aspx             <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
inline VOID KrnlHlprFwpTokenInformationPurgeLocalCopy(_Inout_ FWP_TOKEN_INFORMATION* pTokenInfo)
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprFwpTokenInformationPurgeLocalCopy()\n");

#endif /// DBG
   
   NT_ASSERT(pTokenInfo);

   for(UINT32 i = 0;
       i < pTokenInfo->sidCount;
       i++)
   {
      HLPR_DELETE_ARRAY(pTokenInfo->sids[i].Sid,
                        WFPSAMPLER_SYSLIB_TAG);
   }

   HLPR_DELETE_ARRAY(pTokenInfo->sids,
                     WFPSAMPLER_SYSLIB_TAG);

   for(UINT32 i = 0;
       i < pTokenInfo->restrictedSidCount;
       i++)
   {
      HLPR_DELETE_ARRAY(pTokenInfo->restrictedSids[i].Sid,
                        WFPSAMPLER_SYSLIB_TAG);
   }

   HLPR_DELETE_ARRAY(pTokenInfo->restrictedSids,
                     WFPSAMPLER_SYSLIB_TAG);

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprFwpTokenInformationPurgeLocalCopy()\n");

#endif /// DBG

   return;
}

/**
 @kernel_helper_function="KrnlHlprFwpTokenInformationDestroyLocalCopy"
 
   Purpose:  Cleanup and free a local copy of an FWP_TOKEN_INFORMATION.                         <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF552440.aspx             <br>
*/
_At_(*ppTokenInfo, _Pre_ _Notnull_)
_At_(*ppTokenInfo, _Post_ _Null_ __drv_freesMem(Pool))
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Success_(*ppTokenInfo == 0)
inline VOID KrnlHlprFwpTokenInformationDestroyLocalCopy(_Inout_ FWP_TOKEN_INFORMATION** ppTokenInfo)
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprFwpTokenInformationDestroyLocalCopy()\n");

#endif /// DBG
   
   NT_ASSERT(ppTokenInfo);

   if(*ppTokenInfo)
      KrnlHlprFwpTokenInformationPurgeLocalCopy(*ppTokenInfo);

   HLPR_DELETE(*ppTokenInfo,
               WFPSAMPLER_SYSLIB_TAG);

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprFwpTokenInformationDestroyLocalCopy()\n");

#endif /// DBG

   return;
}

/**
 @kernel_helper_function="KrnlHlprFwpTokenInformationPopulateLocalCopy"
 
   Purpose:  Populate a local copy of an FWP_TOKEN_INFORMATION.                                 <br>
                                                                                                <br>
   Notes:    The caller is responsible for freeing the allocated memory using
             KrnlHlprFwpTokenInformationPurgeLocalCopy().                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF552440.aspx             <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Check_return_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprFwpTokenInformationPopulateLocalCopy(_In_ const FWP_TOKEN_INFORMATION* pOriginalTokenInfo,
                                                      _Inout_ FWP_TOKEN_INFORMATION* pTokenInfo)
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprFwpTokenInformationPopulateLocalCopy()\n");

#endif /// DBG
   
   NT_ASSERT(pOriginalTokenInfo);
   NT_ASSERT(pTokenInfo);

   NTSTATUS status = STATUS_SUCCESS;
   KIRQL    irql   = KeGetCurrentIrql();

   HLPR_NEW_ARRAY(pTokenInfo->sids,
                  SID_AND_ATTRIBUTES,
                  pOriginalTokenInfo->sidCount,
                  WFPSAMPLER_SYSLIB_TAG);
   HLPR_BAIL_ON_ALLOC_FAILURE(pTokenInfo->sids,
                              status);

   pTokenInfo->sidCount = pOriginalTokenInfo->sidCount;

   for(UINT32 i = 0;
       i < pTokenInfo->sidCount;
       i++)
   {
      UINT32 sidSize = 0;

      if(irql < DISPATCH_LEVEL)
      {
         if(RtlValidSid(pOriginalTokenInfo->sids[i].Sid))
         {
            sidSize = RtlLengthSid(pOriginalTokenInfo->sids[i].Sid);
            if(sidSize)
            {
               HLPR_NEW_ARRAY(pTokenInfo->sids[i].Sid,
                              BYTE,
                              sidSize,
                              WFPSAMPLER_SYSLIB_TAG);
               HLPR_BAIL_ON_ALLOC_FAILURE(pTokenInfo->sids[i].Sid,
                                          status);

               status = RtlCopySid(sidSize,
                                   pTokenInfo->sids[i].Sid,
                                   pOriginalTokenInfo->sids[i].Sid);
               HLPR_BAIL_ON_FAILURE(status);
            }
            else
            {
               status = STATUS_INVALID_BUFFER_SIZE;

               HLPR_BAIL;
            }
         }
         else
         {
            status = STATUS_INVALID_SID;

            HLPR_BAIL;
         }
      }
      else
      {
         sidSize = SID_LENGTH(pOriginalTokenInfo->sids[i].Sid);
         if(sidSize)
         {
            HLPR_NEW_ARRAY(pTokenInfo->sids[i].Sid,
                           BYTE,
                           sidSize,
                           WFPSAMPLER_SYSLIB_TAG);
            HLPR_BAIL_ON_ALLOC_FAILURE(pTokenInfo->sids[i].Sid,
                                       status);

            RtlCopyMemory(pTokenInfo->sids[i].Sid,
                          pOriginalTokenInfo->sids[i].Sid,
                          sidSize);
         }
         else
         {
            status = STATUS_INVALID_BUFFER_SIZE;

            HLPR_BAIL;
         }
      }

      pTokenInfo->sids[i].Attributes = pTokenInfo->sids[i].Attributes;
   }

   HLPR_NEW_ARRAY(pTokenInfo->restrictedSids,
                  SID_AND_ATTRIBUTES,
                  pOriginalTokenInfo->restrictedSidCount,
                  WFPSAMPLER_SYSLIB_TAG);
   HLPR_BAIL_ON_ALLOC_FAILURE(pTokenInfo->restrictedSids,
                              status);

   pTokenInfo->restrictedSidCount = pOriginalTokenInfo->restrictedSidCount;

   for(UINT32 i = 0;
       i < pTokenInfo->restrictedSidCount;
       i++)
   {
      UINT32 sidSize = 0;

      if(irql < DISPATCH_LEVEL)
      {
#pragma warning(push)
#pragma warning(disable: 28118) /// IRQL check has already been performed

         if(RtlValidSid(pOriginalTokenInfo->restrictedSids[i].Sid))
         {

#pragma warning(pop)

            sidSize = RtlLengthSid(pOriginalTokenInfo->restrictedSids[i].Sid);
            if(sidSize)
            {
               HLPR_NEW_ARRAY(pTokenInfo->restrictedSids[i].Sid,
                              BYTE,
                              sidSize,
                              WFPSAMPLER_SYSLIB_TAG);
               HLPR_BAIL_ON_ALLOC_FAILURE(pTokenInfo->restrictedSids[i].Sid,
                                          status);

               status = RtlCopySid(sidSize,
                                   pTokenInfo->restrictedSids[i].Sid,
                                   pOriginalTokenInfo->restrictedSids[i].Sid);
               HLPR_BAIL_ON_FAILURE(status);
            }
            else
            {
               status = STATUS_INVALID_BUFFER_SIZE;

               HLPR_BAIL;
            }
         }
         else
         {
            status = STATUS_INVALID_SID;

            HLPR_BAIL;
         }
      }
      else
      {
         sidSize = SID_LENGTH(pOriginalTokenInfo->restrictedSids[i].Sid);
         if(sidSize)
         {
            HLPR_NEW_ARRAY(pTokenInfo->restrictedSids[i].Sid,
                           BYTE,
                           sidSize,
                           WFPSAMPLER_SYSLIB_TAG);
            HLPR_BAIL_ON_ALLOC_FAILURE(pTokenInfo->restrictedSids[i].Sid,
                                       status);

            RtlCopyMemory(pTokenInfo->restrictedSids[i].Sid,
                          pOriginalTokenInfo->restrictedSids[i].Sid,
                          sidSize);
         }
         else
         {
            status = STATUS_INVALID_BUFFER_SIZE;

            HLPR_BAIL;
         }
      }

      pTokenInfo->restrictedSids[i].Attributes = pTokenInfo->restrictedSids[i].Attributes;
   }

   HLPR_BAIL_LABEL:

   if(status != STATUS_SUCCESS)
      KrnlHlprFwpTokenInformationDestroyLocalCopy(&pTokenInfo);

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprFwpTokenInformationPopulateLocalCopy() [status: %#x]\n",
              status);

#endif /// DBG

   return status;
}

/**
 @kernel_helper_function="KrnlHlprFwpTokenInformationCreateLocalCopy"
 
   Purpose:  Allocate and populate a local copy of an FWP_TOKEN_INFORMATION.                    <br>
                                                                                                <br>
   Notes:    The caller is responsible for freeing the allocated memory using
             KrnlHlprFwpTokenInformationDestroyLocalCopy().                                     <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF552440.aspx             <br>
*/
__drv_allocatesMem(Pool)
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Must_inspect_result_
_Success_(return != 0)
FWP_TOKEN_INFORMATION* KrnlHlprFwpTokenInformationCreateLocalCopy(_In_ const FWP_TOKEN_INFORMATION* pOriginalTokenInfo)
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprFwpTokenInformationCreateLocalCopy()\n");

#endif /// DBG
   
   NT_ASSERT(pOriginalTokenInfo);

   NTSTATUS               status     = STATUS_SUCCESS;
   FWP_TOKEN_INFORMATION* pTokenInfo = 0;

#pragma warning(push)
#pragma warning(disable: 6014) /// pTokenInfo is expected to be cleaned up by caller using KrnlHlprFwpTokenInformationDestroyLocalCopy

   HLPR_NEW(pTokenInfo,
            FWP_TOKEN_INFORMATION,
            WFPSAMPLER_SYSLIB_TAG);
   HLPR_BAIL_ON_ALLOC_FAILURE(pTokenInfo,
                              status);

#pragma warning(pop)

   status = KrnlHlprFwpTokenInformationPopulateLocalCopy(pOriginalTokenInfo,
                                                         pTokenInfo);

   HLPR_BAIL_LABEL:

#pragma warning(push)
#pragma warning(disable: 6001) /// pTokenInfo initialized with call to HLPR_NEW & KrnlHlprFwpTokenInformationPopulateLocalCopy 

   if(status != STATUS_SUCCESS &&
      pTokenInfo)
      KrnlHlprFwpTokenInformationDestroyLocalCopy(&pTokenInfo);

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprFwpTokenInformationCreateLocalCopy() [pTokenInfo: %#p]\n",
              pTokenInfo);

#endif /// DBG

   return pTokenInfo;

#pragma warning(pop)
}

#endif /// FWP_TOKEN_INFORMATION____

#ifndef FWP_VALUE____
#define FWP_VALUE____

/**
 @kernel_helper_function="KrnlHlprFwpValueGetStringForFwpsIncomingValue"
 
   Purpose:  Return a string representing the condition field of the FWPS_INCOMING_VALUE.       <br> 
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF552450.aspx             <br>
             HTTP://MSDN.Microsoft.com/En-US/Library/Windwos/Hardware/FF549939.aspx             <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Success_(return != 0)
PSTR KrnlHlprFwpValueGetStringForFwpsIncomingValue(_In_ UINT32 layerID,
                                                   _In_ UINT32 fieldID,
                                                   _Out_opt_ BOOLEAN* pFormat) /* 0 */
{
   PSTR pString = 0;

   if(pFormat)
      *pFormat = FALSE;

   switch(layerID)
   {
      case FWPS_LAYER_INBOUND_IPPACKET_V4:
      case FWPS_LAYER_INBOUND_IPPACKET_V4_DISCARD:
      case FWPS_LAYER_INBOUND_IPPACKET_V6:
      case FWPS_LAYER_INBOUND_IPPACKET_V6_DISCARD:
      {
         if(fieldID == FWPS_FIELD_INBOUND_IPPACKET_V4_IP_LOCAL_ADDRESS)
         {
            pString = "FWPM_CONDITION_IP_LOCAL_ADDRESS";

            if(pFormat)
               *pFormat = ADDRESS_VALUE;
         }
         else if(fieldID == FWPS_FIELD_INBOUND_IPPACKET_V4_IP_REMOTE_ADDRESS)
         {
            pString = "FWPM_CONDITION_IP_REMOTE_ADDRESS";

            if(pFormat)
               *pFormat = ADDRESS_VALUE;
         }
         else if(fieldID == FWPS_FIELD_INBOUND_IPPACKET_V4_IP_LOCAL_ADDRESS_TYPE)
            pString = "FWPM_CONDITION_IP_LOCAL_ADDRESS_TYPE";
         else if(fieldID == FWPS_FIELD_INBOUND_IPPACKET_V4_IP_LOCAL_INTERFACE)
            pString = "FWPM_CONDITION_IP_LOCAL_INTERFACE";
         else if(fieldID == FWPS_FIELD_INBOUND_IPPACKET_V4_INTERFACE_INDEX)
            pString = "FWPM_CONDITION_INTERFACE_INDEX";
         else if(fieldID == FWPS_FIELD_INBOUND_IPPACKET_V4_SUB_INTERFACE_INDEX)
            pString = "FWPM_CONDITION_SUB_INTERFACE_INDEX";
         else if(fieldID == FWPS_FIELD_INBOUND_IPPACKET_V4_FLAGS)
         {
            pString = "FWPM_CONDITION_FLAGS";

            if(pFormat)
               *pFormat = FLAGS_VALUE;
         }
         else if(fieldID == FWPS_FIELD_INBOUND_IPPACKET_V4_INTERFACE_TYPE)
            pString = "FWPM_CONDITION_INTERFACE_TYPE";
         else if(fieldID == FWPS_FIELD_INBOUND_IPPACKET_V4_TUNNEL_TYPE)
            pString = "FWPM_CONDITION_TUNNEL_TYPE";

         break;
      }
      case FWPS_LAYER_OUTBOUND_IPPACKET_V4:
      case FWPS_LAYER_OUTBOUND_IPPACKET_V4_DISCARD:
      case FWPS_LAYER_OUTBOUND_IPPACKET_V6:
      case FWPS_LAYER_OUTBOUND_IPPACKET_V6_DISCARD:
      {
         if(fieldID == FWPS_FIELD_OUTBOUND_IPPACKET_V4_IP_LOCAL_ADDRESS)
         {
            pString = "FWPM_CONDITION_IP_LOCAL_ADDRESS";

            if(pFormat)
               *pFormat = ADDRESS_VALUE;
         }
         else if(fieldID == FWPS_FIELD_OUTBOUND_IPPACKET_V4_IP_LOCAL_ADDRESS_TYPE)
            pString = "FWPM_CONDITION_IP_LOCAL_ADDRESS_TYPE";
         else if(fieldID == FWPS_FIELD_OUTBOUND_IPPACKET_V4_IP_REMOTE_ADDRESS)
         {
            pString = "FWPM_CONDITION_IP_REMOTE_ADDRESS";

            if(pFormat)
               *pFormat = ADDRESS_VALUE;
         }
         else if(fieldID == FWPS_FIELD_OUTBOUND_IPPACKET_V4_IP_LOCAL_INTERFACE)
            pString = "FWPM_CONDITION_IP_LOCAL_INTERFACE";
         else if(fieldID == FWPS_FIELD_OUTBOUND_IPPACKET_V4_INTERFACE_INDEX)
            pString = "FWPM_CONDITION_INTERFACE_INDEX";
         else if(fieldID == FWPS_FIELD_OUTBOUND_IPPACKET_V4_SUB_INTERFACE_INDEX)
            pString = "FWPM_CONDITION_SUB_INTERFACE_INDEX";
         else if(fieldID == FWPS_FIELD_OUTBOUND_IPPACKET_V4_FLAGS)
         {
            pString = "FWPM_CONDITION_FLAGS";

            if(pFormat)
               *pFormat = FLAGS_VALUE;
         }
         else if(fieldID == FWPS_FIELD_OUTBOUND_IPPACKET_V4_INTERFACE_TYPE)
            pString = "FWPM_CONDITION_INTERFACE_TYPE";
         else if(fieldID == FWPS_FIELD_OUTBOUND_IPPACKET_V4_TUNNEL_TYPE)
            pString = "FWPM_CONDITION_TUNNEL_TYPE";

         break;
      }
      case FWPS_LAYER_IPFORWARD_V4:
      case FWPS_LAYER_IPFORWARD_V4_DISCARD:
      case FWPS_LAYER_IPFORWARD_V6:
      case FWPS_LAYER_IPFORWARD_V6_DISCARD:
      {
         if(fieldID == FWPS_FIELD_IPFORWARD_V4_IP_SOURCE_ADDRESS)
         {
            pString = "FWPM_CONDITION_IP_SOURCE_ADDRESS";

            if(pFormat)
               *pFormat = ADDRESS_VALUE;
         }
         else if(fieldID == FWPS_FIELD_IPFORWARD_V4_IP_DESTINATION_ADDRESS)
         {
            pString = "FWPM_CONDITION_IP_DESTINATION_ADDRESS";

            if(pFormat)
               *pFormat = ADDRESS_VALUE;
         }
         else if(fieldID == FWPS_FIELD_IPFORWARD_V4_IP_DESTINATION_ADDRESS_TYPE)
            pString = "FWPM_CONDITION_IP_DESTINATION_ADDRESS_TYPE";
         else if(fieldID == FWPS_FIELD_IPFORWARD_V4_IP_LOCAL_INTERFACE)
            pString = "FWPM_CONDITION_IP_LOCAL_INTERFACE";
         else if(fieldID == FWPS_FIELD_IPFORWARD_V4_IP_FORWARD_INTERFACE)
            pString = "FWPM_CONDITION_IP_FORWARD_INTERFACE";
         else if(fieldID == FWPS_FIELD_IPFORWARD_V4_SOURCE_INTERFACE_INDEX)
            pString = "FWPM_CONDITION_SOURCE_INTERFACE_INDEX";
         else if(fieldID == FWPS_FIELD_IPFORWARD_V4_SOURCE_SUB_INTERFACE_INDEX)
            pString = "FWPM_CONDITION_SOURCE_SUB_INTERFACE_INDEX";
         else if(fieldID == FWPS_FIELD_IPFORWARD_V4_DESTINATION_INTERFACE_INDEX)
            pString = "FWPM_CONDITION_DESTINATION_INTERFACE_INDEX";
         else if(fieldID == FWPS_FIELD_IPFORWARD_V4_DESTINATION_SUB_INTERFACE_INDEX)
            pString = "FWPM_CONDITION_DESTINATION_SUB_INTERFACE_INDEX";
         else if(fieldID == FWPS_FIELD_IPFORWARD_V4_FLAGS)
         {
            pString = "FWPM_CONDITION_FLAGS";

            if(pFormat)
               *pFormat = FLAGS_VALUE;
         }

#if(NTDDI_VERSION >= NTDDI_WIN7)

         else if(fieldID == FWPS_FIELD_IPFORWARD_V4_IP_PHYSICAL_ARRIVAL_INTERFACE)
            pString = "FWPM_CONDITION_IP_PHYSICAL_ARRIVAL_INTERFACE";
         else if(fieldID == FWPS_FIELD_IPFORWARD_V4_ARRIVAL_INTERFACE_PROFILE_ID)
            pString = "FWPM_CONDITION_ARRIVAL_INTERFACE_PROFILE_ID";
         else if(fieldID == FWPS_FIELD_IPFORWARD_V4_IP_PHYSICAL_NEXTHOP_INTERFACE)
            pString = "FWPM_CONDITION_IP_PHYSICAL_NEXTHOP_INTERFACE";
         else if(fieldID == FWPS_FIELD_IPFORWARD_V4_NEXTHOP_INTERFACE_PROFILE_ID)
            pString = "FWPM_CONDITION_NEXTHOP_INTERFACE_PROFILE_ID";

#endif /// (NTDDI_VERSION >= NTDDI_WIN7)

         break;
      }
      case FWPS_LAYER_INBOUND_TRANSPORT_V4:
      case FWPS_LAYER_INBOUND_TRANSPORT_V4_DISCARD:
      case FWPS_LAYER_INBOUND_TRANSPORT_V6:
      case FWPS_LAYER_INBOUND_TRANSPORT_V6_DISCARD:
      {
         if(fieldID == FWPS_FIELD_INBOUND_TRANSPORT_V4_IP_PROTOCOL)
            pString = "FWPM_CONDITION_IP_PROTOCOL";
         else if(fieldID == FWPS_FIELD_INBOUND_TRANSPORT_V4_IP_LOCAL_ADDRESS)
         {
            pString = "FWPM_CONDITION_IP_LOCAL_ADDRESS";

            if(pFormat)
               *pFormat = ADDRESS_VALUE;
         }
         else if(fieldID == FWPS_FIELD_INBOUND_TRANSPORT_V4_IP_REMOTE_ADDRESS)
         {
            pString = "FWPM_CONDITION_IP_REMOTE_ADDRESS";

            if(pFormat)
               *pFormat = ADDRESS_VALUE;
         }
         else if(fieldID == FWPS_FIELD_INBOUND_TRANSPORT_V4_IP_LOCAL_ADDRESS_TYPE)
            pString = "FWPM_CONDITION_IP_LOCAL_ADDRESS_TYPE";
         else if(fieldID == FWPS_FIELD_INBOUND_TRANSPORT_V4_IP_LOCAL_PORT)
            pString = "FWPM_CONDITION_IP_LOCAL_PORT";
         else if(fieldID == FWPS_FIELD_INBOUND_TRANSPORT_V4_IP_REMOTE_PORT)
            pString = "FWPM_CONDITION_IP_REMOTE_PORT";
         else if(fieldID == FWPS_FIELD_INBOUND_TRANSPORT_V4_IP_LOCAL_INTERFACE)
            pString = "FWPM_CONDITION_IP_LOCAL_INTERFACE";
         else if(fieldID == FWPS_FIELD_INBOUND_TRANSPORT_V4_INTERFACE_INDEX)
            pString = "FWPM_CONDITION_INTERFACE_INDEX";
         else if(fieldID == FWPS_FIELD_INBOUND_TRANSPORT_V4_SUB_INTERFACE_INDEX)
            pString = "FWPM_CONDITION_SUB_INTERFACE_INDEX";
         else if(fieldID == FWPS_FIELD_INBOUND_TRANSPORT_V4_FLAGS)
         {
            pString = "FWPM_CONDITION_FLAGS";

            if(pFormat)
               *pFormat = FLAGS_VALUE;
         }
         else if(fieldID == FWPS_FIELD_INBOUND_TRANSPORT_V4_INTERFACE_TYPE)
            pString = "FWPM_CONDITION_INTERFACE_TYPE";
         else if(fieldID == FWPS_FIELD_INBOUND_TRANSPORT_V4_TUNNEL_TYPE)
            pString = "FWPM_CONDITION_TUNNEL_TYPE";

#if(NTDDI_VERSION >= NTDDI_WIN7)

         else if(fieldID == FWPS_FIELD_INBOUND_TRANSPORT_V4_PROFILE_ID)
            pString = "FWPM_CONDITION_PROFILE_ID";

#if(NTDDI_VERSION >= NTDDI_WINTHRESHOLD)
         
         else if(fieldID == FWPS_FIELD_INBOUND_TRANSPORT_V4_IPSEC_SECURITY_REALM_ID)
            pString = "FWPM_CONDITION_IPSEC_SECURITY_REALM_ID";
         
#endif /// (NTDDI_VERSION >= NTDDI_WINTHRESHOLD)

#endif /// (NTDDI_VERSION >= NTDDI_WIN7)

         break;
      }
      case FWPS_LAYER_OUTBOUND_TRANSPORT_V4:
      case FWPS_LAYER_OUTBOUND_TRANSPORT_V4_DISCARD:
      case FWPS_LAYER_OUTBOUND_TRANSPORT_V6:
      case FWPS_LAYER_OUTBOUND_TRANSPORT_V6_DISCARD:
      {
         if(fieldID == FWPS_FIELD_OUTBOUND_TRANSPORT_V4_IP_PROTOCOL)
            pString = "FWPM_CONDITION_IP_PROTOCOL";
         else if(fieldID == FWPS_FIELD_OUTBOUND_TRANSPORT_V4_IP_LOCAL_ADDRESS)
         {
            pString = "FWPM_CONDITION_IP_LOCAL_ADDRESS";

            if(pFormat)
               *pFormat = ADDRESS_VALUE;
         }
         else if(fieldID == FWPS_FIELD_OUTBOUND_TRANSPORT_V4_IP_LOCAL_ADDRESS_TYPE)
            pString = "FWPM_CONDITION_IP_LOCAL_ADDRESS_TYPE";
         else if(fieldID == FWPS_FIELD_OUTBOUND_TRANSPORT_V4_IP_REMOTE_ADDRESS)
         {
            pString = "FWPM_CONDITION_IP_REMOTE_ADDRESS";

            if(pFormat)
               *pFormat = ADDRESS_VALUE;
         }
         else if(fieldID == FWPS_FIELD_OUTBOUND_TRANSPORT_V4_IP_LOCAL_PORT)
            pString = "FWPM_CONDITION_IP_LOCAL_PORT";
         else if(fieldID == FWPS_FIELD_OUTBOUND_TRANSPORT_V4_IP_REMOTE_PORT)
            pString = "FWPM_CONDITION_IP_REMOTE_PORT";
         else if(fieldID == FWPS_FIELD_OUTBOUND_TRANSPORT_V4_IP_LOCAL_INTERFACE)
            pString = "FWPM_CONDITION_IP_LOCAL_INTERFACE";
         else if(fieldID == FWPS_FIELD_OUTBOUND_TRANSPORT_V4_INTERFACE_INDEX)
            pString = "FWPM_CONDITION_INTERFACE_INDEX";
         else if(fieldID == FWPS_FIELD_OUTBOUND_TRANSPORT_V4_SUB_INTERFACE_INDEX)
            pString = "FWPM_CONDITION_SUB_INTERFACE_INDEX";
         else if(fieldID == FWPS_FIELD_OUTBOUND_TRANSPORT_V4_IP_DESTINATION_ADDRESS_TYPE)
            pString = "FWPM_CONDITION_IP_DESTINATION_ADDRESS_TYPE";
         else if(fieldID == FWPS_FIELD_OUTBOUND_TRANSPORT_V4_FLAGS)
         {
            pString = "FWPM_CONDITION_FLAGS";

            if(pFormat)
               *pFormat = FLAGS_VALUE;
         }
         else if(fieldID == FWPS_FIELD_OUTBOUND_TRANSPORT_V4_INTERFACE_TYPE)
            pString = "FWPM_CONDITION_INTERFACE_TYPE";
         else if(fieldID == FWPS_FIELD_OUTBOUND_TRANSPORT_V4_TUNNEL_TYPE)
            pString = "FWPM_CONDITION_TUNNEL_TYPE";

#if(NTDDI_VERSION >= NTDDI_WIN7)

         else if(fieldID == FWPS_FIELD_OUTBOUND_TRANSPORT_V4_PROFILE_ID)
            pString = "FWPM_CONDITION_PROFILE_ID";

#if(NTDDI_VERSION >= NTDDI_WINTHRESHOLD)

         else if(fieldID == FWPS_FIELD_OUTBOUND_TRANSPORT_V4_IPSEC_SECURITY_REALM_ID)
            pString = "FWPM_CONDITION_IPSEC_SECURITY_REALM_ID";

#endif /// (NTDDI_VERSION >= NTDDI_WINTHRESHOLD)
#endif /// (NTDDI_VERSION >= NTDDI_WIN7)

         break;
      }
      case FWPS_LAYER_STREAM_V4:
      case FWPS_LAYER_STREAM_V4_DISCARD:
      case FWPS_LAYER_STREAM_V6:
      case FWPS_LAYER_STREAM_V6_DISCARD:
      {
         if(fieldID == FWPS_FIELD_STREAM_V4_IP_LOCAL_ADDRESS)
         {
            pString = "FWPM_CONDITION_IP_LOCAL_ADDRESS";

            if(pFormat)
               *pFormat = ADDRESS_VALUE;
         }
         else if(fieldID == FWPS_FIELD_STREAM_V4_IP_LOCAL_ADDRESS_TYPE)
            pString = "FWPM_CONDITION_IP_LOCAL_ADDRESS_TYPE";
         else if(fieldID == FWPS_FIELD_STREAM_V4_IP_REMOTE_ADDRESS)
         {
            pString = "FWPM_CONDITION_IP_REMOTE_ADDRESS";

            if(pFormat)
               *pFormat = ADDRESS_VALUE;
         }
         else if(fieldID == FWPS_FIELD_STREAM_V4_IP_LOCAL_PORT)
            pString = "FWPM_CONDITION_IP_LOCAL_PORT";
         else if(fieldID == FWPS_FIELD_STREAM_V4_IP_REMOTE_PORT)
            pString = "FWPM_CONDITION_IP_REMOTE_PORT";
         else if(fieldID == FWPS_FIELD_STREAM_V4_DIRECTION)
            pString = "FWPM_CONDITION_DIRECTION";

#if(NTDDI_VERSION >= NTDDI_VISTASP1)

         else if(fieldID == FWPS_FIELD_STREAM_V4_FLAGS)
         {
            pString = "FWPM_CONDITION_FLAGS";

            if(pFormat)
               *pFormat = FLAGS_VALUE;
         }

#endif /// (NTDDI_VERSION >= NTDDI_VISTASP1)

         break;
      }
      case FWPS_LAYER_DATAGRAM_DATA_V4:
      case FWPS_LAYER_DATAGRAM_DATA_V4_DISCARD:
      case FWPS_LAYER_DATAGRAM_DATA_V6:
      case FWPS_LAYER_DATAGRAM_DATA_V6_DISCARD:
      {
         if(fieldID == FWPS_FIELD_DATAGRAM_DATA_V4_IP_PROTOCOL)
            pString = "FWPM_CONDITION_IP_PROTOCOL";
         else if(fieldID == FWPS_FIELD_DATAGRAM_DATA_V4_IP_LOCAL_ADDRESS)
         {
            pString = "FWPM_CONDITION_IP_LOCAL_ADDRESS";

            if(pFormat)
               *pFormat = ADDRESS_VALUE;
         }
         else if(fieldID == FWPS_FIELD_DATAGRAM_DATA_V4_IP_REMOTE_ADDRESS)
         {
            pString = "FWPM_CONDITION_IP_REMOTE_ADDRESS";

            if(pFormat)
               *pFormat = ADDRESS_VALUE;
         }
         else if(fieldID == FWPS_FIELD_DATAGRAM_DATA_V4_IP_LOCAL_ADDRESS_TYPE)
            pString = "FWPM_CONDITION_IP_LOCAL_ADDRESS_TYPE";
         else if(fieldID == FWPS_FIELD_DATAGRAM_DATA_V4_IP_LOCAL_PORT)
            pString = "FWPM_CONDITION_IP_LOCAL_PORT";
         else if(fieldID == FWPS_FIELD_DATAGRAM_DATA_V4_IP_REMOTE_PORT)
            pString = "FWPM_CONDITION_IP_REMOTE_PORT";
         else if(fieldID == FWPS_FIELD_DATAGRAM_DATA_V4_IP_LOCAL_INTERFACE)
            pString = "FWPM_CONDITION_IP_LOCAL_INTERFACE";
         else if(fieldID == FWPS_FIELD_DATAGRAM_DATA_V4_INTERFACE_INDEX)
            pString = "FWPM_CONDITION_INTERFACE_INDEX";
         else if(fieldID == FWPS_FIELD_DATAGRAM_DATA_V4_SUB_INTERFACE_INDEX)
            pString = "FWPM_CONDITION_SUB_INTERFACE_INDEX";
         else if(fieldID == FWPS_FIELD_DATAGRAM_DATA_V4_DIRECTION)
            pString = "FWPM_CONDITION_DIRECTION";
         else if(fieldID == FWPS_FIELD_DATAGRAM_DATA_V4_FLAGS)
         {
            pString = "FWPM_CONDITION_FLAGS";

            if(pFormat)
               *pFormat = FLAGS_VALUE;
         }
         else if(fieldID == FWPS_FIELD_DATAGRAM_DATA_V4_INTERFACE_TYPE)
            pString = "FWPM_CONDITION_INTERFACE_TYPE";
         else if(fieldID == FWPS_FIELD_DATAGRAM_DATA_V4_TUNNEL_TYPE)
            pString = "FWPM_CONDITION_TUNNEL_TYPE";

         break;
      }
      case FWPS_LAYER_INBOUND_ICMP_ERROR_V4:
      case FWPS_LAYER_INBOUND_ICMP_ERROR_V4_DISCARD:
      case FWPS_LAYER_INBOUND_ICMP_ERROR_V6:
      case FWPS_LAYER_INBOUND_ICMP_ERROR_V6_DISCARD:
      {
         if(fieldID == FWPS_FIELD_INBOUND_ICMP_ERROR_V4_EMBEDDED_PROTOCOL)
            pString = "FWPM_CONDITION_EMBEDDED_PROTOCOL";
         else if(fieldID == FWPS_FIELD_INBOUND_ICMP_ERROR_V4_IP_LOCAL_ADDRESS)
         {
            pString = "FWPM_CONDITION_IP_LOCAL_ADDRESS";

            if(pFormat)
               *pFormat = ADDRESS_VALUE;
         }
         else if(fieldID == FWPS_FIELD_INBOUND_ICMP_ERROR_V4_IP_REMOTE_ADDRESS)
         {
            pString = "FWPM_CONDITION_IP_REMOTE_ADDRESS";

            if(pFormat)
               *pFormat = ADDRESS_VALUE;
         }
         else if(fieldID == FWPS_FIELD_INBOUND_ICMP_ERROR_V4_EMBEDDED_REMOTE_ADDRESS)
            pString = "FWPM_CONDITION_EMBEDDED_REMOTE_ADDRESS";
         else if(fieldID == FWPS_FIELD_INBOUND_ICMP_ERROR_V4_EMBEDDED_LOCAL_ADDRESS_TYPE)
            pString = "FWPM_CONDITION_EMBEDDED_LOCAL_ADDRESS_TYPE";
         else if(fieldID == FWPS_FIELD_INBOUND_ICMP_ERROR_V4_EMBEDDED_LOCAL_PORT)
            pString = "FWPM_CONDITION_EMBEDDED_LOCAL_PORT";
         else if(fieldID == FWPS_FIELD_INBOUND_ICMP_ERROR_V4_EMBEDDED_REMOTE_PORT)
            pString = "FWPM_CONDITION_EMBEDDED_REMOTE_PORT";
         else if(fieldID == FWPS_FIELD_INBOUND_ICMP_ERROR_V4_IP_LOCAL_INTERFACE)
            pString = "FWPM_CONDITION_IP_LOCAL_INTERFACE";
         else if(fieldID == FWPS_FIELD_INBOUND_ICMP_ERROR_V4_ICMP_TYPE)
            pString = "FWPM_CONDITION_ICMP_TYPE";
         else if(fieldID == FWPS_FIELD_INBOUND_ICMP_ERROR_V4_ICMP_CODE)
            pString = "FWPM_CONDITION_ICMP_CODE";
         else if(fieldID == FWPS_FIELD_INBOUND_ICMP_ERROR_V4_INTERFACE_INDEX)
            pString = "FWPM_CONDITION_INTERFACE_INDEX";
         else if(fieldID == FWPS_FIELD_INBOUND_ICMP_ERROR_V4_SUB_INTERFACE_INDEX)
            pString = "FWPM_CONDITION_SUB_INTERFACE_INDEX";
         else if(fieldID == FWPS_FIELD_INBOUND_ICMP_ERROR_V4_INTERFACE_TYPE)
            pString = "FWPM_CONDITION_INTERFACE_TYPE";
         else if(fieldID == FWPS_FIELD_INBOUND_ICMP_ERROR_V4_TUNNEL_TYPE)
            pString = "FWPM_CONDITION_TUNNEL_TYPE";

#if(NTDDI_VERSION >= NTDDI_VISTASP1)

         else if(fieldID == FWPS_FIELD_INBOUND_ICMP_ERROR_V4_IP_ARRIVAL_INTERFACE)
            pString = "FWPM_CONDITION_IP_ARRIVAL_INTERFACE";
         else if(fieldID == FWPS_FIELD_INBOUND_ICMP_ERROR_V4_ARRIVAL_INTERFACE_INDEX)
            pString = "FWPM_CONDITION_ARRIVAL_INTERFACE_INDEX";
         else if(fieldID == FWPS_FIELD_INBOUND_ICMP_ERROR_V4_ARRIVAL_INTERFACE_TYPE)
            pString = "FWPM_CONDITION_ARRIVAL_INTERFACE_TYPE";
         else if(fieldID == FWPS_FIELD_INBOUND_ICMP_ERROR_V4_ARRIVAL_TUNNEL_TYPE)
            pString = "FWPM_CONDITION_ARRIVAL_TUNNEL_TYPE";
         else if(fieldID == FWPS_FIELD_INBOUND_ICMP_ERROR_V4_FLAGS)
         {
            pString = "FWPM_CONDITION_FLAGS";

            if(pFormat)
               *pFormat = FLAGS_VALUE;
         }

#if(NTDDI_VERSION >= NTDDI_WIN7)

         else if(fieldID == FWPS_FIELD_INBOUND_ICMP_ERROR_V4_ARRIVAL_INTERFACE_PROFILE_ID)
            pString = "FWPM_CONDITION_ARRIVAL_INTERFACE_PROFILE_ID";
         else if(fieldID == FWPS_FIELD_INBOUND_ICMP_ERROR_V4_INTERFACE_QUARANTINE_EPOCH)
            pString = "FWPM_CONDITION_INTERFACE_QUARANTINE_EPOCH";

#endif /// (NTDDI_VERSION >= NTDDI_WIN7)
#endif /// (NTDDI_VERSION >= NTDDI_VISTASP1)

         break;
      }
      case FWPS_LAYER_OUTBOUND_ICMP_ERROR_V4:
      case FWPS_LAYER_OUTBOUND_ICMP_ERROR_V4_DISCARD:
      case FWPS_LAYER_OUTBOUND_ICMP_ERROR_V6:
      case FWPS_LAYER_OUTBOUND_ICMP_ERROR_V6_DISCARD:
      {
         if(fieldID == FWPS_FIELD_OUTBOUND_ICMP_ERROR_V4_IP_LOCAL_ADDRESS)
         {
            pString = "FWPM_CONDITION_IP_LOCAL_ADDRESS";

            if(pFormat)
               *pFormat = ADDRESS_VALUE;
         }
         else if(fieldID == FWPS_FIELD_OUTBOUND_ICMP_ERROR_V4_IP_REMOTE_ADDRESS)
         {
            pString = "FWPM_CONDITION_IP_REMOTE_ADDRESS";

            if(pFormat)
               *pFormat = ADDRESS_VALUE;
         }
         else if(fieldID == FWPS_FIELD_OUTBOUND_ICMP_ERROR_V4_IP_LOCAL_ADDRESS_TYPE)
            pString = "FWPM_CONDITION_IP_LOCAL_ADDRESS_TYPE";
         else if(fieldID == FWPS_FIELD_OUTBOUND_ICMP_ERROR_V4_IP_LOCAL_INTERFACE)
            pString = "FWPM_CONDITION_IP_LOCAL_INTERFACE";
         else if(fieldID == FWPS_FIELD_OUTBOUND_ICMP_ERROR_V4_ICMP_TYPE)
            pString = "FWPM_CONDITION_ICMP_TYPE";
         else if(fieldID == FWPS_FIELD_OUTBOUND_ICMP_ERROR_V4_ICMP_CODE)
            pString = "FWPM_CONDITION_ICMP_CODE";
         else if(fieldID == FWPS_FIELD_OUTBOUND_ICMP_ERROR_V4_INTERFACE_INDEX)
            pString = "FWPM_CONDITION_INTERFACE_INDEX";
         else if(fieldID == FWPS_FIELD_OUTBOUND_ICMP_ERROR_V4_SUB_INTERFACE_INDEX)
            pString = "FWPM_CONDITION_SUB_INTERFACE_INDEX";
         else if(fieldID == FWPS_FIELD_OUTBOUND_ICMP_ERROR_V4_INTERFACE_TYPE)
            pString = "FWPM_CONDITION_INTERFACE_TYPE";
         else if(fieldID == FWPS_FIELD_OUTBOUND_ICMP_ERROR_V4_TUNNEL_TYPE)
            pString = "FWPM_CONDITION_TUNNEL_TYPE";

#if(NTDDI_VERSION >= NTDDI_VISTASP1)

         else if(fieldID == FWPS_FIELD_OUTBOUND_ICMP_ERROR_V4_FLAGS)
         {
            pString = "FWPM_CONDITION_FLAGS";

            if(pFormat)
               *pFormat = FLAGS_VALUE;
         }

#if(NTDDI_VERSION >= NTDDI_WIN7)

         else if(fieldID == FWPS_FIELD_OUTBOUND_ICMP_ERROR_V4_NEXTHOP_INTERFACE_PROFILE_ID)
            pString = "FWPM_CONDITION_NEXTHOP_INTERFACE_PROFILE_ID";
         else if(fieldID == FWPS_FIELD_OUTBOUND_ICMP_ERROR_V4_INTERFACE_QUARANTINE_EPOCH)
            pString = "FWPM_CONDITION_INTERFACE_QUARANTINE_EPOCH";

#endif /// (NTDDI_VERSION >= NTDDI_WIN7)
#endif /// (NTDDI_VERSION >= NTDDI_VISTASP1)

         break;
      }
      case FWPS_LAYER_ALE_RESOURCE_ASSIGNMENT_V4:
      case FWPS_LAYER_ALE_RESOURCE_ASSIGNMENT_V4_DISCARD:
      case FWPS_LAYER_ALE_RESOURCE_ASSIGNMENT_V6:
      case FWPS_LAYER_ALE_RESOURCE_ASSIGNMENT_V6_DISCARD:
      {
         if(fieldID == FWPS_FIELD_ALE_RESOURCE_ASSIGNMENT_V4_ALE_APP_ID)
         {
            pString = "FWPM_CONDITION_ALE_APP_ID";

            if(pFormat)
               *pFormat = UNICODE_STRING_VALUE;
         }
         else if(fieldID == FWPS_FIELD_ALE_RESOURCE_ASSIGNMENT_V4_ALE_USER_ID)
            pString = "FWPM_CONDITION_ALE_USER_ID";
         else if(fieldID == FWPS_FIELD_ALE_RESOURCE_ASSIGNMENT_V4_IP_LOCAL_ADDRESS)
         {
            pString = "FWPM_CONDITION_IP_LOCAL_ADDRESS";

            if(pFormat)
               *pFormat = ADDRESS_VALUE;
         }
         else if(fieldID == FWPS_FIELD_ALE_RESOURCE_ASSIGNMENT_V4_IP_LOCAL_ADDRESS_TYPE)
            pString = "FWPM_CONDITION_IP_LOCAL_ADDRESS_TYPE";
         else if(fieldID == FWPS_FIELD_ALE_RESOURCE_ASSIGNMENT_V4_IP_LOCAL_PORT)
            pString = "FWPM_CONDITION_IP_LOCAL_PORT";
         else if(fieldID == FWPS_FIELD_ALE_RESOURCE_ASSIGNMENT_V4_IP_PROTOCOL)
            pString = "FWPM_CONDITION_IP_PROTOCOL";
         else if(fieldID == FWPS_FIELD_ALE_RESOURCE_ASSIGNMENT_V4_ALE_PROMISCUOUS_MODE)
            pString = "FWPM_CONDITION_ALE_PROMISCUOUS_MODE";
         else if(fieldID == FWPS_FIELD_ALE_RESOURCE_ASSIGNMENT_V4_IP_LOCAL_INTERFACE)
            pString = "FWPM_CONDITION_IP_LOCAL_INTERFACE";
         else if(fieldID == FWPS_FIELD_ALE_RESOURCE_ASSIGNMENT_V4_FLAGS)
         {
            pString = "FWPM_CONDITION_FLAGS";

            if(pFormat)
               *pFormat = FLAGS_VALUE;
         }
         else if(fieldID == FWPS_FIELD_ALE_RESOURCE_ASSIGNMENT_V4_INTERFACE_TYPE)
            pString = "FWPM_CONDITION_INTERFACE_TYPE";
         else if(fieldID == FWPS_FIELD_ALE_RESOURCE_ASSIGNMENT_V4_TUNNEL_TYPE)
            pString = "FWPM_CONDITION_TUNNEL_TYPE";

#if(NTDDI_VERSION >= NTDDI_WIN7)

         else if(fieldID == FWPS_FIELD_ALE_RESOURCE_ASSIGNMENT_V4_LOCAL_INTERFACE_PROFILE_ID)
            pString = "FWPM_CONDITION_LOCAL_INTERFACE_PROFILE_ID";
         else if(fieldID == FWPS_FIELD_ALE_RESOURCE_ASSIGNMENT_V4_SIO_FIREWALL_SOCKET_PROPERTY)
            pString = "FWPM_CONDITION_SIO_FIREWALL_SOCKET_PROPERTY";

#if(NTDDI_VERSION >= NTDDI_WIN8)

         else if(fieldID == FWPS_FIELD_ALE_RESOURCE_ASSIGNMENT_V4_ALE_PACKAGE_ID)
            pString = "FWPM_CONDITION_ALE_PACKAGE_ID";

#if(NTDDI_VERSION >= NTDDI_WINTHRESHOLD)
         
         else if(fieldID == FWPS_FIELD_ALE_RESOURCE_ASSIGNMENT_V4_ALE_SECURITY_ATTRIBUTE_FQBN_VALUE)
            pString = "FWPM_CONDITION_ALE_SECURITY_ATTRIBUTE_FQBN_VALUE";
         
#endif /// (NTDDI_VERSION >= NTDDI_WINTHRESHOLD)
#endif /// (NTDDI_VERSION >= NTDDI_WIN8)
#endif /// (NTDDI_VERSION >= NTDDI_WIN7)

         break;
      }
      case FWPS_LAYER_ALE_AUTH_LISTEN_V4:
      case FWPS_LAYER_ALE_AUTH_LISTEN_V4_DISCARD:
      case FWPS_LAYER_ALE_AUTH_LISTEN_V6:
      case FWPS_LAYER_ALE_AUTH_LISTEN_V6_DISCARD:
      {
         if(fieldID == FWPS_FIELD_ALE_AUTH_LISTEN_V4_ALE_APP_ID)
         {
            pString = "FWPM_CONDITION_ALE_APP_ID";

            if(pFormat)
               *pFormat = UNICODE_STRING_VALUE;
         }
         else if(fieldID == FWPS_FIELD_ALE_AUTH_LISTEN_V4_ALE_USER_ID)
            pString = "FWPM_CONDITION_ALE_USER_ID";
         else if(fieldID == FWPS_FIELD_ALE_AUTH_LISTEN_V4_IP_LOCAL_ADDRESS)
         {
            pString = "FWPM_CONDITION_IP_LOCAL_ADDRESS";

            if(pFormat)
               *pFormat = ADDRESS_VALUE;
         }
         else if(fieldID == FWPS_FIELD_ALE_AUTH_LISTEN_V4_IP_LOCAL_ADDRESS_TYPE)
            pString = "FWPM_CONDITION_IP_LOCAL_ADDRESS_TYPE";
         else if(fieldID == FWPS_FIELD_ALE_AUTH_LISTEN_V4_IP_LOCAL_PORT)
            pString = "FWPM_CONDITION_IP_LOCAL_PORT";
         else if(fieldID == FWPS_FIELD_ALE_AUTH_LISTEN_V4_IP_LOCAL_INTERFACE)
            pString = "FWPM_CONDITION_IP_LOCAL_INTERFACE";
         else if(fieldID == FWPS_FIELD_ALE_AUTH_LISTEN_V4_FLAGS)
         {
            pString = "FWPM_CONDITION_FLAGS";

            if(pFormat)
               *pFormat = FLAGS_VALUE;
         }
         else if(fieldID == FWPS_FIELD_ALE_AUTH_LISTEN_V4_INTERFACE_TYPE)
            pString = "FWPM_CONDITION_INTERFACE_TYPE";
         else if(fieldID == FWPS_FIELD_ALE_AUTH_LISTEN_V4_TUNNEL_TYPE)
            pString = "FWPM_CONDITION_TUNNEL_TYPE";

#if(NTDDI_VERSION >= NTDDI_WIN7)

         else if(fieldID == FWPS_FIELD_ALE_AUTH_LISTEN_V4_LOCAL_INTERFACE_PROFILE_ID)
            pString = "FWPM_CONDITION_LOCAL_INTERFACE_PROFILE_ID";
         else if(fieldID == FWPS_FIELD_ALE_AUTH_LISTEN_V4_SIO_FIREWALL_SOCKET_PROPERTY)
            pString = "FWPM_CONDITION_SIO_FIREWALL_SOCKET_PROPERTY";

#if(NTDDI_VERSION >= NTDDI_WIN8)

         else if(fieldID == FWPS_FIELD_ALE_AUTH_LISTEN_V4_ALE_PACKAGE_ID)
            pString = "FWPM_CONDITION_ALE_PACKAGE_ID";

#if(NTDDI_VERSION >= NTDDI_WINTHRESHOLD)
         
         else if(fieldID == FWPS_FIELD_ALE_AUTH_LISTEN_V4_ALE_SECURITY_ATTRIBUTE_FQBN_VALUE)
            pString = "FWPM_CONDITION_ALE_SECURITY_ATTRIBUTE_FQBN_VALUE";
         
#endif /// (NTDDI_VERSION >= NTDDI_WINTHRESHOLD)
#endif /// (NTDDI_VERSION >= NTDDI_WIN8)
#endif /// (NTDDI_VERSION >= NTDDI_WIN7)

         break;
      }
      case FWPS_LAYER_ALE_AUTH_RECV_ACCEPT_V4:
      case FWPS_LAYER_ALE_AUTH_RECV_ACCEPT_V4_DISCARD:
      case FWPS_LAYER_ALE_AUTH_RECV_ACCEPT_V6:
      case FWPS_LAYER_ALE_AUTH_RECV_ACCEPT_V6_DISCARD:
      {
         if(fieldID == FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V4_ALE_APP_ID)
         {
            pString = "FWPM_CONDITION_ALE_APP_ID";

            if(pFormat)
               *pFormat = UNICODE_STRING_VALUE;
         }
         else if(fieldID == FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V4_ALE_USER_ID)
            pString = "FWPM_CONDITION_ALE_USER_ID";
         else if(fieldID == FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V4_IP_LOCAL_ADDRESS)
         {
            pString = "FWPM_CONDITION_IP_LOCAL_ADDRESS";

            if(pFormat)
               *pFormat = ADDRESS_VALUE;
         }
         else if(fieldID == FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V4_IP_LOCAL_ADDRESS_TYPE)
            pString = "FWPM_CONDITION_IP_LOCAL_ADDRESS_TYPE";
         else if(fieldID == FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V4_IP_LOCAL_PORT)
            pString = "FWPM_CONDITION_IP_LOCAL_PORT";
         else if(fieldID == FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V4_IP_PROTOCOL)
            pString = "FWPM_CONDITION_IP_PROTOCOL";
         else if(fieldID == FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V4_IP_REMOTE_ADDRESS)
         {
            pString = "FWPM_CONDITION_IP_REMOTE_ADDRESS";

            if(pFormat)
               *pFormat = ADDRESS_VALUE;
         }
         else if(fieldID == FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V4_IP_REMOTE_PORT)
            pString = "FWPM_CONDITION_IP_REMOTE_PORT";
         else if(fieldID == FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V4_ALE_REMOTE_USER_ID)
            pString = "FWPM_CONDITION_ALE_REMOTE_USER_ID";
         else if(fieldID == FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V4_ALE_REMOTE_MACHINE_ID)
            pString = "FWPM_CONDITION_ALE_REMOTE_MACHINE_ID";
         else if(fieldID == FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V4_IP_LOCAL_INTERFACE)
            pString = "FWPM_CONDITION_IP_LOCAL_INTERFACE";
         else if(fieldID == FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V4_FLAGS)
         {
            pString = "FWPM_CONDITION_FLAGS";

            if(pFormat)
               *pFormat = FLAGS_VALUE;
         }
         else if(fieldID == FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V4_SIO_FIREWALL_SYSTEM_PORT)
            pString = "FWPM_CONDITION_SIO_FIREWALL_SYSTEM_PORT";
         else if(fieldID == FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V4_NAP_CONTEXT)
            pString = "FWPM_CONDITION_NAP_CONTEXT";
         else if(fieldID == FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V4_INTERFACE_TYPE)
            pString = "FWPM_CONDITION_INTERFACE_TYPE";
         else if(fieldID == FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V4_TUNNEL_TYPE)
            pString = "FWPM_CONDITION_TUNNEL_TYPE";
         else if(fieldID == FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V4_INTERFACE_INDEX)
            pString = "FWPM_CONDITION_INTERFACE_INDEX";
         else if(fieldID == FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V4_SUB_INTERFACE_INDEX)
            pString = "FWPM_CONDITION_SUB_INTERFACE_INDEX";

#if(NTDDI_VERSION >= NTDDI_VISTASP1)

         else if(fieldID == FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V4_IP_ARRIVAL_INTERFACE)
            pString = "FWPM_CONDITION_IP_ARRIVAL_INTERFACE";
         else if(fieldID == FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V4_ARRIVAL_INTERFACE_TYPE)
            pString = "FWPM_CONDITION_ARRIVAL_INTERFACE_TYPE";
         else if(fieldID == FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V4_ARRIVAL_TUNNEL_TYPE)
            pString = "FWPM_CONDITION_ARRIVAL_TUNNEL_TYPE";
         else if(fieldID == FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V4_ARRIVAL_INTERFACE_INDEX)
            pString = "FWPM_CONDITION_ARRIVAL_INTERFACE_INDEX";

#if(NTDDI_VERSION >= NTDDI_WIN7)

         else if(fieldID == FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V4_NEXTHOP_SUB_INTERFACE_INDEX)
            pString = "FWPM_CONDITION_NEXTHOP_SUB_INTERFACE_INDEX";
         else if(fieldID == FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V4_IP_NEXTHOP_INTERFACE)
            pString = "FWPM_CONDITION_IP_NEXTHOP_INTERFACE";
         else if(fieldID == FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V4_NEXTHOP_INTERFACE_TYPE)
            pString = "FWPM_CONDITION_NEXTHOP_INTERFACE_TYPE";
         else if(fieldID == FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V4_NEXTHOP_TUNNEL_TYPE)
            pString = "FWPM_CONDITION_NEXTHOP_TUNNEL_TYPE";
         else if(fieldID == FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V4_NEXTHOP_INTERFACE_INDEX)
            pString = "FWPM_CONDITION_NEXTHOP_INTERFACE_INDEX";
         else if(fieldID == FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V4_ORIGINAL_PROFILE_ID)
            pString = "FWPM_CONDITION_ORIGINAL_PROFILE_ID";
         else if(fieldID == FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V4_CURRENT_PROFILE_ID)
            pString = "FWPM_CONDITION_CURRENT_PROFILE_ID";
         else if(fieldID == FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V4_REAUTHORIZE_REASON)
            pString = "FWPM_CONDITION_REAUTHORIZE_REASON";
         else if(fieldID == FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V4_ORIGINAL_ICMP_TYPE)
            pString = "FWPM_CONDITION_ORIGINAL_ICMP_TYPE";
         else if(fieldID == FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V4_INTERFACE_QUARANTINE_EPOCH)
            pString = "FWPM_CONDITION_INTERFACE_QUARANTINE_EPOCH";

#if(NTDDI_VERSION >= NTDDI_WIN8)

         else if(fieldID == FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V4_ALE_PACKAGE_ID)
            pString = "FWPM_CONDITION_ALE_PACKAGE_ID";

#if(NTDDI_VERSION >= NTDDI_WINTHRESHOLD)
         
         else if(fieldID == FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V4_ALE_SECURITY_ATTRIBUTE_FQBN_VALUE)
            pString = "FWPM_CONDITION_ALE_SECURITY_ATTRIBUTE_FQBN_VALUE";
         
#endif /// (NTDDI_VERSION >= NTDDI_WINTHRESHOLD)
#endif /// (NTDDI_VERSION >= NTDDI_WIN8)
#endif /// (NTDDI_VERSION >= NTDDI_WIN7)
#endif /// (NTDDI_VERSION >= NTDDI_VISTASP1)

         break;
      }
      case FWPS_LAYER_ALE_AUTH_CONNECT_V4:
      case FWPS_LAYER_ALE_AUTH_CONNECT_V4_DISCARD:
      case FWPS_LAYER_ALE_AUTH_CONNECT_V6:
      case FWPS_LAYER_ALE_AUTH_CONNECT_V6_DISCARD:
      {
         if(fieldID == FWPS_FIELD_ALE_AUTH_CONNECT_V4_ALE_APP_ID)
         {
            pString = "FWPM_CONDITION_ALE_APP_ID";

            if(pFormat)
               *pFormat = UNICODE_STRING_VALUE;
         }
         else if(fieldID == FWPS_FIELD_ALE_AUTH_CONNECT_V4_ALE_USER_ID)
            pString = "FWPM_CONDITION_ALE_USER_ID";
         else if(fieldID == FWPS_FIELD_ALE_AUTH_CONNECT_V4_IP_LOCAL_ADDRESS)
         {
            pString = "FWPM_CONDITION_IP_LOCAL_ADDRESS";

            if(pFormat)
               *pFormat = ADDRESS_VALUE;
         }
         else if(fieldID == FWPS_FIELD_ALE_AUTH_CONNECT_V4_IP_LOCAL_ADDRESS_TYPE)
            pString = "FWPM_CONDITION_IP_LOCAL_ADDRESS_TYPE";
         else if(fieldID == FWPS_FIELD_ALE_AUTH_CONNECT_V4_IP_LOCAL_PORT)
            pString = "FWPM_CONDITION_IP_LOCAL_PORT";
         else if(fieldID == FWPS_FIELD_ALE_AUTH_CONNECT_V4_IP_PROTOCOL)
            pString = "FWPM_CONDITION_IP_PROTOCOL";
         else if(fieldID == FWPS_FIELD_ALE_AUTH_CONNECT_V4_IP_REMOTE_ADDRESS)
         {
            pString = "FWPM_CONDITION_IP_REMOTE_ADDRESS";

            if(pFormat)
               *pFormat = ADDRESS_VALUE;
         }
         else if(fieldID == FWPS_FIELD_ALE_AUTH_CONNECT_V4_IP_REMOTE_PORT)
            pString = "FWPM_CONDITION_IP_REMOTE_PORT";
         else if(fieldID == FWPS_FIELD_ALE_AUTH_CONNECT_V4_ALE_REMOTE_USER_ID)
            pString = "FWPM_CONDITION_ALE_REMOTE_USER_ID";
         else if(fieldID == FWPS_FIELD_ALE_AUTH_CONNECT_V4_ALE_REMOTE_MACHINE_ID)
            pString = "FWPM_CONDITION_ALE_REMOTE_MACHINE_ID";
         else if(fieldID == FWPS_FIELD_ALE_AUTH_CONNECT_V4_IP_DESTINATION_ADDRESS_TYPE)
            pString = "FWPM_CONDITION_IP_DESTINATION_ADDRESS_TYPE";
         else if(fieldID == FWPS_FIELD_ALE_AUTH_CONNECT_V4_IP_LOCAL_INTERFACE)
            pString = "FWPM_CONDITION_IP_LOCAL_INTERFACE";
         else if(fieldID == FWPS_FIELD_ALE_AUTH_CONNECT_V4_FLAGS)
         {
            pString = "FWPM_CONDITION_FLAGS";

            if(pFormat)
               *pFormat = FLAGS_VALUE;
         }
         else if(fieldID == FWPS_FIELD_ALE_AUTH_CONNECT_V4_INTERFACE_TYPE)
            pString = "FWPM_CONDITION_INTERFACE_TYPE";
         else if(fieldID == FWPS_FIELD_ALE_AUTH_CONNECT_V4_TUNNEL_TYPE)
            pString = "FWPM_CONDITION_TUNNEL_TYPE";

#if(NTDDI_VERSION >= NTDDI_VISTASP1)

         else if(fieldID == FWPS_FIELD_ALE_AUTH_CONNECT_V4_INTERFACE_INDEX)
            pString = "FWPM_CONDITION_INTERFACE_INDEX";
         else if(fieldID == FWPS_FIELD_ALE_AUTH_CONNECT_V4_SUB_INTERFACE_INDEX)
            pString = "FWPM_CONDITION_SUB_INTERFACE_INDEX";

#if(NTDDI_VERSION >= NTDDI_WIN7)

         else if(fieldID == FWPS_FIELD_ALE_AUTH_CONNECT_V4_IP_ARRIVAL_INTERFACE)
            pString = "FWPM_CONDITION_IP_ARRIVAL_INTERFACE";
         else if(fieldID == FWPS_FIELD_ALE_AUTH_CONNECT_V4_ARRIVAL_INTERFACE_TYPE)
            pString = "FWPM_CONDITION_ARRIVAL_INTERFACE_TYPE";
         else if(fieldID == FWPS_FIELD_ALE_AUTH_CONNECT_V4_ARRIVAL_TUNNEL_TYPE)
            pString = "FWPM_CONDITION_ARRIVAL_TUNNEL_TYPE";
         else if(fieldID == FWPS_FIELD_ALE_AUTH_CONNECT_V4_ARRIVAL_INTERFACE_INDEX)
            pString = "FWPM_CONDITION_ARRIVAL_INTERFACE_INDEX";
         else if(fieldID == FWPS_FIELD_ALE_AUTH_CONNECT_V4_NEXTHOP_SUB_INTERFACE_INDEX)
            pString = "FWPM_CONDITION_NEXTHOP_SUB_INTERFACE_INDEX";
         else if(fieldID == FWPS_FIELD_ALE_AUTH_CONNECT_V4_IP_NEXTHOP_INTERFACE)
            pString = "FWPM_CONDITION_IP_NEXTHOP_INTERFACE";
         else if(fieldID == FWPS_FIELD_ALE_AUTH_CONNECT_V4_NEXTHOP_INTERFACE_TYPE)
            pString = "FWPM_CONDITION_NEXTHOP_INTERFACE_TYPE";
         else if(fieldID == FWPS_FIELD_ALE_AUTH_CONNECT_V4_NEXTHOP_TUNNEL_TYPE)
            pString = "FWPM_CONDITION_NEXTHOP_TUNNEL_TYPE";
         else if(fieldID == FWPS_FIELD_ALE_AUTH_CONNECT_V4_NEXTHOP_INTERFACE_INDEX)
            pString = "FWPM_CONDITION_NEXTHOP_INTERFACE_INDEX";
         else if(fieldID == FWPS_FIELD_ALE_AUTH_CONNECT_V4_ORIGINAL_PROFILE_ID)
            pString = "FWPM_CONDITION_ORIGINAL_PROFILE_ID";
         else if(fieldID == FWPS_FIELD_ALE_AUTH_CONNECT_V4_CURRENT_PROFILE_ID)
            pString = "FWPM_CONDITION_CURRENT_PROFILE_ID";
         else if(fieldID == FWPS_FIELD_ALE_AUTH_CONNECT_V4_REAUTHORIZE_REASON)
            pString = "FWPM_CONDITION_REAUTHORIZE_REASON";
         else if(fieldID == FWPS_FIELD_ALE_AUTH_CONNECT_V4_PEER_NAME)
            pString = "FWPM_CONDITION_PEER_NAME";
         else if(fieldID == FWPS_FIELD_ALE_AUTH_CONNECT_V4_ORIGINAL_ICMP_TYPE)
            pString = "FWPM_CONDITION_ORIGINAL_ICMP_TYPE";
         else if(fieldID == FWPS_FIELD_ALE_AUTH_CONNECT_V4_INTERFACE_QUARANTINE_EPOCH)
            pString = "FWPM_CONDITION_INTERFACE_QUARANTINE_EPOCH";

#if(NTDDI_VERSION >= NTDDI_WIN8)

         else if(fieldID == FWPS_FIELD_ALE_AUTH_CONNECT_V4_ALE_ORIGINAL_APP_ID)
         {
            pString = "FWPM_CONDITION_ALE_ORIGINAL_APP_ID";

            if(pFormat)
               *pFormat = UNICODE_STRING_VALUE;
         }
         else if(fieldID == FWPS_FIELD_ALE_AUTH_CONNECT_V4_ALE_PACKAGE_ID)
            pString = "FWPM_CONDITION_ALE_PACKAGE_ID";

#if(NTDDI_VERSION >= NTDDI_WINTHRESHOLD)
         
         else if(fieldID == FWPS_FIELD_ALE_AUTH_CONNECT_V4_ALE_SECURITY_ATTRIBUTE_FQBN_VALUE)
            pString = "FWPM_CONDITION_ALE_SECURITY_ATTRIBUTE_FQBN_VALUE";
         
#endif /// (NTDDI_VERSION >= NTDDI_WINTHRESHOLD)
#endif /// (NTDDI_VERSION >= NTDDI_WIN8)
#endif /// (NTDDI_VERSION >= NTDDI_WIN7)
#endif /// (NTDDI_VERSION >= NTDDI_VISTASP1)

         break;
      }
      case FWPS_LAYER_ALE_FLOW_ESTABLISHED_V4:
      case FWPS_LAYER_ALE_FLOW_ESTABLISHED_V4_DISCARD:
      case FWPS_LAYER_ALE_FLOW_ESTABLISHED_V6:
      case FWPS_LAYER_ALE_FLOW_ESTABLISHED_V6_DISCARD:
      {
         if(fieldID == FWPS_FIELD_ALE_FLOW_ESTABLISHED_V4_ALE_APP_ID)
         {
            pString = "FWPM_CONDITION_ALE_APP_ID";

            if(pFormat)
               *pFormat = UNICODE_STRING_VALUE;
         }
         else if(fieldID == FWPS_FIELD_ALE_FLOW_ESTABLISHED_V4_ALE_USER_ID)
            pString = "FWPM_CONDITION_ALE_USER_ID";
         else if(fieldID == FWPS_FIELD_ALE_FLOW_ESTABLISHED_V4_IP_LOCAL_ADDRESS)
         {
            pString = "FWPM_CONDITION_IP_LOCAL_ADDRESS";

            if(pFormat)
               *pFormat = ADDRESS_VALUE;
         }
         else if(fieldID == FWPS_FIELD_ALE_FLOW_ESTABLISHED_V4_IP_LOCAL_ADDRESS_TYPE)
            pString = "FWPM_CONDITION_IP_LOCAL_ADDRESS_TYPE";
         else if(fieldID == FWPS_FIELD_ALE_FLOW_ESTABLISHED_V4_IP_LOCAL_PORT)
            pString = "FWPM_CONDITION_IP_LOCAL_PORT";
         else if(fieldID == FWPS_FIELD_ALE_FLOW_ESTABLISHED_V4_IP_PROTOCOL)
            pString = "FWPM_CONDITION_IP_PROTOCOL";
         else if(fieldID == FWPS_FIELD_ALE_FLOW_ESTABLISHED_V4_IP_REMOTE_ADDRESS)
         {
            pString = "FWPM_CONDITION_IP_REMOTE_ADDRESS";

            if(pFormat)
               *pFormat = ADDRESS_VALUE;
         }
         else if(fieldID == FWPS_FIELD_ALE_FLOW_ESTABLISHED_V4_IP_REMOTE_PORT)
            pString = "FWPM_CONDITION_IP_REMOTE_PORT";
         else if(fieldID == FWPS_FIELD_ALE_FLOW_ESTABLISHED_V4_ALE_REMOTE_USER_ID)
            pString = "FWPM_CONDITION_ALE_REMOTE_USER_ID";
         else if(fieldID == FWPS_FIELD_ALE_FLOW_ESTABLISHED_V4_ALE_REMOTE_MACHINE_ID)
            pString = "FWPM_CONDITION_ALE_REMOTE_MACHINE_ID";
         else if(fieldID == FWPS_FIELD_ALE_FLOW_ESTABLISHED_V4_IP_DESTINATION_ADDRESS_TYPE)
            pString = "FWPM_CONDITION_IP_DESTINATION_ADDRESS_TYPE";
         else if(fieldID == FWPS_FIELD_ALE_FLOW_ESTABLISHED_V4_IP_LOCAL_INTERFACE)
            pString = "FWPM_CONDITION_IP_LOCAL_INTERFACE";
         else if(fieldID == FWPS_FIELD_ALE_FLOW_ESTABLISHED_V4_DIRECTION)
            pString = "FWPM_CONDITION_DIRECTION";
         else if(fieldID == FWPS_FIELD_ALE_FLOW_ESTABLISHED_V4_INTERFACE_TYPE)
            pString = "FWPM_CONDITION_INTERFACE_TYPE";
         else if(fieldID == FWPS_FIELD_ALE_FLOW_ESTABLISHED_V4_TUNNEL_TYPE)
            pString = "FWPM_CONDITION_TUNNEL_TYPE";

#if(NTDDI_VERSION >= NTDDI_VISTASP1)

         else if(fieldID == FWPS_FIELD_ALE_FLOW_ESTABLISHED_V4_FLAGS)
         {
            pString = "FWPM_CONDITION_FLAGS";

            if(pFormat)
               *pFormat = FLAGS_VALUE;
         }

#if(NTDDI_VERSION >= NTDDI_WIN8)

         else if(fieldID == FWPS_FIELD_ALE_FLOW_ESTABLISHED_V4_ALE_ORIGINAL_APP_ID)
         {
            pString = "FWPM_CONDITION_ALE_ORIGINAL_APP_ID";

            if(pFormat)
               *pFormat = UNICODE_STRING_VALUE;
         }
         else if(fieldID == FWPS_FIELD_ALE_FLOW_ESTABLISHED_V4_ALE_PACKAGE_ID)
            pString = "FWPM_CONDITION_ALE_PACKAGE_ID";

#if(NTDDI_VERSION >= NTDDI_WINTHRESHOLD)
         
         else if(fieldID == FWPS_FIELD_ALE_FLOW_ESTABLISHED_V4_ALE_SECURITY_ATTRIBUTE_FQBN_VALUE)
            pString = "FWPM_CONDITION_ALE_SECURITY_ATTRIBUTE_FQBN_VALUE";
         
#endif /// (NTDDI_VERSION >= NTDDI_WINTHRESHOLD)
#endif /// (NTDDI_VERSION >= NTDDI_WIN8)
#endif /// (NTDDI_VERSION >= NTDDI_VISTASP1)

         break;
      }

#if(NTDDI_VERSION >= NTDDI_WIN7)

      case FWPS_LAYER_NAME_RESOLUTION_CACHE_V4:
      case FWPS_LAYER_NAME_RESOLUTION_CACHE_V6:
      {
         if(fieldID == FWPS_FIELD_NAME_RESOLUTION_CACHE_V4_ALE_USER_ID)
            pString = "FWPM_CONDITION_ALE_USER_ID";
         else if(fieldID == FWPS_FIELD_NAME_RESOLUTION_CACHE_V4_ALE_APP_ID)
         {
            pString = "FWPM_CONDITION_ALE_APP_ID";

            if(pFormat)
               *pFormat = UNICODE_STRING_VALUE;
         }
         else if(fieldID == FWPS_FIELD_NAME_RESOLUTION_CACHE_V4_IP_REMOTE_ADDRESS)
         {
            pString = "FWPM_CONDITION_IP_REMOTE_ADDRESS";

            if(pFormat)
               *pFormat = ADDRESS_VALUE;
         }
         else if(fieldID == FWPS_FIELD_NAME_RESOLUTION_CACHE_V4_PEER_NAME)
            pString = "FWPM_CONDITION_PEER_NAME";

         break;
      }
      case FWPS_LAYER_ALE_RESOURCE_RELEASE_V4:
      case FWPS_LAYER_ALE_RESOURCE_RELEASE_V6:
      {
         if(fieldID == FWPS_FIELD_ALE_RESOURCE_RELEASE_V4_ALE_APP_ID)
         {
            pString = "FWPM_CONDITION_ALE_APP_ID";

            if(pFormat)
               *pFormat = UNICODE_STRING_VALUE;
         }
         else if(fieldID == FWPS_FIELD_ALE_RESOURCE_RELEASE_V4_ALE_USER_ID)
            pString = "FWPM_CONDITION_ALE_USER_ID";
         else if(fieldID == FWPS_FIELD_ALE_RESOURCE_RELEASE_V4_IP_LOCAL_ADDRESS)
         {
            pString = "FWPM_CONDITION_IP_LOCAL_ADDRESS";

            if(pFormat)
               *pFormat = ADDRESS_VALUE;
         }
         else if(fieldID == FWPS_FIELD_ALE_RESOURCE_RELEASE_V4_IP_LOCAL_ADDRESS_TYPE)
            pString = "FWPM_CONDITION_IP_LOCAL_ADDRESS_TYPE";
         else if(fieldID == FWPS_FIELD_ALE_RESOURCE_RELEASE_V4_IP_LOCAL_PORT)
            pString = "FWPM_CONDITION_IP_LOCAL_PORT";
         else if(fieldID == FWPS_FIELD_ALE_RESOURCE_RELEASE_V4_IP_PROTOCOL)
            pString = "FWPM_CONDITION_IP_PROTOCOL";
         else if(fieldID == FWPS_FIELD_ALE_RESOURCE_RELEASE_V4_IP_LOCAL_INTERFACE)
            pString = "FWPM_CONDITION_IP_LOCAL_INTERFACE";
         else if(fieldID == FWPS_FIELD_ALE_RESOURCE_RELEASE_V4_FLAGS)
         {
            pString = "FWPM_CONDITION_FLAGS";

            if(pFormat)
               *pFormat = FLAGS_VALUE;
         }

#if(NTDDI_VERSION >= NTDDI_WIN8)

         else if(fieldID == FWPS_FIELD_ALE_RESOURCE_RELEASE_V4_ALE_PACKAGE_ID)
            pString = "FWPM_CONDITION_ALE_PACKAGE_ID";

#endif /// (NTDDI_VERSION >= NTDDI_WIN8)

         break;
      }
      case FWPS_LAYER_ALE_ENDPOINT_CLOSURE_V4:
      case FWPS_LAYER_ALE_ENDPOINT_CLOSURE_V6:
      {
         if(fieldID == FWPS_FIELD_ALE_ENDPOINT_CLOSURE_V4_ALE_APP_ID)
         {
            pString = "FWPM_CONDITION_ALE_APP_ID";

            if(pFormat)
               *pFormat = UNICODE_STRING_VALUE;
         }
         else if(fieldID == FWPS_FIELD_ALE_ENDPOINT_CLOSURE_V4_ALE_USER_ID)
            pString = "FWPM_CONDITION_ALE_USER_ID";
         else if(fieldID == FWPS_FIELD_ALE_ENDPOINT_CLOSURE_V4_IP_LOCAL_ADDRESS)
         {
            pString = "FWPM_CONDITION_IP_LOCAL_ADDRESS";

            if(pFormat)
               *pFormat = ADDRESS_VALUE;
         }
         else if(fieldID == FWPS_FIELD_ALE_ENDPOINT_CLOSURE_V4_IP_LOCAL_ADDRESS_TYPE)
            pString = "FWPM_CONDITION_IP_LOCAL_ADDRESS_TYPE";
         else if(fieldID == FWPS_FIELD_ALE_ENDPOINT_CLOSURE_V4_IP_LOCAL_PORT)
            pString = "FWPM_CONDITION_IP_LOCAL_PORT";
         else if(fieldID == FWPS_FIELD_ALE_ENDPOINT_CLOSURE_V4_IP_PROTOCOL)
            pString = "FWPM_CONDITION_IP_PROTOCOL";
         else if(fieldID == FWPS_FIELD_ALE_ENDPOINT_CLOSURE_V4_IP_REMOTE_ADDRESS)
         {
            pString = "FWPM_CONDITION_IP_REMOTE_ADDRESS";

            if(pFormat)
               *pFormat = ADDRESS_VALUE;
         }
         else if(fieldID == FWPS_FIELD_ALE_ENDPOINT_CLOSURE_V4_IP_REMOTE_PORT)
            pString = "FWPM_CONDITION_IP_REMOTE_PORT";
         else if(fieldID == FWPS_FIELD_ALE_ENDPOINT_CLOSURE_V4_IP_LOCAL_INTERFACE)
            pString = "FWPM_CONDITION_IP_LOCAL_INTERFACE";
         else if(fieldID == FWPS_FIELD_ALE_ENDPOINT_CLOSURE_V4_FLAGS)
         {
            pString = "FWPM_CONDITION_FLAGS";

            if(pFormat)
               *pFormat = FLAGS_VALUE;
         }

#if(NTDDI_VERSION >= NTDDI_WIN8)

         else if(fieldID == FWPS_FIELD_ALE_ENDPOINT_CLOSURE_V4_ALE_PACKAGE_ID)
            pString = "FWPM_CONDITION_ALE_PACKAGE_ID";

#if(NTDDI_VERSION >= NTDDI_WINTHRESHOLD)
         
         else if(fieldID == FWPS_FIELD_ALE_ENDPOINT_CLOSURE_V4_ALE_SECURITY_ATTRIBUTE_FQBN_VALUE)
            pString = "FWPM_CONDITION_ALE_SECURITY_ATTRIBUTE_FQBN_VALUE";
         
#endif /// (NTDDI_VERSION >= NTDDI_WINTHRESHOLD)
#endif /// (NTDDI_VERSION >= NTDDI_WIN8)

         break;
      }
      case FWPS_LAYER_ALE_CONNECT_REDIRECT_V4:
      case FWPS_LAYER_ALE_CONNECT_REDIRECT_V6:
      {
         if(fieldID == FWPS_FIELD_ALE_CONNECT_REDIRECT_V4_ALE_APP_ID)
         {
            pString = "FWPM_CONDITION_ALE_APP_ID";

            if(pFormat)
               *pFormat = UNICODE_STRING_VALUE;
         }
         else if(fieldID == FWPS_FIELD_ALE_CONNECT_REDIRECT_V4_ALE_USER_ID)
            pString = "FWPM_CONDITION_ALE_USER_ID";
         else if(fieldID == FWPS_FIELD_ALE_CONNECT_REDIRECT_V4_IP_LOCAL_ADDRESS)
         {
            pString = "FWPM_CONDITION_IP_LOCAL_ADDRESS";

            if(pFormat)
               *pFormat = ADDRESS_VALUE;
         }
         else if(fieldID == FWPS_FIELD_ALE_CONNECT_REDIRECT_V4_IP_LOCAL_ADDRESS_TYPE)
            pString = "FWPM_CONDITION_IP_LOCAL_ADDRESS_TYPE";
         else if(fieldID == FWPS_FIELD_ALE_CONNECT_REDIRECT_V4_IP_LOCAL_PORT)
            pString = "FWPM_CONDITION_IP_LOCAL_PORT";
         else if(fieldID == FWPS_FIELD_ALE_CONNECT_REDIRECT_V4_IP_PROTOCOL)
            pString = "FWPM_CONDITION_IP_PROTOCOL";
         else if(fieldID == FWPS_FIELD_ALE_CONNECT_REDIRECT_V4_IP_REMOTE_ADDRESS)
         {
            pString = "FWPM_CONDITION_IP_REMOTE_ADDRESS";

            if(pFormat)
               *pFormat = ADDRESS_VALUE;
         }
         else if(fieldID == FWPS_FIELD_ALE_CONNECT_REDIRECT_V4_IP_DESTINATION_ADDRESS_TYPE)
            pString = "FWPM_CONDITION_IP_DESTINATION_ADDRESS_TYPE";
         else if(fieldID == FWPS_FIELD_ALE_CONNECT_REDIRECT_V4_IP_REMOTE_PORT)
            pString = "FWPM_CONDITION_IP_REMOTE_PORT";
         else if(fieldID == FWPS_FIELD_ALE_CONNECT_REDIRECT_V4_FLAGS)
         {
            pString = "FWPM_CONDITION_FLAGS";

            if(pFormat)
               *pFormat = FLAGS_VALUE;
         }

#if(NTDDI_VERSION >= NTDDI_WIN8)

         else if(fieldID == FWPS_FIELD_ALE_CONNECT_REDIRECT_V4_ALE_ORIGINAL_APP_ID)
         {
            pString = "FWPM_CONDITION_ALE_ORIGINAL_APP_ID";

            if(pFormat)
               *pFormat = UNICODE_STRING_VALUE;
         }
         else if(fieldID == FWPS_FIELD_ALE_CONNECT_REDIRECT_V4_ALE_PACKAGE_ID)
            pString = "FWPM_CONDITION_ALE_PACKAGE_ID";

#if(NTDDI_VERSION >= NTDDI_WINTHRESHOLD)
         
         else if(fieldID == FWPS_FIELD_ALE_CONNECT_REDIRECT_V4_ALE_SECURITY_ATTRIBUTE_FQBN_VALUE)
            pString = "FWPM_CONDITION_ALE_SECURITY_ATTRIBUTE_FQBN_VALUE";
         
#endif /// (NTDDI_VERSION >= NTDDI_WINTHRESHOLD)
#endif /// (NTDDI_VERSION >= NTDDI_WIN8)

         break;
      }
      case FWPS_LAYER_ALE_BIND_REDIRECT_V4:
      case FWPS_LAYER_ALE_BIND_REDIRECT_V6:
      {
         if(fieldID == FWPS_FIELD_ALE_BIND_REDIRECT_V4_ALE_APP_ID)
         {
            pString = "FWPM_CONDITION_ALE_APP_ID";

            if(pFormat)
               *pFormat = UNICODE_STRING_VALUE;
         }
         else if(fieldID == FWPS_FIELD_ALE_BIND_REDIRECT_V4_ALE_USER_ID)
            pString = "FWPM_CONDITION_ALE_USER_ID";
         else if(fieldID == FWPS_FIELD_ALE_BIND_REDIRECT_V4_IP_LOCAL_ADDRESS)
         {
            pString = "FWPM_CONDITION_IP_LOCAL_ADDRESS";

            if(pFormat)
               *pFormat = ADDRESS_VALUE;
         }
         else if(fieldID == FWPS_FIELD_ALE_BIND_REDIRECT_V4_IP_LOCAL_ADDRESS_TYPE)
            pString = "FWPM_CONDITION_IP_LOCAL_ADDRESS_TYPE";
         else if(fieldID == FWPS_FIELD_ALE_BIND_REDIRECT_V4_IP_LOCAL_PORT)
            pString = "FWPM_CONDITION_IP_LOCAL_PORT";
         else if(fieldID == FWPS_FIELD_ALE_BIND_REDIRECT_V4_IP_PROTOCOL)
            pString = "FWPM_CONDITION_IP_PROTOCOL";
         else if(fieldID == FWPS_FIELD_ALE_BIND_REDIRECT_V4_FLAGS)
         {
            pString = "FWPM_CONDITION_FLAGS";

            if(pFormat)
               *pFormat = FLAGS_VALUE;
         }

#if(NTDDI_VERSION >= NTDDI_WIN8)

         else if(fieldID == FWPS_FIELD_ALE_BIND_REDIRECT_V4_ALE_PACKAGE_ID)
            pString = "FWPM_CONDITION_ALE_PACKAGE_ID";

#if(NTDDI_VERSION >= NTDDI_WINTHRESHOLD)
         
         else if(fieldID == FWPS_FIELD_ALE_BIND_REDIRECT_V4_ALE_SECURITY_ATTRIBUTE_FQBN_VALUE)
            pString = "FWPM_CONDITION_ALE_SECURITY_ATTRIBUTE_FQBN_VALUE";
         
#endif /// (NTDDI_VERSION >= NTDDI_WINTHRESHOLD)
#endif /// (NTDDI_VERSION >= NTDDI_WIN8)

         break;
      }
      case FWPS_LAYER_STREAM_PACKET_V4:
      case FWPS_LAYER_STREAM_PACKET_V6:
      {
         if(fieldID == FWPS_FIELD_STREAM_PACKET_V4_IP_LOCAL_ADDRESS)
         {
            pString = "FWPM_CONDITION_IP_LOCAL_ADDRESS";

            if(pFormat)
               *pFormat = ADDRESS_VALUE;
         }
         else if(fieldID == FWPS_FIELD_STREAM_PACKET_V4_IP_REMOTE_ADDRESS)
         {
            pString = "FWPM_CONDITION_IP_REMOTE_ADDRESS";

            if(pFormat)
               *pFormat = ADDRESS_VALUE;
         }
         else if(fieldID == FWPS_FIELD_STREAM_PACKET_V4_IP_LOCAL_PORT)
            pString = "FWPM_CONDITION_IP_LOCAL_PORT";
         else if(fieldID == FWPS_FIELD_STREAM_PACKET_V4_IP_REMOTE_PORT)
            pString = "FWPM_CONDITION_IP_REMOTE_PORT";
         else if(fieldID == FWPS_FIELD_STREAM_PACKET_V4_IP_LOCAL_INTERFACE)
            pString = "FWPM_CONDITION_IP_LOCAL_INTERFACE";
         else if(fieldID == FWPS_FIELD_STREAM_PACKET_V4_INTERFACE_INDEX)
            pString = "FWPM_CONDITION_INTERFACE_INDEX";
         else if(fieldID == FWPS_FIELD_STREAM_PACKET_V4_SUB_INTERFACE_INDEX)
            pString = "FWPM_CONDITION_SUB_INTERFACE_INDEX";
         else if(fieldID == FWPS_FIELD_STREAM_PACKET_V4_DIRECTION)
            pString = "FWPM_CONDITION_DIRECTION";
         else if(fieldID == FWPS_FIELD_STREAM_PACKET_V4_FLAGS)
         {
            pString = "FWPM_CONDITION_FLAGS";

            if(pFormat)
               *pFormat = FLAGS_VALUE;
         }
         else if(fieldID == FWPS_FIELD_STREAM_PACKET_V4_INTERFACE_TYPE)
            pString = "FWPM_CONDITION_INTERFACE_TYPE";
         else if(fieldID == FWPS_FIELD_STREAM_PACKET_V4_TUNNEL_TYPE)
            pString = "FWPM_CONDITION_TUNNEL_TYPE";

         break;
      }

#if(NTDDI_VERSION >= NTDDI_WIN8)

      case FWPS_LAYER_INBOUND_MAC_FRAME_ETHERNET:
      {
         if(fieldID == FWPS_FIELD_INBOUND_MAC_FRAME_ETHERNET_INTERFACE_MAC_ADDRESS)
            pString = "FWPM_CONDITION_INTERFACE_MAC_ADDRESS";
         else if(fieldID == FWPS_FIELD_INBOUND_MAC_FRAME_ETHERNET_MAC_LOCAL_ADDRESS)
            pString = "FWPM_CONDITION_MAC_LOCAL_ADDRESS";
         else if(fieldID == FWPS_FIELD_INBOUND_MAC_FRAME_ETHERNET_MAC_REMOTE_ADDRESS)
            pString = "FWPM_CONDITION_MAC_REMOTE_ADDRESS";
         else if(fieldID == FWPS_FIELD_INBOUND_MAC_FRAME_ETHERNET_MAC_LOCAL_ADDRESS_TYPE)
            pString = "FWPM_CONDITION_MAC_LOCAL_ADDRESS_TYPE";
         else if(fieldID == FWPS_FIELD_INBOUND_MAC_FRAME_ETHERNET_MAC_REMOTE_ADDRESS_TYPE)
            pString = "FWPM_CONDITION_MAC_REMOTE_ADDRESS_TYPE";
         else if(fieldID == FWPS_FIELD_INBOUND_MAC_FRAME_ETHERNET_ETHER_TYPE)
            pString = "FWPM_CONDITION_ETHER_TYPE";
         else if(fieldID == FWPS_FIELD_INBOUND_MAC_FRAME_ETHERNET_VLAN_ID)
            pString = "FWPM_CONDITION_VLAN_ID";
         else if(fieldID == FWPS_FIELD_INBOUND_MAC_FRAME_ETHERNET_INTERFACE)
            pString = "FWPM_CONDITION_INTERFACE";
         else if(fieldID == FWPS_FIELD_INBOUND_MAC_FRAME_ETHERNET_INTERFACE_INDEX)
            pString = "FWPM_CONDITION_INTERFACE_INDEX";
         else if(fieldID == FWPS_FIELD_INBOUND_MAC_FRAME_ETHERNET_NDIS_PORT)
            pString = "FWPM_CONDITION_NDIS_PORT";
         else if(fieldID == FWPS_FIELD_INBOUND_MAC_FRAME_ETHERNET_L2_FLAGS)
         {
            pString = "FWPM_CONDITION_L2_FLAGS";

            if(pFormat)
               *pFormat = L2_FLAGS_VALUE;
         }

         break;
      }
      case FWPS_LAYER_OUTBOUND_MAC_FRAME_ETHERNET:
      {
         if(fieldID == FWPS_FIELD_OUTBOUND_MAC_FRAME_ETHERNET_INTERFACE_MAC_ADDRESS)
            pString = "FWPM_CONDITION_INTERFACE_MAC_ADDRESS";
         else if(fieldID == FWPS_FIELD_OUTBOUND_MAC_FRAME_ETHERNET_MAC_LOCAL_ADDRESS)
            pString = "FWPM_CONDITION_MAC_LOCAL_ADDRESS";
         else if(fieldID == FWPS_FIELD_OUTBOUND_MAC_FRAME_ETHERNET_MAC_REMOTE_ADDRESS)
            pString = "FWPM_CONDITION_MAC_REMOTE_ADDRESS";
         else if(fieldID == FWPS_FIELD_OUTBOUND_MAC_FRAME_ETHERNET_MAC_LOCAL_ADDRESS_TYPE)
            pString = "FWPM_CONDITION_MAC_LOCAL_ADDRESS_TYPE";
         else if(fieldID == FWPS_FIELD_OUTBOUND_MAC_FRAME_ETHERNET_MAC_REMOTE_ADDRESS_TYPE)
            pString = "FWPM_CONDITION_MAC_REMOTE_ADDRESS_TYPE";
         else if(fieldID == FWPS_FIELD_OUTBOUND_MAC_FRAME_ETHERNET_ETHER_TYPE)
            pString = "FWPM_CONDITION_ETHER_TYPE";
         else if(fieldID == FWPS_FIELD_OUTBOUND_MAC_FRAME_ETHERNET_VLAN_ID)
            pString = "FWPM_CONDITION_VLAN_ID";
         else if(fieldID == FWPS_FIELD_OUTBOUND_MAC_FRAME_ETHERNET_INTERFACE)
            pString = "FWPM_CONDITION_INTERFACE";
         else if(fieldID == FWPS_FIELD_OUTBOUND_MAC_FRAME_ETHERNET_INTERFACE_INDEX)
            pString = "FWPM_CONDITION_INTERFACE_INDEX";
         else if(fieldID == FWPS_FIELD_OUTBOUND_MAC_FRAME_ETHERNET_NDIS_PORT)
            pString = "FWPM_CONDITION_NDIS_PORT";
         else if(fieldID == FWPS_FIELD_OUTBOUND_MAC_FRAME_ETHERNET_L2_FLAGS)
         {
            pString = "FWPM_CONDITION_L2_FLAGS";

            if(pFormat)
               *pFormat = L2_FLAGS_VALUE;
         }

         break;
      }
      case FWPS_LAYER_INBOUND_MAC_FRAME_NATIVE:
      {
         if(fieldID == FWPS_FIELD_INBOUND_MAC_FRAME_NATIVE_NDIS_MEDIA_TYPE)
            pString = "FWPM_CONDITION_NDIS_MEDIA_TYPE";
         else if(fieldID == FWPS_FIELD_INBOUND_MAC_FRAME_NATIVE_NDIS_PHYSICAL_MEDIA_TYPE)
            pString = "FWPM_CONDITION_NDIS_PHYSICAL_MEDIA_TYPE";
         else if(fieldID == FWPS_FIELD_INBOUND_MAC_FRAME_NATIVE_INTERFACE)
            pString = "FWPM_CONDITION_INTERFACE";
         else if(fieldID == FWPS_FIELD_INBOUND_MAC_FRAME_NATIVE_INTERFACE_TYPE)
            pString = "FWPM_CONDITION_INTERFACE_TYPE";
         else if(fieldID == FWPS_FIELD_INBOUND_MAC_FRAME_NATIVE_INTERFACE_INDEX)
            pString = "FWPM_CONDITION_INTERFACE_INDEX";
         else if(fieldID == FWPS_FIELD_INBOUND_MAC_FRAME_NATIVE_NDIS_PORT)
            pString = "FWPM_CONDITION_NDIS_PORT";
         else if(fieldID == FWPS_FIELD_INBOUND_MAC_FRAME_NATIVE_L2_FLAGS)
         {
            pString = "FWPM_CONDITION_L2_FLAGS";

            if(pFormat)
               *pFormat = L2_FLAGS_VALUE;
         }

         break;
      }
      case FWPS_LAYER_OUTBOUND_MAC_FRAME_NATIVE:
      {
         if(fieldID == FWPS_FIELD_OUTBOUND_MAC_FRAME_NATIVE_NDIS_MEDIA_TYPE)
            pString = "FWPM_CONDITION_NDIS_MEDIA_TYPE";
         else if(fieldID == FWPS_FIELD_OUTBOUND_MAC_FRAME_NATIVE_NDIS_PHYSICAL_MEDIA_TYPE)
            pString = "FWPM_CONDITION_NDIS_PHYSICAL_MEDIA_TYPE";
         else if(fieldID == FWPS_FIELD_OUTBOUND_MAC_FRAME_NATIVE_INTERFACE)
            pString = "FWPM_CONDITION_INTERFACE";
         else if(fieldID == FWPS_FIELD_OUTBOUND_MAC_FRAME_NATIVE_INTERFACE_TYPE)
            pString = "FWPM_CONDITION_INTERFACE_TYPE";
         else if(fieldID == FWPS_FIELD_OUTBOUND_MAC_FRAME_NATIVE_INTERFACE_INDEX)
            pString = "FWPM_CONDITION_INTERFACE_INDEX";
         else if(fieldID == FWPS_FIELD_OUTBOUND_MAC_FRAME_NATIVE_NDIS_PORT)
            pString = "FWPM_CONDITION_NDIS_PORT";
         else if(fieldID == FWPS_FIELD_OUTBOUND_MAC_FRAME_NATIVE_L2_FLAGS)
         {
            pString = "FWPM_CONDITION_L2_FLAGS";

            if(pFormat)
               *pFormat = L2_FLAGS_VALUE;
         }

         break;
      }
      case FWPS_LAYER_INGRESS_VSWITCH_ETHERNET:
      {
         if(fieldID == FWPS_FIELD_INGRESS_VSWITCH_ETHERNET_MAC_SOURCE_ADDRESS)
            pString = "FWPS_FIELD_INGRESS_VSWITCH_ETHERNET_MAC_SOURCE_ADDRESS";
         else if(fieldID == FWPS_FIELD_INGRESS_VSWITCH_ETHERNET_MAC_SOURCE_ADDRESS_TYPE)
            pString = "FWPS_FIELD_INGRESS_VSWITCH_ETHERNET_MAC_SOURCE_ADDRESS_TYPE";
         else if(fieldID == FWPS_FIELD_INGRESS_VSWITCH_ETHERNET_MAC_DESTINATION_ADDRESS)
            pString = "FWPS_FIELD_INGRESS_VSWITCH_ETHERNET_MAC_DESTINATION_ADDRESS";
         else if(fieldID == FWPS_FIELD_INGRESS_VSWITCH_ETHERNET_MAC_DESTINATION_ADDRESS_TYPE)
            pString = "FWPS_FIELD_INGRESS_VSWITCH_ETHERNET_MAC_DESTINATION_ADDRESS_TYPE";
         else if(fieldID == FWPS_FIELD_INGRESS_VSWITCH_ETHERNET_ETHER_TYPE)
            pString = "FWPS_FIELD_INGRESS_VSWITCH_ETHERNET_ETHER_TYPE";
         else if(fieldID == FWPS_FIELD_INGRESS_VSWITCH_ETHERNET_VLAN_ID)
            pString = "FWPS_FIELD_INGRESS_VSWITCH_ETHERNET_VLAN_ID";
         else if(fieldID == FWPS_FIELD_INGRESS_VSWITCH_ETHERNET_VSWITCH_TENANT_NETWORK_ID)
            pString = "FWPS_FIELD_INGRESS_VSWITCH_ETHERNET_VSWITCH_TENANT_NETWORK_ID";
         else if(fieldID == FWPS_FIELD_INGRESS_VSWITCH_ETHERNET_VSWITCH_ID)
            pString = "FWPS_FIELD_INGRESS_VSWITCH_ETHERNET_VSWITCH_ID";
         else if(fieldID == FWPS_FIELD_INGRESS_VSWITCH_ETHERNET_VSWITCH_NETWORK_TYPE)
            pString = "FWPS_FIELD_INGRESS_VSWITCH_ETHERNET_VSWITCH_NETWORK_TYPE";
         else if(fieldID == FWPS_FIELD_INGRESS_VSWITCH_ETHERNET_VSWITCH_SOURCE_INTERFACE_ID)
            pString = "FWPS_FIELD_INGRESS_VSWITCH_ETHERNET_VSWITCH_SOURCE_INTERFACE_ID";
         else if(fieldID == FWPS_FIELD_INGRESS_VSWITCH_ETHERNET_VSWITCH_SOURCE_INTERFACE_TYPE)
            pString = "FWPS_FIELD_INGRESS_VSWITCH_ETHERNET_VSWITCH_SOURCE_INTERFACE_TYPE";
         else if(fieldID == FWPS_FIELD_INGRESS_VSWITCH_ETHERNET_VSWITCH_SOURCE_VM_ID)
            pString = "FWPS_FIELD_INGRESS_VSWITCH_ETHERNET_VSWITCH_SOURCE_VM_ID";
         else if(fieldID == FWPS_FIELD_INGRESS_VSWITCH_ETHERNET_L2_FLAGS)
            pString = "FWPS_FIELD_INGRESS_VSWITCH_ETHERNET_L2_FLAGS";

         break;
      }
      case FWPS_LAYER_EGRESS_VSWITCH_ETHERNET:
      {
         if(fieldID == FWPS_FIELD_EGRESS_VSWITCH_ETHERNET_MAC_SOURCE_ADDRESS)
            pString = "FWPS_FIELD_EGRESS_VSWITCH_ETHERNET_MAC_SOURCE_ADDRESS";
         else if(fieldID == FWPS_FIELD_EGRESS_VSWITCH_ETHERNET_MAC_SOURCE_ADDRESS_TYPE)
            pString = "FWPS_FIELD_EGRESS_VSWITCH_ETHERNET_MAC_SOURCE_ADDRESS_TYPE";
         else if(fieldID == FWPS_FIELD_EGRESS_VSWITCH_ETHERNET_MAC_DESTINATION_ADDRESS)
            pString = "FWPS_FIELD_EGRESS_VSWITCH_ETHERNET_MAC_DESTINATION_ADDRESS";
         else if(fieldID == FWPS_FIELD_EGRESS_VSWITCH_ETHERNET_MAC_DESTINATION_ADDRESS_TYPE)
            pString = "FWPS_FIELD_EGRESS_VSWITCH_ETHERNET_MAC_DESTINATION_ADDRESS_TYPE";
         else if(fieldID == FWPS_FIELD_EGRESS_VSWITCH_ETHERNET_ETHER_TYPE)
            pString = "FWPS_FIELD_EGRESS_VSWITCH_ETHERNET_ETHER_TYPE";
         else if(fieldID == FWPS_FIELD_EGRESS_VSWITCH_ETHERNET_VLAN_ID)
            pString = "FWPS_FIELD_EGRESS_VSWITCH_ETHERNET_VLAN_ID";
         else if(fieldID == FWPS_FIELD_EGRESS_VSWITCH_ETHERNET_VSWITCH_TENANT_NETWORK_ID)
            pString = "FWPS_FIELD_EGRESS_VSWITCH_ETHERNET_VSWITCH_TENANT_NETWORK_ID";
         else if(fieldID == FWPS_FIELD_EGRESS_VSWITCH_ETHERNET_VSWITCH_ID)
            pString = "FWPS_FIELD_EGRESS_VSWITCH_ETHERNET_VSWITCH_ID";
         else if(fieldID == FWPS_FIELD_EGRESS_VSWITCH_ETHERNET_VSWITCH_NETWORK_TYPE)
            pString = "FWPS_FIELD_EGRESS_VSWITCH_ETHERNET_VSWITCH_NETWORK_TYPE";
         else if(fieldID == FWPS_FIELD_EGRESS_VSWITCH_ETHERNET_VSWITCH_SOURCE_INTERFACE_ID)
            pString = "FWPS_FIELD_EGRESS_VSWITCH_ETHERNET_VSWITCH_SOURCE_INTERFACE_ID";
         else if(fieldID == FWPS_FIELD_EGRESS_VSWITCH_ETHERNET_VSWITCH_SOURCE_INTERFACE_TYPE)
            pString = "FWPS_FIELD_EGRESS_VSWITCH_ETHERNET_VSWITCH_SOURCE_INTERFACE_TYPE";
         else if(fieldID == FWPS_FIELD_EGRESS_VSWITCH_ETHERNET_VSWITCH_SOURCE_VM_ID)
            pString = "FWPS_FIELD_EGRESS_VSWITCH_ETHERNET_VSWITCH_SOURCE_VM_ID";
         else if(fieldID == FWPS_FIELD_EGRESS_VSWITCH_ETHERNET_VSWITCH_DESTINATION_INTERFACE_ID)
            pString = "FWPS_FIELD_EGRESS_VSWITCH_ETHERNET_VSWITCH_DESTINATION_INTERFACE_ID";
         else if(fieldID == FWPS_FIELD_EGRESS_VSWITCH_ETHERNET_VSWITCH_DESTINATION_INTERFACE_TYPE)
            pString = "FWPS_FIELD_EGRESS_VSWITCH_ETHERNET_VSWITCH_DESTINATION_INTERFACE_TYPE";
         else if(fieldID == FWPS_FIELD_EGRESS_VSWITCH_ETHERNET_VSWITCH_DESTINATION_VM_ID)
            pString = "FWPS_FIELD_EGRESS_VSWITCH_ETHERNET_VSWITCH_DESTINATION_VM_ID";
         else if(fieldID == FWPS_FIELD_EGRESS_VSWITCH_ETHERNET_L2_FLAGS)
            pString = "FWPS_FIELD_EGRESS_VSWITCH_ETHERNET_L2_FLAGS";

         break;
      }
      case FWPS_LAYER_INGRESS_VSWITCH_TRANSPORT_V4:
      case FWPS_LAYER_INGRESS_VSWITCH_TRANSPORT_V6:
      {
         if(fieldID == FWPS_FIELD_INGRESS_VSWITCH_TRANSPORT_V4_IP_SOURCE_ADDRESS)
         {
            pString = "FWPM_CONDITION_IP_SOURCE_ADDRESS";

            if(pFormat)
               *pFormat = ADDRESS_VALUE;
         }
         else if(fieldID == FWPS_FIELD_INGRESS_VSWITCH_TRANSPORT_V4_IP_DESTINATION_ADDRESS)
         {
            pString = "FWPM_CONDITION_IP_DESTINATION_ADDRESS";

            if(pFormat)
               *pFormat = ADDRESS_VALUE;
         }
         else if(fieldID == FWPS_FIELD_INGRESS_VSWITCH_TRANSPORT_V4_IP_PROTOCOL)
            pString = "FWPM_CONDITION_IP_PROTOCOL";
         else if(fieldID == FWPS_FIELD_INGRESS_VSWITCH_TRANSPORT_V4_IP_SOURCE_PORT)
            pString = "FWPM_CONDITION_IP_SOURCE_PORT";
         else if(fieldID == FWPS_FIELD_INGRESS_VSWITCH_TRANSPORT_V4_IP_DESTINATION_PORT)
            pString = "FWPM_CONDITION_IP_DESTINATION_PORT";
         else if(fieldID == FWPS_FIELD_INGRESS_VSWITCH_TRANSPORT_V4_VLAN_ID)
            pString = "FWPM_CONDITION_VLAN_ID";
         else if(fieldID == FWPS_FIELD_INGRESS_VSWITCH_TRANSPORT_V4_VSWITCH_TENANT_NETWORK_ID)
            pString = "FWPM_CONDITION_VSWITCH_TENANT_NETWORK_ID";
         else if(fieldID == FWPS_FIELD_INGRESS_VSWITCH_TRANSPORT_V4_VSWITCH_ID)
            pString = "FWPM_CONDITION_VSWITCH_ID";
         else if(fieldID == FWPS_FIELD_INGRESS_VSWITCH_TRANSPORT_V4_VSWITCH_NETWORK_TYPE)
            pString = "FWPM_CONDITION_VSWITCH_NETWORK_TYPE";
         else if(fieldID == FWPS_FIELD_INGRESS_VSWITCH_TRANSPORT_V4_VSWITCH_SOURCE_INTERFACE_ID)
            pString = "FWPM_CONDITION_VSWITCH_SOURCE_INTERFACE_ID";
         else if(fieldID == FWPS_FIELD_INGRESS_VSWITCH_TRANSPORT_V4_VSWITCH_SOURCE_INTERFACE_TYPE)
            pString = "FWPM_CONDITION_VSWITCH_SOURCE_INTERFACE_TYPE";
         else if(fieldID == FWPS_FIELD_INGRESS_VSWITCH_TRANSPORT_V4_VSWITCH_SOURCE_VM_ID)
            pString = "FWPM_CONDITION_VSWITCH_SOURCE_VM_ID";
         else if(fieldID == FWPS_FIELD_INGRESS_VSWITCH_TRANSPORT_V4_L2_FLAGS)
         {
            pString = "FWPM_CONDITION_L2_FLAGS";

            if(pFormat)
               *pFormat = L2_FLAGS_VALUE;
         }

         break;
      }
      case FWPS_LAYER_EGRESS_VSWITCH_TRANSPORT_V4:
      case FWPS_LAYER_EGRESS_VSWITCH_TRANSPORT_V6:
      {
         if(fieldID == FWPS_FIELD_EGRESS_VSWITCH_TRANSPORT_V4_IP_SOURCE_ADDRESS)
         {
            pString = "FWPM_CONDITION_IP_SOURCE_ADDRESS";

            if(pFormat)
               *pFormat = ADDRESS_VALUE;
         }
         else if(fieldID == FWPS_FIELD_EGRESS_VSWITCH_TRANSPORT_V4_IP_DESTINATION_ADDRESS)
         {
            pString = "FWPM_CONDITION_IP_DESTINATION_ADDRESS";

            if(pFormat)
               *pFormat = ADDRESS_VALUE;
         }
         else if(fieldID == FWPS_FIELD_EGRESS_VSWITCH_TRANSPORT_V4_IP_PROTOCOL)
            pString = "FWPM_CONDITION_IP_PROTOCOL";
         else if(fieldID == FWPS_FIELD_EGRESS_VSWITCH_TRANSPORT_V4_IP_SOURCE_PORT)
            pString = "FWPM_CONDITION_IP_SOURCE_PORT";
         else if(fieldID == FWPS_FIELD_EGRESS_VSWITCH_TRANSPORT_V4_IP_DESTINATION_PORT)
            pString = "FWPM_CONDITION_IP_DESTINATION_PORT";
         else if(fieldID == FWPS_FIELD_EGRESS_VSWITCH_TRANSPORT_V4_VLAN_ID)
            pString = "FWPM_CONDITION_VLAN_ID";
         else if(fieldID == FWPS_FIELD_EGRESS_VSWITCH_TRANSPORT_V4_VSWITCH_TENANT_NETWORK_ID)
            pString = "FWPM_CONDITION_VSWITCH_TENANT_NETWORK_ID";
         else if(fieldID == FWPS_FIELD_EGRESS_VSWITCH_TRANSPORT_V4_VSWITCH_ID)
            pString = "FWPM_CONDITION_VSWITCH_ID";
         else if(fieldID == FWPS_FIELD_EGRESS_VSWITCH_TRANSPORT_V4_VSWITCH_NETWORK_TYPE)
            pString = "FWPM_CONDITION_VSWITCH_NETWORK_TYPE";
         else if(fieldID == FWPS_FIELD_EGRESS_VSWITCH_TRANSPORT_V4_VSWITCH_SOURCE_INTERFACE_ID)
            pString = "FWPM_CONDITION_VSWITCH_SOURCE_INTERFACE_ID";
         else if(fieldID == FWPS_FIELD_EGRESS_VSWITCH_TRANSPORT_V4_VSWITCH_SOURCE_INTERFACE_TYPE)
            pString = "FWPM_CONDITION_VSWITCH_SOURCE_INTERFACE_TYPE";
         else if(fieldID == FWPS_FIELD_EGRESS_VSWITCH_TRANSPORT_V4_VSWITCH_SOURCE_VM_ID)
            pString = "FWPM_CONDITION_VSWITCH_SOURCE_VM_ID";
         else if(fieldID == FWPS_FIELD_EGRESS_VSWITCH_TRANSPORT_V4_VSWITCH_DESTINATION_INTERFACE_ID)
            pString = "FWPM_CONDITION_VSWITCH_DESTINATION_INTERFACE_ID";
         else if(fieldID == FWPS_FIELD_EGRESS_VSWITCH_TRANSPORT_V4_VSWITCH_DESTINATION_INTERFACE_TYPE)
            pString = "FWPM_CONDITION_VSWITCH_DESTINATION_INTERFACE_TYPE";
         else if(fieldID == FWPS_FIELD_EGRESS_VSWITCH_TRANSPORT_V4_VSWITCH_DESTINATION_VM_ID)
            pString = "FWPM_CONDITION_VSWITCH_DESTINATION_VM_ID";
         else if(fieldID == FWPS_FIELD_EGRESS_VSWITCH_TRANSPORT_V4_L2_FLAGS)
         {
            pString = "FWPM_CONDITION_L2_FLAGS";

            if(pFormat)
               *pFormat = L2_FLAGS_VALUE;
         }

         break;
      }

#endif /// (NTDDI_VERSION >= NTDDI_WIN8)
#endif /// (NTDDI_VERSION >= NTDDI_WIN7)

   }

   return pString;
}


/**
 @kernel_helper_function="KrnlHlprFwpValueGetFromFwpsIncomingValues"
 
   Purpose:  Retrieve a pointer to the FWP_VALUE for a given condition in the 
             FWPS_INCOMING_VALUES.                                                              <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF552450.aspx             <br>
             HTTP://MSDN.Microsoft.com/En-US/Library/Windwos/Hardware/FF549939.aspx             <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Must_inspect_result_
_Success_(return != 0)
FWP_VALUE* KrnlHlprFwpValueGetFromFwpsIncomingValues(_In_ const FWPS_INCOMING_VALUES* pClassifyValues,
                                                     _In_ const GUID* pConditionKey)
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprFwpValueGetFromFwpsIncomingValues()\n");

#endif /// DBG
   
   NT_ASSERT(pClassifyValues);
   NT_ASSERT(pConditionKey);

   FWP_VALUE* pValue = 0;

   switch(pClassifyValues->layerId)
   {
      case FWPS_LAYER_INBOUND_IPPACKET_V4:
      case FWPS_LAYER_INBOUND_IPPACKET_V4_DISCARD:
      case FWPS_LAYER_INBOUND_IPPACKET_V6:
      case FWPS_LAYER_INBOUND_IPPACKET_V6_DISCARD:
      {
         if(pConditionKey == &FWPM_CONDITION_IP_LOCAL_ADDRESS)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_INBOUND_IPPACKET_V4_IP_LOCAL_ADDRESS].value);
         else if(pConditionKey == &FWPM_CONDITION_IP_REMOTE_ADDRESS)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_INBOUND_IPPACKET_V4_IP_REMOTE_ADDRESS].value);
         else if(pConditionKey == &FWPM_CONDITION_IP_LOCAL_ADDRESS_TYPE)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_INBOUND_IPPACKET_V4_IP_LOCAL_ADDRESS_TYPE].value);
         else if(pConditionKey == &FWPM_CONDITION_IP_LOCAL_INTERFACE)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_INBOUND_IPPACKET_V4_IP_LOCAL_INTERFACE].value);
         else if(pConditionKey == &FWPM_CONDITION_INTERFACE_INDEX)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_INBOUND_IPPACKET_V4_INTERFACE_INDEX].value);
         else if(pConditionKey == &FWPM_CONDITION_SUB_INTERFACE_INDEX)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_INBOUND_IPPACKET_V4_SUB_INTERFACE_INDEX].value);
         else if(pConditionKey == &FWPM_CONDITION_FLAGS)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_INBOUND_IPPACKET_V4_FLAGS].value);
         else if(pConditionKey == &FWPM_CONDITION_INTERFACE_TYPE)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_INBOUND_IPPACKET_V4_INTERFACE_TYPE].value);
         else if(pConditionKey == &FWPM_CONDITION_TUNNEL_TYPE)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_INBOUND_IPPACKET_V4_TUNNEL_TYPE].value);

         break;
      }
      case FWPS_LAYER_OUTBOUND_IPPACKET_V4:
      case FWPS_LAYER_OUTBOUND_IPPACKET_V4_DISCARD:
      case FWPS_LAYER_OUTBOUND_IPPACKET_V6:
      case FWPS_LAYER_OUTBOUND_IPPACKET_V6_DISCARD:
      {
         if(pConditionKey == &FWPM_CONDITION_IP_LOCAL_ADDRESS)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_OUTBOUND_IPPACKET_V4_IP_LOCAL_ADDRESS].value);
         else if(pConditionKey == &FWPM_CONDITION_IP_LOCAL_ADDRESS_TYPE)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_OUTBOUND_IPPACKET_V4_IP_LOCAL_ADDRESS_TYPE].value);
         else if(pConditionKey == &FWPM_CONDITION_IP_REMOTE_ADDRESS)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_OUTBOUND_IPPACKET_V4_IP_REMOTE_ADDRESS].value);
         else if(pConditionKey == &FWPM_CONDITION_IP_LOCAL_INTERFACE)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_OUTBOUND_IPPACKET_V4_IP_LOCAL_INTERFACE].value);
         else if(pConditionKey == &FWPM_CONDITION_INTERFACE_INDEX)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_OUTBOUND_IPPACKET_V4_INTERFACE_INDEX].value);
         else if(pConditionKey == &FWPM_CONDITION_SUB_INTERFACE_INDEX)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_OUTBOUND_IPPACKET_V4_SUB_INTERFACE_INDEX].value);
         else if(pConditionKey == &FWPM_CONDITION_FLAGS)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_OUTBOUND_IPPACKET_V4_FLAGS].value);
         else if(pConditionKey == &FWPM_CONDITION_INTERFACE_TYPE)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_OUTBOUND_IPPACKET_V4_INTERFACE_TYPE].value);
         else if(pConditionKey == &FWPM_CONDITION_TUNNEL_TYPE)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_OUTBOUND_IPPACKET_V4_TUNNEL_TYPE].value);

         break;
      }
      case FWPS_LAYER_IPFORWARD_V4:
      case FWPS_LAYER_IPFORWARD_V4_DISCARD:
      case FWPS_LAYER_IPFORWARD_V6:
      case FWPS_LAYER_IPFORWARD_V6_DISCARD:
      {
         if(pConditionKey == &FWPM_CONDITION_IP_SOURCE_ADDRESS)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_IPFORWARD_V4_IP_SOURCE_ADDRESS].value);
         else if(pConditionKey == &FWPM_CONDITION_IP_DESTINATION_ADDRESS)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_IPFORWARD_V4_IP_DESTINATION_ADDRESS].value);
         else if(pConditionKey == &FWPM_CONDITION_IP_DESTINATION_ADDRESS_TYPE)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_IPFORWARD_V4_IP_DESTINATION_ADDRESS_TYPE].value);
         else if(pConditionKey == &FWPM_CONDITION_IP_LOCAL_INTERFACE)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_IPFORWARD_V4_IP_LOCAL_INTERFACE].value);
         else if(pConditionKey == &FWPM_CONDITION_IP_FORWARD_INTERFACE)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_IPFORWARD_V4_IP_FORWARD_INTERFACE].value);
         else if(pConditionKey == &FWPM_CONDITION_SOURCE_INTERFACE_INDEX)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_IPFORWARD_V4_SOURCE_INTERFACE_INDEX].value);
         else if(pConditionKey == &FWPM_CONDITION_SOURCE_SUB_INTERFACE_INDEX)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_IPFORWARD_V4_SOURCE_SUB_INTERFACE_INDEX].value);
         else if(pConditionKey == &FWPM_CONDITION_DESTINATION_INTERFACE_INDEX)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_IPFORWARD_V4_DESTINATION_INTERFACE_INDEX].value);
         else if(pConditionKey == &FWPM_CONDITION_DESTINATION_SUB_INTERFACE_INDEX)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_IPFORWARD_V4_DESTINATION_SUB_INTERFACE_INDEX].value);
         else if(pConditionKey == &FWPM_CONDITION_FLAGS)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_IPFORWARD_V4_FLAGS].value);

#if(NTDDI_VERSION >= NTDDI_WIN7)

         else if(pConditionKey == &FWPM_CONDITION_IP_PHYSICAL_ARRIVAL_INTERFACE)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_IPFORWARD_V4_IP_PHYSICAL_ARRIVAL_INTERFACE].value);
         else if(pConditionKey == &FWPM_CONDITION_ARRIVAL_INTERFACE_PROFILE_ID)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_IPFORWARD_V4_ARRIVAL_INTERFACE_PROFILE_ID].value);
         else if(pConditionKey == &FWPM_CONDITION_IP_PHYSICAL_NEXTHOP_INTERFACE)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_IPFORWARD_V4_IP_PHYSICAL_NEXTHOP_INTERFACE].value);
         else if(pConditionKey == &FWPM_CONDITION_NEXTHOP_INTERFACE_PROFILE_ID)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_IPFORWARD_V4_NEXTHOP_INTERFACE_PROFILE_ID].value);

#endif /// (NTDDI_VERSION >= NTDDI_WIN7)

         break;
      }
      case FWPS_LAYER_INBOUND_TRANSPORT_V4:
      case FWPS_LAYER_INBOUND_TRANSPORT_V4_DISCARD:
      case FWPS_LAYER_INBOUND_TRANSPORT_V6:
      case FWPS_LAYER_INBOUND_TRANSPORT_V6_DISCARD:
      {
         if(pConditionKey == &FWPM_CONDITION_IP_PROTOCOL)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_INBOUND_TRANSPORT_V4_IP_PROTOCOL].value);
         else if(pConditionKey == &FWPM_CONDITION_IP_LOCAL_ADDRESS)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_INBOUND_TRANSPORT_V4_IP_LOCAL_ADDRESS].value);
         else if(pConditionKey == &FWPM_CONDITION_IP_REMOTE_ADDRESS)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_INBOUND_TRANSPORT_V4_IP_REMOTE_ADDRESS].value);
         else if(pConditionKey == &FWPM_CONDITION_IP_LOCAL_ADDRESS_TYPE)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_INBOUND_TRANSPORT_V4_IP_LOCAL_ADDRESS_TYPE].value);
         else if(pConditionKey == &FWPM_CONDITION_IP_LOCAL_PORT)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_INBOUND_TRANSPORT_V4_IP_LOCAL_PORT].value);
         else if(pConditionKey == &FWPM_CONDITION_IP_REMOTE_PORT)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_INBOUND_TRANSPORT_V4_IP_REMOTE_PORT].value);
         else if(pConditionKey == &FWPM_CONDITION_IP_LOCAL_INTERFACE)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_INBOUND_TRANSPORT_V4_IP_LOCAL_INTERFACE].value);
         else if(pConditionKey == &FWPM_CONDITION_INTERFACE_INDEX)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_INBOUND_TRANSPORT_V4_INTERFACE_INDEX].value);
         else if(pConditionKey == &FWPM_CONDITION_SUB_INTERFACE_INDEX)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_INBOUND_TRANSPORT_V4_SUB_INTERFACE_INDEX].value);
         else if(pConditionKey == &FWPM_CONDITION_FLAGS)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_INBOUND_TRANSPORT_V4_FLAGS].value);
         else if(pConditionKey == &FWPM_CONDITION_INTERFACE_TYPE)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_INBOUND_TRANSPORT_V4_INTERFACE_TYPE].value);
         else if(pConditionKey == &FWPM_CONDITION_TUNNEL_TYPE)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_INBOUND_TRANSPORT_V4_TUNNEL_TYPE].value);

#if(NTDDI_VERSION >= NTDDI_WIN7)

         else if(pConditionKey == &FWPM_CONDITION_CURRENT_PROFILE_ID)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_INBOUND_TRANSPORT_V4_PROFILE_ID].value);

#endif /// (NTDDI_VERSION >= NTDDI_WIN7)

         break;
      }
      case FWPS_LAYER_OUTBOUND_TRANSPORT_V4:
      case FWPS_LAYER_OUTBOUND_TRANSPORT_V4_DISCARD:
      case FWPS_LAYER_OUTBOUND_TRANSPORT_V6:
      case FWPS_LAYER_OUTBOUND_TRANSPORT_V6_DISCARD:
      {
         if(pConditionKey == &FWPM_CONDITION_IP_PROTOCOL)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_OUTBOUND_TRANSPORT_V4_IP_PROTOCOL].value);
         else if(pConditionKey == &FWPM_CONDITION_IP_LOCAL_ADDRESS)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_OUTBOUND_TRANSPORT_V4_IP_LOCAL_ADDRESS].value);
         else if(pConditionKey == &FWPM_CONDITION_IP_LOCAL_ADDRESS_TYPE)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_OUTBOUND_TRANSPORT_V4_IP_LOCAL_ADDRESS_TYPE].value);
         else if(pConditionKey == &FWPM_CONDITION_IP_REMOTE_ADDRESS)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_OUTBOUND_TRANSPORT_V4_IP_REMOTE_ADDRESS].value);
         else if(pConditionKey == &FWPM_CONDITION_IP_LOCAL_PORT)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_OUTBOUND_TRANSPORT_V4_IP_LOCAL_PORT].value);
         else if(pConditionKey == &FWPM_CONDITION_IP_REMOTE_PORT)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_OUTBOUND_TRANSPORT_V4_IP_REMOTE_PORT].value);
         else if(pConditionKey == &FWPM_CONDITION_IP_LOCAL_INTERFACE)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_OUTBOUND_TRANSPORT_V4_IP_LOCAL_INTERFACE].value);
         else if(pConditionKey == &FWPM_CONDITION_INTERFACE_INDEX)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_OUTBOUND_TRANSPORT_V4_INTERFACE_INDEX].value);
         else if(pConditionKey == &FWPM_CONDITION_SUB_INTERFACE_INDEX)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_OUTBOUND_TRANSPORT_V4_SUB_INTERFACE_INDEX].value);
         else if(pConditionKey == &FWPM_CONDITION_IP_DESTINATION_ADDRESS_TYPE)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_OUTBOUND_TRANSPORT_V4_IP_DESTINATION_ADDRESS_TYPE].value);
         else if(pConditionKey == &FWPM_CONDITION_FLAGS)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_OUTBOUND_TRANSPORT_V4_FLAGS].value);
         else if(pConditionKey == &FWPM_CONDITION_INTERFACE_TYPE)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_OUTBOUND_TRANSPORT_V4_INTERFACE_TYPE].value);
         else if(pConditionKey == &FWPM_CONDITION_TUNNEL_TYPE)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_OUTBOUND_TRANSPORT_V4_TUNNEL_TYPE].value);

#if(NTDDI_VERSION >= NTDDI_WIN7)

         else if(pConditionKey == &FWPM_CONDITION_CURRENT_PROFILE_ID)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_OUTBOUND_TRANSPORT_V4_PROFILE_ID].value);

#if(NTDDI_VERSION >= NTDDI_WINTHRESHOLD)
         
         else if(pConditionKey == &FWPM_CONDITION_IPSEC_SECURITY_REALM_ID)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_OUTBOUND_TRANSPORT_V4_IPSEC_SECURITY_REALM_ID].value);
         
#endif /// (NTDDI_VERSION >= NTDDI_WINTHRESHOLD)
#endif /// (NTDDI_VERSION >= NTDDI_WIN7)

         break;
      }
      case FWPS_LAYER_STREAM_V4:
      case FWPS_LAYER_STREAM_V4_DISCARD:
      case FWPS_LAYER_STREAM_V6:
      case FWPS_LAYER_STREAM_V6_DISCARD:
      {
         if(pConditionKey == &FWPM_CONDITION_IP_LOCAL_ADDRESS)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_STREAM_V4_IP_LOCAL_ADDRESS].value);
         else if(pConditionKey == &FWPM_CONDITION_IP_LOCAL_ADDRESS_TYPE)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_STREAM_V4_IP_LOCAL_ADDRESS_TYPE].value);
         else if(pConditionKey == &FWPM_CONDITION_IP_REMOTE_ADDRESS)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_STREAM_V4_IP_REMOTE_ADDRESS].value);
         else if(pConditionKey == &FWPM_CONDITION_IP_LOCAL_PORT)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_STREAM_V4_IP_LOCAL_PORT].value);
         else if(pConditionKey == &FWPM_CONDITION_IP_REMOTE_PORT)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_STREAM_V4_IP_REMOTE_PORT].value);
         else if(pConditionKey == &FWPM_CONDITION_DIRECTION)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_STREAM_V4_DIRECTION].value);

#if(NTDDI_VERSION >= NTDDI_VISTASP1)

         else if(pConditionKey == &FWPM_CONDITION_FLAGS)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_STREAM_V4_FLAGS].value);

#endif /// (NTDDI_VERSION >= NTDDI_VISTASP1)

         break;
      }
      case FWPS_LAYER_DATAGRAM_DATA_V4:
      case FWPS_LAYER_DATAGRAM_DATA_V4_DISCARD:
      case FWPS_LAYER_DATAGRAM_DATA_V6:
      case FWPS_LAYER_DATAGRAM_DATA_V6_DISCARD:
      {
         if(pConditionKey == &FWPM_CONDITION_IP_PROTOCOL)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_DATAGRAM_DATA_V4_IP_PROTOCOL].value);
         else if(pConditionKey == &FWPM_CONDITION_IP_LOCAL_ADDRESS)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_DATAGRAM_DATA_V4_IP_LOCAL_ADDRESS].value);
         else if(pConditionKey == &FWPM_CONDITION_IP_REMOTE_ADDRESS)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_DATAGRAM_DATA_V4_IP_REMOTE_ADDRESS].value);
         else if(pConditionKey == &FWPM_CONDITION_IP_LOCAL_ADDRESS_TYPE)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_DATAGRAM_DATA_V4_IP_LOCAL_ADDRESS_TYPE].value);
         else if(pConditionKey == &FWPM_CONDITION_IP_LOCAL_PORT)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_DATAGRAM_DATA_V4_IP_LOCAL_PORT].value);
         else if(pConditionKey == &FWPM_CONDITION_IP_REMOTE_PORT)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_DATAGRAM_DATA_V4_IP_REMOTE_PORT].value);
         else if(pConditionKey == &FWPM_CONDITION_IP_LOCAL_INTERFACE)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_DATAGRAM_DATA_V4_IP_LOCAL_INTERFACE].value);
         else if(pConditionKey == &FWPM_CONDITION_INTERFACE_INDEX)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_DATAGRAM_DATA_V4_INTERFACE_INDEX].value);
         else if(pConditionKey == &FWPM_CONDITION_SUB_INTERFACE_INDEX)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_DATAGRAM_DATA_V4_SUB_INTERFACE_INDEX].value);
         else if(pConditionKey == &FWPM_CONDITION_DIRECTION)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_DATAGRAM_DATA_V4_DIRECTION].value);
         else if(pConditionKey == &FWPM_CONDITION_FLAGS)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_DATAGRAM_DATA_V4_FLAGS].value);
         else if(pConditionKey == &FWPM_CONDITION_INTERFACE_TYPE)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_DATAGRAM_DATA_V4_INTERFACE_TYPE].value);
         else if(pConditionKey == &FWPM_CONDITION_TUNNEL_TYPE)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_DATAGRAM_DATA_V4_TUNNEL_TYPE].value);

         break;
      }
      case FWPS_LAYER_INBOUND_ICMP_ERROR_V4:
      case FWPS_LAYER_INBOUND_ICMP_ERROR_V4_DISCARD:
      case FWPS_LAYER_INBOUND_ICMP_ERROR_V6:
      case FWPS_LAYER_INBOUND_ICMP_ERROR_V6_DISCARD:
      {
         if(pConditionKey == &FWPM_CONDITION_EMBEDDED_PROTOCOL)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_INBOUND_ICMP_ERROR_V4_EMBEDDED_PROTOCOL].value);
         else if(pConditionKey == &FWPM_CONDITION_IP_LOCAL_ADDRESS)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_INBOUND_ICMP_ERROR_V4_IP_LOCAL_ADDRESS].value);
         else if(pConditionKey == &FWPM_CONDITION_IP_REMOTE_ADDRESS)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_INBOUND_ICMP_ERROR_V4_IP_REMOTE_ADDRESS].value);
         else if(pConditionKey == &FWPM_CONDITION_EMBEDDED_REMOTE_ADDRESS)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_INBOUND_ICMP_ERROR_V4_EMBEDDED_REMOTE_ADDRESS].value);
         else if(pConditionKey == &FWPM_CONDITION_EMBEDDED_LOCAL_ADDRESS_TYPE)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_INBOUND_ICMP_ERROR_V4_EMBEDDED_LOCAL_ADDRESS_TYPE].value);
         else if(pConditionKey == &FWPM_CONDITION_EMBEDDED_LOCAL_PORT)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_INBOUND_ICMP_ERROR_V4_EMBEDDED_LOCAL_PORT].value);
         else if(pConditionKey == &FWPM_CONDITION_EMBEDDED_REMOTE_PORT)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_INBOUND_ICMP_ERROR_V4_EMBEDDED_REMOTE_PORT].value);
         else if(pConditionKey == &FWPM_CONDITION_IP_LOCAL_INTERFACE)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_INBOUND_ICMP_ERROR_V4_IP_LOCAL_INTERFACE].value);
         else if(pConditionKey == &FWPM_CONDITION_ICMP_TYPE)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_INBOUND_ICMP_ERROR_V4_ICMP_TYPE].value);
         else if(pConditionKey == &FWPM_CONDITION_ICMP_CODE)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_INBOUND_ICMP_ERROR_V4_ICMP_CODE].value);
         else if(pConditionKey == &FWPM_CONDITION_INTERFACE_INDEX)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_INBOUND_ICMP_ERROR_V4_INTERFACE_INDEX].value);
         else if(pConditionKey == &FWPM_CONDITION_SUB_INTERFACE_INDEX)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_INBOUND_ICMP_ERROR_V4_SUB_INTERFACE_INDEX].value);
         else if(pConditionKey == &FWPM_CONDITION_INTERFACE_TYPE)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_INBOUND_ICMP_ERROR_V4_INTERFACE_TYPE].value);
         else if(pConditionKey == &FWPM_CONDITION_TUNNEL_TYPE)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_INBOUND_ICMP_ERROR_V4_TUNNEL_TYPE].value);

#if(NTDDI_VERSION >= NTDDI_VISTASP1)

         else if(pConditionKey == &FWPM_CONDITION_IP_ARRIVAL_INTERFACE)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_INBOUND_ICMP_ERROR_V4_IP_ARRIVAL_INTERFACE].value);
         else if(pConditionKey == &FWPM_CONDITION_ARRIVAL_INTERFACE_INDEX)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_INBOUND_ICMP_ERROR_V4_ARRIVAL_INTERFACE_INDEX].value);
         else if(pConditionKey == &FWPM_CONDITION_ARRIVAL_INTERFACE_TYPE)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_INBOUND_ICMP_ERROR_V4_ARRIVAL_INTERFACE_TYPE].value);
         else if(pConditionKey == &FWPM_CONDITION_ARRIVAL_TUNNEL_TYPE)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_INBOUND_ICMP_ERROR_V4_ARRIVAL_TUNNEL_TYPE].value);
         else if(pConditionKey == &FWPM_CONDITION_FLAGS)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_INBOUND_ICMP_ERROR_V4_FLAGS].value);

#if(NTDDI_VERSION >= NTDDI_WIN7)

         else if(pConditionKey == &FWPM_CONDITION_ARRIVAL_INTERFACE_PROFILE_ID)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_INBOUND_ICMP_ERROR_V4_ARRIVAL_INTERFACE_PROFILE_ID].value);
         else if(pConditionKey == &FWPM_CONDITION_INTERFACE_QUARANTINE_EPOCH)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_INBOUND_ICMP_ERROR_V4_INTERFACE_QUARANTINE_EPOCH].value);

#endif /// (NTDDI_VERSION >= NTDDI_WIN7)
#endif /// (NTDDI_VERSION >= NTDDI_VISTASP1)

         break;
      }
      case FWPS_LAYER_OUTBOUND_ICMP_ERROR_V4:
      case FWPS_LAYER_OUTBOUND_ICMP_ERROR_V4_DISCARD:
      case FWPS_LAYER_OUTBOUND_ICMP_ERROR_V6:
      case FWPS_LAYER_OUTBOUND_ICMP_ERROR_V6_DISCARD:
      {
         if(pConditionKey == &FWPM_CONDITION_IP_LOCAL_ADDRESS)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_OUTBOUND_ICMP_ERROR_V4_IP_LOCAL_ADDRESS].value);
         else if(pConditionKey == &FWPM_CONDITION_IP_REMOTE_ADDRESS)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_OUTBOUND_ICMP_ERROR_V4_IP_REMOTE_ADDRESS].value);
         else if(pConditionKey == &FWPM_CONDITION_IP_LOCAL_ADDRESS_TYPE)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_OUTBOUND_ICMP_ERROR_V4_IP_LOCAL_ADDRESS_TYPE].value);
         else if(pConditionKey == &FWPM_CONDITION_IP_LOCAL_INTERFACE)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_OUTBOUND_ICMP_ERROR_V4_IP_LOCAL_INTERFACE].value);
         else if(pConditionKey == &FWPM_CONDITION_ICMP_TYPE)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_OUTBOUND_ICMP_ERROR_V4_ICMP_TYPE].value);
         else if(pConditionKey == &FWPM_CONDITION_ICMP_CODE)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_OUTBOUND_ICMP_ERROR_V4_ICMP_CODE].value);
         else if(pConditionKey == &FWPM_CONDITION_INTERFACE_INDEX)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_OUTBOUND_ICMP_ERROR_V4_INTERFACE_INDEX].value);
         else if(pConditionKey == &FWPM_CONDITION_SUB_INTERFACE_INDEX)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_OUTBOUND_ICMP_ERROR_V4_SUB_INTERFACE_INDEX].value);
         else if(pConditionKey == &FWPM_CONDITION_INTERFACE_TYPE)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_OUTBOUND_ICMP_ERROR_V4_INTERFACE_TYPE].value);
         else if(pConditionKey == &FWPM_CONDITION_TUNNEL_TYPE)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_OUTBOUND_ICMP_ERROR_V4_TUNNEL_TYPE].value);

#if(NTDDI_VERSION >= NTDDI_VISTASP1)

         else if(pConditionKey == &FWPM_CONDITION_FLAGS)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_OUTBOUND_ICMP_ERROR_V4_FLAGS].value);

#if(NTDDI_VERSION >= NTDDI_WIN7)

         else if(pConditionKey == &FWPM_CONDITION_NEXTHOP_INTERFACE_PROFILE_ID)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_OUTBOUND_ICMP_ERROR_V4_NEXTHOP_INTERFACE_PROFILE_ID].value);
         else if(pConditionKey == &FWPM_CONDITION_INTERFACE_QUARANTINE_EPOCH)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_OUTBOUND_ICMP_ERROR_V4_INTERFACE_QUARANTINE_EPOCH].value);

#endif /// (NTDDI_VERSION >= NTDDI_WIN7)
#endif /// (NTDDI_VERSION >= NTDDI_VISTASP1)

         break;
      }
      case FWPS_LAYER_ALE_RESOURCE_ASSIGNMENT_V4:
      case FWPS_LAYER_ALE_RESOURCE_ASSIGNMENT_V4_DISCARD:
      case FWPS_LAYER_ALE_RESOURCE_ASSIGNMENT_V6:
      case FWPS_LAYER_ALE_RESOURCE_ASSIGNMENT_V6_DISCARD:
      {
         if(pConditionKey == &FWPM_CONDITION_ALE_APP_ID)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_RESOURCE_ASSIGNMENT_V4_ALE_APP_ID].value);
         else if(pConditionKey == &FWPM_CONDITION_ALE_USER_ID)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_RESOURCE_ASSIGNMENT_V4_ALE_USER_ID].value);
         else if(pConditionKey == &FWPM_CONDITION_IP_LOCAL_ADDRESS)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_RESOURCE_ASSIGNMENT_V4_IP_LOCAL_ADDRESS].value);
         else if(pConditionKey == &FWPM_CONDITION_IP_LOCAL_ADDRESS_TYPE)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_RESOURCE_ASSIGNMENT_V4_IP_LOCAL_ADDRESS_TYPE].value);
         else if(pConditionKey == &FWPM_CONDITION_IP_LOCAL_PORT)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_RESOURCE_ASSIGNMENT_V4_IP_LOCAL_PORT].value);
         else if(pConditionKey == &FWPM_CONDITION_IP_PROTOCOL)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_RESOURCE_ASSIGNMENT_V4_IP_PROTOCOL].value);
         else if(pConditionKey == &FWPM_CONDITION_ALE_PROMISCUOUS_MODE)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_RESOURCE_ASSIGNMENT_V4_ALE_PROMISCUOUS_MODE].value);
         else if(pConditionKey == &FWPM_CONDITION_IP_LOCAL_INTERFACE)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_RESOURCE_ASSIGNMENT_V4_IP_LOCAL_INTERFACE].value);
         else if(pConditionKey == &FWPM_CONDITION_FLAGS)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_RESOURCE_ASSIGNMENT_V4_FLAGS].value);
         else if(pConditionKey == &FWPM_CONDITION_INTERFACE_TYPE)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_RESOURCE_ASSIGNMENT_V4_INTERFACE_TYPE].value);
         else if(pConditionKey == &FWPM_CONDITION_TUNNEL_TYPE)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_RESOURCE_ASSIGNMENT_V4_TUNNEL_TYPE].value);

#if(NTDDI_VERSION >= NTDDI_WIN7)

         else if(pConditionKey == &FWPM_CONDITION_LOCAL_INTERFACE_PROFILE_ID)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_RESOURCE_ASSIGNMENT_V4_LOCAL_INTERFACE_PROFILE_ID].value);
         else if(pConditionKey == &FWPM_CONDITION_ALE_SIO_FIREWALL_SOCKET_PROPERTY)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_RESOURCE_ASSIGNMENT_V4_SIO_FIREWALL_SOCKET_PROPERTY].value);

#if(NTDDI_VERSION >= NTDDI_WIN8)

         else if(pConditionKey == &FWPM_CONDITION_ALE_PACKAGE_ID)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_RESOURCE_ASSIGNMENT_V4_ALE_PACKAGE_ID].value);

#if(NTDDI_VERSION >= NTDDI_WINTHRESHOLD)
                  
         else if(pConditionKey == &FWPM_CONDITION_ALE_SECURITY_ATTRIBUTE_FQBN_VALUE)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_RESOURCE_ASSIGNMENT_V4_ALE_SECURITY_ATTRIBUTE_FQBN_VALUE].value);
                  
#endif /// (NTDDI_VERSION >= NTDDI_WINTHRESHOLD)
#endif /// (NTDDI_VERSION >= NTDDI_WIN8)
#endif /// (NTDDI_VERSION >= NTDDI_WIN7)

         break;
      }
      case FWPS_LAYER_ALE_AUTH_LISTEN_V4:
      case FWPS_LAYER_ALE_AUTH_LISTEN_V4_DISCARD:
      case FWPS_LAYER_ALE_AUTH_LISTEN_V6:
      case FWPS_LAYER_ALE_AUTH_LISTEN_V6_DISCARD:
      {
         if(pConditionKey == &FWPM_CONDITION_ALE_APP_ID)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_AUTH_LISTEN_V4_ALE_APP_ID].value);
         else if(pConditionKey == &FWPM_CONDITION_ALE_USER_ID)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_AUTH_LISTEN_V4_ALE_USER_ID].value);
         else if(pConditionKey == &FWPM_CONDITION_IP_LOCAL_ADDRESS)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_AUTH_LISTEN_V4_IP_LOCAL_ADDRESS].value);
         else if(pConditionKey == &FWPM_CONDITION_IP_LOCAL_ADDRESS_TYPE)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_AUTH_LISTEN_V4_IP_LOCAL_ADDRESS_TYPE].value);
         else if(pConditionKey == &FWPM_CONDITION_IP_LOCAL_PORT)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_AUTH_LISTEN_V4_IP_LOCAL_PORT].value);
         else if(pConditionKey == &FWPM_CONDITION_IP_LOCAL_INTERFACE)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_AUTH_LISTEN_V4_IP_LOCAL_INTERFACE].value);
         else if(pConditionKey == &FWPM_CONDITION_FLAGS)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_AUTH_LISTEN_V4_FLAGS].value);
         else if(pConditionKey == &FWPM_CONDITION_INTERFACE_TYPE)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_AUTH_LISTEN_V4_INTERFACE_TYPE].value);
         else if(pConditionKey == &FWPM_CONDITION_TUNNEL_TYPE)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_AUTH_LISTEN_V4_TUNNEL_TYPE].value);

#if(NTDDI_VERSION >= NTDDI_WIN7)

         else if(pConditionKey == &FWPM_CONDITION_LOCAL_INTERFACE_PROFILE_ID)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_AUTH_LISTEN_V4_LOCAL_INTERFACE_PROFILE_ID].value);
         else if(pConditionKey == &FWPM_CONDITION_ALE_SIO_FIREWALL_SOCKET_PROPERTY)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_AUTH_LISTEN_V4_SIO_FIREWALL_SOCKET_PROPERTY].value);

#if(NTDDI_VERSION >= NTDDI_WIN8)

         else if(pConditionKey == &FWPM_CONDITION_ALE_PACKAGE_ID)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_AUTH_LISTEN_V4_ALE_PACKAGE_ID].value);

#if(NTDDI_VERSION >= NTDDI_WINTHRESHOLD)
                  
         else if(pConditionKey == &FWPM_CONDITION_ALE_SECURITY_ATTRIBUTE_FQBN_VALUE)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_AUTH_LISTEN_V4_ALE_SECURITY_ATTRIBUTE_FQBN_VALUE].value);
                  
#endif /// (NTDDI_VERSION >= NTDDI_WINTHRESHOLD)
#endif /// (NTDDI_VERSION >= NTDDI_WIN8)
#endif /// (NTDDI_VERSION >= NTDDI_WIN7)

         break;
      }
      case FWPS_LAYER_ALE_AUTH_RECV_ACCEPT_V4:
      case FWPS_LAYER_ALE_AUTH_RECV_ACCEPT_V4_DISCARD:
      case FWPS_LAYER_ALE_AUTH_RECV_ACCEPT_V6:
      case FWPS_LAYER_ALE_AUTH_RECV_ACCEPT_V6_DISCARD:
      {
         if(pConditionKey == &FWPM_CONDITION_ALE_APP_ID)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V4_ALE_APP_ID].value);
         else if(pConditionKey == &FWPM_CONDITION_ALE_USER_ID)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V4_ALE_USER_ID].value);
         else if(pConditionKey == &FWPM_CONDITION_IP_LOCAL_ADDRESS)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V4_IP_LOCAL_ADDRESS].value);
         else if(pConditionKey == &FWPM_CONDITION_IP_LOCAL_ADDRESS_TYPE)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V4_IP_LOCAL_ADDRESS_TYPE].value);
         else if(pConditionKey == &FWPM_CONDITION_IP_LOCAL_PORT)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V4_IP_LOCAL_PORT].value);
         else if(pConditionKey == &FWPM_CONDITION_IP_PROTOCOL)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V4_IP_PROTOCOL].value);
         else if(pConditionKey == &FWPM_CONDITION_IP_REMOTE_ADDRESS)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V4_IP_REMOTE_ADDRESS].value);
         else if(pConditionKey == &FWPM_CONDITION_IP_REMOTE_PORT)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V4_IP_REMOTE_PORT].value);
         else if(pConditionKey == &FWPM_CONDITION_ALE_REMOTE_USER_ID)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V4_ALE_REMOTE_USER_ID].value);
         else if(pConditionKey == &FWPM_CONDITION_ALE_REMOTE_MACHINE_ID)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V4_ALE_REMOTE_MACHINE_ID].value);
         else if(pConditionKey == &FWPM_CONDITION_IP_LOCAL_INTERFACE)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V4_IP_LOCAL_INTERFACE].value);
         else if(pConditionKey == &FWPM_CONDITION_FLAGS)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V4_FLAGS].value);
         else if(pConditionKey == &FWPM_CONDITION_ALE_SIO_FIREWALL_SYSTEM_PORT)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V4_SIO_FIREWALL_SYSTEM_PORT].value);
         else if(pConditionKey == &FWPM_CONDITION_ALE_NAP_CONTEXT)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V4_NAP_CONTEXT].value);
         else if(pConditionKey == &FWPM_CONDITION_INTERFACE_TYPE)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V4_INTERFACE_TYPE].value);
         else if(pConditionKey == &FWPM_CONDITION_TUNNEL_TYPE)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V4_TUNNEL_TYPE].value);
         else if(pConditionKey == &FWPM_CONDITION_INTERFACE_INDEX)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V4_INTERFACE_INDEX].value);
         else if(pConditionKey == &FWPM_CONDITION_SUB_INTERFACE_INDEX)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V4_SUB_INTERFACE_INDEX].value);

#if(NTDDI_VERSION >= NTDDI_VISTASP1)

         else if(pConditionKey == &FWPM_CONDITION_IP_ARRIVAL_INTERFACE)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V4_IP_ARRIVAL_INTERFACE].value);
         else if(pConditionKey == &FWPM_CONDITION_ARRIVAL_INTERFACE_TYPE)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V4_ARRIVAL_INTERFACE_TYPE].value);
         else if(pConditionKey == &FWPM_CONDITION_ARRIVAL_TUNNEL_TYPE)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V4_ARRIVAL_TUNNEL_TYPE].value);
         else if(pConditionKey == &FWPM_CONDITION_ARRIVAL_INTERFACE_INDEX)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V4_ARRIVAL_INTERFACE_INDEX].value);

#if(NTDDI_VERSION >= NTDDI_WIN7)

         else if(pConditionKey == &FWPM_CONDITION_NEXTHOP_SUB_INTERFACE_INDEX)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V4_NEXTHOP_SUB_INTERFACE_INDEX].value);
         else if(pConditionKey == &FWPM_CONDITION_IP_NEXTHOP_INTERFACE)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V4_IP_NEXTHOP_INTERFACE].value);
         else if(pConditionKey == &FWPM_CONDITION_NEXTHOP_INTERFACE_TYPE)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V4_NEXTHOP_INTERFACE_TYPE].value);
         else if(pConditionKey == &FWPM_CONDITION_NEXTHOP_TUNNEL_TYPE)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V4_NEXTHOP_TUNNEL_TYPE].value);
         else if(pConditionKey == &FWPM_CONDITION_NEXTHOP_INTERFACE_INDEX)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V4_NEXTHOP_INTERFACE_INDEX].value);
         else if(pConditionKey == &FWPM_CONDITION_ORIGINAL_PROFILE_ID)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V4_ORIGINAL_PROFILE_ID].value);
         else if(pConditionKey == &FWPM_CONDITION_CURRENT_PROFILE_ID)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V4_CURRENT_PROFILE_ID].value);
         else if(pConditionKey == &FWPM_CONDITION_REAUTHORIZE_REASON)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V4_REAUTHORIZE_REASON].value);
         else if(pConditionKey == &FWPM_CONDITION_ORIGINAL_ICMP_TYPE)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V4_ORIGINAL_ICMP_TYPE].value);
         else if(pConditionKey == &FWPM_CONDITION_INTERFACE_QUARANTINE_EPOCH)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V4_INTERFACE_QUARANTINE_EPOCH].value);

#if(NTDDI_VERSION >= NTDDI_WIN8)

         else if(pConditionKey == &FWPM_CONDITION_ALE_PACKAGE_ID)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V4_ALE_PACKAGE_ID].value);

#if(NTDDI_VERSION >= NTDDI_WINTHRESHOLD)
                  
         else if(pConditionKey == &FWPM_CONDITION_ALE_SECURITY_ATTRIBUTE_FQBN_VALUE)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V4_ALE_SECURITY_ATTRIBUTE_FQBN_VALUE].value);
                  
#endif /// (NTDDI_VERSION >= NTDDI_WINTHRESHOLD)
#endif /// (NTDDI_VERSION >= NTDDI_WIN8)
#endif /// (NTDDI_VERSION >= NTDDI_WIN7)
#endif /// (NTDDI_VERSION >= NTDDI_VISTASP1)

         break;
      }
      case FWPS_LAYER_ALE_AUTH_CONNECT_V4:
      case FWPS_LAYER_ALE_AUTH_CONNECT_V4_DISCARD:
      case FWPS_LAYER_ALE_AUTH_CONNECT_V6:
      case FWPS_LAYER_ALE_AUTH_CONNECT_V6_DISCARD:
      {
         if(pConditionKey == &FWPM_CONDITION_ALE_APP_ID)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_AUTH_CONNECT_V4_ALE_APP_ID].value);
         else if(pConditionKey == &FWPM_CONDITION_ALE_USER_ID)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_AUTH_CONNECT_V4_ALE_USER_ID].value);
         else if(pConditionKey == &FWPM_CONDITION_IP_LOCAL_ADDRESS)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_AUTH_CONNECT_V4_IP_LOCAL_ADDRESS].value);
         else if(pConditionKey == &FWPM_CONDITION_IP_LOCAL_ADDRESS_TYPE)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_AUTH_CONNECT_V4_IP_LOCAL_ADDRESS_TYPE].value);
         else if(pConditionKey == &FWPM_CONDITION_IP_LOCAL_PORT)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_AUTH_CONNECT_V4_IP_LOCAL_PORT].value);
         else if(pConditionKey == &FWPM_CONDITION_IP_PROTOCOL)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_AUTH_CONNECT_V4_IP_PROTOCOL].value);
         else if(pConditionKey == &FWPM_CONDITION_IP_REMOTE_ADDRESS)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_AUTH_CONNECT_V4_IP_REMOTE_ADDRESS].value);
         else if(pConditionKey == &FWPM_CONDITION_IP_REMOTE_PORT)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_AUTH_CONNECT_V4_IP_REMOTE_PORT].value);
         else if(pConditionKey == &FWPM_CONDITION_ALE_REMOTE_USER_ID)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_AUTH_CONNECT_V4_ALE_REMOTE_USER_ID].value);
         else if(pConditionKey == &FWPM_CONDITION_ALE_REMOTE_MACHINE_ID)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_AUTH_CONNECT_V4_ALE_REMOTE_MACHINE_ID].value);
         else if(pConditionKey == &FWPM_CONDITION_IP_DESTINATION_ADDRESS_TYPE)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_AUTH_CONNECT_V4_IP_DESTINATION_ADDRESS_TYPE].value);
         else if(pConditionKey == &FWPM_CONDITION_IP_LOCAL_INTERFACE)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_AUTH_CONNECT_V4_IP_LOCAL_INTERFACE].value);
         else if(pConditionKey == &FWPM_CONDITION_FLAGS)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_AUTH_CONNECT_V4_FLAGS].value);
         else if(pConditionKey == &FWPM_CONDITION_INTERFACE_TYPE)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_AUTH_CONNECT_V4_INTERFACE_TYPE].value);
         else if(pConditionKey == &FWPM_CONDITION_TUNNEL_TYPE)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_AUTH_CONNECT_V4_TUNNEL_TYPE].value);

#if(NTDDI_VERSION >= NTDDI_VISTASP1)

         else if(pConditionKey == &FWPM_CONDITION_INTERFACE_INDEX)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_AUTH_CONNECT_V4_INTERFACE_INDEX].value);
         else if(pConditionKey == &FWPM_CONDITION_SUB_INTERFACE_INDEX)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_AUTH_CONNECT_V4_SUB_INTERFACE_INDEX].value);

#if(NTDDI_VERSION >= NTDDI_WIN7)

         else if(pConditionKey == &FWPM_CONDITION_IP_ARRIVAL_INTERFACE)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_AUTH_CONNECT_V4_IP_ARRIVAL_INTERFACE].value);
         else if(pConditionKey == &FWPM_CONDITION_ARRIVAL_INTERFACE_TYPE)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_AUTH_CONNECT_V4_ARRIVAL_INTERFACE_TYPE].value);
         else if(pConditionKey == &FWPM_CONDITION_ARRIVAL_TUNNEL_TYPE)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_AUTH_CONNECT_V4_ARRIVAL_TUNNEL_TYPE].value);
         else if(pConditionKey == &FWPM_CONDITION_ARRIVAL_INTERFACE_INDEX)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_AUTH_CONNECT_V4_ARRIVAL_INTERFACE_INDEX].value);
         else if(pConditionKey == &FWPM_CONDITION_NEXTHOP_SUB_INTERFACE_INDEX)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_AUTH_CONNECT_V4_NEXTHOP_SUB_INTERFACE_INDEX].value);
         else if(pConditionKey == &FWPM_CONDITION_IP_NEXTHOP_INTERFACE)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_AUTH_CONNECT_V4_IP_NEXTHOP_INTERFACE].value);
         else if(pConditionKey == &FWPM_CONDITION_NEXTHOP_INTERFACE_TYPE)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_AUTH_CONNECT_V4_NEXTHOP_INTERFACE_TYPE].value);
         else if(pConditionKey == &FWPM_CONDITION_NEXTHOP_TUNNEL_TYPE)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_AUTH_CONNECT_V4_NEXTHOP_TUNNEL_TYPE].value);
         else if(pConditionKey == &FWPM_CONDITION_NEXTHOP_INTERFACE_INDEX)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_AUTH_CONNECT_V4_NEXTHOP_INTERFACE_INDEX].value);
         else if(pConditionKey == &FWPM_CONDITION_ORIGINAL_PROFILE_ID)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_AUTH_CONNECT_V4_ORIGINAL_PROFILE_ID].value);
         else if(pConditionKey == &FWPM_CONDITION_CURRENT_PROFILE_ID)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_AUTH_CONNECT_V4_CURRENT_PROFILE_ID].value);
         else if(pConditionKey == &FWPM_CONDITION_REAUTHORIZE_REASON)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_AUTH_CONNECT_V4_REAUTHORIZE_REASON].value);
         else if(pConditionKey == &FWPM_CONDITION_PEER_NAME)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_AUTH_CONNECT_V4_PEER_NAME].value);
         else if(pConditionKey == &FWPM_CONDITION_ORIGINAL_ICMP_TYPE)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_AUTH_CONNECT_V4_ORIGINAL_ICMP_TYPE].value);
         else if(pConditionKey == &FWPM_CONDITION_INTERFACE_QUARANTINE_EPOCH)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_AUTH_CONNECT_V4_INTERFACE_QUARANTINE_EPOCH].value);

#if(NTDDI_VERSION >= NTDDI_WIN8)

         else if(pConditionKey == &FWPM_CONDITION_ALE_ORIGINAL_APP_ID)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_AUTH_CONNECT_V4_ALE_ORIGINAL_APP_ID].value);
         else if(pConditionKey == &FWPM_CONDITION_ALE_PACKAGE_ID)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_AUTH_CONNECT_V4_ALE_PACKAGE_ID].value);

#if(NTDDI_VERSION >= NTDDI_WINTHRESHOLD)
                  
         else if(pConditionKey == &FWPM_CONDITION_ALE_SECURITY_ATTRIBUTE_FQBN_VALUE)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_AUTH_CONNECT_V4_ALE_SECURITY_ATTRIBUTE_FQBN_VALUE].value);
                  
#endif /// (NTDDI_VERSION >= NTDDI_WINTHRESHOLD)
#endif /// (NTDDI_VERSION >= NTDDI_WIN8)
#endif /// (NTDDI_VERSION >= NTDDI_WIN7)
#endif /// (NTDDI_VERSION >= NTDDI_VISTASP1)

         break;
      }
      case FWPS_LAYER_ALE_FLOW_ESTABLISHED_V4:
      case FWPS_LAYER_ALE_FLOW_ESTABLISHED_V4_DISCARD:
      case FWPS_LAYER_ALE_FLOW_ESTABLISHED_V6:
      case FWPS_LAYER_ALE_FLOW_ESTABLISHED_V6_DISCARD:
      {
         if(pConditionKey == &FWPM_CONDITION_ALE_APP_ID)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_FLOW_ESTABLISHED_V4_ALE_APP_ID].value);
         else if(pConditionKey == &FWPM_CONDITION_ALE_USER_ID)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_FLOW_ESTABLISHED_V4_ALE_USER_ID].value);
         else if(pConditionKey == &FWPM_CONDITION_IP_LOCAL_ADDRESS)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_FLOW_ESTABLISHED_V4_IP_LOCAL_ADDRESS].value);
         else if(pConditionKey == &FWPM_CONDITION_IP_LOCAL_ADDRESS_TYPE)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_FLOW_ESTABLISHED_V4_IP_LOCAL_ADDRESS_TYPE].value);
         else if(pConditionKey == &FWPM_CONDITION_IP_LOCAL_PORT)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_FLOW_ESTABLISHED_V4_IP_LOCAL_PORT].value);
         else if(pConditionKey == &FWPM_CONDITION_IP_PROTOCOL)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_FLOW_ESTABLISHED_V4_IP_PROTOCOL].value);
         else if(pConditionKey == &FWPM_CONDITION_IP_REMOTE_ADDRESS)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_FLOW_ESTABLISHED_V4_IP_REMOTE_ADDRESS].value);
         else if(pConditionKey == &FWPM_CONDITION_IP_REMOTE_PORT)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_FLOW_ESTABLISHED_V4_IP_REMOTE_PORT].value);
         else if(pConditionKey == &FWPM_CONDITION_ALE_REMOTE_USER_ID)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_FLOW_ESTABLISHED_V4_ALE_REMOTE_USER_ID].value);
         else if(pConditionKey == &FWPM_CONDITION_ALE_REMOTE_MACHINE_ID)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_FLOW_ESTABLISHED_V4_ALE_REMOTE_MACHINE_ID].value);
         else if(pConditionKey == &FWPM_CONDITION_IP_DESTINATION_ADDRESS_TYPE)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_FLOW_ESTABLISHED_V4_IP_DESTINATION_ADDRESS_TYPE].value);
         else if(pConditionKey == &FWPM_CONDITION_IP_LOCAL_INTERFACE)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_FLOW_ESTABLISHED_V4_IP_LOCAL_INTERFACE].value);
         else if(pConditionKey == &FWPM_CONDITION_DIRECTION)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_FLOW_ESTABLISHED_V4_DIRECTION].value);
         else if(pConditionKey == &FWPM_CONDITION_INTERFACE_TYPE)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_FLOW_ESTABLISHED_V4_INTERFACE_TYPE].value);
         else if(pConditionKey == &FWPM_CONDITION_TUNNEL_TYPE)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_FLOW_ESTABLISHED_V4_TUNNEL_TYPE].value);

#if(NTDDI_VERSION >= NTDDI_VISTASP1)

         else if(pConditionKey == &FWPM_CONDITION_FLAGS)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_FLOW_ESTABLISHED_V4_FLAGS].value);

#if(NTDDI_VERSION >= NTDDI_WIN8)

         else if(pConditionKey == &FWPM_CONDITION_ALE_ORIGINAL_APP_ID)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_FLOW_ESTABLISHED_V4_ALE_ORIGINAL_APP_ID].value);
         else if(pConditionKey == &FWPM_CONDITION_ALE_PACKAGE_ID)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_FLOW_ESTABLISHED_V4_ALE_PACKAGE_ID].value);

#if(NTDDI_VERSION >= NTDDI_WINTHRESHOLD)
                  
         else if(pConditionKey == &FWPM_CONDITION_ALE_SECURITY_ATTRIBUTE_FQBN_VALUE)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_FLOW_ESTABLISHED_V4_ALE_SECURITY_ATTRIBUTE_FQBN_VALUE].value);
                  
#endif /// (NTDDI_VERSION >= NTDDI_WINTHRESHOLD)
#endif /// (NTDDI_VERSION >= NTDDI_WIN8)
#endif /// (NTDDI_VERSION >= NTDDI_VISTASP1)

         break;
      }

#if(NTDDI_VERSION >= NTDDI_WIN7)

      case FWPS_LAYER_NAME_RESOLUTION_CACHE_V4:
      case FWPS_LAYER_NAME_RESOLUTION_CACHE_V6:
      {
         if(pConditionKey == &FWPM_CONDITION_ALE_USER_ID)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_NAME_RESOLUTION_CACHE_V4_ALE_USER_ID].value);
         else if(pConditionKey == &FWPM_CONDITION_ALE_APP_ID)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_NAME_RESOLUTION_CACHE_V4_ALE_APP_ID].value);
         else if(pConditionKey == &FWPM_CONDITION_IP_REMOTE_ADDRESS)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_NAME_RESOLUTION_CACHE_V4_IP_REMOTE_ADDRESS].value);
         else if(pConditionKey == &FWPM_CONDITION_PEER_NAME)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_NAME_RESOLUTION_CACHE_V4_PEER_NAME].value);

         break;
      }
      case FWPS_LAYER_ALE_RESOURCE_RELEASE_V4:
      case FWPS_LAYER_ALE_RESOURCE_RELEASE_V6:
      {
         if(pConditionKey == &FWPM_CONDITION_ALE_APP_ID)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_RESOURCE_RELEASE_V4_ALE_APP_ID].value);
         else if(pConditionKey == &FWPM_CONDITION_ALE_USER_ID)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_RESOURCE_RELEASE_V4_ALE_USER_ID].value);
         else if(pConditionKey == &FWPM_CONDITION_IP_LOCAL_ADDRESS)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_RESOURCE_RELEASE_V4_IP_LOCAL_ADDRESS].value);
         else if(pConditionKey == &FWPM_CONDITION_IP_LOCAL_ADDRESS_TYPE)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_RESOURCE_RELEASE_V4_IP_LOCAL_ADDRESS_TYPE].value);
         else if(pConditionKey == &FWPM_CONDITION_IP_LOCAL_PORT)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_RESOURCE_RELEASE_V4_IP_LOCAL_PORT].value);
         else if(pConditionKey == &FWPM_CONDITION_IP_PROTOCOL)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_RESOURCE_RELEASE_V4_IP_PROTOCOL].value);
         else if(pConditionKey == &FWPM_CONDITION_IP_LOCAL_INTERFACE)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_RESOURCE_RELEASE_V4_IP_LOCAL_INTERFACE].value);
         else if(pConditionKey == &FWPM_CONDITION_FLAGS)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_RESOURCE_RELEASE_V4_FLAGS].value);

#if(NTDDI_VERSION >= NTDDI_WIN8)

         else if(pConditionKey == &FWPM_CONDITION_ALE_PACKAGE_ID)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_RESOURCE_RELEASE_V4_ALE_PACKAGE_ID].value);

#if(NTDDI_VERSION >= NTDDI_WINTHRESHOLD)
                  
         else if(pConditionKey == &FWPM_CONDITION_ALE_SECURITY_ATTRIBUTE_FQBN_VALUE)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_RESOURCE_RELEASE_V4_ALE_SECURITY_ATTRIBUTE_FQBN_VALUE].value);
                  
#endif /// (NTDDI_VERSION >= NTDDI_WINTHRESHOLD)
#endif /// (NTDDI_VERSION >= NTDDI_WIN8)

         break;
      }
      case FWPS_LAYER_ALE_ENDPOINT_CLOSURE_V4:
      case FWPS_LAYER_ALE_ENDPOINT_CLOSURE_V6:
      {
         if(pConditionKey == &FWPM_CONDITION_ALE_APP_ID)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_ENDPOINT_CLOSURE_V4_ALE_APP_ID].value);
         else if(pConditionKey == &FWPM_CONDITION_ALE_USER_ID)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_ENDPOINT_CLOSURE_V4_ALE_USER_ID].value);
         else if(pConditionKey == &FWPM_CONDITION_IP_LOCAL_ADDRESS)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_ENDPOINT_CLOSURE_V4_IP_LOCAL_ADDRESS].value);
         else if(pConditionKey == &FWPM_CONDITION_IP_LOCAL_ADDRESS_TYPE)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_ENDPOINT_CLOSURE_V4_IP_LOCAL_ADDRESS_TYPE].value);
         else if(pConditionKey == &FWPM_CONDITION_IP_LOCAL_PORT)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_ENDPOINT_CLOSURE_V4_IP_LOCAL_PORT].value);
         else if(pConditionKey == &FWPM_CONDITION_IP_PROTOCOL)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_ENDPOINT_CLOSURE_V4_IP_PROTOCOL].value);
         else if(pConditionKey == &FWPM_CONDITION_IP_REMOTE_ADDRESS)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_ENDPOINT_CLOSURE_V4_IP_REMOTE_ADDRESS].value);
         else if(pConditionKey == &FWPM_CONDITION_IP_REMOTE_PORT)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_ENDPOINT_CLOSURE_V4_IP_REMOTE_PORT].value);
         else if(pConditionKey == &FWPM_CONDITION_IP_LOCAL_INTERFACE)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_ENDPOINT_CLOSURE_V4_IP_LOCAL_INTERFACE].value);
         else if(pConditionKey == &FWPM_CONDITION_FLAGS)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_ENDPOINT_CLOSURE_V4_FLAGS].value);

#if(NTDDI_VERSION >= NTDDI_WIN8)

         else if(pConditionKey == &FWPM_CONDITION_ALE_PACKAGE_ID)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_ENDPOINT_CLOSURE_V4_ALE_PACKAGE_ID].value);

#if(NTDDI_VERSION >= NTDDI_WINTHRESHOLD)

         else if(pConditionKey == &FWPM_CONDITION_ALE_SECURITY_ATTRIBUTE_FQBN_VALUE)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_ENDPOINT_CLOSURE_V4_ALE_SECURITY_ATTRIBUTE_FQBN_VALUE].value);

#endif /// (NTDDI_VERSION >= NTDDI_WINTHRESHOLD)
#endif /// (NTDDI_VERSION >= NTDDI_WIN8)

         break;
      }
      case FWPS_LAYER_ALE_CONNECT_REDIRECT_V4:
      case FWPS_LAYER_ALE_CONNECT_REDIRECT_V6:
      {
         if(pConditionKey == &FWPM_CONDITION_ALE_APP_ID)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_CONNECT_REDIRECT_V4_ALE_APP_ID].value);
         else if(pConditionKey == &FWPM_CONDITION_ALE_USER_ID)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_CONNECT_REDIRECT_V4_ALE_USER_ID].value);
         else if(pConditionKey == &FWPM_CONDITION_IP_LOCAL_ADDRESS)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_CONNECT_REDIRECT_V4_IP_LOCAL_ADDRESS].value);
         else if(pConditionKey == &FWPM_CONDITION_IP_LOCAL_ADDRESS_TYPE)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_CONNECT_REDIRECT_V4_IP_LOCAL_ADDRESS_TYPE].value);
         else if(pConditionKey == &FWPM_CONDITION_IP_LOCAL_PORT)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_CONNECT_REDIRECT_V4_IP_LOCAL_PORT].value);
         else if(pConditionKey == &FWPM_CONDITION_IP_PROTOCOL)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_CONNECT_REDIRECT_V4_IP_PROTOCOL].value);
         else if(pConditionKey == &FWPM_CONDITION_IP_REMOTE_ADDRESS)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_CONNECT_REDIRECT_V4_IP_REMOTE_ADDRESS].value);
         else if(pConditionKey == &FWPM_CONDITION_IP_DESTINATION_ADDRESS_TYPE)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_CONNECT_REDIRECT_V4_IP_DESTINATION_ADDRESS_TYPE].value);
         else if(pConditionKey == &FWPM_CONDITION_IP_REMOTE_PORT)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_CONNECT_REDIRECT_V4_IP_REMOTE_PORT].value);
         else if(pConditionKey == &FWPM_CONDITION_FLAGS)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_CONNECT_REDIRECT_V4_FLAGS].value);

#if(NTDDI_VERSION >= NTDDI_WIN8)

         else if(pConditionKey == &FWPM_CONDITION_ALE_ORIGINAL_APP_ID)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_CONNECT_REDIRECT_V4_ALE_ORIGINAL_APP_ID].value);
         else if(pConditionKey == &FWPM_CONDITION_ALE_PACKAGE_ID)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_CONNECT_REDIRECT_V4_ALE_PACKAGE_ID].value);

#if(NTDDI_VERSION >= NTDDI_WINTHRESHOLD)

         else if(pConditionKey == &FWPM_CONDITION_ALE_SECURITY_ATTRIBUTE_FQBN_VALUE)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_CONNECT_REDIRECT_V4_ALE_SECURITY_ATTRIBUTE_FQBN_VALUE].value);

#endif /// (NTDDI_VERSION >= NTDDI_WINTHRESHOLD)
#endif /// (NTDDI_VERSION >= NTDDI_WIN8)

         break;
      }
      case FWPS_LAYER_ALE_BIND_REDIRECT_V4:
      case FWPS_LAYER_ALE_BIND_REDIRECT_V6:
      {
         if(pConditionKey == &FWPM_CONDITION_ALE_APP_ID)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_BIND_REDIRECT_V4_ALE_APP_ID].value);
         else if(pConditionKey == &FWPM_CONDITION_ALE_USER_ID)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_BIND_REDIRECT_V4_ALE_USER_ID].value);
         else if(pConditionKey == &FWPM_CONDITION_IP_LOCAL_ADDRESS)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_BIND_REDIRECT_V4_IP_LOCAL_ADDRESS].value);
         else if(pConditionKey == &FWPM_CONDITION_IP_LOCAL_ADDRESS_TYPE)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_BIND_REDIRECT_V4_IP_LOCAL_ADDRESS_TYPE].value);
         else if(pConditionKey == &FWPM_CONDITION_IP_LOCAL_PORT)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_BIND_REDIRECT_V4_IP_LOCAL_PORT].value);
         else if(pConditionKey == &FWPM_CONDITION_IP_PROTOCOL)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_BIND_REDIRECT_V4_IP_PROTOCOL].value);
         else if(pConditionKey == &FWPM_CONDITION_FLAGS)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_BIND_REDIRECT_V4_FLAGS].value);

#if(NTDDI_VERSION >= NTDDI_WIN8)

         else if(pConditionKey == &FWPM_CONDITION_ALE_PACKAGE_ID)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_BIND_REDIRECT_V4_ALE_PACKAGE_ID].value);

#if(NTDDI_VERSION >= NTDDI_WINTHRESHOLD)

         else if(pConditionKey == &FWPM_CONDITION_ALE_SECURITY_ATTRIBUTE_FQBN_VALUE)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_ALE_BIND_REDIRECT_V4_ALE_SECURITY_ATTRIBUTE_FQBN_VALUE].value);

#endif /// (NTDDI_VERSION >= NTDDI_WINTHRESHOLD)
#endif /// (NTDDI_VERSION >= NTDDI_WIN8)

         break;
      }
      case FWPS_LAYER_STREAM_PACKET_V4:
      case FWPS_LAYER_STREAM_PACKET_V6:
      {
         if(pConditionKey == &FWPM_CONDITION_IP_LOCAL_ADDRESS)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_STREAM_PACKET_V4_IP_LOCAL_ADDRESS].value);
         else if(pConditionKey == &FWPM_CONDITION_IP_REMOTE_ADDRESS)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_STREAM_PACKET_V4_IP_REMOTE_ADDRESS].value);
         else if(pConditionKey == &FWPM_CONDITION_IP_LOCAL_PORT)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_STREAM_PACKET_V4_IP_LOCAL_PORT].value);
         else if(pConditionKey == &FWPM_CONDITION_IP_REMOTE_PORT)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_STREAM_PACKET_V4_IP_REMOTE_PORT].value);
         else if(pConditionKey == &FWPM_CONDITION_IP_LOCAL_INTERFACE)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_STREAM_PACKET_V4_IP_LOCAL_INTERFACE].value);
         else if(pConditionKey == &FWPM_CONDITION_INTERFACE_INDEX)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_STREAM_PACKET_V4_INTERFACE_INDEX].value);
         else if(pConditionKey == &FWPM_CONDITION_SUB_INTERFACE_INDEX)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_STREAM_PACKET_V4_SUB_INTERFACE_INDEX].value);
         else if(pConditionKey == &FWPM_CONDITION_DIRECTION)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_STREAM_PACKET_V4_DIRECTION].value);
         else if(pConditionKey == &FWPM_CONDITION_FLAGS)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_STREAM_PACKET_V4_FLAGS].value);
         else if(pConditionKey == &FWPM_CONDITION_INTERFACE_TYPE)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_STREAM_PACKET_V4_INTERFACE_TYPE].value);
         else if(pConditionKey == &FWPM_CONDITION_TUNNEL_TYPE)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_STREAM_PACKET_V4_TUNNEL_TYPE].value);

         break;
      }

#if(NTDDI_VERSION >= NTDDI_WIN8)

      case FWPS_LAYER_INBOUND_MAC_FRAME_ETHERNET:
      {
         if(pConditionKey == &FWPM_CONDITION_INTERFACE_MAC_ADDRESS)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_INBOUND_MAC_FRAME_ETHERNET_INTERFACE_MAC_ADDRESS].value);
         else if(pConditionKey == &FWPM_CONDITION_MAC_LOCAL_ADDRESS)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_INBOUND_MAC_FRAME_ETHERNET_MAC_LOCAL_ADDRESS].value);
         else if(pConditionKey == &FWPM_CONDITION_MAC_REMOTE_ADDRESS)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_INBOUND_MAC_FRAME_ETHERNET_MAC_REMOTE_ADDRESS].value);
         else if(pConditionKey == &FWPM_CONDITION_MAC_LOCAL_ADDRESS_TYPE)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_INBOUND_MAC_FRAME_ETHERNET_MAC_LOCAL_ADDRESS_TYPE].value);
         else if(pConditionKey == &FWPM_CONDITION_MAC_REMOTE_ADDRESS_TYPE)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_INBOUND_MAC_FRAME_ETHERNET_MAC_REMOTE_ADDRESS_TYPE].value);
         else if(pConditionKey == &FWPM_CONDITION_ETHER_TYPE)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_INBOUND_MAC_FRAME_ETHERNET_ETHER_TYPE].value);
         else if(pConditionKey == &FWPM_CONDITION_VLAN_ID)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_INBOUND_MAC_FRAME_ETHERNET_VLAN_ID].value);
         else if(pConditionKey == &FWPM_CONDITION_INTERFACE)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_INBOUND_MAC_FRAME_ETHERNET_INTERFACE].value);
         else if(pConditionKey == &FWPM_CONDITION_INTERFACE_INDEX)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_INBOUND_MAC_FRAME_ETHERNET_INTERFACE_INDEX].value);
         else if(pConditionKey == &FWPM_CONDITION_NDIS_PORT)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_INBOUND_MAC_FRAME_ETHERNET_NDIS_PORT].value);
         else if(pConditionKey == &FWPM_CONDITION_L2_FLAGS)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_INBOUND_MAC_FRAME_ETHERNET_L2_FLAGS].value);

         break;
      }
      case FWPS_LAYER_OUTBOUND_MAC_FRAME_ETHERNET:
      {
         if(pConditionKey == &FWPM_CONDITION_INTERFACE_MAC_ADDRESS)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_OUTBOUND_MAC_FRAME_ETHERNET_INTERFACE_MAC_ADDRESS].value);
         else if(pConditionKey == &FWPM_CONDITION_MAC_LOCAL_ADDRESS)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_OUTBOUND_MAC_FRAME_ETHERNET_MAC_LOCAL_ADDRESS].value);
         else if(pConditionKey == &FWPM_CONDITION_MAC_REMOTE_ADDRESS)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_OUTBOUND_MAC_FRAME_ETHERNET_MAC_REMOTE_ADDRESS].value);
         else if(pConditionKey == &FWPM_CONDITION_MAC_LOCAL_ADDRESS_TYPE)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_OUTBOUND_MAC_FRAME_ETHERNET_MAC_LOCAL_ADDRESS_TYPE].value);
         else if(pConditionKey == &FWPM_CONDITION_MAC_REMOTE_ADDRESS_TYPE)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_OUTBOUND_MAC_FRAME_ETHERNET_MAC_REMOTE_ADDRESS_TYPE].value);
         else if(pConditionKey == &FWPM_CONDITION_ETHER_TYPE)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_OUTBOUND_MAC_FRAME_ETHERNET_ETHER_TYPE].value);
         else if(pConditionKey == &FWPM_CONDITION_VLAN_ID)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_OUTBOUND_MAC_FRAME_ETHERNET_VLAN_ID].value);
         else if(pConditionKey == &FWPM_CONDITION_INTERFACE)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_OUTBOUND_MAC_FRAME_ETHERNET_INTERFACE].value);
         else if(pConditionKey == &FWPM_CONDITION_INTERFACE_INDEX)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_OUTBOUND_MAC_FRAME_ETHERNET_INTERFACE_INDEX].value);
         else if(pConditionKey == &FWPM_CONDITION_NDIS_PORT)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_OUTBOUND_MAC_FRAME_ETHERNET_NDIS_PORT].value);
         else if(pConditionKey == &FWPM_CONDITION_L2_FLAGS)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_OUTBOUND_MAC_FRAME_ETHERNET_L2_FLAGS].value);

         break;
      }
      case FWPS_LAYER_INBOUND_MAC_FRAME_NATIVE:
      {
         if(pConditionKey == &FWPM_CONDITION_NDIS_MEDIA_TYPE)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_INBOUND_MAC_FRAME_NATIVE_NDIS_MEDIA_TYPE].value);
         else if(pConditionKey == &FWPM_CONDITION_NDIS_PHYSICAL_MEDIA_TYPE)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_INBOUND_MAC_FRAME_NATIVE_NDIS_PHYSICAL_MEDIA_TYPE].value);
         else if(pConditionKey == &FWPM_CONDITION_INTERFACE)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_INBOUND_MAC_FRAME_NATIVE_INTERFACE].value);
         else if(pConditionKey == &FWPM_CONDITION_INTERFACE_TYPE)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_INBOUND_MAC_FRAME_NATIVE_INTERFACE_TYPE].value);
         else if(pConditionKey == &FWPM_CONDITION_INTERFACE_INDEX)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_INBOUND_MAC_FRAME_NATIVE_INTERFACE_INDEX].value);
         else if(pConditionKey == &FWPM_CONDITION_NDIS_PORT)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_INBOUND_MAC_FRAME_NATIVE_NDIS_PORT].value);
         else if(pConditionKey == &FWPM_CONDITION_L2_FLAGS)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_INBOUND_MAC_FRAME_NATIVE_L2_FLAGS].value);

         break;
      }
      case FWPS_LAYER_OUTBOUND_MAC_FRAME_NATIVE:
      {
         if(pConditionKey == &FWPM_CONDITION_NDIS_MEDIA_TYPE)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_OUTBOUND_MAC_FRAME_NATIVE_NDIS_MEDIA_TYPE].value);
         else if(pConditionKey == &FWPM_CONDITION_NDIS_PHYSICAL_MEDIA_TYPE)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_OUTBOUND_MAC_FRAME_NATIVE_NDIS_PHYSICAL_MEDIA_TYPE].value);
         else if(pConditionKey == &FWPM_CONDITION_INTERFACE)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_OUTBOUND_MAC_FRAME_NATIVE_INTERFACE].value);
         else if(pConditionKey == &FWPM_CONDITION_INTERFACE_TYPE)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_OUTBOUND_MAC_FRAME_NATIVE_INTERFACE_TYPE].value);
         else if(pConditionKey == &FWPM_CONDITION_INTERFACE_INDEX)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_OUTBOUND_MAC_FRAME_NATIVE_INTERFACE_INDEX].value);
         else if(pConditionKey == &FWPM_CONDITION_NDIS_PORT)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_OUTBOUND_MAC_FRAME_NATIVE_NDIS_PORT].value);
         else if(pConditionKey == &FWPM_CONDITION_L2_FLAGS)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_OUTBOUND_MAC_FRAME_NATIVE_L2_FLAGS].value);

         break;
      }
      case FWPS_LAYER_INGRESS_VSWITCH_ETHERNET:
      {
         if(pConditionKey == &FWPM_CONDITION_MAC_SOURCE_ADDRESS)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_INGRESS_VSWITCH_ETHERNET_MAC_SOURCE_ADDRESS].value);
         else if(pConditionKey == &FWPM_CONDITION_MAC_SOURCE_ADDRESS_TYPE)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_INGRESS_VSWITCH_ETHERNET_MAC_SOURCE_ADDRESS_TYPE].value);
         else if(pConditionKey == &FWPM_CONDITION_MAC_DESTINATION_ADDRESS)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_INGRESS_VSWITCH_ETHERNET_MAC_DESTINATION_ADDRESS].value);
         else if(pConditionKey == &FWPM_CONDITION_MAC_DESTINATION_ADDRESS_TYPE)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_INGRESS_VSWITCH_ETHERNET_MAC_DESTINATION_ADDRESS_TYPE].value);
         else if(pConditionKey == &FWPM_CONDITION_ETHER_TYPE)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_INGRESS_VSWITCH_ETHERNET_ETHER_TYPE].value);
         else if(pConditionKey == &FWPM_CONDITION_VLAN_ID)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_INGRESS_VSWITCH_ETHERNET_VLAN_ID].value);
         else if(pConditionKey == &FWPM_CONDITION_VSWITCH_TENANT_NETWORK_ID)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_INGRESS_VSWITCH_ETHERNET_VSWITCH_TENANT_NETWORK_ID].value);
         else if(pConditionKey == &FWPM_CONDITION_VSWITCH_ID)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_INGRESS_VSWITCH_ETHERNET_VSWITCH_ID].value);
         else if(pConditionKey == &FWPM_CONDITION_VSWITCH_NETWORK_TYPE)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_INGRESS_VSWITCH_ETHERNET_VSWITCH_NETWORK_TYPE].value);
         else if(pConditionKey == &FWPM_CONDITION_VSWITCH_SOURCE_INTERFACE_ID)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_INGRESS_VSWITCH_ETHERNET_VSWITCH_SOURCE_INTERFACE_ID].value);
         else if(pConditionKey == &FWPM_CONDITION_VSWITCH_SOURCE_INTERFACE_TYPE)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_INGRESS_VSWITCH_ETHERNET_VSWITCH_SOURCE_INTERFACE_TYPE].value);
         else if(pConditionKey == &FWPM_CONDITION_VSWITCH_SOURCE_VM_ID)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_INGRESS_VSWITCH_ETHERNET_VSWITCH_SOURCE_VM_ID].value);
         else if(pConditionKey == &FWPM_CONDITION_L2_FLAGS)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_INGRESS_VSWITCH_ETHERNET_L2_FLAGS].value);

         break;
      }
      case FWPS_LAYER_EGRESS_VSWITCH_ETHERNET:
      {
         if(pConditionKey == &FWPM_CONDITION_MAC_SOURCE_ADDRESS)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_EGRESS_VSWITCH_ETHERNET_MAC_SOURCE_ADDRESS].value);
         else if(pConditionKey == &FWPM_CONDITION_MAC_SOURCE_ADDRESS_TYPE)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_EGRESS_VSWITCH_ETHERNET_MAC_SOURCE_ADDRESS_TYPE].value);
         else if(pConditionKey == &FWPM_CONDITION_MAC_DESTINATION_ADDRESS)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_EGRESS_VSWITCH_ETHERNET_MAC_DESTINATION_ADDRESS].value);
         else if(pConditionKey == &FWPM_CONDITION_MAC_DESTINATION_ADDRESS_TYPE)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_EGRESS_VSWITCH_ETHERNET_MAC_DESTINATION_ADDRESS_TYPE].value);
         else if(pConditionKey == &FWPM_CONDITION_ETHER_TYPE)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_EGRESS_VSWITCH_ETHERNET_ETHER_TYPE].value);
         else if(pConditionKey == &FWPM_CONDITION_VLAN_ID)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_EGRESS_VSWITCH_ETHERNET_VLAN_ID].value);
         else if(pConditionKey == &FWPM_CONDITION_VSWITCH_TENANT_NETWORK_ID)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_EGRESS_VSWITCH_ETHERNET_VSWITCH_TENANT_NETWORK_ID].value);
         else if(pConditionKey == &FWPM_CONDITION_VSWITCH_ID)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_EGRESS_VSWITCH_ETHERNET_VSWITCH_ID].value);
         else if(pConditionKey == &FWPM_CONDITION_VSWITCH_NETWORK_TYPE)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_EGRESS_VSWITCH_ETHERNET_VSWITCH_NETWORK_TYPE].value);
         else if(pConditionKey == &FWPM_CONDITION_VSWITCH_SOURCE_INTERFACE_ID)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_EGRESS_VSWITCH_ETHERNET_VSWITCH_SOURCE_INTERFACE_ID].value);
         else if(pConditionKey == &FWPM_CONDITION_VSWITCH_SOURCE_INTERFACE_TYPE)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_EGRESS_VSWITCH_ETHERNET_VSWITCH_SOURCE_INTERFACE_TYPE].value);
         else if(pConditionKey == &FWPM_CONDITION_VSWITCH_SOURCE_VM_ID)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_EGRESS_VSWITCH_ETHERNET_VSWITCH_SOURCE_VM_ID].value);
         else if(pConditionKey == &FWPM_CONDITION_VSWITCH_DESTINATION_INTERFACE_ID)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_EGRESS_VSWITCH_ETHERNET_VSWITCH_DESTINATION_INTERFACE_ID].value);
         else if(pConditionKey == &FWPM_CONDITION_VSWITCH_DESTINATION_INTERFACE_TYPE)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_EGRESS_VSWITCH_ETHERNET_VSWITCH_DESTINATION_INTERFACE_TYPE].value);
         else if(pConditionKey == &FWPM_CONDITION_VSWITCH_DESTINATION_VM_ID)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_EGRESS_VSWITCH_ETHERNET_VSWITCH_DESTINATION_VM_ID].value);
         else if(pConditionKey == &FWPM_CONDITION_L2_FLAGS)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_EGRESS_VSWITCH_ETHERNET_L2_FLAGS].value);

         break;
      }
      case FWPS_LAYER_INGRESS_VSWITCH_TRANSPORT_V4:
      case FWPS_LAYER_INGRESS_VSWITCH_TRANSPORT_V6:
      {
         if(pConditionKey == &FWPM_CONDITION_IP_SOURCE_ADDRESS)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_INGRESS_VSWITCH_TRANSPORT_V4_IP_SOURCE_ADDRESS].value);
         else if(pConditionKey == &FWPM_CONDITION_IP_DESTINATION_ADDRESS)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_INGRESS_VSWITCH_TRANSPORT_V4_IP_DESTINATION_ADDRESS].value);
         else if(pConditionKey == &FWPM_CONDITION_IP_PROTOCOL)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_INGRESS_VSWITCH_TRANSPORT_V4_IP_PROTOCOL].value);
         else if(pConditionKey == &FWPM_CONDITION_IP_SOURCE_PORT)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_INGRESS_VSWITCH_TRANSPORT_V4_IP_SOURCE_PORT].value);
         else if(pConditionKey == &FWPM_CONDITION_IP_DESTINATION_PORT)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_INGRESS_VSWITCH_TRANSPORT_V4_IP_DESTINATION_PORT].value);
         else if(pConditionKey == &FWPM_CONDITION_VLAN_ID)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_INGRESS_VSWITCH_TRANSPORT_V4_VLAN_ID].value);
         else if(pConditionKey == &FWPM_CONDITION_VSWITCH_TENANT_NETWORK_ID)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_INGRESS_VSWITCH_TRANSPORT_V4_VSWITCH_TENANT_NETWORK_ID].value);
         else if(pConditionKey == &FWPM_CONDITION_VSWITCH_ID)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_INGRESS_VSWITCH_TRANSPORT_V4_VSWITCH_ID].value);
         else if(pConditionKey == &FWPM_CONDITION_VSWITCH_NETWORK_TYPE)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_INGRESS_VSWITCH_TRANSPORT_V4_VSWITCH_NETWORK_TYPE].value);
         else if(pConditionKey == &FWPM_CONDITION_VSWITCH_SOURCE_INTERFACE_ID)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_INGRESS_VSWITCH_TRANSPORT_V4_VSWITCH_SOURCE_INTERFACE_ID].value);
         else if(pConditionKey == &FWPM_CONDITION_VSWITCH_SOURCE_INTERFACE_TYPE)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_INGRESS_VSWITCH_TRANSPORT_V4_VSWITCH_SOURCE_INTERFACE_TYPE].value);
         else if(pConditionKey == &FWPM_CONDITION_VSWITCH_SOURCE_VM_ID)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_INGRESS_VSWITCH_TRANSPORT_V4_VSWITCH_SOURCE_VM_ID].value);
         else if(pConditionKey == &FWPM_CONDITION_L2_FLAGS)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_INGRESS_VSWITCH_TRANSPORT_V4_L2_FLAGS].value);

         break;
      }
      case FWPS_LAYER_EGRESS_VSWITCH_TRANSPORT_V4:
      case FWPS_LAYER_EGRESS_VSWITCH_TRANSPORT_V6:
      {
         if(pConditionKey == &FWPM_CONDITION_IP_SOURCE_ADDRESS)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_INGRESS_VSWITCH_TRANSPORT_V4_IP_SOURCE_ADDRESS].value);
         else if(pConditionKey == &FWPM_CONDITION_IP_DESTINATION_ADDRESS)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_INGRESS_VSWITCH_TRANSPORT_V4_IP_DESTINATION_ADDRESS].value);
         else if(pConditionKey == &FWPM_CONDITION_IP_PROTOCOL)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_INGRESS_VSWITCH_TRANSPORT_V4_IP_PROTOCOL].value);
         else if(pConditionKey == &FWPM_CONDITION_IP_SOURCE_PORT)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_INGRESS_VSWITCH_TRANSPORT_V4_IP_SOURCE_PORT].value);
         else if(pConditionKey == &FWPM_CONDITION_IP_DESTINATION_PORT)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_INGRESS_VSWITCH_TRANSPORT_V4_IP_DESTINATION_PORT].value);
         else if(pConditionKey == &FWPM_CONDITION_VLAN_ID)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_EGRESS_VSWITCH_TRANSPORT_V4_VLAN_ID].value);
         else if(pConditionKey == &FWPM_CONDITION_VSWITCH_TENANT_NETWORK_ID)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_EGRESS_VSWITCH_TRANSPORT_V4_VSWITCH_TENANT_NETWORK_ID].value);
         else if(pConditionKey == &FWPM_CONDITION_VSWITCH_ID)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_EGRESS_VSWITCH_TRANSPORT_V4_VSWITCH_ID].value);
         else if(pConditionKey == &FWPM_CONDITION_VSWITCH_NETWORK_TYPE)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_EGRESS_VSWITCH_TRANSPORT_V4_VSWITCH_NETWORK_TYPE].value);
         else if(pConditionKey == &FWPM_CONDITION_VSWITCH_SOURCE_INTERFACE_ID)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_EGRESS_VSWITCH_TRANSPORT_V4_VSWITCH_SOURCE_INTERFACE_ID].value);
         else if(pConditionKey == &FWPM_CONDITION_VSWITCH_SOURCE_INTERFACE_TYPE)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_EGRESS_VSWITCH_TRANSPORT_V4_VSWITCH_SOURCE_INTERFACE_TYPE].value);
         else if(pConditionKey == &FWPM_CONDITION_VSWITCH_SOURCE_VM_ID)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_EGRESS_VSWITCH_TRANSPORT_V4_VSWITCH_SOURCE_VM_ID].value);
         else if(pConditionKey == &FWPM_CONDITION_VSWITCH_DESTINATION_INTERFACE_ID)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_EGRESS_VSWITCH_TRANSPORT_V4_VSWITCH_DESTINATION_INTERFACE_ID].value);
         else if(pConditionKey == &FWPM_CONDITION_VSWITCH_DESTINATION_INTERFACE_TYPE)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_EGRESS_VSWITCH_TRANSPORT_V4_VSWITCH_DESTINATION_INTERFACE_TYPE].value);
         else if(pConditionKey == &FWPM_CONDITION_VSWITCH_DESTINATION_VM_ID)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_EGRESS_VSWITCH_TRANSPORT_V4_VSWITCH_DESTINATION_VM_ID].value);
         else if(pConditionKey == &FWPM_CONDITION_L2_FLAGS)
            pValue = &(pClassifyValues->incomingValue[FWPS_FIELD_EGRESS_VSWITCH_TRANSPORT_V4_L2_FLAGS].value);

         break;
      }

#endif /// (NTDDI_VERSION >= NTDDI_WIN8)
#endif /// (NTDDI_VERSION >= NTDDI_WIN7)

   }

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprFwpValueGetFromFwpsIncomingValues() [pValue: %#p]\n",
              pValue);

#endif /// DBG

   return pValue;
}

/**
 @kernel_helper_function="KrnlHlprFwpValuePurgeLocalCopy"
 
   Purpose:  Cleanup a local copy of an FWP_VALUE.                                              <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF552450.aspx             <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
inline VOID KrnlHlprFwpValuePurgeLocalCopy(_Inout_ FWP_VALUE* pValue)
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprFwpValuePurgeLocalCopy()\n");

#endif /// DBG
   
   NT_ASSERT(pValue);

   switch(pValue->type)
   {
      case FWP_UINT64:
      {
         HLPR_DELETE(pValue->uint64,
                     WFPSAMPLER_SYSLIB_TAG);

         break;
      }
      case FWP_INT64:
      {
         HLPR_DELETE(pValue->int64,
                     WFPSAMPLER_SYSLIB_TAG);

         break;
      }
      case FWP_DOUBLE:
      {
         HLPR_DELETE(pValue->double64,
                     WFPSAMPLER_SYSLIB_TAG);

         break;
      }
      case FWP_BYTE_ARRAY16_TYPE:
      {
         HLPR_DELETE(pValue->byteArray16,
                     WFPSAMPLER_SYSLIB_TAG);

         break;
      }
      case FWP_BYTE_BLOB_TYPE:
      {
         KrnlHlprFwpByteBlobDestroyLocalCopy(&(pValue->byteBlob));

         break;
      }
      case FWP_SID:
      {
         HLPR_DELETE_ARRAY(pValue->sid,
                           WFPSAMPLER_SYSLIB_TAG);

         break;
      }
      case FWP_SECURITY_DESCRIPTOR_TYPE:
      {
         KrnlHlprFwpByteBlobDestroyLocalCopy(&(pValue->sd));

         break;
      }
      case FWP_TOKEN_INFORMATION_TYPE:
      {
         KrnlHlprFwpTokenInformationDestroyLocalCopy(&(pValue->tokenInformation));

         break;
      }
      case FWP_TOKEN_ACCESS_INFORMATION_TYPE:
      {
         KrnlHlprFwpByteBlobDestroyLocalCopy(&(pValue->tokenAccessInformation));

         break;
      }
      case FWP_UNICODE_STRING_TYPE:
      {
         HLPR_DELETE_ARRAY(pValue->unicodeString,
                           WFPSAMPLER_SYSLIB_TAG);

         break;
      }

#if(NTDDI_VERSION >= NTDDI_WIN7)

      case FWP_BYTE_ARRAY6_TYPE:
      {
         HLPR_DELETE(pValue->byteArray6,
                     WFPSAMPLER_SYSLIB_TAG);

         break;
      }

#endif /// (NTDDI_VERSION >= NTDDI_WIN7)

      default:
      {
         pValue->uint64 = 0;

         break;
      }
   }

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprFwpValuePurgeLocalCopy()\n");

#endif /// DBG

   return;
}

/**
 @kernel_helper_function="KrnlHlprFwpValueDestroyLocalCopy"
 
   Purpose:  Cleanup and free a local copy of an FWP_VALUE.                                     <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF552450.aspx             <br>
*/
_At_(*ppValue, _Pre_ _Notnull_)
_At_(*ppValue, _Post_ _Null_ __drv_freesMem(Pool))
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Success_(*ppValue == 0)
inline VOID KrnlHlprFwpValueDestroyLocalCopy(_Inout_ FWP_VALUE** ppValue)
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprFwpValueDestroyLocalCopy()\n");

#endif /// DBG
   
   NT_ASSERT(ppValue);

   if(*ppValue)
      KrnlHlprFwpValuePurgeLocalCopy(*ppValue);

   HLPR_DELETE(*ppValue,
               WFPSAMPLER_SYSLIB_TAG);

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprFwpValueDestroyLocalCopy()\n");

#endif /// DBG

   return;
}

/**
 @kernel_helper_function="KrnlHlprFwpValuePopulateLocalCopy"
 
   Purpose:  Populate a local copy of an FWP_VALUE.                                             <br>
                                                                                                <br>
   Notes:    The caller is responsible for freeing any allocated memory using
             KrnlHlprFwpValuePurgeLocalCopy().                                                  <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF552450.aspx             <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Check_return_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprFwpValuePopulateLocalCopy(_In_ const FWP_VALUE* pOriginalValue,
                                           _Inout_ FWP_VALUE* pValue)
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprFwpValuePopulateLocalCopy()\n");

#endif /// DBG
   
   NT_ASSERT(pOriginalValue);
   NT_ASSERT(pValue);

   NTSTATUS status = STATUS_SUCCESS;
   KIRQL    irql   = KeGetCurrentIrql();

   pValue->type = pOriginalValue->type;

   switch(pValue->type)
   {
      case FWP_UINT64:
      {
         HLPR_NEW(pValue->uint64,
                  UINT64,
                  WFPSAMPLER_SYSLIB_TAG);
         HLPR_BAIL_ON_ALLOC_FAILURE(pValue->uint64,
                                    status);

         RtlCopyMemory(pValue->uint64,
                       pOriginalValue->uint64,
                       sizeof(UINT64));

         break;
      }
      case FWP_INT64:
      {
         HLPR_NEW(pValue->int64,
                  INT64,
                  WFPSAMPLER_SYSLIB_TAG);
         HLPR_BAIL_ON_ALLOC_FAILURE(pValue->int64,
                                    status);

         RtlCopyMemory(pValue->int64,
                       pOriginalValue->int64,
                       sizeof(INT64));

         break;
      }
      case FWP_DOUBLE:
      {
         HLPR_NEW(pValue->double64,
                  double,
                  WFPSAMPLER_SYSLIB_TAG);
         HLPR_BAIL_ON_ALLOC_FAILURE(pValue->double64,
                                    status);

         RtlCopyMemory(pValue->double64,
                       pOriginalValue->double64,
                       sizeof(double));

         break;
      }
      case FWP_BYTE_ARRAY16_TYPE:
      {
         HLPR_NEW(pValue->byteArray16,
                  FWP_BYTE_ARRAY16,
                  WFPSAMPLER_SYSLIB_TAG);
         HLPR_BAIL_ON_ALLOC_FAILURE(pValue->byteArray16,
                                    status);

         RtlCopyMemory(pValue->byteArray16,
                       pOriginalValue->byteArray16,
                       sizeof(FWP_BYTE_ARRAY16));

         break;
      }
      case FWP_BYTE_BLOB_TYPE:
      {
         pValue->byteBlob = KrnlHlprFwpByteBlobCreateLocalCopy(pOriginalValue->byteBlob);
         HLPR_BAIL_ON_ALLOC_FAILURE(pValue->byteBlob,
                                    status);

         break;
      }
      case FWP_SID:
      {
         HLPR_DELETE_ARRAY(pValue->sid,
                           WFPSAMPLER_SYSLIB_TAG);

         break;
      }
      case FWP_SECURITY_DESCRIPTOR_TYPE:
      {
         pValue->sd = KrnlHlprFwpByteBlobCreateLocalCopy(pOriginalValue->sd);
         HLPR_BAIL_ON_ALLOC_FAILURE(pValue->sd,
                                    status);
         break;
      }
      case FWP_TOKEN_INFORMATION_TYPE:
      {
         pValue->tokenInformation = KrnlHlprFwpTokenInformationCreateLocalCopy(pOriginalValue->tokenInformation);
         HLPR_BAIL_ON_ALLOC_FAILURE(pValue->tokenInformation,
                                    status);

         break;
      }
      case FWP_TOKEN_ACCESS_INFORMATION_TYPE:
      {
         pValue->tokenAccessInformation = KrnlHlprFwpByteBlobCreateLocalCopy(pOriginalValue->tokenAccessInformation);
         HLPR_BAIL_ON_ALLOC_FAILURE(pValue->tokenAccessInformation,
                                    status);

         break;
      }
      case FWP_UNICODE_STRING_TYPE:
      {
         size_t stringSize = 0;

         if(irql == PASSIVE_LEVEL)
         {
            status = RtlStringCchLengthW(pOriginalValue->unicodeString,
                                         NTSTRSAFE_MAX_CCH,
                                         &stringSize);
            HLPR_BAIL_ON_FAILURE(status);
         }
         else
            stringSize = wcslen(pOriginalValue->unicodeString);

         if(stringSize)
         {
            HLPR_NEW_ARRAY(pValue->unicodeString,
                           WCHAR,
                           stringSize + 1,                     /// add space for '\0'
                           WFPSAMPLER_SYSLIB_TAG);
            HLPR_BAIL_ON_ALLOC_FAILURE(pValue->unicodeString,
                                       status);

#pragma warning(push)
#pragma warning(disable: 26018) /// buffers are contrained by stringSize

            RtlCopyMemory(pValue->unicodeString,
                          pOriginalValue->unicodeString,
                          stringSize * sizeof(WCHAR));

            pValue->unicodeString[stringSize] = L'\0';

#pragma warning(pop)
         }
         else
         {
            status = STATUS_INVALID_BUFFER_SIZE;

            HLPR_BAIL;
         }

         break;
      }

#if(NTDDI_VERSION >= NTDDI_WIN7)

      case FWP_BYTE_ARRAY6_TYPE:
      {
         HLPR_NEW(pValue->byteArray6,
                  FWP_BYTE_ARRAY6,
                  WFPSAMPLER_SYSLIB_TAG);
         HLPR_BAIL_ON_ALLOC_FAILURE(pValue->byteArray6,
                                    status);

         RtlCopyMemory(pValue->byteArray6,
                       pOriginalValue->byteArray6,
                       sizeof(FWP_BYTE_ARRAY6));

         break;
      }

#endif /// (NTDDI_VERSION >= NTDDI_WIN7)

      default:
      {
         /// this is a straight value copy so it's safe to use the largest static space
         pValue->uint64 = pOriginalValue->uint64;

         break;
      }
   }

   HLPR_BAIL_LABEL:

   if(status != STATUS_SUCCESS)
      KrnlHlprFwpValuePurgeLocalCopy(pValue);

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprFwpValuePopulateLocalCopy() [status: %#x]\n",
              status);

#endif /// DBG

   return status;
}

/**
 @kernel_helper_function="KrnlHlprFwpValueCreateLocalCopy"
 
   Purpose:  Allocate and populate a local copy of an FWP_VALUE.                                <br>
                                                                                                <br>
   Notes:    The caller is responsible for freeing the allocated memory using
             KrnlHlprFwpValueDestroyLocalCopy().                                                <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF552450.aspx             <br>
*/
__drv_allocatesMem(Pool)
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Must_inspect_result_
_Success_(return != 0)
FWP_VALUE* KrnlHlprFwpValueCreateLocalCopy(_In_ const FWP_VALUE* pOriginalValue)
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprFwpValueCreateLocalCopy()\n");

#endif /// DBG
   
   NT_ASSERT(pOriginalValue);

   NTSTATUS   status = STATUS_SUCCESS;
   FWP_VALUE* pValue = 0;

#pragma warning(push)
#pragma warning(disable: 6014) /// pValue is expected to be cleaned up by caller using KrnlHlprFwpValueDestroyLocalCopy

   HLPR_NEW(pValue,
            FWP_VALUE,
            WFPSAMPLER_SYSLIB_TAG);
   HLPR_BAIL_ON_ALLOC_FAILURE(pValue,
                              status);

#pragma warning(pop)

   status = KrnlHlprFwpValuePopulateLocalCopy(pOriginalValue,
                                              pValue);

   HLPR_BAIL_LABEL:

#pragma warning(push)
#pragma warning(disable: 6001) /// pValue initialized with call to HLPR_NEW & KrnlHlprFwpValuePopulateLocalCopy 

   if(status != STATUS_SUCCESS &&
      pValue)
      KrnlHlprFwpValueDestroyLocalCopy(&pValue);

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprFwpValueCreateLocalCopy() [pValue: %#p]\n",
              pValue);

#endif /// DBG

   return pValue;

#pragma warning(pop)
}

#endif /// FWP_VALUE____

#ifndef FWP_V4_ADDR_AND_MASK____
#define FWP_V4_ADDR_AND_MASK____

/**
 @kernel_helper_function="KrnlHlprFwpV4AddrAndMaskPurgeLocalCopy"
 
   Purpose:  Cleanup a local copy of an FWP_V4_ADDR_AND_MASK.                                   <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF552441.aspx             <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
inline VOID KrnlHlprFwpV4AddrAndMaskPurgeLocalCopy(_Inout_ FWP_V4_ADDR_AND_MASK* pAddrMask)
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprFwpV4AddrAndMaskPurgeLocalCopy()\n");

#endif /// DBG
   
   NT_ASSERT(pAddrMask);

   RtlZeroMemory(pAddrMask,
                 sizeof(FWP_V4_ADDR_AND_MASK));

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprFwpV4AddrAndMaskPurgeLocalCopy()\n");

#endif /// DBG

   return;
}

/**
 @kernel_helper_function="KrnlHlprFwpV4AddrAndMaskDestroyLocalCopy"
 
   Purpose:  Cleanup and free a local copy of an FWP_V4_ADDR_AND_MASK.                          <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF552441.aspx             <br>
*/
_At_(*ppAddrMask, _Pre_ _Notnull_)
_At_(*ppAddrMask, _Post_ _Null_ __drv_freesMem(Pool))
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Success_(*ppAddrMask == 0)
inline VOID KrnlHlprFwpV4AddrAndMaskDestroyLocalCopy(_Inout_ FWP_V4_ADDR_AND_MASK** ppAddrMask)
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprFwpV4AddrAndMaskDestroyLocalCopy()\n");

#endif /// DBG
   
   NT_ASSERT(ppAddrMask);

   if(*ppAddrMask)
      KrnlHlprFwpV4AddrAndMaskPurgeLocalCopy(*ppAddrMask);

   HLPR_DELETE(*ppAddrMask,
               WFPSAMPLER_SYSLIB_TAG);

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprFwpV4AddrAndMaskDestroyLocalCopy()\n");

#endif /// DBG

   return;
}

/**
 @kernel_helper_function="KrnlHlprFwpV4AddrAndMaskPopulateLocalCopy"
 
   Purpose:  Populate a local copy of an FWP_V4_ADDR_AND_MASK.                                  <br>
                                                                                                <br>
   Notes:    The caller is responsible for freeing the allocated memory using
             KrnlHlprFwpV4AddrMaskPurgeLocalCopy().                                             <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF552441.aspx             <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Check_return_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprFwpV4AddrAndMaskPopulateLocalCopy(_In_ const FWP_V4_ADDR_AND_MASK* pOriginalAddrMask,
                                                   _Inout_ FWP_V4_ADDR_AND_MASK* pAddrMask)
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprFwpV4AddrAndMaskPopulateLocalCopy()\n");

#endif /// DBG
   
   NT_ASSERT(pOriginalAddrMask);
   NT_ASSERT(pAddrMask);

   NTSTATUS status = STATUS_SUCCESS;

   pAddrMask->addr = pOriginalAddrMask->addr;
   pAddrMask->mask = pOriginalAddrMask->mask;

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprFwpV4AddrAndMaskPopulateLocalCopy() [status: %#x]\n",
              status);

#endif /// DBG

   return status;
}

/**
 @kernel_helper_function="KrnlHlprFwpV4AddrAndMaskCreateLocalCopy"
 
   Purpose:  Allocate and populate a local copy of an FWP_V4_ADDR_AND_MASK.                     <br>
                                                                                                <br>
   Notes:    The caller is responsible for freeing the allocated memory using
             KrnlHlprFwpV4AddrMaskDestroyLocalCopy().                                           <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF552441.aspx             <br>
*/
__drv_allocatesMem(Pool)
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Must_inspect_result_
_Success_(return != 0)
FWP_V4_ADDR_AND_MASK* KrnlHlprFwpV4AddrAndMaskCreateLocalCopy(_In_ const FWP_V4_ADDR_AND_MASK* pOriginalAddrMask)
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprFwpV4AddrAndMaskCreateLocalCopy()\n");

#endif /// DBG
   
   NT_ASSERT(pOriginalAddrMask);

   NTSTATUS              status    = STATUS_SUCCESS;
   FWP_V4_ADDR_AND_MASK* pAddrMask = 0;

#pragma warning(push)
#pragma warning(disable: 6014) /// pAddrMask is expected to be cleaned up by caller using KrnlHlprFwpV4AddrAndMaskDestroyLocalCopy

   HLPR_NEW(pAddrMask,
            FWP_V4_ADDR_AND_MASK,
            WFPSAMPLER_SYSLIB_TAG);
   HLPR_BAIL_ON_ALLOC_FAILURE(pAddrMask,
                              status);

#pragma warning(pop)

   status = KrnlHlprFwpV4AddrAndMaskPopulateLocalCopy(pOriginalAddrMask,
                                                      pAddrMask);

   HLPR_BAIL_LABEL:

#pragma warning(push)
#pragma warning(disable: 6001) /// pAddrMask initialized with call to HLPR_NEW & KrnlHlprFwpV4AddrAndMaskPopulateLocalCopy 

   if(status != STATUS_SUCCESS &&
      pAddrMask)
      KrnlHlprFwpV4AddrAndMaskDestroyLocalCopy(&pAddrMask);

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprFwpV4AddrAndMaskCreateLocalCopy() [pAddrMask: %#p]\n",
              pAddrMask);

#endif /// DBG

   return pAddrMask;

#pragma warning(pop)
}

#endif /// FWP_V4_ADDR_AND_MASK____

#ifndef FWP_V6_ADDR_AND_MASK____
#define FWP_V6_ADDR_AND_MASK____

/**
 @kernel_helper_function="KrnlHlprFwpV6AddrAndMaskPurgeLocalCopy"
 
   Purpose:  Cleanup a local copy of an FWP_V6_ADDR_AND_MASK.                                   <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF552446.aspx             <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
inline VOID KrnlHlprFwpV6AddrAndMaskPurgeLocalCopy(_Inout_ FWP_V6_ADDR_AND_MASK* pAddrMask)
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprFwpV6AddrAndMaskPurgeLocalCopy()\n");

#endif /// DBG
   
   NT_ASSERT(pAddrMask);

   RtlZeroMemory(pAddrMask,
                 sizeof(FWP_V6_ADDR_AND_MASK));

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprFwpV6AddrAndMaskPurgeLocalCopy()\n");

#endif /// DBG

   return;
}

/**
 @kernel_helper_function="KrnlHlprFwpV6AddrAndMaskDestroyLocalCopy"
 
   Purpose:  Cleanup and free a local copy of an FWP_V6_ADDR_AND_MASK.                          <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF552446.aspx             <br>
*/
_At_(*ppAddrMask, _Pre_ _Notnull_)
_At_(*ppAddrMask, _Post_ _Null_ __drv_freesMem(Pool))
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Success_(*ppAddrMask == 0)
inline VOID KrnlHlprFwpV6AddrAndMaskDestroyLocalCopy(_Inout_ FWP_V6_ADDR_AND_MASK** ppAddrMask)
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprFwpV6AddrAndMaskDestroyLocalCopy()\n");

#endif /// DBG
   
   NT_ASSERT(ppAddrMask);

   if(*ppAddrMask)
      KrnlHlprFwpV6AddrAndMaskPurgeLocalCopy(*ppAddrMask);

   HLPR_DELETE(*ppAddrMask,
               WFPSAMPLER_SYSLIB_TAG);

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprFwpV6AddrAndMaskDestroyLocalCopy()\n");

#endif /// DBG

   return;
}

/**
 @kernel_helper_function="KrnlHlprFwpV6AddrAndMaskPopulateLocalCopy"
 
   Purpose:  Populate a local copy of an FWP_V6_ADDR_AND_MASK.                                  <br>
                                                                                                <br>
   Notes:    The caller is responsible for freeing the allocated memory using
             KrnlHlprFwpV6AddrMaskPurgeLocalCopy().                                             <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF552446.aspx             <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Check_return_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprFwpV6AddrAndMaskPopulateLocalCopy(_In_ const FWP_V6_ADDR_AND_MASK* pOriginalAddrMask,
                                                   _Inout_ FWP_V6_ADDR_AND_MASK* pAddrMask)
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprFwpV6AddrAndMaskPopulateLocalCopy()\n");

#endif /// DBG
   
   NT_ASSERT(pOriginalAddrMask);
   NT_ASSERT(pAddrMask);

   NTSTATUS status = STATUS_SUCCESS;

   RtlCopyMemory(pAddrMask->addr,
                 pOriginalAddrMask->addr,
                 FWP_V6_ADDR_SIZE);

   pAddrMask->prefixLength = pOriginalAddrMask->prefixLength;

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprFwpV6AddrAndMaskPopulateLocalCopy() [status: %#x]\n",
              status);

#endif /// DBG

   return status;
}

/**
 @kernel_helper_function="KrnlHlprFwpV6AddrAndMaskCreateLocalCopy"
 
   Purpose:  Allocate and populate a local copy of an FWP_V6_ADDR_AND_MASK.                     <br>
                                                                                                <br>
   Notes:    The caller is responsible for freeing the allocated memory using
             KrnlHlprFwpV6AddrMaskDestroyLocalCopy().                                           <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF552446.aspx             <br>
*/
__drv_allocatesMem(Pool)
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Must_inspect_result_
_Success_(return != 0)
FWP_V6_ADDR_AND_MASK* KrnlHlprFwpV6AddrAndMaskCreateLocalCopy(_In_ const FWP_V6_ADDR_AND_MASK* pOriginalAddrMask)
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprFwpV6AddrAndMaskCreateLocalCopy()\n");

#endif /// DBG
   
   NT_ASSERT(pOriginalAddrMask);

   NTSTATUS              status    = STATUS_SUCCESS;
   FWP_V6_ADDR_AND_MASK* pAddrMask = 0;

#pragma warning(push)
#pragma warning(disable: 6014) /// pAddrMask is expected to be cleaned up by caller using KrnlHlprFwpV6AddrAndMaskDestroyLocalCopy

   HLPR_NEW(pAddrMask,
            FWP_V6_ADDR_AND_MASK,
            WFPSAMPLER_SYSLIB_TAG);
   HLPR_BAIL_ON_ALLOC_FAILURE(pAddrMask,
                              status);

#pragma warning(pop)

   status = KrnlHlprFwpV6AddrAndMaskPopulateLocalCopy(pOriginalAddrMask,
                                                      pAddrMask);

   HLPR_BAIL_LABEL:

#pragma warning(push)
#pragma warning(disable: 6001) /// pAddrMask initialized with call to HLPR_NEW & KrnlHlprFwpV6AddrAndMaskPopulateLocalCopy 

   if(status != STATUS_SUCCESS &&
      pAddrMask)
      KrnlHlprFwpV6AddrAndMaskDestroyLocalCopy(&pAddrMask);

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprFwpV6AddrAndMaskCreateLocalCopy() [pAddrMask: %#p]\n",
              pAddrMask);

#endif /// DBG

   return pAddrMask;

#pragma warning(pop)
}

#endif /// FWP_V6_ADDR_AND_MASK____

#ifndef FWP_RANGE____
#define FWP_RANGE____

/**
 @kernel_helper_function="KrnlHlprFwpRangePurgeLocalCopy"
 
   Purpose:  Cleanup a local copy of an FWP_RANGE.                                              <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF552438.aspx             <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
inline VOID KrnlHlprFwpRangePurgeLocalCopy(_Inout_ FWP_RANGE* pRange)
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprFwpRangePurgeLocalCopy()\n");

#endif /// DBG
   
   NT_ASSERT(pRange);

   KrnlHlprFwpValuePurgeLocalCopy(&(pRange->valueLow));

   KrnlHlprFwpValuePurgeLocalCopy(&(pRange->valueHigh));

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprFwpRangePurgeLocalCopy()\n");

#endif /// DBG

   return;
}

/**
 @kernel_helper_function="KrnlHlprFwpRangeDestroyLocalCopy"
 
   Purpose:  Cleanup and free a local copy of an FWP_RANGE.                                     <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF552438.aspx             <br>
*/
_At_(*ppRange, _Pre_ _Notnull_)
_At_(*ppRange, _Post_ _Null_ __drv_freesMem(Pool))
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Success_(*ppRange == 0)
inline VOID KrnlHlprFwpRangeDestroyLocalCopy(_Inout_ FWP_RANGE** ppRange)
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprFwpRangeDestroyLocalCopy()\n");

#endif /// DBG
   
   NT_ASSERT(ppRange);

   if(*ppRange)
      KrnlHlprFwpRangePurgeLocalCopy(*ppRange);

   HLPR_DELETE(*ppRange,
               WFPSAMPLER_SYSLIB_TAG);

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprFwpRangeDestroyLocalCopy()\n");

#endif /// DBG

   return;
}

/**
 @kernel_helper_function="KrnlHlprFwpRangePopulateLocalCopy"
 
   Purpose:  Populate a local copy of an FWP_RANGE.                                             <br>
                                                                                                <br>
   Notes:    The caller is responsible for freeing the allocated memory using
             KrnlHlprFwpRangePurgeLocalCopy().                                                  <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF552438.aspx             <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Check_return_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprFwpRangePopulateLocalCopy(_In_ const FWP_RANGE* pOriginalRange,
                                           _Inout_ FWP_RANGE* pRange)
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprFwpRangePopulateLocalCopy()\n");

#endif /// DBG
   
   NT_ASSERT(pOriginalRange);
   NT_ASSERT(pRange);

   NTSTATUS status = STATUS_SUCCESS;

   status = KrnlHlprFwpValuePopulateLocalCopy(&(pOriginalRange->valueLow),
                                              &(pRange->valueLow));
   HLPR_BAIL_ON_FAILURE(status);

   status = KrnlHlprFwpValuePopulateLocalCopy(&(pOriginalRange->valueHigh),
                                              &(pRange->valueHigh));
   HLPR_BAIL_ON_FAILURE(status);

   HLPR_BAIL_LABEL:

   if(status != STATUS_SUCCESS)
      KrnlHlprFwpRangePurgeLocalCopy(pRange);

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprFwpRangePopulateLocalCopy() [status: %#x]\n",
              status);

#endif /// DBG

   return status;
}

/**
 @kernel_helper_function="KrnlHlprFwpRangeCreateLocalCopy"
 
   Purpose:  Allocate and populate a local copy of an FWP_RANGE.                                <br>
                                                                                                <br>
   Notes:    The caller is responsible for freeing the allocated memory using
             KrnlHlprFwpRangeDestroyLocalCopy().                                                <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF552438.aspx             <br>
*/
__drv_allocatesMem(Pool)
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Must_inspect_result_
_Success_(return != 0)
FWP_RANGE* KrnlHlprFwpRangeCreateLocalCopy(_In_ const FWP_RANGE* pOriginalRange)
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprFwpRangeCreateLocalCopy()\n");

#endif /// DBG
   
   NT_ASSERT(pOriginalRange);

   NTSTATUS   status = STATUS_SUCCESS;
   FWP_RANGE* pRange = 0;

#pragma warning(push)
#pragma warning(disable: 6014) /// pRange is expected to be cleaned up by caller using KrnlHlprFwpRangeDestroyLocalCopy

   HLPR_NEW(pRange,
            FWP_RANGE,
            WFPSAMPLER_SYSLIB_TAG);
   HLPR_BAIL_ON_ALLOC_FAILURE(pRange,
                              status);

#pragma warning(pop)

   status = KrnlHlprFwpValuePopulateLocalCopy(&(pOriginalRange->valueLow),
                                              &(pRange->valueLow));
   HLPR_BAIL_ON_FAILURE(status);

   status = KrnlHlprFwpValuePopulateLocalCopy(&(pOriginalRange->valueHigh),
                                              &(pRange->valueHigh));
   HLPR_BAIL_ON_FAILURE(status);

   HLPR_BAIL_LABEL:

#pragma warning(push)
#pragma warning(disable: 6001) /// pRange initialized with call to HLPR_NEW & KrnlHlprFwpValuePopulateLocalCopy 

   if(status != STATUS_SUCCESS &&
      pRange)
      KrnlHlprFwpRangeDestroyLocalCopy(&pRange);

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprFwpRangeCreateLocalCopy() [pRange: %#p]\n",
              pRange);

#endif /// DBG

   return pRange;

#pragma warning(pop)
}

#endif /// FWP_RANGE____

#ifndef FWP_CONDITION_VALUE____
#define FWP_CONDITION_VALUE____

/**
 @kernel_helper_function="KrnlHlprFwpConditionValuePurgeLocalCopy"
 
   Purpose:  Cleanup a local copy of an FWP_CONDITION_VALUE.                                    <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF552430.aspx             <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
inline VOID KrnlHlprFwpConditionValuePurgeLocalCopy(_Inout_ FWP_CONDITION_VALUE* pValue)
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprFwpConditionValuePurgeLocalCopy()\n");

#endif /// DBG
   
   NT_ASSERT(pValue);

   switch(pValue->type)
   {
      case FWP_UINT64:
      {
         HLPR_DELETE(pValue->uint64,
                     WFPSAMPLER_SYSLIB_TAG);

         break;
      }
      case FWP_INT64:
      {
         HLPR_DELETE(pValue->int64,
                     WFPSAMPLER_SYSLIB_TAG);

         break;
      }
      case FWP_DOUBLE:
      {
         HLPR_DELETE(pValue->double64,
                     WFPSAMPLER_SYSLIB_TAG);

         break;
      }
      case FWP_BYTE_ARRAY16_TYPE:
      {
         HLPR_DELETE(pValue->byteArray16,
                     WFPSAMPLER_SYSLIB_TAG);

         break;
      }
      case FWP_BYTE_BLOB_TYPE:
      {
         KrnlHlprFwpByteBlobDestroyLocalCopy(&(pValue->byteBlob));

         break;
      }
      case FWP_SID:
      {
         HLPR_DELETE_ARRAY(pValue->sid,
                           WFPSAMPLER_SYSLIB_TAG);

         break;
      }
      case FWP_SECURITY_DESCRIPTOR_TYPE:
      {
         KrnlHlprFwpByteBlobDestroyLocalCopy(&(pValue->sd));

         break;
      }
      case FWP_TOKEN_INFORMATION_TYPE:
      {
         KrnlHlprFwpTokenInformationDestroyLocalCopy(&(pValue->tokenInformation));

         break;
      }
      case FWP_TOKEN_ACCESS_INFORMATION_TYPE:
      {
         KrnlHlprFwpByteBlobDestroyLocalCopy(&(pValue->tokenAccessInformation));

         break;
      }
      case FWP_UNICODE_STRING_TYPE:
      {
         HLPR_DELETE_ARRAY(pValue->unicodeString,
                           WFPSAMPLER_SYSLIB_TAG);

         break;
      }

#if(NTDDI_VERSION >= NTDDI_WIN7)

      case FWP_BYTE_ARRAY6_TYPE:
      {
         HLPR_DELETE(pValue->byteArray6,
                     WFPSAMPLER_SYSLIB_TAG);

         break;
      }

#endif /// (NTDDI_VERSION >= NTDDI_WIN7)

      case FWP_SINGLE_DATA_TYPE_MAX:
      {
         break;
      }
      case FWP_V4_ADDR_MASK:
      {
         KrnlHlprFwpV4AddrAndMaskDestroyLocalCopy(&(pValue->v4AddrMask));

         break;
      }
      case FWP_V6_ADDR_MASK:
      {
         KrnlHlprFwpV6AddrAndMaskDestroyLocalCopy(&(pValue->v6AddrMask));

         break;
      }
      case FWP_RANGE_TYPE:
      {
         KrnlHlprFwpRangeDestroyLocalCopy(&(pValue->rangeValue));

         break;
      }
      case FWP_DATA_TYPE_MAX:
      {
         break;
      }
      default:
      {
         pValue->uint64 = 0;

         break;
      }
   }

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprFwpConditionValuePurgeLocalCopy()\n");

#endif /// DBG

   return;
}

/**
 @kernel_helper_function="KrnlHlprFwpConditionValueDestroyLocalCopy"
 
   Purpose:  Cleanup and free a local copy of an FWP_CONDITION_VALUE.                           <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF552430.aspx             <br>
*/
_At_(*ppValue, _Pre_ _Notnull_)
_At_(*ppValue, _Post_ _Null_ __drv_freesMem(Pool))
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Success_(*ppValue == 0)
inline VOID KrnlHlprFwpConditionValueDestroyLocalCopy(_Inout_ FWP_CONDITION_VALUE** ppValue)
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprFwpConditionValueDestroyLocalCopy()\n");

#endif /// DBG
   
   NT_ASSERT(ppValue);

   if(*ppValue)
      KrnlHlprFwpConditionValuePurgeLocalCopy(*ppValue);

   HLPR_DELETE(*ppValue,
               WFPSAMPLER_SYSLIB_TAG);

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprFwpConditionValueDestroyLocalCopy()\n");

#endif /// DBG

   return;
}

/**
 @kernel_helper_function="KrnlHlprFwpConditionValuePopulateLocalCopy"
 
   Purpose:  Populate a local copy of an FWP_CONDITION_VALUE.                                   <br>
                                                                                                <br>
   Notes:    The caller is responsible for freeing any allocated memory using
             KrnlHlprFwpConditionValuePurgeLocalCopy().                                         <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF552430.aspx             <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Check_return_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprFwpConditionValuePopulateLocalCopy(_In_ const FWP_CONDITION_VALUE* pOriginalValue,
                                                    _Inout_ FWP_CONDITION_VALUE* pValue)
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprFwpConditionValuePopulateLocalCopy()\n");

#endif /// DBG
   
   NT_ASSERT(pOriginalValue);
   NT_ASSERT(pValue);

   NTSTATUS status = STATUS_SUCCESS;

   KIRQL irql = KeGetCurrentIrql();

   pValue->type = pOriginalValue->type;

   switch(pValue->type)
   {
      case FWP_UINT64:
      {
         HLPR_NEW(pValue->uint64,
                  UINT64,
                  WFPSAMPLER_SYSLIB_TAG);
         HLPR_BAIL_ON_ALLOC_FAILURE(pValue->uint64,
                                    status);

         RtlCopyMemory(pValue->uint64,
                       pOriginalValue->uint64,
                       sizeof(UINT64));

         break;
      }
      case FWP_INT64:
      {
         HLPR_NEW(pValue->int64,
                  INT64,
                  WFPSAMPLER_SYSLIB_TAG);
         HLPR_BAIL_ON_ALLOC_FAILURE(pValue->int64,
                                    status);

         RtlCopyMemory(pValue->int64,
                       pOriginalValue->int64,
                       sizeof(INT64));

         break;
      }
      case FWP_DOUBLE:
      {
         HLPR_NEW(pValue->double64,
                  double,
                  WFPSAMPLER_SYSLIB_TAG);
         HLPR_BAIL_ON_ALLOC_FAILURE(pValue->double64,
                                    status);

         RtlCopyMemory(pValue->double64,
                       pOriginalValue->double64,
                       sizeof(double));

         break;
      }
      case FWP_BYTE_ARRAY16_TYPE:
      {
         HLPR_NEW(pValue->byteArray16,
                  FWP_BYTE_ARRAY16,
                  WFPSAMPLER_SYSLIB_TAG);
         HLPR_BAIL_ON_ALLOC_FAILURE(pValue->byteArray16,
                                    status);

         RtlCopyMemory(pValue->byteArray16,
                       pOriginalValue->byteArray16,
                       sizeof(FWP_BYTE_ARRAY16));

         break;
      }
      case FWP_BYTE_BLOB_TYPE:
      {
         pValue->byteBlob = KrnlHlprFwpByteBlobCreateLocalCopy(pOriginalValue->byteBlob);
         HLPR_BAIL_ON_ALLOC_FAILURE(pValue->byteBlob,
                                    status);

         break;
      }
      case FWP_SID:
      {
         HLPR_DELETE_ARRAY(pValue->sid,
                           WFPSAMPLER_SYSLIB_TAG);

         break;
      }
      case FWP_SECURITY_DESCRIPTOR_TYPE:
      {
         pValue->sd = KrnlHlprFwpByteBlobCreateLocalCopy(pOriginalValue->sd);
         HLPR_BAIL_ON_ALLOC_FAILURE(pValue->sd,
                                    status);
         break;
      }
      case FWP_TOKEN_INFORMATION_TYPE:
      {
         pValue->tokenInformation = KrnlHlprFwpTokenInformationCreateLocalCopy(pOriginalValue->tokenInformation);
         HLPR_BAIL_ON_ALLOC_FAILURE(pValue->tokenInformation,
                                    status);

         break;
      }
      case FWP_TOKEN_ACCESS_INFORMATION_TYPE:
      {
         pValue->tokenAccessInformation = KrnlHlprFwpByteBlobCreateLocalCopy(pOriginalValue->tokenAccessInformation);
         HLPR_BAIL_ON_ALLOC_FAILURE(pValue->tokenAccessInformation,
                                    status);

         break;
      }
      case FWP_UNICODE_STRING_TYPE:
      {
         size_t stringSize = 0;

         if(irql == PASSIVE_LEVEL)
         {
            status = RtlStringCchLengthW(pOriginalValue->unicodeString,
                                         NTSTRSAFE_MAX_CCH,
                                         &stringSize);
            HLPR_BAIL_ON_FAILURE(status);
         }
         else
            stringSize = wcslen(pOriginalValue->unicodeString);

         if(stringSize)
         {
            HLPR_NEW_ARRAY(pValue->unicodeString,
                           WCHAR,
                           stringSize + 1,                     /// add space for '\0'
                           WFPSAMPLER_SYSLIB_TAG);
            HLPR_BAIL_ON_ALLOC_FAILURE(pValue->unicodeString,
                                       status);

#pragma warning(push)
#pragma warning(disable: 26018) /// buffers are contrained by stringSize

            RtlCopyMemory(pValue->unicodeString,
                          pOriginalValue->unicodeString,
                          stringSize * sizeof(WCHAR));

            pValue->unicodeString[stringSize] = L'\0';

#pragma warning(pop)
         }
         else
         {
            status = STATUS_INVALID_BUFFER_SIZE;

            HLPR_BAIL;
         }

         break;
      }

#if(NTDDI_VERSION >= NTDDI_WIN7)

      case FWP_BYTE_ARRAY6_TYPE:
      {
         HLPR_NEW(pValue->byteArray6,
                  FWP_BYTE_ARRAY6,
                  WFPSAMPLER_SYSLIB_TAG);
         HLPR_BAIL_ON_ALLOC_FAILURE(pValue->byteArray6,
                                    status);

         RtlCopyMemory(pValue->byteArray6,
                       pOriginalValue->byteArray6,
                       sizeof(FWP_BYTE_ARRAY6));

         break;
      }

#endif /// (NTDDI_VERSION >= NTDDI_WIN7)

      case FWP_SINGLE_DATA_TYPE_MAX:
      {
         status = STATUS_INVALID_LABEL;

         break;
      }
      case FWP_V4_ADDR_MASK:
      {
         pValue->v4AddrMask = KrnlHlprFwpV4AddrAndMaskCreateLocalCopy(pOriginalValue->v4AddrMask);
         HLPR_BAIL_ON_ALLOC_FAILURE(pValue->v4AddrMask,
                                    status);

         break;
      }
      case FWP_V6_ADDR_MASK:
      {
         pValue->v6AddrMask = KrnlHlprFwpV6AddrAndMaskCreateLocalCopy(pOriginalValue->v6AddrMask);
         HLPR_BAIL_ON_ALLOC_FAILURE(pValue->v6AddrMask,
                                    status);

         break;
      }
      case FWP_RANGE_TYPE:
      {
         pValue->rangeValue = KrnlHlprFwpRangeCreateLocalCopy(pOriginalValue->rangeValue);
         HLPR_BAIL_ON_ALLOC_FAILURE(pValue->rangeValue,
                                    status);

         break;
      }
      case FWP_DATA_TYPE_MAX:
      {
         status = STATUS_INVALID_LABEL;

         break;
      }
      default:
      {
         /// this is a straight value copy so it's safe to use the largest static space
         pValue->uint64 = pOriginalValue->uint64;

         break;
      }
   }

   HLPR_BAIL_LABEL:

   if(status != STATUS_SUCCESS)
      KrnlHlprFwpConditionValuePurgeLocalCopy(pValue);

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprFwpConditionValuePopulateLocalCopy() [status: %#x]\n",
              status);

#endif /// DBG

   return status;
}

/**
 @kernel_helper_function="KrnlHlprFwpConditionValueCreateLocalCopy"
 
   Purpose:  Allocate ans populate a local copy of an FWP_CONDITION_VALUE.                      <br>
                                                                                                <br>
   Notes:    The caller is responsible for freeing any allocated memory using
             KrnlHlprFwpConditionValueDestroyLocalCopy().                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF552430.aspx             <br>
*/
__drv_allocatesMem(Pool)
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Must_inspect_result_
_Success_(return != 0)
FWP_CONDITION_VALUE* KrnlHlprFwpConditionValueCreateLocalCopy(_In_ const FWP_CONDITION_VALUE* pOriginalValue)
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprFwpConditionValueCreateLocalCopy()\n");

#endif /// DBG
   
   NT_ASSERT(pOriginalValue);

   NTSTATUS             status          = STATUS_SUCCESS;
   FWP_CONDITION_VALUE* pConditionValue = 0;

#pragma warning(push)
#pragma warning(disable: 6014) /// pConditionValue is expected to be cleaned up by caller using KrnlHlprFwpConditionValueDestroyLocalCopy

   HLPR_NEW(pConditionValue,
            FWP_CONDITION_VALUE,
            WFPSAMPLER_SYSLIB_TAG);
   HLPR_BAIL_ON_ALLOC_FAILURE(pConditionValue,
                              status);

#pragma warning(push)

   status = KrnlHlprFwpConditionValuePopulateLocalCopy(pOriginalValue,
                                                       pConditionValue);

   HLPR_BAIL_LABEL:

#pragma warning(push)
#pragma warning(disable: 6001) /// pConditionValue initialized with call to HLPR_NEW & KrnlHlprFwpConditionValuePopulateLocalCopy 

   if(status != STATUS_SUCCESS &&
      pConditionValue)
      KrnlHlprFwpConditionValueDestroyLocalCopy(&pConditionValue);

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprFwpConditionValueCreateLocalCopy() [pConditionValue: %#p]\n",
              pConditionValue);

#endif /// DBG

   return pConditionValue;

#pragma warning(pop)
}

#endif /// FWP_CONDITION_VALUE____

#ifndef FWPS_INCOMING_VALUES____
#define FWPS_INCOMING_VALUES____

/**
 @kernel_helper_function="KrnlHlprFwpsIncomingValueConditionFlagsAreSet"
 
   Purpose:  Determine if the FWPS_INCOMING_VALUES has the FLAGS condition set with the given 
             flags.                                                                             <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF552401.aspx             <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Check_return_
BOOLEAN KrnlHlprFwpsIncomingValueConditionFlagsAreSet(_In_ const FWPS_INCOMING_VALUES* pClassifyValues,
                                                      _In_ UINT32 flags)
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprFwpsIncomingValueConditionFlagsAreSet()\n");

#endif /// DBG
   
   NT_ASSERT(pClassifyValues);

   BOOLEAN    flagsAreSet = FALSE;
   FWP_VALUE* pFlagsValue = KrnlHlprFwpValueGetFromFwpsIncomingValues(pClassifyValues,
                                                                      &FWPM_CONDITION_FLAGS);

   if(pFlagsValue &&
      pFlagsValue->uint32 & flags)
      flagsAreSet = TRUE;

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprFwpsIncomingValueConditionFlagsAreSet()\n");

#endif /// DBG

   return flagsAreSet;
}

/**
 @kernel_helper_function="KrnlHlprFwpsIncomingValuesPurgeLocalCopy"
 
   Purpose:  Cleanup a local copy of an FWPS_INCOMING_VALUES.                                   <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF552401.aspx             <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
inline VOID KrnlHlprFwpsIncomingValuesPurgeLocalCopy(_Inout_ FWPS_INCOMING_VALUES* pValues)
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprFwpsIncomingValuesPurgeLocalCopy()\n");

#endif /// DBG
   
   NT_ASSERT(pValues);

   for(UINT32 valueIndex = 0;
       valueIndex < pValues->valueCount;
       valueIndex++)
   {
      KrnlHlprFwpValuePurgeLocalCopy(&(pValues->incomingValue[valueIndex].value));
   }

   HLPR_DELETE_ARRAY(pValues->incomingValue,
                     WFPSAMPLER_SYSLIB_TAG);

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprFwpsIncomingValuesPurgeLocalCopy()\n");

#endif /// DBG

   return;
}

/**
 @kernel_helper_function="KrnlHlprFwpsIncomingValuesDestroyLocalCopy"
 
   Purpose:  Cleanup and free a local copy of an FWPS_INCOMING_VALUES.                          <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF552401.aspx             <br>
*/
_At_(*ppValues, _Pre_ _Notnull_)
_At_(*ppValues, _Post_ _Null_ __drv_freesMem(Pool))
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Success_(*ppValues == 0)
inline VOID KrnlHlprFwpsIncomingValuesDestroyLocalCopy(_Inout_ FWPS_INCOMING_VALUES** ppValues)
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprFwpsIncomingValuesDestroyLocalCopy()\n");

#endif /// DBG
   
   NT_ASSERT(ppValues);

   if(*ppValues)
      KrnlHlprFwpsIncomingValuesPurgeLocalCopy(*ppValues);
      
   HLPR_DELETE(*ppValues,
               WFPSAMPLER_SYSLIB_TAG);

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprFwpsIncomingValuesDestroyLocalCopy()\n");

#endif /// DBG

   return;
}

/**
 @kernel_helper_function="KrnlHlprFwpsIncomingValuesPopulateLocalCopy"
 
   Purpose:  Populate a local copy of an FWPS_INCOMING_VALUES.                                  <br>
                                                                                                <br>
   Notes:    The caller is responsible for freeing the allocated memory using
             KrnlHlprFwpsIncomingValuesPurgeLocalCopy().                                        <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF552401.aspx             <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Check_return_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprFwpsIncomingValuesPopulateLocalCopy(_In_ const FWPS_INCOMING_VALUES* pOriginalValues,
                                                     _Inout_ FWPS_INCOMING_VALUES* pValues)
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprFwpsIncomingValuesPopulateLocalCopy()\n");

#endif /// DBG
   
   NT_ASSERT(pOriginalValues);
   NT_ASSERT(pValues);

   NTSTATUS status = STATUS_SUCCESS;

   HLPR_NEW_ARRAY(pValues->incomingValue,
                  FWPS_INCOMING_VALUE,
                  pOriginalValues->valueCount,
                  WFPSAMPLER_SYSLIB_TAG);
   HLPR_BAIL_ON_ALLOC_FAILURE(pValues->incomingValue,
                              status);

   for(UINT32 valueIndex = 0;
       valueIndex < pOriginalValues->valueCount;
       valueIndex++)
   {
      status = KrnlHlprFwpValuePopulateLocalCopy(&(pOriginalValues->incomingValue[valueIndex].value),
                                                 &(pValues->incomingValue[valueIndex].value));
      HLPR_BAIL_ON_FAILURE(status);
   }

   pValues->valueCount = pOriginalValues->valueCount;
   pValues->layerId    = pOriginalValues->layerId;

   HLPR_BAIL_LABEL:

   if(status != STATUS_SUCCESS)
      KrnlHlprFwpsIncomingValuesPurgeLocalCopy(pValues);

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprFwpsIncomingValuesPopulateLocalCopy() [status: %#x]\n",
              status);

#endif /// DBG

   return status;
}

/**
 @kernel_helper_function="KrnlHlprFwpsIncomingValuesCreateLocalCopy"
 
   Purpose:  Allocate and populate a local copy of an FWPS_INCOMING_VALUES.                     <br>
                                                                                                <br>
   Notes:    The caller is responsible for freeing the allocated memory using
             KrnlHlprFwpsIncomingValuesDestroyLocalCopy().                                      <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF552401.aspx             <br>
*/
__drv_allocatesMem(Pool)
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Must_inspect_result_
_Success_(return != 0)
FWPS_INCOMING_VALUES* KrnlHlprFwpsIncomingValuesCreateLocalCopy(_In_ const FWPS_INCOMING_VALUES* pOriginalValues)
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprFwpsIncomingValuesCreateLocalCopy()\n");

#endif /// DBG
   
   NT_ASSERT(pOriginalValues);

   NTSTATUS              status          = STATUS_SUCCESS;
   FWPS_INCOMING_VALUES* pIncomingValues = 0;

#pragma warning(push)
#pragma warning(disable: 6014) /// pIncomingValues is expected to be cleaned up by caller using KrnlHlprFwpsIncomingValuesDestroyLocalCopy

   HLPR_NEW(pIncomingValues,
            FWPS_INCOMING_VALUES,
            WFPSAMPLER_SYSLIB_TAG);
   HLPR_BAIL_ON_ALLOC_FAILURE(pIncomingValues,
                               status);

#pragma warning(pop)

   status = KrnlHlprFwpsIncomingValuesPopulateLocalCopy(pOriginalValues,
                                                        pIncomingValues);

   HLPR_BAIL_LABEL:

#pragma warning(push)
#pragma warning(disable: 6001) /// pIncomingValues initialized with call to HLPR_NEW & KrnlHlprFwpsIncomingValuesPopulateLocalCopy 

   if(status != STATUS_SUCCESS &&
      pIncomingValues)
      KrnlHlprFwpsIncomingValuesDestroyLocalCopy(&pIncomingValues);

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprFwpsIncomingValuesCreateLocalCopy() [pIncomingValues: %#p]\n",
              pIncomingValues);

#endif /// DBG

   return pIncomingValues;

#pragma warning(pop)
}

#endif /// FWPS_INCOMING_VALUES____

#ifndef FWPS_INCOMING_METADATA_VALUES____
#define FWPS_INCOMING_METADATA_VALUES____

/**
 @kernel_helper_function="KrnlHlprFwpsIncomingMetadataValuesPurgeLocalCopy
 
   Purpose:  Cleanup a local copy of an FWPS_INCOMING_METADATA_VALUES.                          <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF552397.aspx             <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
inline VOID KrnlHlprFwpsIncomingMetadataValuesPurgeLocalCopy(_Inout_ FWPS_INCOMING_METADATA_VALUES* pMetadata)
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprFwpsIncomingMetadataValuesPurgeLocalCopy()\n");

#endif /// DBG
   
   NT_ASSERT(pMetadata);

   KrnlHlprFwpByteBlobDestroyLocalCopy(&(pMetadata->processPath));

   HLPR_DELETE_ARRAY(pMetadata->controlData,
                     WFPSAMPLER_SYSLIB_TAG);

#if(NTDDI_VERSION >= NTDDI_WIN6SP1)

   HLPR_DELETE_ARRAY(pMetadata->headerIncludeHeader,
                     WFPSAMPLER_SYSLIB_TAG);

#if(NTDDI_VERSION >= NTDDI_WIN7)

   HLPR_DELETE_ARRAY(pMetadata->originalDestination,
                     WFPSAMPLER_SYSLIB_TAG);

#if(NTDDI_VERSION >= NTDDI_WIN8)

   if(pMetadata->vSwitchPacketContext)
      FwpsDereferencevSwitchPacketContext(pMetadata->vSwitchPacketContext);

#endif /// (NTDDI_VERSION >= NTDDI_WIN8)
#endif /// (NTDDI_VERSION >= NTDDI_WIN7)
#endif /// (NTDDI_VERSION >= NTDDI_WIN6SP1)

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprFwpsIncomingMetadataValuesPurgeLocalCopy()\n");

#endif /// DBG

   return;
}

/**
 @kernel_helper_function="KrnlHlprFwpsIncomingMetadataValuesDestroyLocalCopy
 
   Purpose:  Cleanup and free a local copy of an FWPS_INCOMING_METADATA_VALUES.                 <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF552397.aspx             <br>
*/
_At_(*ppMetadata, _Pre_ _Notnull_)
_At_(*ppMetadata, _Post_ _Null_ __drv_freesMem(Pool))
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Success_(*ppMetadata == 0)
inline VOID KrnlHlprFwpsIncomingMetadataValuesDestroyLocalCopy(_Inout_ FWPS_INCOMING_METADATA_VALUES** ppMetadata)
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprFwpsIncomingMetadataValuesDestroyLocalCopy()\n");

#endif /// DBG
   
   NT_ASSERT(ppMetadata);

   if(*ppMetadata)
      KrnlHlprFwpsIncomingMetadataValuesPurgeLocalCopy(*ppMetadata);

   HLPR_DELETE(*ppMetadata,
               WFPSAMPLER_SYSLIB_TAG);

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprFwpsIncomingMetadataValuesDestroyLocalCopy()\n");

#endif /// DBG

   return;
}

/**
 @kernel_helper_function="KrnlHlprFwpsIncomingMetadataValuesPopulateLocalCopy
 
   Purpose:  Populate a local copy of the FWPS_INCOMING_METADATA_VALUES.                        <br>
                                                                                                <br>
   Notes:    The caller is responsible for freeing any allocated memory using 
             KrnlHlprFwpsIncomingMetadataValuesPurgeLocalCopy().                                <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF552397.aspx             <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Check_return_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprFwpsIncomingMetadataValuesPopulateLocalCopy(_In_ const FWPS_INCOMING_METADATA_VALUES* pOriginalMetadata,
                                                             _Inout_ FWPS_INCOMING_METADATA_VALUES* pMetadata)
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprFwpsIncomingMetadataValuesPopulateLocalCopy()\n");

#endif /// DBG
   
   NT_ASSERT(pOriginalMetadata);

   NTSTATUS status = STATUS_SUCCESS;

   pMetadata->currentMetadataValues = pOriginalMetadata->currentMetadataValues;
   pMetadata->flags                 = pOriginalMetadata->flags;

   /// Used internally by the filter engine. Callout drivers should ignore this member.
   if(FWPS_IS_METADATA_FIELD_PRESENT(pOriginalMetadata,
                                     FWPS_METADATA_FIELD_RESERVED))
      pMetadata->reserved = pOriginalMetadata->reserved;

   if(FWPS_IS_METADATA_FIELD_PRESENT(pOriginalMetadata,
                                     FWPS_METADATA_FIELD_DISCARD_REASON))
   {
      pMetadata->discardMetadata.discardModule = pOriginalMetadata->discardMetadata.discardModule;
      pMetadata->discardMetadata.discardReason = pOriginalMetadata->discardMetadata.discardReason;
      pMetadata->discardMetadata.filterId      = pOriginalMetadata->discardMetadata.filterId;
   }

   /// Not guaranteed to be available outside of the classifyFn.
   /// Use of an invalid handle will fail gracefully.
   if(FWPS_IS_METADATA_FIELD_PRESENT(pOriginalMetadata,
                                     FWPS_METADATA_FIELD_FLOW_HANDLE))
      pMetadata->flowHandle = pOriginalMetadata->flowHandle;

   if(FWPS_IS_METADATA_FIELD_PRESENT(pOriginalMetadata,
                                     FWPS_METADATA_FIELD_IP_HEADER_SIZE))
      pMetadata->ipHeaderSize = pOriginalMetadata->ipHeaderSize;

   if(FWPS_IS_METADATA_FIELD_PRESENT(pOriginalMetadata,
                                     FWPS_METADATA_FIELD_TRANSPORT_HEADER_SIZE))
      pMetadata->transportHeaderSize = pOriginalMetadata->transportHeaderSize;

   if(FWPS_IS_METADATA_FIELD_PRESENT(pOriginalMetadata,
                                     FWPS_METADATA_FIELD_PROCESS_PATH))
   {
      pMetadata->processPath = KrnlHlprFwpByteBlobCreateLocalCopy(pOriginalMetadata->processPath);
      HLPR_BAIL_ON_ALLOC_FAILURE(pMetadata->processPath,
                                 status);
   }

   if(FWPS_IS_METADATA_FIELD_PRESENT(pOriginalMetadata,
                                     FWPS_METADATA_FIELD_TOKEN))
      pMetadata->token = pOriginalMetadata->token;

   if(FWPS_IS_METADATA_FIELD_PRESENT(pOriginalMetadata,
                                     FWPS_METADATA_FIELD_PROCESS_ID))
      pMetadata->processId = pOriginalMetadata->processId;

   if(FWPS_IS_METADATA_FIELD_PRESENT(pOriginalMetadata,
                                     FWPS_METADATA_FIELD_SOURCE_INTERFACE_INDEX))
      pMetadata->sourceInterfaceIndex = pOriginalMetadata->sourceInterfaceIndex;

   if(FWPS_IS_METADATA_FIELD_PRESENT(pOriginalMetadata,
                                     FWPS_METADATA_FIELD_DESTINATION_INTERFACE_INDEX))
      pMetadata->destinationInterfaceIndex = pOriginalMetadata->destinationInterfaceIndex;

   if(FWPS_IS_METADATA_FIELD_PRESENT(pOriginalMetadata,
                                     FWPS_METADATA_FIELD_COMPARTMENT_ID))
      pMetadata->compartmentId = pOriginalMetadata->compartmentId;

   if(FWPS_IS_METADATA_FIELD_PRESENT(pOriginalMetadata,
                                     FWPS_METADATA_FIELD_FRAGMENT_DATA))
   {
      pMetadata->fragmentMetadata.fragmentIdentification = pOriginalMetadata->fragmentMetadata.fragmentIdentification;
      pMetadata->fragmentMetadata.fragmentOffset         = pOriginalMetadata->fragmentMetadata.fragmentOffset;
      pMetadata->fragmentMetadata.fragmentLength         = pOriginalMetadata->fragmentMetadata.fragmentLength;
   }

   if(FWPS_IS_METADATA_FIELD_PRESENT(pOriginalMetadata,
                                     FWPS_METADATA_FIELD_PATH_MTU))
      pMetadata->pathMtu = pOriginalMetadata->pathMtu;

   /// Not guaranteed to be available outside of the classifyFn.
   /// Use of an invalid handle will fail gracefully.
   if(FWPS_IS_METADATA_FIELD_PRESENT(pOriginalMetadata,
                                     FWPS_METADATA_FIELD_COMPLETION_HANDLE))
      pMetadata->completionHandle = pOriginalMetadata->completionHandle;

   /// Not guaranteed to be available outside of the classifyFn.
   /// Use of an invalid handle will fail gracefully.
   if(FWPS_IS_METADATA_FIELD_PRESENT(pOriginalMetadata,
                                     FWPS_METADATA_FIELD_TRANSPORT_ENDPOINT_HANDLE))
      pMetadata->transportEndpointHandle = pOriginalMetadata->transportEndpointHandle;

   if(FWPS_IS_METADATA_FIELD_PRESENT(pOriginalMetadata,
                                     FWPS_METADATA_FIELD_REMOTE_SCOPE_ID))
      pMetadata->remoteScopeId = pOriginalMetadata->remoteScopeId;

   if(FWPS_IS_METADATA_FIELD_PRESENT(pOriginalMetadata,
                                     FWPS_METADATA_FIELD_TRANSPORT_CONTROL_DATA))
   {
      BYTE* pControlData = 0;

      HLPR_NEW_ARRAY(pControlData,
                     BYTE,
                     pOriginalMetadata->controlDataLength,
                     WFPSAMPLER_SYSLIB_TAG);
      HLPR_BAIL_ON_ALLOC_FAILURE(pControlData,
                                 status);

      RtlCopyMemory(pControlData,
                    pOriginalMetadata->controlData,
                    pOriginalMetadata->controlDataLength);

      pMetadata->controlData       = (WSACMSGHDR*)pControlData;
      pMetadata->controlDataLength = pOriginalMetadata->controlDataLength;
   }

   if(FWPS_IS_METADATA_FIELD_PRESENT(pOriginalMetadata,
                                     FWPS_METADATA_FIELD_PACKET_DIRECTION))
      pMetadata->packetDirection= pOriginalMetadata->packetDirection;

#if(NTDDI_VERSION >= NTDDI_WIN6SP1)

   if(FWPS_IS_METADATA_FIELD_PRESENT(pOriginalMetadata,
                                     FWPS_METADATA_FIELD_TRANSPORT_HEADER_INCLUDE_HEADER))
   {
      HLPR_NEW_ARRAY(pMetadata->headerIncludeHeader,
                     BYTE,
                     pOriginalMetadata->headerIncludeHeaderLength,
                     WFPSAMPLER_SYSLIB_TAG);
      HLPR_BAIL_ON_ALLOC_FAILURE(pMetadata->headerIncludeHeader,
                                 status);

      RtlCopyMemory(pMetadata->headerIncludeHeader,
                    pOriginalMetadata->headerIncludeHeader,
                    pOriginalMetadata->headerIncludeHeaderLength);

      pMetadata->headerIncludeHeaderLength = pOriginalMetadata->headerIncludeHeaderLength;
   }

#if(NTDDI_VERSION >= NTDDI_WIN7)

   if(FWPS_IS_METADATA_FIELD_PRESENT(pOriginalMetadata,
                                     FWPS_METADATA_FIELD_DESTINATION_PREFIX))
      pMetadata->destinationPrefix = pOriginalMetadata->destinationPrefix;

   /// Not guaranteed to be available outside of the classifyFn.
   /// Use of an invalid handle will fail gracefully.
   if(FWPS_IS_METADATA_FIELD_PRESENT(pOriginalMetadata,
                                     FWPS_METADATA_FIELD_PARENT_ENDPOINT_HANDLE))
      pMetadata->parentEndpointHandle  = pOriginalMetadata->parentEndpointHandle;

   if(FWPS_IS_METADATA_FIELD_PRESENT(pOriginalMetadata,
                                     FWPS_METADATA_FIELD_ICMP_ID_AND_SEQUENCE))
      pMetadata->icmpIdAndSequence = pOriginalMetadata->icmpIdAndSequence;

   if(FWPS_IS_METADATA_FIELD_PRESENT(pOriginalMetadata,
                                     FWPS_METADATA_FIELD_LOCAL_REDIRECT_TARGET_PID))
      pMetadata->localRedirectTargetPID = pOriginalMetadata->localRedirectTargetPID;

   if(FWPS_IS_METADATA_FIELD_PRESENT(pOriginalMetadata,
                                     FWPS_METADATA_FIELD_ORIGINAL_DESTINATION))
   {
      BYTE*  pDestination = 0;
      SIZE_T sockAddrSize = (pOriginalMetadata->originalDestination->sa_family == AF_INET) ? 
                            sizeof(SOCKADDR_IN) :
                            sizeof(SOCKADDR_IN6);

      HLPR_NEW_ARRAY(pDestination,
                     BYTE,
                     sockAddrSize,
                     WFPSAMPLER_SYSLIB_TAG);
      HLPR_BAIL_ON_ALLOC_FAILURE(pDestination,
                                 status);

      RtlCopyMemory(pMetadata->originalDestination,
                    pOriginalMetadata->originalDestination,
                    sockAddrSize);

      pMetadata->originalDestination = (SOCKADDR*)pDestination;
   }

#if(NTDDI_VERSION >= NTDDI_WIN8)

   if(FWPS_IS_METADATA_FIELD_PRESENT(pOriginalMetadata,
                                     FWPS_METADATA_FIELD_REDIRECT_RECORD_HANDLE))
      pMetadata->redirectRecords = pOriginalMetadata->redirectRecords;

   if(FWPS_IS_METADATA_FIELD_PRESENT(pOriginalMetadata,
                                     FWPS_METADATA_FIELD_SUB_PROCESS_TAG))
      pMetadata->subProcessTag = pOriginalMetadata->subProcessTag;

   pMetadata->currentL2MetadataValues = pOriginalMetadata->currentL2MetadataValues;
   pMetadata->l2Flags                 = pOriginalMetadata->l2Flags;

   if(FWPS_IS_L2_METADATA_FIELD_PRESENT(pOriginalMetadata,
                                        FWPS_L2_METADATA_FIELD_ETHERNET_MAC_HEADER_SIZE))
      pMetadata->ethernetMacHeaderSize = pOriginalMetadata->ethernetMacHeaderSize;

   if(FWPS_IS_L2_METADATA_FIELD_PRESENT(pOriginalMetadata,
                                        FWPS_L2_METADATA_FIELD_WIFI_OPERATION_MODE))
      pMetadata->wiFiOperationMode = pOriginalMetadata->wiFiOperationMode;

#if(NDIS_SUPPORT_NDIS630)

   if(FWPS_IS_L2_METADATA_FIELD_PRESENT(pOriginalMetadata,
                                        FWPS_L2_METADATA_FIELD_VSWITCH_SOURCE_PORT_ID))
      pMetadata->vSwitchSourcePortId = pOriginalMetadata->vSwitchSourcePortId;

   if(FWPS_IS_L2_METADATA_FIELD_PRESENT(pOriginalMetadata,
                                        FWPS_L2_METADATA_FIELD_VSWITCH_SOURCE_NIC_INDEX))
      pMetadata->vSwitchSourceNicIndex = pOriginalMetadata->vSwitchSourceNicIndex;

   if(FWPS_IS_L2_METADATA_FIELD_PRESENT(pOriginalMetadata,
                                        FWPS_L2_METADATA_FIELD_VSWITCH_DESTINATION_PORT_ID))
      pMetadata->vSwitchDestinationPortId = pOriginalMetadata->vSwitchDestinationPortId;

#endif /// (NDIS_SUPPORT_NDIS630)

   /// Technically only need this if allocating and injecting new NBLs
   if(FWPS_IS_L2_METADATA_FIELD_PRESENT(pOriginalMetadata,
                                        FWPS_L2_METADATA_FIELD_VSWITCH_PACKET_CONTEXT))
   {
      FwpsReferencevSwitchPacketContext(pOriginalMetadata->vSwitchPacketContext);

      pMetadata->vSwitchPacketContext = pOriginalMetadata->vSwitchPacketContext;
   }

#endif /// (NTDDI_VERSION >= NTDDI_WIN8)
#endif /// (NTDDI_VERSION >= NTDDI_WIN7)
#endif /// (NTDDI_VERSION >= NTDDI_WIN6SP1)

   HLPR_BAIL_LABEL:

   if(status != STATUS_SUCCESS)
      KrnlHlprFwpsIncomingMetadataValuesPurgeLocalCopy(pMetadata);

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprFwpsIncomingMetadataValuesPopulateLocalCopy() [status: %#x]\n",
              status);

#endif /// DBG

   return status;
}

/**
 @kernel_helper_function="KrnlHlprFwpsIncomingMetadataValuesCreateLocalCopy
 
   Purpose:  Allocate and populate a local copy of the FWPS_INCOMING_METADATA_VALUES.           <br>
                                                                                                <br>
   Notes:    The caller is responsible for freeing the allocated memory using
             KrnlHlprFwpsIncomingMetadataValuesDestroyLocalCopy().                              <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF552397.aspx             <br>
*/
__drv_allocatesMem(Pool)
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Must_inspect_result_
_Success_(return != 0)
FWPS_INCOMING_METADATA_VALUES* KrnlHlprFwpsIncomingMetadataValuesCreateLocalCopy(_In_ const FWPS_INCOMING_METADATA_VALUES* pOriginalMetadata)
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprFwpsIncomingMetadataValuesCreateLocalCopy()\n");

#endif /// DBG
   
   NT_ASSERT(pOriginalMetadata);

   NTSTATUS                       status    = STATUS_SUCCESS;
   FWPS_INCOMING_METADATA_VALUES* pMetadata = 0;

#pragma warning(push)
#pragma warning(disable: 6014) /// pMetadata is expected to be cleaned up by caller using KrnlHlprFwpsIncomingMetadataValuesDestroyLocalCopy

   HLPR_NEW(pMetadata,
            FWPS_INCOMING_METADATA_VALUES,
            WFPSAMPLER_SYSLIB_TAG);
   HLPR_BAIL_ON_ALLOC_FAILURE(pMetadata,
                              status);

#pragma warning(pop)

   status = KrnlHlprFwpsIncomingMetadataValuesPopulateLocalCopy(pOriginalMetadata,
                                                                pMetadata);

   HLPR_BAIL_LABEL:

#pragma warning(push)
#pragma warning(disable: 6001) /// pMetadata initialized with call to HLPR_NEW & KrnlHlprFwpsIncomingMetadataValuesPopulateLocalCopy 

   if(status != STATUS_SUCCESS &&
      pMetadata)
      KrnlHlprFwpsIncomingMetadataValuesDestroyLocalCopy(&pMetadata);

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprFwpsIncomingMetadataValuesCreateLocalCopy() [pMetadata: %#p]\n",
              pMetadata);

#endif /// DBG

   return pMetadata;

#pragma warning(pop)
}

#endif /// FWPS_INCOMING_METADATA_VALUES____

#ifndef FWPS_CLASSIFY_OUT____
#define FWPS_CLASSIFY_OUT____

/**
 @kernel_helper_function="KrnlHlprFwpsClassifyOutPurgeLocalCopy"
 
   Purpose:  Cleanup  a local copy of an FWPS_CLASSIFY_OUT.                                     <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF551229.aspx             <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
inline VOID KrnlHlprFwpsClassifyOutPurgeLocalCopy(_Inout_ FWPS_CLASSIFY_OUT* pOriginalClassifyOut)
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprFwpsClassifyOutPurgeLocalCopy()\n");

#endif /// DBG
   
   NT_ASSERT(pOriginalClassifyOut);

   RtlZeroMemory(pOriginalClassifyOut,
                 sizeof(FWPS_CLASSIFY_OUT));

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprFwpsClassifyOutPurgeLocalCopy()\n");

#endif /// DBG

   return;
}

/**
 @kernel_helper_function="KrnlHlprFwpsClassifyOutDestroyLocalCopy"
 
   Purpose:  Cleanup and free a local copy of an FWPS_CLASSIFY_OUT.                             <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF551229.aspx             <br>
*/
_At_(*ppClassifyOut, _Pre_ _Notnull_)
_At_(*ppClassifyOut, _Post_ _Null_ __drv_freesMem(Pool))
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Success_(*ppClassifyOut == 0)
VOID KrnlHlprFwpsClassifyOutDestroyLocalCopy(_Inout_ FWPS_CLASSIFY_OUT** ppClassifyOut)
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprFwpsClassifyOutDestroyLocalCopy()\n");

#endif /// DBG
   
   NT_ASSERT(ppClassifyOut);

   if(*ppClassifyOut)
      KrnlHlprFwpsClassifyOutPurgeLocalCopy(*ppClassifyOut);

   HLPR_DELETE(*ppClassifyOut,
               WFPSAMPLER_SYSLIB_TAG);

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprFwpsClassifyOutDestroyLocalCopy()\n");

#endif /// DBG

   return;
}

/**
 @kernel_helper_function="KrnlHlprFwpsClassifyOutPopulateLocalCopy"
 
   Purpose:  Populate a local copy of the FWPS_CLASSIFY_OUT.                                    <br>
                                                                                                <br>
   Notes:    The caller is responsible for freeing the allocated memory using
             KrnlHlprFwpsClassifyOutPurgeLocalCopy().                                           <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF551229.aspx             <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
inline VOID KrnlHlprFwpsClassifyOutPopulateLocalCopy(_In_ const FWPS_CLASSIFY_OUT* pOriginalClassifyOut,
                                                     _Inout_ FWPS_CLASSIFY_OUT* pClassifyOut)
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprFwpsClassifyOutPopulateLocalCopy()\n");

#endif /// DBG
   
   NT_ASSERT(pOriginalClassifyOut);
   NT_ASSERT(pClassifyOut);

   RtlCopyMemory(pClassifyOut,
                 pOriginalClassifyOut,
                 sizeof(FWPS_CLASSIFY_OUT));

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprFwpsClassifyOutPopulateLocalCopy()\n");

#endif /// DBG

   return;
}

/**
 @kernel_helper_function="KrnlHlprFwpsClassifyOutCreateLocalCopy"
 
   Purpose:  Allocate and populate a local copy of the FWPS_CLASSIFY_OUT.                       <br>
                                                                                                <br>
   Notes:    The caller is responsible for freeing the allocated memory using
             KrnlHlprFwpsClassifyOutDestroyLocalCopy().                                         <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF551229.aspx             <br>
*/
__drv_allocatesMem(Pool)
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Must_inspect_result_
_Success_(return != 0)
FWPS_CLASSIFY_OUT* KrnlHlprFwpsClassifyOutCreateLocalCopy(_In_ const FWPS_CLASSIFY_OUT* pOriginalClassifyOut)
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprFwpsClassifyOutCreateLocalCopy()\n");

#endif /// DBG
   
   NT_ASSERT(pOriginalClassifyOut);

   NTSTATUS           status       = 0;
   FWPS_CLASSIFY_OUT* pClassifyOut = 0;

   HLPR_NEW(pClassifyOut,
            FWPS_CLASSIFY_OUT,
            WFPSAMPLER_SYSLIB_TAG);
   HLPR_BAIL_ON_ALLOC_FAILURE(pClassifyOut,
                              status);

   KrnlHlprFwpsClassifyOutPopulateLocalCopy(pOriginalClassifyOut,
                                            pClassifyOut);

   HLPR_BAIL_LABEL:

   if(status != STATUS_SUCCESS)
      DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_ERROR_LEVEL,
              " !!!! KrnlHlprFwpsClassifyOutCreateLocalCopy() [status: %#x]\n",
              status);

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprFwpsClassifyOutCreateLocalCopy() [pClassifyOut: %#p]\n",
              pClassifyOut);

#endif /// DBG

   return pClassifyOut;
}

#endif /// FWPS_CLASSIFY_OUT____

#ifndef FWPS_STREAM_DATA____
#define FWPS_STREAM_DATA____

/**
 @kernel_helper_function="KrnlHlprFwpsStreamDataPurgeLocalCopy"
 
   Purpose:  Cleanup a local copy of an FWPS_STREAM_DATA.                                       <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF552419.aspx             <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
inline VOID KrnlHlprFwpsStreamDataPurgeLocalCopy(_Inout_ FWPS_STREAM_DATA* pStreamData)
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprFwpsStreamDataPurgeLocalCopy()\n");

#endif /// DBG
   
   NT_ASSERT(pStreamData);

   BOOLEAN isDispatch = (KeGetCurrentIrql() == DISPATCH_LEVEL) ? TRUE : FALSE;

   for(NET_BUFFER_LIST* pNBL = pStreamData->netBufferListChain;
       pNBL;
       )
   {
      NET_BUFFER_LIST* pNBLNext = NET_BUFFER_LIST_NEXT_NBL(pNBL);

      FwpsDereferenceNetBufferList(pNBL,
                                   isDispatch);

      pNBL = pNBLNext;
   }

   pStreamData->netBufferListChain = 0;

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprFwpsStreamDataPurgeLocalCopy()\n");

#endif /// DBG

   return;
}

/**
 @kernel_helper_function="KrnlHlprFwpsStreamDataDestroyLocalCopy"
 
   Purpose:  Cleanup and free a local copy of an FWPS_STREAM_DATA.                              <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF552419.aspx             <br>
*/
_At_(*ppStreamData, _Pre_ _Notnull_)
_At_(*ppStreamData, _Post_ _Null_ __drv_freesMem(Pool))
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Success_(*ppStreamData == 0)
inline VOID KrnlHlprFwpsStreamDataDestroyLocalCopy(_Inout_ FWPS_STREAM_DATA** ppStreamData)
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprFwpsStreamDataDestroyLocalCopy()\n");

#endif /// DBG
   
   NT_ASSERT(ppStreamData);

   if(*ppStreamData)
      KrnlHlprFwpsStreamDataPurgeLocalCopy(*ppStreamData);

   HLPR_DELETE(*ppStreamData,
               WFPSAMPLER_SYSLIB_TAG);

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprFwpsStreamDataDestroyLocalCopy()\n");

#endif /// DBG

   return;
}

/**
 @kernel_helper_function="KrnlHlprFwpsStreamDataPopulateLocalCopy
 
   Purpose:  Populate a local copy of the FWPS_STREAM_DATA.                                     <br>
                                                                                                <br>
   Notes:    The caller is responsible for freeing the allocated memory using 
             KrnlHlprFwpsStreamDataPurgeLocalCopy().                                            <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF552419.aspx             <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Check_return_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprFwpsStreamDataPopulateLocalCopy(_In_ const FWPS_STREAM_DATA* pOriginalStreamData,
                                                 _Inout_ FWPS_STREAM_DATA* pStreamData)
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprFwpsStreamDataPopulateLocalCopy()\n");

#endif /// DBG
   
   NT_ASSERT(pOriginalStreamData);
   NT_ASSERT(pStreamData);

   NTSTATUS status = STATUS_SUCCESS;

   for(NET_BUFFER_LIST* pNBL = pOriginalStreamData->netBufferListChain;
       pNBL;
       pNBL = NET_BUFFER_LIST_NEXT_NBL(pNBL))
   {
      FwpsReferenceNetBufferList(pNBL,
                                 TRUE);
   }

   pStreamData->flags              = pOriginalStreamData->flags;
   pStreamData->dataOffset         = pOriginalStreamData->dataOffset;
   pStreamData->dataLength         = pOriginalStreamData->dataLength;
   pStreamData->netBufferListChain = pOriginalStreamData->netBufferListChain;

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprFwpsStreamDataPopulateLocalCopy() [status: %#x]\n",
              status);

#endif /// DBG

   return status;
}

/**
 @kernel_helper_function="KrnlHlprFwpsStreamDataCreateLocalCopy
 
   Purpose:  Allocate and populate a local copy of the FWPS_STREAM_DATA.                        <br>
                                                                                                <br>
   Notes:    The caller is responsible for freeing the allocated memory using 
             KrnlHlprFwpsStreamDataDestroyLocalCopy().                                          <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF552419.aspx             <br>
*/
__drv_allocatesMem(Pool)
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Must_inspect_result_
_Success_(return != 0)
FWPS_STREAM_DATA* KrnlHlprFwpsStreamDataCreateLocalCopy(_In_ const FWPS_STREAM_DATA* pOriginalStreamData)
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprFwpsStreamDataCreateLocalCopy()\n");

#endif /// DBG
   
   NT_ASSERT(pOriginalStreamData);

   NTSTATUS          status      = STATUS_SUCCESS;
   FWPS_STREAM_DATA* pStreamData = 0;

#pragma warning(push)
#pragma warning(disable: 6014) /// pStreamData is expected to be cleaned up by caller using KrnlHlprFwpsStreamDataDestroyLocalCopy

   HLPR_NEW(pStreamData,
            FWPS_STREAM_DATA,
            WFPSAMPLER_SYSLIB_TAG);
   HLPR_BAIL_ON_ALLOC_FAILURE(pStreamData,
                              status);

#pragma warning(pop)

   status = KrnlHlprFwpsStreamDataPopulateLocalCopy(pOriginalStreamData,
                                                    pStreamData);

   HLPR_BAIL_LABEL:

#pragma warning(push)
#pragma warning(disable: 6001) /// pStreamData initialized with call to HLPR_NEW & KrnlHlprFwpsStreamDataPopulateLocalCopy 

   if(status != STATUS_SUCCESS &&
      pStreamData)
      KrnlHlprFwpsStreamDataDestroyLocalCopy(&pStreamData);

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprFwpsStreamDataCreateLocalCopy() [pStreamData: %#p]\n",
              pStreamData);

#endif /// DBG

   return pStreamData;

#pragma warning(pop)
}

#endif /// FWPS_STREAM_DATA____

#ifndef FWPS_STREAM_CALLOUT_IO_PACKET____
#define FWPS_STREAM_CALLOUT_IO_PACKET____

/**
 @kernel_helper_function="KrnlHlprFwpsStreamCalloutIOPacketPurgeLocalCopy"
 
   Purpose:  Cleanup a local copy of an FWPS_STREAM_CALLOUT_IO_PACKET.                          <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF552417.aspx             <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
inline VOID KrnlHlprFwpsStreamCalloutIOPacketPurgeLocalCopy(_Inout_ FWPS_STREAM_CALLOUT_IO_PACKET* pIOPacket)
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprFwpsStreamCalloutIOPacketPurgeLocalCopy()\n");

#endif /// DBG
   
   NT_ASSERT(pIOPacket);

   KrnlHlprFwpsStreamDataDestroyLocalCopy(&(pIOPacket->streamData));

   RtlZeroMemory(pIOPacket,
                 sizeof(FWPS_STREAM_CALLOUT_IO_PACKET));

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprFwpsStreamCalloutIOPacketPurgeLocalCopy()\n");

#endif /// DBG

   return;
}

/**
 @kernel_helper_function="KrnlHlprFwpsStreamCalloutIOPacketDestroyLocalCopy"
 
   Purpose:  Cleanup and free a local copy of an FWPS_STREAM_CALLOUT_IO_PACKET.                 <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF552417.aspx             <br>
*/
_At_(*ppIOPacket, _Pre_ _Notnull_)
_At_(*ppIOPacket, _Post_ _Null_ __drv_freesMem(Pool))
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Success_(*ppIOPacket == 0)
inline VOID KrnlHlprFwpsStreamCalloutIOPacketDestroyLocalCopy(_Inout_ FWPS_STREAM_CALLOUT_IO_PACKET** ppIOPacket)
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprFwpsStreamCalloutIOPacketDestroyLocalCopy()\n");

#endif /// DBG
   
   NT_ASSERT(ppIOPacket);

   if(*ppIOPacket)
      KrnlHlprFwpsStreamCalloutIOPacketPurgeLocalCopy(*ppIOPacket);

   HLPR_DELETE(*ppIOPacket,
               WFPSAMPLER_SYSLIB_TAG);

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprFwpsStreamCalloutIOPacketDestroyLocalCopy()\n");

#endif /// DBG

   return;
}

/**
 @kernel_helper_function="KrnlHlprFwpsStreamCalloutIOPacketPopulateLocalCopy
 
   Purpose:  Populate a local copy of the FWPS_STREAM_CALLOUT_IO_PACKET.                        <br>
                                                                                                <br>
   Notes:    The caller is responsible for freeing the allocated memory using
             KrnlHlprFwpsStreamCalloutIOPacketPurgeLocalCopy().                                 <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF552417.aspx             <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Check_return_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprFwpsStreamCalloutIOPacketPopulateLocalCopy(_In_ const FWPS_STREAM_CALLOUT_IO_PACKET* pOriginalIOPacket,
                                                            _Inout_ FWPS_STREAM_CALLOUT_IO_PACKET* pIOPacket)
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprFwpsStreamCalloutIOPacketPopulateLocalCopy()\n");

#endif /// DBG
   
   NT_ASSERT(pOriginalIOPacket);
   NT_ASSERT(pIOPacket);

   NTSTATUS status = STATUS_SUCCESS;

   pIOPacket->streamData = KrnlHlprFwpsStreamDataCreateLocalCopy(pOriginalIOPacket->streamData);
   HLPR_BAIL_ON_NULL_POINTER_WITH_STATUS(pIOPacket->streamData,
                                         status);

   pIOPacket->missedBytes        = pOriginalIOPacket->missedBytes;
   pIOPacket->countBytesRequired = pOriginalIOPacket->countBytesRequired;
   pIOPacket->countBytesEnforced = pOriginalIOPacket->countBytesEnforced;
   pIOPacket->streamAction       = pOriginalIOPacket->streamAction;

   HLPR_BAIL_LABEL:

   if(status != STATUS_SUCCESS)
      KrnlHlprFwpsStreamCalloutIOPacketPurgeLocalCopy(pIOPacket);

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprFwpsStreamCalloutIOPacketPopulateLocalCopy() [status: %#x]\n",
              status);

#endif /// DBG

   return status;
}

/**
 @kernel_helper_function="KrnlHlprFwpsStreamCalloutIOPacketCreateLocalCopy
 
   Purpose:  Allocate and populate a local copy of the FWPS_STREAM_CALLOUT_IO_PACKET.           <br>
                                                                                                <br>
   Notes:    The caller is responsible for freeing the allocated memory using
             KrnlHlprFwpsStreamCalloutIOPacketDestroyLocalCopy().                               <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF552417.aspx             <br>
*/
__drv_allocatesMem(Pool)
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Must_inspect_result_
_Success_(return != 0)
FWPS_STREAM_CALLOUT_IO_PACKET* KrnlHlprFwpsStreamCalloutIOPacketCreateLocalCopy(_In_ const FWPS_STREAM_CALLOUT_IO_PACKET* pOriginalIOPacket)
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprFwpsStreamCalloutIOPacketCreateLocalCopy()\n");

#endif /// DBG
   
   NT_ASSERT(pOriginalIOPacket);

   NTSTATUS                       status    = STATUS_SUCCESS;
   FWPS_STREAM_CALLOUT_IO_PACKET* pIOPacket = 0;

#pragma warning(push)
#pragma warning(disable: 6014) /// pIOPacket is expected to be cleaned up by caller using KrnlHlprFwpsStreamCalloutIOPacketDestroyLocalCopy

   HLPR_NEW(pIOPacket,
            FWPS_STREAM_CALLOUT_IO_PACKET,
            WFPSAMPLER_SYSLIB_TAG);
   HLPR_BAIL_ON_ALLOC_FAILURE(pIOPacket,
                              status);

#pragma warning(pop)

   status = KrnlHlprFwpsStreamCalloutIOPacketPopulateLocalCopy(pOriginalIOPacket,
                                                               pIOPacket);

   HLPR_BAIL_LABEL:

#pragma warning(push)
#pragma warning(disable: 6001) /// pIOPacket initialized with call to HLPR_NEW & KrnlHlprFwpsStreamCalloutIOPacketPopulateLocalCopy 

   if(status != STATUS_SUCCESS)
      KrnlHlprFwpsStreamCalloutIOPacketDestroyLocalCopy(&pIOPacket);

#pragma warning(push)

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprFwpsStreamCalloutIOPacketCreateLocalCopy() [pIOPacket: %#p]\n",
              pIOPacket);

#endif /// DBG

   return pIOPacket;
}

#endif /// FWPS_STREAM_CALLOUT_IO_PACKET____

#ifndef FWPM_PROVIDER_CONTEXT____
#define FWPM_PROVIDER_CONTEXT____

/**
 @kernel_helper_function="KrnlHlprFwpmProviderContextPurgeLocalCopy"
 
   Purpose:  Cleanup a local copy of an FWPM_PROVIDER_CONTEXT.                                  <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/HH447383.aspx              <br>
             HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/DD744952.aspx              <br>
             HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/AA364289.aspx              <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
inline VOID KrnlHlprFwpmProviderContextPurgeLocalCopy(_Inout_ FWPM_PROVIDER_CONTEXT* pContext)
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprFwpmProviderContextPurgeLocalCopy()\n");

#endif /// DBG
   
   NT_ASSERT(pContext);

   switch(pContext->type)
   {
      case FWPM_IPSEC_KEYING_CONTEXT:
      {
         if(pContext->keyingPolicy)
         {
            HLPR_DELETE_ARRAY(pContext->keyingPolicy->keyModKeys,
                              WFPSAMPLER_SYSLIB_TAG);
         }

         HLPR_DELETE(pContext->keyingPolicy,
                     WFPSAMPLER_SYSLIB_TAG);

         break;
      }
      case FWPM_IPSEC_IKE_QM_TRANSPORT_CONTEXT:
      {
         break;
      }
      case FWPM_IPSEC_IKE_QM_TUNNEL_CONTEXT:
      {
         break;
      }
      case FWPM_IPSEC_AUTHIP_QM_TRANSPORT_CONTEXT:
      {
         break;
      }
      case FWPM_IPSEC_AUTHIP_QM_TUNNEL_CONTEXT:
      {
         break;
      }
      case FWPM_IPSEC_IKE_MM_CONTEXT:
      {
         break;
      }
      case FWPM_IPSEC_AUTHIP_MM_CONTEXT:
      {
         break;
      }
      case FWPM_CLASSIFY_OPTIONS_CONTEXT:
      {
         KrnlHlprFwpmClassifyOptionsDestroyLocalCopy(&(pContext->classifyOptions));

         break;
      }
      case FWPM_GENERAL_CONTEXT:
      {
         KrnlHlprFwpByteBlobDestroyLocalCopy(&(pContext->dataBuffer));

         break;
      }

#if(NTDDI_VERSION >= NTDDI_WIN7)

      case FWPM_IPSEC_IKEV2_QM_TUNNEL_CONTEXT:
      {
         break;
      }
      case FWPM_IPSEC_IKEV2_MM_CONTEXT:
      {
         break;
      }
      case FWPM_IPSEC_DOSP_CONTEXT:
      {
         KrnlHlprIPsecDoSPOptionsDestroyLocalCopy(&(pContext->idpOptions));

         break;
      }

#endif /// (NTDDI_VERSION >= NTDDI_WIN7)

   }

   KrnlHlprFwpByteBlobPurgeLocalCopy(&(pContext->providerData));

   HLPR_DELETE(pContext->providerKey,
               WFPSAMPLER_SYSLIB_TAG);

   KrnlHlprFwpmDisplayDataPurgeLocalCopy(&(pContext->displayData));

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprFwpmProviderContextPurgeLocalCopy()\n");

#endif /// DBG

   return;
}

/**
 @kernel_helper_function="KrnlHlprFwpmProviderContextDestroyLocalCopy"
 
   Purpose:  Cleanup and free a local copy of an FWPM_PROVIDER_CONTEXT.                         <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/HH447383.aspx              <br>
             HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/DD744952.aspx              <br>
             HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/AA364289.aspx              <br>
*/
_At_(*ppContext, _Pre_ _Notnull_)
_At_(*ppContext, _Post_ _Null_ __drv_freesMem(Pool))
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Success_(*ppContext == 0)
inline VOID KrnlHlprFwpmProviderContextDestroyLocalCopy(_Inout_ FWPM_PROVIDER_CONTEXT** ppContext)
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprFwpmProviderContextDestroyLocalCopy()\n");

#endif /// DBG
   
   NT_ASSERT(ppContext);

   if(*ppContext)
      KrnlHlprFwpmProviderContextPurgeLocalCopy(*ppContext);

   HLPR_DELETE(*ppContext,
               WFPSAMPLER_SYSLIB_TAG);

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprFwpmProviderContextDestroyLocalCopy()\n");

#endif /// DBG

   return;
}

/**
 @kernel_helper_function="KrnlHlprFwpmProviderContextPopulateLocalCopy
 
   Purpose:  Populate a local copy of the FWPM_PROVIDER_CONTEXT.                                <br>
                                                                                                <br>
   Notes:    The caller is responsible for freeing the allocated memory using
             KrnlHlprFwpmProviderContextPurgeLocalCopy().                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/HH447383.aspx              <br>
             HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/DD744952.aspx              <br>
             HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/AA364289.aspx              <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Check_return_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprFwpmProviderContextPopulateLocalCopy(_In_ const FWPM_PROVIDER_CONTEXT* pOriginalContext,
                                                      _Inout_ FWPM_PROVIDER_CONTEXT* pProviderContext)
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprFwpmProviderContextPopulateLocalCopy()\n");

#endif /// DBG
   
   NT_ASSERT(pOriginalContext);
   NT_ASSERT(pProviderContext);

   NTSTATUS status = STATUS_SUCCESS;

   pProviderContext->providerContextKey = pOriginalContext->providerContextKey;
   pProviderContext->flags              = pOriginalContext->flags;
   pProviderContext->type               = pOriginalContext->type;
   pProviderContext->providerContextId  = pOriginalContext->providerContextId;

   switch(pProviderContext->type)
   {
      case FWPM_IPSEC_KEYING_CONTEXT:
      {
         if(pOriginalContext->keyingPolicy)
         {
            HLPR_NEW(pProviderContext->keyingPolicy,
                     IPSEC_KEYING_POLICY,
                     WFPSAMPLER_SYSLIB_TAG);
            HLPR_BAIL_ON_ALLOC_FAILURE(pProviderContext->keyingPolicy,
                                       status);

            if(pOriginalContext->keyingPolicy->numKeyMods)
            {
               pProviderContext->keyingPolicy->numKeyMods = pOriginalContext->keyingPolicy->numKeyMods;

               HLPR_NEW_ARRAY(pProviderContext->keyingPolicy->keyModKeys,
                              GUID,
                              pProviderContext->keyingPolicy->numKeyMods,
                              WFPSAMPLER_SYSLIB_TAG);
               HLPR_BAIL_ON_ALLOC_FAILURE(pProviderContext->keyingPolicy->keyModKeys,
                                          status);

               RtlCopyMemory(pProviderContext->keyingPolicy->keyModKeys,
                             pOriginalContext->keyingPolicy->keyModKeys,
                             pProviderContext->keyingPolicy->numKeyMods * sizeof(GUID));
            }
         }

         break;
      }
      case FWPM_IPSEC_IKE_QM_TRANSPORT_CONTEXT:
      {
         break;
      }
      case FWPM_IPSEC_IKE_QM_TUNNEL_CONTEXT:
      {
         break;
      }
      case FWPM_IPSEC_AUTHIP_QM_TRANSPORT_CONTEXT:
      {
         break;
      }
      case FWPM_IPSEC_AUTHIP_QM_TUNNEL_CONTEXT:
      {
         break;
      }
      case FWPM_IPSEC_IKE_MM_CONTEXT:
      {
         break;
      }
      case FWPM_IPSEC_AUTHIP_MM_CONTEXT:
      {
         break;
      }
      case FWPM_CLASSIFY_OPTIONS_CONTEXT:
      {
         break;
      }
      case FWPM_GENERAL_CONTEXT:
      {
         pProviderContext->dataBuffer = KrnlHlprFwpByteBlobCreateLocalCopy(pOriginalContext->dataBuffer);
         HLPR_BAIL_ON_NULL_POINTER_WITH_STATUS(pProviderContext->dataBuffer,
                                               status);

         break;
      }

#if(NTDDI_VERSION >= NTDDI_WIN7)

      case FWPM_IPSEC_IKEV2_QM_TUNNEL_CONTEXT:
      {
         break;
      }
      case FWPM_IPSEC_IKEV2_MM_CONTEXT:
      {
         break;
      }
      case FWPM_IPSEC_DOSP_CONTEXT:
      {
         pProviderContext->idpOptions = KrnlHlprIPsecDoSPOptionsCreateLocalCopy(pOriginalContext->idpOptions);
         HLPR_BAIL_ON_NULL_POINTER_WITH_STATUS(pProviderContext->idpOptions,
                                               status);
         break;
      }

#endif /// (NTDDI_VERSION >= NTDDI_WIN7)

   }

   HLPR_BAIL_LABEL:

   if(status !=  STATUS_SUCCESS)
      KrnlHlprFwpmProviderContextPurgeLocalCopy(pProviderContext);

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprFwpmProviderContextPopulateLocalCopy() [status: %#x]\n",
              status);

#endif /// DBG

   return status;
}

/**
 @kernel_helper_function="KrnlHlprFwpmProviderContextCreateLocalCopy
 
   Purpose:  Allocate and populate a local copy of the FWPM_PROVIDER_CONTEXT.                   <br>
                                                                                                <br>
   Notes:    The caller is responsible for freeing the allocated memory using
             KrnlHlprFwpmProviderContextDestroyLocalCopy().                                     <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/HH447383.aspx              <br>
             HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/DD744952.aspx              <br>
             HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/AA364289.aspx              <br>
*/
__drv_allocatesMem(Pool)
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Must_inspect_result_
_Success_(return != 0)
FWPM_PROVIDER_CONTEXT* KrnlHlprFwpmProviderContextCreateLocalCopy(_In_ const FWPM_PROVIDER_CONTEXT* pOriginalContext)
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprFwpmProviderContextCreateLocalCopy()\n");

#endif /// DBG
   
   NT_ASSERT(pOriginalContext);

   NTSTATUS               status           = STATUS_SUCCESS;
   FWPM_PROVIDER_CONTEXT* pProviderContext = 0;

#pragma warning(push)
#pragma warning(disable: 6014) /// pProviderContext is expected to be cleaned up by caller using KrnlHlprFwpmProviderContextDestroyLocalCopy

   HLPR_NEW(pProviderContext,
            FWPM_PROVIDER_CONTEXT,
            WFPSAMPLER_SYSLIB_TAG);
   HLPR_BAIL_ON_ALLOC_FAILURE(pProviderContext,
                              status);

#pragma warning(pop)

   status = KrnlHlprFwpmProviderContextPopulateLocalCopy(pOriginalContext,
                                                         pProviderContext);

   HLPR_BAIL_LABEL:

#pragma warning(push)
#pragma warning(disable: 6001) /// pProviderContext initialized with call to HLPR_NEW & KrnlHlprFwpmProviderContextPopulateLocalCopy 

   if(status !=  STATUS_SUCCESS)
      KrnlHlprFwpmProviderContextDestroyLocalCopy(&pProviderContext);

#pragma warning(pop)

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprFwpmProviderContextCreateLocalCopy() [pProviderContext: %#p]\n",
              pProviderContext);

#endif /// DBG

   return pProviderContext;
}

#endif /// FWPS_PROVIDER_CONTEXT____

#ifndef FWPS_FILTER_CONDITION____
#define FWPS_FILTER_CONDITION____

/**
 @kernel_helper_function="KrnlHlprFwpsFilterConditionPurgeLocalCopy"
 
   Purpose:  Cleanup a local copy of an FWPS_FILTER_CONDITION.                                  <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF552391.aspx             <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
inline VOID KrnlHlprFwpsFilterConditionPurgeLocalCopy(_Inout_ FWPS_FILTER_CONDITION* pCondition)
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprFwpsFilterConditionPurgeLocalCopy()\n");

#endif /// DBG
   
   NT_ASSERT(pCondition);

   KrnlHlprFwpConditionValuePurgeLocalCopy(&(pCondition->conditionValue));

   RtlZeroMemory(pCondition,
                 sizeof(FWPS_FILTER_CONDITION));

   pCondition->matchType = FWP_MATCH_TYPE_MAX;

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprFwpsFilterConditionPurgeLocalCopy()\n");

#endif /// DBG

   return;
}

/**
 @kernel_helper_function="KrnlHlprFwpsFilterConditionDestroyLocalCopy"
 
   Purpose:  Cleanup and free a local copy of an FWPS_FILTER_CONDITION.                         <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF552391.aspx             <br>
*/
_At_(*ppConditions, _Pre_ _Notnull_)
_At_(*ppConditions, _Post_ _Null_ __drv_freesMem(Pool))
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Success_(*ppConditions == 0)
inline VOID KrnlHlprFwpsFilterConditionDestroyLocalCopy(_Inout_ FWPS_FILTER_CONDITION** ppConditions,
                                                        _In_ UINT32 numConditions)                    /* 1 */
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprFwpsFilterConditionDestroyLocalCopy()\n");

#endif /// DBG
   
   NT_ASSERT(ppConditions);

   if(*ppConditions)
   {
      for(UINT32 conditionIndex = 0;
          conditionIndex < numConditions;
          conditionIndex++)
      {
         KrnlHlprFwpsFilterConditionPurgeLocalCopy(&((*ppConditions)[conditionIndex]));
      }
   }

   HLPR_DELETE_ARRAY(*ppConditions,
                     WFPSAMPLER_SYSLIB_TAG);

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprFwpsFilterConditionDestroyLocalCopy()\n");

#endif /// DBG

   return;
}

/**
 @kernel_helper_function="KrnlHlprFwpsFilterConditionPopulateLocalCopy"
 
   Purpose:  Populate a local copy of an FWPS_FILTER_CONDITION.                                 <br>
                                                                                                <br>
   Notes:    The caller is responsible for freeing any allocated memory using
             KrnlHlprFwpsFilterConditionPurgeLocalCopy().                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF552391.aspx             <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Check_return_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprFwpsFilterConditionPopulateLocalCopy(_In_ const FWPS_FILTER_CONDITION* pOriginalCondition,
                                                      _Inout_ FWPS_FILTER_CONDITION* pCondition)
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprFwpsFilterConditionPopulateLocalCopy()\n");

#endif /// DBG
   
   NT_ASSERT(pOriginalCondition);
   NT_ASSERT(pCondition);

   NTSTATUS status = STATUS_SUCCESS;

   pCondition->fieldId   = pOriginalCondition->fieldId;
   pCondition->reserved  = pOriginalCondition->reserved;
   pCondition->matchType = pOriginalCondition->matchType;

   status = KrnlHlprFwpConditionValuePopulateLocalCopy(&(pOriginalCondition->conditionValue),
                                                       &(pCondition->conditionValue));

   if(status != STATUS_SUCCESS)
      KrnlHlprFwpsFilterConditionPurgeLocalCopy(pCondition);

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprFwpsFilterConditionPopulateLocalCopy() [status: %#x]\n",
              status);

#endif /// DBG

   return status;
}

/**
 @kernel_helper_function="KrnlHlprFwpsFilterConditionCreateLocalCopy"
 
   Purpose:  Allocate and populate a local copy of an FWPS_FILTER_CONDITION.                    <br>
                                                                                                <br>
   Notes:    The caller is responsible for freeing any allocated memory using
             KrnlHlprFwpsFilterConditionDestroyLocalCopy().                                     <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF552391.aspx             <br>
*/
__drv_allocatesMem(Pool)
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Must_inspect_result_
_Success_(return != 0)
FWPS_FILTER_CONDITION* KrnlHlprFwpsFilterConditionCreateLocalCopy(_In_reads_(numConditions) const FWPS_FILTER_CONDITION* pOriginalConditions,
                                                                  _In_ UINT32 numConditions)
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprFwpsFilterConditionCreateLocalCopy()\n");

#endif /// DBG
   
   NT_ASSERT(pOriginalConditions);
   NT_ASSERT(numConditions);

   NTSTATUS               status      = STATUS_SUCCESS;
   FWPS_FILTER_CONDITION* pConditions = 0;

#pragma warning(push)
#pragma warning(disable: 6014) /// pConditions is expected to be cleaned up by caller using KrnlHlprFwpsFilterConditionDestroyLocalCopy

   HLPR_NEW_ARRAY(pConditions,
                  FWPS_FILTER_CONDITION,
                  numConditions,
                  WFPSAMPLER_SYSLIB_TAG);
   HLPR_BAIL_ON_ALLOC_FAILURE(pConditions,
                              status);

#pragma warning(pop)

   for(UINT32 conditionIndex = 0;
       conditionIndex < numConditions;
       conditionIndex++)
   {
      status = KrnlHlprFwpsFilterConditionPopulateLocalCopy(&(pOriginalConditions[conditionIndex]),
                                                            &(pConditions[conditionIndex]));
   }

   HLPR_BAIL_LABEL:

#pragma warning(push)
#pragma warning(disable: 6001) /// pConditions initialized with call to HLPR_NEW_ARRAY & KrnlHlprFwpsFilterConditionPopulateLocalCopy 

   if(status != STATUS_SUCCESS &&
      pConditions)
      KrnlHlprFwpsFilterConditionDestroyLocalCopy(&pConditions);

#pragma warning(pop)

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprFwpsFilterConditionCreateLocalCopy() [pConditions: %#p]\n",
              pConditions);

#endif /// DBG

   return pConditions;
}

#endif /// FWPS_FILTER_CONDITION____

#ifndef FWPS_FILTER____
#define FWPS_FILTER____

/**
 @kernel_helper_function="KrnlHlprFwpsFilterPurgeLocalCopy"
 
   Purpose:  Cleanup a local copy of an FWPS_FILTER.                                            <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/EE220716.aspx                              <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
inline VOID KrnlHlprFwpsFilterPurgeLocalCopy(_Inout_ FWPS_FILTER* pFilter)
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprFwpsFilterPurgeLocalCopy()\n");

#endif /// DBG
   
   NT_ASSERT(pFilter);

   KrnlHlprFwpmProviderContextDestroyLocalCopy(&(pFilter->providerContext));

   KrnlHlprFwpsFilterConditionDestroyLocalCopy(&(pFilter->filterCondition),
                                               pFilter->numFilterConditions);

   KrnlHlprFwpValuePurgeLocalCopy(&(pFilter->weight));

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprFwpsFilterPurgeLocalCopy()\n");

#endif /// DBG

   return;
}

/**
 @kernel_helper_function="KrnlHlprFwpsFilterDestroyLocalCopy"
 
   Purpose:  Cleanup and free a local copy of an FWPS_FILTER.                                   <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/HH439768.aspx             <br>
             HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF552389.aspx             <br>
             HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF552387.aspx             <br>
*/
_At_(*ppFilter, _Pre_ _Notnull_)
_At_(*ppFilter, _Post_ _Null_ __drv_freesMem(Pool))
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Success_(*ppFilter == 0)
inline VOID KrnlHlprFwpsFilterDestroyLocalCopy(_Inout_ FWPS_FILTER** ppFilter)
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprFwpsFilterDestroyLocalCopy()\n");

#endif /// DBG
   
   NT_ASSERT(ppFilter);

   if(*ppFilter)
      KrnlHlprFwpsFilterPurgeLocalCopy(*ppFilter);

   HLPR_DELETE(*ppFilter,
               WFPSAMPLER_SYSLIB_TAG);

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprFwpsFilterDestroyLocalCopy()\n");

#endif /// DBG

   return;
}

/**
 @kernel_helper_function="KrnlHlprFwpsFilterPopulateLocalCopy
 
   Purpose:  Populate a local copy of the FWPS_FILTER.                                          <br>
                                                                                                <br>
   Notes:    The caller is responsible for freeing the allocated memory using
             KrnlHlprFwpsFilterPurgeLocalCopy().                                                <br>
                                                                                                <br>

   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/HH439768.aspx             <br>
             HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF552389.aspx             <br>
             HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF552387.aspx             <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Check_return_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprFwpsFilterPopulateLocalCopy(_In_ const FWPS_FILTER* pOriginalFilter,
                                             _Inout_ FWPS_FILTER* pFilter)
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprFwpsFilterPopulateLocalCopy()\n");

#endif /// DBG
   
   NT_ASSERT(pOriginalFilter);
   NT_ASSERT(pFilter);

   NTSTATUS status = STATUS_SUCCESS;

   pFilter->filterId            = pOriginalFilter->filterId;
   pFilter->subLayerWeight      = pOriginalFilter->subLayerWeight;
   pFilter->flags               = pOriginalFilter->flags;
   pFilter->numFilterConditions = pOriginalFilter->numFilterConditions;
   pFilter->action.type         = pOriginalFilter->action.type;
   pFilter->action.calloutId    = pOriginalFilter->action.calloutId;
   pFilter->context             = pOriginalFilter->context;

   status = KrnlHlprFwpValuePopulateLocalCopy(&(pOriginalFilter->weight),
                                              &(pFilter->weight));
   HLPR_BAIL_ON_FAILURE(status);

   if(pFilter->numFilterConditions)
   {
      pFilter->filterCondition = KrnlHlprFwpsFilterConditionCreateLocalCopy(pOriginalFilter->filterCondition,
                                                                            pFilter->numFilterConditions);
      HLPR_BAIL_ON_NULL_POINTER_WITH_STATUS(pFilter->filterCondition,
                                            status);
   }

   if(pOriginalFilter->providerContext)
   {
      pFilter->providerContext = KrnlHlprFwpmProviderContextCreateLocalCopy(pOriginalFilter->providerContext);
      HLPR_BAIL_ON_ALLOC_FAILURE(pFilter->providerContext,
                                 status);
   }

   HLPR_BAIL_LABEL:

   if(status != STATUS_SUCCESS)
      KrnlHlprFwpsFilterPurgeLocalCopy(pFilter);

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprFwpsFilterPopulateLocalCopy() [status: %#x]\n",
              status);

#endif /// DBG

   return status;
}

/**
 @kernel_helper_function="KrnlHlprFwpsFilterCreateLocalCopy
 
   Purpose:  Allocate and populate a local copy of the FWPS_FILTER.                             <br>
                                                                                                <br>
   Notes:    The caller is responsible for freeing the allocated memory using
             KrnlHlprFwpsFilterDestroyLocalCopy().                                              <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/HH439768.aspx             <br>
             HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF552389.aspx             <br>
             HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF552387.aspx             <br>
*/
__drv_allocatesMem(Pool)
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Must_inspect_result_
_Success_(return != 0)
FWPS_FILTER* KrnlHlprFwpsFilterCreateLocalCopy(_In_ const FWPS_FILTER* pOriginalFilter)
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprFwpsFilterCreateLocalCopy()\n");

#endif /// DBG
   
   NT_ASSERT(pOriginalFilter);

   NTSTATUS     status  = STATUS_SUCCESS;
   FWPS_FILTER* pFilter = 0;

#pragma warning(push)
#pragma warning(disable: 6014) /// pFilter is expected to be cleaned up by caller using KrnlHlprFwpsFilterDestroyLocalCopy

   HLPR_NEW(pFilter,
            FWPS_FILTER,
            WFPSAMPLER_SYSLIB_TAG);
   HLPR_BAIL_ON_ALLOC_FAILURE(pFilter,
                              status);

#pragma warning(pop)

   status = KrnlHlprFwpsFilterPopulateLocalCopy(pOriginalFilter,
                                                pFilter);

   HLPR_BAIL_LABEL:

#pragma warning(push)
#pragma warning(disable: 6001) /// pFilter initialized with call to HLPR_NEW & KrnlHlprFwpsFilterPopulateLocalCopy 

   if(status != STATUS_SUCCESS)
      KrnlHlprFwpsFilterDestroyLocalCopy(&pFilter);

#pragma warning(pop)

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprFwpsFilterCreateLocalCopy() [pFilter: %p]\n",
              pFilter);

#endif /// DBG

   return pFilter;
}

#endif /// FWPS_FILTER____

#ifndef FWPM_SESSION____
#define FWPM_SESSION____

/**
 @kernel_helper_function="KrnlHlprFwpmSessionReleaseHandle
 
   Purpose:  Close an open handle to BFE.                                                       <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF550072.aspx             <br>
*/
_IRQL_requires_(PASSIVE_LEVEL)
_IRQL_requires_same_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprFwpmSessionReleaseHandle(_Inout_ HANDLE* pEngineHandle)
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprFwpmSessionReleaseHandle()\n");

#endif /// DBG
   
   NT_ASSERT(pEngineHandle);

   NTSTATUS status = STATUS_SUCCESS;

   if(*pEngineHandle)
   {
      status = FwpmEngineClose(*pEngineHandle);
      if(status != STATUS_SUCCESS)
      {
         DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                    DPFLTR_ERROR_LEVEL,
                    " !!!! KrnlHlprFwpmSessionReleaseHandle : FwpmEngineClose() [status: %#x]\n",
                    status);

         HLPR_BAIL;
      }

      *pEngineHandle = 0;
   }

   HLPR_BAIL_LABEL:

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprFwpmSessionReleaseHandle() [status: %#x]\n",
              status);

#endif /// DBG

   return status;
}

/**
 @kernel_helper_function="KrnlHlprFwpmSessionDestroyEngineHandle
 
   Purpose:  Close an open handle to BFE and set the handle to 0.                               <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_At_(*ppEngineHandle, _Pre_ _Notnull_)
_At_(*ppEngineHandle, _Post_ _Null_)
_IRQL_requires_(PASSIVE_LEVEL)
_IRQL_requires_same_
_Success_(*ppEngineHandle == 0)
VOID KrnlHlprFwpmSessionDestroyEngineHandle(_Inout_ HANDLE** ppEngineHandle)
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprFwpmSessionDestroyEngineHandle()\n");

#endif /// DBG
   
   NT_ASSERT(ppEngineHandle);

   if(*ppEngineHandle)
      KrnlHlprFwpmSessionReleaseHandle(*ppEngineHandle);

   *ppEngineHandle = 0;

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprFwpmSessionDestroyEngineHandle()\n");

#endif /// DBG

   return;
}

/**
 @kernel_helper_function="KrnlHlprFwpmSessionAcquireHandle
 
   Purpose:  Open a handle to BFE.                                                              <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF550083.aspx             <br>
             HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF550075.aspx             <br>
*/
_IRQL_requires_(PASSIVE_LEVEL)
_IRQL_requires_same_
_Check_return_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprFwpmSessionAcquireHandle(_Inout_ HANDLE* pEngineHandle,
                                          _In_ const GUID* pSessionKey)  /* WFPSAMPLER_SESSION_KM */
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprFwpmSessionAcquireHandle()\n");

#endif /// DBG
   
   NT_ASSERT(pEngineHandle);
   NT_ASSERT(pSessionKey);

   NTSTATUS     status  = STATUS_SUCCESS;
   FWPM_SESSION session = {0};

   session.sessionKey              = *pSessionKey;
   session.displayData.name        = L"Microsoft Corporation - WFPSampler's Session";
   session.displayData.description = L"Session created by WFPSampler";

   status = FwpmEngineOpen(0,
                           RPC_C_AUTHN_WINNT,
                           0,
                           &session,
                           pEngineHandle);
   if(status != STATUS_SUCCESS ||
      *pEngineHandle == 0)
   {
      DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                 DPFLTR_ERROR_LEVEL,
                 " !!!! KrnlHlprFwpmSessionAcquireHandle : FwpmEngineOpen() [status: %#x][*pEngineHandle: %#p]\n",
                 status,
                 *pEngineHandle);

      *pEngineHandle = 0;
   }

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprFwpmSessionAcquireHandle() [status: %#x]\n",
              status);

#endif /// DBG

   return status;
}

/**
 @kernel_helper_function="KrnlHlprFwpmSessionCreateEngineHandle
 
   Purpose:  Open a handle to BFE.                                                              <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_When_(return != STATUS_SUCCESS, _At_(*ppEngineHandle, _Post_ _Null_))
_When_(return == STATUS_SUCCESS, _At_(*ppEngineHandle, _Post_ _Notnull_))
_IRQL_requires_(PASSIVE_LEVEL)
_IRQL_requires_same_
_Check_return_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprFwpmSessionCreateEngineHandle(_Outptr_ HANDLE** ppEngineHandle,
                                               _In_ const GUID* pSessionKey)     /* WFPSAMPLER_SESSION_KM */
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprFwpmSessionCreateEngineHandle()\n");

#endif /// DBG
   
   NT_ASSERT(ppEngineHandle);
   NT_ASSERT(pSessionKey);

   NTSTATUS status = STATUS_SUCCESS;

   *ppEngineHandle = 0;

   status = KrnlHlprFwpmSessionAcquireHandle(&g_EngineHandle,
                                             pSessionKey);
   HLPR_BAIL_ON_FAILURE(status);

   *ppEngineHandle = &g_EngineHandle;

   HLPR_BAIL_LABEL:

   if(status != STATUS_SUCCESS)
      KrnlHlprFwpmSessionReleaseHandle(&g_EngineHandle);

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprFwpmSessionCreateEngineHandle() [status: %#x]\n",
              status);

#endif /// DBG

   return status;
}

#endif /// FWPS_SESSION____

#ifndef FwpsInjection____
#define FwpsInjection____

/**
 @kernel_helper_function="KrnlHlprFwpsInjectionReleaseHandle
 
   Purpose:  Destroy an opened injection handle and set it to 0.                                <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF551181.aspx             <br>
*/
_IRQL_requires_(PASSIVE_LEVEL)
_IRQL_requires_same_
_Check_return_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprFwpsInjectionReleaseHandle(_In_ HANDLE* pInjectionHandle)
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprFwpsInjectionReleaseHandle()\n");

#endif /// DBG
   
   NT_ASSERT(pInjectionHandle);

   NTSTATUS status = STATUS_SUCCESS;

   status = FwpsInjectionHandleDestroy(*pInjectionHandle);
   if(status != STATUS_SUCCESS)
   {
      DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                 DPFLTR_ERROR_LEVEL,
                 " !!!! KrnlHlprFwpsInjectionReleaseHandle : FwpsInjectionHandleDestroy() [status: %#x]\n",
                 status);

      HLPR_BAIL;
   }

   *pInjectionHandle = 0;

   HLPR_BAIL_LABEL:

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprFwpsInjectionReleaseHandle() {status: %#x]\n",
              status);

#endif /// DBG

   return status;
}

/**
 @kernel_helper_function="KrnlHlprFwpsInjectionAcquireHandle
 
   Purpose:  Open injection handle for use with the various injection APIs.                     <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF551180.aspx             <br>
*/
_IRQL_requires_(PASSIVE_LEVEL)
_IRQL_requires_same_
_Check_return_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprFwpsInjectionAcquireHandle(_Inout_ HANDLE* pInjectionHandle,
                                            _In_ ADDRESS_FAMILY addressFamily, /* AF_UNSPEC */
                                            _In_ UINT32 injectionType)         /* 0 */
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprFwpsInjectionAcquireHandle()\n");

#endif /// DBG
   
   NT_ASSERT(pInjectionHandle);

   NTSTATUS status = STATUS_SUCCESS;

   status = FwpsInjectionHandleCreate(addressFamily,
                                      injectionType,
                                      pInjectionHandle);
   if(status != STATUS_SUCCESS)
      DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                 DPFLTR_ERROR_LEVEL,
                 " !!!! KrnlHlprFwpsInjectionAcquireHandle : FwpsInjectionHandleCreate() [status: %#x]\n",
                 status);

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprFwpsInjectionAcquireHandle() [status: %#x]\n",
              status);

#endif /// DBG

   return status;
}

#endif /// FwpsInjection____

#if(NTDDI_VERSION >= NTDDI_WIN8)

#ifndef FwpsRedirection____
#define FwpsRedirection____

/**
 @kernel_helper_function="KrnlHlprFwpsRedirectHandleDestroy
 
   Purpose:  Destroy an opened redirection handle and set it to 0.                              <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/HH439684.aspx             <br>
*/
_IRQL_requires_(PASSIVE_LEVEL)
_IRQL_requires_same_
VOID KrnlHlprFwpsRedirectHandleDestroy(_In_ HANDLE* pRedirectionHandle)
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprFwpsRedirectHandleDestroy()\n");

#endif /// DBG
   
   NT_ASSERT(pRedirectionHandle);

   FwpsRedirectHandleDestroy(*pRedirectionHandle);

   *pRedirectionHandle = 0;

#if DBG

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprFwpsRedirectHandleDestroy()\n");

#endif /// DBG

   return;
}

/**
 @kernel_helper_function="KrnlHlprFwpsRedirectHandleCreate
 
   Purpose:  Open redirection handle for use with the various redirection APIs.                 <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/HH439681.aspx             <br>
*/
_IRQL_requires_(PASSIVE_LEVEL)
_IRQL_requires_same_
_Check_return_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprFwpsRedirectHandleCreate(_Inout_ HANDLE* pRedirectionHandle) /* 0 */
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprFwpsRedirectHandleCreate()\n");

#endif /// DBG
   
   NT_ASSERT(pRedirectionHandle);

   NTSTATUS status = STATUS_SUCCESS;

   status = FwpsRedirectHandleCreate(&WFPSAMPLER_PROVIDER,
                                     0,
                                     pRedirectionHandle);
   if(status != STATUS_SUCCESS)
      DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                 DPFLTR_ERROR_LEVEL,
                 " !!!! KrnlHlprFwpsRedirectHandleCreate : FwpsRedirectHandleCreate() [status: %#x]\n",
                 status);

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprFwpsRedirectHandleCreate() [status: %#x]\n",
              status);

#endif /// DBG

   return status;
}

#endif /// FwpsRedirection____

#endif /// (NTDDI_VERSION >= NTDDI_WIN8)

#ifndef FwpsLayer____
#define FwpsLayer____

/**
 @helper function="KrnlHlprFwpsLayerGetDirection"
 
   Purpose: Determine the direction of the NBL based on the layer.                              <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
FWP_DIRECTION KrnlHlprFwpsLayerGetDirection(_In_ UINT32 layerID)
{
   FWP_DIRECTION direction = FWP_DIRECTION_MAX;

   if(layerID == FWPS_LAYER_INBOUND_IPPACKET_V4 ||
      layerID == FWPS_LAYER_INBOUND_IPPACKET_V4_DISCARD ||
      layerID == FWPS_LAYER_INBOUND_IPPACKET_V6 ||
      layerID == FWPS_LAYER_INBOUND_IPPACKET_V6_DISCARD ||
      layerID == FWPS_LAYER_INBOUND_TRANSPORT_V4 ||
      layerID == FWPS_LAYER_INBOUND_TRANSPORT_V4_DISCARD ||
      layerID == FWPS_LAYER_INBOUND_TRANSPORT_V6 ||
      layerID == FWPS_LAYER_INBOUND_TRANSPORT_V6_DISCARD ||
      layerID == FWPS_LAYER_INBOUND_ICMP_ERROR_V4 ||
      layerID == FWPS_LAYER_INBOUND_ICMP_ERROR_V4_DISCARD ||
      layerID == FWPS_LAYER_INBOUND_ICMP_ERROR_V6 ||
      layerID == FWPS_LAYER_INBOUND_ICMP_ERROR_V6_DISCARD ||
      layerID == FWPS_LAYER_ALE_AUTH_RECV_ACCEPT_V4 ||
      layerID == FWPS_LAYER_ALE_AUTH_RECV_ACCEPT_V4_DISCARD ||
      layerID == FWPS_LAYER_ALE_AUTH_RECV_ACCEPT_V6 ||
      layerID == FWPS_LAYER_ALE_AUTH_RECV_ACCEPT_V6_DISCARD

#if(NTDDI_VERSION >= NTDDI_WIN8)

      ||
      layerID == FWPS_LAYER_INBOUND_MAC_FRAME_ETHERNET ||
      layerID == FWPS_LAYER_INBOUND_MAC_FRAME_NATIVE

#endif /// (NTDDI_VERSION >= NTDDI_WIN8)

      )
      direction = FWP_DIRECTION_INBOUND;
   else if(layerID == FWPS_LAYER_OUTBOUND_IPPACKET_V4 ||
           layerID == FWPS_LAYER_OUTBOUND_IPPACKET_V4_DISCARD ||
           layerID == FWPS_LAYER_OUTBOUND_IPPACKET_V6 ||
           layerID == FWPS_LAYER_OUTBOUND_IPPACKET_V6_DISCARD ||
           layerID == FWPS_LAYER_OUTBOUND_TRANSPORT_V4 ||
           layerID == FWPS_LAYER_OUTBOUND_TRANSPORT_V4_DISCARD ||
           layerID == FWPS_LAYER_OUTBOUND_TRANSPORT_V6 ||
           layerID == FWPS_LAYER_OUTBOUND_TRANSPORT_V6_DISCARD ||
           layerID == FWPS_LAYER_OUTBOUND_ICMP_ERROR_V4 ||
           layerID == FWPS_LAYER_OUTBOUND_ICMP_ERROR_V4_DISCARD ||
           layerID == FWPS_LAYER_OUTBOUND_ICMP_ERROR_V6 ||
           layerID == FWPS_LAYER_OUTBOUND_ICMP_ERROR_V6_DISCARD ||
           layerID == FWPS_LAYER_ALE_AUTH_CONNECT_V4 ||
           layerID == FWPS_LAYER_ALE_AUTH_CONNECT_V4_DISCARD ||
           layerID == FWPS_LAYER_ALE_AUTH_CONNECT_V6 ||
           layerID == FWPS_LAYER_ALE_AUTH_CONNECT_V6_DISCARD

#if(NTDDI_VERSION >= NTDDI_WIN8)

           ||
           layerID == FWPS_LAYER_OUTBOUND_MAC_FRAME_ETHERNET ||
           layerID == FWPS_LAYER_OUTBOUND_MAC_FRAME_NATIVE

#endif /// (NTDDI_VERSION >= NTDDI_WIN8)

           )
      direction = FWP_DIRECTION_OUTBOUND;

   return direction;
}

/**
 @helper function="KrnlHlprFwpsLayerIsIPv4"
 
   Purpose: Determine if the layer is an IPv4 layer.                                            <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
BOOLEAN KrnlHlprFwpsLayerIsIPv4(_In_ UINT32 layerID)
{
   BOOLEAN isIPv4 = FALSE;

   if(layerID == FWPS_LAYER_INBOUND_IPPACKET_V4 ||
      layerID == FWPS_LAYER_INBOUND_IPPACKET_V4_DISCARD ||
      layerID == FWPS_LAYER_OUTBOUND_IPPACKET_V4 ||
      layerID == FWPS_LAYER_OUTBOUND_IPPACKET_V4_DISCARD ||
      layerID == FWPS_LAYER_IPFORWARD_V4 ||
      layerID == FWPS_LAYER_IPFORWARD_V4_DISCARD ||
      layerID == FWPS_LAYER_INBOUND_TRANSPORT_V4 ||
      layerID == FWPS_LAYER_INBOUND_TRANSPORT_V4_DISCARD ||
      layerID == FWPS_LAYER_OUTBOUND_TRANSPORT_V4 ||
      layerID == FWPS_LAYER_OUTBOUND_TRANSPORT_V4_DISCARD ||
      layerID == FWPS_LAYER_STREAM_V4 ||
      layerID == FWPS_LAYER_STREAM_V4_DISCARD ||
      layerID == FWPS_LAYER_DATAGRAM_DATA_V4 ||
      layerID == FWPS_LAYER_DATAGRAM_DATA_V4_DISCARD ||
      layerID == FWPS_LAYER_INBOUND_ICMP_ERROR_V4 ||
      layerID == FWPS_LAYER_INBOUND_ICMP_ERROR_V4_DISCARD ||
      layerID == FWPS_LAYER_OUTBOUND_ICMP_ERROR_V4 ||
      layerID == FWPS_LAYER_OUTBOUND_ICMP_ERROR_V4_DISCARD ||
      layerID == FWPS_LAYER_ALE_RESOURCE_ASSIGNMENT_V4 ||
      layerID == FWPS_LAYER_ALE_RESOURCE_ASSIGNMENT_V4_DISCARD ||
      layerID == FWPS_LAYER_ALE_AUTH_LISTEN_V4 ||
      layerID == FWPS_LAYER_ALE_AUTH_LISTEN_V4_DISCARD ||
      layerID == FWPS_LAYER_ALE_AUTH_CONNECT_V4 ||
      layerID == FWPS_LAYER_ALE_AUTH_CONNECT_V4_DISCARD ||
      layerID == FWPS_LAYER_ALE_AUTH_RECV_ACCEPT_V4 ||
      layerID == FWPS_LAYER_ALE_AUTH_RECV_ACCEPT_V4_DISCARD ||
      layerID == FWPS_LAYER_ALE_FLOW_ESTABLISHED_V4 ||
      layerID == FWPS_LAYER_ALE_FLOW_ESTABLISHED_V4_DISCARD ||

#if(NTDDI_VERSION >= NTDDI_WIN7)

      layerID == FWPS_LAYER_NAME_RESOLUTION_CACHE_V4 ||
      layerID == FWPS_LAYER_ALE_RESOURCE_RELEASE_V4 ||
      layerID == FWPS_LAYER_ALE_ENDPOINT_CLOSURE_V4 ||
      layerID == FWPS_LAYER_ALE_CONNECT_REDIRECT_V4 ||
      layerID == FWPS_LAYER_ALE_BIND_REDIRECT_V4 ||
      layerID == FWPS_LAYER_STREAM_PACKET_V4 ||

#if(NTDDI_VERSION >= NTDDI_WIN8)

      layerID == FWPS_LAYER_INGRESS_VSWITCH_TRANSPORT_V4 ||
      layerID == FWPS_LAYER_EGRESS_VSWITCH_TRANSPORT_V4 ||

#endif /// (NTDDI_VERSION >= NTDDI_WIN8)
#endif /// (NTDDI_VERSION >= NTDDI_WIN7)

      layerID == FWPS_LAYER_IPSEC_KM_DEMUX_V4 ||
      layerID == FWPS_LAYER_IPSEC_V4 ||
      layerID == FWPS_LAYER_IKEEXT_V4)
      isIPv4 = TRUE;

   return isIPv4;
}

/**
 @helper function="KrnlHlprFwpsLayerIsIPv6"
 
   Purpose: Determine if the layer is an IPv6 layer.                                            <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
BOOLEAN KrnlHlprFwpsLayerIsIPv6(_In_ UINT32 layerID)
{
   BOOLEAN isIPv6 = FALSE;

   if(layerID == FWPS_LAYER_INBOUND_IPPACKET_V6 ||
      layerID == FWPS_LAYER_INBOUND_IPPACKET_V6_DISCARD ||
      layerID == FWPS_LAYER_OUTBOUND_IPPACKET_V6 ||
      layerID == FWPS_LAYER_OUTBOUND_IPPACKET_V6_DISCARD ||
      layerID == FWPS_LAYER_IPFORWARD_V6 ||
      layerID == FWPS_LAYER_IPFORWARD_V6_DISCARD ||
      layerID == FWPS_LAYER_INBOUND_TRANSPORT_V6 ||
      layerID == FWPS_LAYER_INBOUND_TRANSPORT_V6_DISCARD ||
      layerID == FWPS_LAYER_OUTBOUND_TRANSPORT_V6 ||
      layerID == FWPS_LAYER_OUTBOUND_TRANSPORT_V6_DISCARD ||
      layerID == FWPS_LAYER_STREAM_V6 ||
      layerID == FWPS_LAYER_STREAM_V6_DISCARD ||
      layerID == FWPS_LAYER_DATAGRAM_DATA_V6 ||
      layerID == FWPS_LAYER_DATAGRAM_DATA_V6_DISCARD ||
      layerID == FWPS_LAYER_INBOUND_ICMP_ERROR_V6 ||
      layerID == FWPS_LAYER_INBOUND_ICMP_ERROR_V6_DISCARD ||
      layerID == FWPS_LAYER_OUTBOUND_ICMP_ERROR_V6 ||
      layerID == FWPS_LAYER_OUTBOUND_ICMP_ERROR_V6_DISCARD ||
      layerID == FWPS_LAYER_ALE_RESOURCE_ASSIGNMENT_V6 ||
      layerID == FWPS_LAYER_ALE_RESOURCE_ASSIGNMENT_V6_DISCARD ||
      layerID == FWPS_LAYER_ALE_AUTH_LISTEN_V6 ||
      layerID == FWPS_LAYER_ALE_AUTH_LISTEN_V6_DISCARD ||
      layerID == FWPS_LAYER_ALE_AUTH_CONNECT_V6 ||
      layerID == FWPS_LAYER_ALE_AUTH_CONNECT_V6_DISCARD ||
      layerID == FWPS_LAYER_ALE_AUTH_RECV_ACCEPT_V6 ||
      layerID == FWPS_LAYER_ALE_AUTH_RECV_ACCEPT_V6_DISCARD ||
      layerID == FWPS_LAYER_ALE_FLOW_ESTABLISHED_V6 ||
      layerID == FWPS_LAYER_ALE_FLOW_ESTABLISHED_V6_DISCARD ||

#if(NTDDI_VERSION >= NTDDI_WIN7)

      layerID == FWPS_LAYER_NAME_RESOLUTION_CACHE_V6 ||
      layerID == FWPS_LAYER_ALE_RESOURCE_RELEASE_V6 ||
      layerID == FWPS_LAYER_ALE_ENDPOINT_CLOSURE_V6 ||
      layerID == FWPS_LAYER_ALE_CONNECT_REDIRECT_V6 ||
      layerID == FWPS_LAYER_ALE_BIND_REDIRECT_V6 ||
      layerID == FWPS_LAYER_STREAM_PACKET_V6 ||

#if(NTDDI_VERSION >= NTDDI_WIN8)

      layerID == FWPS_LAYER_INGRESS_VSWITCH_TRANSPORT_V6 ||
      layerID == FWPS_LAYER_EGRESS_VSWITCH_TRANSPORT_V6 ||

#endif /// (NTDDI_VERSION >= NTDDI_WIN8)
#endif /// (NTDDI_VERSION >= NTDDI_WIN7)

      layerID == FWPS_LAYER_IPSEC_KM_DEMUX_V6 ||
      layerID == FWPS_LAYER_IPSEC_V6 ||
      layerID == FWPS_LAYER_IKEEXT_V6)
      isIPv6 = TRUE;

   return isIPv6;
}

/**
 @helper function="KrnlHlprFwpsLayerIsDiscard"
 
   Purpose: Determine if the layer is a discard layer.                                          <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
BOOLEAN KrnlHlprFwpsLayerIsDiscard(_In_ UINT32 layerID)
{
   BOOLEAN isDiscard = FALSE;

   if(layerID == FWPS_LAYER_INBOUND_IPPACKET_V4_DISCARD ||
      layerID == FWPS_LAYER_INBOUND_IPPACKET_V6_DISCARD ||
      layerID == FWPS_LAYER_OUTBOUND_IPPACKET_V4_DISCARD ||
      layerID == FWPS_LAYER_OUTBOUND_IPPACKET_V6_DISCARD ||
      layerID == FWPS_LAYER_IPFORWARD_V4_DISCARD ||
      layerID == FWPS_LAYER_IPFORWARD_V6_DISCARD ||
      layerID == FWPS_LAYER_INBOUND_TRANSPORT_V4_DISCARD ||
      layerID == FWPS_LAYER_INBOUND_TRANSPORT_V6_DISCARD ||
      layerID == FWPS_LAYER_OUTBOUND_TRANSPORT_V4_DISCARD ||
      layerID == FWPS_LAYER_OUTBOUND_TRANSPORT_V6_DISCARD ||
      layerID == FWPS_LAYER_STREAM_V4_DISCARD ||
      layerID == FWPS_LAYER_STREAM_V6_DISCARD ||
      layerID == FWPS_LAYER_DATAGRAM_DATA_V4_DISCARD ||
      layerID == FWPS_LAYER_DATAGRAM_DATA_V6_DISCARD ||
      layerID == FWPS_LAYER_INBOUND_ICMP_ERROR_V4_DISCARD ||
      layerID == FWPS_LAYER_INBOUND_ICMP_ERROR_V6_DISCARD ||
      layerID == FWPS_LAYER_OUTBOUND_ICMP_ERROR_V4_DISCARD ||
      layerID == FWPS_LAYER_OUTBOUND_ICMP_ERROR_V6_DISCARD ||
      layerID == FWPS_LAYER_ALE_RESOURCE_ASSIGNMENT_V4_DISCARD ||
      layerID == FWPS_LAYER_ALE_RESOURCE_ASSIGNMENT_V6_DISCARD ||
      layerID == FWPS_LAYER_ALE_AUTH_LISTEN_V4_DISCARD ||
      layerID == FWPS_LAYER_ALE_AUTH_LISTEN_V6_DISCARD ||
      layerID == FWPS_LAYER_ALE_AUTH_CONNECT_V4_DISCARD ||
      layerID == FWPS_LAYER_ALE_AUTH_CONNECT_V6_DISCARD ||
      layerID == FWPS_LAYER_ALE_AUTH_RECV_ACCEPT_V4_DISCARD ||
      layerID == FWPS_LAYER_ALE_AUTH_RECV_ACCEPT_V6_DISCARD ||
      layerID == FWPS_LAYER_ALE_FLOW_ESTABLISHED_V4_DISCARD ||
      layerID == FWPS_LAYER_ALE_FLOW_ESTABLISHED_V6_DISCARD)
      isDiscard = TRUE;

   return isDiscard;
}

/**
 @helper function="KrnlHlprFwpsLayerIDToString"
 
   Purpose: Return a string representation of the supplied layer.                               <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Success_(return != 0)
PSTR KrnlHlprFwpsLayerIDToString(_In_ UINT32 layerID)
{
   PSTR pLayerString = 0;

   if(layerID == FWPS_LAYER_INBOUND_IPPACKET_V4)
      pLayerString = "FWPS_LAYER_INBOUND_IPPACKET_V4";
   else if(layerID == FWPS_LAYER_INBOUND_IPPACKET_V4_DISCARD)
      pLayerString = "FWPS_LAYER_INBOUND_IPPACKET_V4_DISCARD";
   else if(layerID == FWPS_LAYER_INBOUND_IPPACKET_V6)
      pLayerString = "FWPS_LAYER_INBOUND_IPPACKET_V6";
   else if(layerID == FWPS_LAYER_INBOUND_IPPACKET_V6_DISCARD)
      pLayerString = "FWPS_LAYER_INBOUND_IPPACKET_V6_DISCARD";
   else if(layerID == FWPS_LAYER_OUTBOUND_IPPACKET_V4)
      pLayerString = "FWPS_LAYER_OUTBOUND_IPPACKET_V4";
   else if(layerID == FWPS_LAYER_OUTBOUND_IPPACKET_V4_DISCARD)
      pLayerString = "FWPS_LAYER_OUTBOUND_IPPACKET_V4_DISCARD";
   else if(layerID == FWPS_LAYER_OUTBOUND_IPPACKET_V6)
      pLayerString = "FWPS_LAYER_OUTBOUND_IPPACKET_V6";
   else if(layerID == FWPS_LAYER_OUTBOUND_IPPACKET_V6_DISCARD)
      pLayerString = "FWPS_LAYER_OUTBOUND_IPPACKET_V6_DISCARD";
   else if(layerID == FWPS_LAYER_IPFORWARD_V4)
      pLayerString = "FWPS_LAYER_IPFORWARD_V4";
   else if(layerID == FWPS_LAYER_IPFORWARD_V4_DISCARD)
      pLayerString = "FWPS_LAYER_IPFORWARD_V4_DISCARD";
   else if(layerID == FWPS_LAYER_IPFORWARD_V6)
      pLayerString = "FWPS_LAYER_IPFORWARD_V6";
   else if(layerID == FWPS_LAYER_IPFORWARD_V6_DISCARD)
      pLayerString = "FWPS_LAYER_IPFORWARD_V6_DISCARD";
   else if(layerID == FWPS_LAYER_INBOUND_TRANSPORT_V4)
      pLayerString = "FWPS_LAYER_INBOUND_TRANSPORT_V4";
   else if(layerID == FWPS_LAYER_INBOUND_TRANSPORT_V4_DISCARD)
      pLayerString = "FWPS_LAYER_INBOUND_TRANSPORT_V4_DISCARD";
   else if(layerID == FWPS_LAYER_INBOUND_TRANSPORT_V6)
      pLayerString = "FWPS_LAYER_INBOUND_TRANSPORT_V6";
   else if(layerID == FWPS_LAYER_INBOUND_TRANSPORT_V6_DISCARD)
      pLayerString = "FWPS_LAYER_INBOUND_TRANSPORT_V6_DISCARD";
   else if(layerID == FWPS_LAYER_OUTBOUND_TRANSPORT_V4)
      pLayerString = "FWPS_LAYER_OUTBOUND_TRANSPORT_V4";
   else if(layerID == FWPS_LAYER_OUTBOUND_TRANSPORT_V4_DISCARD)
      pLayerString = "FWPS_LAYER_OUTBOUND_TRANSPORT_V4_DISCARD";
   else if(layerID == FWPS_LAYER_OUTBOUND_TRANSPORT_V6)
      pLayerString = "FWPS_LAYER_OUTBOUND_TRANSPORT_V6";
   else if(layerID == FWPS_LAYER_OUTBOUND_TRANSPORT_V6_DISCARD)
      pLayerString = "FWPS_LAYER_OUTBOUND_TRANSPORT_V6_DISCARD";
   else if(layerID == FWPS_LAYER_STREAM_V4)
      pLayerString = "FWPS_LAYER_STREAM_V4";
   else if(layerID == FWPS_LAYER_STREAM_V4_DISCARD)
      pLayerString = "FWPS_LAYER_STREAM_V4_DISCARD";
   else if(layerID == FWPS_LAYER_STREAM_V6)
      pLayerString = "FWPS_LAYER_STREAM_V6";
   else if(layerID == FWPS_LAYER_STREAM_V6_DISCARD)
      pLayerString = "FWPS_LAYER_STREAM_V6_DISCARD";
   else if(layerID == FWPS_LAYER_DATAGRAM_DATA_V4)
      pLayerString = "FWPS_LAYER_DATAGRAM_DATA_V4";
   else if(layerID == FWPS_LAYER_DATAGRAM_DATA_V4_DISCARD)
      pLayerString = "FWPS_LAYER_DATAGRAM_DATA_V4_DISCARD";
   else if(layerID == FWPS_LAYER_DATAGRAM_DATA_V6)
      pLayerString = "FWPS_LAYER_DATAGRAM_DATA_V6";
   else if(layerID == FWPS_LAYER_DATAGRAM_DATA_V6_DISCARD)
      pLayerString = "FWPS_LAYER_DATAGRAM_DATA_V6_DISCARD";
   else if(layerID == FWPS_LAYER_INBOUND_ICMP_ERROR_V4)
      pLayerString = "FWPS_LAYER_INBOUND_ICMP_ERROR_V4";
   else if(layerID == FWPS_LAYER_INBOUND_ICMP_ERROR_V4_DISCARD)
      pLayerString = "FWPS_LAYER_INBOUND_ICMP_ERROR_V4_DISCARD";
   else if(layerID == FWPS_LAYER_INBOUND_ICMP_ERROR_V6)
      pLayerString = "FWPS_LAYER_INBOUND_ICMP_ERROR_V6";
   else if(layerID == FWPS_LAYER_INBOUND_ICMP_ERROR_V6_DISCARD)
      pLayerString = "FWPS_LAYER_INBOUND_ICMP_ERROR_V6_DISCARD";
   else if(layerID == FWPS_LAYER_OUTBOUND_ICMP_ERROR_V4)
      pLayerString = "FWPS_LAYER_OUTBOUND_ICMP_ERROR_V4";
   else if(layerID == FWPS_LAYER_OUTBOUND_ICMP_ERROR_V4_DISCARD)
      pLayerString = "FWPS_LAYER_OUTBOUND_ICMP_ERROR_V4_DISCARD";
   else if(layerID == FWPS_LAYER_OUTBOUND_ICMP_ERROR_V6)
      pLayerString = "FWPS_LAYER_OUTBOUND_ICMP_ERROR_V6";
   else if(layerID == FWPS_LAYER_OUTBOUND_ICMP_ERROR_V6_DISCARD)
      pLayerString = "FWPS_LAYER_OUTBOUND_ICMP_ERROR_V6_DISCARD";
   else if(layerID == FWPS_LAYER_ALE_RESOURCE_ASSIGNMENT_V4)
      pLayerString = "FWPS_LAYER_ALE_RESOURCE_ASSIGNMENT_V4";
   else if(layerID == FWPS_LAYER_ALE_RESOURCE_ASSIGNMENT_V4_DISCARD)
      pLayerString = "FWPS_LAYER_ALE_RESOURCE_ASSIGNMENT_V4_DISCARD";
   else if(layerID == FWPS_LAYER_ALE_RESOURCE_ASSIGNMENT_V6)
      pLayerString = "FWPS_LAYER_ALE_RESOURCE_ASSIGNMENT_V6";
   else if(layerID == FWPS_LAYER_ALE_RESOURCE_ASSIGNMENT_V6_DISCARD)
      pLayerString = "FWPS_LAYER_ALE_RESOURCE_ASSIGNMENT_V6_DISCARD";
   else if(layerID == FWPS_LAYER_ALE_AUTH_LISTEN_V4)
      pLayerString = "FWPS_LAYER_ALE_AUTH_LISTEN_V4";
   else if(layerID == FWPS_LAYER_ALE_AUTH_LISTEN_V4_DISCARD)
      pLayerString = "FWPS_LAYER_ALE_AUTH_LISTEN_V4_DISCARD";
   else if(layerID == FWPS_LAYER_ALE_AUTH_LISTEN_V6)
      pLayerString = "FWPS_LAYER_ALE_AUTH_LISTEN_V6";
   else if(layerID == FWPS_LAYER_ALE_AUTH_LISTEN_V6_DISCARD)
      pLayerString = "FWPS_LAYER_ALE_AUTH_LISTEN_V6_DISCARD";
   else if(layerID == FWPS_LAYER_ALE_AUTH_RECV_ACCEPT_V4)
      pLayerString = "FWPS_LAYER_ALE_AUTH_RECV_ACCEPT_V4";
   else if(layerID == FWPS_LAYER_ALE_AUTH_RECV_ACCEPT_V4_DISCARD)
      pLayerString = "FWPS_LAYER_ALE_AUTH_RECV_ACCEPT_V4_DISCARD";
   else if(layerID == FWPS_LAYER_ALE_AUTH_RECV_ACCEPT_V6)
      pLayerString = "FWPS_LAYER_ALE_AUTH_RECV_ACCEPT_V6";
   else if(layerID == FWPS_LAYER_ALE_AUTH_RECV_ACCEPT_V6_DISCARD)
      pLayerString = "FWPS_LAYER_ALE_AUTH_RECV_ACCEPT_V6_DISCARD";
   else if(layerID == FWPS_LAYER_ALE_AUTH_CONNECT_V4)
      pLayerString = "FWPS_LAYER_ALE_AUTH_CONNECT_V4";
   else if(layerID == FWPS_LAYER_ALE_AUTH_CONNECT_V4_DISCARD)
      pLayerString = "FWPS_LAYER_ALE_AUTH_CONNECT_V4_DISCARD";
   else if(layerID == FWPS_LAYER_ALE_AUTH_CONNECT_V6)
      pLayerString = "FWPS_LAYER_ALE_AUTH_CONNECT_V6";
   else if(layerID == FWPS_LAYER_ALE_AUTH_CONNECT_V6_DISCARD)
      pLayerString = "FWPS_LAYER_ALE_AUTH_CONNECT_V6_DISCARD";
   else if(layerID == FWPS_LAYER_ALE_FLOW_ESTABLISHED_V4)
      pLayerString = "FWPS_LAYER_ALE_FLOW_ESTABLISHED_V4";
   else if(layerID == FWPS_LAYER_ALE_FLOW_ESTABLISHED_V4_DISCARD)
      pLayerString = "FWPS_LAYER_ALE_FLOW_ESTABLISHED_V4_DISCARD";
   else if(layerID == FWPS_LAYER_ALE_FLOW_ESTABLISHED_V6)
      pLayerString = "FWPS_LAYER_ALE_FLOW_ESTABLISHED_V6";
   else if(layerID == FWPS_LAYER_ALE_FLOW_ESTABLISHED_V6_DISCARD)
      pLayerString = "FWPS_LAYER_ALE_FLOW_ESTABLISHED_V6_DISCARD";

#if(NTDDI_VERSION >= NTDDI_WIN7)

   else if(layerID == FWPS_LAYER_NAME_RESOLUTION_CACHE_V4)
      pLayerString = "FWPS_LAYER_NAME_RESOLUTION_CACHE_V4";
   else if(layerID == FWPS_LAYER_NAME_RESOLUTION_CACHE_V6)
      pLayerString = "FWPS_LAYER_NAME_RESOLUTION_CACHE_V6";
   else if(layerID == FWPS_LAYER_ALE_RESOURCE_RELEASE_V4)
      pLayerString = "FWPS_LAYER_ALE_RESOURCE_RELEASE_V4";
   else if(layerID == FWPS_LAYER_ALE_RESOURCE_RELEASE_V6)
      pLayerString = "FWPS_LAYER_ALE_RESOURCE_RELEASE_V6";
   else if(layerID == FWPS_LAYER_ALE_ENDPOINT_CLOSURE_V4)
      pLayerString = "FWPS_LAYER_ALE_ENDPOINT_CLOSURE_V4";
   else if(layerID == FWPS_LAYER_ALE_ENDPOINT_CLOSURE_V6)
      pLayerString = "FWPS_LAYER_ALE_ENDPOINT_CLOSURE_V6";
   else if(layerID == FWPS_LAYER_ALE_CONNECT_REDIRECT_V4)
      pLayerString = "FWPS_LAYER_ALE_CONNECT_REDIRECT_V4";
   else if(layerID == FWPS_LAYER_ALE_CONNECT_REDIRECT_V6)
      pLayerString = "FWPS_LAYER_ALE_CONNECT_REDIRECT_V6";
   else if(layerID == FWPS_LAYER_ALE_BIND_REDIRECT_V4)
      pLayerString = "FWPS_LAYER_ALE_BIND_REDIRECT_V4";
   else if(layerID == FWPS_LAYER_ALE_BIND_REDIRECT_V6)
      pLayerString = "FWPS_LAYER_ALE_BIND_REDIRECT_V6";
   else if(layerID == FWPS_LAYER_STREAM_PACKET_V4)
      pLayerString = "FWPS_LAYER_STREAM_PACKET_V4";
   else if(layerID == FWPS_LAYER_STREAM_PACKET_V6)
      pLayerString = "FWPS_LAYER_STREAM_PACKET_V6";

#if(NTDDI_VERSION >= NTDDI_WIN8)

   else if(layerID == FWPS_LAYER_INBOUND_MAC_FRAME_ETHERNET)
      pLayerString = "FWPS_LAYER_INBOUND_MAC_FRAME_ETHERNET";
   else if(layerID == FWPS_LAYER_OUTBOUND_MAC_FRAME_ETHERNET)
      pLayerString = "FWPS_LAYER_OUTBOUND_MAC_FRAME_ETHERNET";
   else if(layerID == FWPS_LAYER_INBOUND_MAC_FRAME_NATIVE)
      pLayerString = "FWPS_LAYER_INBOUND_MAC_FRAME_NATIVE";
   else if(layerID == FWPS_LAYER_OUTBOUND_MAC_FRAME_NATIVE)
      pLayerString = "FWPS_LAYER_OUTBOUND_MAC_FRAME_NATIVE";
   else if(layerID == FWPS_LAYER_INGRESS_VSWITCH_ETHERNET)
      pLayerString = "FWPS_LAYER_INGRESS_VSWITCH_ETHERNET";
   else if(layerID == FWPS_LAYER_EGRESS_VSWITCH_ETHERNET)
      pLayerString = "FWPS_LAYER_EGRESS_VSWITCH_ETHERNET";
   else if(layerID == FWPS_LAYER_INGRESS_VSWITCH_TRANSPORT_V4)
      pLayerString = "FWPS_LAYER_INGRESS_VSWITCH_TRANSPORT_V4";
   else if(layerID == FWPS_LAYER_INGRESS_VSWITCH_TRANSPORT_V6)
      pLayerString = "FWPS_LAYER_INGRESS_VSWITCH_TRANSPORT_V6";
   else if(layerID == FWPS_LAYER_EGRESS_VSWITCH_TRANSPORT_V4)
      pLayerString = "FWPS_LAYER_EGRESS_VSWITCH_TRANSPORT_V4";
   else if(layerID == FWPS_LAYER_EGRESS_VSWITCH_TRANSPORT_V6)
      pLayerString = "FWPS_LAYER_EGRESS_VSWITCH_TRANSPORT_V6";

#if(NTDDI_VERSION >= NTDDI_WINBLUE)

   else if(layerID == FWPS_LAYER_INBOUND_TRANSPORT_FAST)
      pLayerString = "FWPS_LAYER_INBOUND_TRANSPORT_FAST";
   else if(layerID == FWPS_LAYER_OUTBOUND_TRANSPORT_FAST)
      pLayerString = "FWPS_LAYER_OUTBOUND_TRANSPORT_FAST";
   else if(layerID == FWPS_LAYER_INBOUND_MAC_FRAME_NATIVE_FAST)
      pLayerString = "FWPS_LAYER_INBOUND_MAC_FRAME_NATIVE_FAST";
   else if(layerID == FWPS_LAYER_OUTBOUND_MAC_FRAME_NATIVE_FAST)
      pLayerString = "FWPS_LAYER_OUTBOUND_MAC_FRAME_NATIVE_FAST";

#endif /// (NTDDI_VERSION >= NTDDI_WINBLUE)
#endif /// (NTDDI_VERSION >= NTDDI_WIN8)
#endif /// (NTDDI_VERSION >= NTDDI_WIN7)

   return pLayerString;
}

#endif /// FwpsLayer____
