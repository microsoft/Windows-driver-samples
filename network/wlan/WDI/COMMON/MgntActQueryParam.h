#ifndef __INC_MGNTACTQUERYPARAM_H
#define __INC_MGNTACTQUERYPARAM_H


BOOLEAN
MgntActQuery_802_11_ASSOCIATION_INFORMATION(
	PADAPTER		Adapter,
	PNDIS_802_11_ASSOCIATION_INFORMATION	          	pAssocInfo
);

// Returns the current AP MAC address
BOOLEAN
MgntActQuery_802_11_BSSID(
	PADAPTER		Adapter,
	pu1Byte          	bssidbuf
);


// Returns the SSID with which the NIC is associated. 
// The driver returns 0 SSIDLength if the NIC is not associated with any SSID.
BOOLEAN
MgntActQuery_802_11_SSID(
	PADAPTER		Adapter,
	pu1Byte          	ssidbuf,
	pu2Byte	          	ssidlen
);



s4Byte
MgntActQuery_TX_POWER_DBM(
	PADAPTER		Adapter
);

RT_JOIN_NETWORKTYPE
MgntActQuery_802_11_INFRASTRUCTURE_MODE(
	PADAPTER		Adapter
);


// Returns the current fragmentation threshold in bytes.
u2Byte
MgntActQuery_802_11_FRAGMENTATION_THRESHOLD(
	PADAPTER		Adapter
);

//Returns the current RTS threshold.
u2Byte
MgntActQuery_802_11_RTS_THRESHOLD(
	PADAPTER		Adapter
);

//Returns the set of supported data rates that the radio is capable of running.
BOOLEAN
MgntActQuery_802_11_SUPPORTED_RATES(
	PADAPTER		Adapter,
	pu1Byte	        RateSetbuf,
	pu2Byte			RateSetlen
);



BOOLEAN
MgntActQuery_802_11_BSSID_LIST(
	PADAPTER				Adapter,
	PRT_802_11_BSSID_LIST	pBssidList,
	BOOLEAN					bRealCase
	);


BOOLEAN
MgntActQuery_802_11_AUTHENTICATION_MODE(
	PADAPTER		Adapter,
	PRT_AUTH_MODE	pauthmode
);


BOOLEAN
MgntActQuery_802_11_ENCRYPTION_STATUS(
	PADAPTER		Adapter,
	PRT_ENC_ALG		pEncAlgorithm
);


BOOLEAN
MgntActQuery_802_11_ENCRYPTION_KEY(
	PADAPTER		Adapter,
	RT_ENC_ALG		EncAlgorithm,
	u4Byte			KeyIndex,
	pu4Byte			KeyLength,
	pu1Byte			KeyMaterial
);


u1Byte
MgntActQuery_802_11_CHANNEL_NUMBER(
	PADAPTER		Adapter
);

u2Byte   //Modify by Jacken 2008-05-12
MgntActQuery_RT_11N_USER_SHOW_RATES(
	PADAPTER		Adapter,
	BOOLEAN			TxorRx,
	BOOLEAN			bLinkStateRx
);

u2Byte
MgntActQuery_802_11_TX_RATES(
	PADAPTER		Adapter
);

u2Byte
MgntActQuery_802_11_RX_RATES(
	PADAPTER		Adapter
);


WIRELESS_MODE
MgntActQuery_802_11_WIRELESS_MODE(
	PADAPTER		Adapter
);


BOOLEAN
MgntActQuery_802_11_RETRY_LIMIT(
	PADAPTER Adapter, 
	pu2Byte ShortRetryLimit, 
	pu2Byte LongRetryLimit
);


 
BOOLEAN
MgntActQuery_MultiDomainImp(
	IN	PADAPTER		Adapter
);
 
BOOLEAN
MgntActQuery_CfPollable(
	IN	PADAPTER		Adapter
);

BOOLEAN
MgntActQuery_StrictlyOrderedImp(
	IN	PADAPTER		Adapter
);

