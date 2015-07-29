////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2014 Microsoft Corporation.  All Rights Reserved.
//
//   Module Name:
//      ClassifyFunctions_BasicPacketExaminationCallouts.cpp
//
//   Abstract:
//      This module contains WFP Classify functions for examining indicated NET_BUFFER_LISTS.
//
//   Naming Convention:
//
//      <Module><Scenario>
//  
//      i.e.
//
//       ClassifyBasicPacketExamination
//
//       <Module>
//          Classify               - Function is an FWPS_CALLOUT_CLASSIFY_FN.
//       <Scenario>
//          BasicPacketExamination - Function demonstates examining classified packets.
//
//   Private Functions:
//
//   Public Functions:
//      ClassifyBasicPacketExamination(),
//
//   Author:
//      Dusty Harper      (DHarper)
//
//   Revision History:
//
//      [ Month ][Day] [Year] - [Revision]-[ Comments ]
//      May       01,   2010  -     1.0   -  Creation
//      December  13,   2013  -     1.1   -  Enhance function declaration for IntelliSense, enhance 
//                                              traces, add support for discard layers, and support 
//                                              tracing all of the classifyFn parameters.
//
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "Framework_WFPSamplerCalloutDriver.h"                  /// .
#include "ClassifyFunctions_BasicPacketExaminationCallouts.tmh" /// $(OBJ_PATH)\$(O)\

#define MAX_STRING_SIZE  2048

/**
 @private_function="LogFlags"
 
   Purpose:  Logs each of the flags from the FWPM_CONDITION_FLAGS value.                        <br>
                                                                                                <br>
   Notes:    Uses ETW Tracing for the logging.                                                  <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/AA364002.aspx              <br>
*/
VOID LogFlags(_In_ UINT32 flags)
{
   if(flags & FWP_CONDITION_FLAG_IS_LOOPBACK)
      DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                 DPFLTR_INFO_LEVEL,
                 "\t\t\t\t\t\tFWP_CONDITION_FLAG_IS_LOOPBACK");

   if(flags & FWP_CONDITION_FLAG_IS_IPSEC_SECURED)
      DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                 DPFLTR_INFO_LEVEL,
                 "\t\t\t\t\t\tFWP_CONDITION_FLAG_IS_IPSEC_SECURED");

   if(flags & FWP_CONDITION_FLAG_IS_REAUTHORIZE)
      DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                 DPFLTR_INFO_LEVEL,
                 "\t\t\t\t\t\tFWP_CONDITION_FLAG_IS_REAUTHORIZE");

   if(flags & FWP_CONDITION_FLAG_IS_WILDCARD_BIND)
      DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                 DPFLTR_INFO_LEVEL,
                 "\t\t\t\t\t\tFWP_CONDITION_FLAG_IS_WILDCARD_BIND");

   if(flags & FWP_CONDITION_FLAG_IS_RAW_ENDPOINT)
      DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                 DPFLTR_INFO_LEVEL,
                 "\t\t\t\t\t\tFWP_CONDITION_FLAG_IS_RAW_ENDPOINT");

   if(flags & FWP_CONDITION_FLAG_IS_FRAGMENT)
      DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                 DPFLTR_INFO_LEVEL,
                 "\t\t\t\t\t\tFWP_CONDITION_FLAG_IS_FRAGMENT");

   if(flags & FWP_CONDITION_FLAG_IS_FRAGMENT_GROUP)
      DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                 DPFLTR_INFO_LEVEL,
                 "\t\t\t\t\t\tFWP_CONDITION_FLAG_IS_FRAGMENT_GROUP");

   if(flags & FWP_CONDITION_FLAG_IS_IPSEC_NATT_RECLASSIFY)
      DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                 DPFLTR_INFO_LEVEL,
                 "\t\t\t\t\t\tFWP_CONDITION_FLAG_IS_IPSEC_NATT_RECLASSIFY");

   if(flags & FWP_CONDITION_FLAG_REQUIRES_ALE_CLASSIFY)
      DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                 DPFLTR_INFO_LEVEL,
                 "\t\t\t\t\t\tFWP_CONDITION_FLAG_REQUIRES_ALE_CLASSIFY");

   if(flags & FWP_CONDITION_FLAG_IS_IMPLICIT_BIND)
      DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                 DPFLTR_INFO_LEVEL,
                 "\t\t\t\t\t\tFWP_CONDITION_FLAG_IS_IMPLICIT_BIND");


#if(NTDDI_VERSION >= NTDDI_VISTASP1)

   if(flags & FWP_CONDITION_FLAG_IS_REASSEMBLED)
      DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                 DPFLTR_INFO_LEVEL,
                 "\t\t\t\t\t\tFWP_CONDITION_FLAG_IS_REASSEMBLED");


#if(NTDDI_VERSION >= NTDDI_WIN7)

   if(flags & FWP_CONDITION_FLAG_IS_NAME_APP_SPECIFIED)
      DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                 DPFLTR_INFO_LEVEL,
                 "\t\t\t\t\t\tFWP_CONDITION_FLAG_IS_NAME_APP_SPECIFIED");

   if(flags & FWP_CONDITION_FLAG_IS_PROMISCUOUS)
      DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                 DPFLTR_INFO_LEVEL,
                 "\t\t\t\t\t\tFWP_CONDITION_FLAG_IS_PROMISCUOUS");

   if(flags & FWP_CONDITION_FLAG_IS_AUTH_FW)
      DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                 DPFLTR_INFO_LEVEL,
                 "\t\t\t\t\t\tFWP_CONDITION_FLAG_IS_AUTH_FW");

   if(flags & FWP_CONDITION_FLAG_IS_RECLASSIFY)
      DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                 DPFLTR_INFO_LEVEL,
                 "\t\t\t\t\t\tFWP_CONDITION_FLAG_IS_RECLASSIFY");

   if(flags & FWP_CONDITION_FLAG_IS_OUTBOUND_PASS_THRU)
      DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                 DPFLTR_INFO_LEVEL,
                 "\t\t\t\t\t\tFWP_CONDITION_FLAG_IS_OUTBOUND_PASS_THRU");

   if(flags & FWP_CONDITION_FLAG_IS_INBOUND_PASS_THRU)
      DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                 DPFLTR_INFO_LEVEL,
                 "\t\t\t\t\t\tFWP_CONDITION_FLAG_IS_INBOUND_PASS_THRU");

   if(flags & FWP_CONDITION_FLAG_IS_CONNECTION_REDIRECTED)
      DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                 DPFLTR_INFO_LEVEL,
                 "\t\t\t\t\t\tFWP_CONDITION_FLAG_IS_CONNECTION_REDIRECTED");


#if(NTDDI_VERSION >= NTDDI_WIN8)

   if(flags & FWP_CONDITION_FLAG_IS_PROXY_CONNECTION)
      DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                 DPFLTR_INFO_LEVEL,
                 "\t\t\t\t\t\tFWP_CONDITION_FLAG_IS_PROXY_CONNECTION");

   if(flags & FWP_CONDITION_FLAG_IS_APPCONTAINER_LOOPBACK)
      DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                 DPFLTR_INFO_LEVEL,
                 "\t\t\t\t\t\tFWP_CONDITION_FLAG_IS_APPCONTAINER_LOOPBACK");

   if(flags & FWP_CONDITION_FLAG_IS_NON_APPCONTAINER_LOOPBACK)
      DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                 DPFLTR_INFO_LEVEL,
                 "\t\t\t\t\t\tFWP_CONDITION_FLAG_IS_NON_APPCONTAINER_LOOPBACK");

   if(flags & FWP_CONDITION_FLAG_IS_RESERVED)
      DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                 DPFLTR_INFO_LEVEL,
                 "\t\t\t\t\t\tFWP_CONDITION_FLAG_IS_RESERVED");

   if(flags & FWP_CONDITION_FLAG_IS_HONORING_POLICY_AUTHORIZE)
      DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                 DPFLTR_INFO_LEVEL,
                 "\t\t\t\t\t\tFWP_CONDITION_FLAG_IS_HONORING_POLICY_AUTHORIZE");

#endif /// (NTDDI_VERSION >= NTDDI_WIN8)
#endif /// (NTDDI_VERSION >= NTDDI_WIN7)
#endif /// (NTDDI_VERSION >= NTDDI_VISTASP1)

   return;
}

#if(NTDDI_VERSION >= NTDDI_WIN8)

/**
 @private_function="LogL2Flags"
 
   Purpose:  Logs each of the flags from the FWPM_CONDITION_FLAGS value.                        <br>
                                                                                                <br>
   Notes:    Uses ETW Tracing for the logging.                                                  <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/AA364002.aspx              <br>
*/
VOID LogL2Flags(_In_ UINT32 flags)
{

   if(flags & FWP_CONDITION_L2_IS_NATIVE_ETHERNET)
      DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                 DPFLTR_INFO_LEVEL,
                 "\t\t\t\t\t\tFWP_CONDITION_L2_IS_NATIVE_ETHERNET");

   if(flags & FWP_CONDITION_L2_IS_WIFI)
      DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                 DPFLTR_INFO_LEVEL,
                 "\t\t\t\t\t\tFWP_CONDITION_L2_IS_WIFI");

   if(flags & FWP_CONDITION_L2_IS_MOBILE_BROADBAND)
      DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                 DPFLTR_INFO_LEVEL,
                 "\t\t\t\t\t\tFWP_CONDITION_L2_IS_MOBILE_BROADBAND");

   if(flags & FWP_CONDITION_L2_IS_WIFI_DIRECT_DATA)
      DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                 DPFLTR_INFO_LEVEL,
                 "\t\t\t\t\t\tFWP_CONDITION_L2_IS_WIFI_DIRECT_DATA");

   if(flags & FWP_CONDITION_L2_IS_VM2VM)
      DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                 DPFLTR_INFO_LEVEL,
                 "\t\t\t\t\t\tFWP_CONDITION_L2_IS_VM2VM");

   if(flags & FWP_CONDITION_L2_IS_MALFORMED_PACKET)
      DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                 DPFLTR_INFO_LEVEL,
                 "\t\t\t\t\t\tFWP_CONDITION_L2_IS_MALFORMED_PACKET");

   if(flags & FWP_CONDITION_L2_IS_IP_FRAGMENT_GROUP)
      DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                 DPFLTR_INFO_LEVEL,
                 "\t\t\t\t\t\tFWP_CONDITION_L2_IS_IP_FRAGMENT_GROUP");

   if(flags & FWP_CONDITION_L2_IF_CONNECTOR_PRESENT)
      DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                 DPFLTR_INFO_LEVEL,
                 "\t\t\t\t\t\tFWP_CONDITION_L2_IF_CONNECTOR_PRESENT");
   return;
}

#endif /// (NTDDI_VERSION >= NTDDI_WIN8)

/**
 @private_function="LogValue"
 
   Purpose:  Logs each of the FWPS_INCOMING_VALUE from the FWPS_INCOMING_VALUES in classifyFn.  <br>
                                                                                                <br>
   Notes:    Uses ETW Tracing for the logging.                                                  <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF552401.aspx             <br>
*/
VOID LogValue(_In_ const FWP_VALUE* pValue,
              _In_ BOOLEAN shouldFormat = FALSE)
{
   if(pValue)
   {
      NTSTATUS status  = STATUS_SUCCESS;
      PSTR     pString = 0;

      HLPR_NEW_ARRAY(pString,
                     CHAR,
                     MAX_STRING_SIZE,
                     WFPSAMPLER_CALLOUT_DRIVER_TAG);
      HLPR_BAIL_ON_ALLOC_FAILURE(pString,
                                 status);

      switch(pValue->type)
      {
         case FWP_EMPTY:
         {
            status = RtlStringCchPrintfA(pString,
                                         MAX_STRING_SIZE,
                                         "\t\t\t\ttype: FWP_EMPTY\n"
                                         "\t\t\t\t\tuint64: %#p",
                                         pValue->uint64);

            break;
         }
         case FWP_UINT8:
         {
            status = RtlStringCchPrintfA(pString,
                                         MAX_STRING_SIZE,
                                         "\t\t\t\ttype: FWP_UINT8\n"
                                         "\t\t\t\t\tuint8: %#x",
                                         pValue->uint8);

            break;
         }
         case FWP_UINT16:
         {
            status = RtlStringCchPrintfA(pString,
                                         MAX_STRING_SIZE,
                                         "\t\t\t\ttype: FWP_UINT16\n"
                                         "\t\t\t\t\tuint16: %#x",
                                         pValue->uint16);

            break;
         }
         case FWP_UINT32:
         {
            switch(shouldFormat)
            {
               case ADDRESS_VALUE:
               {
                  BYTE pBytes[4] = {0};

                  RtlCopyMemory(pBytes,
                                &(pValue->uint32),
                                4);

                  status = RtlStringCchPrintfA(pString,
                                               MAX_STRING_SIZE,
                                               "\t\t\t\ttype: FWP_UINT32\n"
                                               "\t\t\t\t\tuint32: %d.%d.%d.%d",
                                               pBytes[3],
                                               pBytes[2],
                                               pBytes[1],
                                               pBytes[0]);

                  break;
               }
               case FLAGS_VALUE:
               {
                  status = RtlStringCchPrintfA(pString,
                                               MAX_STRING_SIZE,
                                               "\t\t\t\ttype: FWP_UINT32\n"
                                               "\t\t\t\t\tuint32: %#x",
                                               pValue->uint32);

                  LogFlags(pValue->uint32);

                  break;
               }

#if(NTDDI_VERSION >= NTDDI_WIN8)

               case L2_FLAGS_VALUE:
               {
                  status = RtlStringCchPrintfA(pString,
                                               MAX_STRING_SIZE,
                                               "\t\t\t\ttype: FWP_UINT32\n"
                                               "\t\t\t\t\tuint32: %#x",
                                               pValue->uint32);

                  LogL2Flags(pValue->uint32);

                  break;
               }

#endif /// (NTDDI_VERSION >= NTDDI_WIN8)

               default:
                  status = RtlStringCchPrintfA(pString,
                                               MAX_STRING_SIZE,
                                               "\t\t\t\ttype: FWP_UINT32\n"
                                               "\t\t\t\t\tuint32: %#x",
                                               pValue->uint32);
            }

            break;
         }
         case FWP_UINT64:
         {
            status = RtlStringCchPrintfA(pString,
                                         MAX_STRING_SIZE,
                                         "\t\t\t\ttype: FWP_UINT64\n"
                                         "\t\t\t\t\tuint64: %#I64x",
                                         *(pValue->uint64));

            break;
         }
         case FWP_INT8:
         {
            status = RtlStringCchPrintfA(pString,
                                         MAX_STRING_SIZE,
                                         "\t\t\t\ttype: FWP_INT8\n"
                                         "\t\t\t\t\tint8: %#x",
                                         pValue->int8);

            break;
         }
         case FWP_INT16:
         {
            status = RtlStringCchPrintfA(pString,
                                         MAX_STRING_SIZE,
                                         "\t\t\t\ttype: FWP_INT16\n"
                                         "\t\t\t\t\tint16: %#x",
                                         pValue->int16);

            break;
         }
         case FWP_INT32:
         {
            status = RtlStringCchPrintfA(pString,
                                         MAX_STRING_SIZE,
                                         "\t\t\t\ttype: FWP_INT32\n"
                                         "\t\t\t\t\tint32: %#x",
                                         pValue->int32);

            break;
         }
         case FWP_INT64:
         {
            status = RtlStringCchPrintfA(pString,
                                         MAX_STRING_SIZE,
                                         "\t\t\t\ttype: FWP_INT64\n"
                                         "\t\t\t\t\tint64: %#I64x",
                                         *(pValue->int64));

            break;
         }
         case FWP_FLOAT:
         {
            /// Using uint32 in place of float as floats are unsafe in kernel.
            /// This is safe as its part of a union.
            status = RtlStringCchPrintfA(pString,
                                         MAX_STRING_SIZE,
                                         "\t\t\t\ttype: FWP_FLOAT\n"
                                         "\t\t\t\t\tfloat32: %#x",
                                         pValue->uint32);

            break;
         }
         case FWP_DOUBLE:
         {
            /// Using uint64 in place of double as doubles are unsafe in kernel.
            /// This is safe as its part of a union.
            status = RtlStringCchPrintfA(pString,
                                         MAX_STRING_SIZE,
                                         "\t\t\t\ttype: FWP_DOUBLE\n"
                                         "\t\t\t\t\tdouble64: %#I64x",
                                         *(pValue->uint64));

            break;
         }
         case FWP_BYTE_ARRAY16_TYPE:
         {
            status = RtlStringCchPrintfA(pString,
                                         MAX_STRING_SIZE,
                                         "\t\t\t\ttype: FWP_BYTE_ARRAY16_TYPE\n"
                                         "\t\t\t\t\tbyteArray16: %02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x",
                                         pValue->byteArray16->byteArray16[0],
                                         pValue->byteArray16->byteArray16[1],
                                         pValue->byteArray16->byteArray16[2],
                                         pValue->byteArray16->byteArray16[3],
                                         pValue->byteArray16->byteArray16[4],
                                         pValue->byteArray16->byteArray16[5],
                                         pValue->byteArray16->byteArray16[6],
                                         pValue->byteArray16->byteArray16[7],
                                         pValue->byteArray16->byteArray16[8],
                                         pValue->byteArray16->byteArray16[9],
                                         pValue->byteArray16->byteArray16[10],
                                         pValue->byteArray16->byteArray16[11],
                                         pValue->byteArray16->byteArray16[12],
                                         pValue->byteArray16->byteArray16[13],
                                         pValue->byteArray16->byteArray16[14],
                                         pValue->byteArray16->byteArray16[15]);

            break;
         }
         case FWP_BYTE_BLOB_TYPE:
         {
            if(shouldFormat == UNICODE_STRING_VALUE &&
               KeGetCurrentIrql == PASSIVE_LEVEL)
               status = RtlStringCchPrintfA(pString,
                                            MAX_STRING_SIZE,
                                            "\t\t\t\ttype: FWP_BYTE_BLOB_TYPE\n"
                                            "\t\t\t\t\tbyteBlob:\n"
                                            "\t\t\t\t\t\tsize: %d\n"
                                            "\t\t\t\t\t\tdata: %S",
                                            pValue->byteBlob->size,
                                            (PWSTR)(pValue->byteBlob->data));
            else
               status = RtlStringCchPrintfA(pString,
                                            MAX_STRING_SIZE,
                                            "\t\t\t\ttype: FWP_BYTE_BLOB_TYPE\n"
                                            "\t\t\t\t\tbyteBlob:\n"
                                            "\t\t\t\t\t\tsize: %d\n"
                                            "\t\t\t\t\t\tdata: %#p",
                                            pValue->byteBlob->size,
                                            pValue->byteBlob->data);

            break;
         }
         case FWP_SID:
         {
            status = RtlStringCchPrintfA(pString,
                                         MAX_STRING_SIZE,
                                         "\t\t\t\ttype: FWP_SID\n"
                                         "\t\t\t\t\tsid: %#p",
                                         pValue->sid);

            break;
         }
         case FWP_SECURITY_DESCRIPTOR_TYPE:
         {
            status = RtlStringCchPrintfA(pString,
                                         MAX_STRING_SIZE,
                                         "\t\t\t\ttype: FWP_SECURITY_DESCRIPTOR_TYPE\n"
                                         "\t\t\t\t\tsd:\n"
                                         "\t\t\t\t\t\tsize: %d\n"
                                         "\t\t\t\t\t\tdata: %#p",
                                         pValue->sd->size,
                                         pValue->sd->data);
            break;
         }
         case FWP_TOKEN_INFORMATION_TYPE:
         {
            status = RtlStringCchPrintfA(pString,
                                         MAX_STRING_SIZE,
                                         "\t\t\t\ttype: FWP_TOKEN_INFORMATION_TYPE\n"
                                         "\t\t\t\t\ttokenInformation: %#p",
                                         pValue->tokenInformation);

            break;
         }
         case FWP_TOKEN_ACCESS_INFORMATION_TYPE:
         {
            status = RtlStringCchPrintfA(pString,
                                         MAX_STRING_SIZE,
                                         "\t\t\t\ttype: FWP_TOKEN_ACCESS_INFORMATION_TYPE\n"
                                         "\t\t\t\t\ttokenAccessInformation:\n"
                                         "\t\t\t\t\t\tsize: %d\n"
                                         "\t\t\t\t\t\tdata: %#p",
                                         pValue->tokenAccessInformation->size,
                                         pValue->tokenAccessInformation->data);
            break;
         }
         case FWP_UNICODE_STRING_TYPE:
         {
            status = RtlStringCchPrintfA(pString,
                                         MAX_STRING_SIZE,
                                         "\t\t\t\ttype: FWP_UNICODE_STRING_TYPE\n"
                                         "\t\t\t\t\tunicodeString: %S",
                                         pValue->unicodeString);

            break;
         }
         case FWP_BYTE_ARRAY6_TYPE:
         {
            status = RtlStringCchPrintfA(pString,
                                         MAX_STRING_SIZE,
                                         "\t\t\t\ttype: FWP_BYTE_ARRAY6_TYPE\n"
                                         "\t\t\t\t\tbyteArray6: %02x:%02x:%02x:%02x:%02x:%02x",
                                         pValue->byteArray6->byteArray6[0],
                                         pValue->byteArray6->byteArray6[1],
                                         pValue->byteArray6->byteArray6[2],
                                         pValue->byteArray6->byteArray6[3],
                                         pValue->byteArray6->byteArray6[4],
                                         pValue->byteArray6->byteArray6[5]);

            break;
         }
      }

      if(status == STATUS_SUCCESS)
         DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                    DPFLTR_INFO_LEVEL,
                    "%s",
                    pString);

      HLPR_BAIL_LABEL:

      HLPR_DELETE_ARRAY(pString,
                        WFPSAMPLER_CALLOUT_DRIVER_TAG);
   }

   return;
}

/**
 @private_function="LogValues"
 
   Purpose:  Logs each of the FWPS_INCOMING_VALUE from the FWPS_INCOMING_VALUES in classifyFn.  <br>
                                                                                                <br>
   Notes:    Uses ETW Tracing for the logging.                                                  <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF552401.aspx             <br>
*/
VOID LogValues(_In_reads_(numValues) const FWPS_INCOMING_VALUE* pValues,
               _In_ UINT32 numValues,
               _In_ UINT32 layerId)
{
   if(pValues)
   {
      DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                 DPFLTR_INFO_LEVEL,
                 "\t\tincomingValue:\n");

      for(UINT32 index = 0;
          index < numValues;
          index++)
      {
         BOOLEAN shouldFormat = FALSE;

         DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                    DPFLTR_INFO_LEVEL,
                    "\t\t\tvalue: %d : %s\n",
                    index,
                    KrnlHlprFwpValueGetStringForFwpsIncomingValue(layerId,
                                                                  index,
                                                                  &shouldFormat));

         LogValue(&(pValues[index].value),
                  shouldFormat);
      }
   }

   return;
}

/**
 @private_function="LogClassifyValues"
 
   Purpose:  Logs the FWPS_INCOMING_VALUES from the classifyFn.                                 <br>
                                                                                                <br>
   Notes:    Uses ETW Tracing for the logging.                                                  <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF552401.aspx             <br>
*/
VOID LogClassifyValues(_In_ const FWPS_INCOMING_VALUES* pClassifyValues)
{
   if(pClassifyValues)
   {
      NTSTATUS status  = STATUS_SUCCESS;
      PSTR     pString = 0;

      HLPR_NEW_ARRAY(pString,
                     CHAR,
                     MAX_STRING_SIZE,
                     WFPSAMPLER_CALLOUT_DRIVER_TAG);
      HLPR_BAIL_ON_ALLOC_FAILURE(pString,
                                 status);

      status = RtlStringCchPrintfA(pString,
                                   MAX_STRING_SIZE,
                                   "\tClassifyValues:\n"
                                   "\t\tlayerId: %d : %s\n"
                                   "\t\tvalueCount: %d",
                                   pClassifyValues->layerId,
                                   KrnlHlprFwpsLayerIDToString(pClassifyValues->layerId),
                                   pClassifyValues->valueCount);
      if(status == STATUS_SUCCESS)
         DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                    DPFLTR_INFO_LEVEL,
                    "%s",
                    pString);

      LogValues(pClassifyValues->incomingValue,
                pClassifyValues->valueCount,
                pClassifyValues->layerId);

      HLPR_BAIL_LABEL:

      HLPR_DELETE_ARRAY(pString,
                        WFPSAMPLER_CALLOUT_DRIVER_TAG);
   }

   return;
}

/**
 @private_function="LogMetadata"
 
   Purpose:  Logs the FWPS_INCOMING_METADATA_VALUES from the classifyFn.                        <br>
                                                                                                <br>
   Notes:    Uses ETW Tracing for the logging.                                                  <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF552397.aspx             <br>
*/
VOID LogMetadata(_In_ const FWPS_INCOMING_METADATA_VALUES* pMetadata)
{
   if(pMetadata)
   {
      NTSTATUS status  = STATUS_SUCCESS;
      PSTR     pString = 0;

      HLPR_NEW_ARRAY(pString,
                     CHAR,
                     MAX_STRING_SIZE,
                     WFPSAMPLER_CALLOUT_DRIVER_TAG);
      HLPR_BAIL_ON_ALLOC_FAILURE(pString,
                                 status);

      status = RtlStringCchPrintfA(pString,
                                   MAX_STRING_SIZE,
                                   "\tMetadata:\n"
                                   "\t\tCurrentMetadataValues: %#x",
                                   pMetadata->currentMetadataValues);
      if(status == STATUS_SUCCESS)
      {
         DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                    DPFLTR_INFO_LEVEL,
                    "%s",
                    pString);

         RtlZeroMemory(pString,
                       MAX_STRING_SIZE);
      }

      if(FWPS_IS_METADATA_FIELD_PRESENT(pMetadata,
                                        FWPS_METADATA_FIELD_DISCARD_REASON))
      {
         status = RtlStringCchPrintfA(pString,
                                      MAX_STRING_SIZE,
                                      "\t\t\tdiscardMetadata:\n"
                                      "\t\t\t\tdiscardModule: %d\n"
                                      "\t\t\t\tdiscardReason: %d\n"
                                      "\t\t\t\tfilterId: %I64d",
                                      pMetadata->discardMetadata.discardModule,
                                      pMetadata->discardMetadata.discardReason,
                                      pMetadata->discardMetadata.filterId);
         if(status == STATUS_SUCCESS)
         {
            DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                       DPFLTR_INFO_LEVEL,
                       "%s",
                       pString);

            RtlZeroMemory(pString,
                          MAX_STRING_SIZE);
         }
      }

      if(FWPS_IS_METADATA_FIELD_PRESENT(pMetadata,
                                        FWPS_METADATA_FIELD_FLOW_HANDLE))
         DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                    DPFLTR_INFO_LEVEL,
                    "\t\t\tflowHandle: %#I64x\n",
                    pMetadata->flowHandle);

      if(FWPS_IS_METADATA_FIELD_PRESENT(pMetadata,
                                        FWPS_METADATA_FIELD_IP_HEADER_SIZE))
         DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                    DPFLTR_INFO_LEVEL,
                    "\t\t\tipHeaderSize: %d\n",
                    pMetadata->ipHeaderSize);

      if(FWPS_IS_METADATA_FIELD_PRESENT(pMetadata,
                                        FWPS_METADATA_FIELD_PROCESS_PATH))
         DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                    DPFLTR_INFO_LEVEL,
                    "\t\t\tprocessPath: %s\n",
                    (PSTR)(pMetadata->processPath->data));

      if(FWPS_IS_METADATA_FIELD_PRESENT(pMetadata,
                                        FWPS_METADATA_FIELD_TOKEN))
         DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                    DPFLTR_INFO_LEVEL,
                    "\t\t\ttoken: %#I64x\n",
                    pMetadata->token);

      if(FWPS_IS_METADATA_FIELD_PRESENT(pMetadata,
                                        FWPS_METADATA_FIELD_PROCESS_ID))
         DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                    DPFLTR_INFO_LEVEL,
                    "\t\t\tprocessId: %#I64x\n",
                    pMetadata->processId);

      if(FWPS_IS_METADATA_FIELD_PRESENT(pMetadata,
                                        FWPS_METADATA_FIELD_SYSTEM_FLAGS))
         DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                    DPFLTR_INFO_LEVEL,
                    "\t\t\tflags: %#x\n",
                    pMetadata->flags);

      if(FWPS_IS_METADATA_FIELD_PRESENT(pMetadata,
                                        FWPS_METADATA_FIELD_RESERVED))
         DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                    DPFLTR_INFO_LEVEL,
                    "\t\t\treserved: %#I64x\n",
                    pMetadata->reserved);

      if(FWPS_IS_METADATA_FIELD_PRESENT(pMetadata,
                                        FWPS_METADATA_FIELD_SOURCE_INTERFACE_INDEX))
         DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                    DPFLTR_INFO_LEVEL,
                    "\t\t\tsourceInterfaceIndex: %d\n",
                    pMetadata->sourceInterfaceIndex);

      if(FWPS_IS_METADATA_FIELD_PRESENT(pMetadata,
                                        FWPS_METADATA_FIELD_DESTINATION_INTERFACE_INDEX))
         DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                    DPFLTR_INFO_LEVEL,
                    "\t\t\tdestinationInterfaceIndex: %d\n",
                    pMetadata->destinationInterfaceIndex);

      if(FWPS_IS_METADATA_FIELD_PRESENT(pMetadata,
                                        FWPS_METADATA_FIELD_TRANSPORT_HEADER_SIZE))
         DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                    DPFLTR_INFO_LEVEL,
                    "\t\t\ttransportHeaderSize: %d\n",
                    pMetadata->transportHeaderSize);

      if(FWPS_IS_METADATA_FIELD_PRESENT(pMetadata,
                                        FWPS_METADATA_FIELD_COMPARTMENT_ID))
         DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                    DPFLTR_INFO_LEVEL,
                    "\t\t\tcompartmentId: %d\n",
                    pMetadata->compartmentId);

      if(FWPS_IS_METADATA_FIELD_PRESENT(pMetadata,
                                        FWPS_METADATA_FIELD_FRAGMENT_DATA))
      {
         status = RtlStringCchPrintfA(pString,
                                      MAX_STRING_SIZE,
                                      "\t\t\tfragmentMetadata:\n"
                                      "\t\t\t\tfragmentIdentification: %#x\n"
                                      "\t\t\t\tfragmentOffset: %d\n"
                                      "\t\t\t\tfragmentLength: %d",
                                      pMetadata->fragmentMetadata.fragmentIdentification,
                                      pMetadata->fragmentMetadata.fragmentOffset,
                                      pMetadata->fragmentMetadata.fragmentLength);
         if(status == STATUS_SUCCESS)
         {
            DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                       DPFLTR_INFO_LEVEL,
                       "%s",
                       pString);

            RtlZeroMemory(pString,
                          MAX_STRING_SIZE);
         }
      }

      if(FWPS_IS_METADATA_FIELD_PRESENT(pMetadata,
                                        FWPS_METADATA_FIELD_PATH_MTU))
         DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                    DPFLTR_INFO_LEVEL,
                    "\t\t\tpathMtu: %d\n",
                    pMetadata->pathMtu);

      if(FWPS_IS_METADATA_FIELD_PRESENT(pMetadata,
                                        FWPS_METADATA_FIELD_COMPLETION_HANDLE))
         DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                    DPFLTR_INFO_LEVEL,
                    "\t\t\tcompletionHandle: %#I64x\n",
                    (UINT64)pMetadata->completionHandle);

      if(FWPS_IS_METADATA_FIELD_PRESENT(pMetadata,
                                        FWPS_METADATA_FIELD_TRANSPORT_ENDPOINT_HANDLE))
         DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                    DPFLTR_INFO_LEVEL,
                    "\t\t\ttransportEndpointHandle: %#I64x\n",
                    pMetadata->transportEndpointHandle);

      if(FWPS_IS_METADATA_FIELD_PRESENT(pMetadata,
                                        FWPS_METADATA_FIELD_TRANSPORT_CONTROL_DATA))
      {
         status = RtlStringCchPrintfA(pString,
                                      MAX_STRING_SIZE,
                                      "\t\t\tcontrolData:\n"
                                      "\t\t\t\tcmsg_len: %I64d\n"
                                      "\t\t\t\tcmsg_level: %d\n"
                                      "\t\t\t\tcmsg_type: %d\n"
                                      "\t\t\tcontrolDataLength: %d",
                                      (UINT64)(pMetadata->controlData->cmsg_len),
                                      pMetadata->controlData->cmsg_level,
                                      pMetadata->controlData->cmsg_type,
                                      pMetadata->controlDataLength);
         if(status == STATUS_SUCCESS)
         {
            DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                       DPFLTR_INFO_LEVEL,
                       "%s",
                       pString);

            RtlZeroMemory(pString,
                          MAX_STRING_SIZE);
         }
      }

      if(FWPS_IS_METADATA_FIELD_PRESENT(pMetadata,
                                        FWPS_METADATA_FIELD_REMOTE_SCOPE_ID))
         DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                    DPFLTR_INFO_LEVEL,
                    "\t\t\tremoteScopeId: %#x\n",
                    pMetadata->remoteScopeId.Value);

      if(FWPS_IS_METADATA_FIELD_PRESENT(pMetadata,
                                        FWPS_METADATA_FIELD_PACKET_DIRECTION))
      {
         PSTR pDirection = "FWP_DIRECTION_MAX";

         if(pMetadata->packetDirection == FWP_DIRECTION_OUTBOUND)
            pDirection = "FWP_DIRECTION_OUTBOUND";
         else if(pMetadata->packetDirection == FWP_DIRECTION_INBOUND)
            pDirection = "FWP_DIRECTION_INBOUND";

         DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                    DPFLTR_INFO_LEVEL,
                    "\t\t\tpacketDirection: %s\n",
                    pDirection);
      }

