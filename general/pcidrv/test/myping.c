/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    MYPING.C

Abstract:



Environment:

    usermode console application

--*/
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include "testapp.h"
#include "nuiouser.h"
#include <intsafe.h>

#define MAX_HEADER_SIZE (sizeof(ETH_HEADER) + sizeof(IpHeader) + sizeof(IcmpHeader))
#define MAX_ECHO_PAY_LOAD (ETH_MAX_PACKET_SIZE - MAX_HEADER_SIZE)


#define ETH_HEADER_SIZE             14
#define ETH_MAX_DATA_SIZE           1500
#define ETH_MAX_PACKET_SIZE         ETH_HEADER_SIZE + ETH_MAX_DATA_SIZE
#define ETH_MIN_PACKET_SIZE         60

#define ARP_ETYPE_ARP   0x806
#define IP_PROT_TYPE   0x800
#define ARP_REQUEST     1
#define ARP_RESPONSE    2
#define ARP_HW_ENET     1

#define PROTOCOL_ICMP   1

// ICMP types and codes
#define ICMPV4_ECHO_REQUEST_TYPE   8
#define ICMPV4_ECHO_REQUEST_CODE   0
#define ICMPV4_ECHO_REPLY_TYPE     0
#define ICMPV4_ECHO_REPLY_CODE     0
#define ICMPV4_MINIMUM_HEADER      8

#define DEFAULT_DATA_SIZE      32       // default data size

#define DEFAULT_SEND_COUNT     32767   // number of ICMP requests to send

#define DEFAULT_RECV_TIMEOUT   6000     // six second

#define DEFAULT_TTL            128

#define IP_ADDR_LEN             4

#include <pshpack1.h>

// ========================================================================
// The IP header
//

typedef struct iphdr {
    unsigned char  verlen;
    unsigned char  tos;            // Type of service
    unsigned short total_len;      // total length of the packet
    unsigned short ident;          // unique identifier
    unsigned short frag_and_flags; // flags
    unsigned char  ttl;            // time to live
    unsigned char  proto;          // protocol (TCP, UDP etc)
    unsigned char  checksumHigh;
    unsigned char  checksumLow;
    unsigned int sourceIP;         // source      ip address
    unsigned int destIP;           // destination ip address

} IpHeader;

// ========================================================================
// ICMP header
//

typedef struct _ihdr {
    BYTE   i_type;
    BYTE   i_code;    /* type sub code */
    USHORT i_cksum;
    USHORT i_id;
    USHORT i_seq;
    ULONG  timestamp; /* non standard, reserve space for time */
} IcmpHeader;

typedef struct _ETH_HEADER
{
    UCHAR       DstAddr[MAC_ADDR_LEN];
    UCHAR       SrcAddr[MAC_ADDR_LEN];
    USHORT      EthType;
} ETH_HEADER, *PETH_HEADER;

// Structure of an ARP header.
typedef struct _ARP_BODY {
    USHORT      hw;                      // Hardware address space. = 00 01
    USHORT      pro;                     // Protocol address space. = 08 00
    UCHAR       hlen;                    // Hardware address length. = 06
    UCHAR       plen;                    // Protocol address length.  = 04
    USHORT      opcode;                  // Opcode.
    UCHAR       SenderHwAddr[MAC_ADDR_LEN]; // Source HW address.
    UINT        SenderIpAddr;                  // Source protocol address.
    UCHAR       DestHwAddr[MAC_ADDR_LEN]; // Destination HW address.
    UINT        DestIPAddr;                  // Destination protocol address.
} _ARP_BODY, *PARP_BODY;

#include <poppack.h>

//
// For Read operation.
//
typedef struct _RCB {
    OVERLAPPED      Overlapped;
    char            Buffer[ETH_MAX_PACKET_SIZE];
    PDEVICE_INFO    DeviceInfo;
}RCB, *PRCB;

//
// For Write operation.
//
typedef struct _TCB {
    OVERLAPPED      Overlapped;
    char            *Buffer; // packet length is user specified.
    ULONG           BufferLength;
    PDEVICE_INFO    DeviceInfo;

}TCB, *PTCB;

unsigned short PacketId;

VOID
PostNextRead(
    RCB *pRCB
    );


//
// Function: SetIcmpSequence
//
// Description:
//    This routine sets the sequence number of the ICMP request packet.
//
VOID
SetIcmpSequence(
    _At_((IcmpHeader*)buf, _Out_writes_bytes_all_(bufSize)) char *buf,
    _In_ _In_range_(==, sizeof(IcmpHeader)) ULONG bufSize
    )
{
    ULONG    sequence=0;
    IcmpHeader    *icmpv4=NULL;

    #pragma prefast(suppress:__WARNING_USE_OTHER_FUNCTION, "The recommended function GeTickCount64 is available only on Windows Vista/Server 2008 and above")
    sequence = GetTickCount();

    icmpv4 = (IcmpHeader *)buf;

    icmpv4->i_seq = (USHORT)sequence;
}

