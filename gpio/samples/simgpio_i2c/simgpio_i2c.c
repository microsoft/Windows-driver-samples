
/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    simgpio_i2c.c

Abstract:

    This sample implements a GPIO client driver for simulated GPIO (SimGpio)
    controller.

    Note: DIRQL in the comments below refers to device IRQL, which is any
        IRQL > DISPATCH_LEVEL (and less than some IRQL reserved for OS use).


Environment:

    Kernel mode

--*/

//
// The SimGpio_I2C controller has 2 GPIO banks, with banks consisting of
// 8 pins each. Any pin can be configured for interrupt, input or output.
// Each bank has the the following registers. There are two sets of registers:
// the first set for pins 0...7 and second for pins 8...15.
//
//      Register         |   Address     |   Offset      |   Size
// ----------------------------------------------------------------
//
//          (Register set for pins 0...7)
//
// ModeRegister1         |    0x0        |   0x0         |  1 byte
// PolarityRegister1     |    0x1        |   0x1         |  1 byte
// EnableRegister1       |    0x2        |   0x2         |  1 byte
// StatusRegister1       |    0x3        |   0x3         |  1 byte
// DirectionRegister1    |    0x4        |   0x4         |  1 byte
// LevelRegister1        |    0x5        |   0x5         |  1 byte
//
//          (Register set for pins 8...15)
//
// ModeRegister2         |    0x6        |   0x6         |  1 byte
// PolarityRegister2     |    0x7        |   0x7         |  1 byte
// EnableRegister2       |    0x8        |   0x8         |  1 byte
// StatusRegister2       |    0x9        |   0x9         |  1 byte
// DirectionRegister2    |    0xA        |   0xA         |  1 byte
// LevelRegister2        |    0xB        |   0xB         |  1 byte
//
// Mode Register - Interrupt mode (Level = 0x1 or Edge = 0x0) for pins
//      enabled for interrupts.
//
// Polarity Register - ActiveLow (0x0) or ActiveHigh (0x1) for Level-triggered
//      interrupts; Falling-edge (0x0), Rising-edge (0x1). Note this controller
//      does not support ActiveBoth.
//
// Enable Register - A mask of which pins are enabled for interrupt. A bit
//      is set if the pin is enabled for interrupt.
//
// Status Register - A register containing which pins are actively interrupting.
//      If any of the bit is set, then the GPIO controller's interrupt fires.
//
// Direction Register - A register which indicates whether a given pin is
//      configured for input (0x1) or output (0x0).
//
// LevelRegister - A bitmask which indicates holds the value for a given pin
//      (i.e. whether the pin is high (0x1) or low (0x0)).
//

//
// ------------------------------------------------------------------- Includes
//

#include <ntddk.h>
#include <wdf.h>
#include <gpioclx.h>
#include "simgpio_i2c.h"
#include "trace.h"
#include "simgpio_i2c.tmh"

//
// -------------------------------------------------------------------- Defines
//

//
// Determine whether the given pin is reserved or not. Currently no pins are
// reserved on the simulated GPIO controller.
//

__pragma(warning(disable: 4127))        // conditional expression is a constant

//
// ----------------------------------------------------------------- Prototypes
//

DRIVER_INITIALIZE DriverEntry;

EVT_WDF_DRIVER_DEVICE_ADD SimGpioEvtDeviceAdd;
EVT_WDF_DRIVER_UNLOAD SimGpioEvtDriverUnload;

//
// General interfaces.
//

GPIO_CLIENT_PREPARE_CONTROLLER SimGpioPrepareController;
GPIO_CLIENT_RELEASE_CONTROLLER SimGpioReleaseController;
GPIO_CLIENT_QUERY_CONTROLLER_BASIC_INFORMATION
    SimGpioQueryControllerBasicInformation;

GPIO_CLIENT_START_CONTROLLER SimGpioStartController;
GPIO_CLIENT_STOP_CONTROLLER SimGpioStopController;

//
// Interrupt enable, disable, mask and unmask handlers.
//

GPIO_CLIENT_ENABLE_INTERRUPT SimGpioEnableInterrupt;
GPIO_CLIENT_DISABLE_INTERRUPT SimGpioDisableInterrupt;
GPIO_CLIENT_MASK_INTERRUPTS SimGpioMaskInterrupts;
GPIO_CLIENT_UNMASK_INTERRUPT SimGpioUnmaskInterrupt;
GPIO_CLIENT_RECONFIGURE_INTERRUPT SimGpioReconfigureInterrupt;

//
// Handlers to query active/enabled interrupts and clear active interrupts.
//

GPIO_CLIENT_QUERY_ACTIVE_INTERRUPTS SimGpioQueryActiveInterrupts;
GPIO_CLIENT_CLEAR_ACTIVE_INTERRUPTS SimGpioClearActiveInterrupts;
GPIO_CLIENT_QUERY_ENABLED_INTERRUPTS SimGpioQueryEnabledInterrupts;

//
// Handlers for GPIO I/O operations.
//

GPIO_CLIENT_CONNECT_IO_PINS SimGpioConnectIoPins;
GPIO_CLIENT_DISCONNECT_IO_PINS SimGpioDisconnectIoPins;
GPIO_CLIENT_READ_PINS_MASK SimGpioReadGpioPins;
GPIO_CLIENT_WRITE_PINS_MASK SimGpioWriteGpioPins;

//
// -------------------------------------------------------------------- Pragmas
//

#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE, SimGpioEvtDeviceAdd)
#pragma alloc_text(PAGE, SimGpioEvtDriverUnload)

//
// ------------------------------------------------------------------ Functions
//

NTSTATUS
DriverEntry (
    _In_ PDRIVER_OBJECT  DriverObject,
    _In_ PUNICODE_STRING RegistryPath
    )

/*++

Routine Description:

    This routine is the driver initialization entry point.

Arguments:

    DriverObject - Pointer to the driver object created by the I/O manager.

    RegistryPath - Pointer to the driver specific registry key.

Return Value:

    NTSTATUS code.

--*/

{

    WDFDRIVER Driver;
    WDF_DRIVER_CONFIG DriverConfig;
    GPIO_CLIENT_REGISTRATION_PACKET RegistrationPacket;
    NTSTATUS Status;

    WPP_INIT_TRACING(DriverObject, RegistryPath);

    //
    // Initialize the driver configuration structure.
    //

    WDF_DRIVER_CONFIG_INIT(&DriverConfig, SimGpioEvtDeviceAdd);
    DriverConfig.EvtDriverUnload = SimGpioEvtDriverUnload;

    //
    // Create a framework driver object to represent our driver.
    //

    Status = WdfDriverCreate(DriverObject,
                             RegistryPath,
                             WDF_NO_OBJECT_ATTRIBUTES,
                             &DriverConfig,
                             &Driver);

    if (!NT_SUCCESS(Status)) {
        TraceEvents(
                TRACE_LEVEL_ERROR,
                TRACE_FLAG_INIT,
                "%s: WdfDriverCreate failed status:%#x\n",
                __FUNCTION__,
                Status);

        WPP_CLEANUP(DriverObject);
        goto DriverEntryEnd;
    }

    //
    // Initialize the client driver registration packet.
    //

    RtlZeroMemory(&RegistrationPacket, sizeof(GPIO_CLIENT_REGISTRATION_PACKET));
    RegistrationPacket.Version = GPIO_CLIENT_VERSION;
    RegistrationPacket.Size = sizeof(GPIO_CLIENT_REGISTRATION_PACKET);

    //
    // Initialize the device context size.
    //

    RegistrationPacket.ControllerContextSize = sizeof(SIM_GPIO_CONTEXT);

    //
    // General interfaces.
    //

    RegistrationPacket.CLIENT_PrepareController = SimGpioPrepareController;
    RegistrationPacket.CLIENT_QueryControllerBasicInformation =
        SimGpioQueryControllerBasicInformation;

    RegistrationPacket.CLIENT_StartController = SimGpioStartController;
    RegistrationPacket.CLIENT_StopController = SimGpioStopController;
    RegistrationPacket.CLIENT_ReleaseController = SimGpioReleaseController;

    //
    // Interrupt enable and disable handlers.
    //

    RegistrationPacket.CLIENT_DisableInterrupt = SimGpioDisableInterrupt;
    RegistrationPacket.CLIENT_EnableInterrupt = SimGpioEnableInterrupt;

    //
    // Interrupt mask, unmask and reconfigure interrupt handlers.
    //

    RegistrationPacket.CLIENT_MaskInterrupts = SimGpioMaskInterrupts;
    RegistrationPacket.CLIENT_UnmaskInterrupt = SimGpioUnmaskInterrupt;
    RegistrationPacket.CLIENT_ReconfigureInterrupt = SimGpioReconfigureInterrupt;

    //
    // Handlers to query active/enabled interrupts and clear active interrupts.
    //

    RegistrationPacket.CLIENT_ClearActiveInterrupts = SimGpioClearActiveInterrupts;
    RegistrationPacket.CLIENT_QueryActiveInterrupts = SimGpioQueryActiveInterrupts;
    RegistrationPacket.CLIENT_QueryEnabledInterrupts = SimGpioQueryEnabledInterrupts;

    //
    // Handlers for GPIO I/O operations.
    //

    RegistrationPacket.CLIENT_ConnectIoPins = SimGpioConnectIoPins;
    RegistrationPacket.CLIENT_DisconnectIoPins = SimGpioDisconnectIoPins;
    RegistrationPacket.CLIENT_ReadGpioPinsUsingMask = SimGpioReadGpioPins;
    RegistrationPacket.CLIENT_WriteGpioPinsUsingMask = SimGpioWriteGpioPins;

    //
    // Register the SimGPIO client driver with the GPIO class extension.
    //

    Status = GPIO_CLX_RegisterClient(Driver, &RegistrationPacket, RegistryPath);

DriverEntryEnd:
    return Status;
}

