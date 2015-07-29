////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2014 Microsoft Corporation.  All Rights Reserved.
//
//   Module Name:
//      Framework_WFPSamplerService.cpp
//
//   Abstract:
//      This module contains functions which form the entry point to the WFPSampler service 
//         (WFPSamplerService.exe).
//
//   Naming Convention:
//
//      <Object><Action><Modifier>
//  
//      i.e.
//
//       <Object>
//          {
//            ServiceEvent
//            ServiceStatus
//            ServiceControl
//            Service
//          }
//       <Action>
//          {
//            Log
//            Report
//            Handler
//            Terminate
//            Initialize
//            Uninstall
//            Install
//          }
//       <Modifier>
//          {
//            Error
//            ToSCM
//          }
//
//   Exported Functions:
//
//   Internal Functions:
//      ServiceEventLogError(),
//      ServiceStatusReportToSCM(),
//      ServiceControlHandler(),
//      ServiceTerminate(),
//      ServiceInitialize(),
//      ServiceUninstall(),
//      ServiceInstall(),
//      ServiceMain(),
//      wmain()
//
//   Author:
//      Dusty Harper      (DHarper)
//
//   Revision History:
//
//      [ Month ][Day] [Year] - [Revision]-[ Comments ]
//      May       01,   2010  -     1.0   -  Creation
//      December  13,   2013  -     1.1   -  Change sublayer weight
//
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "Framework_Include.h" /// .

/// Module's Local Structures

typedef struct SERVICE_DATA_
{
   SERVICE_STATUS        serviceStatus;
   SERVICE_STATUS_HANDLE serviceStatusHandle;
   HANDLE                serviceStopEventHandle;
}SERVICE_DATA, *PSERVICE_DATA;

///

BOOLEAN g_isCOMInitialized     = FALSE;
BOOLEAN g_takeFirewallCategory = TRUE;

/// Module's Local Variables

SERVICE_DATA* pServiceData = 0;

///

/**
 @private_function="PrvRegistryCreateEntries"
 
   Purpose: Adds registry keys that allow operation during Safe-Mode with Networking.           <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_Success_(return == NO_ERROR)
UINT32 PrvRegistryCreateEntries()
{
   UINT32 status    = NO_ERROR;
   PCWSTR pService  = L"Service";
   size_t valueSize = (wcslen(pService) + 1) * sizeof(WCHAR);

   /// This Registry Setting allows functionality during Safe-Mode with Networking
   status = HlprRegistrySetValue(HKEY_LOCAL_MACHINE,
                                 L"SYSTEM\\CurrentControlSet\\Control\\SafeBoot\\Network\\WFPSampler",
                                 REG_SZ,
                                 0,
                                 (BYTE*)pService,
                                 (UINT32)valueSize);
   HLPR_BAIL_ON_FAILURE(status);

   status = HlprRegistrySetValue(HKEY_LOCAL_MACHINE,
                                 L"SYSTEM\\CurrentControlSet\\Control\\SafeBoot\\Network\\WFPSamplerCallouts",
                                 REG_SZ,
                                 0,
                                 (BYTE*)pService,
                                 (UINT32)valueSize);

   HLPR_BAIL_LABEL:

   return status;
}

/**
 @private_function="PrvRegistryDestroyEntries"
 
   Purpose: Removes the registry keys that allow operation during Safe-Mode.                    <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_Success_(return == NO_ERROR)
UINT32 PrvRegistryDestroyEntries()
{
   UINT32 status = NO_ERROR;

   status = HlprRegistryDeleteKey(HKEY_LOCAL_MACHINE,
                                  L"SYSTEM\\CurrentControlSet\\Control\\SafeBoot\\Network\\WFPSampler");
   HLPR_BAIL_ON_FAILURE(status);

   status = HlprRegistryDeleteKey(HKEY_LOCAL_MACHINE,
                                  L"SYSTEM\\CurrentControlSet\\Control\\SafeBoot\\Network\\WFPSamplerCallouts");

   HLPR_BAIL_LABEL:

   return status;
}

/**
 @framework_function="ServiceEventLogError"
 
   Purpose:  Log errors in the eventlog.                                                        <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/AA363678.aspx                              <br>
             HTTP://MSDN.Microsoft.com/En-US/Library/AA363679.aspx                              <br>
             HTTP://MSDN.Microsoft.com/En-US/Library/AA363642.aspx                              <br>
*/
VOID ServiceEventLogError(_In_ PCWSTR pFunctionName,
                          _In_ UINT32 errorCode)
{
   UINT32 status          = NO_ERROR;
   HANDLE eventLogHandle  = 0;
   PCWSTR pFormatString   = L"%s [status: %#x]";
   const size_t MAX_CHARS = 256;
   WCHAR  pMessage[MAX_CHARS];
   PCWSTR ppStrings[2];

   ZeroMemory(pMessage,
              sizeof(WCHAR) * MAX_CHARS);

   ZeroMemory(ppStrings,
              sizeof(PCWSTR) * 2);

   status = StringCchPrintf(pMessage,
                            MAX_CHARS,
                            pFormatString,
                            pFunctionName,
                            errorCode);
   if(status != NO_ERROR)
   {
      wprintf(L"HlprLogError : StringCchVPrintf() [status: %#x]",
              status);

      HLPR_BAIL;
   }

#pragma warning(push)
#pragma warning(disable: 28735) /// Use ETW

   eventLogHandle = RegisterEventSource(0,
                                        g_pServiceName);
   HLPR_BAIL_ON_NULL_POINTER(eventLogHandle);

#pragma warning(pop)

   ppStrings[0] = g_pServiceName;
   ppStrings[1] = pMessage;

   ReportEvent(eventLogHandle,
               EVENTLOG_ERROR_TYPE,
               0,
               SVC_ERROR,
               0,
               2,
               0,
               ppStrings,
               0);

   HLPR_BAIL_LABEL:

   if(eventLogHandle)
      DeregisterEventSource(eventLogHandle);

   return;
}

