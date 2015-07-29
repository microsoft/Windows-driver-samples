////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2014 Microsoft Corporation.  All Rights Reserved.
//
//   Module Name:
//      Framework_RPCServerInterface.cpp
//
//   Abstract:
//      This module contains functions which implement the RPC server interface.
//
//   Author:
//      Dusty Harper      (DHarper)
//
//   Naming Convention:
//
//      <Scope><Object><Action><Modifier>
//  
//      i.e.
//
//       <Scope>
//          {
//                               - Function is likely visible to other modules.
//          }
//       <Object>
//          {
//            RPCServerInterface - Function pertains to the RPC Server Interface.
//          }
//       <Action>
//          {
//            Initialize         - Function prepares environment for use.
//            SecurityCallback   - Function enforces RPC security measures.
//            Terminate          - Function cleans up the environment.
//          }
//       <Modifier>
//          {
//          }
//
//   Private Functions:
//
//   Public Functions:
//      RPCClientInterfaceInitialize(),
//      RPCClientInterfaceSecurityCallback(),
//      RPCClientInterfaceTerminate(),
//
//   Revision History:
//
//      [ Month ][Day] [Year] - [Revision]-[ Comments ]
//      May       01,   2010  -     1.0   -  Creation
//      December  13,   2013  -     1.1   -  Enhance function declaration for IntelliSense
//
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "Framework_WFPSamplerService.h" /// .
#include "WFPSamplerRPC_s.c"             /// $(OBJ_PATH)\..\idl\$(O)

/// Module's Local Structures

typedef struct RPC_DATA_
{
   RPC_IF_HANDLE       rpcInterfaceHandle;
   RPC_BINDING_VECTOR* pBindingVector;
   UINT32              protocolSequence;
   RPC_WSTR            pProtocolSequence;
   PWSTR               pEndpoint;
   UINT32              interfaceFlags;
   BOOLEAN             isInterfaceRegistered;
   BOOLEAN             isEndpointRegistered;
}RPC_DATA, *PRPC_DATA;

///

/// Module's Local Enumerations

typedef enum RPC_INITIALIZATION_TASKS_
{
   RPC_SELECT_PROTOCOL_SEQUENCE   = 0,
   RPC_REGISTER_INTERFACE         = 1,
   RPC_CREATE_BINDING_INFORMATION = 2,
   RPC_REGISTER_ENDPOINT          = 3,
   RPC_INITIALIZATION_TASKS_MAX
}RPC_INITIALIZATION_TASKS;

typedef enum RPC_TERMINATION_TASKS_
{
   RPC_UNREGISTER_ENDPOINT      = 0,
   RPC_FREE_BINDING_INFORMATION = 1,
   RPC_UNREGISTER_INTERFACE     = 2,
   RPC_TERMINATION_TASKS_MAX
}RPC_TERMINATION_TASKS;

///

/// Module's Local Variables

RPC_DATA* pRPCData = 0;

///

/// Functions implemented in Framework_WFPSamplerSvc.cpp

_Success_(return == NO_ERROR)
UINT32 ServiceStatusReportToSCM(_In_ UINT32 currentState,
                                _In_ UINT32 exitCode,
                                _In_ UINT32 waitHint);

VOID ServiceEventLogError(_In_ PCWSTR pFunctionName,
                          _In_ UINT32 status);

///

extern "C"
{
   /**
    @framework_function="MIDL_user_free"
    
      Purpose:  RPC stub routine to allocate memory.                                            <br>
                                                                                                <br>
      Notes:                                                                                    <br>
                                                                                                <br>
      MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/AA378651.aspx           <br>
                HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/AA378715.aspx           <br>
   */
   _Must_inspect_result_
   _Ret_maybenull_ _Post_writable_byte_size_(size)
   VOID* __RPC_USER MIDL_user_allocate(/* _In_ */ size_t size)
   {
      BYTE* pBuffer = 0;

      if(size)
      {
         HLPR_NEW_ARRAY(pBuffer,
                        BYTE,
                        size);
      }

      return pBuffer;
   }

   /**
    @framework_function="MIDL_user_free"
    
      Purpose:  RPC stub routine to free allocated memory.                                      <br>
                                                                                                <br>
      Notes:                                                                                    <br>
                                                                                                <br>
      MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/AA378651.aspx           <br>
                HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/AA378716.aspx           <br>
   */
   VOID __RPC_USER MIDL_user_free(/* _Inout_ */ _Pre_maybenull_ _Post_invalid_ VOID* pBuffer)
   {
      HLPR_DELETE_ARRAY(pBuffer);

      return;
   }
}

