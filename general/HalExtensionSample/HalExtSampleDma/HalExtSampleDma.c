/*++

Copyright (c) 2011 Microsoft Corporation

Module Name:

    HalExtSampleDma.c

Abstract:

    This file implements a HAL Extension Module for the fictitious chDMA
    controller.

Author:

    Cody Hartwig (chartwig) 9-Jun-2011

--*/

//
// --------------------------------------------------------------------Includes
//

#include <nthalext.h>

//
// ---------------------------------------------------------------- Definitions
//

//
// Disable warnings of features used by the standard headers
//
// Disable warning C4214: nonstandard extension used : bit field types other than int
// Disable warning C4201: nonstandard extension used : nameless struct/union
// Disable warning C4115: named type definition in parentheses
// Disable warning C4127: conditional expression is constant
// Disable warning C4200: zero-sized array in struct/union
//

#pragma warning(disable:4214 4201 4115 4127 4200)

//
// Define register set size.
//

#define CH_DMA_REGISTER_SIZE            (0x210)

//
// Define maximums.
//

#define CH_DMA_MAX_REQUEST_LINES        (32)
#define CH_DMA_MAX_CHANNELS             (32)

#define CONFIGURE_ADD_REQ_CONFIG        (0x80000000)

//
// Define register offsets.
//

#define CH_DMA_CONTROL                  (0x00)
#define CH_DMA_STATUS                   (0x04)
#define CH_DMA_INTERRUPT_MASK           (0x08)

#define CH_DMA_CHAN_CONTROL             (0x00)
#define CH_DMA_CHAN_MEM_PTR             (0x04)
#define CH_DMA_CHAN_DEV_PTR             (0x08)
#define CH_DMA_CHAN_STATUS              (0x1c)

//
// Define device-side bus widths.
//

#define CH_DMA_WIDTH_8_BIT              (0)
#define CH_DMA_WIDTH_16_BIT             (1)
#define CH_DMA_WIDTH_32_BIT             (2)

//
// Define transfer burst sizes.
//

#define CH_DMA_BURST_1_WORD             (0)
#define CH_DMA_BURST_2_WORD             (1)
#define CH_DMA_BURST_4_WORD             (2)
#define CH_DMA_BURST_8_WORD             (3)

//
// Define transfer directions.
//

#define CH_DMA_DEVICE_WRITE             (0)
#define CH_DMA_DEVICE_READ              (1)

//
// ------------------------------------------------------ Data Type Definitions
//

#pragma pack(push, 1)

//
// Register Definitions.
//

typedef struct _CH_DMA_CONTROL_REGISTER {
    union {
        struct {
            ULONG ControllerEnable:1;
            ULONG Reserved:31;
        };
        ULONG AsUlong;
    };
} CH_DMA_CONTROL_REGISTER, *PCH_DMA_CONTROL_REGISTER;

typedef struct _CH_DMA_CHAN_CONTROL_REGISTER {
    union {
        struct {
            ULONG ChannelEnable:1;
            ULONG InterruptEnable:1;
            ULONG DeviceWidth:2;
            ULONG BurstSize:2;
            ULONG FlowControl:1;
            ULONG Loop:1;
            ULONG RequestLine:5;
            ULONG ReadFromDevice:1;
            ULONG Reserved:2;
            ULONG Length:16;
        };
        ULONG AsUlong;
    };
} CH_DMA_CHAN_CONTROL_REGISTER, *PCH_DMA_CHAN_CONTROL_REGISTER;

typedef struct _CH_DMA_CHAN_STATUS_REGISTER {
    union {
        struct {
            ULONG Busy:1;
            ULONG Reserved:15;
            ULONG BytesTransferred:16;
        };
        ULONG AsUlong;
    };
} CH_DMA_CHAN_STATUS_REGISTER, *PCH_DMA_CHAN_STATUS_REGISTER;

//
// CSRT resource descriptor types for chDMA.
//

typedef struct _CH_DMA_ADD_REQ_LINE_CONFIG {
    ULONG RequestLine;
    CH_DMA_CHAN_CONTROL_REGISTER Ctrl;
} CH_DMA_ADD_REQ_LINE_CONFIG, *PCH_DMA_ADD_REQ_LINE_CONFIG;

