////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2012 Microsoft Corporation.  All Rights Reserved.
//
//   Module Name:
//      Framework_RPCClientInterface.cpp
//
//   Abstract:
//      This module contains functions which implement the RPC client interface.
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
//            Prv                - Function is private to this module.
//          }
//       <Object>
//          {
//            RPC                - Function pertains to Remote Procedule Call.
//            RPCClientInterface - Function pertains to the RPC Client Interface.
//          }
//       <Action>
//          {
//            Initialize         - Function prepares environment for use.
//            Terminate          - Function cleans up the environment.
//            Troubleshoot       - Function attempts to provide extra information to diagnose cause 
//                                    of failure.
//          }
//       <Modifier>
//          {
//            Error              - Function acts on errors.
//          }
//
//   Private Functions:
//      PrvRPCTroubleshootError(),
//
//   Public Functions:
//      RPCClientInterfaceInitialize(),
//      RPCClientInterfaceTerminate(),
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

#include "Framework_WFPSampler.h" /// .
#include "WFPSamplerRPC_c.c"      /// $(OBJ_PATH)\..\idl\$(O)

/// Module's Local Structures

typedef struct RPC_DATA_
{
   RPC_IF_HANDLE      rpcClientInterfaceHandle;
   RPC_BINDING_HANDLE bindingHandle;
   UINT32             protocolSequence;
   RPC_WSTR           pProtocolSequence;
   RPC_WSTR           pEndpoint;
   BOOLEAN            isBound;
}RPC_DATA, *PRPC_DATA;

/// Module's Local Variables

RPC_DATA* pRPCData = 0;

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
 @private_function="PrvRPCTroubleshootError"
 
   Purpose:  Function to retrieve extended error information about RPC's inner workings.        <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/AA378651.aspx              <br>
             HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/AA374351.aspx              <br>
             HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/AA375686.aspx              <br>
             HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/AA375668.aspx              <br>
             HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/AA375664.aspx              <br>
