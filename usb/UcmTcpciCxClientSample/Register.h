/*++

Module Name:

    register.h

Abstract:

    This file contains the definitions for TCPCI device register addresses as defined in the
    USB Type-C Port Controller Interface Specification, Revision 1.0.

Environment:

    Kernel mode

--*/

#pragma once

#define VENDOR_ID                       0x00
#define PRODUCT_ID                      0x02
#define DEVICE_ID                       0x04
#define USBTYPEC_REV                    0x06
#define USBPD_REV_VER                   0x08
#define PD_INTERFACE_REV                0x0A

// 0x0C - 0x0F are reserved.

#define ALERT                           0x10
#define ALERT_MASK                      0x12
#define POWER_STATUS_MASK               0x14
#define FAULT_STATUS_MASK               0x15

// 0x16 - 0x17 are reserved.

#define CONFIG_STANDARD_OUTPUT          0x18
#define TCPC_CONTROL                    0x19
#define ROLE_CONTROL                    0x1A
#define FAULT_CONTROL                   0x1B
#define POWER_CONTROL                   0x1C
#define CC_STATUS                       0x1D
#define POWER_STATUS                    0x1E
#define FAULT_STATUS                    0x1F

// 0x20 - 0x22 are reserved.

#define COMMAND                         0x23
#define DEVICE_CAPABILITIES_1           0x24
#define DEVICE_CAPABILITIES_2           0x26
#define STANDARD_INPUT_CAPABILITIES     0x28
#define STANDARD_OUTPUT_CAPABILITIES    0x29

// 0x2A - 0x2D are reserved.

#define MESSAGE_HEADER_INFO             0x2E
#define RECEIVE_DETECT                  0x2F

// Receive buffer. The driver will read the entire receive buffer at once.
// Since the receive buffer registers are consecutive, we need to only define the starting address.

#define RECEIVE_BUFFER                  0x30

#define TRANSMIT                        0x50

// Transmit buffer. The driver will write the entire transmit buffer at once.
// Since the transmit buffer registers are consecutive, we need to only define the starting address.

#define TRANSMIT_BUFFER                 0x51

#define VBUS_VOLTAGE                    0x70
#define VBUS_SINK_DISCONNECT_THRESHOLD  0x72
#define VBUS_STOP_DISCHARGE_THRESHOLD   0x74
#define VBUS_VOLTAGE_ALARM_HI_CFG       0x76
#define VBUS_VOLTAGE_ALARM_LO_CFG       0x78

// 0x7A - 0x7F are reserved.

// TODO: Define any vendor-defined bits here.
// The TCPCI spec allocates registers 0x80 - 0xFF for vendor defined bits.