typedef struct _RD_DMA_CONTROLLER {
    CSRT_RESOURCE_DESCRIPTOR_HEADER Header;
    ULONGLONG BasePhysicalAddress;
    ULONG ChannelCount;
    ULONG MinimumRequestLine;
    ULONG MaximumRequestLine;
    ULONG InterruptGsi;
    BOOLEAN CacheCoherent;
    ULONG ReqLineConfigCount;
    CH_DMA_ADD_REQ_LINE_CONFIG ReqLineConfigs[ANYSIZE_ARRAY];
} RD_DMA_CONTROLLER, *PRD_DMA_CONTROLLER;

//
// Resource description matching chDMA controller described in chdmaReadme.txt:
//
// CH_DMA_CHAN_CONTROL_REGISTER UartControl;
// UartControl.AsUlong = 0;
// UartControl.DeviceWidth = CH_DMA_WIDTH_8_BIT;
// UartControl.BurstSize = CH_DMA_BURST_1_WORD;
// UartControl.FlowControl = 1;
//
// CH_DMA_ADD_REQ_LINE_CONFIG UartConfig = {
//      0x15,
//      UartControl
// };
//
// RD_DMA_CONTROLLER DmaDesc = {
//      Header,
//      0x70001000,
//      32,
//      0,
//      31,
//      0x27,
//      FALSE,
//      1,
//      { UartConfig }
// };
//

typedef struct _RD_DMA_CHANNEL {
    CSRT_RESOURCE_DESCRIPTOR_HEADER Header;
    ULONG ChannelNumber;
} RD_DMA_CHANNEL, *PRD_DMA_CHANNEL;

//
// Resource description matching chDMA channel described in chdmaReadme.txt:
//
// RD_DMA_CHANNEL DmaDesc = {
//      Header,
//      0
// };
//

#pragma pack(pop)

//
// Extension Request line to configuration mapping.
//

typedef struct _CH_REQ_LINE_CONFIG {
    BOOLEAN Valid;
    CH_DMA_CHAN_CONTROL_REGISTER Ctrl;
} CH_REQ_LINE_CONFIG, *PCH_REQ_LINE_CONFIG;

typedef struct _CH_REQ_LINE_CONFIG_TABLE {
    CH_REQ_LINE_CONFIG Entries[CH_DMA_MAX_REQUEST_LINES];
} CH_DMA_REQ_LINE_CONFIG_TABLE, *PCH_DMA_REQ_LINE_CONFIG_TABLE;

//
// Define extension internal data for the controller and channel.
//

typedef struct _CH_DMA_CHANNEL {
    BOOLEAN Active;
    BOOLEAN AutoInit;
} CH_DMA_CHANNEL, *PCH_DMA_CHANNEL;

typedef struct _CH_DMA_CONTROLLER {

    //
    // Virtual and physical base addresses of the controller.
    //

    PULONG ControllerBaseVa;
    PHYSICAL_ADDRESS ControllerBasePa;

    ULONG ChannelCount;
    ULONG MinimumRequestLine;
    ULONG MaximumRequestLine;

    //
    // Individual channel extension status.
    //

    CH_DMA_CHANNEL Channels[CH_DMA_MAX_CHANNELS];

    //
    // Request line to configuration mapping.
    //

    CH_DMA_REQ_LINE_CONFIG_TABLE ReqConfig;
} CH_DMA_CONTROLLER, *PCH_DMA_CONTROLLER;

//
// ----------------------------------------------------------------- Prototypes
//

VOID
ChInitializeController (
    __in PVOID ControllerContext
    );

BOOLEAN
ChValidateRequestLineBinding (
    __in PVOID ControllerContext,
    __in PDMA_REQUEST_LINE_BINDING_DESCRIPTION BindingDescription
    );

ULONG
ChQueryMaxFragments (
    __in PVOID ControllerContext,
    __in ULONG ChannelNumber,
    __in ULONG MaxFragmentsRequested
    );

VOID
ChProgramChannel (
    __in PVOID ControllerContext,
    __in ULONG ChannelNumber,
    __in ULONG RequestLine,
    __in PDMA_SCATTER_GATHER_LIST MemoryAddresses,
    __in PHYSICAL_ADDRESS DeviceAddress,
    __in BOOLEAN WriteToDevice,
    __in BOOLEAN LoopTransfer
    );

