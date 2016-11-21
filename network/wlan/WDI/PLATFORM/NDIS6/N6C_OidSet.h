#ifndef __INC_NDIS6_OID_SET_H
#define __INC_NDIS6_OID_SET_H


/*
RT_AUTH_MODE
N6CDot11ToAuthMode(
	IN	DOT11_AUTH_ALGORITHM			dot11AuthAlg
);


RT_ENC_ALG
N6CDot11ToEncAlgorithm(
	IN	DOT11_CIPHER_ALGORITHM			dot11CipherAlg
);
*/


NDIS_STATUS
N6CSet_DOT11_ENABLED_AUTHENTICATION_ALGORITHM(
	IN	PADAPTER						Adapter,
	OUT	PVOID							InformationBuffer,
	IN	ULONG							InformationBufferLength,
	OUT	PULONG							BytesRead,
	OUT	PULONG							BytesNeeded
);


// Name : N6CSet_DOT11_AUTHENTICATION_ALOGORITHM
// Function : 
//		Set Authentication alg
//Input :
//      AlgorithmId : open , shared ,wpa ,wpa-psk, rsna,rsna-psk
VOID
N6CSet_DOT11_AUTHENTICATION_ALOGORITHM(
	IN	PADAPTER						Adapter,
	IN 	DOT11_AUTH_ALGORITHM       		AlgorithmId
);

NDIS_STATUS
N6CSet_DOT11_ENABLED_UNICAST_CIPHER_ALGORITHM(
	IN	PADAPTER						Adapter,
	OUT	PVOID							InformationBuffer,
	IN	ULONG							InformationBufferLength,
	OUT	PULONG							BytesRead,
	OUT	PULONG							BytesNeeded
);


// Name : N6CSet_DOT11_UNICAST_CIPHER_ALGORITHM
// Function : 
//		Set Unicase chiper alg
//Input :
//      AlgorithmId : none ,wep40 ,wep104 ,tkip ,aes.
VOID
N6CSet_DOT11_UNICAST_CIPHER_ALGORITHM(
	IN	PADAPTER						Adapter,
	IN 	DOT11_CIPHER_ALGORITHM  		AlgorithmId
);

NDIS_STATUS
N6CSet_DOT11_ENABLED_MULTICAST_CIPHER_ALGORITHM(
	IN	PADAPTER						Adapter,
	OUT	PVOID							InformationBuffer,
	IN	ULONG							InformationBufferLength,
	OUT	PULONG							BytesRead,
	OUT	PULONG							BytesNeeded
);

// Name : N6CSet_DOT11_MULTICAST_CIPHER_ALGORITHM
// Function : 
//		Set Unicase chiper alg
//Input :
//      AlgorithmId : none ,wep40 ,wep104 ,tkip ,aes.
VOID
N6CSet_DOT11_MULTICAST_CIPHER_ALGORITHM(
	IN	PADAPTER						Adapter,
	IN 	DOT11_CIPHER_ALGORITHM  		AlgorithmId
);


NDIS_STATUS
N6CSet_DOT11_CIPHER_DEFAULT_KEY(
	IN	PADAPTER							Adapter,
	OUT	PVOID							InformationBuffer,
	IN	ULONG							InformationBufferLength,
	OUT	PULONG							BytesRead,
	OUT	PULONG							BytesNeeded
);

NDIS_STATUS
N6CSet_DOT11_CIPHER_KEY_MAPPING_KEY(
	IN	PADAPTER						Adapter,
	OUT	PVOID							InformationBuffer,
	IN	ULONG							InformationBufferLength,
	OUT	PULONG							BytesRead,
	OUT	PULONG							BytesNeeded
);

NDIS_STATUS
N6CSet_DOT11_PMKID_LIST(
	IN	PADAPTER							Adapter,
	IN	PDOT11_PMKID_LIST					pPMKIDList,
	IN	ULONG								InfoBufLength
);

NDIS_STATUS
N6CSet_DOT11_CONNECT_REQUEST(
	IN	PADAPTER						Adapter,
	OUT	PVOID							InformationBuffer,
	IN	ULONG							InformationBufferLength,
	OUT	PULONG							BytesRead,
	OUT	PULONG							BytesNeeded
);

NDIS_STATUS
N6CSet_DOT11_DISCONNECT_REQUEST(
	IN	PADAPTER						Adapter,
	OUT	PVOID							InformationBuffer,
	IN	ULONG							InformationBufferLength,
	OUT	PULONG							BytesRead,
	OUT	PULONG							BytesNeeded
);

