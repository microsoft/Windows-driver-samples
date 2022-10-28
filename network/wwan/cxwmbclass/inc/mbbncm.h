//
//    Copyright (C) Microsoft.  All rights reserved.
//

#pragma once

#define NCM10_CC_INTERFACE_CLASS     (0x02)
#define NCM10_CC_INTERFACE_SUBCLASS  (0x0d)
#define NCM10_CC_INTERFACE_PROTOCOL  (0x00)

#define MBIM_CC_INTERFACE_CLASS     (0x02)
#define MBIM_CC_INTERFACE_SUBCLASS  (0x0e)
#define MBIM_CC_INTERFACE_PROTOCOL  (0x00)
#define MBIM_CC_INTERFACE_NBL_PROTOCOL         (0x01)
#define MBIM_CC_INTERFACE_NETPACKET_PROTOCOL  (0x02)


#define MBIM_DC_INTERFACE_CLASS     (0x0A)
#define MBIM_DC_INTERFACE_SUBCLASS  (0x00)
#define MBIM_DC_INTERFACE_PROTOCOL  (0x02)

#define MBIM_MBB_FUNCDESC_MIN_VERSION (0x0100)
#define MBIM_MBB_FUNCDESC_EXTENDED_MIN_VERSION (0x0100)

#define SEND_ENCAPSULATE_COMMAND    (0x00)
#define GET_ENCAPSULATE_RESPONSE    (0x01)
#define RESET_FUNCTION              (0x05)
#define SET_ETHERNET_POWER_MANAGEMENT_PATTERN_FILTER       (0x41)
#define GET_ETHERNET_POWER_MANAGEMENT_PATTERN_FILTER       (0x42)
#define SET_PACKET_FILTER           (0x43)
#define GET_STATISTIC               (0x44)

#define GET_NTB_PARAMETERS          (0x80)
#define GET_NBT_FORMAT              (0x83)
#define SET_NBT_FORMAT              (0x84)
#define GET_NTB_INPUT_SIZE          (0x85)
#define SET_NTB_INPUT_SIZE          (0x86)
#define GET_MAX_DATAGRAM_SIZE       (0x87)


#define MBIM_MIN_SEGMENT_SIZE       (1280)
#define MBIM_MAX_PACKET_FILTER_SIZE (192)
#define MBIM_MIN_NUMBER_OF_PACKET_FILTERS   (16)
#define MBIM_MIN_MTU_SIZE       (1280)
#define MBIM_MAX_MTU_SIZE       (1500)

#define USB_CDC_CS_DESCRIPTOR_TYPE  (0x24)

#define USB_CDC_CS_DESCRIPTOR_SUBTYPE      (0x0)
#define USB_CDC_UNION_DESCRIPTOR_SUBTYPE   (0x6)
#define USB_CDC_CS_ECM_DESCRIPTOR_SUBTYPE  (0xf)
#define USB_CDC_CS_NCM_DESCRIPTOR_SUBTYPE  (0x1a)
#define USB_CDC_CS_MBB_DESCRIPTOR_SUBTYPE  (0x1b)
#define USB_CDC_CS_MBB_DESCRIPTOR_EXTENDED_SUBTYPE  (0x1c)


#define USB_CDC_NOTIFICATION_NETWORK_CONNECTION       (0x00)
#define USB_CDC_NOTIFICATION_RESPONSE_AVAILABLE       (0x01)
#define USB_CDC_NOTIFICATION_CONNECTION_SPEED_CHANGE  (0x2a)

#define NCM_NETWORK_CAPS_SET_FILTER_SUPPORTED               (1 << 0)
#define NCM_NETWORK_CAPS_GET_SET_ADDRESS_SUPPORTED          (1 << 1)
#define NCM_NETWORK_CAPS_ENCAPSULATED_COMMAND_SUPPORTED     (1 << 2)
#define NCM_NETWORK_CAPS_MAX_DATAGRAM_SUPPORTED             (1 << 3)
#define NCM_NETWORK_CAPS_GET_SET_CRC_SUPPORTED              (1 << 4)
#define NCM_NETWORK_CAPS_GET_8_BYTE_INPUT_SIZE_SUPPORTED    (1 << 5)

