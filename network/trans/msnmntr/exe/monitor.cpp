/*++

Copyright (c) Microsoft Corporation. All rights reserved

Abstract:

    Stream monitor sample executable

Environment:

    User mode
    
--*/

#include "windows.h"
#include "winioctl.h"
#include "strsafe.h"

#ifndef _CTYPE_DISABLE_MACROS
#define _CTYPE_DISABLE_MACROS
#endif

#include "fwpmu.h"

#include "winsock2.h"
#include "ws2def.h"

#include <conio.h>
#include <stdio.h>

#include "ioctl.h"

#define INITGUID
#include <guiddef.h>
#include "mntrguid.h"


#define MONITOR_FLOW_ESTABLISHED_CALLOUT_DESCRIPTION L"Monitor Sample - Flow Established Callout"
#define MONITOR_FLOW_ESTABLISHED_CALLOUT_NAME L"Flow Established Callout"

#define MONITOR_STREAM_CALLOUT_DESCRIPTION L"Monitor Sample - Stream Callout"
#define MONITOR_STREAM_CALLOUT_NAME L"Stream Callout"

HANDLE quitEvent;

DWORD
MonitorAppOpenMonitorDevice(
   _Out_ HANDLE* monitorDevice)
/*++

Routine Description:

    Opens the Monitor Sample monitorDevice

Arguments:

    [out] HANDLE* monitorDevice

Return Value:

    NO_ERROR, ERROR_INVALID_PARAMETER or a CreateFile specific result.

--*/
{
    if (!monitorDevice)
    {
        return ERROR_INVALID_PARAMETER;
    }
    *monitorDevice = CreateFileW(MONITOR_DOS_NAME, 
                                 GENERIC_READ | GENERIC_WRITE, 
                                 FILE_SHARE_READ | FILE_SHARE_WRITE, 
                                 NULL, 
                                 OPEN_EXISTING, 
                                 0, 
                                 NULL);

    if (*monitorDevice == INVALID_HANDLE_VALUE)
    {
       return GetLastError();
    }

    return NO_ERROR;
}

BOOL MonitorAppCloseMonitorDevice(
   _In_ HANDLE monitorDevice)
/*++

Routine Description:

    Closes the Monitor Sample monitorDevice

Arguments:

Return Value:

    None.

--*/
{
    return CloseHandle(monitorDevice);
}