#if(NTDDI_VERSION >= NTDDI_VISTASP1)

      if(FWPS_IS_METADATA_FIELD_PRESENT(pMetadata,
                                        FWPS_METADATA_FIELD_PACKET_SYSTEM_CRITICAL))
         DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                    DPFLTR_INFO_LEVEL,
                    "\t\t\tpacketSystemCritical\n");

      if(FWPS_IS_METADATA_FIELD_PRESENT(pMetadata,
                                        FWPS_METADATA_FIELD_FORWARD_LAYER_OUTBOUND_PASS_THRU))
         DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                    DPFLTR_INFO_LEVEL,
                    "\t\t\tforwardLayerOutboundPassThru\n");

      if(FWPS_IS_METADATA_FIELD_PRESENT(pMetadata,
                                        FWPS_METADATA_FIELD_FORWARD_LAYER_INBOUND_PASS_THRU))
         DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                    DPFLTR_INFO_LEVEL,
                    "\t\t\tforwardLayerInboundPassThru\n");

      if(FWPS_IS_METADATA_FIELD_PRESENT(pMetadata,
                                        FWPS_METADATA_FIELD_ALE_CLASSIFY_REQUIRED))
         DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                    DPFLTR_INFO_LEVEL,
                    "\t\t\taleClassifyRequired\n");

      if(FWPS_IS_METADATA_FIELD_PRESENT(pMetadata,
                                        FWPS_METADATA_FIELD_TRANSPORT_HEADER_INCLUDE_HEADER))
      {
         DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                    DPFLTR_INFO_LEVEL,
                    "\t\t\theaderIncludeHeader: %#p\n",
                    pMetadata->headerIncludeHeader);

         DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                    DPFLTR_INFO_LEVEL,
                    "\t\t\theaderIncludeHeaderLength: %d\n",
                    pMetadata->headerIncludeHeaderLength);
      }

#if(NTDDI_VERSION >= NTDDI_WIN7)

      if(FWPS_IS_METADATA_FIELD_PRESENT(pMetadata,
                                        FWPS_METADATA_FIELD_DESTINATION_PREFIX))
      {
         PSTR pAddrFamily = "AF_UNSPEC";

         if(pMetadata->destinationPrefix.Prefix.si_family == AF_INET)
            pAddrFamily = "AF_INET";
         else if(pMetadata->destinationPrefix.Prefix.si_family == AF_INET6)
            pAddrFamily = "AF_INET6";

         DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                    DPFLTR_INFO_LEVEL,
                    "\t\t\tdestinationPrefix:\n"
                    "\t\t\t\tPrefix:\n"
                    "\t\t\t\t\tIpv4:\n"
                    "\t\t\t\t\t\tsin_family: %d\n"
                    "\t\t\t\t\t\tsin_port: %d\n"
                    "\t\t\t\t\t\tsin_addr: %d.%d.%d.%d\n"
                    "\t\t\t\t\tIpv6:\n"
                    "\t\t\t\t\t\tsin6_family: %d\n"
                    "\t\t\t\t\t\tsin6_port: %d\n"
                    "\t\t\t\t\t\tsin6_flowinfo: %d\n"
                    "\t\t\t\t\t\tsin6_addr: %04x:%04x:%04x:%04x:%04x:%04x:%04x:%04x\n"
                    "\t\t\t\t\tsi_family: %s\n"
                    "\t\t\t\tPrefixLength: %d\n",
                    pMetadata->destinationPrefix.Prefix.Ipv4.sin_family,
                    pMetadata->destinationPrefix.Prefix.Ipv4.sin_port,
                    pMetadata->destinationPrefix.Prefix.Ipv4.sin_addr.S_un.S_un_b.s_b1,
                    pMetadata->destinationPrefix.Prefix.Ipv4.sin_addr.S_un.S_un_b.s_b2,
                    pMetadata->destinationPrefix.Prefix.Ipv4.sin_addr.S_un.S_un_b.s_b3,
                    pMetadata->destinationPrefix.Prefix.Ipv4.sin_addr.S_un.S_un_b.s_b4,
                    pMetadata->destinationPrefix.Prefix.Ipv6.sin6_family,
                    pMetadata->destinationPrefix.Prefix.Ipv6.sin6_port,
                    pMetadata->destinationPrefix.Prefix.Ipv6.sin6_flowinfo,
                    pMetadata->destinationPrefix.Prefix.Ipv6.sin6_addr.u.Word[0],
                    pMetadata->destinationPrefix.Prefix.Ipv6.sin6_addr.u.Word[1],
                    pMetadata->destinationPrefix.Prefix.Ipv6.sin6_addr.u.Word[2],
                    pMetadata->destinationPrefix.Prefix.Ipv6.sin6_addr.u.Word[3],
                    pMetadata->destinationPrefix.Prefix.Ipv6.sin6_addr.u.Word[4],
                    pMetadata->destinationPrefix.Prefix.Ipv6.sin6_addr.u.Word[5],
                    pMetadata->destinationPrefix.Prefix.Ipv6.sin6_addr.u.Word[6],
                    pMetadata->destinationPrefix.Prefix.Ipv6.sin6_addr.u.Word[7],
                    pAddrFamily,
                    pMetadata->destinationPrefix.PrefixLength);
      }

      if(FWPS_IS_METADATA_FIELD_PRESENT(pMetadata,
                                        FWPS_METADATA_FIELD_ETHER_FRAME_LENGTH))
         DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                    DPFLTR_INFO_LEVEL,
                    "\t\t\tframeLength: %d\n",
                    pMetadata->frameLength);

      if(FWPS_IS_METADATA_FIELD_PRESENT(pMetadata,
                                        FWPS_METADATA_FIELD_PARENT_ENDPOINT_HANDLE))
         DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                    DPFLTR_INFO_LEVEL,
                    "\t\t\tparentEndpointHandle: %#I64x\n",
                    pMetadata->parentEndpointHandle);

      if(FWPS_IS_METADATA_FIELD_PRESENT(pMetadata,
                                        FWPS_METADATA_FIELD_ICMP_ID_AND_SEQUENCE))
         DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                    DPFLTR_INFO_LEVEL,
                    "\t\t\ticmpIdAndSequence: %#x\n",
                    pMetadata->icmpIdAndSequence);

      if(FWPS_IS_METADATA_FIELD_PRESENT(pMetadata,
                                        FWPS_METADATA_FIELD_LOCAL_REDIRECT_TARGET_PID))
         DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                    DPFLTR_INFO_LEVEL,
                    "\t\t\tlocalRedirectTargetPID: %#x\n",
                    pMetadata->localRedirectTargetPID);

      if(FWPS_IS_METADATA_FIELD_PRESENT(pMetadata,
                                        FWPS_METADATA_FIELD_ORIGINAL_DESTINATION))
         DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                    DPFLTR_INFO_LEVEL,
                    "\t\t\toriginalDestination: %#p\n",
                    pMetadata->originalDestination);

#if(NTDDI_VERSION >= NTDDI_WIN8)

      if(FWPS_IS_METADATA_FIELD_PRESENT(pMetadata,
                                        FWPS_METADATA_FIELD_REDIRECT_RECORD_HANDLE))
         DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                    DPFLTR_INFO_LEVEL,
                    "\t\t\tredirectRecords: %#p\n",
                    pMetadata->redirectRecords);

      if(FWPS_IS_METADATA_FIELD_PRESENT(pMetadata,
                                        FWPS_METADATA_FIELD_SUB_PROCESS_TAG))
         DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                    DPFLTR_INFO_LEVEL,
                    "\t\t\tsubProcessTag: %#p\n",
                    pMetadata->subProcessTag);

      DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                 DPFLTR_INFO_LEVEL,
                 "\t\tCurrentL2MetadataValues: %#x\n",
                 pMetadata->currentL2MetadataValues);

      if(FWPS_IS_L2_METADATA_FIELD_PRESENT(pMetadata,
                                           FWPS_L2_METADATA_FIELD_ETHERNET_MAC_HEADER_SIZE))
         DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                    DPFLTR_INFO_LEVEL,
                    "\t\t\tethernetMacHeaderSize: %d\n",
                    pMetadata->ethernetMacHeaderSize);

      if(FWPS_IS_L2_METADATA_FIELD_PRESENT(pMetadata,
                                           FWPS_L2_METADATA_FIELD_WIFI_OPERATION_MODE))
      {
         PSTR WIFIMode = "UNKNOWN";

         if(pMetadata->wiFiOperationMode == DOT11_OPERATION_MODE_EXTENSIBLE_AP)
            WIFIMode = "DOT11_OPERATION_MODE_EXTENSIBLE_AP";
         else if(pMetadata->wiFiOperationMode == DOT11_OPERATION_MODE_EXTENSIBLE_STATION)
            WIFIMode = "DOT11_OPERATION_MODE_EXTENSIBLE_STATION";
         else if(pMetadata->wiFiOperationMode == DOT11_OPERATION_MODE_NETWORK_MONITOR)
            WIFIMode = "DOT11_OPERATION_MODE_NETWORK_MONITOR";
         else if(pMetadata->wiFiOperationMode == DOT11_OPERATION_MODE_WFD_DEVICE)
            WIFIMode = "DOT11_OPERATION_MODE_WFD_DEVICE";
         else if(pMetadata->wiFiOperationMode == DOT11_OPERATION_MODE_WFD_GROUP_OWNER)
            WIFIMode = "DOT11_OPERATION_MODE_WFD_GROUP_OWNER";
         else if(pMetadata->wiFiOperationMode == DOT11_OPERATION_MODE_WFD_CLIENT)
            WIFIMode = "DOT11_OPERATION_MODE_WFD_CLIENT";

         DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                    DPFLTR_INFO_LEVEL,
                    "\t\t\twiFiOperationMode: %s\n",
                    WIFIMode);
      }

      if(FWPS_IS_L2_METADATA_FIELD_PRESENT(pMetadata,
                                           FWPS_L2_METADATA_FIELD_VSWITCH_SOURCE_PORT_ID))
         DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                    DPFLTR_INFO_LEVEL,
                    "\t\t\tvSwitchSourcePortId: %#x\n",
                    pMetadata->vSwitchSourcePortId);

      if(FWPS_IS_L2_METADATA_FIELD_PRESENT(pMetadata,
                                           FWPS_L2_METADATA_FIELD_VSWITCH_SOURCE_NIC_INDEX))
         DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                    DPFLTR_INFO_LEVEL,
                    "\t\t\tvSwitchSourceNicIndex: %#x\n",
                    pMetadata->vSwitchSourceNicIndex);

      if(FWPS_IS_L2_METADATA_FIELD_PRESENT(pMetadata,
                                           FWPS_L2_METADATA_FIELD_VSWITCH_PACKET_CONTEXT))
         DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                    DPFLTR_INFO_LEVEL,
                    "\t\t\tvSwitchPacketContext: %#p\n",
                    pMetadata->vSwitchPacketContext);

      if(FWPS_IS_L2_METADATA_FIELD_PRESENT(pMetadata,
                                           FWPS_L2_METADATA_FIELD_VSWITCH_DESTINATION_PORT_ID))
         DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                    DPFLTR_INFO_LEVEL,
                    "\t\t\tvSwitchDestinationPortId: %#x\n",
                    pMetadata->vSwitchDestinationPortId);

#if(NTDDI_VERSION >= NTDDI_WINBLUE)

      if(FWPS_IS_L2_METADATA_FIELD_PRESENT(pMetadata,
                                           FWPS_L2_METADATA_FIELD_RESERVED))
         DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                    DPFLTR_INFO_LEVEL,
                    "\t\t\treserved1: %#I64x\n",
                    pMetadata->reserved1);

#endif /// (NTDDI_VERSION >= NTDDI_WINBLUE)
#endif /// (NTDDI_VERSION >= NTDDI_WIN8)
#endif /// (NTDDI_VERSION >= NTDDI_WIN7)
#endif /// (NTDDI_VERSION >= NTDDI_VISTASP1)

      HLPR_BAIL_LABEL:

      HLPR_DELETE_ARRAY(pString,
                        WFPSAMPLER_CALLOUT_DRIVER_TAG);
   }

   return;
}

/**
 @private_function="LogFilter"
 
   Purpose:  Logs the FWPS_FILTER from the classifyFn.                                          <br>
                                                                                                <br>
   Notes:    Uses ETW Tracing for the logging.                                                  <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF552387.aspx             <br>
             HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF552389.aspx             <br>
             HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/HH439768.aspx             <br>
*/
VOID LogFilter(_In_ const FWPS_FILTER* pFilter)
{
   if(pFilter)
   {
      NTSTATUS status  = STATUS_SUCCESS;
      PSTR     pType   = "FWP_EMPTY";
      UINT64   data    = 0;
      PSTR     pAction = "FWP_ACTION_CALLOUT_UNKNOWN";
      PSTR     pString = 0;

      if(pFilter->weight.type == FWP_UINT64)
      {
         pType = "FWP_UINT64";

         if(pFilter->weight.uint64)
            data = *(pFilter->weight.uint64);
      }

      if(pFilter->action.type == FWP_ACTION_CALLOUT_TERMINATING)
         pAction = "FWP_ACTION_CALLOUT_TERMINATING";
      else if(pFilter->action.type == FWP_ACTION_CALLOUT_INSPECTION)
         pAction = "FWP_ACTION_CALLOUT_INSPECTION";

      HLPR_NEW_ARRAY(pString,
                     CHAR,
                     MAX_STRING_SIZE,
                     WFPSAMPLER_CALLOUT_DRIVER_TAG);
      HLPR_BAIL_ON_ALLOC_FAILURE(pString,
                                 status);

      status = RtlStringCchPrintfA(pString,
                                   MAX_STRING_SIZE,
                                   "\tFilter:\n"
                                   "\t\tfilterId: %#I64x\n"
                                   "\t\tweight:\n"
                                   "\t\t\ttype: %s\n"
                                   "\t\t\tuint64: %#I64x\n"
                                   "\t\tsubLayerWeight: %#x\n"
                                   "\t\tflags: %#x\n"
                                   "\t\tnumFilterConditions: %d:\n"
                                   "\t\tfilterCondition: %#p\n"
                                   "\t\taction:\n"
                                   "\t\t\ttype: %s\n"
                                   "\t\t\tcalloutId: %#x\n"
                                   "\t\tcontext: %#I64d\n"
                                   "\t\tproviderContext: %#p\n",
                                   pFilter->filterId,
                                   pType,
                                   data,
                                   pFilter->subLayerWeight,
                                   pFilter->flags,
                                   pFilter->numFilterConditions,
                                   pFilter->filterCondition,
                                   pAction,
                                   pFilter->action.calloutId,
                                   pFilter->context,
                                   pFilter->providerContext);
      if(status == STATUS_SUCCESS)
         DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                    DPFLTR_INFO_LEVEL,
                    "%s",
                    pString);

      HLPR_BAIL_LABEL:

      HLPR_DELETE_ARRAY(pString,
                        WFPSAMPLER_CALLOUT_DRIVER_TAG);
   }

   return;
}

/**
 @private_function="LogClassifyOut"
 
   Purpose:  Logs the FWPS_CLASSIFY_OUT from the classifyFn.                                    <br>
                                                                                                <br>
   Notes:    Uses ETW Tracing for the logging.                                                  <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF551229.aspx             <br>
*/
VOID LogClassifyOut(_In_ const FWPS_CLASSIFY_OUT* pClassifyOut)
{
   if(pClassifyOut)
   {
      NTSTATUS status  = STATUS_SUCCESS;
      PSTR     pAction = "FWP_ACTION_NONE";
      PSTR     pString = 0;


      if(pClassifyOut->actionType == FWP_ACTION_BLOCK)
         pAction = "FWP_ACTION_BLOCK";
      else if(pClassifyOut->actionType == FWP_ACTION_CONTINUE)
         pAction = "FWP_ACTION_CONTINUE";
      else if(pClassifyOut->actionType == FWP_ACTION_NONE_NO_MATCH)
         pAction = "FWP_ACTION_NONE_NO_MATCH";
      else if(pClassifyOut->actionType == FWP_ACTION_PERMIT)
         pAction = "FWP_ACTION_PERMIT";

      HLPR_NEW_ARRAY(pString,
                     CHAR,
                     MAX_STRING_SIZE,
                     WFPSAMPLER_CALLOUT_DRIVER_TAG);
      HLPR_BAIL_ON_ALLOC_FAILURE(pString,
                                 status);

      status = RtlStringCchPrintfA(pString,
                                   MAX_STRING_SIZE,
                                   "\tClassifyOut:\n"
                                   "\t\tactionType: %s\n"
                                   "\t\toutContext: %#I64x\n"
                                   "\t\tfilterId: %I64d\n"
                                   "\t\trights: %#x",
                                   pAction,
                                   pClassifyOut->outContext,
                                   pClassifyOut->filterId,
                                   pClassifyOut->rights);
      if(status == STATUS_SUCCESS)
         DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                    DPFLTR_INFO_LEVEL,
                    "%s",
                    pString);

      if(pClassifyOut->rights & FWPS_RIGHT_ACTION_WRITE)
         DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                    DPFLTR_INFO_LEVEL,
                    "\t\t\tFWPS_RIGHT_ACTION_WRITE");


      DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                 DPFLTR_INFO_LEVEL,
                 "\t\tflags: %#x",
                 pClassifyOut->flags);

      if(pClassifyOut->flags & FWPS_CLASSIFY_OUT_FLAG_ABSORB)
         DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                    DPFLTR_INFO_LEVEL,
                    "\t\t\tFWPS_CLASSIFY_OUT_FLAG_ABSORB");

      if(pClassifyOut->flags & FWPS_CLASSIFY_OUT_FLAG_BUFFER_LIMIT_REACHED)
         DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                    DPFLTR_INFO_LEVEL,
                    "\t\t\tFWPS_CLASSIFY_OUT_FLAG_BUFFER_LIMIT_REACHED");

      if(pClassifyOut->flags & FWPS_CLASSIFY_OUT_FLAG_NO_MORE_DATA)
         DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                    DPFLTR_INFO_LEVEL,
                    "\t\t\tFWPS_CLASSIFY_OUT_FLAG_NO_MORE_DATA");

      DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                 DPFLTR_INFO_LEVEL,
                 "\t\treserved: %#x",
                 pClassifyOut->reserved);

      HLPR_BAIL_LABEL:

      HLPR_DELETE_ARRAY(pString,
                        WFPSAMPLER_CALLOUT_DRIVER_TAG);
   }

   return;
}

/**
 @private_function="LogEthernetIIHeader"
 
   Purpose:  Logs the Ethernet II Header into a more easily readable format.                    <br>
                                                                                                <br>
   Notes:    Uses ETW Tracing for the logging, which is not ideal in a real world scenario.     <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/FF562859.aspx                              <br>
*/
VOID LogEthernetIIHeader(_In_ ETHERNET_II_HEADER* pEthernetIIHeader,
                         _In_ UINT32 vlanID = 0) 
{
#if DBG

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> LogEthernetIIHeader()\n");

#endif /// DBG

   NT_ASSERT(pEthernetIIHeader);

   NTSTATUS status  = STATUS_SUCCESS;
   PSTR     pString = 0;

   HLPR_NEW_ARRAY(pString,
                  CHAR,
                  MAX_STRING_SIZE,
                  WFPSAMPLER_CALLOUT_DRIVER_TAG);
   HLPR_BAIL_ON_ALLOC_FAILURE(pString,
                              status);

   if(vlanID)
      status = RtlStringCchPrintfA(pString,
                                   MAX_STRING_SIZE,
                                   "\n"
                                   "\t\t 0                   1                   2\n"
                                   "\t\t 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3\n"
                                   "\t\t+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+\n"
                                   "\t\t|                                               |\n"
                                   "\t\t+            Destination MAC Address            +\n"
                                   "\t\t|                                               |\n"
                                   "\t\t|                %02x:%02x:%02x:%02x:%02x:%02x              |\n"
                                   "\t\t+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+\n"
                                   "\t\t|                                               |\n"
                                   "\t\t+               Source MAC Address              +\n"
                                   "\t\t|                                               |\n"
                                   "\t\t|                %02x:%02x:%02x:%02x:%02x:%02x              |\n"
                                   "\t\t+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+\n"
                                   "\t\t|                    VLAN ID ...                >\n"
                                   "\t\t|                                               >\n"
                                   "\t\t+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+\n"
                                   "\t\t|VLAN ID (cont.)|              Type             |\n"
                                   "\t\t|               |                               |\n"
                                   "\t\t+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+\n",
                                   pEthernetIIHeader->pDestinationAddress[0],
                                   pEthernetIIHeader->pDestinationAddress[1],
                                   pEthernetIIHeader->pDestinationAddress[2],
                                   pEthernetIIHeader->pDestinationAddress[3],
                                   pEthernetIIHeader->pDestinationAddress[4],
                                   pEthernetIIHeader->pDestinationAddress[5],
                                   pEthernetIIHeader->pSourceAddress[0],
                                   pEthernetIIHeader->pSourceAddress[1],
                                   pEthernetIIHeader->pSourceAddress[2],
                                   pEthernetIIHeader->pSourceAddress[3],
                                   pEthernetIIHeader->pSourceAddress[4],
                                   pEthernetIIHeader->pSourceAddress[5]);
   else
      status = RtlStringCchPrintfA(pString,
                                   MAX_STRING_SIZE,
                                   "\n"
                                   "\t\t 0                   1                   2\n"
                                   "\t\t 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3\n"
                                   "\t\t+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+\n"
                                   "\t\t|                                               |\n"
                                   "\t\t+            Destination MAC Address            +\n"
                                   "\t\t|                                               |\n"
                                   "\t\t|                %02x:%02x:%02x:%02x:%02x:%02x              |\n"
                                   "\t\t+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+\n"
                                   "\t\t|                                               |\n"
                                   "\t\t+               Source MAC Address              +\n"
                                   "\t\t|                                               |\n"
                                   "\t\t|                %02x:%02x:%02x:%02x:%02x:%02x              |\n"
                                   "\t\t+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+\n"
                                   "\t\t|              Type             |    Data...    |\n"
                                   "\t\t|              %04x             |               |\n"
                                   "\t\t+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+\n",
                                   pEthernetIIHeader->pDestinationAddress[0],
                                   pEthernetIIHeader->pDestinationAddress[1],
                                   pEthernetIIHeader->pDestinationAddress[2],
                                   pEthernetIIHeader->pDestinationAddress[3],
                                   pEthernetIIHeader->pDestinationAddress[4],
                                   pEthernetIIHeader->pDestinationAddress[5],
                                   pEthernetIIHeader->pSourceAddress[0],
                                   pEthernetIIHeader->pSourceAddress[1],
                                   pEthernetIIHeader->pSourceAddress[2],
                                   pEthernetIIHeader->pSourceAddress[3],
                                   pEthernetIIHeader->pSourceAddress[4],
                                   pEthernetIIHeader->pSourceAddress[5],
                                   ntohs(pEthernetIIHeader->type));

   if(status == STATUS_SUCCESS)
      DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                 DPFLTR_INFO_LEVEL,
                 "\tEthernet II Header:%s",
                 pString);

   HLPR_BAIL_LABEL:

   HLPR_DELETE_ARRAY(pString,
                     WFPSAMPLER_CALLOUT_DRIVER_TAG);

#if DBG

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- LogEthernetIIHeader [status: %#x]\n",
              status);

#else

   /// Used to get around preFast Warning 28931
   UNREFERENCED_PARAMETER(status);

#endif /// DBG

   return;
}

/**
 @private_function="LogEthernetSNAPHeader"
 
   Purpose:  Logs the MAC Ethernet SNAP Header into a more easily readable format.              <br>
                                                                                                <br>
   Notes:    Uses ETW Tracing for the logging, which is not ideal in a real world scenario.     <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/FF562859.aspx                              <br>
*/
VOID LogEthernetSNAPHeader(_In_ ETHERNET_SNAP_HEADER* pEthernetSNAPHeader)
{
#if DBG

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> LogEthernetSNAPHeader()\n");

#endif /// DBG

   NT_ASSERT(pEthernetSNAPHeader);

   NTSTATUS status  = STATUS_SUCCESS;
   PSTR     pString = 0;

   HLPR_NEW_ARRAY(pString,
                  CHAR,
                  MAX_STRING_SIZE,
                  WFPSAMPLER_CALLOUT_DRIVER_TAG);
   HLPR_BAIL_ON_ALLOC_FAILURE(pString,
                              status);

   status = RtlStringCchPrintfA(pString,
                                MAX_STRING_SIZE,
                                "\n"
                                "\t\t 0                   1                   2\n"
                                "\t\t 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3\n"
                                "\t\t+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+\n"
                                "\t\t|                                               |\n"
                                "\t\t+            Destination MAC Address            +\n"
                                "\t\t|                                               |\n"
                                "\t\t|                %02x:%02x:%02x:%02x:%02x:%02x              |\n"
                                "\t\t+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+\n"
                                "\t\t|                                               |\n"
                                "\t\t+               Source MAC Address              +\n"
                                "\t\t|                                               |\n"
                                "\t\t|                %02x:%02x:%02x:%02x:%02x:%02x              |\n"
                                "\t\t+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+\n"
                                "\t\t|            Length             |      DSAP     |\n"
                                "\t\t|              %04x             |       %02x      |\n"
                                "\t\t+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+\n"
                                "\t\t|      SSAP     | Control Byte  |    OUI ...    >\n"
                                "\t\t|       %02x      |       %02x      |       %02x      |\n"
                                "\t\t+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+\n"
                                "\t\t<           OUI (cont.)         |    Type ...   >\n"
                                "\t\t<              %02x%02x             |       %04x      >\n"
                                "\t\t+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+\n"
                                "\t\t<  Type (cont.) |            Data ...           |\n"
                                "\t\t<               |                               |\n"
                                "\t\t+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+\n",
                                pEthernetSNAPHeader->pDestinationAddress[0],
                                pEthernetSNAPHeader->pDestinationAddress[1],
                                pEthernetSNAPHeader->pDestinationAddress[2],
                                pEthernetSNAPHeader->pDestinationAddress[3],
                                pEthernetSNAPHeader->pDestinationAddress[4],
                                pEthernetSNAPHeader->pDestinationAddress[5],
                                pEthernetSNAPHeader->pSourceAddress[0],
                                pEthernetSNAPHeader->pSourceAddress[1],
                                pEthernetSNAPHeader->pSourceAddress[2],
                                pEthernetSNAPHeader->pSourceAddress[3],
                                pEthernetSNAPHeader->pSourceAddress[4],
                                pEthernetSNAPHeader->pSourceAddress[5],
                                pEthernetSNAPHeader->length,
                                pEthernetSNAPHeader->destinationSAP,
                                pEthernetSNAPHeader->sourceSAP,
                                pEthernetSNAPHeader->controlByte,
                                pEthernetSNAPHeader->pOUI[0],
                                pEthernetSNAPHeader->pOUI[1],
                                pEthernetSNAPHeader->pOUI[2],
                                ntohs(pEthernetSNAPHeader->type));
   if(status == STATUS_SUCCESS)
      DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                 DPFLTR_INFO_LEVEL,
                 "\tEthernet SNAP Header:%s",
                 pString);

   HLPR_BAIL_LABEL:

   HLPR_DELETE_ARRAY(pString,
                     WFPSAMPLER_CALLOUT_DRIVER_TAG);