#define NCM_NTB_FORMAT_16_BIT                               (1 << 0)
#define NCM_NTB_FORMAT_32_BIT                               (1 << 1)

#define NCM_PACKET_FILTER_DIRECTED                          (1 << 2)

#define NCM_SET_NTB_FORMAT_16_BIT                           (0x0)
#define NCM_SET_NTB_FORMAT_32_BIT                           (0x1)



#define NCM_STAT_RESERVED           0x00
#define NCM_XMIT_OK                 0x01
#define NCM_RCV_OK                  0x02
#define NCM_XMIT_ERROR              0x03
#define NCM_RCV_ERROR               0x04
#define NCM_RCV_NO_BUFFER           0x05
#define NCM_DIRECTED_BYTES_XMIT     0x06
#define NCM_DIRECTED_FRAMES_XMIT    0x07
#define NCM_MULTICAST_BYTES_XMIT    0x08
#define NCM_MULTICAST_FRAMES_XMIT   0x09
#define NCM_BROADCAST_BYTES_XMIT    0x0A
#define NCM_BROADCAST_FRAMES_XMIT   0x0B
#define NCM_DIRECTED_BYTES_RCV      0x0C
#define NCM_DIRECTED_FRAMES_RCV     0x0D
#define NCM_MULTICAST_BYTES_RCV     0x0E
#define NCM_MULTICAST_FRAMES_RCV    0x0F
#define NCM_BROADCAST_BYTES_RCV     0x10
#define NCM_BROADCAST_FRAMES_RCV    0x11
#define NCM_RCV_CRC_ERROR           0x12
#define NCM_TRANSMIT_QUEUE_LENGTH   0x13
#define NCM_RCV_ERROR_ALIGNMENT     0x14
#define NCM_XMIT_ONE_COLLISION      0x15
#define NCM_XMIT_MORE_COLLISIONS    0x16
#define NCM_XMIT_DEFERRED           0x17
#define NCM_XMIT_MAX_COLLISIONS     0x18
#define NCM_RCV_OVERRUN             0x19
#define NCM_XMIT_UNDERRUN           0x1A
#define NCM_XMIT_HEARTBEAT_FAILURE  0x1B
#define NCM_XMIT_TIMES_CRS_LOST     0x1C
#define NCM_XMIT_LATE_COLLISIONS    0x1D

#define NCM_NDP_SESSION_SHIFT       24
#define NCM_NDP_SIGNATURE_MASK      0x00FFFFFF

#define NCM_NTH16_SIG               'HMCN'
#define NCM_NTH32_SIG               'hmcn'

#define NCM_NDP16_IPS               'SPI'
#define NCM_NDP16_VENDOR            'SSD'

#define NCM_NDP32_IPS               'spi'
#define NCM_NDP32_VENDOR            'ssd'

#define NCM_CID_SIG                 'dcbm'

#pragma pack( push,1)

typedef struct _NCM_NTB_PARAMETER {

    USHORT          wLength;
    USHORT          bmNtbFormatSupported;
    ULONG           dwNtbInMaxSize;
    USHORT          wNdpInDivisor;
    USHORT          wNdpInPayloadRemainder;
    USHORT          wNdpInAlignment;
    USHORT          wReserved1;
    ULONG           dwNtbOutMaxSize;
    USHORT          wNdpOutDivisor;
    USHORT          wNdpOutPayloadRemainder;
    USHORT          wNdpOutAlignment;
    USHORT          wNtbOutMaxDatagrams;

} NCM_NTB_PARAMETER, *PNCM_NTB_PARAMETER;

typedef struct _USB_CS_DESCRIPTOR {

    BYTE    bLength;
    BYTE    bDescriptorType;
    BYTE    bSubType;

} USB_CS_DESCRIPTOR, *PUSB_CS_DESCRIPTOR;

