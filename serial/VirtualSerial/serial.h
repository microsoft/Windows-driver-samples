/*++

Module Name :

    serial.h

Abstract:

    Type definitions and data for the serial port driver

Author:


--*/

#pragma once

#define SYMBOLIC_NAME_LENGTH        32
#define SYMBOLIC_LINK_NAME_PREFIX   L"\\DosDevices\\Global\\"

//
// This defines the bit used to control whether the device is sending
// a break.  When this bit is set the device is sending a space (logic 0).
//
// Most protocols will assume that this is a hangup.
//
#define SERIAL_LCR_BREAK    0x40

//
// These defines are used to set the line control register.
//
#define SERIAL_5_DATA       ((UCHAR)0x00)
#define SERIAL_6_DATA       ((UCHAR)0x01)
#define SERIAL_7_DATA       ((UCHAR)0x02)
#define SERIAL_8_DATA       ((UCHAR)0x03)
#define SERIAL_DATA_MASK    ((UCHAR)0x03)

#define SERIAL_1_STOP       ((UCHAR)0x00)
#define SERIAL_1_5_STOP     ((UCHAR)0x04) // Only valid for 5 data bits
#define SERIAL_2_STOP       ((UCHAR)0x04) // Not valid for 5 data bits
#define SERIAL_STOP_MASK    ((UCHAR)0x04)

#define SERIAL_NONE_PARITY  ((UCHAR)0x00)
#define SERIAL_ODD_PARITY   ((UCHAR)0x08)
#define SERIAL_EVEN_PARITY  ((UCHAR)0x18)
#define SERIAL_MARK_PARITY  ((UCHAR)0x28)
#define SERIAL_SPACE_PARITY ((UCHAR)0x38)
#define SERIAL_PARITY_MASK  ((UCHAR)0x38)
