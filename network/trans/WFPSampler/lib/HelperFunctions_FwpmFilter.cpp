////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2014 Microsoft Corporation.  All Rights Reserved.
//
//   Module Name:
//      HelperFunctions_FwpmFilter.cpp
//
//   Abstract:
//      This module contains functions which assist in actions pertaining to FWPM_FILTER objects.
//
//   Naming Convention:
//
//      <Scope><Module><Object><Action><Modifier>
//  
//      i.e.
//
//       <Scope>
//          {
//                                - Function is likely visible to other modules.
//            Prv                 - Function is private to this module.
//          }
//       <Module>
//          {
//            Hlpr                - Function is from HelperFunctions_* Modules.
//          }
//       <Object>
//          {
//            FwpByteBlob         - Function pertains to FWP_BYTE_BLOB objects.
//            FwpTokenInformation - Function pertains to FWP_TOKEN_INFORMATION objects.
//            FwpV4AddrMask       - Function pertains to FWP_V4_ADDR_MASK objects.
//            FwpV6AddrMask       - Function pertains to FWP_V6_ADDR_MASK objects.
//            FwpValue            - Function pertains to FWP_VALUE objects.
//            FwpRange            - Function pertains to FWP_RANGE objects.
//            FwpmFilter          - Function pertains to FWPM_FILTER objects.
//            FwpmFilterCondition - Function pertains to FWPM_FILTER_CONDITION objects.
//          }
//       <Action>
//          {
//            Add                 - Function adds an object.
//            Create              - Function allocates memory for an object.
//            Delete              - Function deletes an object.
//            Destroy             - Function frees memory for an object.
//            Enum                - Function returns list of requested objects.
//            IsValid             - Function compares values.
//            Purge               - Function cleans up values.
//            Remove              - Function deletes objects.
//          }
//       <Modifier>
//          {
//            All                 - Functions acts on all of WFPSampler's filters.
//            ByKey               - Function acts off of the object's GUID.
//            EnumHandle          - Function acts on the enumeration handle.
//            ForLayer            - Function acts based on provided layer value.
//            FromPacketData      - Function acts off of the provided PACKET_DATA.
//          }
//
//   Private Functions:
//      PrvFwpByteBlobDestroy(),
//      PrvFwpByteBlobPurge(),
//      PrvFwpRangeDestroy(),
//      PrvFwpTokenInformationDestroy(),
//      PrvFwpTokenInformationPurge(),
//      PrvFwpV4AddrAndMaskDestroy(),
//      PrvFwpV6AddrAndMaskDestroy(),
//      PrvFwpValuePurge(),
//
//   Public Functions:
//      HlprFwpmFilterAdd(),
//      HlprFwpmFilterConditionCreateFromFrameData(),
//      HlprFwpmFilterConditionCreateFromPacketData(),
//      HlprFwpmFilterConditionCreateFromScenarioData(),
//      HlprFwpmFilterConditionDestroy(),
//      HlprFwpmFilterConditionIsValidForLayer(),
//      HlprFwpmFilterConditionPrune(),
//      HlprFwpmFilterConditionPurge(),
//      HlprFwpmFilterCreateEnumHandle(),
//      HlprFwpmFilterDeleteByKey(),
//      HlprFwpmFilterDestroyEnumHandle(),
//      HlprFwpmFilterEnum(),
//      HlprFwpmFilterPurge(),
//      HlprFwpmFilterRemoveAll(),
//
//   Author:
//      Dusty Harper      (DHarper)
//
//   Revision History:
//
//      [ Month ][Day] [Year] - [Revision]-[ Comments ]
//      May       01,   2010  -     1.0   -  Creation
//      December  13,   2013  -     1.1   -  Add HlprFwpmFilterConditionPrune
//
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "HelperFunctions_Include.h" /// .

/**
 @private_function="PrvFwpByteBlobPurge"
 
   Purpose:  Cleanup an FWP_BYTE_BLOB.                                                          <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/AA364911.aspx              <br>
*/
inline VOID PrvFwpByteBlobPurge(_Inout_ FWP_BYTE_BLOB* pBlob)
{
   ASSERT(pBlob);

   HLPR_DELETE_ARRAY(pBlob->data);

   return;
}

/**
 @private_function="PrvFwpByteBlobDestroy"
 
   Purpose:  Cleanup and free an FWP_BYTE_BLOB.                                                 <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/AA364911.aspx              <br>
*/
_At_(*ppBlob, _Post_ _Null_)
VOID PrvFwpByteBlobDestroy(_Inout_ FWP_BYTE_BLOB** ppBlob)
{
   ASSERT(ppBlob);

   if(*ppBlob)
      PrvFwpByteBlobPurge(*ppBlob);

   HLPR_DELETE(*ppBlob);

   return;
}

/**
 @private_function="PrvFwpTokenInformationPurge"
 
   Purpose:  Cleanup an FWP_TOKEN_INFORMATION.                                                  <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/BB485774.aspx              <br>
*/
inline VOID PrvFwpTokenInformationPurge(_Inout_ FWP_TOKEN_INFORMATION* pTokenInfo)
{
   ASSERT(pTokenInfo);

   for(UINT32 i = 0;
       i < pTokenInfo->sidCount;
       i++)
   {
      HLPR_DELETE_ARRAY(pTokenInfo->sids[i].Sid);
   }
   
   HLPR_DELETE_ARRAY(pTokenInfo->sids);
   
   for(UINT32 i = 0;
       i < pTokenInfo->restrictedSidCount;
       i++)
   {
      HLPR_DELETE_ARRAY(pTokenInfo->restrictedSids[i].Sid);
   }
   
   HLPR_DELETE_ARRAY(pTokenInfo->restrictedSids);

   return;
}

/**
 @private_function="PrvFwpTokenInformationDestroy"
 
   Purpose:  Cleanup and free an FWP_TOKEN_INFORMATION.                                         <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/BB485774.aspx              <br>
*/
_At_(*ppTokenInfo, _Post_ _Null_)
VOID PrvFwpTokenInformationDestroy(_Inout_ FWP_TOKEN_INFORMATION** ppTokenInfo)
{
   ASSERT(ppTokenInfo);

   if(*ppTokenInfo)
      PrvFwpTokenInformationPurge(*ppTokenInfo);

   HLPR_DELETE(*ppTokenInfo);

   return;
}

/**
 @private_function="PrvFwpV4AddrAndMaskDestroy"

   Purpose:  Cleanup and free an FWP_V4_ADDR_AND_MASK.                                          <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/AA364950.aspx              <br>
*/
_At_(*ppAddrMask, _Post_ _Null_)
VOID PrvFwpV4AddrAndMaskDestroy(_Inout_ FWP_V4_ADDR_AND_MASK** ppAddrMask)
{
   ASSERT(ppAddrMask);

   HLPR_DELETE(*ppAddrMask);

   return;
}

