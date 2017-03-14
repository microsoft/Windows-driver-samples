/*++

Module Name:

    Alert.c

Abstract:

    This file contains functions that handle alerts from the port controller hardware.

Environment:

    Kernel-mode Driver Framework

--*/

#include "Driver.h"
#include "alert.tmh"

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, OnInterruptPassiveIsr)
#pragma alloc_text (PAGE, ProcessAndSendAlerts)
#endif

BOOLEAN
OnInterruptPassiveIsr(
    _In_ WDFINTERRUPT PortControllerInterrupt,
    _In_ ULONG MessageID
)
/*++

Routine Description:

    Per the TCPCI spec, the port controller hardware will drive the Alert pin high
    when a hardware event occurs. This routine services such a hardware interrupt at PASSIVE_LEVEL.
    The routine determines if an interrupt is an alert from the port controller hardware;
    if so, it completes processing of the alert.

Arguments:

    Interrupt: A handle to a framework interrupt object.

    MessageID: If the device is using message-signaled interrupts (MSIs), this parameter
    is the message number that identifies the device's hardware interrupt message.
    Otherwise, this value is 0.

Return Value:

    TRUE if the function services the hardware interrupt.
    Otherwise, this function must return FALSE.

--*/
{
    TRACE_FUNC_ENTRY(TRACE_ALERT);

    UNREFERENCED_PARAMETER(MessageID);
    PAGED_CODE();

    NTSTATUS status;
    PDEVICE_CONTEXT deviceContext;
    ALERT_REGISTER alertRegister;
    BOOLEAN interruptRecognized = FALSE;
    int numAlertsProcessed = 0;
    deviceContext = DeviceGetContext(WdfInterruptGetDevice(PortControllerInterrupt));

    // Process the alerts as long as there are bits set in the alert register.
    // Set a maximum number of alerts to process in this loop. If the hardware is messed up and we're unable
    // to quiesce the interrupt by writing to the alert register, then we don't want to be stuck in an
    // infinite loop.
    while (numAlertsProcessed <= MAX_ALERTS_TO_PROCESS)
    {
        status = I2CReadSynchronously(deviceContext,
            I2CRequestSourceAlertIsr,
            ALERT,
            &alertRegister,
            sizeof(alertRegister));
        if (!NT_SUCCESS(status))
        {
            goto Exit;
        }
        // If there are no bits set in the alert register, we should not service this interrupt.
        if (alertRegister.AsUInt16 == 0)
        {
            goto Exit;
        }

        // Since there are bits set in the alert register, we can safely assume that the
        // interrupt is ours to process.
        interruptRecognized = TRUE;

        ProcessAndSendAlerts(&alertRegister, deviceContext);
        ++numAlertsProcessed;
    }

Exit:
    TRACE_FUNC_EXIT(TRACE_ALERT);
    return interruptRecognized;
}

