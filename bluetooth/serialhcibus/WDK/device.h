/*++

Copyright (c) Microsoft Corporation All Rights Reserved

Module Name:

   device.h

Abstract:

    Header definitions and structs that are device specific 

Author:

Environment:

   Kernel mode only


Revision History:

--*/

#ifndef __DEVICE_H__
#define __DEVICE_H__

#pragma warning(disable:4214) // bit field types other than int


#define BT_PDO_HARDWARE_IDS L"SerialBusWdk\\UART_H4"
#define BT_PDO_COMPATIBLE_IDS L"MS_BTHX_BTHMINI"
#define BT_PDO_DEVICE_LOCATION L"Serial HCI Bus - Bluetooth Function"


//
// 255 bytes of data + 3 bytes for HCI cmd hdr (2-byte opcode + 1-byte Parameter).
//
#define MIN_HCI_CMD_SIZE          (3)  
#define MAX_HCI_CMD_SIZE        (258)   

//
// 255 bytes of data + 2 byte hdr (1-byte event code + 1-byte parameter).
//
#define MIN_HCI_EVENT_SIZE        (2)  
#define MAX_HCI_EVENT_SIZE      (257)   

//
// Can be variable but usually 1021-byte (largest 3-DH5 ACL packet size) 
//
#define HCI_ACL_HEADER_SIZE         (4)
#define HCI_MAX_ACL_PAYLOAD_SIZE    (1021)
#define MIN_HCI_ACLDATA_SIZE         HCI_ACL_HEADER_SIZE   
#define MAX_HCI_ACLDATA_SIZE        (HCI_ACL_HEADER_SIZE + HCI_MAX_ACL_PAYLOAD_SIZE)

#endif