typedef struct _USB_CS_UNION_DESCRIPTOR {

    BYTE    bLength;
    BYTE    bDescriptorType;
    BYTE    bSubType;
    BYTE    bControlInterface;
    BYTE    bSubordinateInterface[1];

} USB_CS_UNION_DESCRIPTOR, *PUSB_CS_UNION_DESCRIPTOR;


typedef struct _USB_CS_CDC_DESCRIPTOR {

    BYTE    bLength;
    BYTE    bDescriptorType;
    BYTE    bSubType;
    USHORT  wCDCVersion;

} USB_CS_CDC_DESCRIPTOR, *PUSB_CS_CDC_DESCRIPTOR;

typedef struct _USB_CS_ECM_DESCRIPTOR {

    BYTE    bLength;
    BYTE    bDescriptorType;
    BYTE    bSubType;
    BYTE    iMacAddress;
    ULONG   bmEthernetStatistics;
    USHORT  wMaxSegmentSize;
    USHORT  wNumberMCFilters;
    BYTE    bNumberPowerFilters;

} USB_CS_ECM_DESCRIPTOR, *PUSB_CS_ECM_DESCRIPTOR;

typedef struct _USB_CS_NCM_DESCRIPTOR {

    BYTE    bLength;
    BYTE    bDescriptorType;
    BYTE    bSubType;
    USHORT  wNcmVersion;
    BYTE    bmNetworkCapabilities;

} USB_CS_NCM_DESCRIPTOR, *PUSB_CS_NCM_DESCRIPTOR;


typedef struct _USB_CS_MBB_DESCRIPTOR {

    BYTE    bLength;
    BYTE    bDescriptorType;
    BYTE    bSubType;
    USHORT  wMbbVersion;
    USHORT  wMaxControlMessage;
    BYTE    bNumberPowerFilters;
    BYTE    bMaxFilterSize;
    USHORT  wMaxSegmentSize;
    BYTE    bmNetworkCapabilities;

} USB_CS_MBB_DESCRIPTOR, *PUSB_CS_MBB_DESCRIPTOR;

typedef struct _USB_CS_MBB_DESCRIPTOR_EXTENDED {

    BYTE    bLength;
    BYTE    bDescriptorType;
    BYTE    bSubType;
    USHORT  wMbbVersion;
    BYTE    bMaxOutstandingCommandMessages;
    USHORT  wMTU;

} USB_CS_MBB_DESCRIPTOR_EXTENDED, *PUSB_CS_MBB_DESCRIPTOR_EXTENDED;

typedef struct _USB_CDC_NOTIFICATION {

    BYTE    bmRequestType;
    BYTE    bNotificationCode;
    USHORT  wValue;
    USHORT  wIndex;
    USHORT  wLength;

} USB_CDC_NOTIFICATION, *PUSB_CDC_NOTIFICATION;

typedef struct _USB_CDC_NOTIFICATION_SPEED_CHANGE {

    USB_CDC_NOTIFICATION    NotificationHeader;
    ULONG                   DownLinkBitRate;
    ULONG                   UpLinkBitRate;

} USB_CDC_NOTIFICATION_SPEED_CHANGE, *PUSB_CDC_NOTIFICATION_SPEED_CHANGE;


typedef struct _NCM_NTH16 {

    ULONG       dwSignature;
    USHORT      wHeaderLength;
    USHORT      wSequence;
    USHORT      wBlockLength;
    USHORT      wFpIndex;

} NCM_NTH16, *PNCM_NTH16;

typedef struct _NCM_NTH32 {

    ULONG       dwSignature;
    USHORT      wHeaderLength;
    USHORT      wSequence;
    ULONG       dwBlockLength;
    ULONG       dwFpIndex;

} NCM_NTH32, *PNCM_NTH32;

typedef struct _NCM_NDP16_DATAGRAM {

    USHORT      wDatagramIndex;
    USHORT      wDatagramLength;

} NCM_NDP16_DATAGRAM, *PNCM_NDP16_DATAGRAM;