//
// Using _Inexpressible_ to suppress prefast warning 2014 : "Potential overflow
// using expression at icmp_hdr->i_code"
//
VOID
InitIcmpHeader(
    _Out_writes_bytes_(_Inexpressible_(bufSize + datasize)) char *buf,
    ULONG bufSize,
    int datasize)
{
    IcmpHeader   *icmp_hdr=NULL;
    char       *datapart=NULL;

    icmp_hdr = (IcmpHeader *)buf;
    icmp_hdr->i_type     = ICMPV4_ECHO_REQUEST_TYPE;        // request an ICMP echo
    icmp_hdr->i_code     = ICMPV4_ECHO_REQUEST_CODE;
    icmp_hdr->i_id       = htons((USHORT)PacketId++);
    icmp_hdr->i_cksum = 0;
    icmp_hdr->i_seq = 0;
    #pragma prefast(suppress:__WARNING_USE_OTHER_FUNCTION, "The recommended function GeTickCount64 is available only on Windows Vista/Server 2008 and above")
    icmp_hdr->timestamp= htonl(GetTickCount());

    datapart = buf + sizeof(IcmpHeader);
    //
    // Place some junk in the buffer.
    //
    memset(datapart, 'X', datasize);
}

VOID
ComputeIPChecksum (
    IpHeader    *pIPHeader
    )
//////////////////////////////////////////////////////////////////////////////
{
    ULONG           Checksum;
    PUCHAR          NextChar;

    pIPHeader->checksumHigh = pIPHeader->checksumLow = 0;
    Checksum = 0;
    for ( NextChar = (PUCHAR) pIPHeader
        ; (NextChar - (PUCHAR) pIPHeader) <= (sizeof(IpHeader) - 2)
        ; NextChar += 2)
    {
        Checksum += ((ULONG) (NextChar[0]) << 8) + (ULONG) (NextChar[1]);
    }

    Checksum = (Checksum >> 16) + (Checksum & 0xffff);
    Checksum += (Checksum >> 16);
    Checksum = ~Checksum;

    pIPHeader->checksumHigh = (UCHAR) ((Checksum >> 8) & 0xff);
    pIPHeader->checksumLow = (UCHAR) (Checksum & 0xff);
}

//
// Function: checksum
//
// Description:
//    This function calculates the 16-bit one's complement sum
//    of the supplied buffer (ICMP) header.
//
USHORT
checksum(
    USHORT *buffer,
    int size
    )
{
    unsigned long cksum=0;

    while (size > 1)
    {
        cksum += *buffer++;
        size -= sizeof(USHORT);
    }
    if (size)
    {
        cksum += *(UCHAR*)buffer;
    }
    cksum = (cksum >> 16) + (cksum & 0xffff);
    cksum += (cksum >>16);
    return (USHORT)(~cksum);
}


VOID
ComputeIcmpChecksum(
    _At_((IcmpHeader*) buf, _Inout_updates_bytes_all_(bufSize)) char *buf,
    _In_ _In_range_(==, sizeof(IcmpHeader)) ULONG bufSize,
    int packetlen
    )
{
        IcmpHeader *icmpv4;

        icmpv4 = (IcmpHeader *)buf;
        icmpv4->i_cksum = 0;
        icmpv4->i_cksum = checksum((USHORT *)buf, packetlen);
}

void
InitEtherHeader(
    PDEVICE_INFO DeviceInfo,
    PETH_HEADER pEthHeader
    )
{

    pEthHeader->EthType = htons(IP_PROT_TYPE);
    memcpy(pEthHeader->DstAddr, DeviceInfo->TargetMacAddr, MAC_ADDR_LEN);
    memcpy(pEthHeader->SrcAddr, DeviceInfo->SrcMacAddr, MAC_ADDR_LEN);


}

