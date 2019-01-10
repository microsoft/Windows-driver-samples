/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    queue.cpp

Abstract:

    This file contains the queue entry points and callbacks.

Environment:

    Windows User-Mode Driver Framework

--*/

#include "precomp.h"
#include "Trace.h"
#include "Defaults.h"
#include "Device.h"
#include "FixSession.h"
#include "FixHandler.h"
#include "Queue.h"

#include "Queue.tmh"


CQueue::CQueue(
    WDFQUEUE Queue
) :
    _Queue(Queue),
    _ForcedDriverVersion(GNSS_DRIVER_DDK_VERSION)
{
    InitializeCriticalSection(&_Lock);
}

CQueue::~CQueue()
{
    DeleteCriticalSection(&_Lock);
    _Queue = nullptr;
}


void
CQueue::OnCleanup(
    _In_ WDFOBJECT Object
)
{
    CQueue *pQueue = GetQueueObject(Object);

    // Queue object constructed using placement 'new' so explicitly invoke destructor
    pQueue->~CQueue();
}

NTSTATUS
CQueue::Initialize()
{
    NTSTATUS status = STATUS_SUCCESS;

    // Initialize the Fix Handler
    status = _FixHandler.Initialize(_Queue);
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_QUEUE, "_FixHandler->Initialize failed with %!STATUS!", status);
        goto Exit;
    }

Exit:
    return status;
}

void
CQueue::OnIoDeviceControl(
    _In_ WDFQUEUE Queue,
    _In_ WDFREQUEST Request,
    _In_ size_t OutputBufferLength,
    _In_ size_t InputBufferLength,
    _In_ ULONG IoControlCode
)
{
    CQueue *pQueue = GetQueueObject(Queue);

    pQueue->OnIoDeviceControl(Request, IoControlCode, InputBufferLength, OutputBufferLength);
}

NTSTATUS
CQueue::AddQueueToDevice(
    _In_ WDFDEVICE Device,
    _Outptr_ CQueue** Queue
)
{
    NTSTATUS status = STATUS_SUCCESS;

    // Create a sequential IO Queue for serial operation and supply all the callbacks.
    // A sequential queue will cause all IO requests to be serialized to the device (i.e., only one request will sent at a time).
    WDF_IO_QUEUE_CONFIG queueConfig;
    WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&queueConfig,
                                           WdfIoQueueDispatchSequential);

    queueConfig.PowerManaged = WdfFalse;
    queueConfig.EvtIoDeviceControl = CQueue::OnIoDeviceControl;

    WDF_OBJECT_ATTRIBUTES queueAttributes;
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&queueAttributes,
                                            CQueue);

    queueAttributes.EvtCleanupCallback = CQueue::OnCleanup;

    WDFQUEUE queue = nullptr;
    status = WdfIoQueueCreate(Device,
                              &queueConfig,
                              &queueAttributes,
                              &queue);

    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DEVICE, "WdfIoQueueCreate failed with Status %!STATUS!", status);
        goto Exit;
    }

    // Construct a queue object on the preallocated buffer using placement new operation
    *Queue = new (GetQueueObject(queue)) CQueue(queue);

    status = (*Queue)->Initialize();
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DEVICE, "CQueue::Initialize failed with %!STATUS!", status);
        goto Exit;
    }
 
Exit:
    return status;
}