NTSTATUS
SimGpioEvtDeviceAdd (
    _In_ WDFDRIVER Driver,
    _Inout_ PWDFDEVICE_INIT DeviceInit
    )

/*++

Routine Description:

    This routine is the AddDevice entry point for the client driver. This
    routine is called by the framework in response to AddDevice call from the
    PnP manager. It will create and initialize the device object to represent
    a new instance of the simulated GPIO controller.

Arguments:

    Driver - Supplies a handle to the driver object created in DriverEntry.

    DeviceInit - Supplies a pointer to a framework-allocated WDFDEVICE_INIT
        structure.

Return Value:

    NTSTATUS code.

--*/

{

    WDFDEVICE Device;
    WDF_OBJECT_ATTRIBUTES FdoAttributes;
    NTSTATUS Status;

    PAGED_CODE();

    //
    // Call the GPIO class extension's pre-device create interface.
    //

    Status = GPIO_CLX_ProcessAddDevicePreDeviceCreate(Driver,
                                                       DeviceInit,
                                                       &FdoAttributes);
    if (!NT_SUCCESS(Status)) {
        TraceEvents(
                TRACE_LEVEL_ERROR,
                TRACE_FLAG_INIT,
                "%s: GPIO_CLX_ProcessAddDevicePreDeviceCreate() failed! "
                "Status = %#x\n",
                __FUNCTION__,
                Status);

        goto EvtDeviceAddEnd;
    }

    //
    // Call the framework to create the device and attach it to the lower stack.
    //

    Status = WdfDeviceCreate(&DeviceInit, &FdoAttributes, &Device);
    if (!NT_SUCCESS(Status)) {
        TraceEvents(
                TRACE_LEVEL_ERROR,
                TRACE_FLAG_INIT,
                "%s: WdfDeviceCreate() failed! Status = %#x\n",
                __FUNCTION__,
                Status);

        goto EvtDeviceAddEnd;
    }

    //
    // Call the GPIO class extension's post-device create interface.
    //

    Status = GPIO_CLX_ProcessAddDevicePostDeviceCreate(Driver, Device);
    if (!NT_SUCCESS(Status)) {
        TraceEvents(
                TRACE_LEVEL_ERROR,
                TRACE_FLAG_INIT,
                "%s: GPIO_CLX_ProcessAddDevicePostDeviceCreate() failed! "
                "Status = %#x\n",
                __FUNCTION__,
                Status);

        goto EvtDeviceAddEnd;
    }

EvtDeviceAddEnd:
    return Status;
}

//
// ---------------------------------------------------------- General intefaces
//

_Must_inspect_result_
_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
SimGpioPrepareController (
    _In_ WDFDEVICE Device,
    _In_ PVOID Context,
    _In_ WDFCMRESLIST ResourcesRaw,
    _In_ WDFCMRESLIST ResourcesTranslated
    )

/*++

Routine Description:

    This routine is called by the GPIO class extension to prepare the
    simulated GPIO controller for use.

    N.B. This function is not marked pageable because this function is in
         the device power up path.

Arguments:

    Context - Supplies a pointer to the GPIO client driver's device extension.

    ResourcesRaw - Supplies a handle to a collection of framework resource
        objects. This collection identifies the raw (bus-relative) hardware
        resources that have been assigned to the device.

    ResourcesTranslated - Supplies a handle to a collection of framework
        resource objects. This collection identifies the translated
        (system-physical) hardware resources that have been assigned to the
        device. The resources appear from the CPU's point of view.

Return Value:

    NTSTATUS code.

--*/

{

    UCHAR ConnectionClass;
    UCHAR ConnectionType;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR Descriptor;
    PSIM_GPIO_CONTEXT GpioContext;
    ULONG Index;
    BOOLEAN I2ResourceFound;
    ULONG InterruptResourceCount;
    ULONG ResourceCount;
    NTSTATUS Status;

    UNREFERENCED_PARAMETER(Device);
    UNREFERENCED_PARAMETER(ResourcesRaw);

    GpioContext = (PSIM_GPIO_CONTEXT)Context;
    RtlZeroMemory(GpioContext, sizeof(SIM_GPIO_CONTEXT));
    GpioContext->TotalPins = SIM_GPIO_TOTAL_PINS;

    //
    // Walk through the resource list and map all the resources. Atleast one
    // memory resource and one interrupt resource is expected. The resources
    // are described in the ACPI namespace.
    //

    I2ResourceFound = FALSE;
    InterruptResourceCount = 0;
    ResourceCount = WdfCmResourceListGetCount(ResourcesTranslated);
    Status = STATUS_SUCCESS;
    for (Index = 0; Index < ResourceCount; Index += 1) {
        Descriptor = WdfCmResourceListGetDescriptor(ResourcesTranslated, Index);
        switch(Descriptor->Type) {

        //
        // Look for a connection resource that supplies the connection ID
        // required to open a handle to the I2C controller.
        //

        case CmResourceTypeConnection:
            ConnectionClass = Descriptor->u.Connection.Class;
            ConnectionType = Descriptor->u.Connection.Type;
            if ((ConnectionClass == CM_RESOURCE_CONNECTION_CLASS_SERIAL) &&
                (ConnectionType == CM_RESOURCE_CONNECTION_TYPE_SERIAL_I2C) &&
                (I2ResourceFound == FALSE)) {

                GpioContext->SpbConnectionId.LowPart =
                        Descriptor->u.Connection.IdLowPart;

                GpioContext->SpbConnectionId.HighPart =
                        Descriptor->u.Connection.IdHighPart;

                I2ResourceFound = TRUE;
            }

            break;

        //
        // Interrupt resource which supplies the GPIO controller interrupt
        // (that connects to the GIC).
        //
        // N.B. Connecting of the interrupt is handled by the GPIO class
        //      extension. Only ensure that appropriate number of interrupts
        //      were described.
        //

        case CmResourceTypeInterrupt:
            InterruptResourceCount += 1;
            break;

            //
            // This could be device-private type added by the underlying bus
            // driver. Do not filter or alter this resource information.
            //

        default:
            break;
        }

        if (!NT_SUCCESS(Status)) {
            goto PrepareControllerEnd;
        }
    }

    //
    // Fail initialization if minimum number of interrupt and connection
    // resources were not described correctly.
    //

    if ((InterruptResourceCount != 1) || (I2ResourceFound == FALSE)) {
        TraceEvents(
                TRACE_LEVEL_ERROR,
                TRACE_FLAG_INIT,
                "%s: Resource do not match expected values! "
                "Interrupt = %d, I2C = %d\n",
                __FUNCTION__,
                InterruptResourceCount,
                I2ResourceFound);

        Status = STATUS_UNSUCCESSFUL;
        goto PrepareControllerEnd;
    }

    GpioContext->TotalPins = SIM_GPIO_TOTAL_PINS;
    GpioContext->TotalBanks = SIM_GPIO_TOTAL_BANKS;
    GpioContext->Device = Device;
    for (Index = 0; Index < GpioContext->TotalBanks; Index += 1) {
        GpioContext->Banks[Index].GpioContext = GpioContext;
        GpioContext->Banks[Index].AddressBase =
            (USHORT)(Index * MaximumSimGpioAddress);
    }

    //
    // For SimGpio_i2c, the connection to the I2C controller needs to be
    // established.
    //

    Status = SimGpioSetupSpbConnection(GpioContext);
    if (!NT_SUCCESS(Status)) {
        TraceEvents(
                TRACE_LEVEL_ERROR,
                TRACE_FLAG_INIT,
                "%s: SimGpioSetupSpbConnection() failed! Status = %#x\n",
                __FUNCTION__,
                Status);

        goto PrepareControllerEnd;
    }

PrepareControllerEnd:
    return Status;
}

_Must_inspect_result_
_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
SimGpioReleaseController (
    _In_ WDFDEVICE Device,
    _In_ PVOID Context
    )

/*++

Routine Description:

    This routine is called by the GPIO class extension to uninitialize the GPIO
    controller.

    N.B. This function is not marked pageable because this function is in
         the device power down path.

Arguments:

    Context - Supplies a pointer to the GPIO client driver's device extension.

Return Value:

    NTSTATUS code.

--*/

{

    UNREFERENCED_PARAMETER(Device);

    //
    // Release the mappings established in the initialize callback.
    //
    // N.B. Disconnecting of the interrupt is handled by the GPIO class
    //      extension.
    //

    SimGpioDestroySpbConnection(Context);

    //
    // Stop WPP Tracing.
    //
    // TODO: DO THIS in the cleanup callback!
    //

    WPP_CLEANUP(NULL);

    return STATUS_SUCCESS;
}

_Must_inspect_result_
_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
SimGpioQueryControllerBasicInformation (
    _In_ PVOID Context,
    _Out_ PCLIENT_CONTROLLER_BASIC_INFORMATION ControllerInformation
    )

/*++

Routine Description:

    This routine returns the GPIO controller's attributes to the class extension.

    N.B. This function is not marked pageable because this function is in
         the device power up path.

Arguments:

    Context - Supplies a pointer to the GPIO client driver's device extension.

    ControllerInformation - Supplies a pointer to a buffer that receives
        controller's information.

Return Value:

    NTSTATUS code.

--*/

