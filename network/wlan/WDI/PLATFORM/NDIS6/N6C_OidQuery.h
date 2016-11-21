#ifndef __INC_NDIS6_OID_QUERY_H
#define __INC_NDIS6_OID_QUERY_H

DOT11_AUTH_ALGORITHM
N6CAuthModeToDot11(
	IN	PVOID						pAuthMode
);

DOT11_AUTH_ALGORITHM
N6CAuthAlgToDot11(
	IN	PVOID						pAuthMode
);

DOT11_CIPHER_ALGORITHM
N6CEncAlgorithmToDot11(
	IN	PVOID						pEncAlgorithm
);

NDIS_STATUS
N6CQuery_DOT11_ENABLED_AUTHENTICATION_ALGORITHM(
	IN	PADAPTER					Adapter,
	OUT	PVOID							InformationBuffer,
	IN	ULONG							InformationBufferLength,
	OUT	PULONG							BytesWritten,
	OUT	PULONG							BytesNeeded
);

NDIS_STATUS
N6CQuery_DOT11_ENABLED_UNICAST_CIPHER_ALGORITHM(
	IN	PADAPTER						Adapter,
	OUT	PVOID							InformationBuffer,
	IN	ULONG							InformationBufferLength,
	OUT	PULONG							BytesWritten,
	OUT	PULONG							BytesNeeded
);

NDIS_STATUS
N6CQuery_DOT11_ENABLED_MULTICAST_CIPHER_ALGORITHM(
	IN	PADAPTER						Adapter,
	OUT	PVOID							InformationBuffer,
	IN	ULONG							InformationBufferLength,
	OUT	PULONG							BytesWritten,
	OUT	PULONG							BytesNeeded
);

NDIS_STATUS
N6CQuery_DOT11_SUPPORTED_UNICAST_ALGORITHM_PAIR(
	IN	PADAPTER						Adapter,
	OUT	PVOID							InformationBuffer,
	IN	ULONG							InformationBufferLength,
	OUT	PULONG							BytesWritten,
	OUT	PULONG							BytesNeeded
);

NDIS_STATUS
N6CQuery_DOT11_SUPPORTED_MULTICAST_ALGORITHM_PAIR(
	IN	PADAPTER						Adapter,
	OUT	PVOID							InformationBuffer,
	IN	ULONG							InformationBufferLength,
	OUT	PULONG							BytesWritten,
	OUT	PULONG							BytesNeeded
);

NDIS_STATUS
N6CQuery_DOT11_PMKID_LIST(
	IN	PADAPTER						Adapter,
	OUT	PDOT11_PMKID_LIST				pPMKIDList,
	IN	ULONG							TotalLength
);

NDIS_STATUS
N6CQuery_DOT11_EXTSTA_CAPABILITY(
	IN	PADAPTER						Adapter,
	OUT	PVOID							InformationBuffer,
	IN	ULONG							InformationBufferLength,
	OUT	PULONG							BytesWritten,
	OUT	PULONG							BytesNeeded
);

VOID
N6CQuery_DOT11_POWER_MGMT_REQUEST(
	IN	PADAPTER						Adapter,
	OUT	pu4Byte							pPowerSaveLevel
);
	
NDIS_STATUS
N6CQuery_DOT11_EXCLUDED_MAC_ADDRESS_LIST(
	IN	PADAPTER						Adapter,
	OUT	PVOID							InformationBuffer,
	IN	ULONG							InformationBufferLength,
	OUT	PULONG							BytesWritten,
	OUT	PULONG							BytesNeeded
);

NDIS_STATUS
N6CQuery_DOT11_DESIRED_BSSID_LIST(
	IN	PADAPTER						Adapter,
	OUT	PVOID							InformationBuffer,
	IN	ULONG							InformationBufferLength,
	OUT	PULONG							BytesWritten,
	OUT	PULONG							BytesNeeded
);

NDIS_STATUS
N6CQuery_DOT11_OPERATION_MODE_CAPABILITY(
	IN	PADAPTER						Adapter,
	OUT	PVOID							InformationBuffer,
	IN	ULONG							InformationBufferLength,
	OUT	PULONG							BytesWritten,
	OUT	PULONG							BytesNeeded
);

NDIS_STATUS
N6CQuery_DOT11_CURRENT_OPERATION_MODE(
	IN	PADAPTER						Adapter,
	OUT	PVOID							InformationBuffer,
	IN	ULONG							InformationBufferLength,
	OUT	PULONG							BytesWritten,
	OUT	PULONG							BytesNeeded	
);

NDIS_STATUS
N6CQuery_DOT11_DESIRED_SSID_LIST(
	IN	PADAPTER						Adapter,
	OUT	PVOID							InformationBuffer,
	IN	ULONG							InformationBufferLength,
	OUT	PULONG							BytesWritten,
	OUT	PULONG							BytesNeeded
);

