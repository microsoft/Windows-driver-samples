/*++

Copyright (c) Microsoft Corporation All Rights Reserved

Module Name:

   io.h

Abstract:

    Common header definitions and structs for read and write (IO) operation

Author:

Environment:

   Kernel mode only


Revision History:

--*/

#ifndef __IO_H__
#define __IO_H__

//
// 255 bytes of data + 3 bytes for HCI cmd hdr (2-byte opcode + 1-byte Parameter).
//
#define MIN_HCI_CMD_SIZE          (3)  
#define MAX_HCI_CMD_SIZE        (258)   

//
// 255 bytes of data + 2 byte hdr (1-byte event code + 1-byte parameter).
//
#define MIN_HCI_EVENT_SIZE        (2) 
#define HCI_EVENT_HEADER_SIZE     (2) 
#define MAX_HCI_EVENT_SIZE      (257)   

//
// Can be variable but usually 1021-byte (largest 3-DH5 ACL packet size) 
//
#define HCI_ACL_HEADER_SIZE         (4)
#define HCI_MAX_ACL_PAYLOAD_SIZE    (1021)
#define MIN_HCI_ACLDATA_SIZE         HCI_ACL_HEADER_SIZE   
#define MAX_HCI_ACLDATA_SIZE        (HCI_ACL_HEADER_SIZE + HCI_MAX_ACL_PAYLOAD_SIZE)

#define INITIAL_H4_READ_SIZE        (1+HCI_EVENT_HEADER_SIZE)
#define MAX_H4_HCI_PACKET_SIZE      (1+HCI_ACL_HEADER_SIZE + HCI_MAX_ACL_PAYLOAD_SIZE)  // include packet type

#define BUFFER_AND_SIZE_ADJUSTED(Buffer, Size, SegmentCount, Increment) {Buffer += Increment; Size -= Increment; SegmentCount += Increment;}

#define POOLTAG_BTHSERIALHCIBUSSAMPLE 'htbw'

#include <PSHPACK1.H>

//
// Standard HCI packet structs for Command, Event and ACL Data
//
typedef struct _HCI_COMMAND_PACKET {
    UINT16  Opcode;
    UCHAR   ParamsCount;            // 0..255
    UCHAR   Params[1];
} HCI_COMMAND_PACKET, *PHCI_COMMAND_PACKET;
#define HCI_COMMAND_HEADER_LEN  FIELD_OFFSET(HCI_COMMAND_PACKET, Params)

typedef struct _HCI_EVENT_PACKET {
    UCHAR   EventCode;
    UCHAR   ParamsCount;            // 0..255
    UCHAR   Params[1];    
} HCI_EVENT_PACKET, *PHCI_EVENT_PACKET;
#define HCI_EVENT_HEADER_LEN  FIELD_OFFSET(HCI_EVENT_PACKET, Params)

typedef struct _HCI_ACLDATA_PACKET {   
    UINT16    ConnectionHandle : 12;
    UINT16    PBFlag : 2;
    UINT16    BCFlag : 2;
    UINT16    DataLength;           // 0..65535
    UCHAR     Data[1];    
} HCI_ACLDATA_PACKET, *PHCI_ACLDATA_PACKET;
#define HCI_ACLDATA_HEADER_LEN  FIELD_OFFSET(HCI_ACLDATA_PACKET, Data)

//
// UART packet that has a leading packet type over standard HCI packet
//

typedef struct _H4_PACKET {
    UCHAR  Type;
    union {        
        HCI_COMMAND_PACKET Command;
        HCI_EVENT_PACKET   Event;
        HCI_ACLDATA_PACKET AclData;
        UCHAR Raw[MAX_HCI_ACLDATA_SIZE];
    } Packet;
} H4_PACKET, *PH4_PACKET;

typedef struct _UART_COMMAND_PACKET {
    UCHAR   Type;
    HCI_COMMAND_PACKET Packet;
} UART_COMMAND_PACKET, *PUART_COMMAND_PACKET;

typedef struct _UART_EVENT_PACKET {
    UCHAR   Type;    
    HCI_EVENT_PACKET Packet;      
} UART_EVENT_PACKET, *PUART_EVENT_PACKET;

