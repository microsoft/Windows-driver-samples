////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2014 Microsoft Corporation.  All Rights Reserved.
//
//   Module Name:
//      SubscriptionFunctions_BFEState.cpp
//
//   Abstract:
//      This module contains WFP callback functions for changes in the BFE state.
//
//   Naming Convention:
//
//      <Module><Scenario>
//  
//      i.e.
//
//       SubscriptionBFEStateChangeCallback
//
//       <Module>
//          Subscription           -   Function pertains to a subscription.
//       <Scenario>
//          BFEStateChangeCallback -   Function demonstates use of the BFE service state change 
//                                        callback.
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
#include "SubscriptionFunctions_BFEState.tmh"  /// $(OBJ_PATH)\$(O)\ 

/**
 @notify_function="SubscriptionBFEStateChangeCallback"
 
   Purpose:  Callback, invoked on BFE service state change, which will get or release a handle 
             to the engine.                                                                     <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF550062.aspx             <br>
*/
_IRQL_requires_(PASSIVE_LEVEL)
_IRQL_requires_same_
VOID CALLBACK SubscriptionBFEStateChangeCallback(_Inout_ VOID* pContext,
                                                 _In_ FWPM_SERVICE_STATE bfeState)
{
#if DBG

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> SubscriptionBFEStateChangeCallback()\n");

#endif /// DBG
   
   NT_ASSERT(pContext);

   NTSTATUS                status      = STATUS_SUCCESS;
   WFPSAMPLER_DEVICE_DATA* pDeviceData = (WFPSAMPLER_DEVICE_DATA*)pContext;

   switch(bfeState)
   {
      case FWPM_SERVICE_RUNNING:
      {
         if(pDeviceData->pEngineHandle == 0)
         {
            status = KrnlHlprFwpmSessionCreateEngineHandle(&(pDeviceData->pEngineHandle));
            HLPR_BAIL_ON_FAILURE(status);
         }

         break;
      }
      case FWPM_SERVICE_STOP_PENDING:
      {
         KrnlHlprFwpmSessionDestroyEngineHandle(&(pDeviceData->pEngineHandle));

         break;
      }
   }

   HLPR_BAIL_LABEL:

#if DBG

      DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                 DPFLTR_INFO_LEVEL,
                 " <--- SubscriptionBFEStateChangeCallback() [status: %#x]\n",
                 status);

#endif /// DBG

   return;
}
