////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2014 Microsoft Corporation.  All Rights Reserved.
//
//   Module Name:
//      CompletionFunctions_FastStreamInjectionCallouts.cpp
//
//   Abstract:
//      This module contains WFP Completion functions for data injected back into the stream using 
//         the clone / block / inject method.
//
//   Naming Convention:
//
//      <Module><Scenario>
//  
//      i.e.
//       CompleteFastStreamInjection
//
//       <Module>
//          Complete             - Function is an FWPS_INJECT_COMPLETE function.
//       <Scenario>
//          FastStreamInjection  - Function demonstrates the clone / block / inject model in the 
//                                    fastest form available (inline, no validation, etc.).
//
//   Private Functions:
//
//   Public Functions:
//      CompleteFastStreamInjection(),
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
 @completion_function="CompleteFastStreamInjection"
 
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
VOID NTAPI CompleteFastStreamInjection(_In_ VOID* pContext,
                                       _Inout_ NET_BUFFER_LIST* pNetBufferList,
                                       _In_ BOOLEAN dispatchLevel)
{
   UNREFERENCED_PARAMETER(pContext);
   UNREFERENCED_PARAMETER(dispatchLevel);

   NT_ASSERT(NT_SUCCESS(pNetBufferList->Status));

   FwpsFreeCloneNetBufferList(pNetBufferList,
                              0);

   return;
}