BOOL
InitIpHeader(
    PDEVICE_INFO DeviceInfo,
    IpHeader *pIpHeader,
    unsigned short IpLength)
{
    pIpHeader->verlen = 0x45;
    pIpHeader->tos = 0; //Normal service
    pIpHeader->total_len = htons(IpLength);
    pIpHeader->ident = htons(0xABBA);
    pIpHeader->frag_and_flags =  0;
    pIpHeader->ttl = DEFAULT_TTL;
    pIpHeader->proto = PROTOCOL_ICMP;
    #pragma prefast(suppress:__WARNING_IPV6_NAME_RESOLUTION_IPV4_SPECIFIC, "This test app doesn't support IPv6")
    pIpHeader->sourceIP = inet_addr(DeviceInfo->SourceIp);
    #pragma prefast(suppress:__WARNING_IPV6_NAME_RESOLUTION_IPV4_SPECIFIC, "This test app doesn't support IPv6")
    pIpHeader->destIP = inet_addr(DeviceInfo->DestIp);

    return TRUE;
}
VOID WriteComplete(DWORD dwError, DWORD dwBytesTransferred, LPOVERLAPPED pOvl)
{
   TCB* pTCB = (TCB *)pOvl;

    if (dwError) {
        if(dwError == ERROR_DEVICE_NOT_CONNECTED) {
            Display(TEXT("WriteComplete: Device not connected"));
        }
        else {
            Display(TEXT("WriteComplete: Error %x"), dwError);

        }
    }

    DisplayV(TEXT("Write Complete: %x"), dwBytesTransferred);
    HeapFree (GetProcessHeap(), 0, pTCB->Buffer);
    HeapFree (GetProcessHeap(), 0, pTCB);
}

VOID WriteCompleteArp(DWORD dwError, DWORD dwBytesTransferred, LPOVERLAPPED pOvl)
{
   if (dwError) {
        if(dwError == ERROR_DEVICE_NOT_CONNECTED) {
            Display(TEXT("WriteCompleteArp: Device not connected"));
        }
        else {
            Display(TEXT("WriteCompleteArp: Error %x"), dwError);

        }
    }

    DisplayV(TEXT("Write Complete ARP: %x"), dwBytesTransferred);
}

VOID
ReadMacAddrComplete(
    DWORD dwError,
    DWORD dwBytesTransferred,
    LPOVERLAPPED pOvl
    )
{
    if (ERROR_OPERATION_ABORTED != dwError)
    {
        RCB* pRCB = (RCB *)pOvl;

        char *Buffer = pRCB->Buffer;
        PETH_HEADER ethHeader = (PETH_HEADER) Buffer;
        PARP_BODY   pBody;

        PDEVICE_INFO deviceInfo = pRCB->DeviceInfo;
        WCHAR unicodeIpAddr[MAX_LEN];
        char *ipAddr;

        DisplayV(TEXT("ReadMacAddrComplete: %x"), dwBytesTransferred);

         if(ntohs(ethHeader->EthType) == ARP_ETYPE_ARP){

             pBody = (PARP_BODY)(Buffer + sizeof(ETH_HEADER));

             if(ntohs(pBody->opcode) == ARP_RESPONSE){
                #pragma prefast(suppress:__WARNING_IPV6_ADDRESS_STRUCTURE_IPV4_SPECIFIC, "This test app doesn't support IPv6")
                struct in_addr IPAddr = {0};

                memcpy(&IPAddr, &pBody->SenderIpAddr, IP_ADDR_LEN);

                if(memcmp(pBody->DestHwAddr, deviceInfo->SrcMacAddr, MAC_ADDR_LEN) == 0){

                   #pragma prefast(suppress:__WARNING_IPV6_NAME_RESOLUTION_IPV4_SPECIFIC, "This test app doesn't support IPv6")
                   ipAddr = inet_ntoa(IPAddr);

                   if (!MultiByteToWideChar (
                                CP_ACP,
                                0,
                                ipAddr,
                                -1, // string is null terminated
                                unicodeIpAddr,
                                sizeof(unicodeIpAddr)/sizeof(WCHAR)
                                )) {
                        Display(TEXT("AnsitoUnicode conversion failed"));

                   }

                   DisplayV(TEXT("Target IP Address: %ws"), unicodeIpAddr);
                   
                   DisplayV(TEXT("Target Mac Address: %02X-%02X-%02X-%02X-%02X-%02X-"),
                                        pBody->SenderHwAddr[0],
                                        pBody->SenderHwAddr[1],
                                        pBody->SenderHwAddr[2],
                                        pBody->SenderHwAddr[3],
                                        pBody->SenderHwAddr[4],
                                        pBody->SenderHwAddr[5]
                                        );
                   memcpy(deviceInfo->TargetMacAddr, pBody->SenderHwAddr, MAC_ADDR_LEN);

                   SetEvent(deviceInfo->PingEvent);
                   return;
                }
            }
        }

        if(!ReadFileEx(pRCB->DeviceInfo->hDevice, pRCB->Buffer, sizeof(pRCB->Buffer),
            &pRCB->Overlapped, (LPOVERLAPPED_COMPLETION_ROUTINE) ReadMacAddrComplete)){
            Display(TEXT("ReadMacAddrComplete: ReadFileEx failed %x"), GetLastError());
        }

    }
}