/**
 @private_function="PrvFwpV6AddrAndMaskDestroy"

   Purpose:  Cleanup and free an FWP_V6_ADDR_AND_MASK.                                          <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/AA364954.aspx              <br>
*/
_At_(*ppAddrMask, _Post_ _Null_)
VOID PrvFwpV6AddrAndMaskDestroy(_Inout_ FWP_V6_ADDR_AND_MASK** ppAddrMask)
{
   ASSERT(ppAddrMask);

   HLPR_DELETE(*ppAddrMask);

   return;
}

/**
 @private_function="PrvFwpValuePurge"
 
   Purpose:  Cleanup an FWP_VALUE.                                                              <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/AA364956.aspx              <br>
*/
VOID PrvFwpValuePurge(_Inout_ FWP_VALUE* pValue)
{
   ASSERT(pValue);

   switch(pValue->type)
   {
      case FWP_UINT64:
      {
         HLPR_DELETE(pValue->uint64);

         break;
      }
      case FWP_INT64:
      {
         HLPR_DELETE(pValue->int64);

         break;
      }
      case FWP_DOUBLE:
      {
         HLPR_DELETE(pValue->double64);

         break;
      }
      case FWP_BYTE_ARRAY16_TYPE:
      {
         HLPR_DELETE(pValue->byteArray16);

         break;
      }
      case FWP_BYTE_BLOB_TYPE:
      {
         PrvFwpByteBlobDestroy(&(pValue->byteBlob));

         break;
      }
      case FWP_SID:
      {
         HLPR_DELETE_ARRAY(pValue->sid);

         break;
      }
      case FWP_SECURITY_DESCRIPTOR_TYPE:
      {
         PrvFwpByteBlobDestroy(&(pValue->sd));

         break;
      }
      case FWP_TOKEN_INFORMATION_TYPE:
      {
         PrvFwpTokenInformationDestroy(&(pValue->tokenInformation));

         break;
      }
      case FWP_TOKEN_ACCESS_INFORMATION_TYPE:
      {
         PrvFwpByteBlobDestroy(&(pValue->tokenAccessInformation));

         break;
      }
      case FWP_UNICODE_STRING_TYPE:
      {
         HLPR_DELETE_ARRAY(pValue->unicodeString);

         break;
      }

#if(NTDDI_VERSION >= NTDDI_WIN7)

      case FWP_BYTE_ARRAY6_TYPE:
      {
         HLPR_DELETE(pValue->byteArray6);

         break;
      }

#endif /// (NTDDI_VERSION >= NTDDI_WIN7)

      default:
      {
         pValue->uint64 = 0;

         break;
      }
   }

   pValue->type = FWP_EMPTY;

   return;
}

/**
 @private_function="PrvFwpRangeDestroy"
 
   Purpose:  Cleanup and free an FWP_RANGE.                                                     <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/AA814108.aspx              <br>
*/
_At_(*ppRange, _Post_ _Null_)
_Success_(*ppRange == 0)
VOID PrvFwpRangeDestroy(_Inout_ FWP_RANGE** ppRange)
{
   ASSERT(ppRange);

   if(*ppRange)
   {
      PrvFwpValuePurge(&((*ppRange)->valueLow));

      PrvFwpValuePurge(&((*ppRange)->valueHigh));
   }

   HLPR_DELETE(*ppRange);

   return;
}
/**
 @helper_function="HlprFwpmFilterConditionPrune"
 
   Purpose:  Remove duplicate conditions from the supplied filterCondition array.               <br>
                                                                                                <br>
   Notes:    Due to a bug in WFP, filter enumeration cannot handle multiple consecutive 
             conditions.                                                                        <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/AA364268.aspx              <br>
*/
VOID HlprFwpmFilterConditionPrune(_In_reads_(numFilterConditions) const FWPM_FILTER_CONDITION* pFilterConditions,
                                  _Inout_ UINT32 numFilterConditions,
                                  _Inout_updates_all_(*pNewNumFilterConditions) FWPM_FILTER_CONDITION** ppNewFilterConditions,
                                  _Inout_ UINT32* pNewNumFilterConditions)
{
   UINT32 numConditions = numFilterConditions;

   if(numFilterConditions >= 2)
   {
      for(UINT32 index = 0;
          (index + 1 ) < numFilterConditions;
          index++)
      {
         if(pFilterConditions[index].fieldKey == pFilterConditions[index + 1].fieldKey)
            numConditions--;
      }

      if(numConditions != numFilterConditions)
      {
         UINT32                 status               = 0;
         FWPM_FILTER_CONDITION* pNewFilterConditions = 0;
         UINT32                 newIndex             = 0;

         HLPR_NEW_ARRAY(pNewFilterConditions,
                        FWPM_FILTER_CONDITION,
                        numConditions);
         HLPR_BAIL_ON_ALLOC_FAILURE(pNewFilterConditions,
                                    status);

         for(UINT32 index = 0;
             index < numFilterConditions &&
             newIndex < numConditions;
             index++)
         {
            // Always copy the first condition ...
            if(index == 0)
            {
               RtlCopyMemory(&(pNewFilterConditions[newIndex]),
                             &(pFilterConditions[index]),
                             sizeof(FWPM_FILTER_CONDITION));

               newIndex++;
            }
            else
            {
               /// ... then copy only if the current condition is not the same as the previous 
               /// condition.
               if(pFilterConditions[index].fieldKey != pFilterConditions[index - 1].fieldKey)
               {
                  RtlCopyMemory(&(pNewFilterConditions[newIndex]),
                                &(pFilterConditions[index]),
                                sizeof(FWPM_FILTER_CONDITION));

                  newIndex++;
               }
            }
         }

         *ppNewFilterConditions = pNewFilterConditions;

         *pNewNumFilterConditions = numConditions;
      }
   }

   HLPR_BAIL_LABEL:

   return;
}

/**
 @helper_function="HlprFwpmFilterConditionPurge"
 
   Purpose:  Cleanup an FWPM_FILTER_CONDITION.                                                  <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/AA364268.aspx              <br>
*/
_Success_(pFilterCondition->conditionValue.type == FWP_EMPTY &&
          pFilterCondition->conditionValue.uint64 == 0)