{

    PSIM_GPIO_CONTEXT GpioContext;

    ControllerInformation->Version = GPIO_CONTROLLER_BASIC_INFORMATION_VERSION;
    ControllerInformation->Size = sizeof(CLIENT_CONTROLLER_BASIC_INFORMATION);

    //
    // Specify the number of pins on the SimGPIO controller.
    //

    GpioContext = (PSIM_GPIO_CONTEXT)Context;
    ControllerInformation->TotalPins = GpioContext->TotalPins;
    ControllerInformation->NumberOfPinsPerBank = SIM_GPIO_PINS_PER_BANK;

    //
    // Indicate that the GPIO controller is off-SoC (i.e. behind some I2C bus)
    // and thus can only be serviced at PASSIVE_LEVEL IRQL.
    //

    ControllerInformation->Flags.MemoryMappedController = FALSE;

    //
    // Indicate that status register must be cleared explicitly.
    //

    ControllerInformation->Flags.ActiveInterruptsAutoClearOnRead = FALSE;

    //
    // Indicate that the client driver would like to receive IO requests as a
    // set of bitmasks as that maps directly to the register operations.
    //

    ControllerInformation->Flags.FormatIoRequestsAsMasks = TRUE;

    //
    // Indicate that the GPIO controller does not support controller-level
    // D-state power management or F-state power management.
    //
    // N.B. F-state management is only supported for on-SoC GPIO controllers.
    //

    ControllerInformation->Flags.DeviceIdlePowerMgmtSupported = FALSE;
    ControllerInformation->Flags.BankIdlePowerMgmtSupported = FALSE;

    //
    // Note the IdleTimeout parameter does not need to be initialized if
    // D-state power management is not supported.
    //
    // ControllerInformation->IdleTimeout = IdleTimeoutDefaultValue;
    //

    //
    // Indicate that the client driver prefers GPIO class extension ActiveBoth
    // emulation.
    //

    ControllerInformation->Flags.EmulateActiveBoth = TRUE;

    return STATUS_SUCCESS;
}

_Must_inspect_result_
_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
SimGpioStartController (
    _In_ PVOID Context,
    _In_ BOOLEAN RestoreContext,
    _In_ WDF_POWER_DEVICE_STATE PreviousPowerState
    )

/*++

Routine Description:

    This routine starts the simulated GPIO controller. This routine is
    responsible for configuring all the pins to their default modes.

    N.B. This function is not marked pageable because this function is in
         the device power up path. It is called at PASSIVE_IRQL though.

Arguments:

    Context - Supplies a pointer to the GPIO client driver's device extension.

    RestoreContext - Supplies a flag that indicates whether the client driver
        should restore the GPIO controller state to a previously saved state
        or not.

    PreviousPowerState - Supplies the device power state that the device was in
        before this transition to D0.

Return Value:

    NTSTATUS code.

--*/

{

    BANK_ID BankId;
    UCHAR EnableValue;
    PSIM_GPIO_BANK GpioBank;
    PSIM_GPIO_CONTEXT GpioContext;
    NTSTATUS Status;

    UNREFERENCED_PARAMETER(PreviousPowerState);

    //
    // Perform all the steps necessary to start the device.
    //

    //
    // If restore context is FALSE, then this is initial transition into D0
    // power state for this controller. In such case, disable any interrupts
    // that may have been left enabled (e.g. perhaps by FW, previous D0 -> Dx
    // transition etc.) Otherwise, such interrupts could trigger an interrupt
    // storm if they were to assert without any driver being registered to
    // handle such interrupts.
    //
    // If restore context is TRUE, then this is a transition into D0 power
    // state from a lower power Dx state. In such case, restore the context
    // that was present before the controller transitioned into the lower
    // power state.
    //

    GpioContext = (PSIM_GPIO_CONTEXT)Context;
    Status = STATUS_SUCCESS;
    if (RestoreContext == FALSE) {
        for (BankId = 0; BankId < GpioContext->TotalBanks; BankId += 1) {
            GpioBank = &GpioContext->Banks[BankId];

            //
            // Read the current values of the interrupt enable register.
            //

            EnableValue = 0;
            Status = SimGpioSpbReadByte(GpioBank, EnableRegister, &EnableValue);
            if (!NT_SUCCESS(Status)) {
                TraceEvents(TRACE_LEVEL_ERROR,
                            TRACE_FLAG_INIT,
                            "%s: SimGpioSpbReadByte(EnableRegister) failed! "
                            "Status = %#x\n",
                            __FUNCTION__,
                            Status);

                goto StartControllerEnd;
            }

            //
            // If no interrupts are enabled, then skip to the next bank.
            //

            if (EnableValue == 0) {
                continue;
            }

            //
            // Disable all interrupts on this bank by clearing the enable
            // register.
            //

            EnableValue = 0;
            Status = SimGpioSpbWriteByte(GpioBank, EnableRegister, EnableValue);
            if (!NT_SUCCESS(Status)) {
                TraceEvents(TRACE_LEVEL_ERROR,
                            TRACE_FLAG_INIT,
                            "%s: SimGpioSpbWriteByte(EnableRegister) failed! "
                            "Status = %#x\n",
                            __FUNCTION__,
                            Status);

                goto StartControllerEnd;
            }
        }

    } else {

        //
        // Restoring the controller state involves restoring the state of
        // each SimGPIO bank.
        //

        for (BankId = 0; BankId < GpioContext->TotalBanks; BankId += 1) {
            Status = SimGpioRestoreBankHardwareContext(Context, BankId);
            if (!NT_SUCCESS(Status)) {
                TraceEvents(TRACE_LEVEL_ERROR,
                            TRACE_FLAG_INIT,
                            "%s: SimGpioRestoreBankHardwareContext() failed! "
                            "Context = %p, Bank = %d, Status = %#x\n",
                            __FUNCTION__,
                            Context,
                            BankId,
                            Status);

                goto StartControllerEnd;
            }
        }
    }

StartControllerEnd:
    return Status;
}

_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
SimGpioStopController (
    _In_ PVOID Context,
    _In_ BOOLEAN SaveContext,
    _In_ WDF_POWER_DEVICE_STATE TargetState
    )

/*++

Routine Description:

    This routine stops the GPIO controller. This routine is responsible for
    resetting all the pins to their default modes.

    N.B. This function is not marked pageable because this function is in
         the device power up path.

Arguments:

    Context - Supplies a pointer to the GPIO client driver's device extension.

    SaveContext - Supplies a flag that indicates whether the client driver
        should save the GPIO controller state or not. The state may need
        to be restored when the controller is restarted.

    TargetState - Supplies the device power state which the device will be put
        in once the callback is complete.

Return Value:

    NTSTATUS code.

--*/

{

    BANK_ID BankId;
    PSIM_GPIO_CONTEXT GpioContext;
    NTSTATUS Status;

    UNREFERENCED_PARAMETER(TargetState);

    //
    // Perform all the steps necessary to stop the device.
    //

    //
    // If save context is FALSE, then this is a final transition into D3/off
    // power state. Hence saving of context is not necessary.
    //
    // If save context is TRUE, then this is a transition into a lower power
    // Dx state. In such case, save the context as it will need to be
    // restored when the device is brought back to D0 (i.e. ON) power state.
    //

    GpioContext = (PSIM_GPIO_CONTEXT)Context;
    Status = STATUS_SUCCESS;
    if (SaveContext == TRUE) {
        for (BankId = 0; BankId < GpioContext->TotalBanks; BankId += 1) {
            Status = SimGpioSaveBankHardwareContext(Context, BankId);
            if (!NT_SUCCESS(Status)) {
                TraceEvents(TRACE_LEVEL_ERROR,
                            TRACE_FLAG_INIT,
                            "%s: SimGpioSaveBankHardwareContext() failed! "
                            "Context = %p, Bank = %d, Status = %#x\n",
                            __FUNCTION__,
                            Context,
                            BankId,
                            Status);

                goto StopControllerEnd;
            }
        }
    }

StopControllerEnd:
    return Status;
}

//
// --------------------------------------------------------- Interrupt Handlers
//

_Must_inspect_result_
_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
SimGpioEnableInterrupt (
    _In_ PVOID Context,
    _In_ PGPIO_ENABLE_INTERRUPT_PARAMETERS EnableParameters
    )

/*++

Routine Description:

    This routine configures the supplied pin for interrupt.

    N.B. For off-SOC GPIOs, the GPIO class extension synchronizes this callback
         with other interrupt-related callbacks. Thus the locking is
         automatically taken care of by the class extension.

Arguments:

    Context - Supplies a pointer to the GPIO client driver's device extension.

    EnableParameters - Supplies a pointer to a structure containing enable
        operation parameters. Fields are:

        BankId - Supplies the ID for the GPIO bank.

        PinNumber - Supplies the interrupt line that should be enabled. The pin
            number is relative to the bank.

        Flags - Supplies flags controlling the enable operation. Currently
            no flags are defined.

        InterruptMode - Supplies the trigger mode (edge or level) configured for
            this interrupt when it was enabled.

        Polarity - Supplies the polarity (active low or active high) configured
            for this interrupt when it was enabled.
            Note: For edge-triggered interrupts, ActiveLow corresponds to the
                falling edge; ActiveHigh corresponds to the rising edge.

        PullConfiguration - Supplies the pin pull-up/pull-down configuration.

        DebouceTimeout - Supplies the debounce timeout to be applied. The
            field is in 100th of milli-seconds (i.e., 5.84ms will be supplied
            as 584). Default value is zero, which implies, no debounce.

        VendorData - Supplies an optional pointer to a buffer containing the
            vendor data supplied in the GPIO descriptor. This field will be
            NULL if no vendor data was supplied. This buffer is read-only.

        VendorDataLength - Supplies the length of the vendor data buffer.

Return Value:

    NTSTATUS code.

Environment:

    Entry IRQL: PASSIVE_LEVEL.

    Synchronization: The GPIO class extension will synchronize this call
        against other passive-level interrupt callbacks (e.g. enable/disable/
        unmask) and IO callbacks.

--*/

