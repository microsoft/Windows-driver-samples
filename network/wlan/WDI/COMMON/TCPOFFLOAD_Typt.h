#ifndef __INC_TCPOFFLOAD_H
#define __INC_TCPOFFLOAD_H
/*
typedef struct _NDIS_TASK_TCP_IP_CHECKSUM {
  struct {
    ULONG IpOptionsSupported;
    ULONG TcpOptionsSupported;
    ULONG TcpChecksum;
    ULONG UdpChecksum;
    ULONG IpChecksum;
  } V4Transmit;
  struct {
    ULONG IpOptionsSupported;
    ULONG TcpOptionsSupported;
    ULONG TcpChecksum;
    ULONG UdpChecksum;
    ULONG IpChecksum;
  } V4Receive;
  struct {
    ULONG IpOptionsSupported;
    ULONG TcpOptionsSupported;
    ULONG TcpChecksum;
    ULONG UdpChecksum;
  } V6Transmit;
  struct {
    ULONG IpOptionsSupported;
    ULONG TcpOptionsSupported;
    ULONG TcpChecksum;
    ULONG UdpChecksum;
 } V6Receive;
} NDIS_TASK_TCP_IP_CHECKSUM, *PNDIS_TASK_TCP_IP_CHECKSUM;

*/
typedef struct _RT_SUPPORT_CHECKSUM_TYPE{
	u4Byte 	IpOptionsSupported;
    	u4Byte 	TcpOptionsSupported;
    	u4Byte 	TcpChecksum;
    	u4Byte	UdpChecksum;
	u4Byte	IpChecksum;
}RT_SUPPORT_CHECKSUM_TYPE, *PRT_SUPPORT_CHECKSUM_TYPE;



typedef struct _RT_SUPPORT_TCPOFFLOAD_CAP{
	RT_SUPPORT_CHECKSUM_TYPE		IPV4Send;
	RT_SUPPORT_CHECKSUM_TYPE		IPV4Recv;
	RT_SUPPORT_CHECKSUM_TYPE		IPV6Send;
	RT_SUPPORT_CHECKSUM_TYPE		IPV6Recv;
}RT_SUPPORT_TCPOFFLOAD_CAP, *PRT_SUPPORT_TCPOFFLOAD_CAP;



//  IPv4 used Tx : bit1 , Rx : bit2
//  IPv6 used Tx : bit5 , Rx : bit6 
typedef enum _RT_SUPPORT_TCPOFFLOAD_MODE{
		TCPOFFLOAD_DISAB				= 0x00,
		TCPOFFLOAD_IPV4_TX			= 0x01,
		TCPOFFLOAD_IPV4_RX			= 0x02,
		TCPOFFLOAD_IPV4_TRX			= 0x03,
		TCPOFFLOAD_IPV6_TX			= 0x10,
		TCPOFFLOAD_IPV6_RX			= 0x20,
		TCPOFFLOAD_IPV6_TRX			= 0x30,
		TCPOFFLOAD_IPV46_TX			= 0x11,
		TCPOFFLOAD_IPV46_RX			= 0x22,
		TCPOFFLOAD_IPV46_TRX			= 0x33,
}RT_SUPPORT_TCPOFFLOAD_MODE,*PRT_SUPPORT_TCPOFFLOAD_MODE;


typedef enum _RT_TCPOFFLOAD_TX_MODE{
		TCPOFFLOAD_TX_DISAB				= 0x00,
		TCPOFFLOAD_TX_IPV4_IP				= 0x01,
		TCPOFFLOAD_TX_IPV4_TCP			= 0x02,
		TCPOFFLOAD_TX_IPV4_UDP			= 0x04,
		TCPOFFLOAD_TX_IPV6_IP				= 0x10,
		TCPOFFLOAD_TX_IPV6_TCP			= 0x20,
		TCPOFFLOAD_TX_IPV6_UDP			= 0x40,
}RT_TCPOFFLOAD_TX_MODE,*PRT_TCPOFFLOAD_TX_MODE;


#define 	TCPOFFLOAD_SUPPORT_OPTION(_pSetSupTCPCap)				\
	(															\
		((_pSetSupTCPCap)->IPV4Recv.IpOptionsSupported == 1 )		||	\
		((_pSetSupTCPCap)->IPV4Recv.TcpOptionsSupported == 1)		||	\
		((_pSetSupTCPCap)->IPV4Send.IpOptionsSupported == 1 )		||	\
		((_pSetSupTCPCap)->IPV4Send.TcpOptionsSupported == 1)		||	\
		((_pSetSupTCPCap)->IPV6Recv.IpOptionsSupported == 1 )		||	\
		((_pSetSupTCPCap)->IPV6Recv.TcpOptionsSupported == 1)		||	\
		((_pSetSupTCPCap)->IPV6Send.IpOptionsSupported == 1 )		||	\
		((_pSetSupTCPCap)->IPV6Send.TcpOptionsSupported == 1)			\
	)
	