BOOL
GetTargetMac(
    PDEVICE_INFO DeviceInfo,
    PRCB pRCB
     )
{
    char        *Buffer = NULL;
    PETH_HEADER ethHeader = NULL;
    PARP_BODY   pBody = NULL;
    unsigned int retries =0;
    DWORD       status;
    HANDLE      hDevice = pRCB->DeviceInfo->hDevice;
    PTCB pTCB  = NULL;

    if(!ReadFileEx(hDevice, pRCB->Buffer, sizeof(pRCB->Buffer),
                &pRCB->Overlapped, (LPOVERLAPPED_COMPLETION_ROUTINE) ReadMacAddrComplete))
    {
        Display(TEXT("GetTargetMac: Error in ReadFile %x"), GetLastError());
        return FALSE;
    }

    //
    // We will try asking for mac address ten times. If we don't get valid response,
    // we will bail out.
    //
    while(retries < 10){

        //
        // Allocate memory for the TCB and it's buffer. Writes are overlapped.
        // We don't wait for the write requests to complete before posting
        // another one.
        //
        pTCB = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(TCB));
        if(!pTCB){
            Display(TEXT("Ping: HeapAlloc Failed"));
            return FALSE;
        }

        pTCB->Buffer = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, ETH_MAX_PACKET_SIZE);
        if(!pTCB->Buffer){
            Display(TEXT("Ping: HeapAlloc Failed"));
            HeapFree (GetProcessHeap(), 0, pTCB);
            return FALSE;
        }

        pTCB->BufferLength = ETH_MAX_PACKET_SIZE;
        memset(&pTCB->Overlapped, 0, sizeof(OVERLAPPED));
        pTCB->DeviceInfo = DeviceInfo;

        Buffer = pTCB->Buffer;
        memset(Buffer, 0, ETH_MIN_PACKET_SIZE);

        ethHeader = (PETH_HEADER) Buffer;


        memcpy(ethHeader->SrcAddr, DeviceInfo->SrcMacAddr, MAC_ADDR_LEN);
        memset(ethHeader->DstAddr, 0xff, MAC_ADDR_LEN);
        ethHeader->EthType = htons(ARP_ETYPE_ARP);        //Network byte order

        pBody = (PARP_BODY)(Buffer + sizeof(ETH_HEADER));
        pBody->hw = htons(ARP_REQUEST);    //Network byte order
        pBody->pro = htons(IP_PROT_TYPE);    //Network byte order
        pBody->hlen = MAC_ADDR_LEN;
        pBody->plen = IP_ADDR_LEN;
        pBody->opcode = 0x0100;

        #pragma prefast(suppress:__WARNING_IPV6_NAME_RESOLUTION_IPV4_SPECIFIC, "This test app doesn't support IPv6")
        pBody->SenderIpAddr = inet_addr(DeviceInfo->SourceIp);
        #pragma prefast(suppress:__WARNING_IPV6_NAME_RESOLUTION_IPV4_SPECIFIC, "This test app doesn't support IPv6")
        pBody->DestIPAddr = inet_addr(DeviceInfo->DestIp);

        memcpy(pBody->SenderHwAddr, DeviceInfo->SrcMacAddr, MAC_ADDR_LEN);

        if(!WriteFileEx(hDevice, pTCB->Buffer, ETH_MIN_PACKET_SIZE, &pTCB->Overlapped, (LPOVERLAPPED_COMPLETION_ROUTINE) WriteComplete))
        {
                Display(TEXT("GetTargetMac WriteFile failed %x"), GetLastError());
                HeapFree (GetProcessHeap(), 0, pTCB->Buffer);
                HeapFree (GetProcessHeap(), 0, pTCB);
                return FALSE;
        }

        retries++;
        //
        // Wait for the PingEvent to be signalled. This even is signalled when the
        // ReadMacAddrComplete received a valid packet
        //
wait:
        status = WaitForSingleObjectEx(DeviceInfo->PingEvent, 1000, TRUE );

        if ( status == WAIT_OBJECT_0 ) {    // event fired, not timeout
            //
            // Got a valid response. Hurray!
            //
            return TRUE;
        }

        if ( status == WAIT_IO_COMPLETION ) {
            // Either Read/Write completed. Go back to waiting until the completion rotuine
            // processes the response and sigals the event.
            goto wait;
        }
        if (status != WAIT_TIMEOUT){
            //
            // It has to be timeout at this point. Or else something fatal, break out.
            Display(TEXT("WaitForSingleObjectEx returned error %d"), status);
            return FALSE;
        }
    }

    return FALSE;
}

