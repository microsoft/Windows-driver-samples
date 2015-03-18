/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    simgpio_i2c.h

Abstract:

    This sample implements a GPIO client driver for simulated GPIO (SimGpio)
    controller.

    Note: DIRQL in the comments below refers to device IRQL, which is any
        IRQL > DISPATCH_LEVEL (and less than some IRQL reserved for OS use).


Environment:

    Kernel mode

--*/

#pragma once

//
// -------------------------------------------------------------------- Defines
//

//
// Define total number of pins on the simulated GPIO controller.
//

#define SIM_GPIO_TOTAL_PINS (16)
#define SIM_GPIO_PINS_PER_BANK (8)
#define SIM_GPIO_TOTAL_BANKS (SIM_GPIO_TOTAL_PINS / SIM_GPIO_PINS_PER_BANK)

#define SIM_GPIO_REGISTER_ADDRESS_SIZE (sizeof(USHORT))

//
// Pool tag for SimGpio allocations.
//

#define SIM_GPIO_POOL_TAG 'GmiS'

//
// Macro for pointer arithmetic.
//

#define Add2Ptr(Ptr, Value) ((PVOID)((PUCHAR)(Ptr) + (Value)))

//
// ---------------------------------------------------------------------- Types
//

//
// Define the registers within the SimGPIO controller. There are 16 pins per
// controller. Note this is a logical device and thus may correspond to a
// physical bank or module if the GPIO controller in hardware has more than
// 16 pins. Below is the register set from a logical perspective.
//
// typedef struct _SIM_GPIO_REGISTERS {
//    UCHAR ModeRegister1;
//    UCHAR PolarityRegister1;
//    UCHAR EnableRegister1;
//    UCHAR StatusRegister1;
//    UCHAR DirectionRegister1;
//    UCHAR LevelRegister1;
//    UCHAR ModeRegister2;
//    UCHAR PolarityRegister2;
//    UCHAR EnableRegister2;
//    UCHAR StatusRegister2;
//    UCHAR DirectionRegister2;
//    UCHAR LevelRegister2;
// } SIM_GPIO_REGISTERS, *PSIM_GPIO_REGISTERS;
//
//

typedef enum _SIM_GPIO_REGISTER_ADDRESS {
    ModeRegister = 0x0,
    PolarityRegister,
    EnableRegister,
    StatusRegister,
    DirectionRegister,
    LevelRegister,
    MaximumSimGpioAddress
} SIM_GPIO_REGISTER_ADDRESS, *PSIM_GPIO_REGISTER_ADDRESS;

struct _SIM_GPIO_CONTEXT;

typedef struct _SIM_GPIO_BANK {
    USHORT AddressBase;
    struct _SIM_GPIO_CONTEXT *GpioContext;
    UCHAR SavedRegisterContext[MaximumSimGpioAddress];
} SIM_GPIO_BANK, *PSIM_GPIO_BANK;

//
// The SimGPIO client driver device extension.
//

struct _SIM_GPIO_CONTEXT {
    USHORT TotalBanks;
    USHORT TotalPins;
    WDFDEVICE Device;
    WDFIOTARGET SpbIoTarget;
    LARGE_INTEGER SpbConnectionId;
    WDFREQUEST SpbRequest;
    // PSIM_GPIO_REGISTERS ControllerBase;
    // ULONG Length;
    SIM_GPIO_BANK Banks[SIM_GPIO_TOTAL_BANKS];
};

typedef struct _SIM_GPIO_CONTEXT SIM_GPIO_CONTEXT;
typedef SIM_GPIO_CONTEXT *PSIM_GPIO_CONTEXT;

//
// Request context
//
typedef struct _SIM_GPIO_REQUEST_CONTEXT {
    WDFDEVICE Device;

    //
    // Whether the request is a sequence or not.
    //

    BOOLEAN SequenceRequest;
} SIM_GPIO_REQUEST_CONTEXT, *PSIM_GPIO_REQUEST_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(SIM_GPIO_REQUEST_CONTEXT, GetRequestContext);

//
// ----------------------------------------------------------------- Prototypes
//

VOID
SimGpioDestroySpbConnection (
    _In_ PSIM_GPIO_CONTEXT SimGpioContext
    );

NTSTATUS
SimGpioSpbReadByte (
    _In_ PSIM_GPIO_BANK GpioBank,
    _In_ SIM_GPIO_REGISTER_ADDRESS RegisterAddress,
    _Out_writes_(sizeof(UCHAR)) PUCHAR Data
    );

NTSTATUS
SimGpioSetupSpbConnection (
    _In_ PSIM_GPIO_CONTEXT GpioContext
    );

NTSTATUS
SimGpioSpbWriteByte (
    _In_ PSIM_GPIO_BANK GpioBank,
    _In_ SIM_GPIO_REGISTER_ADDRESS RegisterAddress,
    _In_ UCHAR Data
    );

NTSTATUS
SimGpioRestoreBankHardwareContext (
    _In_ PVOID Context,
    _In_ BANK_ID BankId
    );

NTSTATUS
SimGpioSaveBankHardwareContext (
    _In_ PVOID Context,
    _In_ BANK_ID BankId
    );