DWORD
MonitorAppAddCallouts()
/*++

Routine Description:

   Adds the callouts during installation

Arguments:
   
   [in]  PCWSTR AppPath - The path to the application to monitor.

Return Value:

    NO_ERROR or a specific FWP result.

--*/
{
   FWPM_CALLOUT callout;
   DWORD result;
   FWPM_DISPLAY_DATA displayData;
   HANDLE engineHandle = NULL;
   FWPM_SESSION session;
   RtlZeroMemory(&session, sizeof(FWPM_SESSION));

   session.displayData.name = L"Monitor Sample Non-Dynamic Session";
   session.displayData.description = L"For Adding callouts";

   printf("Opening Filtering Engine\n");
   result =  FwpmEngineOpen(
                            NULL,
                            RPC_C_AUTHN_WINNT,
                            NULL,
                            &session,
                            &engineHandle
                            );

   if (NO_ERROR != result)
   {
      goto cleanup;
   }

   printf("Starting Transaction for adding callouts\n");
   result = FwpmTransactionBegin(engineHandle, 0);
   if (NO_ERROR != result)
   {
      goto abort;
   }

   printf("Successfully started the Transaction\n");

   RtlZeroMemory(&callout, sizeof(FWPM_CALLOUT));
   displayData.description = MONITOR_FLOW_ESTABLISHED_CALLOUT_DESCRIPTION;
   displayData.name = MONITOR_FLOW_ESTABLISHED_CALLOUT_NAME;

   callout.calloutKey = MONITOR_SAMPLE_FLOW_ESTABLISHED_CALLOUT_V4;
   callout.displayData = displayData;
   callout.applicableLayer = FWPM_LAYER_ALE_FLOW_ESTABLISHED_V4;
   callout.flags = FWPM_CALLOUT_FLAG_PERSISTENT; // Make this a persistent callout.

   printf("Adding Persistent Flow Established callout through the Filtering Engine\n");

   result = FwpmCalloutAdd(engineHandle, &callout, NULL, NULL);
   if (NO_ERROR != result)
   {
      goto abort;
   }

   printf("Successfully Added Persistent Flow Established callout.\n");

   RtlZeroMemory(&callout, sizeof(FWPM_CALLOUT));

   displayData.description = MONITOR_STREAM_CALLOUT_DESCRIPTION;
   displayData.name = MONITOR_STREAM_CALLOUT_DESCRIPTION;

   callout.calloutKey = MONITOR_SAMPLE_STREAM_CALLOUT_V4;
   callout.displayData = displayData;
   callout.applicableLayer = FWPM_LAYER_STREAM_V4;
   callout.flags = FWPM_CALLOUT_FLAG_PERSISTENT; // Make this a persistent callout.

   printf("Adding Persistent Stream callout through the Filtering Engine\n");

   result = FwpmCalloutAdd(engineHandle, &callout, NULL, NULL);
   if (NO_ERROR != result)
   {
      goto abort;
   }

   printf("Successfully Added Persistent Stream callout.\n");
   
   printf("Committing Transaction\n");
   result = FwpmTransactionCommit(engineHandle);
   if (NO_ERROR == result)
   {
      printf("Successfully Committed Transaction.\n");
   }
   goto cleanup;

abort:
   printf("Aborting Transaction\n");
   result = FwpmTransactionAbort(engineHandle);
   if (NO_ERROR == result)
   {
      printf("Successfully Aborted Transaction.\n");
   }

cleanup:

   if (engineHandle)
   {
      FwpmEngineClose(engineHandle);
   }
   return result;
}

DWORD
MonitorAppRemoveCallouts()
/*++

Routine Description:

   Sets the kernel callout ID's through the Monitor Sample device

Arguments:
   
   [in] HANDLE monitorDevice - Monitor Sample device
   [in] CALLOUTS* callouts - Callout structure with ID's set
   [in] DWORD size - Size of the callout structure.

Return Value:

    NO_ERROR or a specific DeviceIoControl result.

--*/
{
   DWORD result;
   HANDLE engineHandle = NULL;
   FWPM_SESSION session;

   RtlZeroMemory(&session, sizeof(FWPM_SESSION));

   session.displayData.name = L"Monitor Sample Non-Dynamic Session";
   session.displayData.description = L"For Adding callouts";

   printf("Opening Filtering Engine\n");
   result =  FwpmEngineOpen(
                            NULL,
                            RPC_C_AUTHN_WINNT,
                            NULL,
                            &session,
                            &engineHandle
                            );

   if (NO_ERROR != result)
   {
      goto cleanup;
   }

   printf("Starting Transaction for Removing callouts\n");

   result = FwpmTransactionBegin(engineHandle, 0);
   if (NO_ERROR != result)
   {
      goto abort;
   }
   printf("Successfully started the Transaction\n");

   printf("Deleting Flow Established callout\n");
   result = FwpmCalloutDeleteByKey(engineHandle,
                                    &MONITOR_SAMPLE_FLOW_ESTABLISHED_CALLOUT_V4);
   if (NO_ERROR != result)
   {
      goto abort;
   }

   printf("Successfully Deleted Flow Established callout\n");

   printf("Deleting Stream callout\n");

   result = FwpmCalloutDeleteByKey(engineHandle,
                                    &MONITOR_SAMPLE_STREAM_CALLOUT_V4);
   if (NO_ERROR != result)
   {
      goto abort;
   }
   printf("Successfully Deleted Stream callout\n");

   printf("Committing Transaction\n");
   result = FwpmTransactionCommit(engineHandle);
   if (NO_ERROR == result)
   {
      printf("Successfully Committed Transaction.\n");
   }
   goto cleanup;
   
abort:
   printf("Aborting Transaction\n");
   result = FwpmTransactionAbort(engineHandle);
   if (NO_ERROR == result)
   {
      printf("Successfully Aborted Transaction.\n");
   }

cleanup:

   if (engineHandle)
   {
      FwpmEngineClose(engineHandle);
   }

   return result;
}

