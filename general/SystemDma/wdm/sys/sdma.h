/*++

Copyright (c) 2011  Microsoft Corporation

Module Name:

    SDMA.H

Abstract:


    Defines the IOCTL codes that will be used by this driver.  The IOCTL code
    contains a command identifier, plus other information about the device,
    the type of access with which the file must have been opened,
    and the type of buffering.

Environment:

    Kernel mode only.

--*/

//
// Device type           -- in the "User Defined" range."
//
#define SDMA_TYPE 40000
//
// The IOCTL function codes from 0x800 to 0xFFF are for customer use.
//

#define IOCTL_SDMA_WRITE \
    CTL_CODE( SDMA_TYPE, 0x902, METHOD_BUFFERED, FILE_ANY_ACCESS  )

#define DRIVER_FUNC_INSTALL     0x01
#define DRIVER_FUNC_REMOVE      0x02

#define DRIVER_NAME       "SDMA"

