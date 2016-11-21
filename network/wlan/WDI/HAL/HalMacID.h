
#ifndef __INC_HAL_MAC_ID_H
#define __INC_HAL_MAC_ID_H

#define NUMBER_OF_HAL_MAC_ID_DESCRIPTOR		128

#define MAC_ID_OWNER_ASSOCIATED_CLIENT_AID_NO_USE 		0


// Static MacID Mapping (cf. Used in MacIdDoStaticMapping) ----------
#define MAC_ID_STATIC_FOR_DEFAULT_PORT				0x00
#define MAC_ID_STATIC_FOR_BROADCAST_MULTICAST		0x01
#define MAC_ID_STATIC_FOR_BT_CLIENT_START			0x02
#define MAC_ID_STATIC_FOR_BT_CLIENT_END				0x03
#define MAC_ID_STATIC_FOR_AP_MULTICAST				0x08
//TODO : temporily we use 0xff as UNSPECIFIED MACID
//       but if new IC support more MACID(more than 0xff)
//       We should enlarge macid data type from 1 byte->2byte
//       Otherwise there will be one MACID never send out. hsiao_ho
#define MAC_ID_STATIC_UNSPECIFIED_MACID				0xff

// -----------------------------------------------------------


typedef enum _MAC_ID_OWNER_TYPE {

	MAC_ID_OWNER_NOT_IN_USE = 0,

	// No Single Peer
	MAC_ID_OWNER_BROADCAST_MULTICAST,

	// Default Port 
	MAC_ID_OWNER_DEFAULT_PORT, 

	// BT High Speed Client
	MAC_ID_OWNER_BT,

	// TDLS Peer
	MAC_ID_OWNER_TDLS,

	// Native Ad-hoc Client or UI-based fake AP Client
	MAC_ID_OWNER_AD_HOC,

	// N62 Virtual Station / N63 P2P Client
	MAC_ID_OWNER_INFRA_STA,

	// N62 Virtual Station / N62 P2P Client for broadcast & multicast
	MAC_ID_OWNER_INFRA_STA_MULTICAST,
	
	// N62 Virtual AP / N63 P2P GO
	MAC_ID_OWNER_INFRA_AP,

	MAC_ID_OWNER_MAX	
} MAC_ID_OWNER_TYPE, *PMAC_ID_OWNER_TYPE;


typedef struct _MAC_ID_DESCRIPTOR {

	PADAPTER			pOwnerAdapter;					// Adapter which owns this descriptor
	MAC_ID_OWNER_TYPE	OwnerType;						// Descriptor owner type
	u4Byte 				OwnerAssociatedClientAID;		// Assign for its client if the owner is AP-like

} MAC_ID_DESCRIPTOR, *PMAC_ID_DESCRIPTOR;


typedef struct _HAL_MAC_ID_COMMON_CONTEXT{

	u1Byte MAC_ID_CONSTRAINT_ON_COUNT_FOR_EACH_OWNER[MAC_ID_OWNER_MAX];

	// Define the dynamically assigned MacID section
	u1Byte MacIdDynamicZoneStart;
	
	MAC_ID_DESCRIPTOR	MacIdDescriptor[NUMBER_OF_HAL_MAC_ID_DESCRIPTOR];
	
} HAL_MAC_ID_COMMON_CONTEXT, *PHAL_MAC_ID_COMMON_CONTEXT;

#define HAL_MAC_ID_SIZE_OF_COMMON_CONTEXT 	sizeof(HAL_MAC_ID_COMMON_CONTEXT)



VOID
MacIdInitializeCommonContext(
	PADAPTER pAdapter
);

VOID
MacIdDeInitializeCommonContext(
	PADAPTER pAdapter
);

VOID
MacIdInitializeMediaStatus(
	PADAPTER			pAdapter
);

VOID
MacIdIndicateMediaStatus(
	PADAPTER			pAdapter,
	u4Byte 				MacId,
	u4Byte 				MacIdEnd,
	RT_MEDIA_STATUS		MediaStatus
);

VOID
MacIdIndicateSpecificMediaStatus(
	PADAPTER			pAdapter,
	u4Byte 				MacId,
	RT_MEDIA_STATUS	MediaStatus
);

VOID
MacIdIndicateDisconnect(
	PADAPTER			pAdapter
);

VOID
MacIdRecoverMediaStatus(
	PADAPTER			pAdapter
);

VOID
MacIdDump(
	PADAPTER pAdapter
);

MAC_ID_OWNER_TYPE
MacIdGetOwnerType(
	PADAPTER 	pAdapter,
	u4Byte		MacId
);

u4Byte
MacIdGetTxMacId(
	PADAPTER	Adapter,
	PRT_TCB pTcb
);

u1Byte
MacIdHalGetMaxMacIdSupportNum(
	IN	PADAPTER		Adapter
);

u1Byte
MacIdHalGetMaxHWMacId(
	IN	PADAPTER		Adapter
);

VOID
MacIdDeregisterMacIdForInfraClient(
	PADAPTER	pAdapter,
	u4Byte		MacId
);

VOID
MacIdDeregisterSpecificMacId(
	PADAPTER pAdapter,
	u4Byte 	MacId
);

VOID
MacIdDeregisterForOwnerAdapter(
	PADAPTER pAdapter
);

u4Byte
MacIdRegisterMacIdForInfraClient(
	PADAPTER pAdapter
);

u4Byte
MacIdRegisterMacIdForMCastClient(
	PADAPTER pAdapter
);

u4Byte
MacIdRegisterMacIdForAssociatedID(
	PADAPTER pAdapter,
	MAC_ID_OWNER_TYPE	OwnerType,
	u4Byte AssociatedID
);

u4Byte
MacIdGetOwnerAssociatedClientAID(
	PADAPTER pAdapter,
	u4Byte MacId
);

u4Byte
MacIdRegisterMacIdForAPMcast(
	PADAPTER pAdapter
	);

MAC_ID_OWNER_TYPE
MacIdGetOwnerType(
	PADAPTER pAdapter,
	u4Byte MacId
);

#endif