{

    BANK_ID BankId;
    UCHAR EnableValue;
    PSIM_GPIO_BANK GpioBank;
    PSIM_GPIO_CONTEXT GpioContext;
    UCHAR ModeValue;
    PIN_NUMBER PinNumber;
    UCHAR PolarityValue;
    NTSTATUS Status;
    UCHAR StatusRegisterValue;

    //
    // If the polarity is not supported, then bail out. Note the interrupt
    // polarity cannot be InterruptActiveBoth as this sample uses ActiveBoth
    // emulation.
    //

    if ((EnableParameters->Polarity != InterruptActiveHigh) &&
        (EnableParameters->Polarity != InterruptActiveLow)) {

        Status = STATUS_NOT_SUPPORTED;
        TraceEvents(
                TRACE_LEVEL_ERROR,
                TRACE_FLAG_INIT,
                "%s: Polarity not supported! Polarity = %d, Status = %#x\n",
                __FUNCTION__,
                EnableParameters->Polarity,
                Status);

        goto EnableInterruptEnd;
    }

    BankId = EnableParameters->BankId;
    PinNumber = EnableParameters->PinNumber;
    Status = STATUS_SUCCESS;
    GpioContext = (PSIM_GPIO_CONTEXT)Context;
    GpioBank = &GpioContext->Banks[BankId];

    //
    // Read the current values of the interrupt mode register, polarity
    // register, enable register.
    //

    Status = SimGpioSpbReadByte(GpioBank, ModeRegister, &ModeValue);
    if (!NT_SUCCESS(Status)) {
        TraceEvents(
                TRACE_LEVEL_ERROR,
                TRACE_FLAG_INIT,
                "%s: SimGpioSpbReadByte(ModeRegister) failed! Status = %#x\n",
                __FUNCTION__,
                Status);

        goto EnableInterruptEnd;
    }

    Status = SimGpioSpbReadByte(GpioBank, PolarityRegister, &PolarityValue);
    if (!NT_SUCCESS(Status)) {
        TraceEvents(
                TRACE_LEVEL_ERROR,
                TRACE_FLAG_INIT,
                "%s: SimGpioSpbReadByte(PolarityRegister) failed! Status = %#x\n",
                __FUNCTION__,
                Status);

        goto EnableInterruptEnd;
    }

    Status = SimGpioSpbReadByte(GpioBank, EnableRegister, &EnableValue);
    if (!NT_SUCCESS(Status)) {
        TraceEvents(
                TRACE_LEVEL_ERROR,
                TRACE_FLAG_INIT,
                "%s: SimGpioSpbReadByte(EnableRegister) failed! Status = %#x\n",
                __FUNCTION__,
                Status);

        goto EnableInterruptEnd;
    }

    //
    // Determine the mode register value. If the interrupt is Level then set
    // the bit; otherwise, clear it (edge-triggered).
    //

    if (EnableParameters->InterruptMode == LevelSensitive) {
        ModeValue |= (1 << PinNumber);
    } else {
        ModeValue &= ~(1 << PinNumber);
    }

    //
    // Determine the new polarity register value. If the interrupt is ActiveHigh
    // (or Rising-edge) then set it to 0x1. Otherwise, clear it.
    //

    switch (EnableParameters->Polarity) {

    case InterruptActiveHigh:
        PolarityValue |= (1 << PinNumber);
        break;

    case InterruptActiveLow:
        PolarityValue &= ~(1 << PinNumber);
        break;
    }

    //
    // Clear the corresponding status bit first to ignore any stale value.
    // The status register is atomically modified on the controller. It takes
    // a bitmask on the bits to be cleared.
    //

    StatusRegisterValue = (1 << PinNumber);

    //
    // Enable the interrupt by setting the bit in the interrupt enable register.
    //

    EnableValue |= (1 << PinNumber);

    //
    // Write the new values for the interrupt mode register, polarity
    // register, status register and enable register.
    //

    Status = SimGpioSpbWriteByte(GpioBank, ModeRegister, ModeValue);
    if (!NT_SUCCESS(Status)) {
        TraceEvents(
                TRACE_LEVEL_ERROR,
                TRACE_FLAG_INIT,
                "%s: SimGpioSpbWriteByte(ModeRegister) failed! Status = %#x\n",
                __FUNCTION__,
                Status);

        goto EnableInterruptEnd;
    }

    Status = SimGpioSpbWriteByte(GpioBank, PolarityRegister, PolarityValue);
    if (!NT_SUCCESS(Status)) {
        TraceEvents(
                TRACE_LEVEL_ERROR,
                TRACE_FLAG_INIT,
                "%s: SimGpioSpbWriteByte(PolarityRegister) failed! Status = %#x\n",
                __FUNCTION__,
                Status);

        goto EnableInterruptEnd;
    }

    Status = SimGpioSpbWriteByte(GpioBank, StatusRegister, StatusRegisterValue);
    if (!NT_SUCCESS(Status)) {
        TraceEvents(
                TRACE_LEVEL_ERROR,
                TRACE_FLAG_INIT,
                "%s: SimGpioSpbWriteByte(StatusRegister) failed! Status = %#x\n",
                __FUNCTION__,
                Status);

        goto EnableInterruptEnd;
    }

    //
    // Enable the interrupt by setting the bit in the interrupt enable register.
    //

    Status = SimGpioSpbWriteByte(GpioBank, EnableRegister, EnableValue);
    if (!NT_SUCCESS(Status)) {
        TraceEvents(
                TRACE_LEVEL_ERROR,
                TRACE_FLAG_INIT,
                "%s: SimGpioSpbWriteByte(EnableRegister) failed! Status = %#x\n",
                __FUNCTION__,
                Status);


        goto EnableInterruptEnd;
    }

EnableInterruptEnd:
    return Status;
}

_Must_inspect_result_
_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
SimGpioDisableInterrupt (
    _In_ PVOID Context,
    _In_ PGPIO_DISABLE_INTERRUPT_PARAMETERS DisableParameters
    )

/*++

Routine Description:

    This routine disables the supplied pin from interrupting.

    This routine is not marked PAGED as it may be called before/after
    the boot device is in D0/D3 if boot device has GPIO dependencies.

Arguments:

    Context - Supplies a pointer to the GPIO client driver's device extension.

    DisableParameters - Supplies a pointer to a structure supplying the
        parameters for disabling the interrupt. Fields are:

        BankId - Supplies the ID for the GPIO bank.

        PinNumber - Supplies the interrupt line that should be disabled. The pin
            number is relative to the bank.

        Flags - Supplies flags controlling the disable operation. Currently
            no flags are defined.

Return Value:

    NTSTATUS code.

Environment:

    Entry IRQL: PASSIVE_LEVEL.

    Synchronization: The GPIO class extension will synchronize this call
        against other passive-level interrupt callbacks (e.g. enable/disable/
        unmask) and IO callbacks.

--*/

{

    UCHAR EnableValue;
    PSIM_GPIO_BANK GpioBank;
    PSIM_GPIO_CONTEXT GpioContext;
    NTSTATUS Status;

    GpioContext = (PSIM_GPIO_CONTEXT)Context;
    GpioBank = &GpioContext->Banks[DisableParameters->BankId];

    //
    // Read the current value of the interrupt enable register.
    //

    Status = SimGpioSpbReadByte(GpioBank, EnableRegister, &EnableValue);
    if (!NT_SUCCESS(Status)) {
        TraceEvents(
                TRACE_LEVEL_ERROR,
                TRACE_FLAG_INIT,
                "%s: SimGpioSpbReadByte(EnableRegister) failed! Status = %#x\n",
                __FUNCTION__,
                Status);

        goto DisableInterruptEnd;
    }

    //
    // Disable the interrupt by clearing the bit in the interrupt enable
    // register.
    //

    EnableValue &= ~(1 << DisableParameters->PinNumber);
    Status = SimGpioSpbWriteByte(GpioBank, EnableRegister, EnableValue);
    if (!NT_SUCCESS(Status)) {
        TraceEvents(
                TRACE_LEVEL_ERROR,
                TRACE_FLAG_INIT,
                "%s: SimGpioSpbWriteByte(EnableRegister) failed! Status = %#x\n",
                __FUNCTION__,
                Status);

        goto DisableInterruptEnd;
    }

DisableInterruptEnd:
    return Status;
}

_Must_inspect_result_
_IRQL_requires_same_
NTSTATUS
SimGpioMaskInterrupts (
    _In_ PVOID Context,
    _In_ PGPIO_MASK_INTERRUPT_PARAMETERS MaskParameters
    )

/*++

Routine Description:

    This routine invokes masks the supplied pin from interrupting.

Arguments:

    Context - Supplies a pointer to the GPIO client driver's device extension.

    MaskParameters - Supplies a pointer to a structure containing mask
        operation parameters. Fields are:

        BankId - Supplies the ID for the GPIO bank.

        PinMask - Supplies a bitmask of pins which should be masked. If a pin
            should be masked, then the corresponding bit is set in the bitmask.

        FailedMask - Supplies a bitmask of pins that failed to be masked. If
            a pin could not be masked, the bit should be set in this field.

            N.B. This should only be done if for non memory-mapped controllers.
                 Memory-mapped controllers are never expected to fail this
                 operation.

Return Value:

    NTSTATUS code.

Environment:

    Entry IRQL: DIRQL if the GPIO controller is memory-mapped; PASSIVE_LEVEL
        if the controller is behind some serial-bus.

        N.B. For memory-mapped controllers, this routine is called from within
             the interrupt context with the interrupt lock acquired by the class
             extension. Hence the lock is not re-acquired here.

    Synchronization: The GPIO class extension will synchronize this call
        against other query/clear active and enabled interrupts.
        Memory-mapped GPIO controllers:
            Callbacks invoked at PASSIVE_LEVEL IRQL (e.g. interrupt
            enable/disable/unmask or IO operations) may be active. Those
            routines should acquire the interrupt lock prior to manipulating
            any state accessed from within this routine.

        Serial-accessible GPIO controllers:
            This call is synchronized with all other interrupt and IO callbacks.

--*/