BOOLEAN
ChCancelTransfer (
    __in PVOID ControllerContext,
    __in ULONG ChannelNumber
    );

NTSTATUS
ChConfigureChannel (
    __in PVOID ControllerContext,
    __in ULONG ChannelNumber,
    __in ULONG FunctionNumber,
    __in PVOID Context
    );

VOID
ChFlushChannel (
    __in PVOID ControllerContext,
    __in ULONG ChannelNumber
    );

_Success_(return != FALSE)
BOOLEAN
ChHandleInterrupt (
    __in PVOID ControllerContext,
    __out PULONG ChannelNumber,
    __out PDMA_INTERRUPT_TYPE InterruptType
    );

ULONG
ChReadDmaCounter (
    __in PVOID ControllerContext,
    __in ULONG ChannelNumber
    );

//
// -------------------------------------------------------------------- Globals
//

DMA_FUNCTION_TABLE ChFunctionTable =
{
    ChInitializeController,
    ChValidateRequestLineBinding,
    ChQueryMaxFragments,
    ChProgramChannel,
    ChConfigureChannel,
    ChFlushChannel,
    ChHandleInterrupt,
    ChReadDmaCounter,
    NULL, /* ReportCommonBuffer */
    ChCancelTransfer
};

//
// ------------------------------------------------------------------- Routines
//

VOID
WriteRegister (
    __in PCH_DMA_CONTROLLER Controller,
    __in ULONG RegisterOffset,
    __in ULONG Value
    )

{

    PULONG Address;

    Address = Controller->ControllerBaseVa;

    //
    // Add register offset.  Register offset is in bytes.  Pointer addition is
    // in ULONGs.
    //

    Address += (RegisterOffset / 4);

    WRITE_REGISTER_ULONG(Address, Value);
}

ULONG
ReadRegister (
    __in PCH_DMA_CONTROLLER Controller,
    __in ULONG RegisterOffset
    )

{

    PULONG Address;

    Address = Controller->ControllerBaseVa;

    //
    // Add register offset.  Register offset is in bytes.  Pointer addition is
    // in ULONGs.
    //

    Address += (RegisterOffset / 4);

    return READ_REGISTER_ULONG(Address);
}

VOID
WriteChannelRegister (
    __in PCH_DMA_CONTROLLER Controller,
    __in ULONG ChannelNumber,
    __in ULONG RegisterOffset,
    __in ULONG Value
    )

{
    PULONG Address;

    Address = Controller->ControllerBaseVa;

    //
    // Add channel base offset.
    // Register offset is in bytes.  Pointer addition is in ULONGs.
    //

    Address += (0x10 / 4);

    //
    // Add channel number offset.
    //

    Address += ((0x10 * ChannelNumber) / 4);

    //
    // Add channel register offset.
    //

    Address += (RegisterOffset / 4);

    WRITE_REGISTER_ULONG(Address, Value);
}

ULONG
ReadChannelRegister (
    __in PCH_DMA_CONTROLLER Controller,
    __in ULONG ChannelNumber,
    __in ULONG RegisterOffset
    )

{

    PULONG Address;

    Address = Controller->ControllerBaseVa;

    //
    // Add channel base offset.
    // Register offset is in bytes.  Pointer addition is in ULONGs.
    //

    Address += (0x10 / 4);

    //
    // Add channel number offset.
    //

    Address += ((0x10 * ChannelNumber) / 4);

    //
    // Add channel register offset.
    //

    Address += (RegisterOffset / 4);

    return READ_REGISTER_ULONG(Address);
}

VOID
ChInitializeController (
    __in PVOID ControllerContext
    )

/*++

Routine Description:

    This routine provides an opportunity for DMA controllers to initialize.

Arguments:

    ControllerContext - Supplies a pointer to the controller's internel data.

Return Value:

    None.

--*/