*/
VOID PrvRPCTroubleshootError()
{
   RPC_STATUS            status     = RPC_S_OK;
   RPC_ERROR_ENUM_HANDLE enumHandle = {0};

   status = RpcErrorStartEnumeration(&enumHandle);
   if(status != RPC_S_OK)
   {
      if(status != RPC_S_ENTRY_NOT_FOUND)
         HlprLogError(L"PrvRPCTroubleshootError : RpcErrorStartEnumeration() [status: %#x]",
                      status);

      HLPR_BAIL;
   }

   for(;
       status == RPC_S_OK;
       )
   {
      RPC_EXTENDED_ERROR_INFO errorInfo = {0};

      errorInfo.Version            = RPC_EEINFO_VERSION;
      errorInfo.NumberOfParameters = MaxNumberOfEEInfoParams;

      status = RpcErrorGetNextRecord(&enumHandle,
                                     TRUE,
                                     &errorInfo);
      if(status == RPC_S_ENTRY_NOT_FOUND)
      {
         HlprLogInfo(L"PrvRPCTroubleshootError : RpcErrorGetNextRecord() [status: %#x]",
                     status);

         HLPR_BAIL;
      }
      else if(status != RPC_S_OK)
      {
         HlprLogError(L"PrvRPCTroubleshootError : RpcErrorGetNextRecord() [status: %#x]",
                      status);

         HLPR_BAIL;
      }
      else
      {
         if(errorInfo.ComputerName)
         {
            HlprLogInfo(L"   [ComputerName: %S]",
                        errorInfo.ComputerName);

            HeapFree(GetProcessHeap(),
                     0,
                     errorInfo.ComputerName);
         }

         HlprLogInfo(L"   [ProcessID: %d]",
                     errorInfo.ProcessID);

         HlprLogInfo(L"   [SystemTime: %02d/%02d/%04d %02d:%02d:%02d:%03d]",
                     errorInfo.u.SystemTime.wMonth,
                     errorInfo.u.SystemTime.wDay,
                     errorInfo.u.SystemTime.wYear,
                     errorInfo.u.SystemTime.wHour,
                     errorInfo.u.SystemTime.wMinute,
                     errorInfo.u.SystemTime.wSecond,
                     errorInfo.u.SystemTime.wMilliseconds);

         HlprLogInfo(L"   [GeneratingComponent:%d]",
                     errorInfo.GeneratingComponent);

         HlprLogInfo(L"   [Status: %#x]",
                     errorInfo.Status);

         HlprLogInfo(L"   [DetectionLocation: %d]",
                     errorInfo.DetectionLocation);

         HlprLogInfo(L"   [Flags: %#x]",
                     errorInfo.Flags);

         HlprLogInfo(L"   [NumberOfParameters: %d]", 
                     errorInfo.NumberOfParameters);

         for(UINT32 i = 0;
             i < (UINT32)errorInfo.NumberOfParameters;
             i++)
         {
            PWSTR pNewLine = L"";

            if(i == ((UINT32)errorInfo.NumberOfParameters - 1))
               pNewLine = L"\n";

            switch(errorInfo.Parameters[i].ParameterType)
            {
               case eeptAnsiString:
               {
                  HlprLogInfo(L"   [AnsiString: %s]%s",
                              errorInfo.Parameters[i].u.AnsiString,
                              pNewLine);

                  HeapFree(GetProcessHeap(),
                           0, 
                           errorInfo.Parameters[i].u.AnsiString);

                  break;
               }
               case eeptUnicodeString:
               {
                  HlprLogInfo(L"   [UnicodeString: %S]%s",
                              errorInfo.Parameters[i].u.UnicodeString,
                              pNewLine);

                  HeapFree(GetProcessHeap(),
                           0, 
                           errorInfo.Parameters[i].u.UnicodeString);

                  break;
               }
               case eeptLongVal:
               {
                  HlprLogInfo(L"   [LongVal: %#x]%s",
                              errorInfo.Parameters[i].u.LVal,
                              pNewLine);

                  break;
               }
               case eeptShortVal:
               {
                  HlprLogInfo(L"   [ShortVal: %#x]%s",
                              errorInfo.Parameters[i].u.SVal,
                              pNewLine);

                  break;
               }
               case eeptPointerVal:
               {
                  HlprLogInfo(L"   [PointerVal: %#p]%s",
                              errorInfo.Parameters[i].u.PVal,
                              pNewLine);

                  break;
               }
               case eeptNone:
               {
                  HlprLogInfo(L"   [Truncated]%s",
                              pNewLine);

                  break;
               }
               default:
                  HlprLogInfo(L"   [ParameterType Invalid: %d]%s", 
                              errorInfo.Parameters[i].ParameterType,
                              pNewLine);
            }
         }
      }
   }

   HLPR_BAIL_LABEL:

   RpcErrorEndEnumeration(&enumHandle);

   return;
}

/**
 @framework_function="RPCClientInterfaceTerminate"
 
   Purpose:  Teardown the RPC client interface by unbinding and freeing the handles.            <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/AA378651.aspx              <br>
             HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/AA375613.aspx              <br>
             HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/AA375588.aspx              <br>
*/
UINT32 RPCClientInterfaceTerminate()
{
   RPC_STATUS status = RPC_S_OK;

   if(pRPCData &&
      pRPCData->bindingHandle)
   {
      if(pRPCData->isBound)
      {
         status = RpcBindingUnbind(pRPCData->bindingHandle);
         if(status != RPC_S_OK)
            HlprLogError(L"RPCClientInterfaceTerminate : RpcBindingUnbind() [status: %#x]",
                         status);
         else
            pRPCData->isBound = FALSE;
      }

      status = RpcBindingFree(&(pRPCData->bindingHandle));
      if(status != RPC_S_OK)
         HlprLogError(L"RPCClientInterfaceTerminate : RpcBindingFree() [status: %#x]",
                      status);
   }

   HLPR_DELETE(pRPCData);

   return status;
}