typedef struct _NCM_NDP16 {

    ULONG       dwSignature;
    USHORT      wLength;
    USHORT      wNextFpIndex;

    NCM_NDP16_DATAGRAM  Datagram[1];

} NCM_NDP16, *PNCM_NDP16;


typedef struct _NCM_NDP32_DATAGRAM {

    ULONG       dwDatagramIndex;
    ULONG       dwDatagramLength;

} NCM_NDP32_DATAGRAM, *PNCM_NDP32_DATAGRAM;


typedef struct _NCM_NDP32 {

    ULONG       dwSignature;
    USHORT      wLength;
    USHORT      wReserved6;
    ULONG       dwNextFpIndex;
    ULONG       dwReserved12;

    NCM_NDP32_DATAGRAM  Datagram[1];

} NCM_NDP32, *PNCM_NDP32;


#pragma pack( pop)

//
// NTB
//

#define MBB_NTB_GET_OFFSET(NTH,_X_)     ((ULONG)(((SIZE_T)(_X_))-((SIZE_T)(NTH))))
#define MBB_NTB_GET_SEQUENCE(NTH)       (((PNCM_NTH32)(NTH))->wSequence)
#define MBB_NTB_GET_SIGNATURE(NTH)      (((PNCM_NTH32)(NTH))->dwSignature)
#define MBB_NTB_GET_HEADER_LENGTH(NTH)  (((PNCM_NTH32)(NTH))->wHeaderLength)
#define MBB_NTB_IS_32BIT(NTH)           (MBB_NTB_GET_SIGNATURE(NTH) == NCM_NTH32_SIG)
#define MBB_NTB_IS_16BIT(NTH)           (MBB_NTB_GET_SIGNATURE(NTH) == NCM_NTH16_SIG)

//
// NTB32

#define MBB_NTH32_GET_SEQUENCE(NTH)                     MBB_NTB_GET_SEQUENCE(NTH)
#define MBB_NTH32_GET_SIGNATURE(NTH)                    MBB_NTB_GET_SIGNATURE(NTH)
#define MBB_NTH32_GET_HEADER_LENGTH(NTH)                MBB_NTB_GET_HEADER_LENGTH(NTH)
#define MBB_NTH32_GET_BLOCK_LENGTH(NTH)                 ((NTH)->dwBlockLength)
#define MBB_NTH32_GET_FIRST_NDP_OFFSET(NTH)             ((NTH)->dwFpIndex)
#define MBB_NTH32_GET_FIRST_NDP(NTH)                    ((MBB_NTH32_GET_FIRST_NDP_OFFSET(NTH) != 0)? ((PNCM_NDP32)(((PCHAR)(NTH)) + MBB_NTH32_GET_FIRST_NDP_OFFSET(NTH))): NULL)
#define MBB_NTH32_IS_VALID_BLOCK_LENGTH(NTH,MAXLEN)     (MBB_NTH32_GET_BLOCK_LENGTH(NTH) <= (MAXLEN))
#define MBB_NTH32_IS_VALID_HEADER_LENGTH(NTH)           (MBB_NTB_GET_HEADER_LENGTH(NTH) == sizeof(NCM_NTH32))
#define MBB_NTH32_IS_VALID_SIGNATURE(NTH)               (MBB_NTB_GET_SIGNATURE(NTH) == NCM_NTH32_SIG)
#define MBB_NTH32_IS_VALID_FIRST_NDP(NTH) \
    ( \
        MBB_NTH32_GET_FIRST_NDP_OFFSET(NTH)<= (ULONG_MAX - sizeof(NCM_NDP32)) && \
        (MBB_NTH32_GET_FIRST_NDP_OFFSET(NTH) + sizeof(NCM_NDP32)) <= MBB_NTH32_GET_BLOCK_LENGTH(NTH) \
    )