void
CQueue::OnIoDeviceControl(
    _In_ WDFREQUEST Request,
    _In_ ULONG IoControlCode,
    _In_ size_t InputBufferLength,
    _In_ size_t OutputBufferLength)
{
    TraceEvents(TRACE_LEVEL_INFORMATION,
                TRACE_QUEUE,
                "Request = %p, IoControlCode = 0x%x InputBufferLength=%d OutputBufferLength=%d",
                Request, IoControlCode, (int)InputBufferLength, (int)OutputBufferLength);

    NTSTATUS status = STATUS_SUCCESS;
    
    switch (IoControlCode)
    {
        // Capability Exchange
    case IOCTL_GNSS_SEND_PLATFORM_CAPABILITY:
        break;
    case IOCTL_GNSS_GET_DEVICE_CAPABILITY:
        status = GetDeviceCapability(Request);
        break;

    case IOCTL_GNSS_GET_CHIPSETINFO:
        status = GetChipsetInfo(Request);
        break;

        // Fix
    case IOCTL_GNSS_START_FIXSESSION:
        status = _FixHandler.StartFix(Request, _InternalState.SetLocationServiceEnabled);
        break;
    case IOCTL_GNSS_STOP_FIXSESSION:
        status = _FixHandler.StopFix(Request);
        break;
    case IOCTL_GNSS_MODIFY_FIXSESSION:
        status = _FixHandler.ModifyFix(Request);
        break;
    case IOCTL_GNSS_GET_FIXDATA:
        status = _FixHandler.GetFixRequest(Request);
        break;

        // Driver Command
    case IOCTL_GNSS_SEND_DRIVERCOMMAND:
        status = HandleDriverCommand(Request);
        break;

        //
        // FIX ME:
        // The below is optional IOCTLs. Driver developer may choose to implement for advanced functionality.
        //

        // Agnss
    case IOCTL_GNSS_INJECT_AGNSS:
    case IOCTL_GNSS_LISTEN_AGNSS:
        // Supl
    case IOCTL_GNSS_SET_SUPL_HSLP:
    case IOCTL_GNSS_CONFIG_SUPL_CERT:
    case IOCTL_GNSS_SET_V2UPL_CONFIG:
        //Ni
    case IOCTL_GNSS_LISTEN_DRIVER_REQUEST:
    case IOCTL_GNSS_RESPOND_NI:
    case IOCTL_GNSS_LISTEN_NI:
        //Geofence
    case IOCTL_GNSS_LISTEN_GEOFENCE_ALERT:
    case IOCTL_GNSS_LISTEN_GEOFENCES_TRACKINGSTATUS:
    case IOCTL_GNSS_CREATE_GEOFENCE:
    case IOCTL_GNSS_DELETE_GEOFENCE:
        // Breadcrumbing
    case IOCTL_GNSS_START_BREADCRUMBING:
    case IOCTL_GNSS_STOP_BREADCRUMBING:
    case IOCTL_GNSS_LISTEN_BREADCRUMBING_ALERT:
    case IOCTL_GNSS_POP_BREADCRUMBS:
        // Ioctls that are not implemented
    case IOCTL_GNSS_LISTEN_ERROR:
    case IOCTL_GNSS_EXECUTE_CWTEST:
    case IOCTL_GNSS_EXECUTE_SELFTEST:
    case IOCTL_GNSS_LISTEN_NMEA:
        status = STATUS_NOT_IMPLEMENTED;
        break;

    default:
        status = STATUS_INVALID_DEVICE_REQUEST;
        break;
    }

    if (status != STATUS_PENDING)
    {
        TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_QUEUE, "%!FUNC! Completing Request with status %!STATUS!", status);
        WdfRequestComplete(Request, status);
    }
}

NTSTATUS
CQueue::GetDeviceCapability(
    _In_ WDFREQUEST Request
)
{
    NTSTATUS status = STATUS_INVALID_DEVICE_REQUEST;
    PGNSS_DEVICE_CAPABILITY outputParam;

    status = WdfRequestRetrieveOutputBuffer(Request, sizeof(*outputParam), (void**)&outputParam, nullptr);
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_QUEUE, "%!FUNC! Could not retrieve output buffer for request with status = %!STATUS!", status);
        goto Exit;
    }

    INIT_POD_GNSS_STRUCT(outputParam);

    //
    // FIX ME:
    // The functionality below is optional. Driver developer may choose to implement for advanced functionality/better performance.
    //
    {
        outputParam->SupportContinuousTracking = FALSE;
        
        outputParam->SupportDistanceTracking = FALSE;
        
        outputParam->GeofencingSupport = FALSE;
        outputParam->MaxGeofencesSupported = 0;
        
        outputParam->RequireAGnssInjection = FALSE;
        outputParam->AgnssFormatSupported = FALSE;
        outputParam->AgnssFormatPreferred = FALSE;

        outputParam->SupportCpLocation = FALSE;
        outputParam->SupportSuplV2 = FALSE;
        outputParam->SupportSuplV1 = FALSE;
        outputParam->SupportUplV2 = FALSE;
        outputParam->SupportedSuplVersion.MajorVersion = 0;
        outputParam->SupportedSuplVersion.MinorVersion = 0;

        outputParam->MaxGnssBreadCrumbFixes = 0;
        outputParam->GnssBreadCrumbPayloadVersion = BREADCRUMBING_UNSUPPORTED;

        // Update the Driver version if forced through IOCTL
        outputParam->Version = _ForcedDriverVersion;
    }

    WdfRequestCompleteWithInformation(Request, STATUS_SUCCESS, sizeof(*outputParam));
    status = STATUS_PENDING;

