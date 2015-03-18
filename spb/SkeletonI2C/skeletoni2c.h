/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name: 

    skeletoni2c.h

Abstract:

    This module contains the controller-specific type 
    definitions for the SPB controller driver hardware.

Environment:

    kernel-mode only

Revision History:

--*/

//
// Includes for hardware register definitions.
//

#ifndef _SKELETONI2C_H_
#define _SKELETONI2C_H_

#include "hw.h"

//
// Skeleton I2C controller registers.
//

typedef struct SKELETONI2C_REGISTERS
{
    // TODO: Update this register structure to match the
    //       register mapping of the controller hardware.

    __declspec(align(4)) HWREG<ULONG>  Reg0;
    __declspec(align(4)) HWREG<ULONG>  Reg1;
}
SKELETONI2C_REGISTERS, *PSKELETONI2C_REGISTERS;

// TODO: Update the following defines to match the bit
//       functionalities of each register.

//
// Reg0 register bits.
//

#define SI2C_REG_0_BITS_31_28               0xF0000000

//
// Reg1 register bits.
//

#define SI2C_REG_1_BITS_31_28               0xF0000000

// TODO: Define other controller-specific values.

#define SI2C_MAX_TRANSFER_LENGTH            0x00001000

// TODO: Remove these generic error defines in favor
//       of real register bit mappings defined above.

#define SI2C_STATUS_ADDRESS_NACK            0x00000000
#define SI2C_STATUS_DATA_NACK               0x00000000
#define SI2C_STATUS_GENERIC_ERROR           0x00000000


//
// Register evaluation functions.
//

FORCEINLINE
bool
TestAnyBits(
    _In_ ULONG V1,
    _In_ ULONG V2
    )
{
    return (V1 & V2) != 0;
}

FORCEINLINE
bool
TestAllBits(
    _In_ ULONG V1,
    _In_ ULONG V2
    )
{
    return ((V1 & V2) == V2);
}

#endif