/**
 @framework_function="ServiceStatusReportToSCM"
 
   Purpose:  Inform the Service Control Manager of the WFPSampler service's current state.      <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/MS686241.aspx              <br>
*/
_Success_(return == NO_ERROR)
UINT32 ServiceStatusReportToSCM(_In_ UINT32 currentState,
                                _In_ UINT32 exitCode,
                                _In_ UINT32 waitHint)
{
   UINT32        status     = NO_ERROR;
   static UINT32 checkpoint = 1;

   pServiceData->serviceStatus.dwCurrentState     = currentState;
   pServiceData->serviceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;
   pServiceData->serviceStatus.dwWin32ExitCode    = exitCode;
   pServiceData->serviceStatus.dwWaitHint         = waitHint;
   pServiceData->serviceStatus.dwCheckPoint       = checkpoint++;

   switch(currentState)
   {
      case SERVICE_START_PENDING:
      {
         pServiceData->serviceStatus.dwControlsAccepted = 0;

         break;
      }
      case SERVICE_RUNNING:
      {
         checkpoint = 1;

         pServiceData->serviceStatus.dwCheckPoint = 0;
/*
   The Action Center APIs are under Non Disclosure Agreement.  If you wish to see the sample code
   for this, you will need to contact your Microsoft representative and request the sample code.
*/
///         ActionCenterReportStatusEnabled();

         break;
      }
      case SERVICE_STOPPED:
      {
         checkpoint = 1;

         pServiceData->serviceStatus.dwCheckPoint = 0;
/*
   The Action Center APIs are under Non Disclosure Agreement.  If you wish to see the sample code
   for this, you will need to contact your Microsoft representative and request the sample code.
*/
///         ActionCenterReportStatusDisabled();

         break;
      }
   }

   if(!SetServiceStatus(pServiceData->serviceStatusHandle,
                        &(pServiceData->serviceStatus)))
   {
      status = GetLastError();

      ServiceEventLogError(L"ServiceStatusReportToSCM : SetServiceStatus()",
                           status);
   }

   return status;
}