{

    GPIO_DISABLE_INTERRUPT_PARAMETERS DisableParameters;
    PIN_NUMBER PinIndex;
    ULONG64 PinMask;
    NTSTATUS Status;

    //
    // Mask is essentially same as disable for SimGPIO controller. The primary
    // difference is that mask callback supplies a bit-mask.
    //

    Status = STATUS_SUCCESS;
    PinMask = MaskParameters->PinMask;
    RtlZeroMemory(&DisableParameters, sizeof(DisableParameters));
    DisableParameters.BankId = MaskParameters->BankId;
    while (PinMask > 0) {
        PinIndex = RtlFindLeastSignificantBit(PinMask);
        if (PinIndex == 0xFF) {

            NT_ASSERT(FALSE);

            goto MaskInterruptsEnd;
        }

        DisableParameters.PinNumber = PinIndex;
        Status = SimGpioDisableInterrupt(Context, &DisableParameters);
        if (!NT_SUCCESS(Status)) {
            goto MaskInterruptsEnd;
        }

        PinMask &= ~(1 << PinIndex);
    }

    //
    // Set the bitmask of pins that could not be successfully masked.
    //

MaskInterruptsEnd:
    MaskParameters->FailedMask = PinMask;
    return Status;
}

NTSTATUS
SimGpioUnmaskInterrupt (
    _In_ PVOID Context,
    _In_ PGPIO_ENABLE_INTERRUPT_PARAMETERS UnmaskParameters
    )

/*++

Routine Description:

    This routine invokes unmasks the supplied interrupt pin.

Arguments:

    Context - Supplies a pointer to the GPIO client driver's device extension.

    UnmaskParameters - Supplies a pointer to a structure containing parameters
        for unmasking the interrupt. Fields are:

        BankId - Supplies the ID for the GPIO bank.

        PinNumber - Supplies the interrupt line that should be unmasked. The pin
            number is relative to the bank.

        InterruptMode - Supplies the trigger mode (edge or level) configured for
            this interrupt when it was enabled.

        Polarity - Supplies the polarity (active low or active high) configured
            for this interrupt when it was enabled.
            Note: For edge-triggered interrupts, ActiveLow corresponds to the
                falling edge; ActiveHigh corresponds to the rising edge.

        PullConfiguration - Supplies the pin pull-up/pull-down configuration.

        DebouceTimeout - Supplies the debounce timeout to be applied. The
            field is in 100th of milli-seconds (i.e., 5.84ms will be supplied
            as 584). Default value is zero, which implies, no debounce.

        VendorData - NULL.

        VendorDataLength - 0.

        N.B. The VendorData and VendorDataLength are not supplied for unmask
             operation (i.e., both fields are zero).

Return Value:

    NTSTATUS code.

Environment:

    Entry IRQL: DIRQL if the GPIO controller is memory-mapped; PASSIVE_LEVEL
        if the controller is behind some serial-bus.

        N.B. For memory-mapped controllers, this routine is called from within
             the interrupt context with the interrupt lock acquired by the class
             extension. Hence the lock is not re-acquired here.

    Synchronization: The GPIO class extension will synchronize this call
        against other query/clear active and enabled interrupts.
        Memory-mapped GPIO controllers:
            Callbacks invoked at PASSIVE_LEVEL IRQL (e.g. interrupt
            enable/disable/unmask or IO operations) may be active. Those
            routines should acquire the interrupt lock prior to manipulating
            any state accessed from within this routine.

        Serial-accessible GPIO controllers:
            This call is synchronized with all other interrupt and IO callbacks.

--*/

{

    UCHAR EnableValue;
    PSIM_GPIO_BANK GpioBank;
    PSIM_GPIO_CONTEXT GpioContext;
    NTSTATUS Status;

    GpioContext = (PSIM_GPIO_CONTEXT)Context;
    GpioBank = &GpioContext->Banks[UnmaskParameters->BankId];

    //
    // Read the current value of the interrupt enable register.
    //

    Status = SimGpioSpbReadByte(GpioBank, EnableRegister, &EnableValue);
    if (!NT_SUCCESS(Status)) {
        TraceEvents(
                TRACE_LEVEL_ERROR,
                TRACE_FLAG_INIT,
                "%s: SimGpioSpbReadByte(EnableRegister) failed! Status = %#x\n",
                __FUNCTION__,
                Status);

        goto UnmaskInterruptEnd;
    }

    //
    // Unmask the interrupt by setting the bit in the interrupt enable
    // register.
    //

    EnableValue |= (1 << UnmaskParameters->PinNumber);
    Status = SimGpioSpbWriteByte(GpioBank, EnableRegister, EnableValue);
    if (!NT_SUCCESS(Status)) {
        TraceEvents(
                TRACE_LEVEL_ERROR,
                TRACE_FLAG_INIT,
                "%s: SimGpioSpbWriteByte(EnableRegister) failed! Status = %#x\n",
                __FUNCTION__,
                Status);

        goto UnmaskInterruptEnd;
    }

UnmaskInterruptEnd:
    return Status;
}

_Must_inspect_result_
_IRQL_requires_same_
NTSTATUS
SimGpioQueryActiveInterrupts (
    _In_ PVOID Context,
    _In_ PGPIO_QUERY_ACTIVE_INTERRUPTS_PARAMETERS QueryActiveParameters
    )

/*++

Routine Description:

    This routine returns the current set of active interrupts.

Arguments:

    Context - Supplies a pointer to the GPIO client driver's device extension.

    QueryActiveParameters - Supplies a pointer to a structure containing query
        parameters. Fields are:

        BankId - Supplies the ID for the GPIO bank.

        EnabledMask - Supplies a bitmask of pins enabled for interrupts
            on the specified GPIO bank.

        ActiveMask - Supplies a bitmask that receives the active interrupt
            mask. If a pin is interrupting and set in EnabledMask, then the
            corresponding bit is set in the bitmask.

Return Value:

    NTSTATUS code.

Environment:

    Entry IRQL: DIRQL if the GPIO controller is memory-mapped; PASSIVE_LEVEL
        if the controller is behind some serial-bus.

        N.B. For memory-mapped controllers, this routine is called from within
             the interrupt context with the interrupt lock acquired by the class
             extension.

    Synchronization: The GPIO class extension will synchronize this call
        against other query/clear active and enabled interrupts.
        Memory-mapped GPIO controllers:
            Callbacks invoked at PASSIVE_LEVEL IRQL (e.g. interrupt
            enable/disable/unmask or IO operations) may be active. Those
            routines should acquire the interrupt lock prior to manipulating
            any state accessed from within this routine.

        Serial-accessible GPIO controllers:
            This call is synchronized with all other interrupt and IO callbacks.

--*/

{

    UCHAR EnableValue;
    PSIM_GPIO_BANK GpioBank;
    PSIM_GPIO_CONTEXT GpioContext;
    NTSTATUS ReturnStatus;
    UCHAR StatusValue;

    GpioContext = (PSIM_GPIO_CONTEXT)Context;
    GpioBank = &GpioContext->Banks[QueryActiveParameters->BankId];

    //
    // NOTE: As SimGPIO is not a real hardware device, no interrupt will ever
    //       fire. Thus the status register value will never change. To pretend
    //       as if a real interrupt happened, it marks all currently enabled
    //       interrupts as asserting. Copy the enable interrupt value into
    //       the status register.
    //
    //       This should NOT be done for a real GPIO controller!!
    //

    //
    // BEGIN: SIMGPIO HACK.
    //

    ReturnStatus = SimGpioSpbReadByte(GpioBank, EnableRegister, &EnableValue);
    if (!NT_SUCCESS(ReturnStatus)) {
        TraceEvents(
                TRACE_LEVEL_ERROR,
                TRACE_FLAG_INIT,
                "%s: SimGpioSpbReadByte(EnableRegister) failed! Status = %#x\n",
                __FUNCTION__,
                ReturnStatus);

        goto QueryActiveInterruptsEnd;
    }

    ReturnStatus = SimGpioSpbWriteByte(GpioBank, StatusRegister, EnableValue);
    if (!NT_SUCCESS(ReturnStatus)) {
        TraceEvents(
                TRACE_LEVEL_ERROR,
                TRACE_FLAG_INIT,
                "%s: SimGpioSpbWriteByte(StatusRegister) failed! "
                "Status = %#x\n",
                __FUNCTION__,
                ReturnStatus);

        goto QueryActiveInterruptsEnd;
    }

    //
    // END: SIMGPIO HACK.
    //

    //
    // Return the current value of the interrupt status register in the
    // ActiveMask parameter.
    //

    ReturnStatus = SimGpioSpbReadByte(GpioBank, StatusRegister, &StatusValue);
    if (!NT_SUCCESS(ReturnStatus)) {
        TraceEvents(
                TRACE_LEVEL_ERROR,
                TRACE_FLAG_INIT,
                "%s: SimGpioSpbWriteByte(EnableRegister) failed! "
                "Status = %#x\n",
                __FUNCTION__,
                ReturnStatus);

        goto QueryActiveInterruptsEnd;
    }

    QueryActiveParameters->ActiveMask = (ULONG64)StatusValue;

QueryActiveInterruptsEnd:
    return ReturnStatus;
}

_Must_inspect_result_
_IRQL_requires_same_
NTSTATUS
SimGpioQueryEnabledInterrupts (
    _In_ PVOID Context,
    _In_ PGPIO_QUERY_ENABLED_INTERRUPTS_PARAMETERS QueryEnabledParameters
    )

/*++

Routine Description:

    This routine returns the current set of enabled interrupts.

Arguments:

    Context - Supplies a pointer to the GPIO client driver's device extension.

    QueryEnabledParameters - Supplies a pointer to a structure containing query
        parameters. Fields are:

        BankId - Supplies the ID for the GPIO bank.

        EnabledMask - Supplies a bitmask that receives the enabled interrupt
            mask. If a pin is enabled, then the corresponding bit is set in the
            mask.

Return Value:

    NTSTATUS code.

Environment:

    Entry IRQL: DIRQL if the GPIO controller is memory-mapped; PASSIVE_LEVEL
        if the controller is behind some serial-bus.

        N.B. For memory-mapped controllers, this routine is called with the
             interrupt lock acquired by the class extension, but not always
             from within the interrupt context.

    Synchronization: The GPIO class extension will synchronize this call
        against other query/clear active and enabled interrupts.
        Memory-mapped GPIO controllers:
            Callbacks invoked at PASSIVE_LEVEL IRQL (e.g. interrupt
            enable/disable/unmask or IO operations) may be active. Those
            routines should acquire the interrupt lock prior to manipulating
            any state accessed from within this routine.

        Serial-accessible GPIO controllers:
            This call is synchronized with all other interrupt and IO callbacks.

--*/

