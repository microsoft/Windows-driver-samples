////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2012 Microsoft Corporation.  All Rights Reserved.
//
//   Module Name:
//      Framework_WindowsFirewall.cpp
//
//   Abstract:
//      This module contains functions for interoperating with the Microsoft Windows Firewall
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

#include "Framework_WFPSamplerService.h"

/// Module's Global variables
namespace
{
   IUnknown*    pFirewallRegistrationHandle = 0;
   THREAD_DATA* pWFNotifyThread             = 0;
   SC_HANDLE    scmHandle                   = 0;
   SC_HANDLE    mpsSvcHandle                = 0;
};

///

/// Functions implemented in Framework_WFPSamplerSvc.cpp

_Success_(return == NO_ERROR)
UINT32 ServiceStatusReportToSCM(_In_ UINT32 currentState,
                                _In_ UINT32 exitCode,
                                _In_ UINT32 waitHint);

VOID ServiceEventLogError(_In_ PCWSTR pFunctionName,
                          _In_ UINT32 errorCode);

///

/// Forward Declarations

_Success_(return == NO_ERROR)
UINT32 WindowsFirewallAcquireFirewallCategory();

///

/**
 @framework_function="WindowsFirewallNotification"
 
   Purpose:  Notification used to take firewall categories from Windows Firewall when MPSSvc 
             transitions to a running                                                           <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/MS685947.aspx              <br>
*/
VOID CALLBACK WindowsFirewallNotification(_In_ VOID* pContext)
{
   ASSERT(pContext);

   SERVICE_NOTIFY* pServiceNotify = (SERVICE_NOTIFY*)pContext;
   THREAD_DATA*    pThreadData    = (THREAD_DATA*)pServiceNotify->pContext;

   if(pServiceNotify->dwNotificationStatus == NO_ERROR)
   {
      if(pServiceNotify->ServiceStatus.dwCurrentState == SERVICE_RUNNING)
      {
         WindowsFirewallAcquireFirewallCategory();

         HlprThreadStop(&pThreadData);
      }
   }
   else if(pServiceNotify->dwNotificationStatus == ERROR_SERVICE_MARKED_FOR_DELETE)
      HlprThreadStop(&pThreadData);

   return;
}

/**
 @framework_function="WindowsFirewallNotifyThreadStartRoutine"

   Purpose:  New thread which waits for a notification that MPSSvc has started.                 <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/MS686736.aspx              <br>
*/
_Success_(return == NO_ERROR)
DWORD WindowsFirewallNotifyThreadStartRoutine(_In_ LPVOID lpThreadParameter)
{
   ASSERT(lpThreadParameter);

   UINT32         status      = NO_ERROR;
   THREAD_DATA*   pThreadData = (THREAD_DATA*)lpThreadParameter;
   SERVICE_NOTIFY svcNotify   = {0};

   HlprEventSet(pThreadData->threadStartEvent);

   svcNotify.dwVersion         = SERVICE_NOTIFY_STATUS_CHANGE;
   svcNotify.pfnNotifyCallback = WindowsFirewallNotification;
   svcNotify.pContext          = pWFNotifyThread;

   status = HlprServiceNotificationStateChangeRegister(L"MPSSvc",
                                                       &svcNotify,
                                                       SERVICE_NOTIFY_RUNNING,
                                                       &scmHandle,
                                                       &mpsSvcHandle);
   HLPR_BAIL_ON_FAILURE(status);

   status = WaitForSingleObjectEx(mpsSvcHandle,
                                  INFINITE,
                                  TRUE);
   if(status != WAIT_IO_COMPLETION)
   {
      if(status == WAIT_FAILED)
         status = GetLastError();

      HlprLogError(L"WindowsFirewallNotifyThreadStartRoutine : WaitForSingleObjectEx() [status: %#x]",
                   status);
   }

   HLPR_BAIL_LABEL:

   HLPR_CLOSE_SERVICE_HANDLE(mpsSvcHandle);

   HLPR_CLOSE_SERVICE_HANDLE(scmHandle);

   return status;
}

/**
 @framework_function="WindowsFirewallReleaseFirewallCategory"
 
   Purpose:  Relinquish ownership of the Firewall Category back to Windows Firewall and 
             unregister as a Firewall product.                                                  <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/DD607247.aspx              <br>
             HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/DD607255.aspx              <br>
             HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/DD607259.aspx              <br>
*/
VOID WindowsFirewallReleaseFirewallCategory()
{
   if(pFirewallRegistrationHandle)
   {
      pFirewallRegistrationHandle->Release();

      pFirewallRegistrationHandle = 0;
   }

   HLPR_CLOSE_SERVICE_HANDLE(mpsSvcHandle);

   HLPR_CLOSE_SERVICE_HANDLE(scmHandle);

   HlprThreadStop(&pWFNotifyThread);

   return;
}