{

    CH_DMA_CONTROL_REGISTER CtrlRegister;
    PCH_DMA_CONTROLLER Controller;
    ULONG Index;
    ULONG InterruptMask;

    Controller = (PCH_DMA_CONTROLLER)ControllerContext;

    //
    // Map the controller base iff this is the first call to init (and it is
    // therefore not already mapped.)
    //

    if (Controller->ControllerBaseVa == NULL) {
        Controller->ControllerBaseVa =
            (PULONG) HalMapIoSpace(Controller->ControllerBasePa,
                                   PAGE_SIZE,
                                   MmNonCached);

        NT_ASSERT(Controller->ControllerBaseVa != NULL);
    }

    if (Controller->ControllerBaseVa == NULL) {
        return;
    }

    //
    // Initialize each channel.
    //

    for (Index = 0; Index < Controller->ChannelCount; Index += 1) {
        WriteChannelRegister(Controller, Index, CH_DMA_CHAN_CONTROL, 0);
        Controller->Channels[Index].Active = FALSE;
        Controller->Channels[Index].AutoInit = FALSE;
    }

    //
    // Enable the DMA controller.
    //

    CtrlRegister.AsUlong = 0;
    CtrlRegister.ControllerEnable = 1;
    WriteRegister(Controller, CH_DMA_CONTROL, CtrlRegister.AsUlong);

    //
    // Enable all interrupts.
    //

    InterruptMask = (1UL << Controller->ChannelCount) - 1;
    WriteRegister(Controller, CH_DMA_INTERRUPT_MASK, InterruptMask);

    return;
}

BOOLEAN
ChValidateRequestLineBinding (
    __in PVOID ControllerContext,
    __in PDMA_REQUEST_LINE_BINDING_DESCRIPTION BindingDescription
    )

/*++

Routine Description:

    This routine queries a DMA controller extension to test the validity of a
    request line binding.

Arguments:

    ControllerContext - Supplies a pointer to the controller's internal data.

    DeviceDescription - Supplies a pointer to the request information.

Return Value:

    TRUE if the request line binding is valid and supported by the controller.

    FALSE if the binding is invalid.

Environment:

    PASSIVE_LEVEL.

--*/

{

    PCH_DMA_CONTROLLER Controller;

    Controller = (PCH_DMA_CONTROLLER)ControllerContext;
    if (BindingDescription->ChannelNumber > Controller->ChannelCount) {
        return FALSE;
    }

    if ((BindingDescription->RequestLine > Controller->MaximumRequestLine) ||
        (BindingDescription->RequestLine < Controller->MinimumRequestLine)) {

        return FALSE;
    }

    return TRUE;
}

ULONG
ChQueryMaxFragments (
    __in PVOID ControllerContext,
    __in ULONG ChannelNumber,
    __in ULONG MaxFragmentsRequested
    )

/*++

Routine Description:

    This routine queries the DMA extension to determine the number of
    scatter gather fragments that the next transfer can support.

Arguments:

    ControllerContext - Supplies a pointer to the controller's internal data.

    ChannelNumber - Supplies the number of the channel to program.

    MaxFragmentsRequested - Supplies a hint to the maximum fragments useful to
        this transfer.

Return Value:

    Number of fragments the next transfer on this channel can support.

--*/

{

    UNREFERENCED_PARAMETER(ControllerContext);
    UNREFERENCED_PARAMETER(ChannelNumber);
    UNREFERENCED_PARAMETER(MaxFragmentsRequested);

    return 1;
}

VOID
ChProgramChannel (
    __in PVOID ControllerContext,
    __in ULONG ChannelNumber,
    __in ULONG RequestLine,
    __in PDMA_SCATTER_GATHER_LIST MemoryAddresses,
    __in PHYSICAL_ADDRESS DeviceAddress,
    __in BOOLEAN WriteToDevice,
    __in BOOLEAN LoopTransfer
    )

/*++

Routine Description:

    This routine programs a DMA controller channel for a specific transfer.

Arguments:

    ControllerContext - Supplies a pointer to the controller's internal data.

    ChannelNumber - Supplies the number of the channel to program.

    RequestLine - Supplies the request line number to program.  This request
        line number is system-unique (as provided to the HAL during
        registration) and must be translated by the extension.

    MemoryAddress - Supplies the address to be programmed into the memory
        side of the channel configuration.

    DeviceAddress - Supplies the address to be programmed into the device
        side of the channel configuration.

    WriteToDevice - Supplies the direction of the transfer.

    LoopTransfer - Supplies whether AutoInitialize has been set in the
        adapter making this request.

Return Value:

    None.

--*/