{

    UCHAR EnableValue;
    PSIM_GPIO_BANK GpioBank;
    PSIM_GPIO_CONTEXT GpioContext;
    NTSTATUS ReturnStatus;

    GpioContext = (PSIM_GPIO_CONTEXT)Context;
    GpioBank = &GpioContext->Banks[QueryEnabledParameters->BankId];

    //
    // Return the current value of the interrupt enable register into the
    // EnabledMask parameter. It is strongly preferred that the true state of
    // the hardware is returned, rather than a software-cached variable, since
    // CLIENT_QueryEnabledInterrupts is used by the class extension to detect
    // interrupt storms.
    //

    ReturnStatus = SimGpioSpbReadByte(GpioBank, EnableRegister, &EnableValue);
    if (!NT_SUCCESS(ReturnStatus)) {
        TraceEvents(
                TRACE_LEVEL_ERROR,
                TRACE_FLAG_INIT,
                "%s: SimGpioSpbReadByte(EnableRegister) failed! Status = %#x\n",
                __FUNCTION__,
                ReturnStatus);

        goto QueryEnabledInterruptsEnd;
    }

    QueryEnabledParameters->EnabledMask = (ULONG64)EnableValue;

QueryEnabledInterruptsEnd:
    return ReturnStatus;
}
_Must_inspect_result_
_IRQL_requires_same_
NTSTATUS
SimGpioClearActiveInterrupts (
    _In_ PVOID Context,
    _In_ PGPIO_CLEAR_ACTIVE_INTERRUPTS_PARAMETERS ClearParameters
    )

/*++

Routine Description:

    This routine clears the GPIO controller's active set of interrupts.

Arguments:

    Context - Supplies a pointer to the GPIO client driver's device extension.

    ClearParameters - Supplies a pointer to a structure containing clear
        operation parameters. Fields are:

        BankId - Supplies the ID for the GPIO bank.

        ClearActiveMask - Supplies a mask of pins which should be marked as
            inactive. If a pin should be cleared, then the corresponding bit is
            set in the mask.

        FailedClearMask - Supplies a bitmask of pins that failed to be cleared. If
            a pin could not be cleared, the bit should be set in this field.

            N.B. This should only be done if for non memory-mapped controllers.
                 Memory-mapped controllers are never expected to fail this
                 operation.

Return Value:

    NTSTATUS code.

Environment:

    Entry IRQL: DIRQL if the GPIO controller is memory-mapped; PASSIVE_LEVEL
        if the controller is behind some serial-bus.

        N.B. For memory-mapped controllers, this routine is called from within
             the interrupt context with the interrupt lock acquired by the class
             extension.

    Synchronization: The GPIO class extension will synchronize this call
        against other query/clear active and enabled interrupts.
        Memory-mapped GPIO controllers:
            Callbacks invoked at PASSIVE_LEVEL IRQL (e.g. interrupt
            enable/disable/unmask or IO operations) may be active. Those
            routines should acquire the interrupt lock prior to manipulating
            any state accessed from within this routine.

        Serial-accessible GPIO controllers:
            This call is synchronized with all other interrupt and IO callbacks.

--*/

{

    PSIM_GPIO_BANK GpioBank;
    PSIM_GPIO_CONTEXT GpioContext;
    NTSTATUS ReturnStatus;
    UCHAR StatusValue;

    GpioContext = (PSIM_GPIO_CONTEXT)Context;
    GpioBank = &GpioContext->Banks[ClearParameters->BankId];

    //
    // Clear the bits that are set in the ClearActiveMask parameter.
    //

    StatusValue = (UCHAR)ClearParameters->ClearActiveMask;
    ReturnStatus = SimGpioSpbWriteByte(GpioBank, StatusRegister, StatusValue);
    if (!NT_SUCCESS(ReturnStatus)) {
        TraceEvents(
                TRACE_LEVEL_ERROR,
                TRACE_FLAG_INIT,
                "%s: SimGpioSpbWriteByte(StatusRegister) failed! Status = %#x\n",
                __FUNCTION__,
                ReturnStatus);

        goto ClearActiveInterruptsEnd;
    }

    //
    // If the operation succeeded, then all the bits were cleared.
    //

    StatusValue = 0x0;

    //
    // Set the bitmask of pins that could not be successfully cleared.
    // Either all bits get written successfully or none does.
    //

ClearActiveInterruptsEnd:
    ClearParameters->FailedClearMask = StatusValue;
    return ReturnStatus;
}

NTSTATUS
SimGpioReconfigureInterrupt (
    _In_ PVOID Context,
    _In_ PGPIO_RECONFIGURE_INTERRUPTS_PARAMETERS ReconfigureParameters
    )

/*++

Routine Description:

    This routine reconfigures the interrupt in the specified mode.

Arguments:

    Context - Supplies a pointer to the GPIO client driver's device extension.

    ReconfigureParameters - Supplies a pointer to a structure containing
        parameters for reconfiguring the interrupt. Fields are:

        BankId - Supplies the ID for the GPIO bank.

        PinNumber - Supplies the interrupt line that should be reconfigured.
            The pin number is relative to the bank.

        InterruptMode - Supplies the trigger mode (edge or level) for the new
            configuration.

        Polarity - Supplies the polarity (active low or active high) for the
            new configuration.
            Note: For edge-triggered interrupts, ActiveLow corresponds to the
                falling edge; ActiveHigh corresponds to the rising edge.

Return Value:

    NTSTATUS code.

Environment:

    Entry IRQL: DIRQL if the GPIO controller is memory-mapped; PASSIVE_LEVEL
        if the controller is behind some serial-bus.

        N.B. For memory-mapped controllers, this routine is called from within
             the interrupt context with the interrupt lock acquired by the class
             extension. Hence the lock is not re-acquired here.

    Synchronization: The GPIO class extension will synchronize this call
        against other query/clear active and enabled interrupts.
        Memory-mapped GPIO controllers:
            Callbacks invoked at PASSIVE_LEVEL IRQL (e.g. interrupt
            enable/disable/unmask or IO operations) may be active. Those
            routines should acquire the interrupt lock prior to manipulating
            any state accessed from within this routine.

        Serial-accessible GPIO controllers:
            This call is synchronized with all other interrupt and IO callbacks.

--*/

{

    BANK_ID BankId;
    PSIM_GPIO_BANK GpioBank;
    PSIM_GPIO_CONTEXT GpioContext;
    UCHAR ModeValue;
    PIN_NUMBER PinNumber;
    UCHAR PolarityValue;
    NTSTATUS Status;
    UCHAR StatusRegisterValue;


    BankId = ReconfigureParameters->BankId;
    PinNumber = ReconfigureParameters->PinNumber;
    GpioContext = (PSIM_GPIO_CONTEXT)Context;
    GpioBank = &GpioContext->Banks[BankId];

    //
    // Read the current values of the interrupt mode register, polarity
    // register, enable register.
    //

    Status = SimGpioSpbReadByte(GpioBank, ModeRegister, &ModeValue);
    if (!NT_SUCCESS(Status)) {
        TraceEvents(
                TRACE_LEVEL_ERROR,
                TRACE_FLAG_INIT,
                "%s: SimGpioSpbReadByte(ModeRegister) failed! Status = %#x\n",
                __FUNCTION__,
                Status);

        goto ReconfigureInterruptEnd;
    }

    Status = SimGpioSpbReadByte(GpioBank, PolarityRegister, &PolarityValue);
    if (!NT_SUCCESS(Status)) {
        TraceEvents(
                TRACE_LEVEL_ERROR,
                TRACE_FLAG_INIT,
                "%s: SimGpioSpbReadByte(PolarityRegister) failed! Status = %#x\n",
                __FUNCTION__,
                Status);

        goto ReconfigureInterruptEnd;
    }

    //
    // Determine the mode register value. If the interrupt is Level then set
    // the bit; otherwise, clear it (edge-triggered).
    //

    if (ReconfigureParameters->InterruptMode == LevelSensitive) {
        ModeValue |= (1 << PinNumber);
    } else {
        ModeValue &= ~(1 << PinNumber);
    }

    //
    // Determine the new polarity register value. If the interrupt is ActiveHigh
    // (or Rising-edge) then set it to 0x1. Otherwise, clear it.
    //

    switch (ReconfigureParameters->Polarity) {

    case InterruptActiveHigh:
        PolarityValue |= (1 << PinNumber);
        break;

    case InterruptActiveLow:
        PolarityValue &= ~(1 << PinNumber);
        break;
    }

    //
    // Clear any stale status bits from the previous configuration.
    //
    // The status register is atomically modified on the controller. It takes
    // a bitmask on the bits to be cleared.
    //

    StatusRegisterValue = (1 << PinNumber);
    Status = SimGpioSpbWriteByte(GpioBank, StatusRegister, StatusRegisterValue);
    if (!NT_SUCCESS(Status)) {
        TraceEvents(
                TRACE_LEVEL_ERROR,
                TRACE_FLAG_INIT,
                "%s: SimGpioSpbWriteByte(StatusRegister) failed! Status = %#x\n",
                __FUNCTION__,
                Status);

        goto ReconfigureInterruptEnd;
    }

    //
    // Write the new values for the interrupt mode register, polarity
    // register, status register and enable register.
    //

    Status = SimGpioSpbWriteByte(GpioBank, ModeRegister, ModeValue);
    if (!NT_SUCCESS(Status)) {
        TraceEvents(
                TRACE_LEVEL_ERROR,
                TRACE_FLAG_INIT,
                "%s: SimGpioSpbWriteByte(ModeRegister) failed! Status = %#x\n",
                __FUNCTION__,
                Status);

        goto ReconfigureInterruptEnd;
    }

    Status = SimGpioSpbWriteByte(GpioBank, PolarityRegister, PolarityValue);
    if (!NT_SUCCESS(Status)) {
        TraceEvents(
                TRACE_LEVEL_ERROR,
                TRACE_FLAG_INIT,
                "%s: SimGpioSpbWriteByte(PolarityRegister) failed! Status = %#x\n",
                __FUNCTION__,
                Status);

        goto ReconfigureInterruptEnd;
    }

ReconfigureInterruptEnd:
    return Status;
}

