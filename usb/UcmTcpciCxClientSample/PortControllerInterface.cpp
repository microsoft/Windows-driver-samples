/*++

Module Name:

    PortControllerInterface.c

Abstract:

    This file contains the definitions of functions to read to and write from the
    Type-C port controller hardware registers.

Environment:

    Kernel-mode Driver Framework

--*/

#include "Driver.h"
#include "portcontrollerinterface.tmh"

void
PostponeToWorkitem(
    _In_ WDFREQUEST Request,
    _In_ WDFDEVICE Device,
    _In_ WDFWORKITEM WorkItem
)
/*++

Routine Description:

    Because EvtIoDeviceControl was called at dispatch-level, it cannot send I/O and then wait synchronously;
    instead, the work has to be postponed to a passive-level workitem.

Arguments:

    Request - Handle to a framework request object.

    Device - Handle to a framework device object.

    WorkItem - Handle to a framework workitem object

--*/
{
    TRACE_FUNC_ENTRY(TRACE_PORTCONTROLLERINTERFACE);
    PWORKITEM_CONTEXT workItemContext;

    workItemContext = WorkitemGetContext(WorkItem);
    workItemContext->Device = Device;
    workItemContext->Request = Request;

    WdfWorkItemEnqueue(WorkItem);

    TRACE_FUNC_EXIT(TRACE_PORTCONTROLLERINTERFACE);
}

void
EvtWorkItemGetStatus(
    _In_ WDFWORKITEM WorkItem
)
/*++

Routine Description:

    This routine handles IOCTL_UCMTCPCI_PORT_CONTROLLER_GET_STATUS.

    Read the contents of the CC Status, Fault Status, and Power Status registers from the device
    and complete the WDFREQUEST.

Arguments:

    WorkItem - Handle to a framework workitem object.

--*/
{
    TRACE_FUNC_ENTRY(TRACE_PORTCONTROLLERINTERFACE);

    PUCMTCPCI_PORT_CONTROLLER_GET_STATUS_OUT_PARAMS outParams;
    PWORKITEM_CONTEXT workItemContext;
    PDEVICE_CONTEXT deviceContext;
    WDFREQUEST request;
    NTSTATUS status;

    workItemContext = WorkitemGetContext(WorkItem);
    deviceContext = DeviceGetContext(workItemContext->Device);
    request = workItemContext->Request;

    status = WdfRequestRetrieveOutputBuffer(request,
        sizeof(*outParams),
        reinterpret_cast<PVOID*>(&outParams),
        NULL);
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_PORTCONTROLLERINTERFACE,
            "WdfRequestRetrieveOutputBuffer failed. Status: %!STATUS!", status);
        goto Exit;
    }

    REGISTER_ITEM items[] = {
        GEN_REGISTER_ITEM(CC_STATUS,    outParams->CCStatus),
        GEN_REGISTER_ITEM(POWER_STATUS, outParams->PowerStatus),
        GEN_REGISTER_ITEM(FAULT_STATUS, outParams->FaultStatus),
    };

    status = I2CReadSynchronouslyMultiple(deviceContext,
        I2CRequestSourceClient,
        items,
        _countof(items));
    if (!NT_SUCCESS(status))
    {
        goto Exit;
    }

    WdfRequestSetInformation(request, sizeof(*outParams));

Exit:
    WdfRequestComplete(request, status);

    TRACE_FUNC_EXIT(TRACE_PORTCONTROLLERINTERFACE);
}

void
EvtWorkItemGetControl(
    _In_ WDFWORKITEM WorkItem
)
/*++

Routine Description:

    This routine handles IOCTL_UCMTCPCI_PORT_CONTROLLER_GET_CONTROL.

    Read the contents of the TCPC Control, Role Control, Fault Control, and Power Control registers from the device
    and complete the WDFREQUEST.

Arguments:

    WorkItem - Handle to a framework workitem object.

--*/
{
    TRACE_FUNC_ENTRY(TRACE_PORTCONTROLLERINTERFACE);

    PUCMTCPCI_PORT_CONTROLLER_GET_CONTROL_OUT_PARAMS outParams;
    PWORKITEM_CONTEXT workItemContext;
    PDEVICE_CONTEXT deviceContext;
    WDFREQUEST request;
    NTSTATUS status;

    workItemContext = WorkitemGetContext(WorkItem);
    deviceContext = DeviceGetContext(workItemContext->Device);
    request = workItemContext->Request;

    status = WdfRequestRetrieveOutputBuffer(request,
        sizeof(*outParams),
        reinterpret_cast<PVOID*>(&outParams),
        NULL);
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_PORTCONTROLLERINTERFACE,
            "WdfRequestRetrieveOutputBuffer failed. Status: %!STATUS!", status);
        goto Exit;
    }

    REGISTER_ITEM items[] = {
        GEN_REGISTER_ITEM(TCPC_CONTROL,     outParams->TCPCControl),
        GEN_REGISTER_ITEM(ROLE_CONTROL,     outParams->RoleControl),
        GEN_REGISTER_ITEM(FAULT_CONTROL,    outParams->FaultControl),
        GEN_REGISTER_ITEM(POWER_CONTROL,    outParams->PowerControl),
    };

    status = I2CReadSynchronouslyMultiple(deviceContext,
        I2CRequestSourceClient,
        items,
        _countof(items));
    if (!NT_SUCCESS(status))
    {
        goto Exit;
    }

    WdfRequestSetInformation(request, sizeof(*outParams));

