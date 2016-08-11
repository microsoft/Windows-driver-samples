
/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    simgpio.c

Abstract:

    This sample implements a GPIO client driver for simulated GPIO (SimGpio)
    controller.

    Note: DIRQL in the comments below refers to device IRQL, which is any
        IRQL > DISPATCH_LEVEL (and less than some IRQL reserved for OS use).


Environment:

    Kernel mode

--*/


//
// The SimGpio controller has 4 GPIO banks, with banks consisting of
// 32 pins each. Any pin can be configured for interrupt, input or output.
// Each bank has the the following registers. The register set for different
// banks are laid out adjacent to one another in the physical memory map.
//
//      Register        |   Offset      |   Size
// ------------------------------------------------
// ModeRegister         |    0x0        |   4 bytes
// PolarityRegister     |    0x4        |   8 bytes
// EnableRegister       |    0xC        |   4 bytes
// StatusRegister       |    0x10       |   4 bytes
// DirectionRegister    |    0x14       |   4 bytes
// LevelRegister        |    0x18       |   4 bytes
//
// Mode Register - Interrupt mode (Level = 0x1 or Edge = 0x0) for pins
//      enabled for interrupts.
//
// Polarity Register - ActiveLow (0x0) or ActiveHigh (0x1) for Level-triggered
//      interrupts; Falling-edge (0x0), Rising-edge (0x1), or Both-Edges (0x3)
//      for Edge-triggered interrupts. Note there are two bits per pin.
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

//
// -------------------------------------------------------------------- Defines
//

//
// Define total number of pins on the simulated GPIO controller.
//

#define SIM_GPIO_TOTAL_PINS (4 * 32)
#define SIM_GPIO_PINS_PER_BANK (32)
#define SIM_GPIO_TOTAL_BANKS (4)

//
// Pool tag for SimGpio allocations.
//

#define SIM_GPIO_POOL_TAG 'GmiS'

//
// Define that controls whether f-state based power management will be supported
// or not by the client driver.
//

#define ENABLE_F_STATE_POWER_MGMT

//
// Define F1 state residency (in sec), latency values (in sec) and nominal
// power.
//
// This sample demonstrates F-state management effected as part of a critical
// system transition (when the system enters connected-standby). To prevent a
// GPIO bank from being transitioned into a low power state as part of runtime
// idle power management (by mini-PEP), specify very high values for residency
// and transition latency.
//
// If GPIO banks need to be runtime power managed, then specify more reasonable
// values below.
//

#ifdef ENABLE_F_STATE_POWER_MGMT

#define SIM_GPIO_F1_NOMINAL_POWER (0)
#define SIM_GPIO_F1_RESIDENCY (8*60*60)          // 8hrs
#define SIM_GPIO_F1_TRANSITION (1*60*60)         // 1hr

#endif

//
// Macro for pointer arithmetic.
//

#define Add2Ptr(Ptr, Value) ((PVOID)((PUCHAR)(Ptr) + (Value)))

//
// Determine whether the given pin is reserved or not. Currently no pins are
// reserved on the simulated GPIO controller.
//

__pragma(warning(disable: 4127))        // conditional expression is a constant

//
// ---------------------------------------------------------------------- Types
//

//
// Define the registers within the SimGPIO controller. There are 32 pins per
// controller. Note this is a logical device and thus may correspond to a
// physical bank or module if the GPIO controller in hardware has more than
// 32 pins.
//



typedef struct _SIM_GPIO_REGISTERS {
    ULONG ModeRegister;
    ULONG PolarityRegister[2];
    ULONG EnableRegister;
    ULONG StatusRegister;
    ULONG DirectionRegister;
    ULONG LevelRegister;
} SIM_GPIO_REGISTERS, *PSIM_GPIO_REGISTERS;

typedef struct _SIM_GPIO_BANK {
    LARGE_INTEGER PhysicalBaseAddress;
    PSIM_GPIO_REGISTERS Registers;
    ULONG Length;
    SIM_GPIO_REGISTERS SavedContext;
} SIM_GPIO_BANK, *PSIM_GPIO_BANK;

//
// The SimGPIO client driver device extension.
//

typedef struct _SIM_GPIO_CONTEXT {
    USHORT TotalPins;
    LARGE_INTEGER PhysicalBaseAddress;
    PSIM_GPIO_REGISTERS ControllerBase;
    ULONG Length;
    SIM_GPIO_BANK Banks[SIM_GPIO_TOTAL_BANKS];
} SIM_GPIO_CONTEXT, *PSIM_GPIO_CONTEXT;

SIM_GPIO_REGISTERS GlobalGpioRegisters[SIM_GPIO_TOTAL_BANKS] = {0};

//
// ----------------------------------------------------------------- Prototypes
//

DRIVER_INITIALIZE DriverEntry;

EVT_WDF_DRIVER_UNLOAD SimGpioEvtDriverUnload;
EVT_WDF_DRIVER_DEVICE_ADD SimGpioEvtDeviceAdd;

//
// General interfaces.
//

GPIO_CLIENT_PREPARE_CONTROLLER SimGpioPrepareController;
GPIO_CLIENT_RELEASE_CONTROLLER SimGpioReleaseController;
GPIO_CLIENT_QUERY_CONTROLLER_BASIC_INFORMATION
    SimGpioQueryControllerBasicInformation;

GPIO_CLIENT_QUERY_SET_CONTROLLER_INFORMATION
    SimGpioQuerySetControllerInformation;

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
// Handlers for save and restore hardware context callbacks.
//