//
// --------------------------------------------------------------- I/O Handlers
//

_Must_inspect_result_
_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
SimGpioConnectIoPins (
    _In_ PVOID Context,
    _In_ PGPIO_CONNECT_IO_PINS_PARAMETERS ConnectParameters
    )

/*++

Routine Description:

    This routine invokes connects the specified pins for IO. The pins can
    be read from if connected for input, or written to if connected for
    output.

    N.B. This routine is called at PASSIVE_LEVEL but is not marked as
         PAGED_CODE as it could be executed late in the hibernate or
         early in resume sequence (or the deep-idle sequence).

Arguments:

    Context - Supplies a pointer to the GPIO client driver's device extension.

    ConnectParameters - Supplies a pointer to a structure supplying the
        parameters for connecting the IO pins. Fields description:

        BankId - Supplies the ID for the GPIO bank.

        PinNumberTable - Supplies an array of pins to be connected for IO. The
            pin numbers are 0-based and relative to the GPIO bank.

        PinCount - Supplies the number of pins in the pin number table.

        ConnectMode - Supplies the mode in which the pins should be configured
            (viz. input or output).

        ConnectFlags - Supplies the flags controlling the IO setup. Currently
            no flags are defined.

        PullConfiguration - Supplies the pin pull-up/pull-down configuration.

        DebouceTimeout - Supplies the debounce timeout to be applied. The
            field is in 100th of milli-seconds (i.e., 5.84ms will be supplied
            as 584). Default value is zero, which implies, no debounce.

        DriveStrength - Supplies the drive strength to be applied. The value
            is in 100th of mA (i.e., 1.21mA will be supplied as 121mA).

        VendorData - Supplies an optional pointer to a buffer containing the
            vendor data supplied in the GPIO descriptor. This field will be
            NULL if no vendor data was supplied. This buffer is read-only.

        VendorDataLength - Supplies the length of the vendor data buffer.

        ConnectFlags - Supplies the flag to be used for connect operation.
            Currently no flags are defined.

Return Value:

    NT status code.

Environment:

    Entry IRQL: PASSIVE_LEVEL.

    Synchronization: The GPIO class extension will synchronize this call
        against other passive-level interrupt callbacks (e.g. enable/disable/
        unmask) and IO callbacks.

--*/

{

    PSIM_GPIO_BANK GpioBank;
    PSIM_GPIO_CONTEXT GpioContext;
    ULONG Index;
    PIN_NUMBER PinNumber;
    PPIN_NUMBER PinNumberTable;
    UCHAR PinValue;
    NTSTATUS Status;

    GpioContext = (PSIM_GPIO_CONTEXT)Context;
    GpioBank = &GpioContext->Banks[ConnectParameters->BankId];

    //
    // Read the current direction register value.
    //

    Status = SimGpioSpbReadByte(GpioBank, DirectionRegister, &PinValue);
    if (!NT_SUCCESS(Status)) {
        TraceEvents(
                TRACE_LEVEL_ERROR,
                TRACE_FLAG_INIT,
                "%s: SimGpioSpbReadByte(DirectionRegister) failed! Status = %#x\n",
                __FUNCTION__,
                Status);

        goto ConnectIoPinsEnd;
    }

    //
    // Walk through all the supplied pins and connect them in the specified
    // mode (input or output).
    //

    PinNumberTable = ConnectParameters->PinNumberTable;
    for (Index = 0; Index < ConnectParameters->PinCount; Index += 1) {
        PinNumber = PinNumberTable[Index];

        //
        // If the pins are being connected for input, then set the bit.
        // Otherwise clear the bit.
        //

        if (ConnectParameters->ConnectMode == ConnectModeInput) {
            PinValue |= (1 << PinNumber);

        } else if (ConnectParameters->ConnectMode == ConnectModeOutput) {
            PinValue &= ~(1 << PinNumber);
        }
    }

    Status = SimGpioSpbWriteByte(GpioBank, DirectionRegister, PinValue);
    if (!NT_SUCCESS(Status)) {
        TraceEvents(
                TRACE_LEVEL_ERROR,
                TRACE_FLAG_INIT,
                "%s: SimGpioSpbWriteByte(DirectionRegister) failed! Status = %#x\n",
                __FUNCTION__,
                Status);

        goto ConnectIoPinsEnd;
    }

ConnectIoPinsEnd:
    return Status;
}

_Must_inspect_result_
_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
SimGpioDisconnectIoPins (
    _In_ PVOID Context,
    _In_ PGPIO_DISCONNECT_IO_PINS_PARAMETERS DisconnectParameters
    )

/*++

Routine Description:

    This routine invokes disconnects the specified IO pins. The pins are
    put back in their original mode.

    N.B. This routine is called at PASSIVE_LEVEL but is not marked as
         PAGED_CODE as it could be executed late in the hibernate or
         early in resume sequence (or the deep-idle sequence).

Arguments:

    Context - Supplies a pointer to the GPIO client driver's device extension.

    DisconnectParameters - Supplies a pointer to a structure containing
        disconnect operation parameters. Fields are:

        BankId - Supplies the ID for the GPIO bank.

        PinNumberTable - Supplies an array of pins to be disconnected. The pin
            numbers are relative to the GPIO bank.

        PinCount - Supplies the number of pins in the pin number table.

        DisconnectMode - Supplies the mode in which the pins are currently
            configured (viz. input or output).

        DisconnectFlags - Supplies the flags controlling the IO setup. Currently
            no flags are defined.

Return Value:

    NT status code.

Environment:

    Entry IRQL: PASSIVE_LEVEL.

    Synchronization: The GPIO class extension will synchronize this call
        against other passive-level interrupt callbacks (e.g. enable/disable/
        unmask) and IO callbacks.

--*/

{

    PSIM_GPIO_BANK GpioBank;
    PSIM_GPIO_CONTEXT GpioContext;
    ULONG Index;
    PIN_NUMBER PinNumber;
    PPIN_NUMBER PinNumberTable;
    UCHAR PinValue;
    NTSTATUS Status;

    //
    // If the pin configuration should be preserved post disconnect, then
    // there is nothing left to do.
    //

    if (DisconnectParameters->DisconnectFlags.PreserveConfiguration == 1) {
        Status = STATUS_SUCCESS;
        goto DisconnectIoPinsEnd;
    }

    GpioContext = (PSIM_GPIO_CONTEXT)Context;
    GpioBank = &GpioContext->Banks[DisconnectParameters->BankId];

    //
    // Read the current direction register value.
    //

    Status = SimGpioSpbReadByte(GpioBank, DirectionRegister, &PinValue);
    if (!NT_SUCCESS(Status)) {
        TraceEvents(
                TRACE_LEVEL_ERROR,
                TRACE_FLAG_INIT,
                "%s: SimGpioSpbReadByte(DirectionRegister) failed! "
                "Status = %#x\n",
                __FUNCTION__,
                Status);

        goto DisconnectIoPinsEnd;
    }

    //
    // Walk through all the supplied pins and disconnect them. On SimGPIO
    // controller, all pins are reset to the default mode (output).
    //

    PinNumberTable = DisconnectParameters->PinNumberTable;
    for (Index = 0; Index < DisconnectParameters->PinCount; Index += 1) {
        PinNumber = PinNumberTable[Index];
        PinValue &= ~(1 << PinNumber);
    }

    Status = SimGpioSpbWriteByte(GpioBank, DirectionRegister, PinValue);
    if (!NT_SUCCESS(Status)) {
        TraceEvents(
                TRACE_LEVEL_ERROR,
                TRACE_FLAG_INIT,
                "%s: SimGpioSpbWriteByte(DirectionRegister) failed! "
                "Status = %#x\n",
                __FUNCTION__,
                Status);

        goto DisconnectIoPinsEnd;
    }

DisconnectIoPinsEnd:
    return Status;
}

_Must_inspect_result_
NTSTATUS
SimGpioReadGpioPins (
    _In_ PVOID Context,
    _In_ PGPIO_READ_PINS_MASK_PARAMETERS ReadParameters
    )

/*++

Routine Description:

    This routine reads the current values for all the pins.

    As the FormatIoRequestsAsMasks bit was set inside
    SimGpioQueryControllerInformation(), all this routine needs to do is read
    the level register value and return to the GPIO class extension. It will
    return the right set of bits to the caller.

    N.B. This routine is called at PASSIVE_LEVEL for off-SoC GPIOs but is not
         marked as PAGED_CODE as it could be executed late in the hibernate or
         early in resume sequence (or the deep-idle sequence).

Arguments:

    Context - Supplies a pointer to the GPIO client driver's device extension.

    ReadParameters - Supplies a pointer to a structure containing read
        operation parameters. Fields are:

        BankId - Supplies the ID for the GPIO bank.

        PinValues - Supplies a pointer to a variable that receives the current
            pin values.

        Flags - Supplies the flag to be used for read operation. Currently
            defined flags are:

            WriteConfiguredPins: If set, the read is being done on a set of
                pin that were configured for write. In such cases, the
                GPIO client driver is expected to read and return the
                output register value.

Return Value:

    NT status code.

Environment:

    Entry IRQL: PASSIVE_LEVEL.

    Synchronization: The GPIO class extension will synchronize this call
        against other passive-level interrupt callbacks (e.g. enable/disable/
        unmask) and IO callbacks.

--*/

