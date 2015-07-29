////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2014 Microsoft Corporation.  All Rights Reserved.
//
//   Module Name:
//      CompletionFunctions_FastPacketInjectionCallouts.cpp
//
//   Abstract:
//      This module contains WFP Completion functions for packets injected back into the data path 
//         using the clone / block / inject method.
//
//   Naming Convention:
//
//      <Module><Scenario>
//  
//      i.e.
//       CompleteFastPacketInjection
//
//       <Module>
//          Complete             - Function is an FWPS_INJECT_COMPLETE function.
//       <Scenario>
//          FastPacketInjection  - Function demonstrates the clone / block / inject model in the 
//                                    fastest form available (inline, no validation, etc.).
//
//   Private Functions:
//
//   Public Functions:
//      CompleteFastPacketInjection(),
//
//   Author:
//      Dusty Harper      (DHarper)
//
//   Revision History:
//
//      [ Month ][Day] [Year] - [Revision]-[ Comments ]
//      May       01,   2010  -     1.0   -  Creation
//      December  13,   2013  -     1.1   -  Enhance function declaration for IntelliSense
//
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "Framework_WFPSamplerCalloutDriver.h" /// .

/**
 @completion_function="CompleteFastPacketInjection"
 
   Purpose:                                                                                     <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-Us/Library/Windows/Hardware/FF545018.aspx             <br>
             HTTP://MSDN.Microsoft.com/En-Us/Library/Windows/Hardware/FF551170.aspx             <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
VOID NTAPI CompleteFastPacketInjection(_In_ VOID* pContext,
                                       _Inout_ NET_BUFFER_LIST* pNetBufferList,
                                       _In_ BOOLEAN dispatchLevel)
{
   UNREFERENCED_PARAMETER(dispatchLevel);

   NT_ASSERT(NT_SUCCESS(pNetBufferList->Status));

   FwpsFreeCloneNetBufferList(pNetBufferList,
                              0);

   if(pContext)
   {
      FWPS_TRANSPORT_SEND_PARAMS* pSendParams = (FWPS_TRANSPORT_SEND_PARAMS*)pContext;

      HLPR_DELETE_ARRAY(pSendParams->remoteAddress,
                        WFPSAMPLER_CALLOUT_DRIVER_TAG);
      
      HLPR_DELETE(pSendParams,
                  WFPSAMPLER_CALLOUT_DRIVER_TAG);
   }

   return;
}