/**
 @framework_function="ServiceControlHandler"
 
   Purpose:  Take action on the indicated service control.                                      <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
VOID ServiceControlHandler(_In_ UINT32 serviceControl)
{
   switch(serviceControl)
   {
      case SERVICE_CONTROL_CONTINUE:
      {
         ServiceStatusReportToSCM(SERVICE_CONTINUE_PENDING,
                                  NO_ERROR,
                                  0);

         break;
      }
      case SERVICE_CONTROL_PAUSE:
      {
         ServiceStatusReportToSCM(SERVICE_PAUSE_PENDING,
                                  NO_ERROR,
                                  0);

         break;
      }
      case SERVICE_CONTROL_SHUTDOWN:
      case SERVICE_CONTROL_STOP:
      {
         ServiceStatusReportToSCM(SERVICE_STOP_PENDING,
                                  NO_ERROR,
                                  0);

         SetEvent(pServiceData->serviceStopEventHandle);

         break;
      }
      default:
         break;
   }

   return;
}

/**
 @framework_function="ServiceTerminate"
 
   Purpose:  Cleanup and stop the WFPSampler service.                                           <br>
                                                                                                <br>
   Notes:    Cleanup entails relinquishing Windows Firewall categories, tearing down the RPC 
             server interface and stopping the service.                                         <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/MS688715.aspx              <br>
*/
_Success_(return == NO_ERROR)
UINT32 ServiceTerminate()
{
   UINT32 status = NO_ERROR;

   if(g_takeFirewallCategory)
      WindowsFirewallReleaseFirewallCategory();
/*
   The Action Center APIs are under Non Disclosure Agreement.  If you wish to see the sample code
   for this, you will need to contact your Microsoft representative and request the sample code.
*/
///   ActionCenterUnregisterFirewall();

   if(g_isCOMInitialized)
   {
      CoUninitialize();

      g_isCOMInitialized = FALSE;
   }

   RPCServerInterfaceTerminate();

   HLPR_CLOSE_HANDLE(pServiceData->serviceStopEventHandle);

   ServiceStatusReportToSCM(SERVICE_STOPPED,
                            status,
                            0);

   return status;
}

/**
 @framework_function="ServiceInitialize"
 
   Purpose:  Create environment to successfully run and start the WFPSampler service.           <br>
                                                                                                <br>
   Notes:    Creation entails taking the firewall category from Windows Firewall, building the 
             RPC server interface, and starting the WFPSampler service.                         <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_Success_(return == NO_ERROR)
UINT32 ServiceInitialize()
{
   UINT32         status       = NO_ERROR;
   REGISTRY_VALUE regValue     = {0};
   HANDLE         engineHandle = 0;
   FWPM_PROVIDER* pProvider    = 0;
   FWPM_SUBLAYER* pSubLayer    = 0;

   ServiceStatusReportToSCM(SERVICE_START_PENDING,
                            status,
                            2500);

   pServiceData->serviceStopEventHandle = CreateEvent(0,
                                                      TRUE,
                                                      FALSE,
                                                      L"WFPSampler_Stop_Event");
   if(pServiceData->serviceStopEventHandle == 0)
   {
      status = GetLastError();

      HLPR_BAIL;
   }

   regValue.type = REG_DWORD;
   regValue.size = sizeof(UINT32);

   HlprRegistryGetValue(HKEY_LOCAL_MACHINE,
                        L"SYSTEM\\CurrentControlSet\\Services\\WFPSampler",
                        L"coexistWithWindowsFirewall",
                        &regValue);
   if(regValue.dword)
      g_takeFirewallCategory = FALSE;

   status = HlprFwpmEngineOpen(&engineHandle);
   HLPR_BAIL_ON_FAILURE(status);

   /// Make sure WFPSampler's provider is available.  If not, create it.
   status = FwpmProviderGetByKey(engineHandle,
                                 &WFPSAMPLER_PROVIDER,
                                 &pProvider);
   if(status == FWP_E_PROVIDER_NOT_FOUND)
   {
      status = HlprFwpmProviderAdd(engineHandle,
                                   &WFPSAMPLER_PROVIDER,
                                   g_pCompanyName,
                                   g_pBinaryDescription,
                                   g_pServiceName,
                                   FWPM_PROVIDER_FLAG_PERSISTENT);
      HLPR_BAIL_ON_FAILURE(status);
   }
   else
      FwpmFreeMemory((VOID**)&pProvider);

   /// Make sure WFPSampler's subLayer is available.  If not, create it.
   status = FwpmSubLayerGetByKey(engineHandle,
                                 &WFPSAMPLER_SUBLAYER,
                                 &pSubLayer);
   if(status == FWP_E_SUBLAYER_NOT_FOUND)
   {
      status = HlprFwpmSubLayerAdd(engineHandle,
                                   &WFPSAMPLER_SUBLAYER,
                                   L"WFP Sampler's default subLayer",
                                   &WFPSAMPLER_PROVIDER,
                                   0x7FFE,
                                   FWPM_SUBLAYER_FLAG_PERSISTENT);
      HLPR_BAIL_ON_FAILURE(status);
   }
   else
      FwpmFreeMemory((VOID**)&pSubLayer);

   status = RPCServerInterfaceInitialize();
   HLPR_BAIL_ON_FAILURE(status);

   if(g_takeFirewallCategory)
   {
      status = WindowsFirewallAcquireFirewallCategory();
      HLPR_BAIL_ON_FAILURE(status);
   }
/*
   The Action Center APIs are under Non Disclosure Agreement.  If you wish to see the sample code
   for this, you will need to contact your Microsoft representative and request the sample code.
*/
///   status = ActionCenterRegisterFirewall();
///   HLPR_BAIL_ON_FAILURE(status);

   ServiceStatusReportToSCM(SERVICE_RUNNING,
                            status,
                            0);

   HLPR_BAIL_LABEL:

   if(engineHandle)
      HlprFwpmEngineClose(&engineHandle);

   if(status != NO_ERROR)
      ServiceStatusReportToSCM(SERVICE_STOPPED,
                               status,
                               0);

   return status;
}