/**
 @framework_function="WindowsFirewallAcquireFirewallCategory"
 
   Purpose:  Take ownership from Windows Firewall of the Firewall Category and register as a 
             Firewall product so as to be displayed in the Windows Firewall User Interface.     <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/DD607247.aspx              <br>
             HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/DD607255.aspx              <br>
             HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/DD607259.aspx              <br>
*/
_Success_(return == NO_ERROR)
UINT32 WindowsFirewallAcquireFirewallCategory()
{
   UINT32 status = NO_ERROR;

   if(HlprServiceQueryState(L"MPSSvc") == SERVICE_RUNNING)
   {
      BSTR            pProductName      = SysAllocString(g_pServiceName);
      INetFwProduct*  pNetFwProduct     = 0;
      INetFwProducts* pNetFwProducts    = 0;
      SAFEARRAY*      pFirewallCategory = 0;
      SAFEARRAY*      pBootTimeCategory = 0;
      SAFEARRAYBOUND  boundary          = {0};
      VARIANT         firewallCategory  = {0};
      VARIANT         boottimeCategory  = {0};

      if(pProductName == 0)
      {
         status = ERROR_GEN_FAILURE;

         ServiceEventLogError(L"WindowsFirewallAcquireFirewallCategory : SysAllocString()",
                              status);

         HLPR_BAIL;
      }

      boundary.lLbound   = 0;
      boundary.cElements = 1;

      pBootTimeCategory = SafeArrayCreate(VT_VARIANT,
                                          1,
                                          &boundary);
      if(pBootTimeCategory == 0)
      {
         status = ERROR_GEN_FAILURE;

         ServiceEventLogError(L"WindowsFirewallAcquireFirewallCategory : SafeArrayCreate()",
                              status);

         HLPR_BAIL;
      }

      V_VT((VARIANT*)pBootTimeCategory->pvData) = VT_I4;
      V_I4((VARIANT*)pBootTimeCategory->pvData) = NET_FW_RULE_CATEGORY_BOOT;

      VariantInit(&firewallCategory);

      V_VT(&boottimeCategory)    = VT_ARRAY | VT_VARIANT;
      V_ARRAY(&boottimeCategory) = pBootTimeCategory;

      pFirewallCategory = SafeArrayCreate(VT_VARIANT,
                                          1,
                                          &boundary);
      if(pFirewallCategory == 0)
      {
         status = ERROR_GEN_FAILURE;

         ServiceEventLogError(L"WindowsFirewallAcquireFirewallCategory : SafeArrayCreate()",
                              status);

         HLPR_BAIL;
      }

      V_VT((VARIANT*)pFirewallCategory->pvData) = VT_I4;
      V_I4((VARIANT*)pFirewallCategory->pvData) = NET_FW_RULE_CATEGORY_FIREWALL;

      VariantInit(&firewallCategory);

      V_VT(&firewallCategory)    = VT_ARRAY | VT_VARIANT;
      V_ARRAY(&firewallCategory) = pFirewallCategory;

      /// Initialize the COM library
      if(!g_isCOMInitialized)
      {
         status = CoInitializeEx(0,
                                 COINIT_MULTITHREADED);
         if(FAILED(status))
         {
            ServiceEventLogError(L"WindowsFirewallAcquireFirewallCategory : CoInitializeEx()",
                                 status);

            HLPR_BAIL;
         }

         g_isCOMInitialized = TRUE;
      }

      /// Create an instance of the NetFwProduct class
      status = CoCreateInstance(__uuidof(NetFwProduct),
                                0,
                                CLSCTX_INPROC_SERVER,
                                __uuidof(INetFwProduct),
                                (LPVOID*)&pNetFwProduct);
      if(FAILED(status))
      {
         ServiceEventLogError(L"WindowsFirewallAcquireFirewallCategory : CoCreateInstance()",
                              status);

         HLPR_BAIL;
      }

      /// Set the DisplayName
      status = pNetFwProduct->put_DisplayName(pProductName);
      if(FAILED(status))
      {
         ServiceEventLogError(L"WindowsFirewallAcquireFirewallCategory : INetFwProduct::put_DisplayName()",
                              status);

         HLPR_BAIL;
      }

      /// Take the category
      status = pNetFwProduct->put_RuleCategories(boottimeCategory);
      if(FAILED(status))
      {
         ServiceEventLogError(L"WindowsFirewallAcquireFirewallCategory : INetFwProduct::put_RuleCategories()",
                              status);

         HLPR_BAIL;
      }

      status = pNetFwProduct->put_RuleCategories(firewallCategory);
      if(FAILED(status))
      {
         ServiceEventLogError(L"WindowsFirewallAcquireFirewallCategory : INetFwProduct::put_RuleCategories()",
                              status);

         HLPR_BAIL;
      }

      /// Create an instance of the NetFwProducts class
      status = CoCreateInstance(__uuidof(NetFwProducts),
                                0,
                                CLSCTX_INPROC_SERVER,
                                __uuidof(INetFwProducts),
                                (LPVOID*)&pNetFwProducts);
      if(FAILED(status))
      {
         ServiceEventLogError(L"WindowsFirewallAcquireFirewallCategory : CoCreateInstance()",
                              status);

         HLPR_BAIL;
      }

      /// Register as a Firewall Product
      if(pFirewallRegistrationHandle == 0)
      {
         status = pNetFwProducts->Register(pNetFwProduct,
                                           &pFirewallRegistrationHandle);
         if(FAILED(status))
         {
            ServiceEventLogError(L"WindowsFirewallAcquireFirewallCategory : INetFwProducts::Register()",
                                 status);

            HLPR_BAIL;
         }
      }

      status = NO_ERROR;

      HLPR_BAIL_LABEL:

      if(FAILED(status))
         WindowsFirewallReleaseFirewallCategory();

      if(pNetFwProducts)
         pNetFwProducts->Release();

      if(pNetFwProduct)
         pNetFwProduct->Release();

      if(pProductName)
         SysFreeString(pProductName);
   }
   else
   {
      HLPR_NEW(pWFNotifyThread,
               THREAD_DATA);
      if(pWFNotifyThread)
      {
         pWFNotifyThread->threadStartRoutine = (LPTHREAD_START_ROUTINE)WindowsFirewallNotifyThreadStartRoutine;

         status = HlprThreadStart(pWFNotifyThread);
      }
      else
         status = ERROR_OUTOFMEMORY;
   }

   if(HlprServiceQueryState(L"WFPSampler") == SERVICE_START_PENDING)
      ServiceStatusReportToSCM(SERVICE_START_PENDING,
                               status,
                               2500);

   return status;
}