Exit:
    WdfRequestComplete(request, status);

    TRACE_FUNC_EXIT(TRACE_PORTCONTROLLERINTERFACE);
}

void
EvtSetControl(
    _In_ WDFREQUEST Request,
    _In_ WDFDEVICE Device
)
/*++

Routine Description:

    Set the contents of the TCPC Control, Role Control, Fault Control, or Power Control
    registers on the device and complete the WDFREQUEST.

Arguments:

    Request - Handle to a framework request object.

    Device - Handle to a framework device object.

--*/
{
    TRACE_FUNC_ENTRY(TRACE_PORTCONTROLLERINTERFACE);

    NTSTATUS status;
    PDEVICE_CONTEXT deviceContext;
    PUCMTCPCI_PORT_CONTROLLER_SET_CONTROL_IN_PARAMS inParams;
    UCMTCPCI_PORT_CONTROLLER_CONTROL_TYPE controlType;

    deviceContext = DeviceGetContext(Device);

    status = WdfRequestRetrieveInputBuffer(Request,
        sizeof(UCMTCPCI_PORT_CONTROLLER_SET_COMMAND_IN_PARAMS),
        reinterpret_cast<PVOID*>(&inParams),
        NULL);
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_PORTCONTROLLERINTERFACE,
            "WdfRequestRetrieveInputBuffer failed. Status: %!STATUS!", status);
        goto Exit;
    }

    controlType = inParams->ControlType;

    switch (controlType)
    {
    case UcmTcpciPortControllerTcpcControl:
    {
        status = I2CWriteAsynchronously(deviceContext,
            TCPC_CONTROL,
            &inParams->TCPCControl,
            sizeof(inParams->TCPCControl));
        if (!NT_SUCCESS(status))
        {
            goto Exit;
        }

        break;
    }
    case UcmTcpciPortControllerRoleControl:
    {
        status = I2CWriteAsynchronously(deviceContext,
            ROLE_CONTROL,
            &inParams->RoleControl,
            sizeof(inParams->RoleControl));
        if (!NT_SUCCESS(status))
        {
            goto Exit;
        }

        break;
    }
    case UcmTcpciPortControllerFaultControl:
    {
        status = I2CWriteAsynchronously(deviceContext,
            FAULT_CONTROL,
            &inParams->FaultControl,
            sizeof(inParams->FaultControl));
        if (!NT_SUCCESS(status))
        {
            goto Exit;
        }

        break;
    }
    case UcmTcpciPortControllerPowerControl:
    {
        status = I2CWriteAsynchronously(deviceContext,
            POWER_CONTROL,
            &inParams->PowerControl,
            sizeof(inParams->PowerControl));
        if (!NT_SUCCESS(status))
        {
            goto Exit;
        }

        break;
    }
    default:
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_PORTCONTROLLERINTERFACE, "Invalid control register type.");
        status = STATUS_INVALID_DEVICE_REQUEST;
        goto Exit;
    }

Exit:
    TRACE_FUNC_EXIT(TRACE_PORTCONTROLLERINTERFACE);
}