{

    CH_DMA_CHAN_CONTROL_REGISTER Ctrl;
    PCH_DMA_CONTROLLER Controller;
    ULONG DevPtr;
    ULONG MemPtr;

    Controller = (PCH_DMA_CONTROLLER)ControllerContext;
    Controller->Channels[ChannelNumber].Active = TRUE;

    //
    // If this request line exists in the request line config table, use
    // those values.  Otherwise, use the reset values.
    //

    Ctrl.AsUlong = 0;
    if (Controller->ReqConfig.Entries[RequestLine].Valid != 0) {
        Ctrl.AsUlong = Controller->ReqConfig.Entries[RequestLine].Ctrl.AsUlong;
    }

    Ctrl.ChannelEnable = 1;
    Ctrl.InterruptEnable = 1;
    Ctrl.Loop = (LoopTransfer == FALSE) ? 0 : 1;
    Ctrl.ReadFromDevice = (WriteToDevice == FALSE) ? 1 : 0;

    //
    // Request lines numbers reported by BIOS may be offset to make them
    // globally unique.  Request lines on the controller are based at 0.
    //

    Ctrl.RequestLine = RequestLine - Controller->MinimumRequestLine;
    Ctrl.Length = MemoryAddresses->Elements[0].Length;

    DevPtr = DeviceAddress.LowPart;
    MemPtr = MemoryAddresses->Elements[0].Address.LowPart;

    WriteChannelRegister(Controller,
                         ChannelNumber,
                         CH_DMA_CHAN_MEM_PTR,
                         MemPtr);

    WriteChannelRegister(Controller,
                         ChannelNumber,
                         CH_DMA_CHAN_DEV_PTR,
                         DevPtr);

    //
    // Channel control is written last as it will enable the channel.
    //

    WriteChannelRegister(Controller,
                         ChannelNumber,
                         CH_DMA_CHAN_CONTROL,
                         Ctrl.AsUlong);
}

BOOLEAN
ChCancelTransfer (
    __in PVOID ControllerContext,
    __in ULONG ChannelNumber
    )

/*++

Routine Description:

    This routine must disable the selected channel.  The channel must not be
    capable of interrupting for this transfer after being cleared in this way.

Arguments:

    ControllerContext - Supplies a pointer to the controller's internal data.

    ChannelNumber - Supplies the channel number.

Return Value:

    FALSE if the channel is already idle or if the channel is already asserting
    an interrupt.  TRUE is the channel is active and no interrupt is asserted.

--*/

{

    PCH_DMA_CONTROLLER Controller;
    ULONG StatusRegister;

    Controller = (PCH_DMA_CONTROLLER)ControllerContext;

    //
    // If the channel is not active (because the interrupt already fired or
    // because the channel was never programmed, return immediately.
    //

    if (Controller->Channels[ChannelNumber].Active == FALSE) {
        return FALSE;
    }

    //
    // Disable the channel.
    //

    WriteChannelRegister(Controller, ChannelNumber, CH_DMA_CHAN_CONTROL, 0);

    //
    // If an interrupt is already pending on the channel, do nothing.  The
    // normal interrupt path will complete it.  If an interrupt is not pending
    // then this was successfully cancelled.  In that case, mark the channel
    // inactive and return TRUE.
    //

    StatusRegister = ReadRegister(Controller, CH_DMA_STATUS);
    if ((StatusRegister & (1UL << ChannelNumber)) != 0) {
        return FALSE;

    } else {
        Controller->Channels[ChannelNumber].Active = FALSE;
        return TRUE;
    }
}

NTSTATUS
AddReqLineConfig (
    __in PCH_DMA_CONTROLLER Controller,
    __in PCH_DMA_ADD_REQ_LINE_CONFIG Config
    )

/*++

Routine Description:

    This routine updates the request line configuration table in the
    extension's internal data for this controller.

Arguments:

    Controller - Supplies a pointer to the internal data for the controller.

    Config - Supplies a pointer to the configuration to modify.

Return Value:

    STATUS_INVALID_PARAMETER if the request line is invalid.
    Else, STATUS_SUCCESS.

--*/