BOOL
GetSrcMac(
    HANDLE  Handle,
    PUCHAR  pSrcMacAddr
    )
{
    DWORD       BytesReturned;
    BOOLEAN     bSuccess;
    UCHAR       QueryBuffer[sizeof(NDISPROT_QUERY_OID) + MAC_ADDR_LEN];
    PNDISPROT_QUERY_OID  pQueryOid;


    DisplayV(TEXT("Trying to get src mac address"), NULL);

    pQueryOid = (PNDISPROT_QUERY_OID)&QueryBuffer[0];
    pQueryOid->Oid = OID_802_3_CURRENT_ADDRESS;

    bSuccess = (BOOLEAN)DeviceIoControl(
                            Handle,
                            IOCTL_NDISPROT_QUERY_OID_VALUE,
                            (LPVOID)&QueryBuffer[0],
                            sizeof(QueryBuffer),
                            (LPVOID)&QueryBuffer[0],
                            sizeof(QueryBuffer),
                            &BytesReturned,
                            NULL);

    if (bSuccess)
    {
        DisplayV(TEXT("GetSrcMac: IoControl success"));
        memcpy(pSrcMacAddr, &pQueryOid->Data[0], MAC_ADDR_LEN);


    }
    else
    {
        Display(TEXT("GetSrcMac: IoControl failed %x\n"), GetLastError());
    }

    return (bSuccess);
}

VOID
PrintIpHeader(
    IpHeader *pIpHeader
    )
{
    #pragma prefast(suppress:__WARNING_IPV6_ADDRESS_STRUCTURE_IPV4_SPECIFIC, "This test app doesn't support IPv6")
    struct in_addr IPAddr = {0};

    DisplayV(TEXT("Ip Proto %x"), pIpHeader->proto);
    DisplayV(TEXT("Ip VerLen %x"), pIpHeader->verlen);
    DisplayV(TEXT("Ip Total Len %d"), pIpHeader->total_len);
    DisplayV(TEXT("Ip Ident %d"), pIpHeader->ident);
    DisplayV(TEXT("Ip tos %x"), pIpHeader->tos);

    memcpy(&IPAddr, &pIpHeader->sourceIP, IP_ADDR_LEN);

    // Display(TEXT("Source IP %s"), inet_ntoa(IPAddr));

    memcpy(&IPAddr, &pIpHeader->destIP, IP_ADDR_LEN);

    // Display(TEXT("Dest IP %s"), inet_ntoa(IPAddr));

}


BOOL
SetPacketFilter(
    HANDLE  Handle,
    ULONG  PacketFilter
    )
{
    DWORD       BytesReturned;
    BOOLEAN     bSuccess;
    NDISPROT_SET_OID  SetOid;


    DisplayV(TEXT("Trying to SetPacketFilter"), NULL);

    SetOid.Oid = OID_GEN_CURRENT_PACKET_FILTER;

    memcpy(&SetOid.Data[0], &PacketFilter, sizeof(ULONG));

    bSuccess = (BOOLEAN)DeviceIoControl(
                            Handle,
                            IOCTL_NDISPROT_SET_OID_VALUE,
                            (LPVOID)&SetOid,
                            sizeof(NDISPROT_SET_OID),
                            NULL,
                            0,
                            &BytesReturned,
                            NULL);

    if (!bSuccess)
    {
        Display(TEXT("SetPacketFilter: IoControl failed: %d"), GetLastError());
    }

    return (bSuccess);
}