void
ProcessAndSendAlerts(
    _In_ PALERT_REGISTER AlertRegister,
    _In_ PDEVICE_CONTEXT DeviceContext
)
/*++

Routine Description:

    Processes the set of hardware alerts that were reported and notifies UcmTcpciCx of the alerts
    along with the contents of relevant registers.

Arguments:

    AlertRegister: Pointer to alert register contents.

    DeviceContext: Device's context space.

--*/
{
    TRACE_FUNC_ENTRY(TRACE_ALERT);
    PAGED_CODE();

    NTSTATUS status;
    size_t numAlerts = 0;
    UCMTCPCI_PORT_CONTROLLER_ALERT_DATA alertData;
    UCMTCPCI_PORT_CONTROLLER_CC_STATUS ccStatus;
    UCMTCPCI_PORT_CONTROLLER_POWER_STATUS powerStatus;
    UCMTCPCI_PORT_CONTROLLER_FAULT_STATUS faultStatus;
    UCMTCPCI_PORT_CONTROLLER_RECEIVE_BUFFER receiveBuffer;

    // UcmTcpciCx expects the information on all of the alerts firing presently.
    UCMTCPCI_PORT_CONTROLLER_ALERT_DATA hardwareAlerts[MAX_ALERTS];

    status = STATUS_SUCCESS;

    if (AlertRegister->CCStatus == 1)
    {
        UCMTCPCI_PORT_CONTROLLER_ALERT_DATA_INIT(&alertData);
        alertData.AlertType = UcmTcpciPortControllerAlertCCStatus;

        // We must read the CC status register and send the contents to
        // UcmTcpciCx along with the CC status alert.
        status = I2CReadSynchronously(DeviceContext,
            I2CRequestSourceAlertIsr,
            CC_STATUS,
            &ccStatus,
            sizeof(ccStatus));
        if (!NT_SUCCESS(status))
        {
            goto Exit;
        }

        alertData.CCStatus = ccStatus;
        hardwareAlerts[numAlerts] = alertData;
        ++numAlerts;
    }

    if (AlertRegister->PowerStatus == 1)
    {
        UCMTCPCI_PORT_CONTROLLER_ALERT_DATA_INIT(&alertData);
        alertData.AlertType = UcmTcpciPortControllerAlertPowerStatus;

        // We must read the power status register and send the contents to
        // UcmTcpciCx along with the power status alert.
        status = I2CReadSynchronously(DeviceContext,
            I2CRequestSourceAlertIsr,
            POWER_STATUS,
            &powerStatus,
            sizeof(powerStatus));
        if (!NT_SUCCESS(status))
        {
            goto Exit;
        }

        alertData.PowerStatus = powerStatus;
        hardwareAlerts[numAlerts] = alertData;
        ++numAlerts;
    }

    if (AlertRegister->Fault == 1)
    {
        UCMTCPCI_PORT_CONTROLLER_ALERT_DATA_INIT(&alertData);
        alertData.AlertType = UcmTcpciPortControllerAlertFault;

        // We must read the fault status register and send the contents to
        // UcmTcpciCx along with the fault alert.
        status = I2CReadSynchronously(DeviceContext,
            I2CRequestSourceAlertIsr,
            FAULT_STATUS,
            &faultStatus,
            sizeof(faultStatus));
        if (!NT_SUCCESS(status))
        {
            goto Exit;
        }

        alertData.FaultStatus = faultStatus;
        hardwareAlerts[numAlerts] = alertData;
        ++numAlerts;

        // Clear FAULT_STATUS Register.

        // Mask reserved bit 7 in TCPCI Rev 1.0 Ver 1.0 only, see spec section 4.4.6.3
        faultStatus.AsUInt8 &= 0x7F;

        status = I2CWriteSynchronously(DeviceContext,
            I2CRequestSourceAlertIsr,
            FAULT_STATUS,
            &faultStatus,
            sizeof(faultStatus));
        if (!NT_SUCCESS(status))
        {
            goto Exit;
        }
    }

    if (AlertRegister->ReceiveSOPMessageStatus == 1)
    {
        UCMTCPCI_PORT_CONTROLLER_ALERT_DATA_INIT(&alertData);
        alertData.AlertType = UcmTcpciPortControllerAlertReceiveSOPMessageStatus;

        // We must read the receive buffer register and send the contents to
        // UcmTcpciCx along with the receive SOP alert.
        status = I2CReadSynchronously(DeviceContext,
            I2CRequestSourceAlertIsr,
            RECEIVE_BUFFER,
            &receiveBuffer,
            sizeof(receiveBuffer));
        if (!NT_SUCCESS(status))
        {
            goto Exit;
        }

        alertData.ReceiveBuffer = &receiveBuffer;
        hardwareAlerts[numAlerts] = alertData;
        ++numAlerts;
    }

    // The remainder of the alert types do not require us to provide any extra
    // information to UcmTcpciCx.
    if (AlertRegister->ReceivedHardReset == 1)
    {
        UCMTCPCI_PORT_CONTROLLER_ALERT_DATA_INIT(&alertData);
        alertData.AlertType = UcmTcpciPortControllerAlertReceivedHardReset;
        hardwareAlerts[numAlerts] = alertData;
        ++numAlerts;
    }

    if (AlertRegister->RxBufferOverflow == 1)
    {
        UCMTCPCI_PORT_CONTROLLER_ALERT_DATA_INIT(&alertData);
        alertData.AlertType = UcmTcpciPortControllerAlertRxBufferOverflow;
        hardwareAlerts[numAlerts] = alertData;
        ++numAlerts;
    }

    if (AlertRegister->TransmitSOPMessageDiscarded == 1)
    {
        UCMTCPCI_PORT_CONTROLLER_ALERT_DATA_INIT(&alertData);
        alertData.AlertType = UcmTcpciPortControllerAlertTransmitSOPMessageDiscarded;
        hardwareAlerts[numAlerts] = alertData;
        ++numAlerts;
    }

    if (AlertRegister->TransmitSOPMessageFailed == 1)
    {
        UCMTCPCI_PORT_CONTROLLER_ALERT_DATA_INIT(&alertData);
        alertData.AlertType = UcmTcpciPortControllerAlertTransmitSOPMessageFailed;
        hardwareAlerts[numAlerts] = alertData;
        ++numAlerts;
    }

    if (AlertRegister->TransmitSOPMessageSuccessful == 1)
    {
        UCMTCPCI_PORT_CONTROLLER_ALERT_DATA_INIT(&alertData);
        alertData.AlertType = UcmTcpciPortControllerAlertTransmitSOPMessageSuccessful;
        hardwareAlerts[numAlerts] = alertData;
        ++numAlerts;
    }

    if (AlertRegister->VbusSinkDisconnectDetected == 1)
    {
        UCMTCPCI_PORT_CONTROLLER_ALERT_DATA_INIT(&alertData);
        alertData.AlertType = UcmTcpciPortControllerAlertVbusSinkDisconnectDetected;
        hardwareAlerts[numAlerts] = alertData;
        ++numAlerts;
    }

    if (AlertRegister->VbusVoltageAlarmHi == 1)
    {
        UCMTCPCI_PORT_CONTROLLER_ALERT_DATA_INIT(&alertData);
        alertData.AlertType = UcmTcpciPortControllerAlertVbusVoltageAlarmHi;
        hardwareAlerts[numAlerts] = alertData;
        ++numAlerts;
    }

    if (AlertRegister->VbusVoltageAlarmLo == 1)
    {
        UCMTCPCI_PORT_CONTROLLER_ALERT_DATA_INIT(&alertData);
        alertData.AlertType = UcmTcpciPortControllerAlertVbusVoltageAlarmLo;
        hardwareAlerts[numAlerts] = alertData;
        ++numAlerts;
    }

    // Only write back non-reserved bits see spec section 4.4.2
    // TCPCI Rev 1.0 Ver 1.0: 0x0FFF
    // TCPCI Rev 1.0 Ver 1.1: 0x8FFF
    AlertRegister->AsUInt16 &= 0x0FFF;

    // Quiesce the interrupt by writing back the alert register.
    // Per TCPCI spec 4.4.2, the alert is cleared by writing a 1 back to the bit position it is to clear.
    status = I2CWriteSynchronously(DeviceContext,
        I2CRequestSourceAlertIsr,
        ALERT,
        AlertRegister,
        sizeof(*AlertRegister));
    if (!NT_SUCCESS(status))
    {
        goto Exit;
    }

Exit:
    if (NT_SUCCESS(status))
    {
        // Send the list of hardware alerts to UcmTcpciCx.
        UcmTcpciPortControllerAlert(DeviceContext->PortController, hardwareAlerts, numAlerts);
    }

    TRACE_FUNC_EXIT(TRACE_ALERT);
}