{

    ULONG RequestLine;

    RequestLine = Config->RequestLine;
    if ((RequestLine < Controller->MinimumRequestLine) ||
        (RequestLine > Controller->MaximumRequestLine)) {

        return STATUS_INVALID_PARAMETER;
    }

    Controller->ReqConfig.Entries[RequestLine].Ctrl.AsUlong =
        Config->Ctrl.AsUlong;

    return STATUS_SUCCESS;
}

NTSTATUS
ChConfigureChannel (
    __in PVOID ControllerContext,
    __in ULONG ChannelNumber,
    __in ULONG FunctionNumber,
    __in PVOID Context
    )

/*++

Routine Description:

    This routine configures the channel for a DMA extension specific operation.

Arguments:

    ControllerContext - Supplies a pointer to the controller's internal data.

    ChannelNumber - Supplies the channel to configure.

    FunctionNumber - Supplies the ID of the operation to perform.

    Context - Supplies parameters for this operation.

Return Value:

    NTSTATUS code.

--*/

{

    PCH_DMA_CONTROLLER Controller;
    NTSTATUS Status;

    UNREFERENCED_PARAMETER(ChannelNumber);

    Controller = (PCH_DMA_CONTROLLER)ControllerContext;
    switch (FunctionNumber) {
    case CONFIGURE_ADD_REQ_CONFIG:
        Status = AddReqLineConfig(Controller,
                                  (PCH_DMA_ADD_REQ_LINE_CONFIG)Context);

        break;
    default:
        Status = STATUS_NOT_IMPLEMENTED;
    }

    return Status;
}

VOID
ChFlushChannel (
    __in PVOID ControllerContext,
    __in ULONG ChannelNumber
    )

/*++

Routine Description:

    This routine flushes a previous transfer from a channel and returns the
    channel to a state ready for the next ProgramChannel call.

Arguments:

    ControllerContext - Supplies a pointer to the controller's internal data.

    ChannelNumber - Supplies the channel to flush.

Return Value:

    None.

--*/

{

    PCH_DMA_CONTROLLER Controller;

    Controller = (PCH_DMA_CONTROLLER)ControllerContext;
    WriteChannelRegister(Controller, ChannelNumber, CH_DMA_CHAN_CONTROL, 0);
}

_Success_(return != FALSE)
BOOLEAN
ChHandleInterrupt (
    __in PVOID ControllerContext,
    __out PULONG ChannelNumber,
    __out PDMA_INTERRUPT_TYPE InterruptType
    )

/*++

Routine Description:

    This routine probes a controller for interrupts, clears any interrupts
    found, fills in channel and interrupt type information.  This routine
    will be called repeatedly until FALSE is returned.

Arguments:

    ControllerContext - Supplies a pointer to the controller's internal data.

    ChannelNumber - Supplies a placeholder for the extension to fill in which
        channel is interrupting.

    InterruptType - Supplies a placeholder for the extension to fill in the
        interrupt type.

Return Value:

    TRUE if an interrupt was found on this controller.

    FALSE otherwise.

--*/

{

    PCH_DMA_CONTROLLER Controller;
    ULONG Index;
    ULONG StatusRegister;

    Controller = (PCH_DMA_CONTROLLER)ControllerContext;
    StatusRegister = ReadRegister(Controller, CH_DMA_STATUS);
    for (Index = 0; Index < Controller->ChannelCount; Index += 1) {
        if ((StatusRegister & (1UL << Index)) != 0) {
            *ChannelNumber = Index;
            *InterruptType = InterruptTypeCompletion;
            WriteRegister(Controller, CH_DMA_STATUS, (1UL << Index));

            //
            // If the channel is not marked as active, then this interrupt
            // is spurious.  Probably this is the result of a transfer being
            // cancelled.
            //

            if (Controller->Channels[Index].Active == FALSE) {
                continue;
            }

            //
            // If the channel is not in autoinitialize mode, is it inactive now.
            // If the channel is in autoinitialize, then it will remain active
            // until it receives a cancel.
            //

            if (Controller->Channels[Index].AutoInit == FALSE) {
                Controller->Channels[Index].Active = FALSE;
            }

            return TRUE;
        }
    }

    return FALSE;
}

ULONG ChReadDmaCounter (
    __in PVOID ControllerContext,
    __in ULONG ChannelNumber
    )