/**
 @framework_function="ServiceUninstall"
 
   Purpose:  Remove the WFPSampler service from the Service Control Manager.                    <br>
                                                                                                <br>
   Notes:    Uninstalling will remove all WFP objects associated to the WFPSampler service.     <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/MS684323.aspx              <br>
             HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/MS684330.aspx              <br>
             HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/MS682108.aspx              <br>
             HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/MS682562.aspx              <br>
*/
VOID ServiceUninstall()
{
   UINT32         status        = NO_ERROR;
   SC_HANDLE      scmHandle     = 0;
   SC_HANDLE      svcHandle     = 0;
   HANDLE         engineHandle  = 0;
   SERVICE_STATUS serviceStatus = {0};

   HlprFwpmEngineOpen(&engineHandle);
   if(engineHandle)
   {
      HlprFwpmFilterRemoveAll(&engineHandle,
                              &WFPSAMPLER_PROVIDER);

      HlprFwpmCalloutRemoveAll(&engineHandle,
                               &WFPSAMPLER_PROVIDER);

      HlprFwpmProviderContextRemoveAll(&engineHandle,
                                       &WFPSAMPLER_PROVIDER);

      HlprFwpmSubLayerDelete(&engineHandle,
                             &WFPSAMPLER_SUBLAYER);

      HlprFwpmProviderDelete(&engineHandle,
                             &WFPSAMPLER_PROVIDER);
   }

   scmHandle = OpenSCManager(0,                        /// local Machine
                             SERVICES_ACTIVE_DATABASE,
                             SC_MANAGER_ALL_ACCESS);
   if(scmHandle == 0)
   {
      status = GetLastError();

      HlprLogError(L"ServiceUninstall : OpenSCManager() [status: %#x]",
                   status);

      HLPR_BAIL;
   }

   svcHandle = OpenService(scmHandle,
                           g_pServiceName,
                           SERVICE_ALL_ACCESS);
   if(svcHandle == 0)
   {
      status = GetLastError();

      HlprLogError(L"ServiceUninstall : OpenService() [status: %#x]",
                   status);

      HLPR_BAIL;
   }

   if(!ControlService(svcHandle,
                      SERVICE_CONTROL_STOP,
                      &serviceStatus))
   {
      status = GetLastError();

      if(status != ERROR_SERVICE_NOT_ACTIVE &&
         status != ERROR_SHUTDOWN_IN_PROGRESS)
         HlprLogError(L"ServiceUninstall : ControlService() [status: %#x]",
                      status);
   }

   if(!DeleteService(svcHandle))
   {
      status = GetLastError();

      HlprLogError(L"ServiceUninstall : DeleteService() [status: %#x]",
                   status);

      HLPR_BAIL;
   }

   status = PrvRegistryDestroyEntries();
   HLPR_BAIL_ON_FAILURE(status);

   HlprLogInfo(L"ServiceUninstall() [status: %#x]",
               status);
   
   HLPR_BAIL_LABEL:

   if(engineHandle)
      HlprFwpmEngineClose(&engineHandle);

   HLPR_CLOSE_SERVICE_HANDLE(svcHandle);

   HLPR_CLOSE_SERVICE_HANDLE(scmHandle);

   return;
}