DWORD
MonitorAppEnableMonitoring(
   _In_    HANDLE            monitorDevice,
   _In_    MONITOR_SETTINGS* monitorSettings)
/*++

Routine Description:

   Enables monitoring on new connections.

Arguments:
   
   [in] HANDLE monitorDevice - Monitor Sample device
   [in] MONITOR_SETTINGS* monitorSettings - Settings for the Monitor Sample driver.

Return Value:

    NO_ERROR or a specific DeviceIoControl result.

--*/
{
   DWORD bytesReturned;
   
   if (!DeviceIoControl(monitorDevice,
                        MONITOR_IOCTL_ENABLE_MONITOR,
                        monitorSettings,
                        sizeof(MONITOR_SETTINGS),
                        NULL,
                        0,
                        &bytesReturned,
                        NULL))
   {
      return GetLastError();
   }

   return NO_ERROR;
}

DWORD
MonitorAppDisableMonitoring(
   _In_    HANDLE            monitorDevice)
/*++

Routine Description:

   Disables monitoring of new flows (existing flows will continue to be
   monitored until the driver is stopped or the flows end).

Arguments:

   [in] HANDLE monitorDevice - Monitor Sample device handle.
   
Return Value:

   NO_ERROR or DeviceIoControl specific code.

--*/
{
   DWORD bytesReturned;
   
   if (!DeviceIoControl(monitorDevice,
                        MONITOR_IOCTL_DISABLE_MONITOR,
                        NULL,
                        0,
                        NULL,
                        0,
                        &bytesReturned,
                        NULL))
   {
      return GetLastError();
   }

   return NO_ERROR;
}

DWORD
MonitorAppAddFilters(
   _In_    HANDLE         engineHandle,
   _In_    FWP_BYTE_BLOB* applicationPath)