#define MBB_NDP32_GET_SIGNATURE(NDP)                    ((NDP)->dwSignature)
#define MBB_NDP32_GET_SESSIONID(NDP)                    (MBB_NDP32_GET_SIGNATURE(NDP) >> NCM_NDP_SESSION_SHIFT)
#define MBB_NDP32_GET_SIGNATURE_TYPE(NDP)               (MBB_NDP32_GET_SIGNATURE(NDP) & NCM_NDP_SIGNATURE_MASK)
#define MBB_NDP32_GET_LENGTH(NDP)                       ((NDP)->wLength)
#define MBB_NDP32_GET_NEXT_NDP_OFFSET(NDP)              ((NDP)->dwNextFpIndex)
#define MBB_NDP32_GET_NEXT_NDP(NTH,NDP)                 ((MBB_NDP32_GET_NEXT_NDP_OFFSET(NDP) != 0)? ((PNCM_NDP32)(((PCHAR)(NTH)) + MBB_NDP32_GET_NEXT_NDP_OFFSET(NDP))): NULL)
#define MBB_NDP32_GET_DATAGRAM_COUNT(NDP)               ((MBB_NDP32_GET_LENGTH(NDP)-FIELD_OFFSET(NCM_NDP32,Datagram)) / sizeof(NCM_NDP32_DATAGRAM))
#define MBB_NDP32_GET_DATAGRAM_OFFSET(NDP,IDX)          ((NDP)->Datagram[(IDX)].dwDatagramIndex)
#define MBB_NDP32_GET_DATAGRAM_LENGTH(NDP,IDX)          ((NDP)->Datagram[(IDX)].dwDatagramLength)
#define MBB_NDP32_GET_DATAGRAM(NTH,NDP,IDX)             ((MBB_NDP32_GET_DATAGRAM_OFFSET(NDP,IDX) != 0)? (((PCHAR)(NTH)) + MBB_NDP32_GET_DATAGRAM_OFFSET(NDP,IDX)): NULL)
#define MBB_NDP32_IS_DATAGRAM_PRESENT(NDP,IDX)          ((FIELD_OFFSET(NCM_NDP32,Datagram)+((IDX)*sizeof(NCM_NDP32_DATAGRAM))) <= MBB_NDP32_GET_LENGTH(NDP))

#define MBB_NTB32_IS_END_DATAGRAM(NTH,NDP,IDX) \
    ( \
        MBB_NDP32_IS_DATAGRAM_PRESENT(NDP,IDX) && \
        ( \
            MBB_NDP32_GET_DATAGRAM_OFFSET(NDP,IDX) == 0 || \
            MBB_NDP32_GET_DATAGRAM_LENGTH(NDP,IDX) == 0 \
        ) \
    )
#define MBB_NTB32_IS_VALID_DATAGRAM(NTH,NDP,IDX) \
    ( \
        MBB_NDP32_IS_DATAGRAM_PRESENT(NDP,IDX) && \
        MBB_NDP32_GET_DATAGRAM_OFFSET(NDP,IDX) != 0 && \
        MBB_NDP32_GET_DATAGRAM_LENGTH(NDP,IDX) != 0 && \
        MBB_NDP32_GET_DATAGRAM_OFFSET(NDP,IDX) < MBB_NTH32_GET_BLOCK_LENGTH(NTH) && \
        MBB_NDP32_GET_DATAGRAM_OFFSET(NDP,IDX) <= (ULONG_MAX - MBB_NDP32_GET_DATAGRAM_LENGTH(NDP,IDX)) && \
        (MBB_NDP32_GET_DATAGRAM_OFFSET(NDP,IDX) + MBB_NDP32_GET_DATAGRAM_LENGTH(NDP,IDX)) <= MBB_NTH32_GET_BLOCK_LENGTH(NTH) \
    )