/**
 @framework_function="ServiceInstall"
 
   Purpose:  Add the WFPSampler service to the Service Control Manager and start it.            <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/MS683197.aspx              <br>
             HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/MS684323.aspx              <br>
             HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/MS682450.aspx              <br>
*/
_Success_(return == NO_ERROR)
UINT32 ServiceInstall()
{
   UINT32    status           = NO_ERROR;
   UINT32    size             = 1024;
   PWSTR     pFileName        = 0;
   SC_HANDLE scmHandle        = 0;
   SC_HANDLE svcHandle        = 0;
   PCWSTR    pWindowsFirewall = L"MpsSvc\0\0\0";
   HANDLE    engineHandle     = 0;

   HLPR_NEW_ARRAY(pFileName,
                  WCHAR,
                  size);
   HLPR_BAIL_ON_ALLOC_FAILURE(pFileName,
                              status);

   if(!GetModuleFileName(0,                                /// this process
                         pFileName,
                         size))
   {
      status = GetLastError();

      HlprLogError(L"ServiceInstall : GetModuleFileName() [status: %#x]",
                   status);

      HLPR_BAIL;
   }

   scmHandle = OpenSCManager(0,                            /// local Machine
                             SERVICES_ACTIVE_DATABASE,
                             SC_MANAGER_ALL_ACCESS);
   if(scmHandle == 0)
   {
      status = GetLastError();

      HlprLogError(L"ServiceInstall : OpenSCManager() [status: %#x]",
                   status);

      HLPR_BAIL;
   }

   svcHandle = CreateService(scmHandle,
                             g_pServiceName,
                             g_pServiceDescription,
                             SERVICE_ALL_ACCESS,
                             SERVICE_WIN32_OWN_PROCESS,
                             SERVICE_AUTO_START,
                             SERVICE_ERROR_CRITICAL,
                             pFileName,
                             0,                            /// No load ordering group
                             0,                            /// No Tag identifier
                             pWindowsFirewall,             /// Create dependency
                             0,                            /// LocalSystem
                             0);                           /// No password
   if(svcHandle == 0)
   {
      status = GetLastError();

      HlprLogError(L"ServiceInstall : CreateService() [status: %#x]",
                   status);

      HLPR_BAIL;
   }

   /// Create keys so service can run in SafeMode with Networking
   PrvRegistryCreateEntries();

   status = HlprFwpmEngineOpen(&engineHandle);
   HLPR_BAIL_ON_FAILURE(status);

   status = HlprFwpmTransactionBegin(engineHandle);
   HLPR_BAIL_ON_FAILURE(status);

   status = HlprFwpmProviderAdd(engineHandle,
                                &WFPSAMPLER_PROVIDER,
                                g_pCompanyName,
                                g_pBinaryDescription,
                                g_pServiceName,
                                FWPM_PROVIDER_FLAG_PERSISTENT);
   HLPR_BAIL_ON_FAILURE(status);

   status = HlprFwpmSubLayerAdd(engineHandle,
                                &WFPSAMPLER_SUBLAYER,
                                L"WFP Sampler's default subLayer",
                                &WFPSAMPLER_PROVIDER,
                                0x7FFF,
                                FWPM_SUBLAYER_FLAG_PERSISTENT);
   HLPR_BAIL_ON_FAILURE(status);

   status = HlprFwpmTransactionCommit(engineHandle);
   HLPR_BAIL_ON_FAILURE(status);

   HlprLogInfo(L"ServiceInstall() [status: %#x]",
               status);

   HLPR_BAIL_LABEL:

   if(engineHandle)
   {
      if(status != NO_ERROR)
         HlprFwpmTransactionAbort(engineHandle);

      HlprFwpmEngineClose(&engineHandle);
   }

   HLPR_CLOSE_SERVICE_HANDLE(svcHandle);

   HLPR_CLOSE_SERVICE_HANDLE(scmHandle);

   HLPR_DELETE_ARRAY(pFileName);

   if(status != NO_ERROR)
      ServiceUninstall();

   return status;
}