#define 	TCPOFFLOAD_SUPPORT_IPV4(_pSetSupTCPCap)				\
	(															\
		((_pSetSupTCPCap)->IPV4Recv.IpOptionsSupported == 1 )		||	\
		((_pSetSupTCPCap)->IPV4Recv.TcpOptionsSupported == 1)		||	\
		((_pSetSupTCPCap)->IPV4Recv.IpChecksum == 1)				||	\
		((_pSetSupTCPCap)->IPV4Recv.TcpChecksum == 1)			||	\
		((_pSetSupTCPCap)->IPV4Recv.UdpChecksum == 1)			||	\
		((_pSetSupTCPCap)->IPV4Send.IpOptionsSupported == 1 )		||	\
		((_pSetSupTCPCap)->IPV4Send.TcpOptionsSupported == 1)		||	\
		((_pSetSupTCPCap)->IPV4Send.IpChecksum == 1)				||	\
		((_pSetSupTCPCap)->IPV4Send.TcpChecksum == 1)			||	\
		((_pSetSupTCPCap)->IPV4Send.UdpChecksum == 1)				\
	)

#define 	TCPOFFLOAD_SUPPORT_IPV4_TX(_pSetSupTCPCap)				\
	(																\
		((_pSetSupTCPCap)->IPV4Send.IpOptionsSupported == 1 )		||	\
		((_pSetSupTCPCap)->IPV4Send.TcpOptionsSupported == 1)		||	\
		((_pSetSupTCPCap)->IPV4Send.IpChecksum == 1)				||	\
		((_pSetSupTCPCap)->IPV4Send.TcpChecksum == 1)			||	\
		((_pSetSupTCPCap)->IPV4Send.UdpChecksum == 1)				\
	)

#define 	TCPOFFLOAD_SUPPORT_IPV4_RX(_pSetSupTCPCap)				\
	(															\
		((_pSetSupTCPCap)->IPV4Recv.IpOptionsSupported == 1 )		||	\
		((_pSetSupTCPCap)->IPV4Recv.TcpOptionsSupported == 1)		||	\
		((_pSetSupTCPCap)->IPV4Recv.IpChecksum == 1)				||	\
		((_pSetSupTCPCap)->IPV4Recv.TcpChecksum == 1)			||	\
		((_pSetSupTCPCap)->IPV4Recv.UdpChecksum == 1)				\
	)



#define 	TCPOFFLOAD_SUPPORT_IPV6(_pSetSupTCPCap)				\
	(															\
		((_pSetSupTCPCap)->IPV6Recv.IpOptionsSupported == 1 )		||	\
		((_pSetSupTCPCap)->IPV6Recv.TcpOptionsSupported == 1)		||	\
		((_pSetSupTCPCap)->IPV6Recv.IpChecksum == 1)				||	\
		((_pSetSupTCPCap)->IPV6Recv.TcpChecksum == 1)			||	\
		((_pSetSupTCPCap)->IPV6Recv.UdpChecksum == 1)			||	\
		((_pSetSupTCPCap)->IPV6Send.IpOptionsSupported == 1 )		||	\
		((_pSetSupTCPCap)->IPV6Send.TcpOptionsSupported == 1)		||	\
		((_pSetSupTCPCap)->IPV6Send.IpChecksum == 1)				||	\
		((_pSetSupTCPCap)->IPV6Send.TcpChecksum == 1)			||	\
		((_pSetSupTCPCap)->IPV6Send.UdpChecksum == 1)				\
	)

#define 	TCPOFFLOAD_SUPPORT_IPV6_TX(_pSetSupTCPCap)				\
	(															\
		((_pSetSupTCPCap)->IPV6Send.IpOptionsSupported == 1 )		||	\
		((_pSetSupTCPCap)->IPV6Send.TcpOptionsSupported == 1)		||	\
		((_pSetSupTCPCap)->IPV6Send.IpChecksum == 1)				||	\
		((_pSetSupTCPCap)->IPV6Send.TcpChecksum == 1)			||	\
		((_pSetSupTCPCap)->IPV6Send.UdpChecksum == 1)				\
	)

#define 	TCPOFFLOAD_SUPPORT_IPV6_RX(_pSetSupTCPCap)				\
	(															\
		((_pSetSupTCPCap)->IPV6Recv.IpOptionsSupported == 1 )		||	\
		((_pSetSupTCPCap)->IPV6Recv.TcpOptionsSupported == 1)		||	\
		((_pSetSupTCPCap)->IPV6Recv.IpChecksum == 1)				||	\
		((_pSetSupTCPCap)->IPV6Recv.TcpChecksum == 1)			||	\
		((_pSetSupTCPCap)->IPV6Recv.UdpChecksum == 1)				\
	)
#endif 