#define MBB_NTB32_IS_VALID_NDP_LENGTH(NTH,NDP) \
    ( \
        (MBB_NTB_GET_OFFSET(NTH,NDP) < MBB_NTH32_GET_BLOCK_LENGTH(NTH)) && \
        (MBB_NDP32_GET_LENGTH(NDP) >= FIELD_OFFSET(NCM_NDP32,Datagram)) && \
        (MBB_NTB_GET_OFFSET(NTH,NDP) <= ULONG_MAX - MBB_NDP32_GET_LENGTH(NDP)) && \
        ((MBB_NTB_GET_OFFSET(NTH,NDP)+MBB_NDP32_GET_LENGTH(NDP)) <= MBB_NTH32_GET_BLOCK_LENGTH(NTH)) \
    )
#define MBB_NTB32_IS_VALID_NDP_SIGNATURE(NDP) \
    ( \
        ((MBB_NDP32_GET_SIGNATURE(NDP) & NCM_NDP_SIGNATURE_MASK) == NCM_NDP32_IPS)  || \
        ((MBB_NDP32_GET_SIGNATURE(NDP) & NCM_NDP_SIGNATURE_MASK) == NCM_NDP32_VENDOR)  \
    )

//
// NTB16
//

#define MBB_NTH16_GET_SEQUENCE(NTH)                     MBB_NTB_GET_SEQUENCE(NTH)
#define MBB_NTH16_GET_SIGNATURE(NTH)                    MBB_NTB_GET_SIGNATURE(NTH)
#define MBB_NTH16_GET_HEADER_LENGTH(NTH)                MBB_NTB_GET_HEADER_LENGTH(NTH)
#define MBB_NTH16_GET_BLOCK_LENGTH(NTH)                 ((NTH)->wBlockLength)
#define MBB_NTH16_GET_FIRST_NDP_OFFSET(NTH)             ((NTH)->wFpIndex)
#define MBB_NTH16_GET_FIRST_NDP(NTH)                    ((MBB_NTH16_GET_FIRST_NDP_OFFSET(NTH) != 0)? ((PNCM_NDP16)(((PCHAR)(NTH)) + MBB_NTH16_GET_FIRST_NDP_OFFSET(NTH))): NULL)
#define MBB_NTH16_IS_VALID_BLOCK_LENGTH(NTH,MAXLEN)     (MBB_NTH16_GET_BLOCK_LENGTH(NTH) <= (MAXLEN))
#define MBB_NTH16_IS_VALID_HEADER_LENGTH(NTH)           (MBB_NTB_GET_HEADER_LENGTH(NTH) == sizeof(NCM_NTH16))
#define MBB_NTH16_IS_VALID_SIGNATURE(NTH)               (MBB_NTB_GET_SIGNATURE(NTH) == NCM_NTH16_SIG)
#define MBB_NTH16_IS_VALID_FIRST_NDP(NTH) \
    ( \
        MBB_NTH16_GET_FIRST_NDP_OFFSET(NTH)  <= (USHRT_MAX - sizeof(NCM_NDP16)) &&\
        (MBB_NTH16_GET_FIRST_NDP_OFFSET(NTH) + sizeof(NCM_NDP16)) <= MBB_NTH16_GET_BLOCK_LENGTH(NTH) \
    )