/**
 @framework_function="RPCClientInterfaceInitialize"
 
   Purpose:  Initialize the RPC client interface by creating a fast binding handle.             <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/AA378651.aspx              <br>
             HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/AA375587.aspx              <br>
             HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/AA375583.aspx              <br>
*/
UINT32 RPCClientInterfaceInitialize()
{
   RPC_STATUS                     status                = RPC_S_OK;
   SID*                           pLocalSystemSID       = 0;
   SIZE_T                         sidSize               = 0;
   RPC_SECURITY_QOS_V4            securityQoS           = {0};
   RPC_BINDING_HANDLE_TEMPLATE_V1 bindingHandleTemplate = {0};
   RPC_BINDING_HANDLE_SECURITY_V1 bindingHandleSecurity = {0};
   RPC_BINDING_HANDLE_OPTIONS_V1  bindingHandleOptions  = {0};

   HLPR_NEW(pRPCData,
            RPC_DATA);
   HLPR_BAIL_ON_ALLOC_FAILURE(pRPCData,
                              status);

   status = HlprSIDCreate(&pLocalSystemSID,
                          &sidSize,
                          0,
                          WinLocalSystemSid);
   HLPR_BAIL_ON_FAILURE(status);

   wfpSamplerBindingHandle = 0;

   pRPCData->rpcClientInterfaceHandle = IWFPSampler_v1_0_c_ifspec;      /// MIDL generated Client Interface Handle
   pRPCData->protocolSequence         = RPC_PROTSEQ_LRPC;               /// Use Local RPC

   securityQoS.Version           = 4;                                   /// Use RPC_SECURITY_QOS_V4 structure
   securityQoS.Capabilities      = RPC_C_QOS_CAPABILITIES_MUTUAL_AUTH;  /// Request mutual authentication from the security provider
   securityQoS.IdentityTracking  = RPC_C_QOS_IDENTITY_STATIC;           /// Security context is created only once
   securityQoS.ImpersonationType = RPC_C_IMP_LEVEL_IDENTIFY;            /// Allow server to get client's identity and allow it's impersonation
   securityQoS.Sid               = pLocalSystemSID;                     /// Security identifier used by Local RPC
   securityQoS.EffectiveOnly     = TRUE;                                /// only see enabled privileges

   bindingHandleTemplate.Version          = 1;                          /// Use BINDING_HANDLE_TEMPLATE_V1 structure
   bindingHandleTemplate.ProtocolSequence = pRPCData->protocolSequence; /// Use Local RPC
   bindingHandleTemplate.StringEndpoint   = (PWSTR)g_pEndpoint;         /// String representation of our endpoint

   bindingHandleSecurity.Version     = 1;                               /// Use BINDING_HANDLE_SECURITY_V1 structure
   bindingHandleSecurity.AuthnLevel  = RPC_C_AUTHN_LEVEL_PKT_PRIVACY;   /// Authentication level which causes Local RPC to use a secure channel
   bindingHandleSecurity.AuthnSvc    = RPC_C_AUTHN_WINNT;               /// Autherntication service to use
   bindingHandleSecurity.SecurityQos = (RPC_SECURITY_QOS*)&securityQoS; /// Security Qos Settings to Use

   bindingHandleOptions.Version    = 1;                                 /// Use BINDING_HANDLE_OPTIONS_V1 structure
   bindingHandleOptions.Flags      = RPC_BHO_NONCAUSAL;                 /// Execute calls in any order
   bindingHandleOptions.ComTimeout = RPC_C_BINDING_DEFAULT_TIMEOUT;     /// Use default communication timeout value

   status = RpcBindingCreate(&bindingHandleTemplate,                    /// Core structure of the binding handle
                             &bindingHandleSecurity,                    /// Security options for the binding handle
                             &bindingHandleOptions,                     /// Additional options to set on the binding handle
                             &wfpSamplerBindingHandle);                 /// Created binding handle
   if(status != RPC_S_OK)
   {
      HlprLogError(L"RPCClientInterfaceInitialize : RpcBindingCreate() [status: %#x]",
                   status);

      HLPR_BAIL;
   }

   pRPCData->bindingHandle = wfpSamplerBindingHandle;

   status = RpcBindingBind(0,                                           /// These RPC calls will be synchronous
                           pRPCData->bindingHandle,                     /// Binding handle that will be used to make the RPC call
                           pRPCData->rpcClientInterfaceHandle);         /// Interface handle that will be used to make the RPC call
   if(status != RPC_S_OK)
   {
      HlprLogError(L"RPCClientInterfaceInitialize : RpcBindingBind() [status: %#x]",
                   status);

      HLPR_BAIL;
   }

   HLPR_BAIL_LABEL:

   if(status != RPC_S_OK)
   {
      HlprLogError(L"[status: %#x]",
                   status);

      PrvRPCTroubleshootError();

      RPCClientInterfaceTerminate();
   }

   if(pLocalSystemSID)
      HlprSIDDestroy(&pLocalSystemSID);

   return status;
}