/**
 @framework_function="RPCInterfaceSecurityCallback"
 
   Purpose:  Callback routine to enforce security measures.                                     <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/AA378651.aspx              <br>
             HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/AA378625.aspx              <br>
             HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/AA378434.aspx              <br>
             HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/AA373553.aspx              <br>
*/
_Success_(return == RPC_S_OK)
RPC_STATUS CALLBACK RPCServerInterfaceSecurityCallback(_In_ RPC_IF_HANDLE interfaceHandle,
                                                       _In_ VOID* pContext)
{
   UNREFERENCED_PARAMETER(interfaceHandle);

   ASSERT(interfaceHandle);
   ASSERT(pContext);

   RPC_STATUS status = RPC_S_OK;

   RPC_CALL_ATTRIBUTES_V2 callAttributes = {0};

   callAttributes.Version = 2;
   callAttributes.Flags   = RPC_QUERY_CLIENT_PID;

   status = RpcServerInqCallAttributes((RPC_BINDING_HANDLE)pContext,         /// Binding handle used by the client when making the RPC call
                                       &callAttributes);                     /// Client's call attributes
   if(status != RPC_S_OK)
   {
      ServiceEventLogError(L"RPCServerInterfaceSecurityCallback : RpcServerinqCallAttributes()",
                           status);

      HLPR_BAIL;
   }

   if(callAttributes.IsClientLocal == rcclRemote ||                         /// make sure this is a local client
      callAttributes.ProtocolSequence != pRPCData->protocolSequence ||      /// make sure this is the correct protocol sequence (in our case Local RPC)
      !HlprGUIDsAreEqual(&(callAttributes.InterfaceUuid),
                         &WFPSAMPLER_INTERFACE) ||                          /// make sure this is the correct interface
      callAttributes.AuthenticationLevel < RPC_C_AUTHN_LEVEL_PKT_PRIVACY)   /// make sure this is highest security
   {
      status = RPC_S_ACCESS_DENIED;

      ServiceEventLogError(L"RPCServerInterfaceSecurityCallback()",
                           status);
   }

   HLPR_BAIL_LABEL:

   return status;
}

/**
 @framework_function="RPCServerInterfaceTerminate"
 
   Purpose:  Remove the RPC server interface by unregistering the endpoint and interface and 
             freeing the bindings.                                                              <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/AA378651.aspx              <br>
             HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/AA375651.aspx              <br>
             HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/AA375615.aspx              <br>
             HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/AA378445.aspx              <br>
*/
_Success_(return == RPC_S_OK)
RPC_STATUS RPCServerInterfaceTerminate()
{
   RPC_STATUS   status     = RPC_S_OK;
   const UINT32 ITERATIONS = RPC_TERMINATION_TASKS_MAX;

   for(UINT32 index = 0;
       index < ITERATIONS;
       index++)
   {
      switch(index)
      {
         case RPC_UNREGISTER_ENDPOINT:
         {
            if(pRPCData->isEndpointRegistered)
            {
               status = RpcEpUnregister(pRPCData->rpcInterfaceHandle,           /// Handle genereated by MIDL compiler for Interface
                                        pRPCData->pBindingVector,               /// Vector of binding Handles
                                        0);                                     /// no object UUIDs
               if(status != RPC_S_OK)
                  ServiceEventLogError(L"RPCServerInterfaceTerminate : RpcEpUnregister()",
                                       status);
            }

            break;
         }
         case RPC_FREE_BINDING_INFORMATION:
         {
            if(pRPCData->pBindingVector)
            {
               status = RpcBindingVectorFree(&(pRPCData->pBindingVector));      /// Vector of binding Handles to free. 0 on success
               if(status != RPC_S_OK)
                  ServiceEventLogError(L"RPCServerInterfaceTerminate : RpcBindingVectorFree()",
                                       status);
            }

            break;
         }
         case RPC_UNREGISTER_INTERFACE:
         {
            if(pRPCData->isInterfaceRegistered)
            {
               status = RpcServerUnregisterIf(pRPCData->rpcInterfaceHandle,     /// Handle genereated by MIDL compiler for Interface
                                              (UUID*)&WFPSAMPLER_INTERFACE,     /// GUID for our Interface
                                              TRUE);                            /// Wait for any outstanding calls to finish
               if(status != RPC_S_OK)
                  ServiceEventLogError(L"RPCServerInterfaceTerminate : RpcServerUnregisterIf()",
                                       status);
            }

            break;
         }
         default:
         {
            status = RPC_S_INVALID_ARG;

            break;
         }
      }

      ServiceStatusReportToSCM(SERVICE_STOP_PENDING,
                               status,
                               0);
   }

   HLPR_DELETE(pRPCData);

   return status;
}