/*++

Routine Description:

    This routine determines how many bytes remain to be transferred on the
    given channel.  If the current transfer is set to loop, this routine
    will return the number of bytes remaining in the current iteration.

Arguments:

    ControllerContext - Supplies a pointer to the controller's internal data.

    ChannelNumber - Supplies the channel number.

Return Value:

    Returns the number of bytes remaining to be transferred on the given
    channel.

--*/

{

    PCH_DMA_CONTROLLER Controller;
    CH_DMA_CHAN_CONTROL_REGISTER Ctrl;
    CH_DMA_CHAN_STATUS_REGISTER StatusReg;

    Controller = (PCH_DMA_CONTROLLER)ControllerContext;
    Ctrl.AsUlong = ReadChannelRegister(Controller,
                                       ChannelNumber,
                                       CH_DMA_CHAN_CONTROL);

    StatusReg.AsUlong = ReadChannelRegister(Controller,
                                            ChannelNumber,
                                            CH_DMA_CHAN_STATUS);

    return (Ctrl.Length - StatusReg.BytesTransferred);
}

NTSTATUS
RegisterDmaController (
    __in ULONG Handle,
    __in PCSRT_RESOURCE_GROUP_HEADER ResourceGroup,
    __in PCSRT_RESOURCE_DESCRIPTOR_HEADER ResourceDescriptor,
    __out PULONG ControllerId
    )

/*++

Routine Description:

    This routine takes a DMA resource descriptor with subtype controller.
    The controller is then registered with the HAL.

Arguments:

    Handle - Supplies handle passed to extension.

    ResourceGroup - Supplies resource group containing this descriptor.

    ResourceDescrpitor - Supplies the resource descriptor.

    ControllerId - Supplies a placeholder for the controller ID returned from
        the HAL after registration.

Return Value:

    NTSTATUS Value.

--*/

{

    PCH_REQ_LINE_CONFIG ConfigEntry;
    PCH_REQ_LINE_CONFIG ConfigTableDst;
    PCH_DMA_ADD_REQ_LINE_CONFIG ConfigTableSrc;
    CH_DMA_CONTROLLER Controller;
    PRD_DMA_CONTROLLER DmaDesc;
    DMA_INITIALIZATION_BLOCK DmaInitBlock;
    ULONG Index;
    ULONG RequestLine;
    NTSTATUS Status;

    RtlZeroMemory(&Controller, sizeof(CH_DMA_CONTROLLER));

    DmaDesc = (PRD_DMA_CONTROLLER)ResourceDescriptor;
    Controller.ControllerBasePa.QuadPart = DmaDesc->BasePhysicalAddress;
    Controller.ChannelCount = DmaDesc->ChannelCount;
    Controller.MinimumRequestLine = DmaDesc->MinimumRequestLine;
    Controller.MaximumRequestLine = DmaDesc->MaximumRequestLine;

    //
    // Build request line configuration mapping table.
    //

    ConfigTableDst = &Controller.ReqConfig.Entries[0];
    ConfigTableSrc = &DmaDesc->ReqLineConfigs[0];

    //
    // Verify the supplied configuration description will fit within
    // the build maximum.
    //

    if (DmaDesc->ReqLineConfigCount >= CH_DMA_MAX_REQUEST_LINES) {
        Status = STATUS_INVALID_PARAMETER_1;
        goto Exit;
    }

    for (Index = 0; Index < DmaDesc->ReqLineConfigCount; Index += 1) {

        RequestLine = ConfigTableSrc[Index].RequestLine;

        //
        // Keep PreFast happy that we're assigning within memory bounds
        //

        if (RequestLine >= CH_DMA_MAX_REQUEST_LINES) {
            Status = STATUS_INVALID_PARAMETER_2;
            goto Exit;
        }

        ConfigEntry = &ConfigTableDst[RequestLine];
        ConfigEntry->Ctrl.AsUlong = ConfigTableSrc[Index].Ctrl.AsUlong;
        ConfigEntry->Valid = TRUE;
    }

    INITIALIZE_DMA_HEADER(&DmaInitBlock);
    DmaInitBlock.ChannelCount = Controller.ChannelCount;
    DmaInitBlock.MinimumTransferUnit = 1;
    DmaInitBlock.MinimumRequestLine = Controller.MinimumRequestLine;
    DmaInitBlock.MaximumRequestLine = Controller.MaximumRequestLine;
    DmaInitBlock.CacheCoherent = DmaDesc->CacheCoherent;
    DmaInitBlock.GeneratesInterrupt = TRUE;
    DmaInitBlock.InternalData = (PVOID)&Controller;
    DmaInitBlock.InternalDataSize = sizeof(CH_DMA_CONTROLLER);
    DmaInitBlock.DmaAddressWidth = 32;
    DmaInitBlock.Gsi = DmaDesc->InterruptGsi;
    DmaInitBlock.InterruptPolarity = InterruptActiveHigh;
    DmaInitBlock.InterruptMode = LevelSensitive;
    DmaInitBlock.Operations = &ChFunctionTable;

    //
    // Register physical address space with the HAL.
    //

    HalRegisterPermanentAddressUsage(Controller.ControllerBasePa,
                                     CH_DMA_REGISTER_SIZE);

    //
    // Register controller.
    //

    Status = RegisterResourceDescriptor(Handle,
                                        ResourceGroup,
                                        ResourceDescriptor,
                                        &DmaInitBlock);

    *ControllerId = DmaInitBlock.ControllerId;

Exit:
    return Status;

}