Exit:
    return status;
}

NTSTATUS
CQueue::GetChipsetInfo(
    _In_ WDFREQUEST Request
)
{
    //
    // FIX ME:
    // The following is an example. Replace the values accordingly
    //
    static const wchar_t s_ManufacturerID[] = L"Microsoft Test";
    static const wchar_t s_HardwareID[] = L"UMDF Gnss";
    static const wchar_t s_FirmwareVersion[] = L"1.0.0.0";

    NTSTATUS status = STATUS_INVALID_DEVICE_REQUEST;
    PGNSS_CHIPSETINFO outputParam;

    status = WdfRequestRetrieveOutputBuffer(Request, sizeof(*outputParam), (void**)&outputParam, nullptr);
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_QUEUE, "%!FUNC! Could not retrieve output buffer for request with status = %!STATUS!", status);
        goto Exit;
    }

    INIT_POD_GNSS_STRUCT(outputParam);
        
    memset(&(outputParam->ManufacturerID), 0, sizeof(outputParam->ManufacturerID));
    memset(&(outputParam->HardwareID), 0, sizeof(outputParam->HardwareID));
    memset(&(outputParam->FirmwareVersion), 0, sizeof(outputParam->FirmwareVersion));

    memcpy_s(&(outputParam->ManufacturerID), sizeof(outputParam->ManufacturerID), &s_ManufacturerID, sizeof(s_ManufacturerID));
    memcpy_s(&(outputParam->HardwareID), sizeof(outputParam->HardwareID), &s_HardwareID, sizeof(s_HardwareID));
    memcpy_s(&(outputParam->FirmwareVersion), sizeof(outputParam->FirmwareVersion), &s_FirmwareVersion, sizeof(s_FirmwareVersion));

    WdfRequestCompleteWithInformation(Request, STATUS_SUCCESS, sizeof(*outputParam));
    status = STATUS_PENDING;

Exit:
    return status;
}

NTSTATUS
CQueue::HandleDriverCommand(
    _In_ WDFREQUEST Request
)
{
    NTSTATUS status = STATUS_INVALID_DEVICE_REQUEST;
    PGNSS_DRIVERCOMMAND_PARAM command = nullptr;

    // Check that there is enough room to output the returned GNSS_DEVICE_CAPABILITY
    status = WdfRequestRetrieveInputBuffer(Request, sizeof(*command), (void**)&command, nullptr);

    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_QUEUE, "%!FUNC! Could not retrieve input buffer for DriverCommand request with status = %!STATUS!", status);
        goto Exit;
    }

    // Check that the sizes are well-formed
    if ((command->Size < (ULONG)FIELD_OFFSET(GNSS_DRIVERCOMMAND_PARAM, CommandData[command->CommandDataSize])))
    {
        TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_QUEUE, "Command is not formatted correctly : unexpected sizes specified");
        goto Exit;
    }

    switch (command->CommandType)
    {
    case GNSS_ClearAgnssData:
        status = ClearAgnssData(command);
    break;
    case GNSS_ResetEngine:
        status = ResetEngine(command);
        break;
    case GNSS_SetLocationServiceEnabled:
        status = SetLocationServiceStatus(command);
        break;
    case GNSS_ForceOperationMode:
        status = SetOperationMode(command);
        break;

    //
    // FIX ME:
    // Below are optional driver commands. Driver developer may choose to implement for advanced functionality.
    //
    case GNSS_SetLocationNIRequestAllowed:
    case GNSS_ResetGeofencesTracking:
    case GNSS_SetNiTimeoutInterval:
    case GNSS_SetSuplVersion:
    case GNSS_SetUplServerAccessInterval:
        status = STATUS_NOT_IMPLEMENTED;
        break;
    default:
        status = STATUS_INVALID_DEVICE_REQUEST;
        break;
    }