VOID HlprFwpmFilterConditionPurge(_Inout_ FWPM_FILTER_CONDITION* pFilterCondition)
{
   ASSERT(pFilterCondition);

   FWP_CONDITION_VALUE* pValue = &(pFilterCondition->conditionValue);

   switch(pValue->type)
   {
      case FWP_UINT64:
      {
         HLPR_DELETE(pValue->uint64);

         break;
      }
      case FWP_INT64:
      {
         HLPR_DELETE(pValue->int64);

         break;
      }
      case FWP_DOUBLE:
      {
         HLPR_DELETE(pValue->double64);

         break;
      }
      case FWP_BYTE_ARRAY16_TYPE:
      {
         HLPR_DELETE(pValue->byteArray16);

         break;
      }
      case FWP_BYTE_BLOB_TYPE:
      {
         PrvFwpByteBlobDestroy(&(pValue->byteBlob));

         break;
      }
      case FWP_SID:
      {
         HLPR_DELETE_ARRAY(pValue->sid);

         break;
      }
      case FWP_SECURITY_DESCRIPTOR_TYPE:
      {
         PrvFwpByteBlobDestroy(&(pValue->sd));

         break;
      }
      case FWP_TOKEN_INFORMATION_TYPE:
      {
         PrvFwpTokenInformationDestroy(&(pValue->tokenInformation));

         break;
      }
      case FWP_TOKEN_ACCESS_INFORMATION_TYPE:
      {
         PrvFwpByteBlobDestroy(&(pValue->tokenAccessInformation));

         break;
      }
      case FWP_UNICODE_STRING_TYPE:
      {
         HLPR_DELETE_ARRAY(pValue->unicodeString);

         break;
      }

#if(NTDDI_VERSION >= NTDDI_WIN7)

      case FWP_BYTE_ARRAY6_TYPE:
      {
         HLPR_DELETE(pValue->byteArray6);

         break;
      }

#endif // (NTDDI_VERSION >= NTDDI_WIN7)

      case FWP_SINGLE_DATA_TYPE_MAX:
      {
         break;
      }
      case FWP_V4_ADDR_MASK:
      {
         PrvFwpV4AddrAndMaskDestroy(&(pValue->v4AddrMask));

         break;
      }
      case FWP_V6_ADDR_MASK:
      {
         PrvFwpV6AddrAndMaskDestroy(&(pValue->v6AddrMask));

         break;
      }
      case FWP_RANGE_TYPE:
      {
         PrvFwpRangeDestroy(&(pValue->rangeValue));

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

   pValue->type = FWP_EMPTY;

   return;
}

/**
 @helper_function="HlprFwpmFilterConditionDestroy"
 
   Purpose:  Cleanup and free an (array of) FWPM_FILTER_CONDITION.                              <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/AA364268.aspx              <br>
*/
_At_(*ppFilterConditions, _Post_ _Null_)
_Success_(*ppFilterConditions == 0)
VOID HlprFwpmFilterConditionDestroy(_Inout_updates_all_(numFilterConditions) FWPM_FILTER_CONDITION** ppFilterConditions,
                                    _In_ const UINT32 numFilterConditions)                                               /* 1 */
{
   ASSERT(ppFilterConditions);
   ASSERT(numFilterConditions);

   for(UINT32 conditionIndex = 0;
       conditionIndex < numFilterConditions;
       conditionIndex++)
   {
      HlprFwpmFilterConditionPurge(&((*ppFilterConditions)[conditionIndex]));
   }

   if(numFilterConditions > 1)
   {
      HLPR_DELETE_ARRAY(*ppFilterConditions);
   }
   else
   {
      HLPR_DELETE(*ppFilterConditions);
   }

   return;
}

/**
 @helper_function="HlprFwpmFilterConditionCreate"
 
   Purpose:  Allocate an (array of) FWPM_FILTER_CONDITION.                                      <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/AA364268.aspx              <br>
*/
_Success_(return == NO_ERROR)
UINT32 HlprFwpmFilterConditionCreate(_Inout_updates_all_(numFilterConditions) FWPM_FILTER_CONDITION** ppFilterConditions,
                                     _In_ const UINT32 numFilterConditions)                                               /* 1 */
{
   UINT32 status = NO_ERROR;

   if(ppFilterConditions &&
      numFilterConditions)
   {
      if(numFilterConditions > 1)
      {
         HLPR_NEW_ARRAY(*ppFilterConditions,
                        FWPM_FILTER_CONDITION,
                        numFilterConditions);
      }
      else
      {
         HLPR_NEW(*ppFilterConditions,
                  FWPM_FILTER_CONDITION);
      }
   }
   else
   {
      status = ERROR_INVALID_PARAMETER;

      HlprLogError(L"HlprFwpmFilterConditionCreate() [status: %#x][pFilterConditions: %#p][numFilterConditions: %d]",
                   status,
                   ppFilterConditions,
                   numFilterConditions);
   }

   return status;
}

/**
 @helper_function="HlprFwpmFilterConditionIsValidForLayer"
 
   Purpose:  Determine if the provided filtering condition is available at the provided layer.  <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/AA363996.aspx              <br>
*/
BOOLEAN HlprFwpmFilterConditionIsValidForLayer(_In_ const GUID* pLayerKey,
                                               _In_ const GUID* pConditionKey)
{
   ASSERT(pLayerKey);
   ASSERT(pConditionKey);

   BOOLEAN isValid = FALSE;

   if(*pLayerKey == FWPM_LAYER_INBOUND_IPPACKET_V4 ||
      *pLayerKey == FWPM_LAYER_INBOUND_IPPACKET_V4_DISCARD ||
      *pLayerKey == FWPM_LAYER_INBOUND_IPPACKET_V6 ||
      *pLayerKey == FWPM_LAYER_INBOUND_IPPACKET_V6_DISCARD)
   {
      for(UINT32 conditionIndex = 0;
          conditionIndex < INBOUND_IPPACKET_CONDITIONS_COUNT;
          conditionIndex++)
      {
         if(HlprGUIDsAreEqual(pConditionKey,
                              ppConditionInboundIPPacket[conditionIndex]))
         {
            isValid = TRUE;

            break;
         }
      }
   }
   else if(*pLayerKey == FWPM_LAYER_OUTBOUND_IPPACKET_V4 ||
           *pLayerKey == FWPM_LAYER_OUTBOUND_IPPACKET_V4_DISCARD ||
           *pLayerKey == FWPM_LAYER_OUTBOUND_IPPACKET_V6 ||
           *pLayerKey == FWPM_LAYER_OUTBOUND_IPPACKET_V6_DISCARD)
   {
      for(UINT32 conditionIndex = 0;
          conditionIndex < OUTBOUND_IPPACKET_CONDITIONS_COUNT;
          conditionIndex++)
      {
         if(HlprGUIDsAreEqual(pConditionKey,
                              ppConditionOutboundIPPacket[conditionIndex]))
         {
            isValid = TRUE;

            break;
         }
      }
   }
   else if(*pLayerKey == FWPM_LAYER_IPFORWARD_V4 ||
           *pLayerKey == FWPM_LAYER_IPFORWARD_V4_DISCARD ||
           *pLayerKey == FWPM_LAYER_IPFORWARD_V6 ||
           *pLayerKey == FWPM_LAYER_IPFORWARD_V6_DISCARD)
   {
      for(UINT32 conditionIndex = 0;
          conditionIndex < IPFORWARD_CONDITIONS_COUNT;
          conditionIndex++)
      {
         if(HlprGUIDsAreEqual(pConditionKey,
                              ppConditionIPForward[conditionIndex]))
         {
            isValid = TRUE;

            break;
         }
      }
   }
   else if(*pLayerKey == FWPM_LAYER_INBOUND_TRANSPORT_V4 ||
           *pLayerKey == FWPM_LAYER_INBOUND_TRANSPORT_V4_DISCARD ||
           *pLayerKey == FWPM_LAYER_INBOUND_TRANSPORT_V6 ||
           *pLayerKey == FWPM_LAYER_INBOUND_TRANSPORT_V6_DISCARD)
   {
      for(UINT32 conditionIndex = 0;
          conditionIndex < INBOUND_TRANSPORT_CONDITIONS_COUNT;
          conditionIndex++)
      {
         if(HlprGUIDsAreEqual(pConditionKey,
                              ppConditionInboundTransport[conditionIndex]))
         {
            isValid = TRUE;

            break;
         }
      }
   }
   else if(*pLayerKey == FWPM_LAYER_OUTBOUND_TRANSPORT_V4 ||
           *pLayerKey == FWPM_LAYER_OUTBOUND_TRANSPORT_V4_DISCARD ||
           *pLayerKey == FWPM_LAYER_OUTBOUND_TRANSPORT_V6 ||
           *pLayerKey == FWPM_LAYER_OUTBOUND_TRANSPORT_V6_DISCARD)
   {
      for(UINT32 conditionIndex = 0;
          conditionIndex < OUTBOUND_TRANSPORT_CONDITIONS_COUNT;
          conditionIndex++)
      {
         if(HlprGUIDsAreEqual(pConditionKey,
                              ppConditionOutboundTransport[conditionIndex]))
         {
            isValid = TRUE;

            break;
         }
      }
   }
   else if(*pLayerKey == FWPM_LAYER_STREAM_V4 ||
           *pLayerKey == FWPM_LAYER_STREAM_V4_DISCARD ||
           *pLayerKey == FWPM_LAYER_STREAM_V6 ||
           *pLayerKey == FWPM_LAYER_STREAM_V6_DISCARD)
   {
      for(UINT32 conditionIndex = 0;
          conditionIndex < STREAM_CONDITIONS_COUNT;
          conditionIndex++)
      {
         if(HlprGUIDsAreEqual(pConditionKey,
                              ppConditionStream[conditionIndex]))
         {
            isValid = TRUE;

            break;
         }
      }
   }
   else if(*pLayerKey == FWPM_LAYER_DATAGRAM_DATA_V4 ||
           *pLayerKey == FWPM_LAYER_DATAGRAM_DATA_V4_DISCARD ||
           *pLayerKey == FWPM_LAYER_DATAGRAM_DATA_V6 ||
           *pLayerKey == FWPM_LAYER_DATAGRAM_DATA_V6_DISCARD)
   {
      for(UINT32 conditionIndex = 0;
          conditionIndex < DATAGRAM_DATA_CONDITIONS_COUNT;
          conditionIndex++)
      {
         if(HlprGUIDsAreEqual(pConditionKey,
                              ppConditionDatagramData[conditionIndex]))
         {
            isValid = TRUE;

            break;
         }
      }
   }
   else if(*pLayerKey == FWPM_LAYER_INBOUND_ICMP_ERROR_V4 ||
           *pLayerKey == FWPM_LAYER_INBOUND_ICMP_ERROR_V4_DISCARD ||
           *pLayerKey == FWPM_LAYER_INBOUND_ICMP_ERROR_V6 ||
           *pLayerKey == FWPM_LAYER_INBOUND_ICMP_ERROR_V6_DISCARD)
   {
      for(UINT32 conditionIndex = 0;
          conditionIndex < INBOUND_ICMP_ERROR_CONDITIONS_COUNT;
          conditionIndex++)
      {
         if(HlprGUIDsAreEqual(pConditionKey,
                              ppConditionInboundICMPError[conditionIndex]))
         {
            isValid = TRUE;

            break;
         }
      }
   }
   else if(*pLayerKey == FWPM_LAYER_OUTBOUND_ICMP_ERROR_V4 ||
           *pLayerKey == FWPM_LAYER_OUTBOUND_ICMP_ERROR_V4_DISCARD ||
           *pLayerKey == FWPM_LAYER_OUTBOUND_ICMP_ERROR_V6 ||
           *pLayerKey == FWPM_LAYER_OUTBOUND_ICMP_ERROR_V6_DISCARD)
   {
      for(UINT32 conditionIndex = 0;
          conditionIndex < OUTBOUND_ICMP_ERROR_CONDITIONS_COUNT;
          conditionIndex++)
      {
         if(HlprGUIDsAreEqual(pConditionKey,
                              ppConditionOutboundICMPError[conditionIndex]))
         {
            isValid = TRUE;

            break;
         }
      }
   }
   else if(*pLayerKey == FWPM_LAYER_ALE_RESOURCE_ASSIGNMENT_V4 ||
           *pLayerKey == FWPM_LAYER_ALE_RESOURCE_ASSIGNMENT_V4_DISCARD ||
           *pLayerKey == FWPM_LAYER_ALE_RESOURCE_ASSIGNMENT_V6 ||
           *pLayerKey == FWPM_LAYER_ALE_RESOURCE_ASSIGNMENT_V6_DISCARD)
   {
      for(UINT32 conditionIndex = 0;
          conditionIndex < ALE_RESOURCE_ASSIGNMENT_CONDITIONS_COUNT;
          conditionIndex++)
      {
         if(HlprGUIDsAreEqual(pConditionKey,
                              ppConditionALEResourceAssignment[conditionIndex]))
         {
            isValid = TRUE;

            break;
         }
      }
   }
   else if(*pLayerKey == FWPM_LAYER_ALE_AUTH_LISTEN_V4 ||
           *pLayerKey == FWPM_LAYER_ALE_AUTH_LISTEN_V4_DISCARD ||
           *pLayerKey == FWPM_LAYER_ALE_AUTH_LISTEN_V6 ||
           *pLayerKey == FWPM_LAYER_ALE_AUTH_LISTEN_V6_DISCARD)
   {
      for(UINT32 conditionIndex = 0;
          conditionIndex < ALE_AUTH_LISTEN_CONDITIONS_COUNT;
          conditionIndex++)
      {
         if(HlprGUIDsAreEqual(pConditionKey,
                              ppConditionALEAuthListen[conditionIndex]))
         {
            isValid = TRUE;

            break;
         }
      }
   }
   else if(*pLayerKey == FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V4 ||
           *pLayerKey == FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V4_DISCARD ||
           *pLayerKey == FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V6 ||
           *pLayerKey == FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V6_DISCARD)
   {
      for(UINT32 conditionIndex = 0;
          conditionIndex < ALE_AUTH_RECV_ACCEPT_CONDITIONS_COUNT;
          conditionIndex++)
      {
         if(HlprGUIDsAreEqual(pConditionKey,
                              ppConditionALEAuthRecvAccept[conditionIndex]))
         {
            isValid = TRUE;

            break;
         }
      }
   }
   else if(*pLayerKey == FWPM_LAYER_ALE_AUTH_CONNECT_V4 ||
           *pLayerKey == FWPM_LAYER_ALE_AUTH_CONNECT_V4_DISCARD ||
           *pLayerKey == FWPM_LAYER_ALE_AUTH_CONNECT_V6 ||
           *pLayerKey == FWPM_LAYER_ALE_AUTH_CONNECT_V6_DISCARD)
   {
      for(UINT32 conditionIndex = 0;
          conditionIndex < ALE_AUTH_CONNECT_CONDITIONS_COUNT;
          conditionIndex++)
      {
         if(HlprGUIDsAreEqual(pConditionKey,
                              ppConditionALEAuthConnect[conditionIndex]))
         {
            isValid = TRUE;

            break;
         }
      }
   }
   else if(*pLayerKey == FWPM_LAYER_ALE_FLOW_ESTABLISHED_V4 ||
           *pLayerKey == FWPM_LAYER_ALE_FLOW_ESTABLISHED_V4_DISCARD ||
           *pLayerKey == FWPM_LAYER_ALE_FLOW_ESTABLISHED_V6 ||
           *pLayerKey == FWPM_LAYER_ALE_FLOW_ESTABLISHED_V6_DISCARD)
   {
      for(UINT32 conditionIndex = 0;
          conditionIndex < ALE_FLOW_ESTABLISHED_CONDITIONS_COUNT;
          conditionIndex++)
      {
         if(HlprGUIDsAreEqual(pConditionKey,
                              ppConditionALEFlowEstablished[conditionIndex]))
         {
            isValid = TRUE;

            break;
         }
      }
   }
   else if(*pLayerKey == FWPM_LAYER_IPSEC_KM_DEMUX_V4 ||
           *pLayerKey == FWPM_LAYER_IPSEC_KM_DEMUX_V6)
   {
      for(UINT32 conditionIndex = 0;
          conditionIndex < IPSEC_KM_DEMUX_CONDITIONS_COUNT;
          conditionIndex++)
      {
         if(HlprGUIDsAreEqual(pConditionKey,
                              ppConditionIPsecKMDemux[conditionIndex]))
         {
            isValid = TRUE;

            break;
         }
      }
   }
   else if(*pLayerKey == FWPM_LAYER_IPSEC_V4 ||
           *pLayerKey == FWPM_LAYER_IPSEC_V6)
   {
      for(UINT32 conditionIndex = 0;
          conditionIndex < IPSEC_CONDITIONS_COUNT;
          conditionIndex++)
      {
         if(HlprGUIDsAreEqual(pConditionKey,
                              ppConditionIPsec[conditionIndex]))
         {
            isValid = TRUE;

            break;
         }
      }
   }
   else if(*pLayerKey == FWPM_LAYER_IKEEXT_V4 ||
           *pLayerKey == FWPM_LAYER_IKEEXT_V6)
   {
      for(UINT32 conditionIndex = 0;
          conditionIndex < IKEEXT_CONDITIONS_COUNT;
          conditionIndex++)
      {
         if(HlprGUIDsAreEqual(pConditionKey,
                              ppConditionIKEExt[conditionIndex]))
         {
            isValid = TRUE;

            break;
         }
      }
   }
   else if(*pLayerKey == FWPM_LAYER_RPC_UM)
   {
      for(UINT32 conditionIndex = 0;
          conditionIndex < RPC_UM_CONDITIONS_COUNT;
          conditionIndex++)
      {
         if(HlprGUIDsAreEqual(pConditionKey,
                              ppConditionRPCUM[conditionIndex]))
         {
            isValid = TRUE;

            break;
         }
      }
   }
   else if(*pLayerKey == FWPM_LAYER_RPC_EPMAP)
   {
      for(UINT32 conditionIndex = 0;
          conditionIndex < RPC_EPMAP_CONDITIONS_COUNT;
          conditionIndex++)
      {
         if(HlprGUIDsAreEqual(pConditionKey,
                              ppConditionRPCEPMap[conditionIndex]))
         {
            isValid = TRUE;

            break;
         }
      }
   }
   else if(*pLayerKey == FWPM_LAYER_RPC_EP_ADD)
   {
      for(UINT32 conditionIndex = 0;
          conditionIndex < RPC_EP_ADD_CONDITIONS_COUNT;
          conditionIndex++)
      {
         if(HlprGUIDsAreEqual(pConditionKey,
                              ppConditionRPCEPAdd[conditionIndex]))
         {
            isValid = TRUE;

            break;
         }
      }
   }
   else if(*pLayerKey == FWPM_LAYER_RPC_PROXY_CONN)
   {
      for(UINT32 conditionIndex = 0;
          conditionIndex < RPC_PROXY_CONN_CONDITIONS_COUNT;
          conditionIndex++)
      {
         if(HlprGUIDsAreEqual(pConditionKey,
                              ppConditionRPCProxyConn[conditionIndex]))
         {
            isValid = TRUE;

            break;
         }
      }
   }
   else if(*pLayerKey == FWPM_LAYER_RPC_PROXY_IF)
   {
      for(UINT32 conditionIndex = 0;
          conditionIndex < RPC_PROXY_IF_CONDITIONS_COUNT;
          conditionIndex++)
      {
         if(HlprGUIDsAreEqual(pConditionKey,
                              ppConditionRPCProxyIf[conditionIndex]))
         {
            isValid = TRUE;

            break;
         }
      }
   }

#if(NTDDI_VERSION >= NTDDI_WIN7)

   else if(*pLayerKey == FWPM_LAYER_NAME_RESOLUTION_CACHE_V4 ||
           *pLayerKey == FWPM_LAYER_NAME_RESOLUTION_CACHE_V6)
   {
      for(UINT32 conditionIndex = 0;
          conditionIndex < NAME_RESOLUTION_CACHE_CONDITIONS_COUNT;
          conditionIndex++)
      {
         if(HlprGUIDsAreEqual(pConditionKey,
                              ppConditionNameResolutionCache[conditionIndex]))
         {
            isValid = TRUE;

            break;
         }
      }
   }
   else if(*pLayerKey == FWPM_LAYER_ALE_RESOURCE_RELEASE_V4 ||
           *pLayerKey == FWPM_LAYER_ALE_RESOURCE_RELEASE_V6)
   {
      for(UINT32 conditionIndex = 0;
          conditionIndex < ALE_RESOURCE_RELEASE_CONDITIONS_COUNT;
          conditionIndex++)
      {
         if(HlprGUIDsAreEqual(pConditionKey,
                              ppConditionALEResourceRelease[conditionIndex]))
         {
            isValid = TRUE;

            break;
         }
      }
   }
   else if(*pLayerKey == FWPM_LAYER_ALE_ENDPOINT_CLOSURE_V4 ||
           *pLayerKey == FWPM_LAYER_ALE_ENDPOINT_CLOSURE_V6)
   {
      for(UINT32 conditionIndex = 0;
          conditionIndex < ALE_ENDPOINT_CLOSURE_CONDITIONS_COUNT;
          conditionIndex++)
      {
         if(HlprGUIDsAreEqual(pConditionKey,
                              ppConditionALEEndpointClosure[conditionIndex]))
         {
            isValid = TRUE;

            break;
         }
      }
   }
   else if(*pLayerKey == FWPM_LAYER_ALE_CONNECT_REDIRECT_V4 ||
           *pLayerKey == FWPM_LAYER_ALE_CONNECT_REDIRECT_V6)
   {
      for(UINT32 conditionIndex = 0;
          conditionIndex < ALE_CONNECT_REDIRECT_CONDITIONS_COUNT;
          conditionIndex++)
      {
         if(HlprGUIDsAreEqual(pConditionKey,
                              ppConditionALEConnectRedirect[conditionIndex]))
         {
            isValid = TRUE;

            break;
         }
      }
   }
   else if(*pLayerKey == FWPM_LAYER_ALE_BIND_REDIRECT_V4 ||
           *pLayerKey == FWPM_LAYER_ALE_BIND_REDIRECT_V6)
   {
      for(UINT32 conditionIndex = 0;
          conditionIndex < ALE_BIND_REDIRECT_CONDITIONS_COUNT;
          conditionIndex++)
      {
         if(HlprGUIDsAreEqual(pConditionKey,
                              ppConditionALEBindRedirect[conditionIndex]))
         {
            isValid = TRUE;

            break;
         }
      }
   }
   else if(*pLayerKey == FWPM_LAYER_STREAM_PACKET_V4 ||
           *pLayerKey == FWPM_LAYER_STREAM_PACKET_V6)
   {
      for(UINT32 conditionIndex = 0;
          conditionIndex < STREAM_PACKET_CONDITIONS_COUNT;
          conditionIndex++)
      {
         if(HlprGUIDsAreEqual(pConditionKey,
                              ppConditionStreamPacket[conditionIndex]))
         {
            isValid = TRUE;

            break;
         }
      }
   }
   else if(*pLayerKey == FWPM_LAYER_KM_AUTHORIZATION)
   {
      for(UINT32 conditionIndex = 0;
          conditionIndex < KM_AUTHORIZATION_CONDITIONS_COUNT;
          conditionIndex++)
      {
         if(HlprGUIDsAreEqual(pConditionKey,
                              ppConditionKMAuthorization[conditionIndex]))
         {
            isValid = TRUE;

            break;
         }
      }
   }

#if(NTDDI_VERSION >= NTDDI_WIN8)

   else if(*pLayerKey == FWPM_LAYER_INBOUND_MAC_FRAME_ETHERNET)
   {
      for(UINT32 conditionIndex = 0;
          conditionIndex < INBOUND_MAC_FRAME_ETHERNET_CONDITIONS_COUNT;
          conditionIndex++)
      {
         if(HlprGUIDsAreEqual(pConditionKey,
                              ppConditionInboundMACFrameEthernet[conditionIndex]))
         {
            isValid = TRUE;

            break;
         }
      }
   }
   else if(*pLayerKey == FWPM_LAYER_OUTBOUND_MAC_FRAME_ETHERNET)
   {
      for(UINT32 conditionIndex = 0;
          conditionIndex < OUTBOUND_MAC_FRAME_ETHERNET_CONDITIONS_COUNT;
          conditionIndex++)
      {
         if(HlprGUIDsAreEqual(pConditionKey,
                              ppConditionOutboundMACFrameEthernet[conditionIndex]))
         {
            isValid = TRUE;

            break;
         }
      }
   }
   else if(*pLayerKey == FWPM_LAYER_INBOUND_MAC_FRAME_NATIVE)
   {
      for(UINT32 conditionIndex = 0;
          conditionIndex < INBOUND_MAC_FRAME_NATIVE_CONDITIONS_COUNT;
          conditionIndex++)
      {
         if(HlprGUIDsAreEqual(pConditionKey,
                              ppConditionInboundMACFrameNative[conditionIndex]))
         {
            isValid = TRUE;

            break;
         }
      }
   }
   else if(*pLayerKey == FWPM_LAYER_OUTBOUND_MAC_FRAME_NATIVE)
   {
      for(UINT32 conditionIndex = 0;
          conditionIndex < OUTBOUND_MAC_FRAME_NATIVE_CONDITIONS_COUNT;
          conditionIndex++)
      {
         if(HlprGUIDsAreEqual(pConditionKey,
                              ppConditionOutboundMACFrameNative[conditionIndex]))
         {
            isValid = TRUE;

            break;
         }
      }
   }
   else if(*pLayerKey == FWPM_LAYER_INGRESS_VSWITCH_ETHERNET)
   {
      for(UINT32 conditionIndex = 0;
          conditionIndex < INGRESS_VSWITCH_ETHERNET_CONDITIONS_COUNT;
          conditionIndex++)
      {
         if(HlprGUIDsAreEqual(pConditionKey,
                              ppConditionIngressVSwitchEthernet[conditionIndex]))
         {
            isValid = TRUE;

            break;
         }
      }
   }
   else if(*pLayerKey == FWPM_LAYER_EGRESS_VSWITCH_ETHERNET)
   {
      for(UINT32 conditionIndex = 0;
          conditionIndex < EGRESS_VSWITCH_ETHERNET_CONDITIONS_COUNT;
          conditionIndex++)
      {
         if(HlprGUIDsAreEqual(pConditionKey,
                              ppConditionEgressVSwitchEthernet[conditionIndex]))
         {
            isValid = TRUE;

            break;
         }
      }
   }
   else if(*pLayerKey == FWPM_LAYER_INGRESS_VSWITCH_TRANSPORT_V4 ||
           *pLayerKey == FWPM_LAYER_INGRESS_VSWITCH_TRANSPORT_V6)
   {
      for(UINT32 conditionIndex = 0;
          conditionIndex < INGRESS_VSWITCH_TRANSPORT_CONDITIONS_COUNT;
          conditionIndex++)
      {
         if(HlprGUIDsAreEqual(pConditionKey,
                              ppConditionIngressVSwitchTransport[conditionIndex]))
         {
            isValid = TRUE;

            break;
         }
      }
   }
   else if(*pLayerKey == FWPM_LAYER_EGRESS_VSWITCH_TRANSPORT_V4 ||
           *pLayerKey == FWPM_LAYER_EGRESS_VSWITCH_TRANSPORT_V6)
   {
      for(UINT32 conditionIndex = 0;
          conditionIndex < EGRESS_VSWITCH_TRANSPORT_CONDITIONS_COUNT;
          conditionIndex++)
      {
         if(HlprGUIDsAreEqual(pConditionKey,
                              ppConditionEgressVSwitchTransport[conditionIndex]))
         {
            isValid = TRUE;

            break;
         }
      }
   }
#if(NTDDI_VERSION >= NTDDI_WINBLUE)

   else if(*pLayerKey == FWPM_LAYER_INBOUND_TRANSPORT_FAST ||
           *pLayerKey == FWPM_LAYER_OUTBOUND_TRANSPORT_FAST ||
           *pLayerKey == FWPM_LAYER_INBOUND_MAC_FRAME_NATIVE_FAST ||
           *pLayerKey == FWPM_LAYER_OUTBOUND_MAC_FRAME_NATIVE_FAST)
      isValid = FALSE;

#endif /// (NTDDI_VERSION >= NTDDI_WINBLUE)
#endif /// (NTDDI_VERSION >= NTDDI_WIN8)
#endif /// (NTDDI_VERSION >= NTDDI_WIN7)

   return isValid;
}

/**
 @helper_function="HlprFwpmFilterEnum"
 
   Purpose:  Wrapper for the FwpmFilterEnum API.                                                <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/AA364058.aspx              <br>
*/
_At_(*pppEntries, _Pre_ _Null_)
_When_(return != NO_ERROR, _At_(*pppEntries, _Post_ _Null_))
_Success_(return == NO_ERROR)
UINT32 HlprFwpmFilterEnum(_In_ const HANDLE engineHandle,
                          _In_ const HANDLE enumHandle,
                          _In_ const UINT32 numEntriesRequested,
                          _Outptr_result_buffer_maybenull_(*pNumEntriesReturned) FWPM_FILTER*** pppEntries,
                          _Out_ UINT32* pNumEntriesReturned)
{
   ASSERT(engineHandle);
   ASSERT(enumHandle);
   ASSERT(numEntriesRequested);
   ASSERT(pppEntries);
   ASSERT(pNumEntriesReturned);

   UINT32 status = NO_ERROR;

   status = FwpmFilterEnum(engineHandle,
                           enumHandle,
                           numEntriesRequested,
                           pppEntries,
                           pNumEntriesReturned);
   if(status != NO_ERROR &&
      status != FWP_E_FILTER_NOT_FOUND &&
      status != FWP_E_NOT_FOUND)
   {
      *pppEntries = 0;

      *pNumEntriesReturned = 0;

      HlprLogError(L"HlprFwpmFilterEnum : FwpmFilterEnum() [status: %#x]",
                   status);
   }

   return status;
}


/**
 @helper_function="HlprFwpmFilterDestroyEnumHandle"
 
   Purpose:  Wrapper for the FwpmFilterDestroyEnumHandle API.                                   <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/AA364055.aspx              <br>
*/
_When_(return != NO_ERROR, _At_(*pEnumHandle, _Post_ _Notnull_))
_When_(return == NO_ERROR, _At_(*pEnumHandle, _Post_ _Null_))
_Success_(return == NO_ERROR)
UINT32 HlprFwpmFilterDestroyEnumHandle(_In_ const HANDLE engineHandle,
                                       _Inout_ HANDLE* pEnumHandle)
{
   ASSERT(engineHandle);
   ASSERT(pEnumHandle);

   UINT32 status = NO_ERROR;

   if(*pEnumHandle)
   {
      status = FwpmFilterDestroyEnumHandle(engineHandle,
                                           *pEnumHandle);
      if(status != NO_ERROR)
      {
         HlprLogError(L"HlprFwpmFilterDestroyEnumHandle : FwpmFilterDestroyEnumHandle() [status: %#x]",
                      status);

         HLPR_BAIL;
      }

      *pEnumHandle = 0;
   }

   HLPR_BAIL_LABEL:

   return status;
}

/**
 @helper_function="HlprFwpmFilterCreateEnumHandle"
 
   Purpose:  Wrapper for the FwpmFilterCreateEnumHandle API.                                    <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/AA364048.aspx              <br>
*/
_When_(return != NO_ERROR, _At_(*pEnumHandle, _Post_ _Null_))
_When_(return == NO_ERROR, _At_(*pEnumHandle, _Post_ _Notnull_))
_Success_(return == NO_ERROR)
UINT32 HlprFwpmFilterCreateEnumHandle(_In_ const HANDLE engineHandle,
                                      _In_opt_ const FWPM_FILTER_ENUM_TEMPLATE* pEnumTemplate,
                                      _Out_ HANDLE* pEnumHandle)
{
   ASSERT(engineHandle);
   ASSERT(pEnumHandle);

   UINT32 status = NO_ERROR;

   status = FwpmFilterCreateEnumHandle(engineHandle,
                                       pEnumTemplate,
                                       pEnumHandle);
   if(status != NO_ERROR)
   {
      *pEnumHandle = 0;

      HlprLogError(L"HlprFwpmFilterCreateEnumHandle : FwpmFilterCreateEnumHandle() [status: %#x]",
                   status);
   }

   return status;
}

/**
 @helper_function="HlprFwpmFilterDeleteByKey"
 
   Purpose:  Wrapper for the FwpmFilterDeleteByKey API.                                         <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/AA364053.aspx              <br>
*/
_Success_(return == NO_ERROR)
UINT32 HlprFwpmFilterDeleteByKey(_In_ const HANDLE engineHandle,
                                 _In_ const GUID* pFilterKey)
{
   ASSERT(engineHandle);
   ASSERT(pFilterKey);

   UINT32 status = NO_ERROR;

   status = FwpmFilterDeleteByKey(engineHandle,
                                  pFilterKey);
   if(status != NO_ERROR)
   {
      if(status != FWP_E_IN_USE &&
         status != FWP_E_BUILTIN_OBJECT &&
         status != FWP_E_FILTER_NOT_FOUND)
         HlprLogError(L"HlprFwpmFilterDeleteByKey : FwpmFilterDeleteByKey() [status: %#x]",
                      status);
      else
      {
         HlprLogInfo(L"HlprFwpmFilterDeleteByKey : FwpmFilterDeleteByKey() [status: %#x]",
                      status);

         status = NO_ERROR;
      }
   }

   return status;
}

/**
 @helper_function="HlprFwpmFilterAdd"
 
   Purpose:  Wrapper for the FwpmFilterAdd API.                                                 <br>
                                                                                                <br>
   Notes:    Filter ID is written to pFilter->filterId.                                         <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/AA364046.aspx              <br>
*/
_Success_(return == NO_ERROR)
UINT32 HlprFwpmFilterAdd(_In_ const HANDLE engineHandle,
                         _Inout_ FWPM_FILTER* pFilter)
{
   ASSERT(engineHandle);
   ASSERT(pFilter);

   UINT32 status = NO_ERROR;

   status = FwpmFilterAdd(engineHandle,
                          pFilter,
                          0,
                          &(pFilter->filterId));
   if(status != NO_ERROR)
      HlprLogError(L"HlprFwpmFilterAdd : FwpmFilterAdd() [status: %#x]",
                   status);

   return status;
}

/**
 @helper_function="HlprFwpmFilterRemoveAll"
 
   Purpose:  Remove all filters associated with the specified provider.                         <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_Success_(return == NO_ERROR)
UINT32 HlprFwpmFilterRemoveAll(_In_opt_ HANDLE* pEngineHandle,
                               _In_opt_ const GUID* pProviderKey)
{
   UINT32  status       = NO_ERROR;
   HANDLE  engineHandle = 0;
   BOOLEAN isLocal      = FALSE;

   if(pEngineHandle &&
      *pEngineHandle)
      engineHandle = *pEngineHandle;
   else
   {
      status = HlprFwpmEngineOpen(&engineHandle);
      HLPR_BAIL_ON_FAILURE(status);

      isLocal = TRUE;
   }

   for(UINT32 layerIndex = 0;
       layerIndex < TOTAL_LAYER_COUNT;
       layerIndex++)
   {
      HANDLE                    enumHandle   = 0;
      FWPM_FILTER**             ppFilters    = 0;
      UINT32                    filterCount  = 0;
      FWPM_FILTER_ENUM_TEMPLATE enumTemplate = {0};

      enumTemplate.providerKey = (GUID*)pProviderKey;
      enumTemplate.layerKey    = *(ppLayerKeyArray[layerIndex]);
      enumTemplate.enumType    = FWP_FILTER_ENUM_FULLY_CONTAINED;
      enumTemplate.flags       = FWP_FILTER_ENUM_FLAG_INCLUDE_BOOTTIME |
                                 FWP_FILTER_ENUM_FLAG_INCLUDE_DISABLED;
      enumTemplate.actionMask  = 0xFFFFFFFF;

      status = HlprFwpmFilterCreateEnumHandle(engineHandle,
                                              &enumTemplate,
                                              &enumHandle);
      HLPR_BAIL_ON_FAILURE(status);

      status = HlprFwpmFilterEnum(engineHandle,
                                  enumHandle,
                                  0xFFFFFFFF,
                                  &ppFilters,
                                  &filterCount);
      if(ppFilters &&
         filterCount)
      {
         for(UINT32 filterIndex = 0;
             filterIndex < filterCount;
             filterIndex++)
         {
            HlprFwpmFilterDeleteByKey(engineHandle,
                                      &(ppFilters[filterIndex]->filterKey));
         }

         FwpmFreeMemory((VOID**)&ppFilters);
      }

      HlprFwpmFilterDestroyEnumHandle(engineHandle,
                                      &enumHandle);
   }

   HLPR_BAIL_LABEL:

   if(engineHandle &&
      isLocal)
      HlprFwpmEngineClose(&engineHandle);

   return status;
}

/**
 @helper_function="HlprFwpmFilterPurge"
 
   Purpose:  Cleanup an FWPM_FILTER.                                                            <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/AA364265.aspx              <br>
*/
VOID HlprFwpmFilterPurge(_Inout_ FWPM_FILTER* pFilter)
{
   ASSERT(pFilter);

   if(pFilter->numFilterConditions)
      HlprFwpmFilterConditionDestroy(&(pFilter->filterCondition),
                                     pFilter->numFilterConditions);

   ZeroMemory(pFilter,
              sizeof(FWPM_FILTER));

   return;
}

/**
 @helper_function="HlprFwpmFilterDestroy"
 
   Purpose:  Cleanup and free an (array of) FWPM_FILTER.                                        <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/AA364265.aspx              <br>
*/
_At_(*ppFilters, _Post_ _Null_)
_Success_(*ppFilters == 0)
VOID HlprFwpmFilterDestroy(_Inout_updates_all_(numFilters) FWPM_FILTER** ppFilters,
                           _In_ const UINT32 numFilters)                            /* 1 */
{
   ASSERT(ppFilters);
   ASSERT(numFilters);

   for(UINT32 filterIndex = 0;
       filterIndex < numFilters;
       filterIndex++)
   {
      HlprFwpmFilterPurge(&((*ppFilters)[filterIndex]));
   }

   if(numFilters > 1)
   {
      HLPR_DELETE_ARRAY(*ppFilters);
   }
   else
   {
      HLPR_DELETE(*ppFilters);
   }

   return;
}

/**
 @helper_function="HlprFwpmFilterCreate"
 
   Purpose:  Allocate an (array of) FWPM_FILTER.                                                <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/AA364265.aspx              <br>
*/
_Success_(return == NO_ERROR)
UINT32 HlprFwpmFilterCreate(_Inout_updates_all_(numFilters) FWPM_FILTER** ppFilters,
                            _In_ const UINT32 numFilters)                            /* 1 */
{
   ASSERT(ppFilters);
   ASSERT(numFilters);

   UINT32 status = NO_ERROR;

   if(numFilters > 1)
   {
      HLPR_NEW_ARRAY(*ppFilters,
                     FWPM_FILTER,
                     numFilters);
   }
   else
   {
      HLPR_NEW(*ppFilters,
               FWPM_FILTER);
   }

   return status;
}