NDIS_STATUS
N6CSet_DOT11_POWER_MGMT_REQUEST(
	IN	PADAPTER						Adapter,
	IN	pu4Byte							pPowerSaveLevel
);

NDIS_STATUS
N6CSet_DOT11_EXCLUDED_MAC_ADDRESS_LIST(
	IN	PADAPTER						Adapter,
	OUT	PVOID							InformationBuffer,
	IN	ULONG							InformationBufferLength,
	OUT	PULONG							BytesRead,
	OUT	PULONG							BytesNeeded
);

NDIS_STATUS
N6CSet_DOT11_DESIRED_BSSID_LIST(
	IN	PADAPTER						Adapter,
	OUT	PVOID							InformationBuffer,
	IN	ULONG							InformationBufferLength,
	OUT	PULONG							BytesRead,
	OUT	PULONG							BytesNeeded
);

NDIS_STATUS
N6CSet_DOT11_CURRENT_OPERATION_MODE(
	IN	PADAPTER						Adapter,
	OUT	PVOID							InformationBuffer,
	IN	ULONG							InformationBufferLength,
	OUT	PULONG							BytesRead,
	OUT	PULONG							BytesNeeded
);

NDIS_STATUS
N6CSet_DOT11_DESIRED_BSS_TYPE(
	IN	PADAPTER						Adapter,
	IN	PDOT11_BSS_TYPE				pBssType
);

NDIS_STATUS
N6CSet_DOT11_SCAN_REQUEST(
	IN	PADAPTER						Adapter,
	OUT	PVOID							InformationBuffer,
	IN	ULONG							InformationBufferLength,
	OUT	PULONG							BytesRead,
	OUT	PULONG							BytesNeeded
);


NDIS_STATUS
N6CSet_DOT11_CUSTOMIZED_SCAN_REQUEST(
	IN	PADAPTER						Adapter,
	OUT	PVOID							InformationBuffer,
	IN	ULONG							InformationBufferLength,
	OUT	PULONG							BytesRead,
	OUT	PULONG							BytesNeeded
	);

NDIS_STATUS
N6CSet_DOT11_DESIRED_SSID_LIST(
	IN	PADAPTER						Adapter,
	OUT	PVOID							InformationBuffer,
	IN	ULONG							InformationBufferLength,
	OUT	PULONG							BytesRead,
	OUT	PULONG							BytesNeeded
);

NDIS_STATUS
N6CSet_DOT11_PRIVACY_EXEMPTION_LIST(
	IN	PADAPTER						Adapter,
	OUT	PVOID							InformationBuffer,
	IN	ULONG							InformationBufferLength,
	OUT	PULONG							BytesRead,
	OUT	PULONG							BytesNeeded
);

NDIS_STATUS
N6CSet_DOT11_IBSS_PARAMS(
	IN	PADAPTER						Adapter,
	OUT	PVOID							InformationBuffer,
	IN	ULONG							InformationBufferLength,
	OUT	PULONG							BytesRead,
	OUT	PULONG							BytesNeeded
);

NDIS_STATUS
N6CSet_DOT11_ATIM_WINDOW(
    IN PADAPTER					pAdapter,
    IN PULONG					pValue
);

NDIS_STATUS
N6CSet_DOT11_NIC_POWER_STATE(
    IN PADAPTER					pAdapter,
	IN PNDIS_OID_REQUEST		pNdisOidRequest,
    IN BOOLEAN					pValue
);

NDIS_STATUS
N6CSet_DOT11_OPERATIONAL_RATE_SET(
	IN	PADAPTER					pAdapter,
	IN	PVOID						InformationBuffer,
	IN	ULONG						InformationBufferLength,
	OUT	PULONG						BytesRead,
	OUT	PULONG						BytesNeeded
);

NDIS_STATUS
N6CSet_DOT11_DESIRED_PHY_LIST(
	IN	PADAPTER					Adapter,
	OUT	PVOID						InformationBuffer,
	IN	ULONG						InformationBufferLength,
	OUT	PULONG						BytesRead,
	OUT	PULONG						BytesNeeded
);

NDIS_STATUS
N6CQuerySet_DOT11_RESET_REQUEST(
	IN	PADAPTER	Adapter,
	OUT	PVOID		InformationBuffer,
	IN	ULONG		InputBufferLength,
	IN	ULONG		OutputBufferLength,
	OUT	PULONG		BytesWritten,
	OUT	PULONG		BytesRead,
	OUT	PULONG		BytesNeeded
);
	