{

    PSIM_GPIO_BANK GpioBank;
    PSIM_GPIO_CONTEXT GpioContext;
    UCHAR PinValue;
    NTSTATUS Status;

    GpioContext = (PSIM_GPIO_CONTEXT)Context;
    GpioBank = &GpioContext->Banks[ReadParameters->BankId];

    //
    // Read the current level register value. Note the GPIO class may invoke
    // the read routine on write-configured pins. In such case the output
    // register values should be read.
    //
    // N.B. In case of SimGPIO, the LevelRegister holds the value for input
    //      as well as output pins. Thus the same register is read in either
    //      case.
    //

    if (ReadParameters->Flags.WriteConfiguredPins == FALSE) {
        Status = SimGpioSpbReadByte(GpioBank, LevelRegister, &PinValue);

    } else {
        Status = SimGpioSpbReadByte(GpioBank, LevelRegister, &PinValue);
    }

    if (!NT_SUCCESS(Status)) {
        TraceEvents(
                TRACE_LEVEL_ERROR,
                TRACE_FLAG_INIT,
                "%s: SimGpioSpbReadByte(DirectionRegister) failed! "
                "Status = %#x\n",
                __FUNCTION__,
                Status);

        goto ReadGpioPinsEnd;
    }

    *ReadParameters->PinValues = PinValue;

ReadGpioPinsEnd:
    return Status;
}

_Must_inspect_result_
NTSTATUS
SimGpioWriteGpioPins (
    _In_ PVOID Context,
    _In_ PGPIO_WRITE_PINS_MASK_PARAMETERS WriteParameters
    )

/*++

Routine Description:

    This routine sets the current values for the specified pins. This call is
    synchronized with the write and connect/disconnect IO calls.

    N.B. This routine is called at PASSIVE_LEVEL for off-SoC GPIOs but is not
         marked as PAGED_CODE as it could be executed late in the hibernate or
         early in resume sequence (or the deep-idle sequence).

Arguments:

    Context - Supplies a pointer to the GPIO client driver's device extension.

    WriteParameters - Supplies a pointer to a structure containing write
        operation parameters. Fields are:

        BankId - Supplies the ID for the GPIO bank.

        SetMask - Supplies a mask of pins which should be set (0x1). If a pin
            should be set, then the corresponding bit is set in the mask.
            All bits that are clear in the mask should be left intact.

        ClearMask - Supplies a mask of pins which should be cleared (0x0). If
            a pin should be cleared, then the bit is set in the bitmask. All
            bits that are clear in the mask should be left intact.

        Flags - Supplies the flag controlling the write operation. Currently
            no flags are defined.

Return Value:

    NT status code.

--*/

{

    PSIM_GPIO_BANK GpioBank;
    PSIM_GPIO_CONTEXT GpioContext;
    UCHAR PinValue;
    NTSTATUS Status;

    GpioContext = (PSIM_GPIO_CONTEXT)Context;
    GpioBank = &GpioContext->Banks[WriteParameters->BankId];

    //
    // Read the current level register value.
    //

    Status = SimGpioSpbReadByte(GpioBank, LevelRegister, &PinValue);
    if (!NT_SUCCESS(Status)) {
        TraceEvents(
                TRACE_LEVEL_ERROR,
                TRACE_FLAG_INIT,
                "%s: SimGpioSpbReadByte(DirectionRegister) failed! "
                "Status = %#x\n",
                __FUNCTION__,
                Status);

        goto WriteGpioPinsEnd;
    }

    //
    // Set the bits specified in the set mask and clear the ones specified
    // in the clear mask.
    //

    PinValue |= WriteParameters->SetMask;
    PinValue &= ~WriteParameters->ClearMask;

    //
    // Write the updated value to the register.
    //

    Status = SimGpioSpbWriteByte(GpioBank, LevelRegister, PinValue);
    if (!NT_SUCCESS(Status)) {
        TraceEvents(
                TRACE_LEVEL_ERROR,
                TRACE_FLAG_INIT,
                "%s: SimGpioSpbWriteByte(DirectionRegister) failed! "
                "Status = %#x\n",
                __FUNCTION__,
                Status);

        goto WriteGpioPinsEnd;
    }

WriteGpioPinsEnd:
    return Status;
}

VOID
SimGpioEvtDriverUnload (
    _In_ WDFDRIVER Driver
    )

/*++

Routine Description:

    This routine is called by WDF to allow final cleanup prior to unloading
    the GPIO class extension. This routine destroys the class library and stops
    tracing.

Arguments:

    Driver - Supplies a handle to a framework driver object.

Return Value:

    None.

--*/

{

    PDRIVER_OBJECT DriverObject;
    NTSTATUS Status;

    PAGED_CODE();

    Status = GPIO_CLX_UnregisterClient(Driver);
    NT_ASSERT(NT_SUCCESS(Status));

    DriverObject = WdfDriverWdmGetDriverObject(Driver);

    TraceEvents(TRACE_LEVEL_INFORMATION,
                TRACE_FLAG_INIT,
                "%s: Driver unloaded!\n",
                __FUNCTION__);

    WPP_CLEANUP(DriverObject);
    return;
}

//
// ------------------------------------------------------- Power mgmt handlers
//

NTSTATUS
SimGpioSaveBankHardwareContext (
    _In_ PVOID Context,
    _In_ BANK_ID BankId
    )

/*++

Routine Description:

    This routine saves the hardware context for the GPIO controller.

    N.B. This function is not marked pageable because this function is in
         the device power up path. It is called at PASSIVE_IRQL though.

Arguments:

    Context - Supplies a pointer to the GPIO client driver's device extension.

    BankId - Supplies the ID of the bank.

Return Value:

    NTSTATUS code.

--*/

{

    PUCHAR DestinationAddress;
    PSIM_GPIO_BANK GpioBank;
    PSIM_GPIO_CONTEXT GpioContext;
    ULONG Index;
    NTSTATUS Status;

    GpioContext = (PSIM_GPIO_CONTEXT)Context;
    GpioBank = &GpioContext->Banks[BankId];
    Status = STATUS_SUCCESS;

    //
    // Loop through all the registers and save them. The status register can
    // only be written to be cleared and thus isn't saved.
    //

    for (Index = 0; Index < MaximumSimGpioAddress; Index += 1) {
        if (Index == StatusRegister) {
            continue;
        }

        DestinationAddress = &GpioBank->SavedRegisterContext[Index];
        Status = SimGpioSpbReadByte(GpioBank, Index, DestinationAddress);
        if (!NT_SUCCESS(Status)) {
            TraceEvents(
                    TRACE_LEVEL_ERROR,
                    TRACE_FLAG_INIT,
                    "%s: SimGpioSpbReadByte(Register = %d) failed! "
                    "Status = %#x\n",
                    __FUNCTION__,
                    Index,
                    Status);

            goto SaveBankHardwareContextEnd;
        }
    }

SaveBankHardwareContextEnd:
    return Status;
}

NTSTATUS
SimGpioRestoreBankHardwareContext (
    _In_ PVOID Context,
    _In_ BANK_ID BankId
    )

/*++

Routine Description:

    This routine saves the hardware context for the GPIO controller.

    N.B. This function is not marked pageable because this function is in
         the device power up path.

Arguments:

    Context - Supplies a pointer to the GPIO client driver's device extension.

    BankId - Supplies the ID of the bank.

Return Value:

    NTSTATUS code.

--*/

{

    PSIM_GPIO_BANK GpioBank;
    PSIM_GPIO_CONTEXT GpioContext;
    ULONG Index;
    NTSTATUS Status;
    UCHAR Value;

    GpioContext = (PSIM_GPIO_CONTEXT)Context;
    GpioBank = &GpioContext->Banks[BankId];
    Status = STATUS_SUCCESS;

    //
    // Loop through all the registers and write them. The order of restore is
    // important. The enable and direction registers need to be programmed
    // after the (mode, polarity) and level registers have been written to.
    // Hence they are restored at the very end.
    //

    for (Index = 0; Index < MaximumSimGpioAddress; Index += 1) {
        if ((Index == EnableRegister) ||
            (Index == DirectionRegister) ||
            (Index == StatusRegister)) {

            continue;
        }

        Value = GpioBank->SavedRegisterContext[Index];
        Status = SimGpioSpbWriteByte(GpioBank, Index, Value);
        if (!NT_SUCCESS(Status)) {
            TraceEvents(
                    TRACE_LEVEL_ERROR,
                    TRACE_FLAG_INIT,
                    "%s: SimGpioSpbWriteByte(Register = %d) failed! "
                    "Status = %#x\n",
                    __FUNCTION__,
                    Index,
                    Status);

            goto RestoreBankHardwareContextEnd;
        }
    }

    //
    // Restore the direction register.
    //

    Value = GpioBank->SavedRegisterContext[DirectionRegister];
    Status = SimGpioSpbWriteByte(GpioBank, DirectionRegister, Value);
    if (!NT_SUCCESS(Status)) {
        TraceEvents(
                TRACE_LEVEL_ERROR,
                TRACE_FLAG_INIT,
                "%s: SimGpioSpbWriteByte(DirectionRegister) failed! Status = %#x\n",
                __FUNCTION__,
                Status);

        goto RestoreBankHardwareContextEnd;
    }

    //
    // Restore the interrupt enable register at the very end.
    //

    Value = GpioBank->SavedRegisterContext[EnableRegister];
    Status = SimGpioSpbWriteByte(GpioBank, EnableRegister, Value);
    if (!NT_SUCCESS(Status)) {
        TraceEvents(
                TRACE_LEVEL_ERROR,
                TRACE_FLAG_INIT,
                "%s: SimGpioSpbWriteByte(EnableRegister) failed! Status = %#x\n",
                __FUNCTION__,
                Status);

        goto RestoreBankHardwareContextEnd;
    }

RestoreBankHardwareContextEnd:
    return Status;
}

__pragma(warning(default: 4127))        // conditional expression is a constant