VOID
ProcessReadBuffer(
    PRCB pRCB,
    ULONG BufferLength
    )
{
    char        *Buffer = pRCB->Buffer;
    PETH_HEADER ethHeader = (PETH_HEADER) Buffer;
    PARP_BODY   pBody = (PARP_BODY)(Buffer + sizeof(ETH_HEADER));
    char        ArpResponse[ETH_MIN_PACKET_SIZE] = {0};
    PETH_HEADER pResEthHeader = (PETH_HEADER)ArpResponse;
    PARP_BODY   pRespBody = (PARP_BODY)(ArpResponse + sizeof(ETH_HEADER));
    OVERLAPPED  ov = {0};
    PDEVICE_INFO deviceInfo = pRCB->DeviceInfo;
    HANDLE      hDevice = deviceInfo->hDevice;

    // Check whether its a broadcast ARP request
    if (ethHeader->DstAddr[0] == 0xff &&
            ethHeader->DstAddr[1] == 0xff )
    {

         if(ntohs(ethHeader->EthType) != ARP_ETYPE_ARP){
            deviceInfo->Sleep = FALSE;
            DisplayV(TEXT("Non an arp eth request"));
            goto End;
         }

         if (IP_PROT_TYPE != ntohs(pBody->pro) ||
             MAC_ADDR_LEN != pBody->hlen ||
             IP_ADDR_LEN != pBody->plen )
        {
            //
            // these are just sanity checks
            //
            deviceInfo->Sleep = FALSE;
            DisplayV(TEXT("Non an arp packet"));
            goto End;
        }

        DisplayV(TEXT("This is an arp request"));

        memcpy(pResEthHeader->DstAddr, ethHeader->SrcAddr, MAC_ADDR_LEN);
        memcpy(pResEthHeader->SrcAddr, deviceInfo->SrcMacAddr, MAC_ADDR_LEN);
        pResEthHeader->EthType = ethHeader->EthType;

        pRespBody->hw = pBody->hw;                                       // Hardware address space. = 00 01

        pRespBody->pro = pBody->pro;                                 // Protocol address space. = 08 00

        pRespBody->hlen = MAC_ADDR_LEN; // 6

        pRespBody->plen = IP_ADDR_LEN; // 4

        pRespBody->opcode = htons(ARP_RESPONSE);                        // Opcode.

        memcpy(pRespBody->SenderHwAddr, deviceInfo->SrcMacAddr, MAC_ADDR_LEN);                     // Destination HW address.

        pRespBody->SenderIpAddr = pBody->DestIPAddr ;                    // Source protocol address.

        memcpy(pRespBody->DestHwAddr, pBody->SenderHwAddr, MAC_ADDR_LEN);                     // Destination HW address.

        pRespBody->DestIPAddr = pBody->SenderIpAddr;

        PostNextRead(pRCB);

        DisplayV(TEXT("Writing Arp response\n"));
        if(!WriteFileEx(hDevice, ArpResponse, sizeof(ArpResponse), &ov, (LPOVERLAPPED_COMPLETION_ROUTINE) WriteCompleteArp))
        {
            Display(TEXT("Couldn't write arp response %d"), GetLastError());
            return ;
        }
    }  
    // Check whether its an unicast ARP request
    else if (ethHeader->DstAddr[0] == deviceInfo->SrcMacAddr[0] && 
             ethHeader->DstAddr[1] == deviceInfo->SrcMacAddr[1] && 
             ethHeader->DstAddr[2] == deviceInfo->SrcMacAddr[2] &&
             ethHeader->DstAddr[3] == deviceInfo->SrcMacAddr[3] &&
             ethHeader->DstAddr[4] == deviceInfo->SrcMacAddr[4] &&
             ethHeader->DstAddr[5] == deviceInfo->SrcMacAddr[5] &&
             ntohs(ethHeader->EthType) == ARP_ETYPE_ARP && 
             IP_PROT_TYPE == ntohs(pBody->pro) && 
             MAC_ADDR_LEN == pBody->hlen &&
             IP_ADDR_LEN == pBody->plen) {

        // If it passed all these checks, its an ARP request
        memcpy(pResEthHeader->DstAddr, ethHeader->SrcAddr, MAC_ADDR_LEN);
        memcpy(pResEthHeader->SrcAddr, deviceInfo->SrcMacAddr, MAC_ADDR_LEN);
        pResEthHeader->EthType = ethHeader->EthType;

        pRespBody->hw = pBody->hw;                                       // Hardware address space. = 00 01

        pRespBody->pro = pBody->pro;                                 // Protocol address space. = 08 00

        pRespBody->hlen = MAC_ADDR_LEN; // 6

        pRespBody->plen = IP_ADDR_LEN; // 4

        pRespBody->opcode = htons(ARP_RESPONSE);                        // Opcode.

        memcpy(pRespBody->SenderHwAddr, deviceInfo->SrcMacAddr, MAC_ADDR_LEN);                     // Destination HW address.

        pRespBody->SenderIpAddr = pBody->DestIPAddr ;                    // Source protocol address.

        memcpy(pRespBody->DestHwAddr, pBody->SenderHwAddr, MAC_ADDR_LEN);                     // Destination HW address.

        pRespBody->DestIPAddr = pBody->SenderIpAddr;

        PostNextRead(pRCB);

        DisplayV(TEXT("Writing Arp response\n"));
        if(!WriteFileEx(hDevice, ArpResponse, sizeof(ArpResponse), &ov, (LPOVERLAPPED_COMPLETION_ROUTINE) WriteCompleteArp))
        {
            Display(TEXT("Couldn't write arp response %d"), GetLastError());
            return ;
        }
    }
    else {

        IpHeader *pIpHeader = (IpHeader *)(Buffer + sizeof(ETH_HEADER));
        IcmpHeader *pIcmpHeader = (IcmpHeader *)(Buffer + sizeof(ETH_HEADER)+sizeof(IpHeader));//TODO: Find the iP len from the packet
        ULONG datalen = BufferLength - sizeof(ETH_HEADER) - sizeof(IpHeader) - sizeof(IcmpHeader);
        #pragma prefast(suppress:__WARNING_IPV6_ADDRESS_STRUCTURE_IPV4_SPECIFIC, "This test app doesn't support IPv6")
        struct in_addr IPAddr = {0};
        char *ipAddr;
        WCHAR unicodeIpAddr[MAX_LEN];

        PrintIpHeader(pIpHeader);

        if(pIpHeader->proto == PROTOCOL_ICMP && pIcmpHeader->i_type == ICMPV4_ECHO_REPLY_TYPE &&
                    pIcmpHeader->i_type == ICMPV4_ECHO_REPLY_CODE){

            memcpy(&IPAddr, &pIpHeader->sourceIP, IP_ADDR_LEN);

            #pragma prefast(suppress:__WARNING_IPV6_NAME_RESOLUTION_IPV4_SPECIFIC, "This test app doesn't support IPv6")
            ipAddr = inet_ntoa(IPAddr);

           if (!MultiByteToWideChar (
                        CP_ACP,
                        0,
                        ipAddr,
                        -1,
                        unicodeIpAddr,
                        sizeof(unicodeIpAddr)/sizeof(unicodeIpAddr[0])
                        )) {
                Display(TEXT("AnsitoUnicode conversion failed"));

           }

            #pragma prefast(suppress:__WARNING_USE_OTHER_FUNCTION, "The recommended function GeTickCount64 is available only on Windows Vista/Server 2008 and above")
            Display(TEXT("Reply %d from %ws: bytes=%d time<%dms TTL=%d"),
                ntohs(pIcmpHeader->i_id),
                unicodeIpAddr,
                datalen,
                (GetTickCount()-ntohl(pIcmpHeader->timestamp)),
                pIpHeader->ttl);

            deviceInfo->NumberOfRequestSent++;
            deviceInfo->Sleep = TRUE;

        }

    }

End:
    PostNextRead(pRCB);
    SetEvent(deviceInfo->PingEvent);

}
VOID
ReadComplete(
    DWORD dwError,
    DWORD dwBytesTransferred,
    LPOVERLAPPED pOvl
    )
{
    if (ERROR_OPERATION_ABORTED != dwError)
    {
        RCB* pRCB = (RCB *)pOvl;
        DisplayV(TEXT("ReadComplete: %d"), dwBytesTransferred);
        ProcessReadBuffer(pRCB, dwBytesTransferred);
    }
}