NDIS_STATUS
N6CQuerySet_DOT11_ENUM_BSS_LIST(
	IN	PADAPTER	Adapter,
	OUT	PVOID		InformationBuffer,
	IN	ULONG		InputBufferLength,
	IN	ULONG		OutputBufferLength,
	OUT	PULONG		BytesWritten,
	OUT	PULONG		BytesNeeded
);

NDIS_STATUS
N6CSet_DOT11_MULTICAST_LIST(
	IN	PADAPTER	Adapter,
	IN	PVOID		InformationBuffer,
	IN	ULONG		InputBufferLength,
	OUT	PULONG		BytesRead,
    OUT PULONG		BytesNeeded
	);

NDIS_STATUS
N6C_OID_DOT11_NIC_POWER_STATE(
	PADAPTER pAdapter,
	PNDIS_OID_REQUEST NdisRequest);


NDIS_STATUS	
N6C_SET_OID_DOT11_DESIRED_PHY_LIST(
	IN	PADAPTER		pTargetAdapter,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesRead,
	OUT	PULONG			BytesNeeded
);

NDIS_STATUS	
N6C_SET_OID_DOT11_AUTO_CONFIG_ENABLED(
	IN	PADAPTER		pTargetAdapter,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesRead,
	OUT	PULONG			BytesNeeded
);

NDIS_STATUS	
N6C_SET_OID_DOT11_BEACON_PERIOD(
	IN	PADAPTER		pTargetAdapter,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesRead,
	OUT	PULONG			BytesNeeded
);

NDIS_STATUS	
N6C_SET_OID_DOT11_DTIM_PERIOD(
	IN	PADAPTER		pTargetAdapter,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesRead,
	OUT	PULONG			BytesNeeded
);

NDIS_STATUS	
N6C_SET_OID_DOT11_DESIRED_SSID_LIST(
	IN	PADAPTER		pTargetAdapter,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesRead,
	OUT	PULONG			BytesNeeded
);

NDIS_STATUS	
N6C_SET_OID_GEN_CURRENT_PACKET_FILTER(
	IN	PADAPTER		pTargetAdapter,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesRead,
	OUT	PULONG			BytesNeeded
);

NDIS_STATUS	
N6C_SET_OID_DOT11_CURRENT_CHANNEL(
	IN	PADAPTER		pTargetAdapter,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesRead,
	OUT	PULONG			BytesNeeded
);

NDIS_STATUS	
N6C_SET_OID_DOT11_DISCONNECT_REQUEST(
	IN	PADAPTER		pTargetAdapter,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesRead,
	OUT	PULONG			BytesNeeded
);

NDIS_STATUS	
N6C_SET_OID_DOT11_CONNECT_REQUEST(
	IN	PADAPTER		pTargetAdapter,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesRead,
	OUT	PULONG			BytesNeeded
);

NDIS_STATUS	
N6C_SET_OID_GEN_LINK_PARAMETERS(
	IN	PADAPTER		pTargetAdapter,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesRead,
	OUT	PULONG			BytesNeeded
);

NDIS_STATUS	
N6C_SET_OID_DOT11_SAFE_MODE_ENABLED(
	IN	PADAPTER		pTargetAdapter,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesRead,
	OUT	PULONG			BytesNeeded
);

NDIS_STATUS	
N6C_SET_OID_DOT11_CURRENT_PHY_ID(
	IN	PADAPTER		pTargetAdapter,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesRead,
	OUT	PULONG			BytesNeeded
);

NDIS_STATUS	
N6C_SET_OID_PNP_ENABLE_WAKE_UP(
	IN	PADAPTER		pTargetAdapter,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesRead,
	OUT	PULONG			BytesNeeded
);

NDIS_STATUS	
N6C_SET_OID_PNP_ADD_WAKE_UP_PATTERN(
	IN	PADAPTER		pTargetAdapter,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesRead,
	OUT	PULONG			BytesNeeded
);

NDIS_STATUS	
N6C_SET_OID_PNP_REMOVE_WAKE_UP_PATTERN(
	IN	PADAPTER		pTargetAdapter,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesRead,
	OUT	PULONG			BytesNeeded
);

NDIS_STATUS	
N6C_SET_OID_PNP_SET_POWER(
	IN	PADAPTER		pTargetAdapter,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesRead,
	OUT	PULONG			BytesNeeded
);
#endif	// #ifndef __INC_NDIS6_OID_SET_H