#if DBG

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- LogEthernetSNAPHeader() [status: %#x]\n",
              status);

#else

   /// Used to get around preFast Warning 28931
   UNREFERENCED_PARAMETER(status);

#endif /// DBG

   return;
}

/**
 @private_function="LogIPv4Header"
 
   Purpose:  Logs the IPv4 Header into a more easily readable format.                           <br>
                                                                                                <br>
   Notes:    Uses ETW Tracing for the logging, which is not ideal in a real world scenario.     <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/FF562859.aspx                              <br>
*/
VOID LogIPv4Header(_In_ IP_HEADER_V4* pIPv4Header)
{
#if DBG

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> LogIPv4Header()\n");

#endif /// DBG

   NT_ASSERT(pIPv4Header);

   NTSTATUS status  = STATUS_SUCCESS;
   PSTR     pString = 0;

   HLPR_NEW_ARRAY(pString,
                  CHAR,
                  MAX_STRING_SIZE,
                  WFPSAMPLER_CALLOUT_DRIVER_TAG);
   HLPR_BAIL_ON_ALLOC_FAILURE(pString,
                              status);

   status = RtlStringCchPrintfA(pString,
                                MAX_STRING_SIZE,
                                "\n"
                                "\t\t 0                   1                   2                   3\n"
                                "\t\t 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1\n"
                                "\t\t+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+\n"
                                "\t\t|Version|  IHL  |Type of Service|         Total Length          |\n"
                                "\t\t|   %01x   |   %01x   |       %02x      |              %04x             |\n"
                                "\t\t+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+\n"
                                "\t\t|        Identification         |Flags|     Fragment Offset     |\n"
                                "\t\t|              %04x             |  %01x  |           %04x          |\n"
                                "\t\t+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+\n"
                                "\t\t|  Time to Live |    Protocol     |        Header Checksum      |\n"
                                "\t\t|       %02x      |        %02x       |             %04x            |\n"
                                "\t\t+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+\n"
                                "\t\t|                        Source Address                         |\n"
                                "\t\t|                           %08x                            |\n"
                                "\t\t+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+\n"
                                "\t\t|                      Destination Address                      |\n"
                                "\t\t|                           %08x                            |\n"
                                "\t\t+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+\n"
                                "\t\t|                    Options                    |    Padding    |\n"
                                "\t\t+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+\n",
                                pIPv4Header->version,
                                pIPv4Header->headerLength,
                                pIPv4Header->typeOfService,
                                pIPv4Header->totalLength,
                                ntohs(pIPv4Header->identification),
                                pIPv4Header->flags,
                                pIPv4Header->fragmentOffset,
                                pIPv4Header->timeToLive,
                                pIPv4Header->protocol,
                                ntohs(pIPv4Header->checksum),
                                ntohl((*((UINT32*)pIPv4Header->pSourceAddress))),
                                ntohl((*((UINT32*)pIPv4Header->pDestinationAddress))));
   if(status == STATUS_SUCCESS)
      DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                 DPFLTR_INFO_LEVEL,
                 "\tIPv4 Header:%s",
                 pString);

   HLPR_BAIL_LABEL:

   HLPR_DELETE_ARRAY(pString,
                     WFPSAMPLER_CALLOUT_DRIVER_TAG);

#if DBG

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- LogIPv4Header() [status: %#x]\n",
              status);

#else

   /// Used to get around preFast Warning 28931
   UNREFERENCED_PARAMETER(status);

#endif /// DBG

   return;
}

/**
 @private_function="LogIPv6Header"
 
   Purpose:  Logs the IPv6 Header into a more easily readable format.                           <br>
                                                                                                <br>
   Notes:    Uses ETW Tracing for the logging, which is not ideal in a real world scenario.     <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/FF562859.aspx                              <br>
*/
VOID LogIPv6Header(_In_ IP_HEADER_V6* pIPv6Header)
{
#if DBG

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> LogIPv6Header()\n");

#endif /// DBG

   NT_ASSERT(pIPv6Header);

   NTSTATUS status                          = STATUS_SUCCESS;
   PSTR     pString                         = 0;
   UINT32   versionTrafficClassAndFlowLabel = 0;

   RtlCopyMemory(&versionTrafficClassAndFlowLabel,
                 pIPv6Header->pVersionTrafficClassAndFlowLabel,
                 4);

   HLPR_NEW_ARRAY(pString,
                  CHAR,
                  MAX_STRING_SIZE,
                  WFPSAMPLER_CALLOUT_DRIVER_TAG);
   HLPR_BAIL_ON_ALLOC_FAILURE(pString,
                              status);

   status = RtlStringCchPrintfA(pString,
                                MAX_STRING_SIZE,
                                "\n"
                                "\t\t 0                   1                   2                   3\n"
                                "\t\t 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1\n"
                                "\t\t+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+\n"
                                "\t\t|Version| Traffic Class |              Flow Label               |\n"
                                "\t\t|   %01x   |       %02x      |                 %05x                 |\n"
                                "\t\t+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+\n"
                                "\t\t|        Payload Length         |  Next Header  |   Hop Limit   |\n"
                                "\t\t|              %04x             |       %02x      |       %02x      |\n"
                                "\t\t+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+\n"
                                "\t\t|                                                               |\n"
                                "\t\t+                                                               +\n"
                                "\t\t|                                                               |\n"
                                "\t\t+                        Source Address                         +\n"
                                "\t\t|                                                               |\n"
                                "\t\t+                                                               +\n"
                                "\t\t|            %02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x            |\n"
                                "\t\t+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+\n"
                                "\t\t|                                                               |\n"
                                "\t\t+                                                               +\n"
                                "\t\t|                                                               |\n"
                                "\t\t+                      Destination Address                      +\n"
                                "\t\t|                                                               |\n"
                                "\t\t+                                                               +\n"
                                "\t\t|            %02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x            |\n"
                                "\t\t+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+\n",
                                pIPv6Header->version.value,
                                (ntohl(versionTrafficClassAndFlowLabel) & 0x0FF00000) >> 20,
                                ntohl(versionTrafficClassAndFlowLabel) & 0x000FFFFF,
                                ntohs(pIPv6Header->payloadLength),
                                pIPv6Header->nextHeader,
                                pIPv6Header->hopLimit,
                                pIPv6Header->pSourceAddress[0],
                                pIPv6Header->pSourceAddress[1],
                                pIPv6Header->pSourceAddress[2],
                                pIPv6Header->pSourceAddress[3],
                                pIPv6Header->pSourceAddress[4],
                                pIPv6Header->pSourceAddress[5],
                                pIPv6Header->pSourceAddress[6],
                                pIPv6Header->pSourceAddress[7],
                                pIPv6Header->pSourceAddress[8],
                                pIPv6Header->pSourceAddress[9],
                                pIPv6Header->pSourceAddress[10],
                                pIPv6Header->pSourceAddress[11],
                                pIPv6Header->pSourceAddress[12],
                                pIPv6Header->pSourceAddress[13],
                                pIPv6Header->pSourceAddress[14],
                                pIPv6Header->pSourceAddress[15],
                                pIPv6Header->pDestinationAddress[0],
                                pIPv6Header->pDestinationAddress[1],
                                pIPv6Header->pDestinationAddress[2],
                                pIPv6Header->pDestinationAddress[3],
                                pIPv6Header->pDestinationAddress[4],
                                pIPv6Header->pDestinationAddress[5],
                                pIPv6Header->pDestinationAddress[6],
                                pIPv6Header->pDestinationAddress[7],
                                pIPv6Header->pDestinationAddress[8],
                                pIPv6Header->pDestinationAddress[9],
                                pIPv6Header->pDestinationAddress[10],
                                pIPv6Header->pDestinationAddress[11],
                                pIPv6Header->pDestinationAddress[12],
                                pIPv6Header->pDestinationAddress[13],
                                pIPv6Header->pDestinationAddress[14],
                                pIPv6Header->pDestinationAddress[15]);
   if(status == STATUS_SUCCESS)
      DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                 DPFLTR_INFO_LEVEL,
                 "\tIPv6 Header:%s",
                 pString);

   HLPR_BAIL_LABEL:

   HLPR_DELETE_ARRAY(pString,
                     WFPSAMPLER_CALLOUT_DRIVER_TAG);

#if DBG

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- LogIPv6Header [status: %#x]\n",
              status);
#else

   /// Used to get around preFast Warning 28931
   UNREFERENCED_PARAMETER(status);

#endif /// DBG

   return;
}

/**
 @private_function="LogIPHeader"
 
   Purpose:  Proxy IP Header logging to appropriate logging function base on IP version.        <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
VOID LogIPHeader(_In_ VOID* pIPHeader,
                 _In_ UINT8 ipVersion)
{
   NT_ASSERT(pIPHeader);

   if(ipVersion == IPV4)
      LogIPv4Header((IP_HEADER_V4*)pIPHeader);
   else
      LogIPv6Header((IP_HEADER_V6*)pIPHeader);

   return;
}

/**
 @private_function="LogICMPv4Header"
 
   Purpose:  Logs the ICMPv4 Header into a more easily readable format.                         <br>
                                                                                                <br>
   Notes:    Uses ETW Tracing for the logging, which is not ideal in a real world scenario.     <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/FF562859.aspx                              <br>
*/
VOID LogICMPv4Header(_In_ ICMP_HEADER_V4* pICMPv4Header)
{
#if DBG

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> LogICMPv4Header()\n");

#endif /// DBG

   NT_ASSERT(pICMPv4Header);

   NTSTATUS status  = STATUS_SUCCESS;
   PSTR     pString = 0;

   HLPR_NEW_ARRAY(pString,
                  CHAR,
                  MAX_STRING_SIZE,
                  WFPSAMPLER_CALLOUT_DRIVER_TAG);
   HLPR_BAIL_ON_ALLOC_FAILURE(pString,
                              status);

   status = RtlStringCchPrintfA(pString,
                                MAX_STRING_SIZE,
                                "\n"
                                "\t\t 0                   1                   2                   3\n"
                                "\t\t 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1\n"
                                "\t\t+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+\n"
                                "\t\t|     Type      |     Code      |           Checksum            |\n"
                                "\t\t|       %02x      |       %02x      |              %04x             |\n"
                                "\t\t+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+\n"
                                "\t\t|              Variable (Dependent on Type / Code)              |\n"
                                "\t\t+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+\n",
                                pICMPv4Header->type,
                                pICMPv4Header->code,
                                ntohs(pICMPv4Header->checksum));
   if(status == STATUS_SUCCESS)
      DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                 DPFLTR_INFO_LEVEL,
                 "\tICMPv4 Header:%s",
                 pString);

   HLPR_BAIL_LABEL:

   HLPR_DELETE_ARRAY(pString,
                     WFPSAMPLER_CALLOUT_DRIVER_TAG);

#if DBG

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- LogICMPv4Header [status: %#x]\n",
              status);

#else

   /// Used to get around preFast Warning 28931
   UNREFERENCED_PARAMETER(status);

#endif /// DBG

   return;
}

/**
 @private_function="LogICMPv6Header"
 
   Purpose:  Logs the ICMPv6 Header into a more easily readable format.                         <br>
                                                                                                <br>
   Notes:    Uses ETW Tracing for the logging, which is not ideal in a real world scenario.     <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/FF562859.aspx                              <br>
*/
VOID LogICMPv6Header(_In_ ICMP_HEADER_V6* pICMPv6Header)
{
#if DBG

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> LogICMPv6Header()\n");

#endif /// DBG

   NT_ASSERT(pICMPv6Header);

   NTSTATUS status  = STATUS_SUCCESS;
   PSTR     pString = 0;

   HLPR_NEW_ARRAY(pString,
                  CHAR,
                  MAX_STRING_SIZE,
                  WFPSAMPLER_CALLOUT_DRIVER_TAG);
   HLPR_BAIL_ON_ALLOC_FAILURE(pString,
                              status);

   status = RtlStringCchPrintfA(pString,
                                MAX_STRING_SIZE,
                                "\n"
                                "\t\t 0                   1                   2                   3\n"
                                "\t\t 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1\n"
                                "\t\t+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+\n"
                                "\t\t|     Type      |     Code      |           Checksum            |\n"
                                "\t\t|       %02x      |       %02x      |              %04x             |\n"
                                "\t\t+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+\n"
                                "\t\t|              Variable (Dependent on Type / Code)              |\n"
                                "\t\t+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+\n",
                                pICMPv6Header->type,
                                pICMPv6Header->code,
                                ntohs(pICMPv6Header->checksum));
   if(status == STATUS_SUCCESS)
      DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                 DPFLTR_INFO_LEVEL,
                 "\tICMPv6 Header:%s",
                 pString);

   HLPR_BAIL_LABEL:

   HLPR_DELETE_ARRAY(pString,
                     WFPSAMPLER_CALLOUT_DRIVER_TAG);

#if DBG

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- LogICMPv6Header() [status: %#x]\n",
              status);

#else

   /// Used to get around preFast Warning 28931
   UNREFERENCED_PARAMETER(status);

#endif /// DBG

   return;
}

/**
 @private_function="LogTCPHeader"
 
   Purpose:  Logs the TCP Header into a more easily readable format.                            <br>
                                                                                                <br>
   Notes:    Uses ETW Tracing for the logging, which is not ideal in a real world scenario.     <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/FF562859.aspx                              <br>
*/
VOID LogTCPHeader(_In_ TCP_HEADER* pTCPHeader)
{
#if DBG

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> LogTCPHeader()\n");

#endif /// DBG

   NT_ASSERT(pTCPHeader);

   NTSTATUS status  = STATUS_SUCCESS;
   PSTR     pString = 0;

   HLPR_NEW_ARRAY(pString,
                  CHAR,
                  MAX_STRING_SIZE,
                  WFPSAMPLER_CALLOUT_DRIVER_TAG);
   HLPR_BAIL_ON_ALLOC_FAILURE(pString,
                              status);

   status = RtlStringCchPrintfA(pString,
                                MAX_STRING_SIZE,
                                "\n"
                                "\t\t 0                   1                   2                   3\n"
                                "\t\t 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1\n"
                                "\t\t+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+\n"
                                "\t\t|          Source Port          |       Destination Port        |\n"
                                "\t\t|              %04x             |              %04x             |\n"
                                "\t\t+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+\n"
                                "\t\t|                        Sequence Number                        |\n"
                                "\t\t|                            %08x                           |\n"
                                "\t\t+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+\n"
                                "\t\t|                     Acknowledgment Number                     |\n"
                                "\t\t|                            %08x                           |\n"
                                "\t\t+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+\n"
                                "\t\t|Offset |Rsvd |N|C|E|U|A|P|R|S|F|            Window             |\n"
                                "\t\t|   %01x   |  %01x  |%01x|%01x|%01x|%01x|%01x|%01x|%01x|%01x|%01x|              %04x             |\n"
                                "\t\t+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+\n"
                                "\t\t|           Checksum            |        Urgent Pointer         |\n"
                                "\t\t|              %04x             |              %04x             |\n"
                                "\t\t+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+\n"
                                "\t\t|                    Options                    |    Padding    |\n"
                                "\t\t+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+\n",
                                ntohs(pTCPHeader->sourcePort),
                                ntohs(pTCPHeader->destinationPort),
                                ntohl(pTCPHeader->sequenceNumber),
                                ntohl(pTCPHeader->acknowledgementNumber),
                                pTCPHeader->dORNS.dataOffset,
                                pTCPHeader->dORNS.reserved,
                                pTCPHeader->dORNS.nonceSum,
                                pTCPHeader->CWR,
                                pTCPHeader->ECE,
                                pTCPHeader->URG,
                                pTCPHeader->ACK,
                                pTCPHeader->PSH,
                                pTCPHeader->RST,
                                pTCPHeader->SYN,
                                pTCPHeader->FIN,
                                ntohs(pTCPHeader->window),
                                ntohs(pTCPHeader->checksum),
                                ntohs(pTCPHeader->urgentPointer));
   if(status == STATUS_SUCCESS)
      DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                 DPFLTR_INFO_LEVEL,
                 "\tTCP Header:%s",
                 pString);

   HLPR_BAIL_LABEL:

   HLPR_DELETE_ARRAY(pString,
                     WFPSAMPLER_CALLOUT_DRIVER_TAG);

#if DBG

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- LogTCPHeader() [status: %#x]\n",
              status);

#else

   /// Used to get around preFast Warning 28931
   UNREFERENCED_PARAMETER(status);

#endif /// DBG

   return;
}

/**
 @private_function="LogUDPHeader"
 
   Purpose:  Logs the UDP Header into a more easily readable format.                            <br>
                                                                                                <br>
   Notes:    Uses ETW Tracing for the logging, which is not ideal in a real world scenario.     <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/FF562859.aspx                              <br>
*/
VOID LogUDPHeader(_In_ UDP_HEADER* pUDPHeader)
{
#if DBG

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> LogUDPHeader()\n");

#endif /// DBG

   NT_ASSERT(pUDPHeader);

   NTSTATUS status  = STATUS_SUCCESS;
   PSTR     pString = 0;

   HLPR_NEW_ARRAY(pString,
                  CHAR,
                  MAX_STRING_SIZE,
                  WFPSAMPLER_CALLOUT_DRIVER_TAG);
   HLPR_BAIL_ON_ALLOC_FAILURE(pString,
                              status);

   status = RtlStringCchPrintfA(pString,
                                MAX_STRING_SIZE,
                                "\n"
                                "\t\t 0                   1                   2                   3\n"
                                "\t\t 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1\n"
                                "\t\t+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+\n"
                                "\t\t|          Source Port          |       Destination Port        |\n"
                                "\t\t|              %04x             |              %04x             |\n"
                                "\t\t+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+\n"
                                "\t\t|            Length             |           Checksum            |\n"
                                "\t\t|              %04x             |              %04x             |\n"
                                "\t\t+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+\n",
                                ntohs(pUDPHeader->sourcePort),
                                ntohs(pUDPHeader->destinationPort),
                                ntohs(pUDPHeader->length),
                                ntohs(pUDPHeader->checksum));
   if(status == STATUS_SUCCESS)
      DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                 DPFLTR_INFO_LEVEL,
                 "\tUDP Header:%s",
                 pString);

   HLPR_BAIL_LABEL:

   HLPR_DELETE_ARRAY(pString,
                     WFPSAMPLER_CALLOUT_DRIVER_TAG);

#if DBG

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- LogUDPHeader() [status: %#x]\n",
              status);

#else

   /// Used to get around preFast Warning 28931
   UNREFERENCED_PARAMETER(status);

#endif /// DBG

   return;
}

/**
 @private_function="LogTransportHeader"
 
   Purpose:  Proxy Transport Header logging to appropriate logging function base on IP protocol.<br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
VOID LogTransportHeader(_In_ VOID* pTransportHeader,
                        _In_ UINT8 ipProtocol)
{
   NT_ASSERT(pTransportHeader);

   switch(ipProtocol)
   {
      case ICMPV4:
      {
         LogICMPv4Header((ICMP_HEADER_V4*)pTransportHeader);

         break;
      }
      case TCP:
      {
         LogTCPHeader((TCP_HEADER*)pTransportHeader);

         break;
      }
      case UDP:
      {
         LogUDPHeader((UDP_HEADER*)pTransportHeader);

         break;
      }
      case ICMPV6:
      {
         LogICMPv6Header((ICMP_HEADER_V6*)pTransportHeader);

         break;
      }
   }

   return;
}

#if(NTDDI_VERSION >= NTDDI_WIN8)

/**
 @private_function="PerformBasicPacketExaminationAtInboundMACFrame"
 
   Purpose:  Examines and logs the contents of the MAC Header and Ip and Transport Headers if 
             available.                                                                         <br>
                                                                                                <br>
   Notes:    Applies to the following framing layers:                                           <br>
                FWPM_LAYER_INBOUND_MAC_FRAME_ETHERNET                                           <br>
                FWPM_LAYER_INBOUND_MAC_FRAME_NATIVE                                             <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/FF560703.aspx                              <br>
             HTTP://MSDN.Microsoft.com/En-US/Library/FF564527.aspx                              <br>
             HTTP://MSDN.Microsoft.com/En-US/Library/FF562631.aspx                              <br>
*/
VOID PerformBasicPacketExaminationAtInboundMACFrame(_In_ CLASSIFY_DATA* pClassifyData)
{
#if DBG

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> PerformBasicPacketExaminationAtInboundMACFrame()\n");

#endif /// DBG

   NT_ASSERT(pClassifyData);

   NTSTATUS                       status           = STATUS_SUCCESS;
   FWPS_INCOMING_VALUES*          pClassifyValues  = (FWPS_INCOMING_VALUES*)pClassifyData->pClassifyValues;
   FWPS_INCOMING_METADATA_VALUES* pMetadata        = (FWPS_INCOMING_METADATA_VALUES*)pClassifyData->pMetadataValues;
   UINT32                         bytesRetreated   = 0;
   UINT32                         bytesAdvanced    = 0;
   UINT32                         macHeaderSize    = 0;
   UINT32                         ipHeaderSize     = 0;
   UINT16                         etherType        = 0;
   UINT8                          ipProtocol       = 0;
   VOID*                          pHeader          = 0;
   BOOLEAN                        needToFreeMemory = FALSE;

   if(FWPS_IS_L2_METADATA_FIELD_PRESENT(pMetadata,
                                        FWPS_L2_METADATA_FIELD_ETHERNET_MAC_HEADER_SIZE))
      macHeaderSize = pMetadata->ethernetMacHeaderSize;

   if(pClassifyValues->layerId == FWPS_LAYER_INBOUND_MAC_FRAME_ETHERNET)
   {
      FWP_VALUE* pDestinationAddressValue = 0;
      FWP_VALUE* pSourceAddressValue      = 0;
      FWP_VALUE* pEtherTypeValue          = 0;
      FWP_VALUE* pVLANIDValue             = 0;

      pSourceAddressValue = KrnlHlprFwpValueGetFromFwpsIncomingValues(pClassifyValues,
                                                                      &FWPM_CONDITION_MAC_REMOTE_ADDRESS);
      HLPR_BAIL_ON_NULL_POINTER(pSourceAddressValue);

      pDestinationAddressValue = KrnlHlprFwpValueGetFromFwpsIncomingValues(pClassifyValues,
                                                                           &FWPM_CONDITION_MAC_LOCAL_ADDRESS);
      HLPR_BAIL_ON_NULL_POINTER(pDestinationAddressValue);

      pEtherTypeValue = KrnlHlprFwpValueGetFromFwpsIncomingValues(pClassifyValues,
                                                                  &FWPM_CONDITION_ETHER_TYPE);
      HLPR_BAIL_ON_NULL_POINTER(pEtherTypeValue);

      pVLANIDValue = KrnlHlprFwpValueGetFromFwpsIncomingValues(pClassifyValues,
                                                                  &FWPM_CONDITION_VLAN_ID);
      HLPR_BAIL_ON_NULL_POINTER(pVLANIDValue);

      /// Initial offset is at the IP Header, so retreat the size of the MAC Header ...
      status = NdisRetreatNetBufferDataStart(NET_BUFFER_LIST_FIRST_NB((NET_BUFFER_LIST*)pClassifyData->pPacket),
                                             macHeaderSize,
                                             0,
                                             0);
      if(status != STATUS_SUCCESS)
      {
         DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                    DPFLTR_ERROR_LEVEL,
                    " !!!! PerformBasicPacketExaminationAtInboundMACFrame: NdisRetreatNetBufferDataStart() [status: %#x]\n",
                    status);

         HLPR_BAIL;
      }
      else
         bytesRetreated = macHeaderSize;

      status = KrnlHlprMACHeaderGet((NET_BUFFER_LIST*)pClassifyData->pPacket,
                                    &pHeader,
                                    &needToFreeMemory,
                                    macHeaderSize);
      HLPR_BAIL_ON_FAILURE(status);

      if(macHeaderSize == sizeof(ETHERNET_SNAP_HEADER))
      {
         ETHERNET_SNAP_HEADER* pEthernetSNAPHeader = (ETHERNET_SNAP_HEADER*)pHeader;

         NT_ASSERT(ntohs(pEthernetSNAPHeader->type) == pEtherTypeValue->uint16);

         LogEthernetSNAPHeader(pEthernetSNAPHeader);
      }
      else
      {
         ETHERNET_II_HEADER* pEthernetIIHeader = (ETHERNET_II_HEADER*)pHeader;

         NT_ASSERT(RtlCompareMemory(pEthernetIIHeader->pDestinationAddress,
                                    pDestinationAddressValue->byteArray6->byteArray6,
                                    ETHERNET_ADDRESS_SIZE) == ETHERNET_ADDRESS_SIZE);
         NT_ASSERT(RtlCompareMemory(pEthernetIIHeader->pSourceAddress,
                                    pSourceAddressValue->byteArray6->byteArray6,
                                    ETHERNET_ADDRESS_SIZE) == ETHERNET_ADDRESS_SIZE);
         NT_ASSERT(ntohs(pEthernetIIHeader->type) == pEtherTypeValue->uint16);

         LogEthernetIIHeader(pEthernetIIHeader);
      }

      etherType = pEtherTypeValue->uint16;
   }
   else
   {
      FWP_VALUE* pNDISMediaType = 0;

      pNDISMediaType = KrnlHlprFwpValueGetFromFwpsIncomingValues(pClassifyValues,
                                                                 &FWPM_CONDITION_NDIS_MEDIA_TYPE);
      HLPR_BAIL_ON_NULL_POINTER(pNDISMediaType);

      /// Initial offset is at the MAC Header ...
      status = KrnlHlprMACHeaderGet((NET_BUFFER_LIST*)pClassifyData->pPacket,
                                    &pHeader,
                                    &needToFreeMemory,
                                    macHeaderSize);
      HLPR_BAIL_ON_FAILURE(status);

      if(pNDISMediaType->uint32 == NdisMedium802_3)
      {
         ETHERNET_II_HEADER* pEthernetIIHeader = (ETHERNET_II_HEADER*)pHeader;

         etherType = ntohs(pEthernetIIHeader->type);

         if(etherType == NDIS_ETH_TYPE_IPV4 ||
            etherType == NDIS_ETH_TYPE_ARP ||
            etherType == NDIS_ETH_TYPE_IPV6 ||
            etherType == NDIS_ETH_TYPE_802_1X ||
            etherType == NDIS_ETH_TYPE_802_1Q ||
            etherType == NDIS_ETH_TYPE_SLOW_PROTOCOL)
         {
            LogEthernetIIHeader(pEthernetIIHeader);

            if(etherType == NDIS_ETH_TYPE_IPV4 ||
               etherType == NDIS_ETH_TYPE_IPV6)
               macHeaderSize = sizeof(ETHERNET_II_HEADER);
         }
         else
         {
            ETHERNET_SNAP_HEADER* pEthernetSNAPHeader = (ETHERNET_SNAP_HEADER*)pHeader;

            LogEthernetSNAPHeader(pEthernetSNAPHeader);

            etherType = ntohs(pEthernetSNAPHeader->type);

            if(etherType == NDIS_ETH_TYPE_IPV4 ||
               etherType == NDIS_ETH_TYPE_IPV6)
               macHeaderSize = sizeof(ETHERNET_SNAP_HEADER);
         }
      }
   }

   if(macHeaderSize)
   {
      if(needToFreeMemory)
      {
         HLPR_DELETE_ARRAY(pHeader,
                           WFPSAMPLER_SYSLIB_TAG);
      
         needToFreeMemory = FALSE;
      }

      if(bytesRetreated)
      {
         /// ... advance the offset back to the original position.
         NdisAdvanceNetBufferDataStart(NET_BUFFER_LIST_FIRST_NB((NET_BUFFER_LIST*)pClassifyData->pPacket),
                                       macHeaderSize,
                                       FALSE,
                                       0);

         bytesRetreated -= macHeaderSize;
      }
      else
      {
         /// ... advance the offset to the IP Header.
         NdisAdvanceNetBufferDataStart(NET_BUFFER_LIST_FIRST_NB((NET_BUFFER_LIST*)pClassifyData->pPacket),
                                       macHeaderSize,
                                       FALSE,
                                       0);

         bytesAdvanced += macHeaderSize;
      }

      status = KrnlHlprIPHeaderGet((NET_BUFFER_LIST*)pClassifyData->pPacket,
                                    &pHeader,
                                    &needToFreeMemory);
      HLPR_BAIL_ON_FAILURE(status);

      if(etherType == NDIS_ETH_TYPE_IPV4)
      {
         IP_HEADER_V4* pIPv4Header = (IP_HEADER_V4*)pHeader;

         ipProtocol = pIPv4Header->protocol;

         ipHeaderSize = pIPv4Header->headerLength * 4;

         LogIPv4Header(pIPv4Header);
      }
      else if(etherType == NDIS_ETH_TYPE_IPV6)
      {
         IP_HEADER_V6* pIPv6Header = (IP_HEADER_V6*)pHeader;

         ipProtocol = pIPv6Header->nextHeader;

         ipHeaderSize = sizeof(IP_HEADER_V6);

         LogIPv6Header(pIPv6Header);
      }
      else
         HLPR_BAIL;

      if(needToFreeMemory)
      {
         HLPR_DELETE_ARRAY(pHeader,
                           WFPSAMPLER_SYSLIB_TAG);
      
         needToFreeMemory = FALSE;
      }

      if(ipProtocol)
      {
         /// ... advance the offset to the Transport Header.
         NdisAdvanceNetBufferDataStart(NET_BUFFER_LIST_FIRST_NB((NET_BUFFER_LIST*)pClassifyData->pPacket),
                                       ipHeaderSize,
                                       FALSE,
                                       0);

         bytesAdvanced += ipHeaderSize;

         status = KrnlHlprTransportHeaderGet((NET_BUFFER_LIST*)pClassifyData->pPacket,
                                             &pHeader,
                                             &needToFreeMemory);
         HLPR_BAIL_ON_FAILURE(status);

         LogTransportHeader(pHeader,
                            ipProtocol);
      }
   }

   HLPR_BAIL_LABEL:

   if(needToFreeMemory)
   {
      HLPR_DELETE_ARRAY(pHeader,
                        WFPSAMPLER_SYSLIB_TAG);
   
      needToFreeMemory = FALSE;
   }

   if(bytesRetreated)
   {
      /// ... advance the offset back to the original position.
      NdisAdvanceNetBufferDataStart(NET_BUFFER_LIST_FIRST_NB((NET_BUFFER_LIST*)pClassifyData->pPacket),
                                    bytesRetreated,
                                    FALSE,
                                    0);
   }

   if(bytesAdvanced)
   {
      /// ... retreat the offset back to the original position.
      status = NdisRetreatNetBufferDataStart(NET_BUFFER_LIST_FIRST_NB((NET_BUFFER_LIST*)pClassifyData->pPacket),
                                             bytesAdvanced,
                                             0,
                                             0);
      if(status != STATUS_SUCCESS)
         DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                    DPFLTR_ERROR_LEVEL,
                    " !!!! PerformBasicPacketExaminationAtInboundMACFrame: NdisRetreatNetBufferDataStart() [status: %#x]\n",
                    status);
   }