NDIS_STATUS
N6CQuery_DOT11_STATISTICS(
	IN	PADAPTER						Adapter,
	OUT	PVOID							InformationBuffer,
	IN	ULONG							InformationBufferLength,
	OUT	PULONG							BytesWritten,
	OUT	PULONG							BytesNeeded
);

NDIS_STATUS
N6CQuery_DOT11_DESIRED_BSS_TYPE(
	IN	PADAPTER						Adapter,
	IN	PDOT11_BSS_TYPE				pBssType
);

NDIS_STATUS
N6CQuery_DOT11_PRIVACY_EXEMPTION_LIST(
	IN	PADAPTER						Adapter,
	OUT	PVOID							InformationBuffer,
	IN	ULONG							InformationBufferLength,
	OUT	PULONG							BytesWritten,
	OUT	PULONG							BytesNeeded
);

NDIS_STATUS
N6CQuery_DOT11_IBSS_PARAMS(
	IN	PADAPTER						Adapter,
	OUT	PVOID							InformationBuffer,
	IN	ULONG							InformationBufferLength,
	OUT	PULONG							BytesWritten,
	OUT	PULONG							BytesNeeded
);

NDIS_STATUS
N6CQuery_DOT11_ATIM_WINDOW(
    IN PADAPTER					pAdapter,
    OUT PULONG				pAtimWindow
);


u4Byte
N6CQuery_DOT11_OPERATING_PHYID(
	IN	PADAPTER						Adapter
);

u4Byte
N6CQuery_DOT11_RTBSS_OPERATING_PHYID(
	IN	PADAPTER						Adapter,
	IN	PVOID 					pRtBss
);
	
NDIS_STATUS
N6CQuery_DOT11_DESIRED_PHY_LIST(
	IN	PADAPTER						Adapter,
	OUT	PVOID							InformationBuffer,
	IN	ULONG							InformationBufferLength,
	OUT	PULONG							BytesWritten,
	OUT	PULONG							BytesNeeded
);

NDIS_STATUS
N6CQuery_DOT11_ACTIVE_PHY_LIST(
	IN	PADAPTER						Adapter,
	OUT	PVOID							InformationBuffer,
	IN	ULONG							InformationBufferLength,
	OUT	PULONG							BytesWritten,
	OUT	PULONG							BytesNeeded
);

NDIS_STATUS
N6CQuery_DOT11_OPTIONAL_CAPABILITY(
    IN PADAPTER					pAdapter,
    OUT PDOT11_OPTIONAL_CAPABILITY				pResult
);

NDIS_STATUS
N6CQuery_DOT11_CURRENT_OPTIONAL_CAPABILITY(
    IN PADAPTER					pAdapter,
    OUT PDOT11_OPTIONAL_CAPABILITY				pResult
);

NDIS_STATUS
N6CQuery_DOT11_CF_POLLABLE(
    IN PADAPTER					pAdapter,
    OUT PBOOLEAN				pResult
);

NDIS_STATUS
N6CQuery_DOT11_OPERATIONAL_RATE_SET(
	IN	PADAPTER							Adapter,
	OUT	PVOID								InformationBuffer,
	IN	ULONG								InformationBufferLength,
	OUT	PULONG								BytesWritten,
	OUT	PULONG								BytesNeeded
);

NDIS_STATUS
N6CQuery_DOT11_BEACON_PERIOD(
    IN PADAPTER					pAdapter,
    OUT PULONG				pResult
);

NDIS_STATUS
N6CQuery_DOT11_SHORT_RETRY_LIMIT(
    IN PADAPTER					pAdapter,
    OUT PULONG				pResult
);

NDIS_STATUS
N6CQuery_DOT11_LONG_RETRY_LIMIT(
    IN PADAPTER					pAdapter,
    OUT PULONG				pResult
);

NDIS_STATUS
N6CQuery_DOT11_MAX_TRANSMIT_MSDU_LIFETIME(
    IN PADAPTER				pAdapter,
    OUT PULONG				pResult
);

NDIS_STATUS
N6CQuery_DOT11_MAX_RECEIVE_LIFETIME(
    IN PADAPTER				pAdapter,
    OUT PULONG				pResult
);

NDIS_STATUS
N6CQuery_DOT11_SUPPORTED_PHY_TYPES(
	IN	PADAPTER						Adapter,
	OUT	PVOID							InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesWritten,
	OUT	PULONG			BytesNeeded
);

NDIS_STATUS
N6CQuery_DOT11_ENUM_ASSOCIATION_INFO(
	IN	PADAPTER						Adapter,
	OUT	PVOID							InformationBuffer,
	IN	ULONG							InformationBufferLength,
	OUT	PULONG							BytesWritten,
	OUT	PULONG							BytesNeeded
);

NDIS_STATUS
N6CQuery_DOT11_MULTICAST_LIST(
	IN	PADAPTER						Adapter,
	OUT	PVOID							InformationBuffer,
	IN	ULONG							InformationBufferLength,
	OUT	PULONG							BytesWritten,
	OUT	PULONG							BytesNeeded
);