/*++

Routine Description:

    Adds the required sublayer, filters and callouts to the Windows
    Filtering Platform (WFP).

Arguments:
   
   [in] HANDLE engineHandle - Handle to the base Filtering engine
   [in] FWP_BYTE_BLOB* applicationPath - full path to the application including
                                         the NULL terminator and size also 
                                         including the NULL the terminator
   [in] CALLOUTS* callouts - The callouts that need to be added.

Return Value:

    NO_ERROR or a specific result

--*/
{
   DWORD result = NO_ERROR;
   FWPM_SUBLAYER monitorSubLayer;
   FWPM_FILTER filter;
   FWPM_FILTER_CONDITION filterConditions[2]; // We only need two for this call.

   RtlZeroMemory(&monitorSubLayer, sizeof(FWPM_SUBLAYER)); 

   monitorSubLayer.subLayerKey = MONITOR_SAMPLE_SUBLAYER;
   monitorSubLayer.displayData.name = L"Monitor Sample Sub layer";
   monitorSubLayer.displayData.description = L"Monitor Sample Sub layer";
   monitorSubLayer.flags = 0;
   // We don't really mind what the order of invocation is.
   monitorSubLayer.weight = 0;
   
   printf("Starting Transaction\n");

   result = FwpmTransactionBegin(engineHandle, 0);
   if (NO_ERROR != result)
   {
      goto abort;
   }
   printf("Successfully Started Transaction\n");

   printf("Adding Sublayer\n");

   result = FwpmSubLayerAdd(engineHandle, &monitorSubLayer, NULL);
   if (NO_ERROR != result)
   {
      goto abort;
   }
   
   printf("Sucessfully added Sublayer\n");
   
   RtlZeroMemory(&filter, sizeof(FWPM_FILTER));

   filter.layerKey = FWPM_LAYER_ALE_FLOW_ESTABLISHED_V4;
   filter.displayData.name = L"Flow established filter.";
   filter.displayData.description = L"Sets up flow for traffic that we are interested in.";
   filter.action.type = FWP_ACTION_CALLOUT_INSPECTION; // We're only doing inspection.
   filter.action.calloutKey = MONITOR_SAMPLE_FLOW_ESTABLISHED_CALLOUT_V4;
   filter.filterCondition = filterConditions;
   filter.subLayerKey = monitorSubLayer.subLayerKey;
   filter.weight.type = FWP_EMPTY; // auto-weight.
      
   filter.numFilterConditions = 2;

   RtlZeroMemory(filterConditions, sizeof(filterConditions));

   //
   // Add the application path to the filter conditions.
   //
   filterConditions[0].fieldKey = FWPM_CONDITION_ALE_APP_ID;
   filterConditions[0].matchType = FWP_MATCH_EQUAL;
   filterConditions[0].conditionValue.type = FWP_BYTE_BLOB_TYPE;
   filterConditions[0].conditionValue.byteBlob = applicationPath;

   //
   // For the purposes of this sample, we will monitor TCP traffic only.
   //
   filterConditions[1].fieldKey = FWPM_CONDITION_IP_PROTOCOL;
   filterConditions[1].matchType = FWP_MATCH_EQUAL;
   filterConditions[1].conditionValue.type = FWP_UINT8;
   filterConditions[1].conditionValue.uint8 = IPPROTO_TCP;

   printf("Adding Flow Established Filter\n");

   result = FwpmFilterAdd(engineHandle,
                       &filter,
                       NULL,
                       NULL);

   if (NO_ERROR != result)
   {
      goto abort;
   }

   printf("Successfully added Flow Established filter\n");
  
   RtlZeroMemory(&filter, sizeof(FWPM_FILTER));

   filter.layerKey = FWPM_LAYER_STREAM_V4;
   filter.action.type = FWP_ACTION_CALLOUT_INSPECTION; // We're only doing inspection.
   filter.action.calloutKey = MONITOR_SAMPLE_STREAM_CALLOUT_V4;
   filter.subLayerKey = monitorSubLayer.subLayerKey;
   filter.weight.type = FWP_EMPTY; // auto-weight.
   
   filter.numFilterConditions = 0;
   
   RtlZeroMemory(filterConditions, sizeof(filterConditions));

   filter.filterCondition = filterConditions;
   
   filter.displayData.name = L"Stream Layer Filter";
   filter.displayData.description = L"Monitors TCP traffic.";

   printf("Adding Stream Filter\n");

   result = FwpmFilterAdd(engineHandle,
                       &filter,
                       NULL,
                       NULL);

   if (NO_ERROR != result)
   {
      goto abort;
   }

   printf("Successfully added Stream filter\n");

   printf("Committing Transaction\n");
   result = FwpmTransactionCommit(engineHandle);
   if (NO_ERROR == result)
   {
      printf("Successfully Committed Transaction\n");
   }
   goto cleanup;

abort:
   printf("Aborting Transaction\n");
   result = FwpmTransactionAbort(engineHandle);
   if (NO_ERROR == result)
   {
      printf("Successfully Aborted Transaction\n");
   }

cleanup:
   
   return result;
}

DWORD
MonitorAppIDFromPath(
    _In_ PCWSTR fileName,
    _Out_ FWP_BYTE_BLOB** appId)
{
   DWORD result = NO_ERROR;
   
   result = FwpmGetAppIdFromFileName(fileName, appId);

   return result;
}