NTSTATUS
RegisterDmaChannel (
    __in ULONG Handle,
    __in PCSRT_RESOURCE_GROUP_HEADER ResourceGroup,
    __in PCSRT_RESOURCE_DESCRIPTOR_HEADER ResourceDescriptor,
    __in ULONG ControllerId
    )

/*++

Routine Description:

    This routine takes a DMA resource descriptor with subtype channel.
    The channel is then registered with the HAL.

Arguments:

    Handle - Supplies handle passed to extension.

    ResourceGroup - Supplies resource group containing this descriptor.

    ResourceDescrpitor - Supplies the resource descriptor.

    ControllerId - Supplies the controller ID this channel is registered with.

Return Value:

    NTSTATUS Value.

--*/

{

    DMA_CHANNEL_INITIALIZATION_BLOCK DmaChannelInitBlock;
    PRD_DMA_CHANNEL DmaDesc;

    NT_ASSERT(ControllerId != 0);

    DmaDesc = (PRD_DMA_CHANNEL)ResourceDescriptor;
    INITIALIZE_DMA_CHANNEL_HEADER(&DmaChannelInitBlock);
    DmaChannelInitBlock.ControllerId = ControllerId;
    DmaChannelInitBlock.GeneratesInterrupt = FALSE;
    DmaChannelInitBlock.ChannelNumber = DmaDesc->ChannelNumber;
    DmaChannelInitBlock.CommonBufferLength = 0;

    return RegisterResourceDescriptor(Handle,
                                      ResourceGroup,
                                      ResourceDescriptor,
                                      &DmaChannelInitBlock);
}

NTSTATUS
AddResourceGroup (
    __in ULONG Handle,
    __in PCSRT_RESOURCE_GROUP_HEADER ResourceGroup
    )

{

    ULONG ControllerId;
    PCSRT_RESOURCE_DESCRIPTOR_HEADER ResourceDescriptor;

    ResourceDescriptor = NULL;
    ControllerId = 0;
    for (;;) {
        ResourceDescriptor =
            GetNextResourceDescriptor(Handle,
                                      ResourceGroup,
                                      ResourceDescriptor,
                                      CSRT_RD_TYPE_DMA,
                                      CSRT_RD_SUBTYPE_ANY,
                                      CSRT_RD_UID_ANY);

        if (ResourceDescriptor == NULL) {
            break;
        }

        if (ResourceDescriptor->Subtype == CSRT_RD_SUBTYPE_DMA_CONTROLLER) {
            RegisterDmaController(Handle,
                                  ResourceGroup,
                                  ResourceDescriptor,
                                  &ControllerId);

        } else if (ResourceDescriptor->Subtype == CSRT_RD_SUBTYPE_DMA_CHANNEL) {
            RegisterDmaChannel(Handle,
                               ResourceGroup,
                               ResourceDescriptor,
                               ControllerId);
        } else {

            NT_ASSERT(FALSE);
        }
    }

    return STATUS_SUCCESS;
}