void
EvtSetCommand(
    _In_ WDFREQUEST Request,
    _In_ WDFDEVICE Device
)
/*++

Routine Description:

    Set the contents of the command register on the device and complete the WDFREQUEST.

Arguments:

    Request - Handle to a framework request object.

    Device - Handle to a framework device object.

--*/
{
    TRACE_FUNC_ENTRY(TRACE_PORTCONTROLLERINTERFACE);

    NTSTATUS status;
    PDEVICE_CONTEXT deviceContext;
    PUCMTCPCI_PORT_CONTROLLER_SET_COMMAND_IN_PARAMS inParams;
    UINT8 cmd;

    deviceContext = DeviceGetContext(Device);

    status = WdfRequestRetrieveInputBuffer(Request,
        sizeof(UCMTCPCI_PORT_CONTROLLER_SET_COMMAND_IN_PARAMS),
        reinterpret_cast<PVOID*>(&inParams),
        NULL);
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_PORTCONTROLLERINTERFACE,
            "WdfRequestRetrieveInputBuffer failed. Status: %!STATUS!", status);
        goto Exit;
    }

    // UCMTCPCI_PORT_CONTROLLER_COMMAND (from inParams->Command) is defined as an enum.
    // Thus sizeof() typically returns 4 bytes, instead of 1 byte as required by TCPCI spec.
    // The workaround is to copy it to a local 1-byte variable before writing it down.
    cmd = static_cast<UINT8>(inParams->Command);

    status = I2CWriteAsynchronously(deviceContext,
        COMMAND,
        &cmd,
        sizeof(cmd));
    if (!NT_SUCCESS(status))
    {
        goto Exit;
    }

Exit:
    TRACE_FUNC_EXIT(TRACE_PORTCONTROLLERINTERFACE);
}

void
EvtSetConfigStandardOutput(
    _In_ WDFREQUEST Request,
    _In_ WDFDEVICE Device
)
/*++

Routine Description:

    Set the contents of the config standard output register on the
    device and complete the WDFREQUEST.

Arguments:

    Request - Handle to a framework request object.

    Device - Handle to a framework device object.

--*/
{
    TRACE_FUNC_ENTRY(TRACE_PORTCONTROLLERINTERFACE);

    NTSTATUS status;
    PDEVICE_CONTEXT deviceContext;
    PUCMTCPCI_PORT_CONTROLLER_SET_CONFIG_STANDARD_OUTPUT_IN_PARAMS inParams;

    deviceContext = DeviceGetContext(Device);

    status = WdfRequestRetrieveInputBuffer(Request,
        sizeof(UCMTCPCI_PORT_CONTROLLER_SET_CONFIG_STANDARD_OUTPUT_IN_PARAMS),
        reinterpret_cast<PVOID*>(&inParams),
        NULL);
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_PORTCONTROLLERINTERFACE,
            "WdfRequestRetrieveInputBuffer failed. Status: %!STATUS!", status);
        goto Exit;
    }

    status = I2CWriteAsynchronously(deviceContext,
        CONFIG_STANDARD_OUTPUT,
        &inParams->ConfigStandardOutput,
        sizeof(inParams->ConfigStandardOutput));
    if (!NT_SUCCESS(status))
    {
        goto Exit;
    }

Exit:
    TRACE_FUNC_EXIT(TRACE_PORTCONTROLLERINTERFACE);
}

void
EvtSetMessageHeaderInfo(
    _In_ WDFREQUEST Request,
    _In_ WDFDEVICE Device
)
/*++

Routine Description:

    Set the contents of the message header info register on the device and complete the WDFREQUEST.

Arguments:

    Request - Handle to a framework request object.

    Device - Handle to a framework device object.

--*/
{
    TRACE_FUNC_ENTRY(TRACE_PORTCONTROLLERINTERFACE);

    NTSTATUS status;
    PDEVICE_CONTEXT deviceContext;
    PUCMTCPCI_PORT_CONTROLLER_SET_MESSAGE_HEADER_INFO_IN_PARAMS inParams;

    deviceContext = DeviceGetContext(Device);

    status = WdfRequestRetrieveInputBuffer(Request,
        sizeof(UCMTCPCI_PORT_CONTROLLER_SET_MESSAGE_HEADER_INFO_IN_PARAMS),
        reinterpret_cast<PVOID*>(&inParams),
        NULL);
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_PORTCONTROLLERINTERFACE,
            "WdfRequestRetrieveInputBuffer failed. Status: %!STATUS!", status);
        goto Exit;
    }

    status = I2CWriteAsynchronously(deviceContext,
        MESSAGE_HEADER_INFO,
        &inParams->MessageHeaderInfo,
        sizeof(inParams->MessageHeaderInfo));
    if (!NT_SUCCESS(status))
    {
        goto Exit;
    }

Exit:
    TRACE_FUNC_EXIT(TRACE_PORTCONTROLLERINTERFACE);
}