VOID
PostNextRead(
    RCB *pRCB
    )
{

    if(!ReadFileEx(pRCB->DeviceInfo->hDevice, pRCB->Buffer, sizeof(pRCB->Buffer),
                &pRCB->Overlapped, (LPOVERLAPPED_COMPLETION_ROUTINE) ReadComplete))
    {
        Display(TEXT("Error in ReadFile: %x"), GetLastError());
    }

}


BOOLEAN
Ping(
    PDEVICE_INFO DeviceInfo
    )
{
    HANDLE hDevice = DeviceInfo->hDevice;
    char *icmpbuf;
    char *ipHeader;
    char *etherHeader;
    unsigned int icmpbuflen, packetlen;
    PTCB pTCB = NULL;

    packetlen = sizeof(IcmpHeader) + sizeof(IpHeader) + sizeof(ETH_HEADER);

    // Add in the data size
    if(FAILED(UIntAdd(packetlen, DeviceInfo->PacketSize, &packetlen))) {
        Display(TEXT("Ping: UIntAdd Failed"));
        goto Error;
    }

    pTCB = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(TCB));
    if(!pTCB){
        Display(TEXT("Ping: HeapAlloc Failed"));
        goto Error;
    }

    pTCB->Buffer = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, packetlen);
    if(!pTCB->Buffer){
        Display(TEXT("Ping: HeapAlloc Failed"));
        goto Error;
    }

    pTCB->BufferLength = packetlen;
    memset(&pTCB->Overlapped, 0, sizeof(OVERLAPPED));


    // Allocate the buffer that will contain the ICMP request
    etherHeader = pTCB->Buffer;

    ipHeader = etherHeader + sizeof(ETH_HEADER);

    icmpbuf = ipHeader + sizeof(IpHeader);


    icmpbuflen = sizeof(IcmpHeader) + DeviceInfo->PacketSize;

    InitEtherHeader(DeviceInfo, (PETH_HEADER)etherHeader);


    InitIcmpHeader(icmpbuf, icmpbuflen, DeviceInfo->PacketSize);

    // Set the sequence number and compute the checksum
    SetIcmpSequence(icmpbuf, sizeof(IcmpHeader));

    DisplayV(TEXT("Icmp header %d packetlen  %d"), sizeof(IcmpHeader), icmpbuflen);

    ComputeIcmpChecksum(icmpbuf, sizeof(IcmpHeader), icmpbuflen);

    if(!InitIpHeader(DeviceInfo, (IpHeader *)ipHeader, (USHORT)icmpbuflen+sizeof(IpHeader))){
        goto Error;
    }

    ComputeIPChecksum((IpHeader *)ipHeader);

    PrintIpHeader((IpHeader *)ipHeader);

    if(!WriteFileEx(hDevice, etherHeader, packetlen, &pTCB->Overlapped, (LPOVERLAPPED_COMPLETION_ROUTINE) WriteComplete))
    {
            Display(TEXT("Ping: WriteFile failed %x"), GetLastError());
            goto Error;
    }

    return TRUE;