DWORD
MonitorAppDoMonitoring(PCWSTR AppPath)
{
   HANDLE            monitorDevice = NULL;
   HANDLE            engineHandle = NULL;
   DWORD             result;
   MONITOR_SETTINGS  monitorSettings;
   FWPM_SESSION     session;
   FWP_BYTE_BLOB*    applicationId = NULL;

   RtlZeroMemory(&monitorSettings, sizeof(MONITOR_SETTINGS));
   RtlZeroMemory(&session, sizeof(FWPM_SESSION));

   session.displayData.name = L"Monitor Sample Session";
   session.displayData.description = L"Monitors traffic at the Stream layer.";

   // Let the Base Filtering Engine cleanup after us.
   session.flags = FWPM_SESSION_FLAG_DYNAMIC;

   printf("Opening Filtering Engine\n");
   result =  FwpmEngineOpen(
                            NULL,
                            RPC_C_AUTHN_WINNT,
                            NULL,
                            &session,
                            &engineHandle
                            );

   if (NO_ERROR != result)
   {
      goto cleanup;
   }

   printf("Successfully opened Filtering Engine\n");

   printf("Looking up Application ID from BFE\n");
   result = MonitorAppIDFromPath(AppPath, &applicationId);

   if (NO_ERROR != result)
   {
      goto cleanup;
   }

   printf("Successfully retrieved Application ID\n");

   printf("Opening Monitor Sample Device\n");

   result = MonitorAppOpenMonitorDevice(&monitorDevice);
   if (NO_ERROR != result)
   {
      goto cleanup;
   }

   printf("Successfully opened Monitor Device\n");

   printf("Adding Filters through the Filtering Engine\n");

   result = MonitorAppAddFilters(engineHandle, 
                                applicationId);

   if (NO_ERROR != result)
   {
      goto cleanup;
   }

   printf("Successfully added Filters through the Filtering Engine\n");

   printf("Enabling monitoring through the Monitor Sample Device\n");

   monitorSettings.monitorOperation = monitorTraffic;
   
   result = MonitorAppEnableMonitoring(monitorDevice,
                                      &monitorSettings);
   if (NO_ERROR != result)
   {
      goto cleanup;
   }

   printf("Successfully enabled monitoring.\n");

   printf("Events will be traced through WMI. Please press any key to exit and cleanup filters.\n");

#pragma prefast(push)
#pragma prefast(disable:6031, "by design the return value of _getch() is ignored here")
   _getch();
#pragma prefast(pop)

cleanup:

   if (NO_ERROR != result)
   {
      printf("Monitor.\tError 0x%x occurred during execution\n", result);
   }

   if (monitorDevice)
   {
      MonitorAppCloseMonitorDevice(monitorDevice);
   }

   //
   // Free the application Id that we retrieved.
   //
   if (applicationId)
   {
      FwpmFreeMemory((void**)&applicationId);
   }
   
   if (engineHandle)
   {
      result =  FwpmEngineClose(engineHandle);
      engineHandle = NULL;
   }

   return result;
}

void
MonitorPrintUsage()
{
   wprintf(L"Usage: monitor ( addcallouts | delcallouts | monitor <targetApp.exe> )\n");
}

DWORD
MonitorAppProcessArguments(_In_ int argc, _In_reads_(argc) PCWSTR argv[])
{
   if (argc == 2)
   {
      if (_wcsicmp(argv[1], L"addcallouts") == 0)
      {
         return MonitorAppAddCallouts();
      }
      if (_wcsicmp(argv[1], L"delcallouts") == 0)
      {
         return MonitorAppRemoveCallouts();
      }
   }

   if (argc == 3)
   {
      if (_wcsicmp(argv[1], L"monitor") == 0)
      {
         return MonitorAppDoMonitoring(argv[2]);
      }
   }

   MonitorPrintUsage();
   return ERROR_INVALID_PARAMETER;
}

int __cdecl wmain(_In_ int argc, _In_reads_(argc) PCWSTR argv[])
{
   DWORD result;
   
   result = MonitorAppProcessArguments(argc, argv);

   return (int)result;
}