void
EvtSetReceiveDetect(
    _In_ WDFREQUEST Request,
    _In_ WDFDEVICE Device
)
/*++

Routine Description:

    Set the contents of the receive detect register on the device and complete the WDFREQUEST.

Arguments:

    Request - Handle to a framework request object.

    Device - Handle to a framework device object.

--*/
{
    TRACE_FUNC_ENTRY(TRACE_PORTCONTROLLERINTERFACE);

    NTSTATUS status;
    PDEVICE_CONTEXT deviceContext;
    PUCMTCPCI_PORT_CONTROLLER_SET_RECEIVE_DETECT_IN_PARAMS inParams;

    deviceContext = DeviceGetContext(Device);

    status = WdfRequestRetrieveInputBuffer(Request,
        sizeof(UCMTCPCI_PORT_CONTROLLER_SET_RECEIVE_DETECT_IN_PARAMS),
        reinterpret_cast<PVOID*>(&inParams),
        NULL);
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_PORTCONTROLLERINTERFACE,
            "WdfRequestRetrieveInputBuffer failed. Status: %!STATUS!", status);
        goto Exit;
    }

    status = I2CWriteAsynchronously(deviceContext,
        RECEIVE_DETECT,
        &inParams->ReceiveDetect,
        sizeof(inParams->ReceiveDetect));
    if (!NT_SUCCESS(status))
    {
        goto Exit;
    }

Exit:
    TRACE_FUNC_EXIT(TRACE_PORTCONTROLLERINTERFACE);
}

void
EvtSetTransmit(
    _In_ WDFREQUEST Request,
    _In_ WDFDEVICE Device
)
/*++

Routine Description:

    Set the contents of the transmit register on the device and complete the WDFREQUEST.

Arguments:

    Request - Handle to a framework request object.

    Device - Handle to a framework device object.

--*/
{
    TRACE_FUNC_ENTRY(TRACE_PORTCONTROLLERINTERFACE);

    NTSTATUS status;
    PDEVICE_CONTEXT deviceContext;
    PUCMTCPCI_PORT_CONTROLLER_SET_TRANSMIT_IN_PARAMS inParams;

    deviceContext = DeviceGetContext(Device);

    status = WdfRequestRetrieveInputBuffer(Request,
        sizeof(PUCMTCPCI_PORT_CONTROLLER_SET_TRANSMIT_IN_PARAMS),
        reinterpret_cast<PVOID*>(&inParams),
        NULL);
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_PORTCONTROLLERINTERFACE,
            "WdfRequestRetrieveInputBuffer failed. Status: %!STATUS!", status);
        goto Exit;
    }

    status = I2CWriteAsynchronously(deviceContext,
        TRANSMIT,
        &inParams->Transmit,
        sizeof(inParams->Transmit));
    if (!NT_SUCCESS(status))
    {
        goto Exit;
    }

Exit:
    TRACE_FUNC_EXIT(TRACE_PORTCONTROLLERINTERFACE);
}

void
EvtSetTransmitBuffer(
    _In_ WDFREQUEST Request,
    _In_ WDFDEVICE Device
)
/*++

Routine Description:

    Set the contents of the transmit buffer register on the device and complete the WDFREQUEST.

Arguments:

    Request - Handle to a framework request object.

    Device - Handle to a framework device object.

--*/
{
    TRACE_FUNC_ENTRY(TRACE_PORTCONTROLLERINTERFACE);

    NTSTATUS status;
    PDEVICE_CONTEXT deviceContext;
    PUCMTCPCI_PORT_CONTROLLER_SET_TRANSMIT_BUFFER_IN_PARAMS inParams;

    deviceContext = DeviceGetContext(Device);

    status = WdfRequestRetrieveInputBuffer(Request,
        sizeof(UCMTCPCI_PORT_CONTROLLER_SET_TRANSMIT_BUFFER_IN_PARAMS),
        reinterpret_cast<PVOID*>(&inParams),
        NULL);
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_PORTCONTROLLERINTERFACE,
            "WdfRequestRetrieveInputBuffer failed. Status: %!STATUS!", status);
        goto Exit;
    }

    status = I2CWriteAsynchronously(deviceContext,
        TRANSMIT_BUFFER,
        &inParams->TransmitBuffer,
        inParams->TransmitBuffer.TransmitByteCount + sizeof(inParams->TransmitBuffer.TransmitByteCount));
    if (!NT_SUCCESS(status))
    {
        goto Exit;
    }

Exit:
    TRACE_FUNC_EXIT(TRACE_PORTCONTROLLERINTERFACE);
}