Error:

    if(pTCB) {
        if(pTCB->Buffer){
            HeapFree (GetProcessHeap(), 0, pTCB->Buffer);
        }
        HeapFree (GetProcessHeap(), 0, pTCB);
    }
    return FALSE;
}

DWORD
PingThread (
    PDEVICE_INFO DeviceInfo
    )
{
    RCB                 RCB;
    HANDLE              hDevice = DeviceInfo->hDevice;
    DWORD               status;


    Display(TEXT("Pinging %ws from %ws with %d bytes of data"),
                            DeviceInfo->UnicodeDestIp,
                            DeviceInfo->UnicodeSourceIp,
                            DeviceInfo->PacketSize);
    Sleep(1000);

    //
    // Every time a ping response is recevied, PingEvent will
    // be signalled.
    //
    DeviceInfo->PingEvent = CreateEvent(NULL, FALSE, FALSE, L"PingEvent");
    if (DeviceInfo->PingEvent == NULL) {
        Display(TEXT("CreateEvent failed 0x%x"), GetLastError());
        goto Exit;
    }

    DeviceInfo->NumberOfRequestSent = 0;
    DeviceInfo->Sleep = FALSE;
    DeviceInfo->TimeOut = 0;
    PacketId = 1;

    //
    // Get the MAC address of the local NIC
    //
    if (!GetSrcMac(hDevice, DeviceInfo->SrcMacAddr))
    {
        Display(TEXT("Failed to obtain local MAC address"));
        goto Exit;
    }

    //
    // Set the hardware filter to receive directed on broadcast packets.
    //
    if (!SetPacketFilter(hDevice, NDIS_PACKET_TYPE_DIRECTED|NDIS_PACKET_TYPE_BROADCAST))
    {
        Display(TEXT("Failed to set packet filter"));
        goto Exit;
    }

    //
    // Initialize read control block. Reads requests are serialized.
    // Only one read is outstanding at any time. We allocate memory
    // for the RCB in the stack.
    //
    RCB.DeviceInfo = DeviceInfo;
    memset(&RCB.Overlapped, 0, sizeof(OVERLAPPED));


    //
    // Get MAC address of the target machine by sending an ARP
    // request. We have the target machine's IP address from the user.
    //
    if(!GetTargetMac(DeviceInfo, &RCB)){
        Display(TEXT("Couldn't find the host"));
        goto Exit;
    }

    //
    // Set the hardware filter to receive directed packets only
    //
    if (!SetPacketFilter(hDevice, NDIS_PACKET_TYPE_DIRECTED))
    {
        Display(TEXT("Failed to set packet filter"));
        goto Exit;
    }

    //
    // Post a read buffer and start sending ping packets.
    //
    PostNextRead(&RCB);

    Ping(DeviceInfo);

    //
    // We will exit out of this thread if the number of ping count
    // exceeds the DEFAULT_SEND_COUNT or the main thread requested
    // us to exit by setting ExitThread value to TRUE.
    //
    while(DeviceInfo->NumberOfRequestSent < DEFAULT_SEND_COUNT
            && DeviceInfo->ExitThread == FALSE){

        status = WaitForSingleObjectEx(DeviceInfo->PingEvent, 1000, TRUE );
        if ( status == WAIT_OBJECT_0 ) {    // event fired, not timeout
            //
            // Probably we received a valid ping response from the target.
            //
            if(DeviceInfo->Sleep){
                Sleep(PING_SLEEP_TIME); // sleep for a sec before sending another ECHO
            }
            Ping(DeviceInfo);
            continue;
        }
        //
        // This is just a notification that either Read/Write operation got
        // completed. We will know the result of the acutal operation later
        // when the APC is called.
        //
        if( status == WAIT_IO_COMPLETION ) {
            continue;
        }
        if (status != WAIT_TIMEOUT){

            Display(TEXT("WaitForSingleObjectEx returned error %d"), status);
            break;
        }
        //
        // It seems like the wait timed out. So let us send another ping
        // and see if we get any response.
        //
        DeviceInfo->TimeOut++;
        if(DeviceInfo->TimeOut > MAX_PING_RETRY) {
            Display(TEXT("No response from the target"));
            break;
        }

        Ping(DeviceInfo);
    }

Exit:

    CloseHandle(DeviceInfo->hDevice);
    DeviceInfo->hDevice = INVALID_HANDLE_VALUE;
    DeviceInfo->ThreadHandle = NULL;
    if (DeviceInfo->PingEvent) {
        CloseHandle(DeviceInfo->PingEvent);
        DeviceInfo->PingEvent = NULL;
    }

    Display(TEXT("PingThread is exiting"));
    return 0;
}