#if DBG

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- PerformBasicPacketExaminationAtInboundMACFrame()\n");

#endif /// DBG

   return;
}

/**
 @private_function="PerformBasicPacketExaminationAtOutboundMACFrame"
 
   Purpose:  Examines and logs the contents of the MAC Header and Ip and Transport Headers if 
             available.                                                                         <br>
                                                                                                <br>
   Notes:    Applies to the following framing layers:                                           <br>
                FWPM_LAYER_OUTBOUND_MAC_FRAME_ETHERNET                                          <br>
                FWPM_LAYER_OUTBOUND_MAC_FRAME_NATIVE                                            <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/FF560703.aspx                              <br>
             HTTP://MSDN.Microsoft.com/En-US/Library/FF564527.aspx                              <br>
             HTTP://MSDN.Microsoft.com/En-US/Library/FF562631.aspx                              <br>
*/
VOID PerformBasicPacketExaminationAtOutboundMACFrame(_In_ CLASSIFY_DATA* pClassifyData)
{
#if DBG

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> PerformBasicPacketExaminationAtOutboundMACFrame()\n");

#endif /// DBG

   NT_ASSERT(pClassifyData);

   NTSTATUS                       status           = STATUS_SUCCESS;
   FWPS_INCOMING_VALUES*          pClassifyValues  = (FWPS_INCOMING_VALUES*)pClassifyData->pClassifyValues;
   FWPS_INCOMING_METADATA_VALUES* pMetadata        = (FWPS_INCOMING_METADATA_VALUES*)pClassifyData->pMetadataValues;
   UINT32                         bytesAdvanced    = 0;
   UINT32                         macHeaderSize    = 0;
   UINT32                         ipHeaderSize     = 0;
   UINT16                         etherType        = 0;
   UINT8                          ipProtocol       = 0;
   VOID*                          pHeader          = 0;
   BOOLEAN                        needToFreeMemory = FALSE;

   if(FWPS_IS_L2_METADATA_FIELD_PRESENT(pMetadata,
                                        FWPS_L2_METADATA_FIELD_ETHERNET_MAC_HEADER_SIZE))
      macHeaderSize = pMetadata->ethernetMacHeaderSize;

   /// Initial offset is at the MAC Header ...
   status = KrnlHlprMACHeaderGet((NET_BUFFER_LIST*)pClassifyData->pPacket,
                                 &pHeader,
                                 &needToFreeMemory,
                                 macHeaderSize);
   HLPR_BAIL_ON_FAILURE(status);

   if(pClassifyValues->layerId == FWPS_LAYER_OUTBOUND_MAC_FRAME_ETHERNET)
   {
      FWP_VALUE* pDestinationAddressValue = 0;
      FWP_VALUE* pSourceAddressValue      = 0;
      FWP_VALUE* pEtherTypeValue          = 0;
      FWP_VALUE* pVLANIDValue             = 0;

      pSourceAddressValue = KrnlHlprFwpValueGetFromFwpsIncomingValues(pClassifyValues,
                                                                      &FWPM_CONDITION_MAC_LOCAL_ADDRESS);
      HLPR_BAIL_ON_NULL_POINTER(pSourceAddressValue);

      pDestinationAddressValue = KrnlHlprFwpValueGetFromFwpsIncomingValues(pClassifyValues,
                                                                           &FWPM_CONDITION_MAC_REMOTE_ADDRESS);
      HLPR_BAIL_ON_NULL_POINTER(pDestinationAddressValue);

      pEtherTypeValue = KrnlHlprFwpValueGetFromFwpsIncomingValues(pClassifyValues,
                                                                  &FWPM_CONDITION_ETHER_TYPE);
      HLPR_BAIL_ON_NULL_POINTER(pEtherTypeValue);

      pVLANIDValue = KrnlHlprFwpValueGetFromFwpsIncomingValues(pClassifyValues,
                                                                  &FWPM_CONDITION_VLAN_ID);
      HLPR_BAIL_ON_NULL_POINTER(pVLANIDValue);

      if(macHeaderSize == sizeof(ETHERNET_SNAP_HEADER))
      {
         ETHERNET_SNAP_HEADER* pEthernetSNAPHeader = (ETHERNET_SNAP_HEADER*)pHeader;

         NT_ASSERT(ntohs(pEthernetSNAPHeader->type) == pEtherTypeValue->uint16);

         LogEthernetSNAPHeader(pEthernetSNAPHeader);
      }
      else
      {
         ETHERNET_II_HEADER* pEthernetIIHeader = (ETHERNET_II_HEADER*)pHeader;

         NT_ASSERT(RtlCompareMemory(pEthernetIIHeader->pDestinationAddress,
                                    pDestinationAddressValue->byteArray6->byteArray6,
                                    ETHERNET_ADDRESS_SIZE) == ETHERNET_ADDRESS_SIZE);
         NT_ASSERT(RtlCompareMemory(pEthernetIIHeader->pSourceAddress,
                                    pSourceAddressValue->byteArray6->byteArray6,
                                    ETHERNET_ADDRESS_SIZE) == ETHERNET_ADDRESS_SIZE);
         NT_ASSERT(ntohs(pEthernetIIHeader->type) == pEtherTypeValue->uint16);

         LogEthernetIIHeader(pEthernetIIHeader,
                             pVLANIDValue->type == FWP_UINT32 ? pVLANIDValue->uint32: 0);
      }

      etherType = pEtherTypeValue->uint16;
   }
   else
   {
      FWP_VALUE* pNDISMediaType = 0;

      pNDISMediaType = KrnlHlprFwpValueGetFromFwpsIncomingValues(pClassifyValues,
                                                                 &FWPM_CONDITION_NDIS_MEDIA_TYPE);
      HLPR_BAIL_ON_NULL_POINTER(pNDISMediaType);

      if(pNDISMediaType->uint32 == NdisMedium802_3)
      {
         ETHERNET_II_HEADER* pEthernetIIHeader = (ETHERNET_II_HEADER*)pHeader;

         etherType = ntohs(pEthernetIIHeader->type);

         if(etherType == NDIS_ETH_TYPE_IPV4 ||
            etherType == NDIS_ETH_TYPE_ARP ||
            etherType == NDIS_ETH_TYPE_IPV6 ||
            etherType == NDIS_ETH_TYPE_802_1X ||
            etherType == NDIS_ETH_TYPE_802_1Q ||
            etherType == NDIS_ETH_TYPE_SLOW_PROTOCOL)
         {
            LogEthernetIIHeader(pEthernetIIHeader);

            if(etherType == NDIS_ETH_TYPE_IPV4 ||
               etherType == NDIS_ETH_TYPE_IPV6)
               macHeaderSize = sizeof(ETHERNET_II_HEADER);
         }
         else
         {
            ETHERNET_SNAP_HEADER* pEthernetSNAPHeader = (ETHERNET_SNAP_HEADER*)pHeader;

            LogEthernetSNAPHeader(pEthernetSNAPHeader);

            etherType = ntohs(pEthernetSNAPHeader->type);

            if(etherType == NDIS_ETH_TYPE_IPV4 ||
               etherType == NDIS_ETH_TYPE_IPV6)
               macHeaderSize = sizeof(ETHERNET_SNAP_HEADER);
         }
      }
   }

   if(macHeaderSize)
   {
      if(needToFreeMemory)
      {
         HLPR_DELETE_ARRAY(pHeader,
                           WFPSAMPLER_SYSLIB_TAG);
      
         needToFreeMemory = FALSE;
      }

      /// ... advance the offset to the IP Header.
      NdisAdvanceNetBufferDataStart(NET_BUFFER_LIST_FIRST_NB((NET_BUFFER_LIST*)pClassifyData->pPacket),
                                    macHeaderSize,
                                    FALSE,
                                    0);

      bytesAdvanced += macHeaderSize;

      status = KrnlHlprIPHeaderGet((NET_BUFFER_LIST*)pClassifyData->pPacket,
                                    &pHeader,
                                    &needToFreeMemory);
      HLPR_BAIL_ON_FAILURE(status);

      if(etherType == NDIS_ETH_TYPE_IPV4)
      {
         IP_HEADER_V4* pIPv4Header = (IP_HEADER_V4*)pHeader;

         ipProtocol = pIPv4Header->protocol;

         ipHeaderSize = pIPv4Header->headerLength * 4;

         LogIPv4Header(pIPv4Header);
      }
      else if(etherType == NDIS_ETH_TYPE_IPV6)
      {
         IP_HEADER_V6* pIPv6Header = (IP_HEADER_V6*)pHeader;

         ipProtocol = pIPv6Header->nextHeader;

         ipHeaderSize = sizeof(IP_HEADER_V6);

         LogIPv6Header(pIPv6Header);
      }
      else
         HLPR_BAIL;

      if(needToFreeMemory)
      {
         HLPR_DELETE_ARRAY(pHeader,
                           WFPSAMPLER_SYSLIB_TAG);
      
         needToFreeMemory = FALSE;
      }

      if(ipProtocol)
      {
         /// ... advance the offset to the Transport Header.
         NdisAdvanceNetBufferDataStart(NET_BUFFER_LIST_FIRST_NB((NET_BUFFER_LIST*)pClassifyData->pPacket),
                                       ipHeaderSize,
                                       FALSE,
                                       0);

         bytesAdvanced += ipHeaderSize;

         status = KrnlHlprTransportHeaderGet((NET_BUFFER_LIST*)pClassifyData->pPacket,
                                             &pHeader,
                                             &needToFreeMemory);
         HLPR_BAIL_ON_FAILURE(status);

         LogTransportHeader(pHeader,
                            ipProtocol);
      }
   }

   HLPR_BAIL_LABEL:

   if(needToFreeMemory)
   {
      HLPR_DELETE_ARRAY(pHeader,
                        WFPSAMPLER_SYSLIB_TAG);
   
      needToFreeMemory = FALSE;
   }

   if(bytesAdvanced)
   {
      /// ... retreat the offset back to the original position.
      status = NdisRetreatNetBufferDataStart(NET_BUFFER_LIST_FIRST_NB((NET_BUFFER_LIST*)pClassifyData->pPacket),
                                             bytesAdvanced,
                                             0,
                                             0);
      if(status != STATUS_SUCCESS)
         DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                    DPFLTR_ERROR_LEVEL,
                    " !!!! PerformBasicPacketExaminationAtOutboundMACFrame: NdisRetreatNetBufferDataStart() [status: %#x]\n",
                    status);
   }

#if DBG

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- PerformBasicPacketExaminationAtOutboundMACFrame()\n");

#endif /// DBG

   return;
}

/**
 @private_function="PerformBasicPacketExaminationAtVSwitchTransport"
 
   Purpose:  Examines and logs the contents of the IP Header and Transport Headers if available.<br>
                                                                                                <br>
   Notes:    Applies to the following vSwitch layers:                                           <br>
                FWPM_LAYER_INGRESS_VSWITCH_ETHERNET                                             <br>
                FWPM_LAYER_EGRESS_VSWITCH_ETHERNET                                              <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/FF560703.aspx                              <br>
             HTTP://MSDN.Microsoft.com/En-US/Library/FF564527.aspx                              <br>
             HTTP://MSDN.Microsoft.com/En-US/Library/FF562631.aspx                              <br>
*/
VOID PerformBasicPacketExaminationAtVSwitchEthernet(_In_ CLASSIFY_DATA* pClassifyData)
{
#if DBG

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> PerformBasicPacketExaminationAtVSwitchEthernet()\n");

#endif /// DBG

   NT_ASSERT(pClassifyData);

   NTSTATUS                       status                   = STATUS_SUCCESS;
   FWPS_INCOMING_VALUES*          pClassifyValues          = (FWPS_INCOMING_VALUES*)pClassifyData->pClassifyValues;
   FWPS_INCOMING_METADATA_VALUES* pMetadata                = (FWPS_INCOMING_METADATA_VALUES*)pClassifyData->pMetadataValues;
   UINT32                         bytesAdvanced            = 0;
   UINT32                         macHeaderSize            = 0;
   UINT32                         ipHeaderSize             = 0;
   UINT16                         etherType                = 0;
   UINT8                          ipProtocol               = 0;
   FWP_VALUE*                     pDestinationAddressValue = 0;
   FWP_VALUE*                     pSourceAddressValue      = 0;
   FWP_VALUE*                     pEtherTypeValue          = 0;
   FWP_VALUE*                     pVLANIDValue             = 0;
   VOID*                          pHeader                  = 0;
   BOOLEAN                        needToFreeMemory         = FALSE;

   if(FWPS_IS_L2_METADATA_FIELD_PRESENT(pMetadata,
                                        FWPS_L2_METADATA_FIELD_ETHERNET_MAC_HEADER_SIZE))
      macHeaderSize = pMetadata->ethernetMacHeaderSize;

   pSourceAddressValue = KrnlHlprFwpValueGetFromFwpsIncomingValues(pClassifyValues,
                                                                   &FWPM_CONDITION_MAC_SOURCE_ADDRESS);
   HLPR_BAIL_ON_NULL_POINTER(pSourceAddressValue);

   pDestinationAddressValue = KrnlHlprFwpValueGetFromFwpsIncomingValues(pClassifyValues,
                                                                        &FWPM_CONDITION_MAC_DESTINATION_ADDRESS);
   HLPR_BAIL_ON_NULL_POINTER(pDestinationAddressValue);

   pEtherTypeValue = KrnlHlprFwpValueGetFromFwpsIncomingValues(pClassifyValues,
                                                               &FWPM_CONDITION_ETHER_TYPE);
   HLPR_BAIL_ON_NULL_POINTER(pEtherTypeValue);

   pVLANIDValue = KrnlHlprFwpValueGetFromFwpsIncomingValues(pClassifyValues,
                                                               &FWPM_CONDITION_VLAN_ID);
   HLPR_BAIL_ON_NULL_POINTER(pVLANIDValue);

   /// Initial offset is at the MAC Header ...
   status = KrnlHlprMACHeaderGet((NET_BUFFER_LIST*)pClassifyData->pPacket,
                                 &pHeader,
                                 &needToFreeMemory,
                                 macHeaderSize);
   HLPR_BAIL_ON_FAILURE(status);

   if(macHeaderSize == sizeof(ETHERNET_SNAP_HEADER))
   {
      ETHERNET_SNAP_HEADER* pEthernetSNAPHeader = (ETHERNET_SNAP_HEADER*)pHeader;

      NT_ASSERT(ntohs(pEthernetSNAPHeader->type) == pEtherTypeValue->uint16);

      LogEthernetSNAPHeader(pEthernetSNAPHeader);
   }
   else
   {
      ETHERNET_II_HEADER* pEthernetIIHeader = (ETHERNET_II_HEADER*)pHeader;

      NT_ASSERT(RtlCompareMemory(pEthernetIIHeader->pDestinationAddress,
                                 pDestinationAddressValue->byteArray6->byteArray6,
                                 ETHERNET_ADDRESS_SIZE) == ETHERNET_ADDRESS_SIZE);
      NT_ASSERT(RtlCompareMemory(pEthernetIIHeader->pSourceAddress,
                                 pSourceAddressValue->byteArray6->byteArray6,
                                 ETHERNET_ADDRESS_SIZE) == ETHERNET_ADDRESS_SIZE);
      NT_ASSERT(ntohs(pEthernetIIHeader->type) == pEtherTypeValue->uint16);

      LogEthernetIIHeader(pEthernetIIHeader,
                          pVLANIDValue->type == FWP_UINT32 ? pVLANIDValue->uint32: 0);
   }

   etherType = pEtherTypeValue->uint16;

   if(needToFreeMemory)
   {
      HLPR_DELETE_ARRAY(pHeader,
                        WFPSAMPLER_SYSLIB_TAG);
   
      needToFreeMemory = FALSE;
   }

   /// ... advance the offset to the IP Header ...
   NdisAdvanceNetBufferDataStart(NET_BUFFER_LIST_FIRST_NB((NET_BUFFER_LIST*)pClassifyData->pPacket),
                                 macHeaderSize,
                                 FALSE,
                                 0);

   bytesAdvanced += macHeaderSize;

   status = KrnlHlprIPHeaderGet((NET_BUFFER_LIST*)pClassifyData->pPacket,
                                &pHeader,
                                &needToFreeMemory);
   HLPR_BAIL_ON_FAILURE(status);

   if(etherType == NDIS_ETH_TYPE_IPV4)
   {
      IP_HEADER_V4* pIPv4Header = (IP_HEADER_V4*)pHeader;

      ipProtocol = pIPv4Header->protocol;

      ipHeaderSize = pIPv4Header->headerLength * 4;

      LogIPv4Header(pIPv4Header);
   }
   else if(etherType == NDIS_ETH_TYPE_IPV6)
   {
      IP_HEADER_V6* pIPv6Header = (IP_HEADER_V6*)pHeader;

      ipProtocol = pIPv6Header->nextHeader;

      ipHeaderSize = sizeof(IP_HEADER_V6);

      LogIPv6Header(pIPv6Header);
   }
   else
      HLPR_BAIL;

   if(needToFreeMemory)
   {
      HLPR_DELETE_ARRAY(pHeader,
                        WFPSAMPLER_SYSLIB_TAG);
   
      needToFreeMemory = FALSE;
   }

   if(ipProtocol)
   {
      /// ... advance the offset to the Transport Header.
      NdisAdvanceNetBufferDataStart(NET_BUFFER_LIST_FIRST_NB((NET_BUFFER_LIST*)pClassifyData->pPacket),
                                    ipHeaderSize,
                                    FALSE,
                                    0);

      bytesAdvanced += ipHeaderSize;

      status = KrnlHlprTransportHeaderGet((NET_BUFFER_LIST*)pClassifyData->pPacket,
                                          &pHeader,
                                          &needToFreeMemory);
      HLPR_BAIL_ON_FAILURE(status);

      LogTransportHeader(pHeader,
                         ipProtocol);
   }

   HLPR_BAIL_LABEL:

   if(needToFreeMemory)
   {
      HLPR_DELETE_ARRAY(pHeader,
                        WFPSAMPLER_SYSLIB_TAG);
   
      needToFreeMemory = FALSE;
   }

   if(bytesAdvanced)
   {
      /// ... retreat the offset back to the original position.
      status = NdisRetreatNetBufferDataStart(NET_BUFFER_LIST_FIRST_NB((NET_BUFFER_LIST*)pClassifyData->pPacket),
                                             bytesAdvanced,
                                             0,
                                             0);
      if(status != STATUS_SUCCESS)
         DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                    DPFLTR_ERROR_LEVEL,
                    " !!!! PerformBasicPacketExaminationAtVSwitchEthernet: NdisRetreatNetBufferDataStart() [status: %#x]\n",
                    status);
   }

#if DBG

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- PerformBasicPacketExaminationAtVSwitchEthernet()\n");

#endif /// DBG

   return;
}

/**
 @private_function="PerformBasicPacketExaminationAtVSwitchTransport"
 
   Purpose:  Examines and logs the contents of the IP Header and Transport Headers if available.<br>
                                                                                                <br>
   Notes:    Applies to the following vSwitch layers:                                           <br>
                FWPM_LAYER_INGRESS_VSWITCH_TRANSPORT_V4                                         <br>
                FWPM_LAYER_INGRESS_VSWITCH_TRANSPORT_V6                                         <br>
                FWPM_LAYER_EGRESS_VSWITCH_TRANSPORT_V4                                          <br>
                FWPM_LAYER_EGRESS_VSWITCH_TRANSPORT_V6                                          <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/FF560703.aspx                              <br>
             HTTP://MSDN.Microsoft.com/En-US/Library/FF564527.aspx                              <br>
             HTTP://MSDN.Microsoft.com/En-US/Library/FF562631.aspx                              <br>
*/
VOID PerformBasicPacketExaminationAtVSwitchTransport(_In_ CLASSIFY_DATA* pClassifyData)
{
#if DBG

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> PerformBasicPacketExaminationAtVSwitchTransport()\n");

#endif /// DBG

   NT_ASSERT(pClassifyData);

   NTSTATUS                       status                   = STATUS_SUCCESS;
   FWPS_INCOMING_VALUES*          pClassifyValues          = (FWPS_INCOMING_VALUES*)pClassifyData->pClassifyValues;
   FWPS_INCOMING_METADATA_VALUES* pMetadata                = (FWPS_INCOMING_METADATA_VALUES*)pClassifyData->pMetadataValues;
   UINT32                         ipHeaderSize             = 0;
   UINT32                         bytesAdvanced            = 0;
   FWP_VALUE*                     pProtocolValue           = 0;
   FWP_VALUE*                     pSourceAddressValue      = 0;
   FWP_VALUE*                     pDestinationAddressValue = 0;
   FWP_VALUE*                     pSourcePortValue         = 0;
   FWP_VALUE*                     pDestinationPortValue    = 0;
   FWP_VALUE*                     pICMPTypeValue           = 0;
   FWP_VALUE*                     pICMPCodeValue           = 0;
   VOID*                          pHeader                  = 0;
   BOOLEAN                        needToFreeMemory         = FALSE;

   pProtocolValue = KrnlHlprFwpValueGetFromFwpsIncomingValues(pClassifyValues,
                                                              &FWPM_CONDITION_IP_PROTOCOL);
   HLPR_BAIL_ON_NULL_POINTER(pProtocolValue);

   pSourceAddressValue = KrnlHlprFwpValueGetFromFwpsIncomingValues(pClassifyValues,
                                                                   &FWPM_CONDITION_IP_SOURCE_ADDRESS);
   HLPR_BAIL_ON_NULL_POINTER(pSourceAddressValue);

   pDestinationAddressValue = KrnlHlprFwpValueGetFromFwpsIncomingValues(pClassifyValues,
                                                                        &FWPM_CONDITION_IP_DESTINATION_ADDRESS);
   HLPR_BAIL_ON_NULL_POINTER(pDestinationAddressValue);

   pSourcePortValue = pICMPTypeValue = KrnlHlprFwpValueGetFromFwpsIncomingValues(pClassifyValues,
                                                                                 &FWPM_CONDITION_IP_SOURCE_PORT);
   HLPR_BAIL_ON_NULL_POINTER(pSourcePortValue);

   pDestinationPortValue = pICMPCodeValue =  KrnlHlprFwpValueGetFromFwpsIncomingValues(pClassifyValues,
                                                                                       &FWPM_CONDITION_IP_DESTINATION_PORT);
   HLPR_BAIL_ON_NULL_POINTER(pDestinationPortValue);

   if(FWPS_IS_METADATA_FIELD_PRESENT(pMetadata,
                                     FWPS_METADATA_FIELD_IP_HEADER_SIZE))
      ipHeaderSize = pMetadata->ipHeaderSize;

   /// Initial offset is at the IP Header ...
   status = KrnlHlprIPHeaderGet((NET_BUFFER_LIST*)pClassifyData->pPacket,
                                &pHeader,
                                &needToFreeMemory,
                                ipHeaderSize);
   HLPR_BAIL_ON_FAILURE(status);

   /// Validate that what is indicated in the classify is what is present in packet's IP Header
   if(KrnlHlprFwpsLayerIsIPv4(pClassifyValues->layerId))
   {
      IP_HEADER_V4* pIPv4Header = (IP_HEADER_V4*)pHeader;

      NT_ASSERT(pIPv4Header->version == IPV4);
      NT_ASSERT(pIPv4Header->protocol == pProtocolValue->uint8);
      NT_ASSERT(ntohl(*((UINT32*)pIPv4Header->pSourceAddress)) == pSourceAddressValue->uint32);
      NT_ASSERT(ntohl(*((UINT32*)pIPv4Header->pDestinationAddress)) == pDestinationAddressValue->uint32);

      LogIPv4Header(pIPv4Header);

      if(ipHeaderSize == 0)
         ipHeaderSize = IPV4_HEADER_MIN_SIZE;
   }
   else
   {
      IP_HEADER_V6* pIPv6Header = (IP_HEADER_V6*)pHeader;

      NT_ASSERT(pIPv6Header->version.value == IPV6);
      NT_ASSERT(pIPv6Header->nextHeader == pProtocolValue->uint8);
      NT_ASSERT(RtlCompareMemory(pIPv6Header->pSourceAddress,
                                 pSourceAddressValue->byteArray16->byteArray16,
                                 IPV6_ADDRESS_SIZE) == IPV6_ADDRESS_SIZE);
      NT_ASSERT(RtlCompareMemory(pIPv6Header->pDestinationAddress,
                                 pDestinationAddressValue->byteArray16->byteArray16,
                                 IPV6_ADDRESS_SIZE) == IPV6_ADDRESS_SIZE);

      LogIPv6Header(pIPv6Header);

      if(ipHeaderSize == 0)
         ipHeaderSize = IPV6_HEADER_MIN_SIZE;
   }

   if(needToFreeMemory)
   {
      HLPR_DELETE_ARRAY(pHeader,
                        WFPSAMPLER_SYSLIB_TAG);
   
      needToFreeMemory = FALSE;
   }

   /// ... advance the offset to the Transport Header.
   NdisAdvanceNetBufferDataStart(NET_BUFFER_LIST_FIRST_NB((NET_BUFFER_LIST*)pClassifyData->pPacket),
                                 ipHeaderSize,
                                 FALSE,
                                 0);

   bytesAdvanced += ipHeaderSize;

   status = KrnlHlprTransportHeaderGet((NET_BUFFER_LIST*)pClassifyData->pPacket,
                                       &pHeader,
                                       &needToFreeMemory);
   HLPR_BAIL_ON_FAILURE(status);

   switch(pProtocolValue->uint8)
   {
      case ICMPV4:
      {
         ICMP_HEADER_V4* pICMPv4Header = (ICMP_HEADER_V4*)pHeader;

         NT_ASSERT(pICMPv4Header->type == pICMPTypeValue->uint16);
         NT_ASSERT(pICMPv4Header->code == pICMPCodeValue->uint16);

         LogICMPv4Header(pICMPv4Header);

         break;
      }
      case ICMPV6:
      {
         ICMP_HEADER_V6* pICMPv6Header = (ICMP_HEADER_V6*)pHeader;

         NT_ASSERT(pICMPv6Header->type == pICMPTypeValue->uint16);
         NT_ASSERT(pICMPv6Header->code == pICMPCodeValue->uint16);

         LogICMPv6Header(pICMPv6Header);

         break;
      }
      case TCP:
      {
         TCP_HEADER* pTCPHeader = (TCP_HEADER*)pHeader;

         NT_ASSERT(ntohs(pTCPHeader->sourcePort) == pSourcePortValue->uint16 );
         NT_ASSERT(ntohs(pTCPHeader->destinationPort) == pDestinationPortValue->uint16);

         LogTCPHeader(pTCPHeader);

         break;
      }
      case UDP:
      {
         UDP_HEADER* pUDPHeader = (UDP_HEADER*)pHeader;

         NT_ASSERT(ntohs(pUDPHeader->sourcePort) == pSourcePortValue->uint16 );
         NT_ASSERT(ntohs(pUDPHeader->destinationPort) == pDestinationPortValue->uint16);

         LogUDPHeader(pUDPHeader);

         break;
      }
   }

   HLPR_BAIL_LABEL:

   if(needToFreeMemory)
   {
      HLPR_DELETE_ARRAY(pHeader,
                        WFPSAMPLER_SYSLIB_TAG);
   
      needToFreeMemory = FALSE;
   }

   if(bytesAdvanced)
   {
      /// ... retreat the offset to the original position.
      status = NdisRetreatNetBufferDataStart(NET_BUFFER_LIST_FIRST_NB((NET_BUFFER_LIST*)pClassifyData->pPacket),
                                             bytesAdvanced,
                                             0,
                                             0);
      if(status != STATUS_SUCCESS)
         DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                    DPFLTR_ERROR_LEVEL,
                    " !!!! PerformBasicPacketExaminationAtInboundMACFrame: NdisRetreatNetBufferDataStart() [status: %#x]\n",
                    status);
   }

#if DBG

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- PerformBasicPacketExaminationAtVSwitchTransport()\n");

#endif /// DBG

   return;
}

#endif /// (NTDDI_VERSION >= NTDDI_WIN8)

