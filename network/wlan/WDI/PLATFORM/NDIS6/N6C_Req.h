#ifndef __INC_N6C_REQ_H
#define __INC_N6C_REQ_H

#pragma pack(1)

#define NIC_DRIVER_VERSION60		0x0600
#define NIC_HEADER_SIZE				24  // short header size
#define NIC_MAX_PACKET_SIZE		(sMaxMpduLng - 4)
#define NIC_MAX_SEND_PACKETS		10		// max number of send packets the MiniportSendPackets function can accept

// For Meeting House, Realtek Vender ID querying. Added by Annie, 2005-07-20.
#define REALTEK_VENDER_ID		0x004c0e00

#define NIC_SUPPORTED_FILTERS \
			NDIS_PACKET_TYPE_DIRECTED | \
			NDIS_PACKET_TYPE_MULTICAST | \
			NDIS_PACKET_TYPE_ALL_MULTICAST | \
			NDIS_PACKET_TYPE_BROADCAST | \
			NDIS_PACKET_TYPE_PROMISCUOUS | \
			NDIS_PACKET_TYPE_802_11_RAW_DATA | \
			NDIS_PACKET_TYPE_802_11_DIRECTED_MGMT | \
			NDIS_PACKET_TYPE_802_11_MULTICAST_MGMT | \
			NDIS_PACKET_TYPE_802_11_ALL_MULTICAST_MGMT | \
			NDIS_PACKET_TYPE_802_11_BROADCAST_MGMT | \
			NDIS_PACKET_TYPE_802_11_PROMISCUOUS_MGMT | \
			NDIS_PACKET_TYPE_802_11_RAW_MGMT | \
			NDIS_PACKET_TYPE_802_11_DIRECTED_CTRL | \
			NDIS_PACKET_TYPE_802_11_BROADCAST_CTRL | \
			NDIS_PACKET_TYPE_802_11_PROMISCUOUS_CTRL



//
// (Ported from MS's code, Annie, 2006-10-11.) 
// Macros for assigning and verifying NDIS_OBJECT_HEADER
//
#define N6_ASSIGN_OBJECT_HEADER(_header, _type, _revision, _size) \
		(_header).Type = _type; \
		(_header).Revision = _revision; \
		(_header).Size = _size; 

//
// (Ported from MS's code, Annie, 2006-10-11.) 
// With NDIS 6.0 header versioning, the driver should allow higher versions
// of structures to be set. This macro verifies that for sets the version is atleast
// the expected one and size is greater or equal to required
//
#define N6_VERIFY_OBJECT_HEADER_DEFAULT(_header, _type, _revision, _size) \
		(((_header).Type == _type) && \
		((_header).Revision >= _revision) && \
		((_header).Size >= _size))
//


// For SW AP, see RtlFunc.h of the UI library, RtlLib.
// 2005.06.09, by rcnjko.
typedef struct _RtlLibAssociateEntry
{
	BOOLEAN			bUsed;
	UCHAR			Sum;
	UCHAR			MacAddr[6];
	ULONG			AuthAlg;		// 0:Open, 1:Shared
	UCHAR			AuthPassSeq;	// 2,4
	BOOLEAN			bAssociated;
	USHORT			AID;
	LARGE_INTEGER	LastActiveTime;
}RtlLibAssociateEntry, *PRtlLibAssociateEntry;

// For SW AP WPA-PSK.
// 2005.09.08, by rcnjko.
typedef struct _RtlLibPassphrase
{
	unsigned int	Length; // Valid passphrase length : 8 ~ 63 characters, 64 is reserved for hex representation of 32 byte PSK.
	char			Passphrase[64];
} RtlLibPassphrase, *PRtlLibPassphrase;

// Fix IE length, include timestamp(8), beacon interval(2), capability(2).
typedef struct _RT_FIXED_IE_FIELD
{
	u8Byte		Timestamp;
	u2Byte		BeaconInterval;
	u2Byte		Capability;
} RT_FIXED_IE_FIELD, *PRT_FIXED_IE_FIELD;

#pragma pack()

NDIS_STATUS
N6CQueryInformation(
	IN	PADAPTER		pAdapter,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesWritten,
	OUT	PULONG			BytesNeeded);

NDIS_STATUS
N6CSetInformation(
	IN	NDIS_HANDLE		MiniportAdapterContext,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesRead,
	OUT	PULONG			BytesNeeded);

NDIS_STATUS
N6CQuerySetInformation(
	IN	PADAPTER	Adapter,
	IN	NDIS_OID	Oid,
	IN	PVOID		InformationBuffer,
	IN	ULONG		InputBufferLength,
	IN	ULONG		OutputBufferLength,
	IN	ULONG		MethodId,
	OUT	PULONG		BytesWritten,
	OUT	PULONG		BytesRead,
	OUT	PULONG		BytesNeeded
	);


NDIS_STATUS
N6CProcessOidRequest(
	IN  PADAPTER	pAdapter,
	IN  PNDIS_OID_REQUEST   NdisRequest,
	IN  BOOLEAN bSelfMadeNdisRequest
);

	
BOOLEAN
N6CompletePendedOID(
	IN 	PADAPTER 			Adapter,
	IN	RT_PENDED_OID_TYPE	OidType,
	IN	NDIS_STATUS			ndisStatus
	);

NDIS_STATUS
N6C_OID_DOT11_FLUSH_BSS_LIST(
	PADAPTER	pAdapter,
	PNDIS_OID_REQUEST  NdisRequest
	);

NDIS_STATUS
N6C_OID_DOT11_SCAN_REQUEST(
	PADAPTER	pAdapter,
	PNDIS_OID_REQUEST  NdisRequest
	);

NDIS_STATUS
N6C_OID_DOT11_ENUM_BSS_LIST(
	PADAPTER	pAdapter,
	PNDIS_OID_REQUEST	NdisRequest);

#endif