#define MBB_NDP16_GET_SIGNATURE(NDP)                    ((NDP)->dwSignature)
#define MBB_NDP16_GET_SESSIONID(NDP)                    (MBB_NDP16_GET_SIGNATURE(NDP) >> NCM_NDP_SESSION_SHIFT)
#define MBB_NDP16_GET_SIGNATURE_TYPE(NDP)               (MBB_NDP16_GET_SIGNATURE(NDP) & NCM_NDP_SIGNATURE_MASK)
#define MBB_NDP16_GET_LENGTH(NDP)                       ((NDP)->wLength)
#define MBB_NDP16_GET_NEXT_NDP_OFFSET(NDP)              ((NDP)->wNextFpIndex)
#define MBB_NDP16_GET_NEXT_NDP(NTH,NDP)                 ((MBB_NDP16_GET_NEXT_NDP_OFFSET(NDP) != 0)? ((PNCM_NDP16)(((PCHAR)(NTH)) + MBB_NDP16_GET_NEXT_NDP_OFFSET(NDP))): NULL)
#define MBB_NDP16_GET_DATAGRAM_COUNT(NDP)               ((MBB_NDP16_GET_LENGTH(NDP)-FIELD_OFFSET(NCM_NDP16,Datagram)) / sizeof(NCM_NDP16_DATAGRAM))
#define MBB_NDP16_GET_DATAGRAM_OFFSET(NDP,IDX)          ((NDP)->Datagram[(IDX)].wDatagramIndex)
#define MBB_NDP16_GET_DATAGRAM_LENGTH(NDP,IDX)          ((NDP)->Datagram[(IDX)].wDatagramLength)
#define MBB_NDP16_GET_DATAGRAM(NTH,NDP,IDX)             ((MBB_NDP16_GET_DATAGRAM_OFFSET(NDP,IDX) != 0)? (((PCHAR)(NTH)) + MBB_NDP16_GET_DATAGRAM_OFFSET(NDP,IDX)): NULL)
#define MBB_NDP16_IS_DATAGRAM_PRESENT(NDP,IDX)          ((FIELD_OFFSET(NCM_NDP16,Datagram)+((IDX)*sizeof(NCM_NDP16_DATAGRAM))) <= MBB_NDP16_GET_LENGTH(NDP))

#define MBB_NTB16_IS_END_DATAGRAM(NTH,NDP,IDX) \
    ( \
        MBB_NDP16_IS_DATAGRAM_PRESENT(NDP,IDX) && \
        ( \
            MBB_NDP16_GET_DATAGRAM_OFFSET(NDP,IDX) == 0 || \
            MBB_NDP16_GET_DATAGRAM_LENGTH(NDP,IDX) == 0 \
        ) \
    )
#define MBB_NTB16_IS_VALID_DATAGRAM(NTH,NDP,IDX) \
    ( \
        MBB_NDP16_IS_DATAGRAM_PRESENT(NDP,IDX) && \
        MBB_NDP16_GET_DATAGRAM_OFFSET(NDP,IDX) != 0 && \
        MBB_NDP16_GET_DATAGRAM_LENGTH(NDP,IDX) != 0 && \
        MBB_NDP16_GET_DATAGRAM_OFFSET(NDP,IDX) < MBB_NTH16_GET_BLOCK_LENGTH(NTH) && \
        MBB_NDP16_GET_DATAGRAM_OFFSET(NDP,IDX) <= (USHRT_MAX - MBB_NDP16_GET_DATAGRAM_LENGTH(NDP,IDX)) &&\
        (MBB_NDP16_GET_DATAGRAM_OFFSET(NDP,IDX) + MBB_NDP16_GET_DATAGRAM_LENGTH(NDP,IDX)) <= MBB_NTH16_GET_BLOCK_LENGTH(NTH) \
    )
#define MBB_NTB16_IS_VALID_NDP_LENGTH(NTH,NDP) \
    ( \
        (MBB_NTB_GET_OFFSET(NTH,NDP) < MBB_NTH16_GET_BLOCK_LENGTH(NTH)) && \
        (MBB_NDP16_GET_LENGTH(NDP) >= FIELD_OFFSET(NCM_NDP16,Datagram)) && \
        (MBB_NTB_GET_OFFSET(NTH,NDP) <= (ULONG)(USHRT_MAX - MBB_NDP16_GET_LENGTH(NDP))) &&\
        ((MBB_NTB_GET_OFFSET(NTH,NDP)+MBB_NDP16_GET_LENGTH(NDP)) <= MBB_NTH16_GET_BLOCK_LENGTH(NTH)) \
    )
#define MBB_NTB16_IS_VALID_NDP_SIGNATURE(NDP) \
    ( \
        ((MBB_NDP16_GET_SIGNATURE(NDP) & NCM_NDP_SIGNATURE_MASK) == NCM_NDP16_IPS)  || \
        ((MBB_NDP16_GET_SIGNATURE(NDP) & NCM_NDP_SIGNATURE_MASK) == NCM_NDP16_VENDOR)  \
    )