/**
 @framework_function="ServiceMain"
 
   Purpose:  Entry point for the Service Control Manager to hook into the WFPSampler service.   <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/MS685138.aspx              <br>
*/
VOID ServiceMain(_In_ const UINT32 argumentCount,
                 _In_reads_(argumentCount) PCWSTR ppArguments[])
{
   UNREFERENCED_PARAMETER(argumentCount);
   UNREFERENCED_PARAMETER(ppArguments);

   UINT32 status = NO_ERROR;

   HLPR_NEW(pServiceData,
            SERVICE_DATA);
   HLPR_BAIL_ON_ALLOC_FAILURE(pServiceData,
                              status);

   pServiceData->serviceStatusHandle = RegisterServiceCtrlHandler(g_pServiceName,
                                                                  (LPHANDLER_FUNCTION)ServiceControlHandler);
   if(pServiceData->serviceStatusHandle == 0)
   {
      ServiceEventLogError(L"ServiceMain : RegisterServiceCtrlHandler()",
                           status);

      HLPR_BAIL;
   }

   pServiceData->serviceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;

   status = ServiceInitialize();
   HLPR_BAIL_ON_FAILURE(status);

   for(;
       ;
      )
   {
      status = WaitForSingleObject(pServiceData->serviceStopEventHandle,
                                   INFINITE);
      if(status == WAIT_OBJECT_0)
      {
         ServiceTerminate();

         break;
      }
   }

   HLPR_BAIL_LABEL:

   HLPR_DELETE(pServiceData);

   return;
}

/**
 @framework_function="wmain"

   Purpose:  Entry point for WFPSamplerService.Exe.  Accepts several parameters which are 
             parsed and dealt with accordingly.                                                 <br>
                -i  - installs the WFPSampler service.                                          <br>
                -u  - uninstalls the WFPSampler service and its associated objects.             <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_Success_(return == NO_ERROR)
int __cdecl wmain(_In_ const int argumentCount,
                  _In_reads_(argumentCount) PCWSTR pArguments[])
{
   UINT32              status           = NO_ERROR;
   SERVICE_TABLE_ENTRY pServiceTable[2] = {0};

   /// First argument is the executable's name
   /// WFPSamplerSvc.exe, so start with next argument
   if(argumentCount > 1)
   {
      PCWSTR* ppCommandLineParameterStrings = (PCWSTR*)&(pArguments[1]);
      UINT32  stringCount                   = argumentCount - 1;

      for(UINT32 index = 0;
          index < stringCount;
          index++)
      {
         if(HlprStringsAreEqual(L"-i",
                                ppCommandLineParameterStrings[index]))
         {
            status = ServiceInstall();
            HLPR_BAIL_ON_FAILURE(status);

            break;
         }
         else if(HlprStringsAreEqual(L"-u",
                                     ppCommandLineParameterStrings[index]))
         {
            ServiceUninstall();

            HLPR_BAIL;
         }
      }
   }

   pServiceTable[0].lpServiceName = (LPWSTR)g_pServiceName;
   pServiceTable[0].lpServiceProc = (LPSERVICE_MAIN_FUNCTION)ServiceMain;

   if(!StartServiceCtrlDispatcher(pServiceTable))
      status = GetLastError();

   HLPR_BAIL_LABEL:

   return status;
}