/**
 @private_function="PerformBasicPacketExaminationAtInboundNetwork"
 
   Purpose:  Examines and logs the contents of the IP Header and Transport Header if available. <br>
                                                                                                <br>
   Notes:    Applies to the following network layers:                                           <br>
                FWPM_LAYER_INBOUND_IPPACKET_V{4/6}                                              <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/FF560703.aspx                              <br>
             HTTP://MSDN.Microsoft.com/En-US/Library/FF564527.aspx                              <br>
             HTTP://MSDN.Microsoft.com/En-US/Library/FF562631.aspx                              <br>
*/
VOID PerformBasicPacketExaminationAtInboundNetwork(_In_ CLASSIFY_DATA* pClassifyData)
{
#if DBG

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> PerformBasicPacketExaminationAtInboundNetwork()\n");

#endif /// DBG

   NT_ASSERT(pClassifyData);

   NTSTATUS                       status                   = STATUS_SUCCESS;
   FWPS_INCOMING_VALUES*          pClassifyValues          = (FWPS_INCOMING_VALUES*)pClassifyData->pClassifyValues;
   FWPS_INCOMING_METADATA_VALUES* pMetadata                = (FWPS_INCOMING_METADATA_VALUES*)pClassifyData->pMetadataValues;
   UINT32                         ipHeaderSize             = 0;
   UINT32                         bytesRetreated           = 0;
   FWP_VALUE*                     pSourceAddressValue      = 0;
   FWP_VALUE*                     pDestinationAddressValue = 0;
   VOID*                          pHeader                  = 0;
   BOOLEAN                        needToFreeMemory         = FALSE;
   UINT8                          protocol                 = IPPROTO_RAW;

   pSourceAddressValue = KrnlHlprFwpValueGetFromFwpsIncomingValues(pClassifyValues,
                                                                   &FWPM_CONDITION_IP_REMOTE_ADDRESS);
   HLPR_BAIL_ON_NULL_POINTER(pSourceAddressValue);

   pDestinationAddressValue = KrnlHlprFwpValueGetFromFwpsIncomingValues(pClassifyValues,
                                                                        &FWPM_CONDITION_IP_LOCAL_ADDRESS);
   HLPR_BAIL_ON_NULL_POINTER(pDestinationAddressValue);

   if(FWPS_IS_METADATA_FIELD_PRESENT(pMetadata,
                                     FWPS_METADATA_FIELD_IP_HEADER_SIZE))
      ipHeaderSize = pMetadata->ipHeaderSize;

   /// Initial offset is at the Transport Header, so retreat the size of the IP Header ...
   status = NdisRetreatNetBufferDataStart(NET_BUFFER_LIST_FIRST_NB((NET_BUFFER_LIST*)pClassifyData->pPacket),
                                          ipHeaderSize,
                                          0,
                                          0);
   if(status != STATUS_SUCCESS)
   {
      bytesRetreated = 0;

      DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                 DPFLTR_ERROR_LEVEL,
                 " !!!! PerformBasicPacketExaminationAtInboundNetwork: NdisRetreatNetBufferDataStart() [status: %#x]\n",
                 status);

      HLPR_BAIL;
   }
   else
      bytesRetreated = ipHeaderSize;

   status = KrnlHlprIPHeaderGet((NET_BUFFER_LIST*)pClassifyData->pPacket,
                                &pHeader,
                                &needToFreeMemory,
                                ipHeaderSize);
   HLPR_BAIL_ON_FAILURE(status);

   /// Validate that what is indicated in the classify is what is present in packet's IP Header
   if(KrnlHlprFwpsLayerIsIPv4(pClassifyValues->layerId))
   {
      IP_HEADER_V4* pIPv4Header = (IP_HEADER_V4*)pHeader;

      NT_ASSERT(pIPv4Header->version == IPV4);
      NT_ASSERT(((UINT32)(pIPv4Header->headerLength * 4)) == ipHeaderSize);
      NT_ASSERT(ntohl(*((UINT32*)pIPv4Header->pSourceAddress)) == pSourceAddressValue->uint32);
      NT_ASSERT(ntohl(*((UINT32*)pIPv4Header->pDestinationAddress)) == pDestinationAddressValue->uint32);

      LogIPv4Header(pIPv4Header);

      protocol = pIPv4Header->protocol;
   }
   else
   {
      IP_HEADER_V6* pIPv6Header = (IP_HEADER_V6*)pHeader;

      NT_ASSERT(sizeof(IP_HEADER_V6) == ipHeaderSize);
      NT_ASSERT(pIPv6Header->version.value == IPV6);
      NT_ASSERT(RtlCompareMemory(pIPv6Header->pSourceAddress,
                                 pSourceAddressValue->byteArray16->byteArray16,
                                 IPV6_ADDRESS_SIZE) == IPV6_ADDRESS_SIZE);
      NT_ASSERT(RtlCompareMemory(pIPv6Header->pDestinationAddress,
                                 pDestinationAddressValue->byteArray16->byteArray16,
                                 IPV6_ADDRESS_SIZE) == IPV6_ADDRESS_SIZE);

      LogIPv6Header(pIPv6Header);

      protocol = pIPv6Header->nextHeader;
   }

   if(needToFreeMemory)
   {
      HLPR_DELETE_ARRAY(pHeader,
                        WFPSAMPLER_SYSLIB_TAG);

      needToFreeMemory = FALSE;
   }

   if(bytesRetreated)
   {
      /// ... and advance the offset back to the original position.
      NdisAdvanceNetBufferDataStart(NET_BUFFER_LIST_FIRST_NB((NET_BUFFER_LIST*)pClassifyData->pPacket),
                                    bytesRetreated,
                                    FALSE,
                                    0);

      bytesRetreated -= ipHeaderSize;
   }

   status = KrnlHlprTransportHeaderGet((NET_BUFFER_LIST*)pClassifyData->pPacket,
                                       &pHeader,
                                       &needToFreeMemory);
   HLPR_BAIL_ON_FAILURE(status);

   LogTransportHeader(pHeader,
                      protocol);

   HLPR_BAIL_LABEL:

   if(needToFreeMemory)
   {
      HLPR_DELETE_ARRAY(pHeader,
                        WFPSAMPLER_SYSLIB_TAG);

      needToFreeMemory = FALSE;
   }

   if(bytesRetreated)
   {
      /// ... and advance the offset back to the original position.
      NdisAdvanceNetBufferDataStart(NET_BUFFER_LIST_FIRST_NB((NET_BUFFER_LIST*)pClassifyData->pPacket),
                                    bytesRetreated,
                                    FALSE,
                                    0);
   }

#if DBG

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- PerformBasicPacketExaminationAtInboundNetwork()\n");

#endif /// DBG

   return;
}

/**
 @private_function="PerformBasicPacketExaminationAtOutboundNetwork"
 
   Purpose:  Examines and logs the contents of the IP Header and Transport Header if available. <br>
                                                                                                <br>
   Notes:    Applies to the following network layers:                                           <br>
                FWPM_LAYER_OUTBOUND_IPPACKET_V{4/6}                                             <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/FF560703.aspx                              <br>
             HTTP://MSDN.Microsoft.com/En-US/Library/FF564527.aspx                              <br>
             HTTP://MSDN.Microsoft.com/En-US/Library/FF562631.aspx                              <br>
*/
VOID PerformBasicPacketExaminationAtOutboundNetwork(_In_ CLASSIFY_DATA* pClassifyData)
{
#if DBG

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> PerformBasicPacketExaminationAtOutboundNetwork()\n");

#endif /// DBG

   NT_ASSERT(pClassifyData);

   NTSTATUS                       status                   = STATUS_SUCCESS;
   FWPS_INCOMING_VALUES*          pClassifyValues          = (FWPS_INCOMING_VALUES*)pClassifyData->pClassifyValues;
   FWPS_INCOMING_METADATA_VALUES* pMetadata                = (FWPS_INCOMING_METADATA_VALUES*)pClassifyData->pMetadataValues;
   UINT32                         ipHeaderSize             = 0;
   UINT32                         bytesAdvanced            = 0;
   FWP_VALUE*                     pSourceAddressValue      = 0;
   FWP_VALUE*                     pDestinationAddressValue = 0;
   VOID*                          pHeader                  = 0;
   BOOLEAN                        needToFreeMemory         = FALSE;
   UINT8                          protocol                 = IPPROTO_RAW;

   pSourceAddressValue = KrnlHlprFwpValueGetFromFwpsIncomingValues(pClassifyValues,
                                                                   &FWPM_CONDITION_IP_LOCAL_ADDRESS);
   HLPR_BAIL_ON_NULL_POINTER(pSourceAddressValue);

   pDestinationAddressValue = KrnlHlprFwpValueGetFromFwpsIncomingValues(pClassifyValues,
                                                                        &FWPM_CONDITION_IP_REMOTE_ADDRESS);
   HLPR_BAIL_ON_NULL_POINTER(pDestinationAddressValue);

   if(FWPS_IS_METADATA_FIELD_PRESENT(pMetadata,
                                     FWPS_METADATA_FIELD_IP_HEADER_SIZE))
      ipHeaderSize = pMetadata->ipHeaderSize;

   /// Initial offset is at the IP Header ...
   status = KrnlHlprIPHeaderGet((NET_BUFFER_LIST*)pClassifyData->pPacket,
                                &pHeader,
                                &needToFreeMemory,
                                ipHeaderSize);
   HLPR_BAIL_ON_FAILURE(status);

   /// Validate that what is indicated in the classify is what is present in packet's IP Header
   if(KrnlHlprFwpsLayerIsIPv4(pClassifyValues->layerId))
   {
      IP_HEADER_V4* pIPv4Header = (IP_HEADER_V4*)pHeader;

      NT_ASSERT(pIPv4Header->version == IPV4);
      NT_ASSERT(((UINT32)(pIPv4Header->headerLength * 4)) == ipHeaderSize);
      NT_ASSERT(ntohl(*((UINT32*)pIPv4Header->pSourceAddress)) == pSourceAddressValue->uint32);
      NT_ASSERT(ntohl(*((UINT32*)pIPv4Header->pDestinationAddress)) == pDestinationAddressValue->uint32);

      LogIPv4Header(pIPv4Header);

      protocol = pIPv4Header->protocol;
   }
   else
   {
      IP_HEADER_V6* pIPv6Header = (IP_HEADER_V6*)pHeader;

      NT_ASSERT(sizeof(IP_HEADER_V6) == ipHeaderSize);
      NT_ASSERT(pIPv6Header->version.value == IPV6);
      NT_ASSERT(RtlCompareMemory(pIPv6Header->pSourceAddress,
                                 pSourceAddressValue->byteArray16->byteArray16,
                                 IPV6_ADDRESS_SIZE) == IPV6_ADDRESS_SIZE);
      NT_ASSERT(RtlCompareMemory(pIPv6Header->pDestinationAddress,
                                 pDestinationAddressValue->byteArray16->byteArray16,
                                 IPV6_ADDRESS_SIZE) == IPV6_ADDRESS_SIZE);

      LogIPv6Header(pIPv6Header);

      protocol = pIPv6Header->nextHeader;
   }

   if(needToFreeMemory)
   {
      HLPR_DELETE_ARRAY(pHeader,
                        WFPSAMPLER_SYSLIB_TAG);

      needToFreeMemory = FALSE;
   }

   /// ... advance the offset to the Transport Header ...
   NdisAdvanceNetBufferDataStart(NET_BUFFER_LIST_FIRST_NB((NET_BUFFER_LIST*)pClassifyData->pPacket),
                                 ipHeaderSize,
                                 FALSE,
                                 0);

   bytesAdvanced = ipHeaderSize;

   status = KrnlHlprTransportHeaderGet((NET_BUFFER_LIST*)pClassifyData->pPacket,
                                       &pHeader,
                                       &needToFreeMemory);
   HLPR_BAIL_ON_FAILURE(status);

   LogTransportHeader(pHeader,
                      protocol);

   HLPR_BAIL_LABEL:

   if(needToFreeMemory)
   {
      HLPR_DELETE_ARRAY(pHeader,
                        WFPSAMPLER_SYSLIB_TAG);

      needToFreeMemory = FALSE;
   }

   if(bytesAdvanced)
   {
      /// ... and retreat the offset back to the original position.
      status = NdisRetreatNetBufferDataStart(NET_BUFFER_LIST_FIRST_NB((NET_BUFFER_LIST*)pClassifyData->pPacket),
                                             bytesAdvanced,
                                             0,
                                             0);
      if(status != STATUS_SUCCESS)
         DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                    DPFLTR_ERROR_LEVEL,
                    " !!!! PerformBasicPacketExaminationAtOutboundNetwork : NdisRetreatNetBufferDataStart() [status: %#x]\n",
                    status);
   }

#if DBG

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- PerformBasicPacketExaminationAtOutboundNetwork()\n");

#endif /// DBG

   return;
}

/**
 @private_function="PerformBasicPacketExaminationAtForward"
 
   Purpose:  Examines and logs the contents of the IP Header and Transport Header if available. <br>
                                                                                                <br>
   Notes:    Applies to the following forwarding layers:                                        <br>
                FWPM_LAYER_IPFORWARD_V{4/6}                                                     <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/FF560703.aspx                              <br>
             HTTP://MSDN.Microsoft.com/En-US/Library/FF564527.aspx                              <br>
             HTTP://MSDN.Microsoft.com/En-US/Library/FF562631.aspx                              <br>
*/
VOID PerformBasicPacketExaminationAtForward(_In_ CLASSIFY_DATA* pClassifyData)
{
#if DBG

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> PerformBasicPacketExaminationAtForward()\n");

#endif /// DBG

   NT_ASSERT(pClassifyData);

   NTSTATUS                       status                   = STATUS_SUCCESS;
   FWPS_INCOMING_VALUES*          pClassifyValues          = (FWPS_INCOMING_VALUES*)pClassifyData->pClassifyValues;
   FWPS_INCOMING_METADATA_VALUES* pMetadata                = (FWPS_INCOMING_METADATA_VALUES*)pClassifyData->pMetadataValues;
   UINT32                         ipHeaderSize             = 0;
   UINT32                         bytesAdvanced            = 0;
   FWP_VALUE*                     pSourceAddressValue      = 0;
   FWP_VALUE*                     pDestinationAddressValue = 0;
   VOID*                          pHeader                  = 0;
   BOOLEAN                        needToFreeMemory         = FALSE;
   UINT8                          protocol                 = IPPROTO_RAW;

   pSourceAddressValue = KrnlHlprFwpValueGetFromFwpsIncomingValues(pClassifyValues,
                                                                   &FWPM_CONDITION_IP_SOURCE_ADDRESS);
   HLPR_BAIL_ON_NULL_POINTER(pSourceAddressValue);

   pDestinationAddressValue = KrnlHlprFwpValueGetFromFwpsIncomingValues(pClassifyValues,
                                                                        &FWPM_CONDITION_IP_DESTINATION_ADDRESS);
   HLPR_BAIL_ON_NULL_POINTER(pDestinationAddressValue);

   if(FWPS_IS_METADATA_FIELD_PRESENT(pMetadata,
                                     FWPS_METADATA_FIELD_IP_HEADER_SIZE))
      ipHeaderSize = pMetadata->ipHeaderSize;

   /// Initial offset is at the IP Header ...
   status = KrnlHlprIPHeaderGet((NET_BUFFER_LIST*)pClassifyData->pPacket,
                                &pHeader,
                                &needToFreeMemory,
                                ipHeaderSize);
   HLPR_BAIL_ON_FAILURE(status);

   /// Validate that what is indicated in the classify is what is present in packet's IP Header
   if(KrnlHlprFwpsLayerIsIPv4(pClassifyValues->layerId))
   {
      IP_HEADER_V4* pIPv4Header = (IP_HEADER_V4*)pHeader;

      NT_ASSERT(pIPv4Header->version == IPV4);
      NT_ASSERT(ntohl(*((UINT32*)pIPv4Header->pSourceAddress)) == pSourceAddressValue->uint32);
      NT_ASSERT(ntohl(*((UINT32*)pIPv4Header->pDestinationAddress)) == pDestinationAddressValue->uint32);

      LogIPv4Header(pIPv4Header);

      protocol = pIPv4Header->protocol;
   }
   else
   {
      IP_HEADER_V6* pIPv6Header = (IP_HEADER_V6*)pHeader;

      NT_ASSERT(pIPv6Header->version.value == IPV6);
      NT_ASSERT(RtlCompareMemory(pIPv6Header->pSourceAddress,
                                 pSourceAddressValue->byteArray16->byteArray16,
                                 IPV6_ADDRESS_SIZE) == IPV6_ADDRESS_SIZE);
      NT_ASSERT(RtlCompareMemory(pIPv6Header->pDestinationAddress,
                                 pDestinationAddressValue->byteArray16->byteArray16,
                                 IPV6_ADDRESS_SIZE) == IPV6_ADDRESS_SIZE);

      LogIPv6Header(pIPv6Header);

      protocol = pIPv6Header->nextHeader;
   }

   if(needToFreeMemory)
   {
      HLPR_DELETE_ARRAY(pHeader,
                        WFPSAMPLER_SYSLIB_TAG);

      needToFreeMemory = FALSE;
   }

   /// ... advance the offset to the Transport Header ...
   NdisAdvanceNetBufferDataStart(NET_BUFFER_LIST_FIRST_NB((NET_BUFFER_LIST*)pClassifyData->pPacket),
                                 ipHeaderSize,
                                 FALSE,
                                 0);

   bytesAdvanced = ipHeaderSize;

   status = KrnlHlprTransportHeaderGet((NET_BUFFER_LIST*)pClassifyData->pPacket,
                                       &pHeader,
                                       &needToFreeMemory);
   HLPR_BAIL_ON_FAILURE(status);

   LogTransportHeader(pHeader,
                      protocol);

   HLPR_BAIL_LABEL:

   if(needToFreeMemory)
   {
      HLPR_DELETE_ARRAY(pHeader,
                        WFPSAMPLER_SYSLIB_TAG);

      needToFreeMemory = FALSE;
   }

   if(bytesAdvanced)
   {
      /// ... and retreat the offset back to the original position.
      status = NdisRetreatNetBufferDataStart(NET_BUFFER_LIST_FIRST_NB((NET_BUFFER_LIST*)pClassifyData->pPacket),
                                             bytesAdvanced,
                                             0,
                                             0);
      if(status != STATUS_SUCCESS)
         DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                    DPFLTR_ERROR_LEVEL,
                    " !!!! PerformBasicPacketExaminationAtForward : NdisRetreatNetBufferDataStart() [status: %#x]\n",
                    status);
   }

#if DBG

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- PerformBasicPacketExaminationAtForward()\n");

#endif /// DBG

   return;
}

/**
 @private_function="PerformBasicPacketExaminationAtInboundTransport"
 
   Purpose:  Examines and logs the contents of the IP Header and Transport Header if available. <br>
                                                                                                <br>
   Notes:    Applies to the following transport layers:                                         <br>
                FWPM_LAYER_INBOUND_TRANSPORT_V{4/6}                                             <br>
                FWPM_LAYER_INBOUND_ICMP_ERROR_V{4/6}                                            <br>
                FWPM_LAYER_DATAGRAM_DATA_V{4/6}        (Inbound only)                           <br>
                FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V{4/6} (Inbound only)                           <br>
                FWPM_LAYER_ALE_AUTH_CONNECT_V{4/6}     (Inbound, reauthorization only)          <br>
                FWPM_LAYER_ALE_FLOW_ESTABLISHED_V{4/6} (Inbound, non-TCP only)                  <br>
                FWPM_LAYER_STREAM_PACKET_V{4/6}        (Inbound only)                           <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/FF560703.aspx                              <br>
             HTTP://MSDN.Microsoft.com/En-US/Library/FF564527.aspx                              <br>
             HTTP://MSDN.Microsoft.com/En-US/Library/FF562631.aspx                              <br>
*/
VOID PerformBasicPacketExaminationAtInboundTransport(_In_ CLASSIFY_DATA* pClassifyData)
{
#if DBG

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> PerformBasicPacketExaminationAtInboundTransport()\n");

#endif /// DBG

   NT_ASSERT(pClassifyData);

   NTSTATUS                       status                   = STATUS_SUCCESS;
   FWPS_INCOMING_VALUES*          pClassifyValues          = (FWPS_INCOMING_VALUES*)pClassifyData->pClassifyValues;
   FWPS_INCOMING_METADATA_VALUES* pMetadata                = (FWPS_INCOMING_METADATA_VALUES*)pClassifyData->pMetadataValues;
   UINT32                         ipHeaderSize             = 0;
   UINT32                         transportHeaderSize      = 0;
   UINT32                         bytesRetreated           = 0;
   UINT8                          protocol                 = IPPROTO_RAW;
   FWP_VALUE*                     pProtocolValue           = 0;
   FWP_VALUE*                     pSourceAddressValue      = 0;
   FWP_VALUE*                     pDestinationAddressValue = 0;
   FWP_VALUE*                     pSourcePortValue         = 0;
   FWP_VALUE*                     pDestinationPortValue    = 0;
   FWP_VALUE*                     pICMPTypeValue           = 0;
   FWP_VALUE*                     pICMPCodeValue           = 0;
   VOID*                          pHeader                  = 0;
   BOOLEAN                        needToFreeMemory         = FALSE;

   if(pClassifyValues->layerId == FWPS_LAYER_INBOUND_ICMP_ERROR_V4)
      protocol = ICMPV4;
   else if(pClassifyValues->layerId == FWPS_LAYER_INBOUND_ICMP_ERROR_V6)
      protocol = ICMPV6;

#if(NTDDI_VERSION >= NTDDI_WIN7)

   else if(pClassifyValues->layerId == FWPS_LAYER_STREAM_PACKET_V4 ||
           pClassifyValues->layerId == FWPS_LAYER_STREAM_PACKET_V6)
      protocol = TCP;

#endif /// (NTDDI_VERSION >= NTDDI_WIN7)

   else
   {
      pProtocolValue = KrnlHlprFwpValueGetFromFwpsIncomingValues(pClassifyValues,
                                                                 &FWPM_CONDITION_IP_PROTOCOL);
      HLPR_BAIL_ON_NULL_POINTER(pProtocolValue);

      protocol = pProtocolValue->uint8;
   }

   pSourceAddressValue = KrnlHlprFwpValueGetFromFwpsIncomingValues(pClassifyValues,
                                                                   &FWPM_CONDITION_IP_REMOTE_ADDRESS);
   HLPR_BAIL_ON_NULL_POINTER(pSourceAddressValue);

   pDestinationAddressValue = KrnlHlprFwpValueGetFromFwpsIncomingValues(pClassifyValues,
                                                                        &FWPM_CONDITION_IP_LOCAL_ADDRESS);
   HLPR_BAIL_ON_NULL_POINTER(pDestinationAddressValue);

   pSourcePortValue = pICMPCodeValue = KrnlHlprFwpValueGetFromFwpsIncomingValues(pClassifyValues,
                                                                                 &FWPM_CONDITION_IP_REMOTE_PORT);
   HLPR_BAIL_ON_NULL_POINTER(pSourcePortValue);

   pDestinationPortValue = pICMPTypeValue =  KrnlHlprFwpValueGetFromFwpsIncomingValues(pClassifyValues,
                                                                                       &FWPM_CONDITION_IP_LOCAL_PORT);
   HLPR_BAIL_ON_NULL_POINTER(pDestinationPortValue);

   if(pClassifyValues->layerId == FWPS_LAYER_ALE_FLOW_ESTABLISHED_V4)
   {
      ipHeaderSize = IPV4_HEADER_MIN_SIZE;
   
      if(protocol == ICMPV4)
         transportHeaderSize = ICMP_HEADER_MIN_SIZE;
      else if(protocol == TCP)
         transportHeaderSize = TCP_HEADER_MIN_SIZE;
      else if(protocol == UDP)
         transportHeaderSize = UDP_HEADER_MIN_SIZE;
   }
   else if(pClassifyValues->layerId == FWPS_LAYER_ALE_FLOW_ESTABLISHED_V6)
   {
      ipHeaderSize = IPV6_HEADER_MIN_SIZE;

      if(protocol == ICMPV6)
         transportHeaderSize = ICMP_HEADER_MIN_SIZE;
      else if(protocol == TCP)
         transportHeaderSize = TCP_HEADER_MIN_SIZE;
      else if(protocol == UDP)
         transportHeaderSize = UDP_HEADER_MIN_SIZE;
   }

   if(FWPS_IS_METADATA_FIELD_PRESENT(pMetadata,
                                     FWPS_METADATA_FIELD_IP_HEADER_SIZE) &&
      pMetadata->ipHeaderSize)
      ipHeaderSize = pMetadata->ipHeaderSize;

   if(FWPS_IS_METADATA_FIELD_PRESENT(pMetadata,
                                     FWPS_METADATA_FIELD_TRANSPORT_HEADER_SIZE) &&
      pMetadata->transportHeaderSize)
      transportHeaderSize = pMetadata->transportHeaderSize;

   if((protocol == ICMPV4 ||
      protocol == ICMPV6) &&
      (pClassifyValues->layerId != FWPS_LAYER_INBOUND_ICMP_ERROR_V4 &&
      pClassifyValues->layerId != FWPS_LAYER_INBOUND_ICMP_ERROR_V6 &&
      pClassifyValues->layerId != FWPS_LAYER_DATAGRAM_DATA_V4 &&
      pClassifyValues->layerId != FWPS_LAYER_DATAGRAM_DATA_V6))
   {
      /// Initial offset is at the ICMP Header, so retreat the size of the IP Header ...
      status = NdisRetreatNetBufferDataStart(NET_BUFFER_LIST_FIRST_NB((NET_BUFFER_LIST*)pClassifyData->pPacket),
                                             ipHeaderSize,
                                             0,
                                             0);
   }
   else
   {
      /// Initial offset is at the Data, so retreat the size of the IP and Transport Headers ...
      status = NdisRetreatNetBufferDataStart(NET_BUFFER_LIST_FIRST_NB((NET_BUFFER_LIST*)pClassifyData->pPacket),
                                             ipHeaderSize + transportHeaderSize,
                                             0,
                                             0);
   }

   if(status != STATUS_SUCCESS)
   {
      DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                 DPFLTR_ERROR_LEVEL,
                 " !!!! PerformBasicPacketExaminationAtInboundTransport: NdisRetreatNetBufferDataStart() [status: %#x]\n",
                 status);

      HLPR_BAIL;
   }
   else
   {
      if((protocol == ICMPV4 ||
         protocol == ICMPV6) &&
         (pClassifyValues->layerId != FWPS_LAYER_INBOUND_ICMP_ERROR_V4 &&
         pClassifyValues->layerId != FWPS_LAYER_INBOUND_ICMP_ERROR_V6 &&
         pClassifyValues->layerId != FWPS_LAYER_DATAGRAM_DATA_V4 &&
         pClassifyValues->layerId != FWPS_LAYER_DATAGRAM_DATA_V6))
         bytesRetreated = ipHeaderSize;
      else
         bytesRetreated = ipHeaderSize + transportHeaderSize;
   }

   status = KrnlHlprIPHeaderGet((NET_BUFFER_LIST*)pClassifyData->pPacket,
                                &pHeader,
                                &needToFreeMemory,
                                ipHeaderSize);
   HLPR_BAIL_ON_FAILURE(status);

   /// Validate that what is indicated in the classify is what is present in packet's IP Header
   if(KrnlHlprFwpsLayerIsIPv4(pClassifyValues->layerId))
   {
      IP_HEADER_V4* pIPv4Header = (IP_HEADER_V4*)pHeader;

      NT_ASSERT(pIPv4Header->version == IPV4);
      NT_ASSERT(((UINT32)(pIPv4Header->headerLength * 4)) == ipHeaderSize);
      NT_ASSERT(pIPv4Header->protocol == protocol);
      NT_ASSERT(ntohl(*((UINT32*)pIPv4Header->pSourceAddress)) == pSourceAddressValue->uint32);
      NT_ASSERT(ntohl(*((UINT32*)pIPv4Header->pDestinationAddress)) == pDestinationAddressValue->uint32);

      if(pIPv4Header->version == IPV4 &&
         ipHeaderSize >= IPV4_HEADER_MIN_SIZE)
         LogIPv4Header(pIPv4Header);
   }
   else
   {
      IP_HEADER_V6* pIPv6Header = (IP_HEADER_V6*)pHeader;

      NT_ASSERT(sizeof(IP_HEADER_V6) == ipHeaderSize);
      NT_ASSERT(pIPv6Header->version.value == IPV6);
      NT_ASSERT(pIPv6Header->nextHeader == protocol);
      NT_ASSERT(RtlCompareMemory(pIPv6Header->pSourceAddress,
                                 pSourceAddressValue->byteArray16->byteArray16,
                                 IPV6_ADDRESS_SIZE) == IPV6_ADDRESS_SIZE);
      NT_ASSERT(RtlCompareMemory(pIPv6Header->pDestinationAddress,
                                 pDestinationAddressValue->byteArray16->byteArray16,
                                 IPV6_ADDRESS_SIZE) == IPV6_ADDRESS_SIZE);

      if(pIPv6Header->version.value == IPV6 &&
         ipHeaderSize >= IPV6_HEADER_MIN_SIZE)
         LogIPv6Header(pIPv6Header);
   }

   if(needToFreeMemory)
   {
      HLPR_DELETE_ARRAY(pHeader,
                        WFPSAMPLER_SYSLIB_TAG);

      needToFreeMemory = FALSE;
   }

   if(bytesRetreated)
   {
      /// and advance the offset to the Transport Header.
      NdisAdvanceNetBufferDataStart(NET_BUFFER_LIST_FIRST_NB((NET_BUFFER_LIST*)pClassifyData->pPacket),
                                    ipHeaderSize,
                                    FALSE,
                                    0);

      bytesRetreated -= ipHeaderSize;
   }

   status = KrnlHlprTransportHeaderGet((NET_BUFFER_LIST*)pClassifyData->pPacket,
                                       &pHeader,
                                       &needToFreeMemory,
                                       transportHeaderSize);
   HLPR_BAIL_ON_FAILURE(status);

   switch(protocol)
   {
      case ICMPV4:
      {
         ICMP_HEADER_V4* pICMPv4Header = (ICMP_HEADER_V4*)pHeader;

         NT_ASSERT(pICMPv4Header->type == pICMPTypeValue->uint16);
         NT_ASSERT(pICMPv4Header->code == pICMPCodeValue->uint16);

         if(transportHeaderSize >= ICMP_HEADER_MIN_SIZE)
            LogICMPv4Header(pICMPv4Header);

         break;
      }
      case ICMPV6:
      {
         ICMP_HEADER_V6* pICMPv6Header = (ICMP_HEADER_V6*)pHeader;

         NT_ASSERT(pICMPv6Header->type == pICMPTypeValue->uint16);
         NT_ASSERT(pICMPv6Header->code == pICMPCodeValue->uint16);

         if(transportHeaderSize >= ICMP_HEADER_MIN_SIZE)
            LogICMPv6Header(pICMPv6Header);

         break;
      }
      case TCP:
      {
         TCP_HEADER* pTCPHeader = (TCP_HEADER*)pHeader;

         NT_ASSERT(ntohs(pTCPHeader->sourcePort) == pSourcePortValue->uint16 );
         NT_ASSERT(ntohs(pTCPHeader->destinationPort) == pDestinationPortValue->uint16);

         if(transportHeaderSize >= TCP_HEADER_MIN_SIZE)
            LogTCPHeader(pTCPHeader);

         break;
      }
      case UDP:
      {
         UDP_HEADER* pUDPHeader = (UDP_HEADER*)pHeader;

         NT_ASSERT(ntohs(pUDPHeader->sourcePort) == pSourcePortValue->uint16 );
         NT_ASSERT(ntohs(pUDPHeader->destinationPort) == pDestinationPortValue->uint16);

         if(transportHeaderSize >= UDP_HEADER_MIN_SIZE)
            LogUDPHeader(pUDPHeader);

         break;
      }
   }

   HLPR_BAIL_LABEL:

   if(needToFreeMemory)
   {
      HLPR_DELETE_ARRAY(pHeader,
                        WFPSAMPLER_SYSLIB_TAG);

      needToFreeMemory = FALSE;
   }

   if(bytesRetreated)
   {
      /// ... and advance the offset back to the original position.
      NdisAdvanceNetBufferDataStart(NET_BUFFER_LIST_FIRST_NB((NET_BUFFER_LIST*)pClassifyData->pPacket),
                                    bytesRetreated,
                                    FALSE,
                                    0);
   }