GPIO_CLIENT_SAVE_BANK_HARDWARE_CONTEXT SimGpioSaveBankHardwareContext;
GPIO_CLIENT_RESTORE_BANK_HARDWARE_CONTEXT SimGpioRestoreBankHardwareContext;

//
// -------------------------------------------------------------------- Pragmas
//

#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE, SimGpioEvtDriverUnload)
#pragma alloc_text(PAGE, SimGpioEvtDeviceAdd)

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

    //
    // The query/set handler is required in this sample only if F-state
    // power management is enabled to indicate which banks support f-state
    // based power maanagement.
    //

#ifdef ENABLE_F_STATE_POWER_MGMT

    RegistrationPacket.CLIENT_QuerySetControllerInformation =
        SimGpioQuerySetControllerInformation;

#endif

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
    // Handlers for GPIO save and restore context (if F-state power mgmt is
    // supported).
    //

#ifdef ENABLE_F_STATE_POWER_MGMT

    RegistrationPacket.CLIENT_SaveBankHardwareContext = SimGpioSaveBankHardwareContext;
    RegistrationPacket.CLIENT_RestoreBankHardwareContext = SimGpioRestoreBankHardwareContext;

#endif

    //
    // Register the SimGPIO client driver with the GPIO class extension.
    //

    Status = GPIO_CLX_RegisterClient(Driver, &RegistrationPacket, RegistryPath);

DriverEntryEnd:
    return Status;
}

VOID
SimGpioEvtDriverUnload (
    _In_ WDFDRIVER Driver
    )

/*++

Routine Description:

    This routine is called by WDF to allow final cleanup prior to unloading the
    SimGPIO client driver. This routine unregisters the client driver from the
    class extension.

Arguments:

    Driver - Supplies a handle to a framework driver object.

Return Value:

    None.

--*/

{

    NTSTATUS Status;

    PAGED_CODE();

    Status = GPIO_CLX_UnregisterClient(Driver);
    NT_ASSERT(NT_SUCCESS(Status));
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
        goto EvtDeviceAddEnd;
    }

    //
    // Call the framework to create the device and attach it to the lower stack.
    //

    Status = WdfDeviceCreate(&DeviceInit, &FdoAttributes, &Device);
    if (!NT_SUCCESS(Status)) {
        goto EvtDeviceAddEnd;
    }

    //
    // Call the GPIO class extension's post-device create interface.
    //

    Status = GPIO_CLX_ProcessAddDevicePostDeviceCreate(Driver, Device);
    if (!NT_SUCCESS(Status)) {
        goto EvtDeviceAddEnd;
    }

EvtDeviceAddEnd:
    return Status;
}

_IRQL_requires_(PASSIVE_LEVEL)
VOID
SimGpiopUnmapControllerBase (
    _In_ PSIM_GPIO_CONTEXT GpioContext
    )

/*++

Routine Description:

    This routine releases the memory mapping for the GPIO controller's
    registers, if one has been established.

    N.B. This function is not marked pageable because this function is in
         the device power down path.

Arguments:

    Context - Supplies a pointer to the GPIO client driver's device extension.

Return Value:

    NTSTATUS code.

--*/