Exit:
    return status;
}

NTSTATUS
CQueue::SetLocationServiceStatus(
    _In_ PGNSS_DRIVERCOMMAND_PARAM CommandParam
)
{
    NTSTATUS status = STATUS_SUCCESS;

    status = GetDataFromCommand(CommandParam, &_InternalState.SetLocationServiceEnabled) ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
    
    if (!_InternalState.SetLocationServiceEnabled)
    {
        //
        // FIX ME:
        // We must stop any ongoing location sessions of any kind (geofence tracking, assistance data refresh, etc. if any).
        // For example, if you have distance tracking fix session, stop the session as well.
        //
        status = _FixHandler.StopExistingSinglsShotFix();
    }

    return status;
}

NTSTATUS
CQueue::ResetEngine(
    _In_ PGNSS_DRIVERCOMMAND_PARAM /*CommandParam*/
)
{
    NTSTATUS status = STATUS_SUCCESS;

    //
    // FIX ME:
    // This is mandatory command, but sample driver cannot support Reset engine command because this is vendor/HW-specific feature.
    // You must customize this accordingly. The sample below demonstrates how it might work by resetting internal structure.
    //
    _AgnssNeeded[AGNSS_TIME_IDX] = TRUE;
    _AgnssNeeded[AGNSS_POS_IDX] = TRUE;
    _AgnssNeeded[AGNSS_BLOB_IDX] = TRUE;

    return status;
}

NTSTATUS
CQueue::ClearAgnssData(
    _In_ PGNSS_DRIVERCOMMAND_PARAM CommandParam
)
{
    NTSTATUS status = STATUS_SUCCESS;
    DWORD data = 0;
    
    //
    // FIX ME:
    // This is mandatory command, but sample driver cannot support Clear AGNSS data because this is vendor/HW-specific feature.
    // You must customize this accordingly. The sample below demonstrates how it might work by resetting internal structure.
    //
    if (GetDataFromCommand(CommandParam, &data))
    {
        GNSS_AGNSS_REQUEST_TYPE reqType = (GNSS_AGNSS_REQUEST_TYPE)data;
        _AgnssNeeded[reqType] = TRUE;
    }
    else
    {
        status = STATUS_UNSUCCESSFUL;
    }

    return status;
}

NTSTATUS
CQueue::SetOperationMode(
    _In_ PGNSS_DRIVERCOMMAND_PARAM CommandParam
)
{
    NTSTATUS status = STATUS_SUCCESS;
 
    //
    // FIX ME:
    // This is mandatory only if SUPL is supported. SUPL is not mandatory unless it is required by mobile operator.
    // You must customize this code accordingly, depending on the requirement.
    //
    status = GetDataFromCommand(CommandParam, &_InternalState.OperationMode) ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;

    return status;
}

template <class DataT>
BOOL CQueue::GetDataFromCommand(
    _In_ PGNSS_DRIVERCOMMAND_PARAM Command,
    _Out_ DataT* Data)
{
    size_t dataLen = sizeof(*Data);

    memset(Data, 0, dataLen);

    if (!Command || (Command->CommandDataSize != dataLen))
    {
        return FALSE;
    }

    memcpy_s(Data, dataLen, Command->CommandData, dataLen);
    return TRUE;
}