u4Byte
MgntActQuery_ShortRetryLimit(
	IN	PADAPTER		Adapter
);

u4Byte
MgntActQuery_LongRetryLimit(
	IN	PADAPTER		Adapter
);

u4Byte
MgntActQuery_MaxTxMsduLifeTime(
	IN	PADAPTER		Adapter
);

u4Byte
MgntActQuery_MaxRxMpduLifeTime(
	IN	PADAPTER		Adapter
);

u4Byte
MgntActQuery_ExcludedMacAddressList(
	IN	PADAPTER		Adapter,
	OUT	pu1Byte			pMacAddrList,
	IN	u4Byte			BufLength
);

PRT_CHANNEL_LIST
MgntActQuery_ChannelList(
	IN	PADAPTER		Adapter
);

BOOLEAN
MgntActQuery_DrvLogTypeList(
	IN	PADAPTER				pAdapter,
	IN	u4Byte					BufferLength,
	OUT	PDRV_LOG_TYPE_LIST_T	pBuffer,
	OUT	pu4Byte					pBytesWritten,
	OUT pu4Byte					pBytesNeeded
);

BOOLEAN
MgntActQuery_DrvLogAttrList(
	IN	PADAPTER				pAdapter,
	IN	u4Byte					BufferLength,
	OUT	PDRV_LOG_ATTR_LIST_T	pBuffer,
	OUT	pu4Byte					pBytesWritten,
	OUT pu4Byte					pBytesNeeded
);

BOOLEAN
MgntActQuery_DrvLogDataList(
	IN	PADAPTER				pAdapter,
	IN	u4Byte					eLogType,
	IN	u4Byte					BufferLength,
	OUT	PDRV_LOG_DATA_LIST_T	pBuffer,
	OUT	pu4Byte					pBytesWritten,
	OUT pu4Byte					pBytesNeeded
);

BOOLEAN
MgntActQuery_AdditionalBeaconIE(
	IN 		PADAPTER	Adapter,
	OUT 	pu1Byte 		pAdditionalIEBuf,
	IN OUT 	pu4Byte		pAdditionalIEBufLen
);

BOOLEAN
MgntActQuery_AdditionalProbeRspIE(
	IN 		PADAPTER	Adapter,
	OUT 	pu1Byte 		pAdditionalIEBuf,
	IN OUT 	pu4Byte		pAdditionalIEBufLen
);

RT_AP_TYPE
MgntActQuery_ApType(
	IN PADAPTER pAdapter
);

P2P_ROLE
MgntActQuery_P2PMode(
	IN	PADAPTER		Adapter
	);

BOOLEAN
MgntActQuery_P2PScanList(
	IN		PADAPTER	Adapter,
	OUT 	pu1Byte 		pBuf,
	IN OUT 	pu4Byte		pBufLen
	);

u4Byte
MgntActQuery_P2PSelfDeviceDescriptor(
	IN PADAPTER Adapter,
	OUT PVOID pDevDesc
	);

RT_STATUS
MgntActQuery_P2PChannelList(
	IN PADAPTER Adapter,
	IN u4Byte ChannelListBufLen,
	OUT pu1Byte pChennelListLen,
	OUT pu1Byte pChannelList
	);

VOID
MgntActQuery_P2PListenChannel(
	IN PADAPTER Adapter,
	OUT pu1Byte pListenChannel
	);

VOID
MgntActQuery_P2PListenChannel(
	IN PADAPTER Adapter,
	OUT pu1Byte pListenChannel
	);

#if (WPS_SUPPORT == 1 )
u4Byte
MgntActQuery_WPS_Information(
	IN		PADAPTER	Adapter,
	OUT 	pu1Byte		InformationBuffer,
	IN OUT 	u4Byte		InformationBufferLength
	);
#endif

#endif // #ifndef __INC_MGNTACTQUERYPARAM_H