{

    if ((GpioContext->ControllerBase != NULL) &&
        (GpioContext->ControllerBase != GlobalGpioRegisters)) {
        MmUnmapIoSpace(GpioContext->ControllerBase, GpioContext->Length);
        GpioContext->ControllerBase = NULL;
    }
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

    BANK_ID BankId;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR Descriptor;
    PSIM_GPIO_BANK GpioBank;
    PSIM_GPIO_CONTEXT GpioContext;
    ULONG Index;
    ULONG InterruptResourceCount;
    ULONG MemoryResourceCount;
    ULONG RequiredSize;
    ULONG Offset;
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

    InterruptResourceCount = 0;
    MemoryResourceCount = 0;
    ResourceCount = WdfCmResourceListGetCount(ResourcesTranslated);
    Status = STATUS_SUCCESS;
    for (Index = 0; Index < ResourceCount; Index += 1) {
        Descriptor = WdfCmResourceListGetDescriptor(ResourcesTranslated, Index);
        switch(Descriptor->Type) {

        //
        // The memory resource supplies the physical register base for the GPIO
        // controller. Map it virtually as non-cached.
        //

        case CmResourceTypeMemory:
            if (MemoryResourceCount == 0) {
                RequiredSize = SIM_GPIO_TOTAL_BANKS * sizeof(SIM_GPIO_REGISTERS);

                NT_ASSERT(Descriptor->u.Memory.Length >= RequiredSize);

                if (Descriptor->u.Memory.Length < RequiredSize) {
                    Status = STATUS_UNSUCCESSFUL;
                    break;
                }

                GpioContext->PhysicalBaseAddress = Descriptor->u.Memory.Start;
                GpioContext->ControllerBase =
                    (PSIM_GPIO_REGISTERS)MmMapIoSpaceEx(
                        Descriptor->u.Memory.Start,
                        RequiredSize,
                        PAGE_NOCACHE | PAGE_READWRITE);

                //
                // Fail initialization if mapping of the memory region failed.
                //

                if (GpioContext->ControllerBase == NULL) {
                    Status = STATUS_UNSUCCESSFUL;
                }

                GpioContext->Length = RequiredSize;
            }

            MemoryResourceCount += 1;
            break;

        //
        // IO port resources are unexpected, fail initialization.
        //

        case CmResourceTypePort:
            Status = STATUS_UNSUCCESSFUL;
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
    // NOTE: As SimGPIO is not a real hardware device, it doesn't really have
    //       any physical registers. For test purposes, fake the GPIO registers
    //       using a software defined global structure. This should NOT be done
    //       for a real GPIO controller
    //
    // BEGIN: SIMGPIO HACK.
    //

    if (MemoryResourceCount == 0) {
        GpioContext->ControllerBase = GlobalGpioRegisters;
        MemoryResourceCount = 1;
    }

    //
    // END: SIMGPIO HACK.
    //

    //
    // Fail initialization if minimum number of interrupt
    // and memory resources were not described correctly.
    //

    if ((InterruptResourceCount < SIM_GPIO_TOTAL_BANKS) ||
        (MemoryResourceCount < 1)) {
        Status = STATUS_UNSUCCESSFUL;
        goto PrepareControllerEnd;
    }

    //
    // Initialize the base address of registers per bank.
    //

    for (BankId = 0; BankId < SIM_GPIO_TOTAL_BANKS; BankId += 1) {
        GpioBank = &GpioContext->Banks[BankId];
        Offset = BankId * sizeof(SIM_GPIO_REGISTERS);
        GpioBank->Registers = Add2Ptr(GpioContext->ControllerBase, Offset);
        GpioBank->Length = sizeof(SIM_GPIO_REGISTERS);
    }

PrepareControllerEnd:
    if (!NT_SUCCESS(Status)) {
        SimGpiopUnmapControllerBase(GpioContext);
    }

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
    UNREFERENCED_PARAMETER(Context);

    //
    // Release the mappings established in the initialize callback.
    //
    // N.B. Disconnecting of the interrupt is handled by the GPIO class
    //      extension.
    //

    SimGpiopUnmapControllerBase((PSIM_GPIO_CONTEXT)Context);
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
    // Indicate that the GPIO controller is memory-mapped and thus can be
    // manipulated at DIRQL.
    //
    // N.B. If the GPIO controller is off-SOC behind some serial bus like
    //      I2C or SPI, then this field must be set to FALSE.
    //

    ControllerInformation->Flags.MemoryMappedController = TRUE;

    //
    // Indicate that the H/W registers used for I/O can be accessed seperately
    // from the registers used for interrupt processing.
    //
    // N.B.: Setting this flag to TRUE will cause the GPIO class extension
    // to optimize I/O processing by skipping the acquisition of
    // interrupt-related locks in I/O paths. Make sure the GPIO controller
    // H/W can support simultaneous I/O and interrupt processing. Otherwise
    // unpredictable race conditions may occur!
    //

    ControllerInformation->Flags.IndependentIoHwSupported = TRUE;

    //
    // Indicate that status register must be cleared explicitly.
    //

    ControllerInformation->Flags.ActiveInterruptsAutoClearOnRead = 0;

    //
    // Indicate that the client driver would like to receive IO requests as a
    // set of bitmasks as that maps directly to the register operations.
    //

    ControllerInformation->Flags.FormatIoRequestsAsMasks = 1;

    //
    // Indicate that the GPIO controller does not support controller-level
    // D-state power management.
    //

    ControllerInformation->Flags.DeviceIdlePowerMgmtSupported = FALSE;

    //
    // Note if bank-level F-state power management is supported, then specify
    // as such. Note the QuerySetControllerInformation() handler also needs to
    // be implemented in this case.
    //

#ifdef ENABLE_F_STATE_POWER_MGMT

    ControllerInformation->Flags.BankIdlePowerMgmtSupported = TRUE;

#else

    ControllerInformation->Flags.BankIdlePowerMgmtSupported = FALSE;

#endif

    //
    // Note the IdleTimeout parameter does not need to be initialized if
    // D-state power management is not supported.
    //
    // ControllerInformation->IdleTimeout = IdleTimeoutDefaultValue;
    //

    //
    // Note if the GPIO controller does not support hardware debouncing and
    // software-debouncing should be used instead, set the EmulateDebouncing
    // flag.
    //
    // ControllerInformation->Flags.EmulateDebouncing = TRUE;
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
SimGpioQuerySetControllerInformation (
    _In_ PVOID Context,
    _In_ PCLIENT_CONTROLLER_QUERY_SET_INFORMATION_INPUT InputBuffer,
    _Out_opt_ PCLIENT_CONTROLLER_QUERY_SET_INFORMATION_OUTPUT OutputBuffer
    )

/*++

Routine Description:

    This routine is the generic GPIO query/set handler. Currently it only
    supports returning bank power information.

Arguments:

    Context - Supplies a pointer to the GPIO client driver's device extension.

    InputBuffer - Supplies a pointer to a buffer that receives the parameters
        for the query or set operation.

    OutputBuffer - Supplies a pointer to the GPIO class extension allocated
        buffer to return the output values. Note on entry, the
        OutputBuffer->Size indicates how big the output buffer is. On exit, the
        OutputBuffer->Size indicates the filled-in size or required size.

Return Value:

    NTSTATUS code.

--*/

{

    PPO_FX_COMPONENT_IDLE_STATE F1Parameters;
    NTSTATUS Status;

    UNREFERENCED_PARAMETER(Context);

    if((InputBuffer == NULL) || (OutputBuffer == NULL)) {
        Status = STATUS_NOT_SUPPORTED;
        goto QuerySetControllerInformationEnd;
    }

    if (InputBuffer->RequestType != QueryBankPowerInformation) {
        Status = STATUS_NOT_SUPPORTED;
        goto QuerySetControllerInformationEnd;
    }

    //
    // Set the version and size of the output buffer.
    //

    OutputBuffer->Version = GPIO_BANK_POWER_INFORMATION_OUTPUT_VERSION;
    OutputBuffer->Size = sizeof(CLIENT_CONTROLLER_QUERY_SET_INFORMATION_OUTPUT);

    //
    // Mark the given bank (InputBuffer->BankPowerInformation.BankId) as
    // supporting F1 state. Since all banks support it, the BankId is not
    // checked.
    //

    OutputBuffer->BankPowerInformation.F1StateSupported = TRUE;

    //
    // Supply the attributes for the F1 power state.
    //

    F1Parameters = &OutputBuffer->BankPowerInformation.F1IdleStateParameters;
    F1Parameters->NominalPower = SIM_GPIO_F1_NOMINAL_POWER;
    F1Parameters->ResidencyRequirement =
        WDF_ABS_TIMEOUT_IN_SEC(SIM_GPIO_F1_RESIDENCY);

    F1Parameters->TransitionLatency =
        WDF_ABS_TIMEOUT_IN_SEC(SIM_GPIO_F1_TRANSITION);

    Status = STATUS_SUCCESS;

QuerySetControllerInformationEnd:
    return Status;
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
    PSIM_GPIO_BANK GpioBank;
    PSIM_GPIO_CONTEXT GpioContext;
    ULONG PinValue;
    GPIO_SAVE_RESTORE_BANK_HARDWARE_CONTEXT_PARAMETERS RestoreParameters;
    PSIM_GPIO_REGISTERS SimGpioRegisters;

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
    if (RestoreContext == FALSE) {
        for (BankId = 0; BankId < SIM_GPIO_TOTAL_BANKS; BankId += 1) {
            GpioBank = &GpioContext->Banks[BankId];
            SimGpioRegisters = GpioBank->Registers;

            //
            // Disable all interrupts on this bank by clearing the enable
            // register.
            //

            PinValue = READ_REGISTER_ULONG(&SimGpioRegisters->EnableRegister);
            if (PinValue > 0) {
                PinValue = 0;
                WRITE_REGISTER_ULONG(&SimGpioRegisters->EnableRegister,
                                     PinValue);
            }
        }

    } else {

        //
        // Restoring the controller state involves restoring the state of
        // each SimGPIO bank.
        //

        for (BankId = 0; BankId < SIM_GPIO_TOTAL_BANKS; BankId += 1) {
            RestoreParameters.BankId = BankId;
            RestoreParameters.State = PreviousPowerState;
            SimGpioRestoreBankHardwareContext(Context, &RestoreParameters);
        }
    }

    return STATUS_SUCCESS;
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
         the device power down path.

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
    GPIO_SAVE_RESTORE_BANK_HARDWARE_CONTEXT_PARAMETERS SaveParameters;

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
    if (SaveContext == TRUE) {
        for (BankId = 0; BankId < SIM_GPIO_TOTAL_BANKS; BankId += 1) {
            SaveParameters.BankId = BankId;
            SaveParameters.State = TargetState;
            SimGpioSaveBankHardwareContext(Context, &SaveParameters);
        }
    }

    return STATUS_SUCCESS;
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

    N.B. This routine is called from within a regular thread context (i.e.,
         non-interrupt context) by the class extension. Thus the interrupt lock
         needs to be explicitly acquired for memory-mapped GPIO controllers
         prior to manipulating any device state that is also affected from a
         routine called within the interrupt context.

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
    PSIM_GPIO_BANK GpioBank;
    PSIM_GPIO_CONTEXT GpioContext;
    ULONG Index;
    PIN_NUMBER PinNumber;
    ULONG PinValue;
    PIN_NUMBER ShiftBits;
    PSIM_GPIO_REGISTERS SimGpioRegisters;
    NTSTATUS Status;

    //
    // If the polarity is not supported, then bail out. Note the interrupt
    // polarity cannot be InterruptActiveBoth as this sample uses ActiveBoth
    // emulation.
    //

    if ((EnableParameters->Polarity != InterruptActiveHigh) &&
        (EnableParameters->Polarity != InterruptActiveLow)) {

        Status = STATUS_NOT_SUPPORTED;
        goto EnableInterruptEnd;
    }

    BankId = EnableParameters->BankId;
    PinNumber = EnableParameters->PinNumber;
    GpioContext = (PSIM_GPIO_CONTEXT)Context;
    GpioBank = &GpioContext->Banks[BankId];
    SimGpioRegisters = GpioBank->Registers;
    Status = STATUS_SUCCESS;

    //
    // The interrupt enable register/bitmap may be manipulated from within the
    // interrupt context. Hence updates to it must be synchronized using the
    // interrupt lock.
    //

    GPIO_CLX_AcquireInterruptLock(Context, BankId);

    //
    // Set the mode register. If the interrupt is Level then set the bit;
    // otherwise, clear it (edge-triggered).
    //

    PinValue = READ_REGISTER_ULONG(&SimGpioRegisters->ModeRegister);
    if (EnableParameters->InterruptMode == LevelSensitive) {
        PinValue |= (1 << PinNumber);
    } else {
        PinValue &= ~(1 << PinNumber);
    }

    WRITE_REGISTER_ULONG(&SimGpioRegisters->ModeRegister, PinValue);

    //
    // Set the polarity register. There are two bits for each pin. If the
    // interrupt is ActiveHigh (or Rising-edge) then set it to 0x1.
    //

    Index = PinNumber / 16;
    ShiftBits = (PinNumber % 16) * 2;
    PinValue = READ_REGISTER_ULONG(&SimGpioRegisters->PolarityRegister[Index]);
    switch (EnableParameters->Polarity) {

    case InterruptActiveHigh:
        PinValue &= ~(0x3 << ShiftBits);
        PinValue |= (1 << ShiftBits);
        break;

    case InterruptActiveLow:
        PinValue &= ~(0x3 << ShiftBits);
        break;
    }

    WRITE_REGISTER_ULONG(&SimGpioRegisters->PolarityRegister[Index], PinValue);

    //
    // NOTE: If the GPIO controller supports a separate set of mask registers,
    //       then any stale value must be cleared here. SimGPIO controller
    //       doesn't and hence this step is skipped here.
    //

    //
    // Clear the corresponding status bit first to ignore any stale value.
    //

    PinValue = READ_REGISTER_ULONG(&SimGpioRegisters->StatusRegister);
    PinValue &= ~(1 << PinNumber);
    WRITE_REGISTER_ULONG(&SimGpioRegisters->StatusRegister, PinValue);

    //
    // Enable the interrupt by setting the bit in the interrupt enable register.
    //

    PinValue = READ_REGISTER_ULONG(&SimGpioRegisters->EnableRegister);
    PinValue |= (1 << PinNumber);
    WRITE_REGISTER_ULONG(&SimGpioRegisters->EnableRegister, PinValue);

    //
    // Release the interrupt lock.
    //

    GPIO_CLX_ReleaseInterruptLock(Context, BankId);

    Status = STATUS_SUCCESS;

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

    N.B. This routine is called from within a regular thread context (i.e.,
         non-interrupt context) by the class extension. Thus the interrupt lock
         needs to be explicitly acquired prior to manipulating the device state.

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

    NTSTATUS code (STATUS_SUCCESS always for memory-mapped GPIO controllers).

Environment:

    Entry IRQL: PASSIVE_LEVEL.

    Synchronization: The GPIO class extension will synchronize this call
        against other passive-level interrupt callbacks (e.g. enable/disable/
        unmask) and IO callbacks.

--*/

{

    BANK_ID BankId;
    PSIM_GPIO_BANK GpioBank;
    PSIM_GPIO_CONTEXT GpioContext;
    ULONG PinValue;
    PSIM_GPIO_REGISTERS SimGpioRegisters;

    BankId = DisableParameters->BankId;
    GpioContext = (PSIM_GPIO_CONTEXT)Context;
    GpioBank = &GpioContext->Banks[BankId];
    SimGpioRegisters = GpioBank->Registers;

    //
    // The interrupt enable register may be manipulated from within the
    // interrupt context. Hence updates to it must be synchronized using the
    // interrupt lock.
    //

    GPIO_CLX_AcquireInterruptLock(Context, BankId);

    //
    // Disable the interrupt by clearing the bit in the interrupt enable
    // register.
    //

    PinValue = READ_REGISTER_ULONG(&SimGpioRegisters->EnableRegister);
    PinValue &= ~(1 << DisableParameters->PinNumber);
    WRITE_REGISTER_ULONG(&SimGpioRegisters->EnableRegister, PinValue);

    GPIO_CLX_ReleaseInterruptLock(Context, BankId);

    return STATUS_SUCCESS;
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

    NTSTATUS code (STATUS_SUCCESS always for memory-mapped GPIO controllers).

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

    PSIM_GPIO_BANK GpioBank;
    PSIM_GPIO_CONTEXT GpioContext;
    ULONG PinValue;
    PSIM_GPIO_REGISTERS SimGpioRegisters;

    GpioContext = (PSIM_GPIO_CONTEXT)Context;
    GpioBank = &GpioContext->Banks[MaskParameters->BankId];
    SimGpioRegisters = GpioBank->Registers;

    //
    // Mask is essentially same as disable for SimGPIO controller. The
    // difference between the routines is that mask callback is called at DIRQL
    // and automatically synchronized with other DIRQL interrupts callbacks.
    //

    PinValue = READ_REGISTER_ULONG(&SimGpioRegisters->EnableRegister);
    PinValue &= ~(ULONG)(MaskParameters->PinMask);
    WRITE_REGISTER_ULONG(&SimGpioRegisters->EnableRegister, PinValue);

    //
    // Set the bitmask of pins that could not be successfully masked.
    // Since this is a memory-mapped controller, the mask operation always
    // succeeds.
    //

    MaskParameters->FailedMask = 0x0;
    return STATUS_SUCCESS;
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

    NTSTATUS code (STATUS_SUCCESS always for memory-mapped GPIO controllers).

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

    PSIM_GPIO_BANK GpioBank;
    PSIM_GPIO_CONTEXT GpioContext;
    ULONG PinValue;
    PSIM_GPIO_REGISTERS SimGpioRegisters;

    GpioContext = (PSIM_GPIO_CONTEXT)Context;
    GpioBank = &GpioContext->Banks[UnmaskParameters->BankId];
    SimGpioRegisters = GpioBank->Registers;

    //
    // Unmask is same as enable on this GPIO controller. The difference between
    // this routine and the enable routine is that unmask callback is called at
    // DIRQL and automatically synchronized with other DIRQL-level callbacks.
    //

    PinValue = READ_REGISTER_ULONG(&SimGpioRegisters->EnableRegister);
    PinValue |= (1 << UnmaskParameters->PinNumber);
    WRITE_REGISTER_ULONG(&SimGpioRegisters->EnableRegister, PinValue);

    return STATUS_SUCCESS;
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

    NTSTATUS code (STATUS_SUCCESS always for memory-mapped GPIO controllers).

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
    ULONG PinValue;
    PSIM_GPIO_REGISTERS SimGpioRegisters;

    GpioContext = (PSIM_GPIO_CONTEXT)Context;
    GpioBank = &GpioContext->Banks[QueryActiveParameters->BankId];
    SimGpioRegisters = GpioBank->Registers;

    //
    // NOTE: As SimGPIO is not a real hardware device, no interrupt will ever
    //       fire. Thus the status register value will never change. To pretend
    //       as if a real interrupt happened, it marks all currently enabled
    //       interrupts as asserting. Copy the enable interrupt value into
    //       the status register.
    //
    //       This should NOT be done for a real GPIO controller.
    //

    //
    // BEGIN: SIMGPIO HACK.
    //

    PinValue = READ_REGISTER_ULONG(&SimGpioRegisters->EnableRegister);
    WRITE_REGISTER_ULONG(&SimGpioRegisters->StatusRegister, PinValue);

    //
    // END: SIMGPIO HACK.
    //

    //
    // Return the current value of the interrupt status register into the
    // ActiveMask parameter.
    //

    PinValue = READ_REGISTER_ULONG(&SimGpioRegisters->StatusRegister);
    QueryActiveParameters->ActiveMask = (ULONG64)PinValue;
    return STATUS_SUCCESS;
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

    NTSTATUS code (STATUS_SUCCESS always for memory-mapped GPIO controllers).

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

    PSIM_GPIO_BANK GpioBank;
    PSIM_GPIO_CONTEXT GpioContext;
    ULONG PinValue;
    PSIM_GPIO_REGISTERS SimGpioRegisters;

    GpioContext = (PSIM_GPIO_CONTEXT)Context;
    GpioBank = &GpioContext->Banks[QueryEnabledParameters->BankId];
    SimGpioRegisters = GpioBank->Registers;

    //
    // Return the current value of the interrupt enable register into the
    // EnabledMask parameter. It is strongly preferred that the true state of
    // the hardware is returned, rather than a software-cached variable, since
    // CLIENT_QueryEnabledInterrupts is used by the class extension to detect
    // interrupt storms.
    //

    PinValue = READ_REGISTER_ULONG(&SimGpioRegisters->EnableRegister);
    QueryEnabledParameters->EnabledMask = (ULONG64)PinValue;
    return STATUS_SUCCESS;
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

        FailedMask - Supplies a bitmask of pins that failed to be cleared. If
            a pin could not be cleared, the bit should be set in this field.

            N.B. This should only be done if for non memory-mapped controllers.
                 Memory-mapped controllers are never expected to fail this
                 operation.

Return Value:

    NTSTATUS code (STATUS_SUCCESS always for memory-mapped GPIO controllers).

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
    ULONG PinValue;
    PSIM_GPIO_REGISTERS SimGpioRegisters;

    GpioContext = (PSIM_GPIO_CONTEXT)Context;
    GpioBank = &GpioContext->Banks[ClearParameters->BankId];
    SimGpioRegisters = GpioBank->Registers;

    //
    // Clear the bits that are set in the ClearActiveMask parameter.
    //

    PinValue = READ_REGISTER_ULONG(&SimGpioRegisters->StatusRegister);
    PinValue &= ~((ULONG)ClearParameters->ClearActiveMask);
    WRITE_REGISTER_ULONG(&SimGpioRegisters->StatusRegister, PinValue);

    //
    // Set the bitmask of pins that could not be successfully cleared.
    // Since this is a memory-mapped controller, the clear operation always
    // succeeds.
    //

    ClearParameters->FailedClearMask = 0x0;
    return STATUS_SUCCESS;
}

NTSTATUS
SimGpioReconfigureInterrupt (
    _In_ PVOID Context,
    _In_ PGPIO_RECONFIGURE_INTERRUPTS_PARAMETERS ReconfigureParameters
    )

/*++

Routine Description:

    This routine reconfigures the interrupt in the specified mode.

    N.B. This routine is called with the interrupt lock acquired by the
         class extension. Hence the lock is not re-acquired here.

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
    ULONG Index;
    PIN_NUMBER PinNumber;
    ULONG PinValue;
    PIN_NUMBER ShiftBits;
    PSIM_GPIO_REGISTERS SimGpioRegisters;
    NTSTATUS Status;

    BankId = ReconfigureParameters->BankId;
    PinNumber = ReconfigureParameters->PinNumber;
    GpioContext = (PSIM_GPIO_CONTEXT)Context;
    GpioBank = &GpioContext->Banks[BankId];
    SimGpioRegisters = GpioBank->Registers;
    Status = STATUS_SUCCESS;

    //
    // Clear any stale status bits from the previous configuration.
    //

    PinValue = READ_REGISTER_ULONG(&SimGpioRegisters->StatusRegister);
    PinValue &= ~(1 << PinNumber);
    WRITE_REGISTER_ULONG(&SimGpioRegisters->StatusRegister, PinValue);

    //
    // Set the mode register. If the interrupt is Level then set the bit;
    // otherwise, clear it (edge-triggered).
    //

    PinValue = READ_REGISTER_ULONG(&SimGpioRegisters->ModeRegister);
    if (ReconfigureParameters->InterruptMode == LevelSensitive) {
        PinValue |= (1 << PinNumber);
    } else {
        PinValue &= ~(1 << PinNumber);
    }

    WRITE_REGISTER_ULONG(&SimGpioRegisters->ModeRegister, PinValue);

    //
    // Set the polarity register. There are two bits for each pin. If the
    // interrupt is ActiveHigh (or Rising-edge) then set it to 0x1.
    //

    Index = PinNumber / 16;
    ShiftBits = (PinNumber % 16) * 2;
    PinValue = READ_REGISTER_ULONG(&SimGpioRegisters->PolarityRegister[Index]);
    switch (ReconfigureParameters->Polarity) {

    case InterruptActiveHigh:
        PinValue &= ~(0x3 << ShiftBits);
        PinValue |= (1 << ShiftBits);
        break;

    case InterruptActiveLow:
        PinValue &= ~(0x3 << ShiftBits);
        break;

    default:

        NT_ASSERT(FALSE);
    }

    WRITE_REGISTER_ULONG(&SimGpioRegisters->PolarityRegister[Index], PinValue);
    return STATUS_SUCCESS;
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
    ULONG PinValue;
    PSIM_GPIO_REGISTERS SimGpioRegisters;
    NTSTATUS Status;

    GpioContext = (PSIM_GPIO_CONTEXT)Context;
    GpioBank = &GpioContext->Banks[ConnectParameters->BankId];
    SimGpioRegisters = GpioBank->Registers;
    Status = STATUS_SUCCESS;

    //
    // Read the current direction register value.
    //

    PinValue = READ_REGISTER_ULONG(&SimGpioRegisters->DirectionRegister);

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

    WRITE_REGISTER_ULONG(&SimGpioRegisters->DirectionRegister, PinValue);
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

    NTSTATUS code (STATUS_SUCCESS always for memory-mapped GPIO controllers).

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
    ULONG PinValue;
    PSIM_GPIO_REGISTERS SimGpioRegisters;

    //
    // If the pin configuration should be preserved post disconnect, then
    // there is nothing left to do.
    //

    if (DisconnectParameters->DisconnectFlags.PreserveConfiguration == 1) {
        return STATUS_SUCCESS;
    }

    GpioContext = (PSIM_GPIO_CONTEXT)Context;
    GpioBank = &GpioContext->Banks[DisconnectParameters->BankId];
    SimGpioRegisters = GpioBank->Registers;

    //
    // Read the current direction register value.
    //

    PinValue = READ_REGISTER_ULONG(&SimGpioRegisters->DirectionRegister);

    //
    // Walk through all the supplied pins and disconnect them. On SimGPIO
    // controller, all pins are reset to the default mode (output).
    //

    PinNumberTable = DisconnectParameters->PinNumberTable;
    for (Index = 0; Index < DisconnectParameters->PinCount; Index += 1) {
        PinNumber = PinNumberTable[Index];
        PinValue &= ~(1 << PinNumber);
    }

    WRITE_REGISTER_ULONG(&SimGpioRegisters->DirectionRegister, PinValue);

    return STATUS_SUCCESS;
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

    N.B. This routine is called at DIRQL for memory-mapped GPIOs and thus not
         marked as PAGED.

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

    NTSTATUS code (STATUS_SUCCESS always for memory-mapped GPIO controllers).

Environment:

    Entry IRQL: DIRQL if the GPIO controller is memory-mapped;
        PASSIVE_LEVEL if the controller is behind some serial-bus.

    Synchronization: The GPIO class extension will synchronize this call
        against other passive-level interrupt callbacks (e.g. enable/disable)
        and IO callbacks (connect/disconnect).

--*/

{

    PSIM_GPIO_BANK GpioBank;
    PSIM_GPIO_CONTEXT GpioContext;
    ULONG PinValue;
    PSIM_GPIO_REGISTERS SimGpioRegisters;

    GpioContext = (PSIM_GPIO_CONTEXT)Context;
    GpioBank = &GpioContext->Banks[ReadParameters->BankId];
    SimGpioRegisters = GpioBank->Registers;

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
        PinValue = READ_REGISTER_ULONG(&SimGpioRegisters->LevelRegister);

    } else {
        PinValue = READ_REGISTER_ULONG(&SimGpioRegisters->LevelRegister);
    }

    *ReadParameters->PinValues = PinValue;
    return STATUS_SUCCESS;
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

    N.B. This routine is called at DIRQL for memory-mapped GPIOs and thus not
         marked as PAGED.

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

    NTSTATUS code (STATUS_SUCCESS always for memory-mapped GPIO controllers).

Environment:

    Entry IRQL: DIRQL if the GPIO controller is memory-mapped;
        PASSIVE_LEVEL if the controller is behind some serial-bus.

    Synchronization: The GPIO class extension will synchronize this call
        against other passive-level interrupt callbacks (e.g. enable/disable)
        and IO callbacks (connect/disconnect).

--*/

{

    PSIM_GPIO_BANK GpioBank;
    PSIM_GPIO_CONTEXT GpioContext;
    ULONG PinValue;
    PSIM_GPIO_REGISTERS SimGpioRegisters;

    GpioContext = (PSIM_GPIO_CONTEXT)Context;
    GpioBank = &GpioContext->Banks[WriteParameters->BankId];
    SimGpioRegisters = GpioBank->Registers;

    //
    // Read the current level register value.
    //

    PinValue = READ_REGISTER_ULONG(&SimGpioRegisters->LevelRegister);

    //
    // Set the bits specified in the set mask and clear the ones specified
    // in the clear mask.
    //

    PinValue |= WriteParameters->SetMask;
    PinValue &= ~WriteParameters->ClearMask;

    //
    // Write the updated value to the register.
    //

    WRITE_REGISTER_ULONG(&SimGpioRegisters->LevelRegister, PinValue);

    return STATUS_SUCCESS;
}

//
// ------------------------------------------------------- Power mgmt handlers
//

VOID
SimGpioSaveBankHardwareContext (
    _In_ PVOID Context,
    _In_ PGPIO_SAVE_RESTORE_BANK_HARDWARE_CONTEXT_PARAMETERS SaveParameters
    )

/*++

Routine Description:

    This routine saves the hardware context for the GPIO controller.

Arguments:

    Context - Supplies a pointer to the GPIO client driver's device extension.

    SaveRestoreParameters - Supplies a pointer to a structure containing
        parameters for the save operation:

        BankId - Supplies the ID for the GPIO bank.

        State - Target F-state the bank will be transitioned into.

        Flags - Supplies flags for the save operation:
            CriticalTransition - TRUE if this is due to a critical transition.

Return Value:

    None.

--*/

{

    PULONG DestinationAddress;
    PSIM_GPIO_BANK GpioBank;
    PSIM_GPIO_CONTEXT GpioContext;
    ULONG Index;
    ULONG RegisterCount;
    PSIM_GPIO_REGISTERS SimGpioRegisters;
    PULONG SourceAddress;

    GpioContext = (PSIM_GPIO_CONTEXT)Context;
    GpioBank = &GpioContext->Banks[SaveParameters->BankId];
    SimGpioRegisters = GpioBank->Registers;

    //
    // Copy the contents of the registers into memory.
    //

    SourceAddress = &SimGpioRegisters->ModeRegister;
    DestinationAddress = &GpioBank->SavedContext.ModeRegister;
    RegisterCount = sizeof(SIM_GPIO_REGISTERS) / sizeof(ULONG);
    for (Index = 0; Index < RegisterCount; Index += 1) {
        *DestinationAddress = READ_REGISTER_ULONG(SourceAddress);
        SourceAddress += 1;
        DestinationAddress += 1;
    }

    return;
}

VOID
SimGpioRestoreBankHardwareContext (
    _In_ PVOID Context,
    _In_ PGPIO_SAVE_RESTORE_BANK_HARDWARE_CONTEXT_PARAMETERS RestoreParameters
    )

/*++

Routine Description:

    This routine saves the hardware context for the GPIO controller.

Arguments:

    Context - Supplies a pointer to the GPIO client driver's device extension.

    SaveRestoreParameters - Supplies a pointer to a structure containing
        parameters for the restore operation:

        BankId - Supplies the ID for the GPIO bank.

        State - Target F-state the bank will be transitioned into.

        Flags - Supplies flags for the save operation:
            CriticalTransition - TRUE if this is due to a critical transition.

Return Value:

    None.

--*/

{

    PSIM_GPIO_BANK GpioBank;
    PSIM_GPIO_CONTEXT GpioContext;
    ULONG PinValue;
    PSIM_GPIO_REGISTERS SimGpioRegisters;

    GpioContext = (PSIM_GPIO_CONTEXT)Context;
    GpioBank = &GpioContext->Banks[RestoreParameters->BankId];
    SimGpioRegisters = GpioBank->Registers;

    //
    // Restore the level register.
    //

    PinValue = GpioBank->SavedContext.LevelRegister;
    WRITE_REGISTER_ULONG(&SimGpioRegisters->LevelRegister, PinValue);

    //
    // Restore the mode, polarity and enable registers.
    //

    PinValue = GpioBank->SavedContext.ModeRegister;
    WRITE_REGISTER_ULONG(&SimGpioRegisters->ModeRegister, PinValue);

    PinValue = GpioBank->SavedContext.PolarityRegister[0];
    WRITE_REGISTER_ULONG(&SimGpioRegisters->PolarityRegister[0], PinValue);

    PinValue = GpioBank->SavedContext.PolarityRegister[1];
    WRITE_REGISTER_ULONG(&SimGpioRegisters->PolarityRegister[1], PinValue);

    PinValue = GpioBank->SavedContext.StatusRegister;
    WRITE_REGISTER_ULONG(&SimGpioRegisters->StatusRegister, PinValue);

    //
    // Restore the direction register.
    //

    PinValue = GpioBank->SavedContext.DirectionRegister;
    WRITE_REGISTER_ULONG(&SimGpioRegisters->DirectionRegister, PinValue);

    //
    // Take care to restore the enable register only after restoring the
    // mode and polarity registers. Otherwise, the interrupt line will get
    // sampled when the enable register gets written to with the mode and
    // polarity at that point in time (and could cause a spurious interrupt).
    //

    PinValue = GpioBank->SavedContext.EnableRegister;
    WRITE_REGISTER_ULONG(&SimGpioRegisters->EnableRegister, PinValue);

    return;
}

__pragma(warning(default: 4127))        // conditional expression is a constant