/**
 @framework_function="RPCServerInterfaceInitialize"
 
   Purpose:  Initialize the RPC server interface by creating the RPC bindings and registering 
             the interface and endpoint.                                                        <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/AA378651.aspx              <br>
             HTTP://MSDN.Microsoft.com/En-US/LibraryWindows/Desktop//AA378452.aspx              <br>
             HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/AA378441.aspx              <br>
             HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/AA378433.aspx              <br>
             HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/AA375637.aspx              <br>
*/
_Success_(return == RPC_S_OK)
RPC_STATUS RPCServerInterfaceInitialize()
{
   RPC_STATUS   status     = RPC_S_OK;
   const UINT32 ITERATIONS = RPC_INITIALIZATION_TASKS_MAX;

   HLPR_NEW(pRPCData,
            RPC_DATA);
   HLPR_BAIL_ON_ALLOC_FAILURE(pRPCData,
                              status);

   pRPCData->rpcInterfaceHandle = IWFPSampler_v1_0_s_ifspec;                    /// MIDL generated Server Interface Handle
   pRPCData->protocolSequence   = RPC_PROTSEQ_LRPC;                             /// Use Local RPC
   pRPCData->pProtocolSequence  = (RPC_WSTR)g_pRPCProtocolSequence;             /// Use Local RPC
   pRPCData->pEndpoint          = (PWSTR)g_pEndpoint;                           /// String representation of our endpoint
   pRPCData->interfaceFlags     = RPC_IF_ALLOW_LOCAL_ONLY |                     /// Reject all non-local RPC calls
                                  RPC_IF_AUTOLISTEN |                           /// Interface will automatically listen
                                  RPC_IF_ALLOW_SECURE_ONLY;                     /// Only allow authorized clients (valid account and credentials)

   for(UINT32 index = 0;
       index < ITERATIONS;
       index++)
   {
      switch(index)
      {
         case RPC_SELECT_PROTOCOL_SEQUENCE:
         {
            status = RpcServerUseProtseqEp(pRPCData->pProtocolSequence,         /// protocolSequence to register.  In our case this is Local RPC
                                           RPC_C_PROTSEQ_MAX_REQS_DEFAULT,      /// Backlog queue length.  In our case this is ignored.
                                           pRPCData->pEndpoint,                 /// Use our endpoint information
                                           0);                                  /// We will implement security via a callback routine
            if(status != RPC_S_OK)
            {
               ServiceEventLogError(L"RPCServerInterfaceInitialize : RpcServerUseProtseqEp()",
                                    status);

               HLPR_BAIL;
            }

            break;
         }
         case RPC_REGISTER_INTERFACE:
         {
            status = RpcServerRegisterIfEx(pRPCData->rpcInterfaceHandle,        /// Handle genereated by MIDL compiler for Interface
                                           0,                                   /// Use default entry point type
                                           0,                                   /// Use default entry point vector
                                           pRPCData->interfaceFlags,            /// Registration flags
                                           RPC_C_LISTEN_MAX_CALLS_DEFAULT,      /// Max number of calls that can be accepted concurrently
                                           RPCServerInterfaceSecurityCallback); /// Callback routine to implement our security
            if(status != RPC_S_OK)
            {
               ServiceEventLogError(L"RPCServerInterfaceInitialize : RpcServerRegisterIf()",
                                    status);

               HLPR_BAIL;
            }

            break;
         }
         case RPC_CREATE_BINDING_INFORMATION:
         {
            status = RpcServerInqBindings(&(pRPCData->pBindingVector));         /// Get vector of binding handles
            if(status != RPC_S_OK)
            {
               ServiceEventLogError(L"RPCServerInterfaceInitialize : RpcServerInqBindings()",
                                    status);

               HLPR_BAIL;
            }

            break;
         }
         case RPC_REGISTER_ENDPOINT:
         {
            status = RpcEpRegister(pRPCData->rpcInterfaceHandle,                /// Handle genereated by MIDL compiler for Interface
                                   pRPCData->pBindingVector,                    /// Vector of Binding Handles
                                   0,                                           /// No object UUIDs
                                   L"WFPSampler Endpoint");                     /// Annotation / Comment
            if(status != RPC_S_OK)
            {
               ServiceEventLogError(L"RPCServerInterfaceInitialize : RpcEpRegister()",
                                    status);

               HLPR_BAIL;
            }

            pRPCData->isEndpointRegistered = TRUE;

            break;
         }
         default:
         {
            status = RPC_S_INVALID_ARG;

            break;
         }
      }

      ServiceStatusReportToSCM(SERVICE_START_PENDING,
                               status,
                               5000);
   }

   HLPR_BAIL_LABEL:

   if(status != RPC_S_OK)
      ServiceStatusReportToSCM(SERVICE_STOPPED,
                               status,
                               0);

   return status;
}
