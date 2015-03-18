/*++

Copyright (c) Microsoft Corporation. All rights reserved

Abstract:

    Monitor Sample driver IO control routines

Environment:

    Kernel mode

--*/

#include <ntddk.h>
#include <wdf.h>

#include <fwpmk.h>

#pragma warning(push)
#pragma warning(disable:4201)       // unnamed struct/union

#include <fwpsk.h>

#pragma warning(pop)


#include "ioctl.h"

#include "msnmntr.h"
#include "ctl.h"

#include "notify.h"

//
// Software Tracing Definitions 
//
#define WPP_CONTROL_GUIDS \
    WPP_DEFINE_CONTROL_GUID(MsnMntrCtl,(eab718af, 52de, 477c, 874d, cb49746bb131),  \
        WPP_DEFINE_BIT(TRACE_INIT)               \
        WPP_DEFINE_BIT(TRACE_DEVICE_CONTROL)     \
        WPP_DEFINE_BIT(TRACE_STATE) )

#include "ctl.tmh"

EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL MonitorEvtDeviceControl;

NTSTATUS
MonitorCtlDriverInit(
    _In_ WDFDEVICE* pDevice
    )
/*++

Routine Description:

   Initializes the request queue for our driver.  This is how 
   DeviceIoControl requests are sent to KMDF drivers.

Arguments:
   
   [in]  WDFDEVICE* pDevice - Our device.

--*/
{
   NTSTATUS status;
   WDF_IO_QUEUE_CONFIG queueConfig;

   DoTraceMessage(TRACE_INIT, "MonitorSample Control Initialization in progress.");

   WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(
      &queueConfig,
      WdfIoQueueDispatchSequential
      );

   queueConfig.EvtIoDeviceControl = MonitorEvtDeviceControl;

   status = WdfIoQueueCreate(
               *pDevice,
               &queueConfig,
               WDF_NO_OBJECT_ATTRIBUTES,
               NULL
               );

   return status;
}

VOID
MonitorEvtDeviceControl (
    _In_ WDFQUEUE Queue,
    _In_ WDFREQUEST Request,
    _In_ size_t OutputBufferLength,
    _In_ size_t InputBufferLength,
    _In_ ULONG IoControlCode
    )
/*++

   Handles device IO control requests. This callback drives all communication
   between the usermode exe and this driver.

--*/
{
   NTSTATUS status = STATUS_SUCCESS;

   UNREFERENCED_PARAMETER(Queue);
   UNREFERENCED_PARAMETER(OutputBufferLength);

   DoTraceMessage(TRACE_DEVICE_CONTROL, "MonitorSample Dispatch Device Control: 0x%x", IoControlCode);

   switch (IoControlCode)
   {
      case MONITOR_IOCTL_ENABLE_MONITOR:
      {
         WDFMEMORY pMemory;
         void* pBuffer;

         if (InputBufferLength < sizeof(MONITOR_SETTINGS))
         {
            status = STATUS_INVALID_PARAMETER;
         }
         else
         {
            status = WdfRequestRetrieveInputMemory(Request, &pMemory);

            if (NT_SUCCESS(status))
            {
               pBuffer = WdfMemoryGetBuffer(pMemory, NULL);
               status = MonitorCoEnableMonitoring((MONITOR_SETTINGS*) pBuffer);
            }
         }
         break;
      }

      case MONITOR_IOCTL_DISABLE_MONITOR:
      {
         status = STATUS_SUCCESS;
         
         MonitorCoDisableMonitoring();

         break;
      }

      default:
      {
         status = STATUS_INVALID_PARAMETER;
      }
   }

   WdfRequestComplete(Request, status);
}