#if DBG

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- PerformBasicPacketExaminationAtInboundTransport()\n");

#endif /// DBG

   return;
}

/**
 @private_function="PerformBasicPacketExaminationAtOutboundTransport"
 
   Purpose:  Examines and logs the contents of the IP Header and Transport Header if available. <br>
                                                                                                <br>
   Notes:    Applies to the following transport layers:                                         <br>
                FWPM_LAYER_OUTBOUND_TRANSPORT_V{4/6}                                            <br>
                FWPM_LAYER_OUTBOUND_ICMP_ERROR_V{4/6}                                           <br>
                FWPM_LAYER_DATAGRAM_DATA_V{4/6}        (Outbound only)                          <br>
                FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V{4/6} (Outbound only)                          <br>
                FWPM_LAYER_ALE_AUTH_CONNECT_V{4/6}     (Outbound reauthorization only)          <br>
                FWPM_LAYER_ALE_FLOW_ESTABLISHED_V{4/6} (Outbound, non-TCP only)                 <br>
                FWPM_LAYER_STREAM_PACKET_V{4/6}        (Outbound only)                          <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/FF560703.aspx                              <br>
             HTTP://MSDN.Microsoft.com/En-US/Library/FF564527.aspx                              <br>
             HTTP://MSDN.Microsoft.com/En-US/Library/FF562631.aspx                              <br>
*/
VOID PerformBasicPacketExaminationAtOutboundTransport(_In_ CLASSIFY_DATA* pClassifyData)
{
#if DBG

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> PerformBasicPacketExaminationAtOutboundTransport()\n");

#endif /// DBG

   NT_ASSERT(pClassifyData);

   NTSTATUS              status                = STATUS_SUCCESS;
   FWPS_INCOMING_VALUES* pClassifyValues       = (FWPS_INCOMING_VALUES*)pClassifyData->pClassifyValues;
   UINT8                 protocol              = IPPROTO_RAW;
   FWP_VALUE*            pProtocolValue        = 0;
   FWP_VALUE*            pSourcePortValue      = 0;
   FWP_VALUE*            pDestinationPortValue = 0;
   FWP_VALUE*            pICMPTypeValue        = 0;
   FWP_VALUE*            pICMPCodeValue        = 0;
   VOID*                 pHeader               = 0;
   BOOLEAN               needToFreeMemory      = FALSE;

   if(pClassifyValues->layerId == FWPS_LAYER_OUTBOUND_ICMP_ERROR_V4)
      protocol = ICMPV4;
   else if(pClassifyValues->layerId == FWPS_LAYER_OUTBOUND_ICMP_ERROR_V6)
      protocol = ICMPV6;

#if(NTDDI_VERSION >= NTDDI_WIN7)

   else if(pClassifyValues->layerId == FWPS_LAYER_STREAM_PACKET_V4 ||
           pClassifyValues->layerId == FWPS_LAYER_STREAM_PACKET_V6)
      protocol = TCP;

#endif /// (NTDDI_VERSION >= NTDDI_WIN7)

   else
   {
      pProtocolValue = KrnlHlprFwpValueGetFromFwpsIncomingValues(pClassifyValues,
                                                                 &FWPM_CONDITION_IP_PROTOCOL);
      HLPR_BAIL_ON_NULL_POINTER(pProtocolValue);

      protocol = pProtocolValue->uint8;
   }

   pSourcePortValue = pICMPTypeValue = KrnlHlprFwpValueGetFromFwpsIncomingValues(pClassifyValues,
                                                                                 &FWPM_CONDITION_IP_LOCAL_PORT);
   HLPR_BAIL_ON_NULL_POINTER(pSourcePortValue);

   pDestinationPortValue = pICMPCodeValue =  KrnlHlprFwpValueGetFromFwpsIncomingValues(pClassifyValues,
                                                                                       &FWPM_CONDITION_IP_REMOTE_PORT);
   HLPR_BAIL_ON_NULL_POINTER(pDestinationPortValue);

   /// Initial offset is at the Transport Header ...
   status = KrnlHlprTransportHeaderGet((NET_BUFFER_LIST*)pClassifyData->pPacket,
                                       &pHeader,
                                       &needToFreeMemory);
   HLPR_BAIL_ON_FAILURE(status);

   switch(protocol)
   {
      case ICMPV4:
      {
         ICMP_HEADER_V4* pICMPv4Header = (ICMP_HEADER_V4*)pHeader;

         NT_ASSERT(pICMPv4Header->type == pICMPTypeValue->uint16);
         NT_ASSERT(pICMPv4Header->code == pICMPCodeValue->uint16);

         LogICMPv4Header(pICMPv4Header);

         break;
      }
      case ICMPV6:
      {
         ICMP_HEADER_V6* pICMPv6Header = (ICMP_HEADER_V6*)pHeader;

         NT_ASSERT(pICMPv6Header->type == pICMPTypeValue->uint16);
         NT_ASSERT(pICMPv6Header->code == pICMPCodeValue->uint16);

         LogICMPv6Header(pICMPv6Header);

         break;
      }
      case TCP:
      {
         TCP_HEADER* pTCPHeader = (TCP_HEADER*)pHeader;

         NT_ASSERT(ntohs(pTCPHeader->sourcePort) == pSourcePortValue->uint16);
         NT_ASSERT(ntohs(pTCPHeader->destinationPort) == pDestinationPortValue->uint16);

         LogTCPHeader(pTCPHeader);

         break;
      }
      case UDP:
      {
         UDP_HEADER* pUDPHeader = (UDP_HEADER*)pHeader;

         NT_ASSERT(ntohs(pUDPHeader->sourcePort) == pSourcePortValue->uint16);
         NT_ASSERT(ntohs(pUDPHeader->destinationPort) == pDestinationPortValue->uint16);

         LogUDPHeader(pUDPHeader);

         break;
      }
   }

   HLPR_BAIL_LABEL:

   if(needToFreeMemory)
   {
      HLPR_DELETE_ARRAY(pHeader,
                        WFPSAMPLER_SYSLIB_TAG);

      needToFreeMemory = FALSE;
   }

#if DBG

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- PerformBasicPacketExaminationAtOutboundTransport()\n");

#endif /// DBG

   return;
}