BOOLEAN
N6CQuery_DOT11_NIC_POWER_STATE(
	IN	PADAPTER	pAdapter	
);

NDIS_STATUS
N6CQuery_DOT11_DataRateMappingTable(
	IN	PADAPTER						Adapter,
	OUT	PVOID							InformationBuffer,
	IN	ULONG							InformationBufferLength,
	OUT	PULONG							BytesWritten,
	OUT	PULONG							BytesNeeded
	);

NDIS_STATUS
N6CQuery_DOT11_Supported_DataRates(
	IN	PADAPTER						Adapter,
	OUT	PVOID							InformationBuffer,
	IN	ULONG							InformationBufferLength,
	OUT	PULONG							BytesWritten,
	OUT	PULONG							BytesNeeded
	);

NDIS_STATUS
N6CQuery_DOT11_Tx_Antenna(
	IN	PADAPTER						Adapter,
	OUT	PVOID							InformationBuffer,
	IN	ULONG							InformationBufferLength,
	OUT	PULONG							BytesWritten,
	OUT	PULONG							BytesNeeded
	);

NDIS_STATUS
N6C_QUERY_OID_DOT11_DESIRED_PHY_LIST(
	IN	PADAPTER		pTargetAdapter,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesWritten,
	OUT	PULONG			BytesNeeded
);

NDIS_STATUS
N6C_QUERY_OID_DOT11_AUTO_CONFIG_ENABLED(
	IN	PADAPTER		pTargetAdapter,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesWritten,
	OUT	PULONG			BytesNeeded
);

NDIS_STATUS
N6C_QUERY_OID_DOT11_BEACON_PERIOD(
	IN	PADAPTER		pTargetAdapter,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesWritten,
	OUT	PULONG			BytesNeeded
);

NDIS_STATUS
N6C_QUERY_OID_DOT11_DESIRED_SSID_LIST(
	IN	PADAPTER		pTargetAdapter,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesWritten,
	OUT	PULONG			BytesNeeded
);

NDIS_STATUS
N6C_QUERY_OID_GEN_CURRENT_PACKET_FILTER(
	IN	PADAPTER		pTargetAdapter,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesWritten,
	OUT	PULONG			BytesNeeded
);

NDIS_STATUS
N6C_QUERY_OID_DOT11_CURRENT_CHANNEL(
	IN	PADAPTER		pTargetAdapter,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesWritten,
	OUT	PULONG			BytesNeeded
);

NDIS_STATUS
N6C_QUERY_OID_GEN_LINK_PARAMETERS(
	IN	PADAPTER		pTargetAdapter,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesWritten,
	OUT	PULONG			BytesNeeded
);

NDIS_STATUS
N6C_QUERY_OID_DOT11_SAFE_MODE_ENABLED(
	IN	PADAPTER		pTargetAdapter,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesWritten,
	OUT	PULONG			BytesNeeded
);

NDIS_STATUS
N6C_QUERY_OID_DOT11_CURRENT_PHY_ID(
	IN	PADAPTER		pTargetAdapter,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesWritten,
	OUT	PULONG			BytesNeeded
);

NDIS_STATUS
N6C_QUERY_OID_GEN_SUPPORTED_LIST(
	IN	PADAPTER		pTargetAdapter,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesWritten,
	OUT	PULONG			BytesNeeded
);

NDIS_STATUS
N6C_QUERY_OID_GEN_VENDOR_DRIVER_VERSION(
	IN	PADAPTER		pTargetAdapter,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesWritten,
	OUT	PULONG			BytesNeeded
);

NDIS_STATUS
N6C_QUERY_OID_PNP_CAPABILITIES(
	IN	PADAPTER		pTargetAdapter,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesWritten,
	OUT	PULONG			BytesNeeded
);

NDIS_STATUS
N6C_QUERY_OID_PNP_QUERY_POWER(
	IN	PADAPTER		pTargetAdapter,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesWritten,
	OUT	PULONG			BytesNeeded
);

NDIS_STATUS
N6C_QUERY_OID_PNP_ENABLE_WAKE_UP(
	IN	PADAPTER		pTargetAdapter,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesWritten,
	OUT	PULONG			BytesNeeded
);
NDIS_STATUS
N6C_QUERY_OID_DOT11_CURRENT_TX_ANTENNA(
	IN	PADAPTER		pTargetAdapter,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesWritten,
	OUT	PULONG			BytesNeeded
);

NDIS_STATUS
N6C_QUERY_OID_DOT11_CURRENT_RX_ANTENNA(
	IN	PADAPTER		pTargetAdapter,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesWritten,
	OUT	PULONG			BytesNeeded
);

NDIS_STATUS
N6C_QUERY_OID_DOT11_SUPPORTED_RX_ANTENNA(
	IN	PADAPTER		pTargetAdapter,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesWritten,
	OUT	PULONG			BytesNeeded
);
#endif	// #ifndef __INC_NDIS6_OID_QUERY_H