typedef struct _UART_ACLDATA_PACKET {   
    UCHAR   Type;
    HCI_ACLDATA_PACKET Packet;     
} UART_ACLDATA_PACKET, *PUART_ACLDATA_PACKET;

#include <POPPACK.H>


typedef struct _FDO_EXTENSION *PFDO_EXTENSION;


#define REQUEST_PATH_NONE           0x00000000
#define REQUEST_PATH_CANCELLATION   0x00000001
#define REQUEST_PATH_COMPLETION     0x00000002


//
// Context used for data transfer to device (write)
//
typedef struct _UART_WRITE_CONTEXT {

    //
    // Back pointer to the FDO's extension
    //
    PFDO_EXTENSION   FdoExtension;

    //
    // Request from BthPort upper driver
    //
    WDFREQUEST  RequestFromBthport;    

    //
    // Flag(Bit) to determine ownership for completing RequestFromBthport
    //
    LONG    RequestCompletePath;
    
    //
    // Request to perform this transfer to UART device
    //
    WDFREQUEST  RequestToUART;

    //
    // Memory object for data
    //
    WDFMEMORY   Memory;

    //
    // The caller's transfer context.  
    //
    PBTHX_HCI_READ_WRITE_CONTEXT HCIContext;

    //
    // Pointer to the data buffer from client's incoming data; not a copy.
    //
    PVOID HCIPacket; 

    //
    // Packet length, including packet type.     
    //
    ULONG HCIPacketLen;

} UART_WRITE_CONTEXT, *PUART_WRITE_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(UART_WRITE_CONTEXT, GetWriteRequestContext)


//
// State machine used for reading incoming data streaming to form HCI event or data packet
//
typedef enum {
    GET_PKT_TYPE    = 1,   // For UART/H4, UCHAR of packet type (Event or Data)
    GET_PKT_HEADER  = 2,   // Get length to determine remaining payload
    GET_PKT_PAYLOAD = 3    // Data payload  
} UART_READ_STATE;


//
// A list to store prefetched (read) HCI packets utill they are retrieved.
//
typedef struct _HCI_PACKET_ENTRY {
    LIST_ENTRY  DataEntry;
    ULONG       PacketLen;
    _Field_size_bytes_(PacketLen) UCHAR Packet[1];
} HCI_PACKET_ENTRY, *PHCI_PACKET_ENTRY;

//
// Use to track request completion path
//
typedef enum _READ_REQUEST_STATE {   
    REQUEST_SENT     = 1,   // Request is being sent
    REQUEST_PENDING  = 2,   // Request is pending first - asynchronous completion
    REQUEST_COMPLETE = 3    // Request has completedly first - synchronous completion
} READ_REQUEST_STATE;


//
// Context used for reading UART operation to form HCI data or event packet
//
typedef struct _UART_READ_CONTEXT {

    //
    // Status of this request
    //
    NTSTATUS Status;
    
    //
    // Back pointer to the device extension
    //
    PFDO_EXTENSION FdoExtension;

    //
    // State machine for the read Request 
    //
    READ_REQUEST_STATE  RequestState;

    //
    // State machine of repeat read (read pump) to complete an HCI packet
    //
    UART_READ_STATE  ReadSegmentState;

    //
    // Bytes read for each Segment (Type, Header, and Paylaod) of a partial H4 packet below
    //
    ULONG BytesReadNextSegment;

    //
    // Bytes to read in order to have a full packet (only meaningful in GET_PKT_PAYLOAD state.
    //
    ULONG BytesToRead4FullPacket;

    //
    // A union of H4 packet
    //
    H4_PACKET H4Packet;     
 
} UART_READ_CONTEXT, *PUART_READ_CONTEXT;

#define MAX_HARDWARE_ERROR_COUNT 0  // Do not allow any error this time


// Timeout value for synchronous read and write requests
#define MAX_WRITE_TIMEOUT_IN_SEC  1  // unit = second
#define MAX_READ_TIMEOUT_IN_SEC   1


#endif