/**
 @private_function="PerformBasicPacketExaminationAtOutboundTransport"
 
   Purpose:  Logs the Discard reason.                                                           <br>
                                                                                                <br>
   Notes:    Applies to the following discard layers:                                           <br>
                FWPM_LAYER_INBOUND_IPPACKET_V{4/6}_DISCARD                                      <br>
                FWPM_LAYER_OUTBOUND_IPPACKET_V{4/6}_DISCARD                                     <br>
                FWPM_LAYER_IPFORWARD_V{4/6}_DISCARD                                             <br>
                FWPM_LAYER_INBOUND_TRANSPORT_V{4/6}_DISCARD                                     <br>
                FWPM_LAYER_OUTBOUND_TRANSPORT_V{4/6}_DISCARD                                    <br>
                FWPM_LAYER_STREAM_V{4/6}_DISCARD                                                <br>
                FWPM_LAYER_DATAGRAM_DATA_V{4/6}                                                 <br>
                FWPM_LAYER_INBOUND_ICMP_ERROR_V{4/6}_DISCARD                                    <br>
                FWPM_LAYER_OUTBOUND_ICMP_ERROR_V{4/6}_DISCARD                                   <br>
                FWPM_LAYER_ALE_RESOURCE_ASSIGNMENT_V{4/6}_DISCARD                               <br>
                FWPM_LAYER_ALE_AUTH_LISTEN_V{4/6}_DISCARD                                       <br>
                FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V{4/6}_DISCARD                                  <br>
                FWPM_LAYER_ALE_AUTH_CONNECT_V{4/6}_DISCARD                                      <br>
                FWPM_LAYER_ALE_FLOW_ESTABLISHED_V{4/6}_DISCARD                                  <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
VOID PerformBasicPacketExaminationAtDiscard(_In_ CLASSIFY_DATA* pClassifyData)
{
#if DBG

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> PerformBasicPacketExaminationAtDiscard()\n");

#endif /// DBG

   NT_ASSERT(pClassifyData);
   NT_ASSERT(pClassifyData->pClassifyValues);
   NT_ASSERT(pClassifyData->pMetadataValues);
   NT_ASSERT(pClassifyData->pMetadataValues->currentMetadataValues & FWPS_METADATA_FIELD_DISCARD_REASON);

   NTSTATUS status         = STATUS_SUCCESS;
   UINT32   discardModule  = FWPS_DISCARD_MODULE_MAX;
   UINT32   discardReason  = IpDiscardMax;
   UINT64   filterID       = 0;
   PSTR     pDiscardModule = "UNKNOWN";
   PSTR     pDiscardReason = "UNKNOWN";
   PSTR     pString        = 0;

   if(FWPS_IS_METADATA_FIELD_PRESENT(pClassifyData->pMetadataValues,
                                     FWPS_METADATA_FIELD_DISCARD_REASON))
   {
      discardModule = pClassifyData->pMetadataValues->discardMetadata.discardModule;

      discardReason = pClassifyData->pMetadataValues->discardMetadata.discardReason;

      filterID = pClassifyData->pMetadataValues->discardMetadata.filterId;
   }

   switch(discardModule)
   {
      case FWPS_DISCARD_MODULE_NETWORK:
      {
         pDiscardModule = "NETWORK";

         switch(discardReason)
         {
            case IpDiscardBadSourceAddress:
            {
               pDiscardReason = "Bad Source Address";
            
               break;
            }
            case IpDiscardNotLocallyDestined:
            {
               pDiscardReason = "Not Locally Destined";
            
               break;
            }
            case IpDiscardProtocolUnreachable:
            {
               pDiscardReason = "Unreachable Protocol";
            
               break;
            }
            case IpDiscardPortUnreachable:
            {
               pDiscardReason = "UnreachablePort";
            
               break;
            }
            case IpDiscardBadLength:
            {
               pDiscardReason = "Bad Length";
            
               break;
            }
            case IpDiscardMalformedHeader:
            {
               pDiscardReason = "Malformed Header";
            
               break;
            }
            case IpDiscardNoRoute:
            {
               pDiscardReason = "No Route";
            
               break;
            }
            case IpDiscardBeyondScope:
            {
               pDiscardReason = "Beyond Scope";
            
               break;
            }
            case IpDiscardInspectionDrop:
            {
               pDiscardReason = "Inspection Drop";
            
               break;
            }
            case IpDiscardTooManyDecapsulations:
            {
               pDiscardReason = "Too Many Decapsulations";
            
               break;
            }
            case IpDiscardAdministrativelyProhibited:
            {
               pDiscardReason = "Administratively Prohibited";
            
               break;
            }
            case IpDiscardHopLimitExceeded:
            {
               pDiscardReason = "Hop Limit Exceeded";
            
               break;
            }
            case IpDiscardAddressUnreachable:
            {
               pDiscardReason = "Unreachable Address";
            
               break;
            }
            case IpDiscardRscPacket:
            {
               pDiscardReason = "RSC Packet";
            
               break;
            }
            case IpDiscardArbitrationUnhandled:
            {
               pDiscardReason = "Unhandled Arbitration";
            
               break;
            }
            case IpDiscardInspectionAbsorb:
            {
               pDiscardReason = "Inspection Absorb";
            
               break;
            }
            case IpDiscardDontFragmentMtuExceeded:
            {
               pDiscardReason = "Don't Fragment / MTU Exceeded";
            
               break;
            }
            case IpDiscardBufferLengthExceeded:
            {
               pDiscardReason = "Buffer Length Exceeded";
            
               break;
            }
            case IpDiscardAddressResolutionTimeout:
            {
               pDiscardReason = "Address Resolution Timeout";
            
               break;
            }
            case IpDiscardAddressResolutionFailure:
            {
               pDiscardReason = "Address Resolution Failure";
            
               break;
            }
            case IpDiscardIpsecFailure:
            {
               pDiscardReason = "IPsec Failure";
            
               break;
            }
            case IpDiscardExtensionHeadersFailure:
            {
               pDiscardReason = "Extension Headers Failure";
            
               break;
            }
            case IpDiscardIPSNPIDrop:
            {
               pDiscardReason = "IPSNPI Drop";
            
               break;
            }
         }

         break;
      }
      case FWPS_DISCARD_MODULE_TRANSPORT:
      {
         pDiscardModule = "TRANSPORT";

         switch(discardReason)
         {
            case InetDiscardSourceUnspecified:
            {
               pDiscardReason = "Unspecified Source";
            
               break;
            }
            case InetDiscardDestinationMulticast:
            {
               pDiscardReason = "Multicast Destination";
            
               break;
            }
            case InetDiscardHeaderInvalid:
            {
               pDiscardReason = "Invalid Header";
            
               break;
            }
            case InetDiscardChecksumInvalid:
            {
               pDiscardReason = "Invalid Checksum";
            
               break;
            }
            case InetDiscardEndpointNotFound:
            {
               pDiscardReason = "Endpoint Not Found";
            
               break;
            }
            case InetDiscardConnectedPath:
            {
               pDiscardReason = "Connected Path";
            
               break;
            }
            case InetDiscardSessionState:
            {
               pDiscardReason = "Session State";
            
               break;
            }
            case InetDiscardReceiveInspection:
            {
               pDiscardReason = "Receive Inspection";
            
               break;
            }
         }

         break;
      }
      case FWPS_DISCARD_MODULE_GENERAL:
      {
         pDiscardModule = "GENERAL";

         switch(discardReason)
         {
            case FWPS_DISCARD_FIREWALL_POLICY:
            {
               pDiscardReason = "Firewall Policy";
            
               break;
            }
            case FWPS_DISCARD_IPSEC:
            {
               pDiscardReason = "IPsec Policy";
            
               break;
            }
         }

         break;
      }
   }

   HLPR_NEW_ARRAY(pString,
                  CHAR,
                  MAX_STRING_SIZE,
                  WFPSAMPLER_CALLOUT_DRIVER_TAG);
   HLPR_BAIL_ON_ALLOC_FAILURE(pString,
                              status);

   status = RtlStringCchPrintfA(pString,
                                MAX_STRING_SIZE,
                                "\n"
                                "\t\tLayer:\t%s"
                                "\t\tModule:\t%s\n"
                                "\t\tReason:\t%s\n"
                                "\t\tFilter:\t%I64d\n",
                                KrnlHlprFwpsLayerIDToString(pClassifyData->pClassifyValues->layerId - 1),
                                pDiscardModule,
                                pDiscardReason,
                                filterID);
   if(status == STATUS_SUCCESS)
      DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                 DPFLTR_INFO_LEVEL,
                 "\tDiscard:%s",
                 pString);

   HLPR_BAIL_LABEL:

   HLPR_DELETE_ARRAY(pString,
                     WFPSAMPLER_CALLOUT_DRIVER_TAG);

#if DBG

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- PerformBasicPacketExaminationAtDiscard()\n");

#endif /// DBG

   return;
}

/**
 @private_function="PerformBasicPacketExaminationAtOther"
 
   Purpose:  Logs the classify.                                                                 <br>
                                                                                                <br>
   Notes:    Applies to the following discard layers:                                           <br>
                FWPM_LAYER_STREAM_V{4/6}                                                        <br>
                FWPM_LAYER_ALE_RESOURCE_ASSIGNMENT_V{4/6}                                       <br>
                FWPM_LAYER_ALE_AUTH_LISTEN_V{4/6}                                               <br>
                FWPM_LAYER_ALE_RESOURCE_RELEASE_V{4/6}                                          <br>
                FWPM_LAYER_ALE_ENDPOINT_CLOSURE_V{4/6}                                          <br>
                FWPM_LAYER_ALE_CONNECT_REDIRECT_V{4/6}                                          <br>
                FWPM_LAYER_ALE_BIND_REDIRECT_V{4/6}                                             <br>
                FWPM_LAYER_ALE_CONNECT_REDIRECT_V{4/6}                                          <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
VOID PerformBasicPacketExaminationAtOther(_In_ CLASSIFY_DATA* pClassifyData)
{
#if DBG

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> PerformBasicPacketExaminationAtOther()\n");

#endif /// DBG

   NT_ASSERT(pClassifyData);
   NT_ASSERT(pClassifyData->pClassifyValues);

   NTSTATUS   status              = STATUS_SUCCESS;
   UINT16     localPort           = 0;
   BYTE*      pLocalAddress       = 0;
   PSTR       pString             = 0;

#pragma warning(push)
#pragma warning(disable: 28193) /// value is checked before use

   FWP_VALUE* pLocalAddressValue  = KrnlHlprFwpValueGetFromFwpsIncomingValues(pClassifyData->pClassifyValues,
                                                                              &FWPM_CONDITION_IP_LOCAL_ADDRESS);
   FWP_VALUE* pProtocolValue      = KrnlHlprFwpValueGetFromFwpsIncomingValues(pClassifyData->pClassifyValues,
                                                                              &FWPM_CONDITION_IP_PROTOCOL);
   FWP_VALUE* pLocalPortValue     = KrnlHlprFwpValueGetFromFwpsIncomingValues(pClassifyData->pClassifyValues,
                                                                              &FWPM_CONDITION_IP_LOCAL_PORT);

#pragma warning(pop)

#if(NTDDI_VERSION >= NTDDI_WIN7)

   UINT16     remotePort          = 0;
   BYTE*      pRemoteAddress      = 0;

#pragma warning(push)
#pragma warning(disable: 28193) /// value is checked before use

   FWP_VALUE* pRemoteAddressValue = KrnlHlprFwpValueGetFromFwpsIncomingValues(pClassifyData->pClassifyValues,
                                                                              &FWPM_CONDITION_IP_REMOTE_ADDRESS);
   FWP_VALUE* pRemotePortValue    = KrnlHlprFwpValueGetFromFwpsIncomingValues(pClassifyData->pClassifyValues,
                                                                              &FWPM_CONDITION_IP_REMOTE_PORT);

#pragma warning(pop)

#endif /// (NTDDI_VERSION >= NTDDI_WIN7)

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              "\tLayer:%s\n",
              KrnlHlprFwpsLayerIDToString(pClassifyData->pClassifyValues->layerId));

   HLPR_NEW_ARRAY(pString,
                  CHAR,
                  MAX_STRING_SIZE,
                  WFPSAMPLER_CALLOUT_DRIVER_TAG);
   HLPR_BAIL_ON_ALLOC_FAILURE(pString,
                              status);

   switch(pClassifyData->pClassifyValues->layerId)
   {
      case FWPS_LAYER_STREAM_V4:
      case FWPS_LAYER_STREAM_V6:
      {
         FWPS_STREAM_CALLOUT_IO_PACKET0* pIOPacket = (FWPS_STREAM_CALLOUT_IO_PACKET*)(pClassifyData->pPacket);

         if(pIOPacket)
         {
            status = RtlStringCchPrintfA(pString,
                                         MAX_STRING_SIZE,
                                         "\n"
                                         "\t\t\tStreamData:\n"
                                         "\t\t\t\tFlags:      %#x\n"
                                         "\t\t\t\tDataLength: %I64d\n"
                                         "\t\t\tBytesMissed:   %I64d\n"
                                         "\t\t\tBytesRequired: %d\n"
                                         "\t\t\tBytesEnforced: %I64d\n"
                                         "\t\t\tStreamAction:  %#x\n",
                                         pIOPacket->streamData ? pIOPacket->streamData->flags : 0,
                                         (UINT64)(pIOPacket->streamData ? pIOPacket->streamData->dataLength : 0),
                                         (UINT64)(pIOPacket->missedBytes),
                                         pIOPacket->countBytesRequired,
                                         (UINT64)(pIOPacket->countBytesEnforced),
                                         pIOPacket->streamAction);
            if(status == STATUS_SUCCESS)
               DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                          DPFLTR_INFO_LEVEL,
                          "\t\tStreamCalloutIOPacket: %s",
                          pString);
         }

         break;
      }
      case FWPS_LAYER_ALE_RESOURCE_ASSIGNMENT_V4:
      case FWPS_LAYER_ALE_RESOURCE_ASSIGNMENT_V6:
      {
         PSTR pProtocolString = 0;

         if(pProtocolValue &&
            pProtocolValue->type == FWP_UINT8)
         {
            if(pProtocolValue->uint8 == ICMPV4 ||
               pProtocolValue->uint8 == ICMPV6)
               pProtocolString = "ICMP";
            else if(pProtocolValue->uint8 == TCP)
               pProtocolString = "TCP";
            else if(pProtocolValue->uint8 == UDP)
               pProtocolString = "UDP";
            else if(pProtocolValue->uint8 == IPPROTO_RAW)
               pProtocolString = "Raw IP";
         }

         if(pLocalPortValue)
         {
            if(pLocalPortValue->type == FWP_UINT16)
               localPort = pLocalPortValue->uint16;
            else if(pLocalPortValue->type == FWP_UINT8)
               localPort = pLocalPortValue->uint8;
         }

         if(pLocalAddressValue)
         {
            if(pLocalAddressValue->type == FWP_UINT32)
            {
               pLocalAddress = (BYTE*)&(pLocalAddressValue->uint32);

               status = RtlStringCchPrintfA(pString,
                                            MAX_STRING_SIZE,
                                            "\n\t\t\t"
                                            "%s %d.%d.%d.%d : %d\n",
                                            pProtocolString,
                                            pLocalAddress[3],
                                            pLocalAddress[2],
                                            pLocalAddress[1],
                                            pLocalAddress[0],
                                            localPort);
            }
            else if(pLocalAddressValue->type == FWP_BYTE_ARRAY16_TYPE)
            {
               pLocalAddress = (BYTE*)pLocalAddressValue->byteArray16;

               status = RtlStringCchPrintfA(pString,
                                            MAX_STRING_SIZE,
                                            "\n\t\t\t"
                                            "%s %02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x : %d",
                                            pProtocolString,
                                            pLocalAddress[0],
                                            pLocalAddress[1],
                                            pLocalAddress[2],
                                            pLocalAddress[3],
                                            pLocalAddress[4],
                                            pLocalAddress[5],
                                            pLocalAddress[6],
                                            pLocalAddress[7],
                                            pLocalAddress[8],
                                            pLocalAddress[9],
                                            pLocalAddress[10],
                                            pLocalAddress[11],
                                            pLocalAddress[12],
                                            pLocalAddress[13],
                                            pLocalAddress[14],
                                            pLocalAddress[15],
                                            localPort);
            }

            if(status == STATUS_SUCCESS)
               DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                          DPFLTR_INFO_LEVEL,
                          "\t\tBinding: %s",
                          pString);
         }

         break;
      }
      case FWPS_LAYER_ALE_AUTH_LISTEN_V4:
      case FWPS_LAYER_ALE_AUTH_LISTEN_V6:
      {
         if(pLocalPortValue)
         {
            if(pLocalPortValue->type == FWP_UINT16)
               localPort = pLocalPortValue->uint16;
            else if(pLocalPortValue->type == FWP_UINT8)
               localPort = pLocalPortValue->uint8;
         }

         if(pLocalAddressValue)
         {
            if(pLocalAddressValue->type == FWP_UINT32)
            {
               pLocalAddress = (BYTE*)&(pLocalAddressValue->uint32);

               status = RtlStringCchPrintfA(pString,
                                            MAX_STRING_SIZE,
                                            "\n\t\t\t"
                                            "%d.%d.%d.%d : %d\n",
                                            pLocalAddress[3],
                                            pLocalAddress[2],
                                            pLocalAddress[1],
                                            pLocalAddress[0],
                                            localPort);
            }
            else if(pLocalAddressValue->type == FWP_BYTE_ARRAY16_TYPE)
            {
               pLocalAddress = (BYTE*)pLocalAddressValue->byteArray16;

               status = RtlStringCchPrintfA(pString,
                                            MAX_STRING_SIZE,
                                            "\n\t\t\t"
                                            "%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x : %d",
                                            pLocalAddress[0],
                                            pLocalAddress[1],
                                            pLocalAddress[2],
                                            pLocalAddress[3],
                                            pLocalAddress[4],
                                            pLocalAddress[5],
                                            pLocalAddress[6],
                                            pLocalAddress[7],
                                            pLocalAddress[8],
                                            pLocalAddress[9],
                                            pLocalAddress[10],
                                            pLocalAddress[11],
                                            pLocalAddress[12],
                                            pLocalAddress[13],
                                            pLocalAddress[14],
                                            pLocalAddress[15],
                                            localPort);
            }

            if(status == STATUS_SUCCESS)
               DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                          DPFLTR_INFO_LEVEL,
                          "\t\tListening: TCP %s",
                          pString);
         }

         break;
      }

#if(NTDDI_VERSION >= NTDDI_WIN7)

      case FWPS_LAYER_ALE_RESOURCE_RELEASE_V4:
      case FWPS_LAYER_ALE_RESOURCE_RELEASE_V6:
      {
         PSTR pProtocolString = 0;

         if(pProtocolValue &&
            pProtocolValue->type == FWP_UINT8)
         {
            if(pProtocolValue->uint8 == ICMPV4 ||
               pProtocolValue->uint8 == ICMPV6)
               pProtocolString = "ICMP";
            else if(pProtocolValue->uint8 == TCP)
               pProtocolString = "TCP";
            else if(pProtocolValue->uint8 == UDP)
               pProtocolString = "UDP";
            else if(pProtocolValue->uint8 == IPPROTO_RAW)
               pProtocolString = "Raw IP";
         }

         if(pLocalPortValue)
         {
            if(pLocalPortValue->type == FWP_UINT16)
               localPort = pLocalPortValue->uint16;
            else if(pLocalPortValue->type == FWP_UINT8)
               localPort = pLocalPortValue->uint8;
         }

         if(pLocalAddressValue)
         {
            if(pLocalAddressValue->type == FWP_UINT32)
            {
               pLocalAddress = (BYTE*)&(pLocalAddressValue->uint32);

               status = RtlStringCchPrintfA(pString,
                                            MAX_STRING_SIZE,
                                            "\n\t\t\t"
                                            "%s %d.%d.%d.%d : %d\n",
                                            pProtocolString,
                                            pLocalAddress[3],
                                            pLocalAddress[2],
                                            pLocalAddress[1],
                                            pLocalAddress[0],
                                            localPort);
            }
            else if(pLocalAddressValue->type == FWP_BYTE_ARRAY16_TYPE)
            {
               pLocalAddress = (BYTE*)pLocalAddressValue->byteArray16;

               status = RtlStringCchPrintfA(pString,
                                            MAX_STRING_SIZE,
                                            "\n\t\t\t"
                                            "%s %02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x : %d",
                                            pProtocolString,
                                            pLocalAddress[0],
                                            pLocalAddress[1],
                                            pLocalAddress[2],
                                            pLocalAddress[3],
                                            pLocalAddress[4],
                                            pLocalAddress[5],
                                            pLocalAddress[6],
                                            pLocalAddress[7],
                                            pLocalAddress[8],
                                            pLocalAddress[9],
                                            pLocalAddress[10],
                                            pLocalAddress[11],
                                            pLocalAddress[12],
                                            pLocalAddress[13],
                                            pLocalAddress[14],
                                            pLocalAddress[15],
                                            localPort);
            }

            if(status == STATUS_SUCCESS)
               DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                          DPFLTR_INFO_LEVEL,
                          "\t\tReleasing: %s",
                          pString);
         }

         break;
      }
      case FWPS_LAYER_ALE_ENDPOINT_CLOSURE_V4:
      case FWPS_LAYER_ALE_ENDPOINT_CLOSURE_V6:
      {
         PSTR pProtocolString = 0;

         if(pProtocolValue &&
            pProtocolValue->type == FWP_UINT8)
         {
            if(pProtocolValue->uint8 == ICMPV4 ||
               pProtocolValue->uint8 == ICMPV6)
               pProtocolString = "ICMP";
            else if(pProtocolValue->uint8 == TCP)
               pProtocolString = "TCP";
            else if(pProtocolValue->uint8 == UDP)
               pProtocolString = "UDP";
            else if(pProtocolValue->uint8 == IPPROTO_RAW)
               pProtocolString = "Raw IP";
         }

         if(pLocalPortValue)
         {
            if(pLocalPortValue->type == FWP_UINT16)
               localPort = pLocalPortValue->uint16;
            else if(pLocalPortValue->type == FWP_UINT8)
               localPort = pLocalPortValue->uint8;
         }

         if(pLocalAddressValue)
         {
            if(pLocalAddressValue->type == FWP_UINT32)
            {
               pLocalAddress = (BYTE*)&(pLocalAddressValue->uint32);

               status = RtlStringCchPrintfA(pString,
                                            MAX_STRING_SIZE,
                                            "\n\t\t\t"
                                            "%s %d.%d.%d.%d : %d\n",
                                            pProtocolString,
                                            pLocalAddress[3],
                                            pLocalAddress[2],
                                            pLocalAddress[1],
                                            pLocalAddress[0],
                                            localPort);
            }
            else if(pLocalAddressValue->type == FWP_BYTE_ARRAY16_TYPE)
            {
               pLocalAddress = (BYTE*)pLocalAddressValue->byteArray16;

               status = RtlStringCchPrintfA(pString,
                                            MAX_STRING_SIZE,
                                            "\n\t\t\t"
                                            "%s %02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x : %d",
                                            pProtocolString,
                                            pLocalAddress[0],
                                            pLocalAddress[1],
                                            pLocalAddress[2],
                                            pLocalAddress[3],
                                            pLocalAddress[4],
                                            pLocalAddress[5],
                                            pLocalAddress[6],
                                            pLocalAddress[7],
                                            pLocalAddress[8],
                                            pLocalAddress[9],
                                            pLocalAddress[10],
                                            pLocalAddress[11],
                                            pLocalAddress[12],
                                            pLocalAddress[13],
                                            pLocalAddress[14],
                                            pLocalAddress[15],
                                            localPort);
            }

            if(status == STATUS_SUCCESS)
               DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                          DPFLTR_INFO_LEVEL,
                          "\t\tClosing: %s",
                          pString);
         }

         break;
      }
      case FWPS_LAYER_ALE_CONNECT_REDIRECT_V4:
      case FWPS_LAYER_ALE_CONNECT_REDIRECT_V6:
      {
         PSTR pProtocolString = 0;

         if(pProtocolValue &&
            pProtocolValue->type == FWP_UINT8)
         {
            if(pProtocolValue->uint8 == ICMPV4 ||
               pProtocolValue->uint8 == ICMPV6)
               pProtocolString = "ICMP";
            else if(pProtocolValue->uint8 == TCP)
               pProtocolString = "TCP";
            else if(pProtocolValue->uint8 == UDP)
               pProtocolString = "UDP";
            else if(pProtocolValue->uint8 == IPPROTO_RAW)
               pProtocolString = "Raw IP";
         }

         if(pLocalPortValue)
         {
            if(pLocalPortValue->type == FWP_UINT16)
               localPort = pLocalPortValue->uint16;
            else if(pLocalPortValue->type == FWP_UINT8)
               localPort = pLocalPortValue->uint8;
         }

         if(pRemotePortValue)
         {
            if(pRemotePortValue->type == FWP_UINT16)
               remotePort = pRemotePortValue->uint16;
            else if(pRemotePortValue->type == FWP_UINT8)
               remotePort = pRemotePortValue->uint8;
         }

         if(pLocalAddressValue &&
            pRemoteAddressValue)
         {
            if(pLocalAddressValue->type == FWP_UINT32 &&
               pRemoteAddressValue->type == FWP_UINT32)
            {
               pLocalAddress = (BYTE*)&(pLocalAddressValue->uint32);

               pRemoteAddress = (BYTE*)&(pRemoteAddressValue->uint32);

               status = RtlStringCchPrintfA(pString,
                                            MAX_STRING_SIZE,
                                            "\n\t\t\t"
                                            "%s %d.%d.%d.%d : %d To %d.%d.%d.%d : %d\n",
                                            pProtocolString,
                                            pLocalAddress[3],
                                            pLocalAddress[2],
                                            pLocalAddress[1],
                                            pLocalAddress[0],
                                            localPort,
                                            pRemoteAddress[3],
                                            pRemoteAddress[2],
                                            pRemoteAddress[1],
                                            pRemoteAddress[0],
                                            remotePort);
            }
            else if(pLocalAddressValue->type == FWP_BYTE_ARRAY16_TYPE &&
                    pRemoteAddressValue->type == FWP_BYTE_ARRAY16_TYPE)
            {
               pLocalAddress = (BYTE*)pLocalAddressValue->byteArray16;

               pRemoteAddress = (BYTE*)pRemoteAddressValue->byteArray16;

               status = RtlStringCchPrintfA(pString,
                                            MAX_STRING_SIZE,
                                            "\n\t\t\t"
                                            "%s %02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x : %d To %02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x : %d\n",
                                            pProtocolString,
                                            pLocalAddress[0],
                                            pLocalAddress[1],
                                            pLocalAddress[2],
                                            pLocalAddress[3],
                                            pLocalAddress[4],
                                            pLocalAddress[5],
                                            pLocalAddress[6],
                                            pLocalAddress[7],
                                            pLocalAddress[8],
                                            pLocalAddress[9],
                                            pLocalAddress[10],
                                            pLocalAddress[11],
                                            pLocalAddress[12],
                                            pLocalAddress[13],
                                            pLocalAddress[14],
                                            pLocalAddress[15],
                                            localPort,
                                            pRemoteAddress[0],
                                            pRemoteAddress[1],
                                            pRemoteAddress[2],
                                            pRemoteAddress[3],
                                            pRemoteAddress[4],
                                            pRemoteAddress[5],
                                            pRemoteAddress[6],
                                            pRemoteAddress[7],
                                            pRemoteAddress[8],
                                            pRemoteAddress[9],
                                            pRemoteAddress[10],
                                            pRemoteAddress[11],
                                            pRemoteAddress[12],
                                            pRemoteAddress[13],
                                            pRemoteAddress[14],
                                            pRemoteAddress[15],
                                            remotePort);
            }

            if(status == STATUS_SUCCESS)
               DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                          DPFLTR_INFO_LEVEL,
                          "\t\tInspecting: %s",
                          pString);
         }

         break;
      }
      case FWPS_LAYER_ALE_BIND_REDIRECT_V4:
      case FWPS_LAYER_ALE_BIND_REDIRECT_V6:
      {
         PSTR pProtocolString = 0;

         if(pProtocolValue &&
            pProtocolValue->type == FWP_UINT8)
         {
            if(pProtocolValue->uint8 == ICMPV4 ||
               pProtocolValue->uint8 == ICMPV6)
               pProtocolString = "ICMP";
            else if(pProtocolValue->uint8 == TCP)
               pProtocolString = "TCP";
            else if(pProtocolValue->uint8 == UDP)
               pProtocolString = "UDP";
            else if(pProtocolValue->uint8 == IPPROTO_RAW)
               pProtocolString = "Raw IP";
         }

         if(pLocalPortValue)
         {
            if(pLocalPortValue->type == FWP_UINT16)
               localPort = pLocalPortValue->uint16;
            else if(pLocalPortValue->type == FWP_UINT8)
               localPort = pLocalPortValue->uint8;
         }

         if(pLocalAddressValue)
         {
            if(pLocalAddressValue->type == FWP_UINT32)
            {
               pLocalAddress = (BYTE*)&(pLocalAddressValue->uint32);

               status = RtlStringCchPrintfA(pString,
                                            MAX_STRING_SIZE,
                                            "\n\t\t\t"
                                            "%s %d.%d.%d.%d : %d\n",
                                            pProtocolString,
                                            pLocalAddress[3],
                                            pLocalAddress[2],
                                            pLocalAddress[1],
                                            pLocalAddress[0],
                                            localPort);
            }
            else if(pLocalAddressValue->type == FWP_BYTE_ARRAY16_TYPE)
            {
               pLocalAddress = (BYTE*)pLocalAddressValue->byteArray16;

               status = RtlStringCchPrintfA(pString,
                                            MAX_STRING_SIZE,
                                            "\n\t\t\t"
                                            "%s %02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x : %d",
                                            pProtocolString,
                                            pLocalAddress[0],
                                            pLocalAddress[1],
                                            pLocalAddress[2],
                                            pLocalAddress[3],
                                            pLocalAddress[4],
                                            pLocalAddress[5],
                                            pLocalAddress[6],
                                            pLocalAddress[7],
                                            pLocalAddress[8],
                                            pLocalAddress[9],
                                            pLocalAddress[10],
                                            pLocalAddress[11],
                                            pLocalAddress[12],
                                            pLocalAddress[13],
                                            pLocalAddress[14],
                                            pLocalAddress[15],
                                            localPort);
            }

            if(status == STATUS_SUCCESS)
               DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                          DPFLTR_INFO_LEVEL,
                          "\t\tInspecting: %s",
                          pString);
         }

         break;
      }

#endif /// (NTDDI_VERSION >= NTDDI_WIN7)

   }

   HLPR_BAIL_LABEL:

   HLPR_DELETE_ARRAY(pString,
                     WFPSAMPLER_CALLOUT_DRIVER_TAG);

#if DBG

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- PerformBasicPacketExaminationAtOther()\n");

#endif /// DBG

   return;
}

#if(NTDDI_VERSION >= NTDDI_WIN7)

/**
 @classify_function="ClassifyBasicPacketExamination"
 
   Purpose:  Examines the packet and returns FWP_ACTION_CONTINUE.                               <br>
                                                                                                <br>
   Notes:    Applies to the following layers:                                                   <br>
                FWPS_LAYER_INBOUND_IPPACKET_V{4/6}                                              <br>
                FWPS_LAYER_INBOUND_IPPACKET_V{4/6}_DISCARD                                      <br>
                FWPS_LAYER_OUTBOUND_IPPACKET_V{4/6}                                             <br>
                FWPS_LAYER_OUTBOUND_IPPACKET_V{4/6}_DISCARD                                     <br>
                FWPS_LAYER_IPFORWARD_V{4/6}                                                     <br>
                FWPS_LAYER_IPFORWARD_V{4/6}_DISCARD                                             <br>
                FWPS_LAYER_INBOUND_TRANSPORT_V{4/6}                                             <br>
                FWPS_LAYER_INBOUND_TRANSPORT_V{4/6}_DISCARD                                     <br>
                FWPS_LAYER_OUTBOUND_TRANSPORT_V{4/6}                                            <br>
                FWPS_LAYER_OUTBOUND_TRANSPORT_V{4/6}_DISCARD                                    <br>
                FWPS_LAYER_STREAM_V{4/6}                                                        <br>
                FWPS_LAYER_STREAM_V{4/6}_DISCARD                                                <br>
                FWPS_LAYER_DATAGRAM_DATA_V{4/6}                                                 <br>
                FWPS_LAYER_DATAGRAM_DATA_V{4/6}_DISCARD                                         <br>
                FWPS_LAYER_INBOUND_ICMP_ERROR_V{4/6}                                            <br>
                FWPS_LAYER_INBOUND_ICMP_ERROR_V{4/6}_DISCARD                                    <br>
                FWPS_LAYER_OUTBOUND_ICMP_ERROR_V{4/6}                                           <br>
                FWPS_LAYER_OUTBOUND_ICMP_ERROR_V{4/6}_DISCARD                                   <br>
                FWPS_LAYER_ALE_RESOURCE_ASSIGNMENT_V{4/6}                                       <br>
                FWPS_LAYER_ALE_RESOURCE_ASSIGNMENT_V{4/6}_DISCARD                               <br>
                FWPS_LAYER_ALE_AUTH_LISTEN_V{4/6}                                               <br>
                FWPS_LAYER_ALE_AUTH_LISTEN_V{4/6}_DISCARD                                       <br>
                FWPS_LAYER_ALE_AUTH_RECV_ACCEPT_V{4/6}                                          <br>
                FWPS_LAYER_ALE_AUTH_RECV_ACCEPT_V{4/6}_DISCARD                                  <br>
                FWPS_LAYER_ALE_AUTH_CONNECT_V{4/6}                                              <br>
                FWPS_LAYER_ALE_AUTH_CONNECT_V{4/6}_DISCARD                                      <br>
                FWPS_LAYER_ALE_FLOW_ESTABLISHED_V{4/6}                                          <br>
                FWPS_LAYER_ALE_FLOW_ESTABLISHED_V{4/6}_DISCARD                                  <br>
                FWPS_LAYER_ALE_RESOURCE_RELEASE_V{4/6}                                          <br>
                FWPS_LAYER_ALE_ENDPOINT_CLOSURE_V{4/6}                                          <br>
                FWPS_LAYER_ALE_CONNECT_REDIRECT_V{4/6}                                          <br>
                FWPS_LAYER_ALE_BIND_REDIRECT_V{4/6}                                             <br>
                FWPS_LAYER_STREAM_PACKET_V{4/6}                                                 <br>
                FWPS_LAYER_INBOUND_MAC_FRAME_ETHERNET                                           <br>
                FWPS_LAYER_OUTBOUND_MAC_FRAME_ETHERNET                                          <br>
                FWPS_LAYER_INBOUND_MAC_FRAME_NATIVE                                             <br>
                FWPS_LAYER_OUTBOUND_MAC_FRAME_NATIVE                                            <br>
                FWPS_LAYER_INGRESS_VSWITCH_ETHERNET                                             <br>
                FWPS_LAYER_EGRESS_VSWITCH_ETHERNET                                              <br>
                FWPS_LAYER_INGRESS_VSWITCH_TRANSPORT_V{4/6}                                     <br>
                FWPS_LAYER_EGRESS_VSWITCH_TRANSPORT_V{4/6}                                      <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/FF544893.aspx                              <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
VOID NTAPI ClassifyBasicPacketExamination(_In_ const FWPS_INCOMING_VALUES0* pClassifyValues,
                                          _In_ const FWPS_INCOMING_METADATA_VALUES0* pMetadata,
                                          _Inout_opt_ VOID* pLayerData,
                                          _In_opt_ const VOID* pClassifyContext,
                                          _In_ const FWPS_FILTER* pFilter,
                                          _In_ UINT64 flowContext,
                                          _Inout_ FWPS_CLASSIFY_OUT* pClassifyOut)
{
   NT_ASSERT(pClassifyValues);
   NT_ASSERT(pMetadata);
   NT_ASSERT(pFilter);
   NT_ASSERT(pClassifyOut);

#if(NTDDI_VERSION >= NTDDI_WIN8)

   NT_ASSERT(pClassifyValues->layerId == FWPS_LAYER_INBOUND_IPPACKET_V4 ||
             pClassifyValues->layerId == FWPS_LAYER_INBOUND_IPPACKET_V4_DISCARD ||
             pClassifyValues->layerId == FWPS_LAYER_INBOUND_IPPACKET_V6 ||
             pClassifyValues->layerId == FWPS_LAYER_INBOUND_IPPACKET_V6_DISCARD ||
             pClassifyValues->layerId == FWPS_LAYER_OUTBOUND_IPPACKET_V4 ||
             pClassifyValues->layerId == FWPS_LAYER_OUTBOUND_IPPACKET_V4_DISCARD ||
             pClassifyValues->layerId == FWPS_LAYER_OUTBOUND_IPPACKET_V6 ||
             pClassifyValues->layerId == FWPS_LAYER_OUTBOUND_IPPACKET_V6_DISCARD ||
             pClassifyValues->layerId == FWPS_LAYER_IPFORWARD_V4 ||
             pClassifyValues->layerId == FWPS_LAYER_IPFORWARD_V4_DISCARD ||
             pClassifyValues->layerId == FWPS_LAYER_IPFORWARD_V6 ||
             pClassifyValues->layerId == FWPS_LAYER_IPFORWARD_V6_DISCARD ||
             pClassifyValues->layerId == FWPS_LAYER_INBOUND_TRANSPORT_V4 ||
             pClassifyValues->layerId == FWPS_LAYER_INBOUND_TRANSPORT_V4_DISCARD ||
             pClassifyValues->layerId == FWPS_LAYER_INBOUND_TRANSPORT_V6 ||
             pClassifyValues->layerId == FWPS_LAYER_INBOUND_TRANSPORT_V6_DISCARD ||
             pClassifyValues->layerId == FWPS_LAYER_OUTBOUND_TRANSPORT_V4 ||
             pClassifyValues->layerId == FWPS_LAYER_OUTBOUND_TRANSPORT_V4_DISCARD ||
             pClassifyValues->layerId == FWPS_LAYER_OUTBOUND_TRANSPORT_V6 ||
             pClassifyValues->layerId == FWPS_LAYER_OUTBOUND_TRANSPORT_V6_DISCARD ||
             pClassifyValues->layerId == FWPS_LAYER_STREAM_V4 ||
             pClassifyValues->layerId == FWPS_LAYER_STREAM_V4_DISCARD ||
             pClassifyValues->layerId == FWPS_LAYER_STREAM_V6 ||
             pClassifyValues->layerId == FWPS_LAYER_STREAM_V6_DISCARD ||
             pClassifyValues->layerId == FWPS_LAYER_DATAGRAM_DATA_V4 ||
             pClassifyValues->layerId == FWPS_LAYER_DATAGRAM_DATA_V4_DISCARD ||
             pClassifyValues->layerId == FWPS_LAYER_DATAGRAM_DATA_V6 ||
             pClassifyValues->layerId == FWPS_LAYER_DATAGRAM_DATA_V6_DISCARD ||
             pClassifyValues->layerId == FWPS_LAYER_INBOUND_ICMP_ERROR_V4 ||
             pClassifyValues->layerId == FWPS_LAYER_INBOUND_ICMP_ERROR_V4_DISCARD ||
             pClassifyValues->layerId == FWPS_LAYER_INBOUND_ICMP_ERROR_V6 ||
             pClassifyValues->layerId == FWPS_LAYER_INBOUND_ICMP_ERROR_V6_DISCARD ||
             pClassifyValues->layerId == FWPS_LAYER_OUTBOUND_ICMP_ERROR_V4 ||
             pClassifyValues->layerId == FWPS_LAYER_OUTBOUND_ICMP_ERROR_V4_DISCARD ||
             pClassifyValues->layerId == FWPS_LAYER_OUTBOUND_ICMP_ERROR_V6 ||
             pClassifyValues->layerId == FWPS_LAYER_OUTBOUND_ICMP_ERROR_V6_DISCARD ||
             pClassifyValues->layerId == FWPS_LAYER_ALE_RESOURCE_ASSIGNMENT_V4 ||
             pClassifyValues->layerId == FWPS_LAYER_ALE_RESOURCE_ASSIGNMENT_V4_DISCARD ||
             pClassifyValues->layerId == FWPS_LAYER_ALE_RESOURCE_ASSIGNMENT_V6 ||
             pClassifyValues->layerId == FWPS_LAYER_ALE_RESOURCE_ASSIGNMENT_V6_DISCARD ||
             pClassifyValues->layerId == FWPS_LAYER_ALE_AUTH_LISTEN_V4 ||
             pClassifyValues->layerId == FWPS_LAYER_ALE_AUTH_LISTEN_V4_DISCARD ||
             pClassifyValues->layerId == FWPS_LAYER_ALE_AUTH_LISTEN_V6 ||
             pClassifyValues->layerId == FWPS_LAYER_ALE_AUTH_LISTEN_V6_DISCARD ||
             pClassifyValues->layerId == FWPS_LAYER_ALE_AUTH_RECV_ACCEPT_V4 ||
             pClassifyValues->layerId == FWPS_LAYER_ALE_AUTH_RECV_ACCEPT_V4_DISCARD ||
             pClassifyValues->layerId == FWPS_LAYER_ALE_AUTH_RECV_ACCEPT_V6 ||
             pClassifyValues->layerId == FWPS_LAYER_ALE_AUTH_RECV_ACCEPT_V6_DISCARD ||
             pClassifyValues->layerId == FWPS_LAYER_ALE_AUTH_CONNECT_V4 ||
             pClassifyValues->layerId == FWPS_LAYER_ALE_AUTH_CONNECT_V4_DISCARD ||
             pClassifyValues->layerId == FWPS_LAYER_ALE_AUTH_CONNECT_V6 ||
             pClassifyValues->layerId == FWPS_LAYER_ALE_AUTH_CONNECT_V6_DISCARD ||
             pClassifyValues->layerId == FWPS_LAYER_ALE_FLOW_ESTABLISHED_V4 ||
             pClassifyValues->layerId == FWPS_LAYER_ALE_FLOW_ESTABLISHED_V4_DISCARD ||
             pClassifyValues->layerId == FWPS_LAYER_ALE_FLOW_ESTABLISHED_V6 ||
             pClassifyValues->layerId == FWPS_LAYER_ALE_FLOW_ESTABLISHED_V6_DISCARD ||
             pClassifyValues->layerId == FWPS_LAYER_ALE_RESOURCE_RELEASE_V4 ||
             pClassifyValues->layerId == FWPS_LAYER_ALE_RESOURCE_RELEASE_V6 ||
             pClassifyValues->layerId == FWPS_LAYER_ALE_ENDPOINT_CLOSURE_V4 ||
             pClassifyValues->layerId == FWPS_LAYER_ALE_ENDPOINT_CLOSURE_V6 ||
             pClassifyValues->layerId == FWPS_LAYER_ALE_CONNECT_REDIRECT_V4 ||
             pClassifyValues->layerId == FWPS_LAYER_ALE_CONNECT_REDIRECT_V6 ||
             pClassifyValues->layerId == FWPS_LAYER_ALE_BIND_REDIRECT_V4 ||
             pClassifyValues->layerId == FWPS_LAYER_ALE_BIND_REDIRECT_V6 ||
             pClassifyValues->layerId == FWPS_LAYER_STREAM_PACKET_V4 ||
             pClassifyValues->layerId == FWPS_LAYER_STREAM_PACKET_V6 ||
             pClassifyValues->layerId == FWPS_LAYER_INBOUND_MAC_FRAME_ETHERNET ||
             pClassifyValues->layerId == FWPS_LAYER_OUTBOUND_MAC_FRAME_ETHERNET ||
             pClassifyValues->layerId == FWPS_LAYER_INBOUND_MAC_FRAME_NATIVE ||
             pClassifyValues->layerId == FWPS_LAYER_OUTBOUND_MAC_FRAME_NATIVE ||
             pClassifyValues->layerId == FWPS_LAYER_INGRESS_VSWITCH_ETHERNET ||
             pClassifyValues->layerId == FWPS_LAYER_EGRESS_VSWITCH_ETHERNET ||
             pClassifyValues->layerId == FWPS_LAYER_INGRESS_VSWITCH_TRANSPORT_V4 ||
             pClassifyValues->layerId == FWPS_LAYER_INGRESS_VSWITCH_TRANSPORT_V6 ||
             pClassifyValues->layerId == FWPS_LAYER_EGRESS_VSWITCH_TRANSPORT_V4 ||
             pClassifyValues->layerId == FWPS_LAYER_EGRESS_VSWITCH_TRANSPORT_V6);

#else

   NT_ASSERT(pClassifyValues->layerId == FWPS_LAYER_INBOUND_IPPACKET_V4 ||
             pClassifyValues->layerId == FWPS_LAYER_INBOUND_IPPACKET_V4_DISCARD ||
             pClassifyValues->layerId == FWPS_LAYER_INBOUND_IPPACKET_V6 ||
             pClassifyValues->layerId == FWPS_LAYER_INBOUND_IPPACKET_V6_DISCARD ||
             pClassifyValues->layerId == FWPS_LAYER_OUTBOUND_IPPACKET_V4 ||
             pClassifyValues->layerId == FWPS_LAYER_OUTBOUND_IPPACKET_V4_DISCARD ||
             pClassifyValues->layerId == FWPS_LAYER_OUTBOUND_IPPACKET_V6 ||
             pClassifyValues->layerId == FWPS_LAYER_OUTBOUND_IPPACKET_V6_DISCARD ||
             pClassifyValues->layerId == FWPS_LAYER_IPFORWARD_V4 ||
             pClassifyValues->layerId == FWPS_LAYER_IPFORWARD_V4_DISCARD ||
             pClassifyValues->layerId == FWPS_LAYER_IPFORWARD_V6 ||
             pClassifyValues->layerId == FWPS_LAYER_IPFORWARD_V6_DISCARD ||
             pClassifyValues->layerId == FWPS_LAYER_INBOUND_TRANSPORT_V4 ||
             pClassifyValues->layerId == FWPS_LAYER_INBOUND_TRANSPORT_V4_DISCARD ||
             pClassifyValues->layerId == FWPS_LAYER_INBOUND_TRANSPORT_V6 ||
             pClassifyValues->layerId == FWPS_LAYER_INBOUND_TRANSPORT_V6_DISCARD ||
             pClassifyValues->layerId == FWPS_LAYER_OUTBOUND_TRANSPORT_V4 ||
             pClassifyValues->layerId == FWPS_LAYER_OUTBOUND_TRANSPORT_V4_DISCARD ||
             pClassifyValues->layerId == FWPS_LAYER_OUTBOUND_TRANSPORT_V6 ||
             pClassifyValues->layerId == FWPS_LAYER_OUTBOUND_TRANSPORT_V6_DISCARD ||
             pClassifyValues->layerId == FWPS_LAYER_STREAM_V4 ||
             pClassifyValues->layerId == FWPS_LAYER_STREAM_V4_DISCARD ||
             pClassifyValues->layerId == FWPS_LAYER_STREAM_V4 ||
             pClassifyValues->layerId == FWPS_LAYER_STREAM_V6_DISCARD ||
             pClassifyValues->layerId == FWPS_LAYER_DATAGRAM_DATA_V4 ||
             pClassifyValues->layerId == FWPS_LAYER_DATAGRAM_DATA_V4_DISCARD ||
             pClassifyValues->layerId == FWPS_LAYER_DATAGRAM_DATA_V6 ||
             pClassifyValues->layerId == FWPS_LAYER_DATAGRAM_DATA_V6_DISCARD ||
             pClassifyValues->layerId == FWPS_LAYER_INBOUND_ICMP_ERROR_V4 ||
             pClassifyValues->layerId == FWPS_LAYER_INBOUND_ICMP_ERROR_V4_DISCARD ||
             pClassifyValues->layerId == FWPS_LAYER_INBOUND_ICMP_ERROR_V6 ||
             pClassifyValues->layerId == FWPS_LAYER_INBOUND_ICMP_ERROR_V6_DISCARD ||
             pClassifyValues->layerId == FWPS_LAYER_OUTBOUND_ICMP_ERROR_V4 ||
             pClassifyValues->layerId == FWPS_LAYER_OUTBOUND_ICMP_ERROR_V4_DISCARD ||
             pClassifyValues->layerId == FWPS_LAYER_OUTBOUND_ICMP_ERROR_V6 ||
             pClassifyValues->layerId == FWPS_LAYER_OUTBOUND_ICMP_ERROR_V6_DISCARD ||
             pClassifyValues->layerId == FWPS_LAYER_ALE_RESOURCE_ASSIGNMENT_V4 ||
             pClassifyValues->layerId == FWPS_LAYER_ALE_RESOURCE_ASSIGNMENT_V4_DISCARD ||
             pClassifyValues->layerId == FWPS_LAYER_ALE_RESOURCE_ASSIGNMENT_V6 ||
             pClassifyValues->layerId == FWPS_LAYER_ALE_RESOURCE_ASSIGNMENT_V6_DISCARD ||
             pClassifyValues->layerId == FWPS_LAYER_ALE_AUTH_LISTEN_V4 ||
             pClassifyValues->layerId == FWPS_LAYER_ALE_AUTH_LISTEN_V4_DISCARD ||
             pClassifyValues->layerId == FWPS_LAYER_ALE_AUTH_LISTEN_V6 ||
             pClassifyValues->layerId == FWPS_LAYER_ALE_AUTH_LISTEN_V6_DISCARD ||
             pClassifyValues->layerId == FWPS_LAYER_ALE_AUTH_RECV_ACCEPT_V4 ||
             pClassifyValues->layerId == FWPS_LAYER_ALE_AUTH_RECV_ACCEPT_V4_DISCARD ||
             pClassifyValues->layerId == FWPS_LAYER_ALE_AUTH_RECV_ACCEPT_V6 ||
             pClassifyValues->layerId == FWPS_LAYER_ALE_AUTH_RECV_ACCEPT_V6_DISCARD ||
             pClassifyValues->layerId == FWPS_LAYER_ALE_AUTH_CONNECT_V4 ||
             pClassifyValues->layerId == FWPS_LAYER_ALE_AUTH_CONNECT_V4_DISCARD ||
             pClassifyValues->layerId == FWPS_LAYER_ALE_AUTH_CONNECT_V6 ||
             pClassifyValues->layerId == FWPS_LAYER_ALE_AUTH_CONNECT_V6_DISCARD ||
             pClassifyValues->layerId == FWPS_LAYER_ALE_FLOW_ESTABLISHED_V4 ||
             pClassifyValues->layerId == FWPS_LAYER_ALE_FLOW_ESTABLISHED_V4_DISCARD ||
             pClassifyValues->layerId == FWPS_LAYER_ALE_FLOW_ESTABLISHED_V6 ||
             pClassifyValues->layerId == FWPS_LAYER_ALE_FLOW_ESTABLISHED_V6_DISCARD ||
             pClassifyValues->layerId == FWPS_LAYER_ALE_RESOURCE_RELEASE_V4 ||
             pClassifyValues->layerId == FWPS_LAYER_ALE_RESOURCE_RELEASE_V6 ||
             pClassifyValues->layerId == FWPS_LAYER_ALE_ENDPOINT_CLOSURE_V4 ||
             pClassifyValues->layerId == FWPS_LAYER_ALE_ENDPOINT_CLOSURE_V6 ||
             pClassifyValues->layerId == FWPS_LAYER_ALE_CONNECT_REDIRECT_V4 ||
             pClassifyValues->layerId == FWPS_LAYER_ALE_CONNECT_REDIRECT_V6 ||
             pClassifyValues->layerId == FWPS_LAYER_ALE_BIND_REDIRECT_V4 ||
             pClassifyValues->layerId == FWPS_LAYER_ALE_BIND_REDIRECT_V6 ||
             pClassifyValues->layerId == FWPS_LAYER_STREAM_PACKET_V4 ||
             pClassifyValues->layerId == FWPS_LAYER_STREAM_PACKET_V6);

#endif /// (NTDDI_VERSION >= NTDDI_WIN8)

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> ClassifyBasicPacketExamination() [Layer: %s][FilterID: %#I64x][Rights: %#x]",
              KrnlHlprFwpsLayerIDToString(pClassifyValues->layerId),
              pFilter->filterId,
              pClassifyOut->rights);

   NTSTATUS       status          = STATUS_SUCCESS;
   CLASSIFY_DATA* pClassifyData   = 0;
   FWP_DIRECTION  direction       = FWP_DIRECTION_MAX;
   FWP_VALUE*     pDirectionValue = 0;
   KIRQL          originalIRQL    = PASSIVE_LEVEL;

   HLPR_NEW(pClassifyData,
            CLASSIFY_DATA,
            WFPSAMPLER_CALLOUT_DRIVER_TAG);
   HLPR_BAIL_ON_ALLOC_FAILURE(pClassifyData,
                              status);

   if(pFilter->context & PCPEF_EXAMINE_UNDER_LOCK)
      KeAcquireSpinLock(&g_bpeSpinLock,
                        &originalIRQL);

   if(pFilter->context & PCPEF_EXAMINE_INCOMING_VALUES)
      LogClassifyValues(pClassifyValues);

   if(pFilter->context & PCPEF_EXAMINE_INCOMING_METADATA_VALUES)
      LogMetadata(pMetadata);

   if(pFilter->context & PCPEF_EXAMINE_LAYER_DATA)
      DbgPrintEx(DPFLTR_IHVNETWORK_ID,
           DPFLTR_INFO_LEVEL,
           "\tLayerData: %#p",
           pLayerData);

   if(pClassifyValues->layerId == FWPS_LAYER_ALE_AUTH_RECV_ACCEPT_V4 ||
      pClassifyValues->layerId == FWPS_LAYER_ALE_AUTH_RECV_ACCEPT_V6)
      direction = FWP_DIRECTION_INBOUND;
   else if(pClassifyValues->layerId == FWPS_LAYER_ALE_AUTH_CONNECT_V4 ||
           pClassifyValues->layerId == FWPS_LAYER_ALE_AUTH_CONNECT_V6)
      direction = FWP_DIRECTION_OUTBOUND;

   pDirectionValue = KrnlHlprFwpValueGetFromFwpsIncomingValues(pClassifyValues,
                                                               &FWPM_CONDITION_DIRECTION);
   if(pDirectionValue)
      direction = (FWP_DIRECTION)pDirectionValue->uint32;
   else
   {
      if(FWPS_IS_METADATA_FIELD_PRESENT(pMetadata,
                                        FWPS_METADATA_FIELD_PACKET_DIRECTION))
         direction = pMetadata->packetDirection;
   }

   pClassifyData->pClassifyValues  = pClassifyValues;
   pClassifyData->pMetadataValues  = pMetadata;
   pClassifyData->pPacket          = pLayerData;
   pClassifyData->pClassifyContext = pClassifyContext;
   pClassifyData->pFilter          = pFilter;
   pClassifyData->flowContext      = flowContext;
   pClassifyData->pClassifyOut     = pClassifyOut;

   if(pClassifyData->pPacket)
   {
      if(pClassifyValues->layerId == FWPS_LAYER_INBOUND_IPPACKET_V4 ||
         pClassifyValues->layerId == FWPS_LAYER_INBOUND_IPPACKET_V6)
         PerformBasicPacketExaminationAtInboundNetwork(pClassifyData);
      else if(pClassifyValues->layerId == FWPS_LAYER_OUTBOUND_IPPACKET_V4 ||
              pClassifyValues->layerId == FWPS_LAYER_OUTBOUND_IPPACKET_V6)
         PerformBasicPacketExaminationAtOutboundNetwork(pClassifyData);
      else if(pClassifyValues->layerId == FWPS_LAYER_IPFORWARD_V4 ||
              pClassifyValues->layerId == FWPS_LAYER_IPFORWARD_V6)
         PerformBasicPacketExaminationAtForward(pClassifyData);
      else if(pClassifyValues->layerId == FWPS_LAYER_INBOUND_TRANSPORT_V4 ||
              pClassifyValues->layerId == FWPS_LAYER_INBOUND_TRANSPORT_V6 ||
              pClassifyValues->layerId == FWPS_LAYER_INBOUND_ICMP_ERROR_V4 ||
              pClassifyValues->layerId == FWPS_LAYER_INBOUND_ICMP_ERROR_V6 ||
              (direction == FWP_DIRECTION_INBOUND &&
              (pClassifyValues->layerId == FWPS_LAYER_DATAGRAM_DATA_V4 ||
              pClassifyValues->layerId == FWPS_LAYER_DATAGRAM_DATA_V6 ||
              pClassifyValues->layerId == FWPS_LAYER_ALE_AUTH_RECV_ACCEPT_V4 ||
              pClassifyValues->layerId == FWPS_LAYER_ALE_AUTH_RECV_ACCEPT_V6 ||
              pClassifyValues->layerId == FWPS_LAYER_ALE_AUTH_CONNECT_V4 ||     /// Policy Change Reauthorization
              pClassifyValues->layerId == FWPS_LAYER_ALE_AUTH_CONNECT_V6 ||     /// Policy Change Reauthorization
              pClassifyValues->layerId == FWPS_LAYER_ALE_FLOW_ESTABLISHED_V4 ||
              pClassifyValues->layerId == FWPS_LAYER_ALE_FLOW_ESTABLISHED_V6 ||
              pClassifyValues->layerId == FWPS_LAYER_STREAM_PACKET_V4 ||
              pClassifyValues->layerId == FWPS_LAYER_STREAM_PACKET_V6)))
         PerformBasicPacketExaminationAtInboundTransport(pClassifyData);
      else if(pClassifyValues->layerId == FWPS_LAYER_OUTBOUND_TRANSPORT_V4 ||
              pClassifyValues->layerId == FWPS_LAYER_OUTBOUND_TRANSPORT_V6 ||
              pClassifyValues->layerId == FWPS_LAYER_OUTBOUND_ICMP_ERROR_V4 ||
              pClassifyValues->layerId == FWPS_LAYER_OUTBOUND_ICMP_ERROR_V6 ||
              (direction == FWP_DIRECTION_OUTBOUND &&
              (pClassifyValues->layerId == FWPS_LAYER_DATAGRAM_DATA_V4 ||
              pClassifyValues->layerId == FWPS_LAYER_DATAGRAM_DATA_V6 ||
              pClassifyValues->layerId == FWPS_LAYER_ALE_AUTH_RECV_ACCEPT_V4 || /// Policy Change Reauthorization
              pClassifyValues->layerId == FWPS_LAYER_ALE_AUTH_RECV_ACCEPT_V6 || /// Policy Change Reauthorization
              pClassifyValues->layerId == FWPS_LAYER_ALE_AUTH_CONNECT_V4 ||
              pClassifyValues->layerId == FWPS_LAYER_ALE_AUTH_CONNECT_V6 ||
              pClassifyValues->layerId == FWPS_LAYER_ALE_FLOW_ESTABLISHED_V4 ||
              pClassifyValues->layerId == FWPS_LAYER_ALE_FLOW_ESTABLISHED_V6 ||
              pClassifyValues->layerId == FWPS_LAYER_STREAM_PACKET_V4 ||
              pClassifyValues->layerId == FWPS_LAYER_STREAM_PACKET_V6)))
         PerformBasicPacketExaminationAtOutboundTransport(pClassifyData);

#if(NTDDI_VERSION >= NTDDI_WIN8)

      else if(pClassifyValues->layerId == FWPS_LAYER_INBOUND_MAC_FRAME_ETHERNET ||
              pClassifyValues->layerId == FWPS_LAYER_INBOUND_MAC_FRAME_NATIVE)
         PerformBasicPacketExaminationAtInboundMACFrame(pClassifyData);
      else if(pClassifyValues->layerId == FWPS_LAYER_OUTBOUND_MAC_FRAME_ETHERNET ||
              pClassifyValues->layerId == FWPS_LAYER_OUTBOUND_MAC_FRAME_NATIVE)
         PerformBasicPacketExaminationAtOutboundMACFrame(pClassifyData);
      else if(pClassifyValues->layerId == FWPS_LAYER_INGRESS_VSWITCH_ETHERNET ||
              pClassifyValues->layerId == FWPS_LAYER_EGRESS_VSWITCH_ETHERNET)
         PerformBasicPacketExaminationAtVSwitchEthernet(pClassifyData);
      else if(pClassifyValues->layerId == FWPS_LAYER_INGRESS_VSWITCH_TRANSPORT_V4 ||
              pClassifyValues->layerId == FWPS_LAYER_INGRESS_VSWITCH_TRANSPORT_V6 ||
              pClassifyValues->layerId == FWPS_LAYER_EGRESS_VSWITCH_TRANSPORT_V4 ||
              pClassifyValues->layerId == FWPS_LAYER_EGRESS_VSWITCH_TRANSPORT_V6)
         PerformBasicPacketExaminationAtVSwitchTransport(pClassifyData);

#endif // (NTDDI_VERSION >= NTDDI_WIN8)

      else if(KrnlHlprFwpsLayerIsDiscard(pClassifyValues->layerId))
         PerformBasicPacketExaminationAtDiscard(pClassifyData);
      else
         PerformBasicPacketExaminationAtOther(pClassifyData);
   }
   else
   {
      if(KrnlHlprFwpsLayerIsDiscard(pClassifyValues->layerId))
         PerformBasicPacketExaminationAtDiscard(pClassifyData);
      else
         PerformBasicPacketExaminationAtOther(pClassifyData);
   }

   if(pFilter->context & PCPEF_EXAMINE_CLASSIFY_CONTEXT)
      DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                 DPFLTR_INFO_LEVEL,
                 "\tClassifyContext: %#p",
                 pClassifyContext);

   if(pFilter->context & PCPEF_EXAMINE_FILTER)
      LogFilter(pFilter);

   if(pFilter->context & PCPEF_EXAMINE_FLOW_CONTEXT)
      DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                 DPFLTR_INFO_LEVEL,
                 "\tflowContext: %#I64x",
                 flowContext);

   if(pFilter->context & PCPEF_EXAMINE_CLASSIFY_OUT)
      LogClassifyOut(pClassifyOut);

   if(pFilter->context & PCPEF_EXAMINE_UNDER_LOCK)
      KeReleaseSpinLock(&g_bpeSpinLock,
                        originalIRQL);

   HLPR_BAIL_LABEL:

   if(status != STATUS_SUCCESS)
      DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                 DPFLTR_ERROR_LEVEL,
                 " !!!! ClassifyBasicPacketExamination: [status: %#x]\n",
                 status);

   HLPR_DELETE(pClassifyData,
               WFPSAMPLER_CALLOUT_DRIVER_TAG);

   if(pClassifyOut->rights & FWPS_RIGHT_ACTION_WRITE)
      pClassifyOut->actionType = FWP_ACTION_CONTINUE;

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- ClassifyBasicPacketExamination() [Layer: %s][FilterID: %#I64x][Action: %#x][Rights: %#x][Absorb: %s]\n",
              KrnlHlprFwpsLayerIDToString(pClassifyValues->layerId),
              pFilter->filterId,
              pClassifyOut->actionType,
              pClassifyOut->rights,
              (pClassifyOut->flags & FWPS_CLASSIFY_OUT_FLAG_ABSORB) ? "TRUE" : "FALSE");

   return;
}

#else

/**
 @classify_function="ClassifyBasicPacketExamination"
 
   Purpose:  Examines the packet and returns FWP_ACTION_CONTINUE.                               <br>
                                                                                                <br>
   Notes:    Applies to the following layers:                                                   <br>
                FWPS_LAYER_INBOUND_IPPACKET_V{4/6}                                              <br>
                FWPS_LAYER_INBOUND_IPPACKET_V{4/6}_DISCARD                                      <br>
                FWPS_LAYER_OUTBOUND_IPPACKET_V{4/6}                                             <br>
                FWPS_LAYER_OUTBOUND_IPPACKET_V{4/6}_DISCARD                                     <br>
                FWPS_LAYER_IPFORWARD_V{4/6}                                                     <br>
                FWPS_LAYER_IPFORWARD_V{4/6}_DISCARD                                             <br>
                FWPS_LAYER_INBOUND_TRANSPORT_V{4/6}                                             <br>
                FWPS_LAYER_INBOUND_TRANSPORT_V{4/6}_DISCARD                                     <br>
                FWPS_LAYER_OUTBOUND_TRANSPORT_V{4/6}                                            <br>
                FWPS_LAYER_OUTBOUND_TRANSPORT_V{4/6}_DISCARD                                    <br>
                FWPS_LAYER_STREAM_V{4/6}                                                        <br>
                FWPS_LAYER_STREAM_V{4/6}_DISCARD                                                <br>
                FWPS_LAYER_DATAGRAM_DATA_V{4/6}                                                 <br>
                FWPS_LAYER_DATAGRAM_DATA_V{4/6}_DISCARD                                         <br>
                FWPS_LAYER_INBOUND_ICMP_ERROR_V{4/6}                                            <br>
                FWPS_LAYER_INBOUND_ICMP_ERROR_V{4/6}_DISCARD                                    <br>
                FWPS_LAYER_OUTBOUND_ICMP_ERROR_V{4/6}                                           <br>
                FWPS_LAYER_OUTBOUND_ICMP_ERROR_V{4/6}_DISCARD                                   <br>
                FWPS_LAYER_ALE_RESOURCE_ASSIGNMENT_V{4/6}                                       <br>
                FWPS_LAYER_ALE_RESOURCE_ASSIGNMENT_V{4/6}_DISCARD                               <br>
                FWPS_LAYER_ALE_AUTH_LISTEN_V{4/6}                                               <br>
                FWPS_LAYER_ALE_AUTH_LISTEN_V{4/6}_DISCARD                                       <br>
                FWPS_LAYER_ALE_AUTH_RECV_ACCEPT_V{4/6}                                          <br>
                FWPS_LAYER_ALE_AUTH_RECV_ACCEPT_V{4/6}_DISCARD                                  <br>
                FWPS_LAYER_ALE_AUTH_CONNECT_V{4/6}                                              <br>
                FWPS_LAYER_ALE_AUTH_CONNECT_V{4/6}_DISCARD                                      <br>
                FWPS_LAYER_ALE_FLOW_ESTABLISHED_V{4/6}                                          <br>
                FWPS_LAYER_ALE_FLOW_ESTABLISHED_V{4/6}_DISCARD                                  <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/FF544890.aspx                              <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
VOID NTAPI ClassifyBasicPacketExamination(_In_ const FWPS_INCOMING_VALUES* pClassifyValues,
                                          _In_ const FWPS_INCOMING_METADATA_VALUES* pMetadata,
                                          _Inout_opt_ VOID* pLayerData,
                                          _In_ const FWPS_FILTER* pFilter,
                                          _In_ UINT64 flowContext,
                                          _Inout_ FWPS_CLASSIFY_OUT* pClassifyOut)
{
   NT_ASSERT(pClassifyValues);
   NT_ASSERT(pMetadata);
   NT_ASSERT(pFilter);
   NT_ASSERT(pClassifyOut);
   NT_ASSERT(pClassifyValues->layerId == FWPS_LAYER_INBOUND_IPPACKET_V4 ||
             pClassifyValues->layerId == FWPS_LAYER_INBOUND_IPPACKET_V4_DISCARD ||
             pClassifyValues->layerId == FWPS_LAYER_INBOUND_IPPACKET_V6 ||
             pClassifyValues->layerId == FWPS_LAYER_INBOUND_IPPACKET_V6_DISCARD ||
             pClassifyValues->layerId == FWPS_LAYER_OUTBOUND_IPPACKET_V4 ||
             pClassifyValues->layerId == FWPS_LAYER_OUTBOUND_IPPACKET_V4_DISCARD ||
             pClassifyValues->layerId == FWPS_LAYER_OUTBOUND_IPPACKET_V6 ||
             pClassifyValues->layerId == FWPS_LAYER_OUTBOUND_IPPACKET_V6_DISCARD ||
             pClassifyValues->layerId == FWPS_LAYER_IPFORWARD_V4 ||
             pClassifyValues->layerId == FWPS_LAYER_IPFORWARD_V4_DISCARD ||
             pClassifyValues->layerId == FWPS_LAYER_IPFORWARD_V6 ||
             pClassifyValues->layerId == FWPS_LAYER_IPFORWARD_V6_DISCARD ||
             pClassifyValues->layerId == FWPS_LAYER_INBOUND_TRANSPORT_V4 ||
             pClassifyValues->layerId == FWPS_LAYER_INBOUND_TRANSPORT_V4_DISCARD ||
             pClassifyValues->layerId == FWPS_LAYER_INBOUND_TRANSPORT_V6 ||
             pClassifyValues->layerId == FWPS_LAYER_INBOUND_TRANSPORT_V6_DISCARD ||
             pClassifyValues->layerId == FWPS_LAYER_OUTBOUND_TRANSPORT_V4 ||
             pClassifyValues->layerId == FWPS_LAYER_OUTBOUND_TRANSPORT_V4_DISCARD ||
             pClassifyValues->layerId == FWPS_LAYER_OUTBOUND_TRANSPORT_V6 ||
             pClassifyValues->layerId == FWPS_LAYER_OUTBOUND_TRANSPORT_V6_DISCARD ||
             pClassifyValues->layerId == FWPS_LAYER_STREAM_V4 ||
             pClassifyValues->layerId == FWPS_LAYER_STREAM_V4_DISCARD ||
             pClassifyValues->layerId == FWPS_LAYER_STREAM_V6 ||
             pClassifyValues->layerId == FWPS_LAYER_STREAM_V6_DISCARD ||
             pClassifyValues->layerId == FWPS_LAYER_DATAGRAM_DATA_V4 ||
             pClassifyValues->layerId == FWPS_LAYER_DATAGRAM_DATA_V4_DISCARD ||
             pClassifyValues->layerId == FWPS_LAYER_DATAGRAM_DATA_V6 ||
             pClassifyValues->layerId == FWPS_LAYER_DATAGRAM_DATA_V6_DISCARD ||
             pClassifyValues->layerId == FWPS_LAYER_INBOUND_ICMP_ERROR_V4 ||
             pClassifyValues->layerId == FWPS_LAYER_INBOUND_ICMP_ERROR_V4_DISCARD ||
             pClassifyValues->layerId == FWPS_LAYER_INBOUND_ICMP_ERROR_V6 ||
             pClassifyValues->layerId == FWPS_LAYER_INBOUND_ICMP_ERROR_V6_DISCARD ||
             pClassifyValues->layerId == FWPS_LAYER_OUTBOUND_ICMP_ERROR_V4 ||
             pClassifyValues->layerId == FWPS_LAYER_OUTBOUND_ICMP_ERROR_V4_DISCARD ||
             pClassifyValues->layerId == FWPS_LAYER_OUTBOUND_ICMP_ERROR_V6 ||
             pClassifyValues->layerId == FWPS_LAYER_OUTBOUND_ICMP_ERROR_V6_DISCARD ||
             pClassifyValues->layerId == FWPS_LAYER_ALE_RESOURCE_ASSIGNMENT_V4 ||
             pClassifyValues->layerId == FWPS_LAYER_ALE_RESOURCE_ASSIGNMENT_V4_DISCARD ||
             pClassifyValues->layerId == FWPS_LAYER_ALE_RESOURCE_ASSIGNMENT_V6 ||
             pClassifyValues->layerId == FWPS_LAYER_ALE_RESOURCE_ASSIGNMENT_V6_DISCARD ||
             pClassifyValues->layerId == FWPS_LAYER_ALE_AUTH_LISTEN_V4 ||
             pClassifyValues->layerId == FWPS_LAYER_ALE_AUTH_LISTEN_V4_DISCARD ||
             pClassifyValues->layerId == FWPS_LAYER_ALE_AUTH_LISTEN_V6 ||
             pClassifyValues->layerId == FWPS_LAYER_ALE_AUTH_LISTEN_V6_DISCARD ||
             pClassifyValues->layerId == FWPS_LAYER_ALE_AUTH_RECV_ACCEPT_V4 ||
             pClassifyValues->layerId == FWPS_LAYER_ALE_AUTH_RECV_ACCEPT_V4_DISCARD ||
             pClassifyValues->layerId == FWPS_LAYER_ALE_AUTH_RECV_ACCEPT_V6 ||
             pClassifyValues->layerId == FWPS_LAYER_ALE_AUTH_RECV_ACCEPT_V6_DISCARD ||
             pClassifyValues->layerId == FWPS_LAYER_ALE_AUTH_CONNECT_V4 ||
             pClassifyValues->layerId == FWPS_LAYER_ALE_AUTH_CONNECT_V4_DISCARD ||
             pClassifyValues->layerId == FWPS_LAYER_ALE_AUTH_CONNECT_V6 ||
             pClassifyValues->layerId == FWPS_LAYER_ALE_AUTH_CONNECT_V6_DISCARD ||
             pClassifyValues->layerId == FWPS_LAYER_ALE_FLOW_ESTABLISHED_V4 ||
             pClassifyValues->layerId == FWPS_LAYER_ALE_FLOW_ESTABLISHED_V4_DISCARD ||
             pClassifyValues->layerId == FWPS_LAYER_ALE_FLOW_ESTABLISHED_V6 ||
             pClassifyValues->layerId == FWPS_LAYER_ALE_FLOW_ESTABLISHED_V6_DISCARD);

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> ClassifyBasicPacketExamination() [Layer: %s][FilterID: %#I64x][Rights: %#x]",
              KrnlHlprFwpsLayerIDToString(pClassifyValues->layerId),
              pFilter->filterId,
              pClassifyOut->rights);

   NTSTATUS       status          = STATUS_SUCCESS;
   CLASSIFY_DATA* pClassifyData   = 0;
   FWP_DIRECTION  direction       = FWP_DIRECTION_MAX;
   FWP_VALUE*     pDirectionValue = 0;
   KIRQL          originalIRQL    = PASSIVE_LEVEL;

   HLPR_NEW(pClassifyData,
            CLASSIFY_DATA,
            WFPSAMPLER_CALLOUT_DRIVER_TAG);
   HLPR_BAIL_ON_ALLOC_FAILURE(pClassifyData,
                              status);

   if(pFilter->context & PCPEF_EXAMINE_UNDER_LOCK)
      KeAcquireSpinLock(&g_bpeSpinLock,
                        &originalIRQL);

   if(pFilter->context & PCPEF_EXAMINE_INCOMING_VALUES)
      LogClassifyValues(pClassifyValues);

   if(pFilter->context & PCPEF_EXAMINE_INCOMING_METADATA_VALUES)
      LogMetadata(pMetadata);

   if(pFilter->context & PCPEF_EXAMINE_LAYER_DATA)
      DbgPrintEx(DPFLTR_IHVNETWORK_ID,
           DPFLTR_INFO_LEVEL,
           "\tLayerData: %#p",
           pLayerData);

   if(pClassifyValues->layerId == FWPS_LAYER_ALE_AUTH_RECV_ACCEPT_V4 ||
      pClassifyValues->layerId == FWPS_LAYER_ALE_AUTH_RECV_ACCEPT_V6)
      direction = FWP_DIRECTION_INBOUND;
   else if(pClassifyValues->layerId == FWPS_LAYER_ALE_AUTH_CONNECT_V4 ||
           pClassifyValues->layerId == FWPS_LAYER_ALE_AUTH_CONNECT_V6)
      direction = FWP_DIRECTION_OUTBOUND;

   pDirectionValue = KrnlHlprFwpValueGetFromFwpsIncomingValues(pClassifyValues,
                                                               &FWPM_CONDITION_DIRECTION);
   if(pDirectionValue)
      direction = (FWP_DIRECTION)pDirectionValue->uint32;
   else
   {
      if(FWPS_IS_METADATA_FIELD_PRESENT(pMetadata,
                                        FWPS_METADATA_FIELD_PACKET_DIRECTION))
         direction = pMetadata->packetDirection;
   }

   pClassifyData->pClassifyValues = pClassifyValues;
   pClassifyData->pMetadataValues = pMetadata;
   pClassifyData->pPacket         = pLayerData;
   pClassifyData->pFilter         = pFilter;
   pClassifyData->flowContext     = flowContext;
   pClassifyData->pClassifyOut    = pClassifyOut;

   if(pClassifyData->pPacket)
   {
      if(pClassifyValues->layerId == FWPS_LAYER_INBOUND_IPPACKET_V4 ||
         pClassifyValues->layerId == FWPS_LAYER_INBOUND_IPPACKET_V6)
         PerformBasicPacketExaminationAtInboundNetwork(pClassifyData);
      else if(pClassifyValues->layerId == FWPS_LAYER_OUTBOUND_IPPACKET_V4 ||
              pClassifyValues->layerId == FWPS_LAYER_OUTBOUND_IPPACKET_V6)
         PerformBasicPacketExaminationAtOutboundNetwork(pClassifyData);
      else if(pClassifyValues->layerId == FWPS_LAYER_IPFORWARD_V4 ||
              pClassifyValues->layerId == FWPS_LAYER_IPFORWARD_V6)
         PerformBasicPacketExaminationAtForward(pClassifyData);
      else if(pClassifyValues->layerId == FWPS_LAYER_INBOUND_TRANSPORT_V4 ||
              pClassifyValues->layerId == FWPS_LAYER_INBOUND_TRANSPORT_V6 ||
              pClassifyValues->layerId == FWPS_LAYER_INBOUND_ICMP_ERROR_V4 ||
              pClassifyValues->layerId == FWPS_LAYER_INBOUND_ICMP_ERROR_V6 ||
              (direction == FWP_DIRECTION_INBOUND &&
              (pClassifyValues->layerId == FWPS_LAYER_DATAGRAM_DATA_V4 ||
              pClassifyValues->layerId == FWPS_LAYER_DATAGRAM_DATA_V6 ||
              pClassifyValues->layerId == FWPS_LAYER_ALE_AUTH_RECV_ACCEPT_V4 ||
              pClassifyValues->layerId == FWPS_LAYER_ALE_AUTH_RECV_ACCEPT_V6 ||
              pClassifyValues->layerId == FWPS_LAYER_ALE_AUTH_CONNECT_V4 ||     /// Policy Change Reauthorization
              pClassifyValues->layerId == FWPS_LAYER_ALE_AUTH_CONNECT_V6 ||     /// Policy Change Reauthorization
              pClassifyValues->layerId == FWPS_LAYER_ALE_FLOW_ESTABLISHED_V4 ||
              pClassifyValues->layerId == FWPS_LAYER_ALE_FLOW_ESTABLISHED_V6)))
         PerformBasicPacketExaminationAtInboundTransport(pClassifyData);
      else if(pClassifyValues->layerId == FWPS_LAYER_OUTBOUND_TRANSPORT_V4 ||
              pClassifyValues->layerId == FWPS_LAYER_OUTBOUND_TRANSPORT_V6 ||
              pClassifyValues->layerId == FWPS_LAYER_OUTBOUND_ICMP_ERROR_V4 ||
              pClassifyValues->layerId == FWPS_LAYER_OUTBOUND_ICMP_ERROR_V6 ||
              (direction == FWP_DIRECTION_OUTBOUND &&
              (pClassifyValues->layerId == FWPS_LAYER_DATAGRAM_DATA_V4 ||
              pClassifyValues->layerId == FWPS_LAYER_DATAGRAM_DATA_V6 ||
              pClassifyValues->layerId == FWPS_LAYER_ALE_AUTH_RECV_ACCEPT_V4 || /// Policy Change Reauthorization
              pClassifyValues->layerId == FWPS_LAYER_ALE_AUTH_RECV_ACCEPT_V6 || /// Policy Change Reauthorization
              pClassifyValues->layerId == FWPS_LAYER_ALE_AUTH_CONNECT_V4 ||
              pClassifyValues->layerId == FWPS_LAYER_ALE_AUTH_CONNECT_V6 ||
              pClassifyValues->layerId == FWPS_LAYER_ALE_FLOW_ESTABLISHED_V4 ||
              pClassifyValues->layerId == FWPS_LAYER_ALE_FLOW_ESTABLISHED_V6)))
         PerformBasicPacketExaminationAtOutboundTransport(pClassifyData);
      else if(KrnlHlprFwpsLayerIsDiscard(pClassifyValues->layerId))
         PerformBasicPacketExaminationAtDiscard(pClassifyData);
      else
         PerformBasicPacketExaminationAtOther(pClassifyData);
   }
   else
   {
      if(KrnlHlprFwpsLayerIsDiscard(pClassifyValues->layerId))
         PerformBasicPacketExaminationAtDiscard(pClassifyData);
      else
         PerformBasicPacketExaminationAtOther(pClassifyData);
   }

   if(pFilter->context & PCPEF_EXAMINE_FILTER)
      LogFilter(pFilter);

   if(pFilter->context & PCPEF_EXAMINE_FLOW_CONTEXT)
      DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                 DPFLTR_INFO_LEVEL,
                 "\tflowContext: %#I64x",
                 flowContext);

   if(pFilter->context & PCPEF_EXAMINE_CLASSIFY_OUT)
      LogClassifyOut(pClassifyOut);

   if(pFilter->context & PCPEF_EXAMINE_UNDER_LOCK)
      KeReleaseSpinLock(&g_bpeSpinLock,
                        originalIRQL);
 
   HLPR_BAIL_LABEL:

   if(status != STATUS_SUCCESS)
      DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                 DPFLTR_ERROR_LEVEL,
                 " !!!! ClassifyBasicPacketExamination: [status: %#x]\n",
                 status);

   HLPR_DELETE(pClassifyData,
               WFPSAMPLER_CALLOUT_DRIVER_TAG);

   if(pClassifyOut->rights & FWPS_RIGHT_ACTION_WRITE)
      pClassifyOut->actionType = FWP_ACTION_CONTINUE;

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- ClassifyBasicPacketExamination() [Layer: %s][FilterID: %#I64x][Action: %#x][Rights: %#x][Absorb: %s]\n",
              KrnlHlprFwpsLayerIDToString(pClassifyValues->layerId),
              pFilter->filterId,
              pClassifyOut->actionType,
              pClassifyOut->rights,
              (pClassifyOut->flags & FWPS_CLASSIFY_OUT_FLAG_ABSORB) ? "TRUE" : "FALSE");

   return;
}

#endif // (NTDDI_VERSION >= NTDDI_WIN7)
